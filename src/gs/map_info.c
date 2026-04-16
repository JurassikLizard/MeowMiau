#include "gs/map_info.h"
#include "toml/tomlc17.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================
   Helpers
   ============================ */

static void ensure_capacity(MapInfo *m)
{
    if (m->count < m->capacity) return;
    int new_cap   = m->capacity ? m->capacity * 2 : 16;
    m->objects    = realloc(m->objects, new_cap * sizeof(MapObject));
    m->capacity   = new_cap;
}

static int datum_int(toml_datum_t tbl, const char *key, int def)
{
    toml_datum_t v = toml_get(tbl, key);
    return (v.type == TOML_INT64) ? (int)v.u.int64 : def;
}

/* ============================
   API
   ============================ */

bool mapinfo_load_from_toml(MapInfo *map, const char *path)
{
    memset(map, 0, sizeof(*map));

    toml_result_t result = toml_parse_file_ex(path);
    if (!result.ok) {
        fprintf(stderr, "mapinfo: TOML parse error in '%s': %s\n",
                path, result.errmsg);
        return false;
    }

    toml_datum_t arr = toml_get(result.toptab, "object");
    if (arr.type != TOML_ARRAY) {
        fprintf(stderr, "mapinfo: no [[object]] array in '%s'\n", path);
        toml_free(result);
        return false;
    }

    for (int i = 0; i < arr.u.arr.size; i++) {
        toml_datum_t obj = arr.u.arr.elem[i];
        if (obj.type != TOML_TABLE) continue;

        ensure_capacity(map);
        MapObject *mo = &map->objects[map->count++];
        memset(mo, 0, sizeof(*mo));

        // id — references an ObjectInfo id
        mo->id = (uint32_t)datum_int(obj, "id", 0);

        // type
        toml_datum_t type_d = toml_get(obj, "type");
        mo->type = MAP_OBJ_STATIC; // default; extend here for future types
        if (type_d.type == TOML_STRING) {
            // currently only STATIC is defined; structure is ready to expand
            (void)type_d.u.s;
        }

        // pos { x, y, z }
        toml_datum_t pos = toml_get(obj, "pos");
        if (pos.type == TOML_TABLE) {
            mo->pos.x = datum_int(pos, "x", 0);
            mo->pos.y = datum_int(pos, "y", 0);
            mo->pos.z = datum_int(pos, "z", 0);
        }
    }

    toml_free(result);
    return true;
}

void mapinfo_free(MapInfo *map)
{
    free(map->objects);
    memset(map, 0, sizeof(*map));
}