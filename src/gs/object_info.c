#include "gs/object_info.h"
#include "toml/tomlc17.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================
   Helpers
   ============================ */

static uint32_t hash_str(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

static void ensure_capacity(ObjectInfoRegistry *reg)
{
    if (reg->count < reg->capacity) return;
    int new_cap = reg->capacity ? reg->capacity * 2 : 16;
    reg->items  = realloc(reg->items, new_cap * sizeof(ObjectInfo));
    reg->capacity = new_cap;
}

// Read an int from a table datum by key, returning def on missing/wrong type.
static int datum_int(toml_datum_t tbl, const char *key, int def)
{
    toml_datum_t v = toml_get(tbl, key);
    return (v.type == TOML_INT64) ? (int)v.u.int64 : def;
}

/* ============================
   API
   ============================ */

bool objinfo_load_from_toml(ObjectInfoRegistry *reg, const char *path)
{
    memset(reg, 0, sizeof(*reg));

    toml_result_t result = toml_parse_file_ex(path);
    if (!result.ok) {
        fprintf(stderr, "objinfo: TOML parse error in '%s': %s\n",
                path, result.errmsg);
        return false;
    }

    // Top-level "object" array
    toml_datum_t arr = toml_get(result.toptab, "object");
    if (arr.type != TOML_ARRAY) {
        fprintf(stderr, "objinfo: no [[object]] array in '%s'\n", path);
        toml_free(result);
        return false;
    }

    for (int i = 0; i < arr.u.arr.size; i++) {
        toml_datum_t obj = arr.u.arr.elem[i];
        if (obj.type != TOML_TABLE) continue;

        ensure_capacity(reg);
        ObjectInfo *info = &reg->items[reg->count++];
        memset(info, 0, sizeof(*info));

        // id
        toml_datum_t id_d = toml_get(obj, "id");
        if (id_d.type == TOML_INT64) {
            info->id = (uint32_t)id_d.u.int64;
        } else {
            // fall back to hashing the name
            toml_datum_t name_d = toml_get(obj, "name");
            info->id = (name_d.type == TOML_STRING) ? hash_str(name_d.u.s) : 0;
        }

        // name
        toml_datum_t name_d = toml_get(obj, "name");
        info->name = (name_d.type == TOML_STRING) ? strdup(name_d.u.s) : NULL;

        // sprite_type
        toml_datum_t stype_d = toml_get(obj, "sprite_type");
        info->sprite_type = OBJINFO_SPRITE_NORMAL; // only type for now

        // texture
        toml_datum_t tex_d = toml_get(obj, "texture");
        if (tex_d.type == TOML_STRING) {
            info->texture = LoadTexture(tex_d.u.s);
        }

        // src_rect — full texture rect for NORMAL
        info->src_rect = (Rectangle){
            0, 0,
            (float)info->texture.width,
            (float)info->texture.height
        };

        // sprite_render_offset_from_position  { x, y, z }
        toml_datum_t off3 = toml_get(obj, "sprite_render_offset_from_position");
        if (off3.type == TOML_TABLE) {
            info->sprite_render_offset_from_position.x = datum_int(off3, "x", 0);
            info->sprite_render_offset_from_position.y = datum_int(off3, "y", 0);
            info->sprite_render_offset_from_position.z = datum_int(off3, "z", 0);
        }

        // sprite_texture_origin  { x, y }
        toml_datum_t off2 = toml_get(obj, "sprite_texture_origin");
        if (off2.type == TOML_TABLE) {
            info->sprite_texture_origin.sx = datum_int(off2, "x", 0);
            info->sprite_texture_origin.sy = datum_int(off2, "y", 0);
        }
    }

    toml_free(result);
    return true;
}

void objinfo_unload(ObjectInfoRegistry *reg)
{
    for (int i = 0; i < reg->count; i++) {
        UnloadTexture(reg->items[i].texture);
        free((void *)reg->items[i].name);
    }
    free(reg->items);
    memset(reg, 0, sizeof(*reg));
}

const ObjectInfo *objinfo_by_id(const ObjectInfoRegistry *reg, uint32_t id)
{
    for (int i = 0; i < reg->count; i++)
        if (reg->items[i].id == id)
            return &reg->items[i];
    return NULL;
}