// #include "object_info.h"
// #include <stdlib.h>
// #include <string.h>
// #include <stdio.h>
// #include "toml/tomlc17.h"   // tomlc17

// /* ============================
//    Helpers
//    ============================ */

// static uint32_t hash_str(const char *s)
// {
//     uint32_t h = 2166136261u;
//     while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
//     return h;
// }

// static void ensure_capacity(ObjectInfoRegistry *reg)
// {
//     if (reg->count < reg->capacity) return;
//     int new_cap = reg->capacity ? reg->capacity * 2 : 16;
//     reg->items = realloc(reg->items, new_cap * sizeof(ObjectInfo));
//     reg->capacity = new_cap;
// }

// static int toml_get_int(toml_table_t *t, const char *key, int def)
// {
//     toml_datum_t d = toml_int_in(t, key);
//     return d.ok ? (int)d.u.i : def;
// }

// static const char *toml_get_str(toml_table_t *t, const char *key)
// {
//     toml_datum_t d = toml_string_in(t, key);
//     return d.ok ? d.u.s : NULL;
// }

// /* ============================
//    API
//    ============================ */

// bool objinfo_load_from_toml(ObjectInfoRegistry *reg, const char *path)
// {
//     memset(reg, 0, sizeof(*reg));

//     FILE *fp = fopen(path, "r");
//     if (!fp) return false;

//     char errbuf[256];
//     toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
//     fclose(fp);
//     if (!root) return false;

//     int i = 0;
//     toml_array_t *arr = toml_array_in(root, "object");
//     if (!arr) { toml_free(root); return false; }

//     while (1)
//     {
//         toml_table_t *obj = toml_table_at(arr, i++);
//         if (!obj) break;

//         ensure_capacity(reg);
//         ObjectInfo *info = &reg->items[reg->count++];
//         memset(info, 0, sizeof(*info));

//         const char *name = toml_get_str(obj, "name");
//         const char *tex_path = toml_get_str(obj, "texture");

//         info->name = name ? strdup(name) : NULL;

//         int id = toml_get_int(obj, "id", -1);
//         info->id = (id >= 0) ? (uint32_t)id : hash_str(name);

//         info->sprite_type = OBJINFO_SPRITE_NORMAL;

//         if (tex_path)
//             info->texture = LoadTexture(tex_path);

//         // NORMAL → full rect
//         info->src_rect = (Rectangle){
//             0, 0,
//             (float)info->texture.width,
//             (float)info->texture.height
//         };

//         // Vec3W offset
//         toml_table_t *off3 = toml_table_in(obj, "sprite_render_offset_from_position");
//         if (off3)
//         {
//             info->sprite_render_offset_from_position.x = toml_get_int(off3, "x", 0);
//             info->sprite_render_offset_from_position.y = toml_get_int(off3, "y", 0);
//             info->sprite_render_offset_from_position.z = toml_get_int(off3, "z", 0);
//         }

//         // Vec2S origin
//         toml_table_t *off2 = toml_table_in(obj, "sprite_texture_origin");
//         if (off2)
//         {
//             info->sprite_texture_origin.x = toml_get_int(off2, "x", 0);
//             info->sprite_texture_origin.y = toml_get_int(off2, "y", 0);
//         }
//     }

//     toml_free(root);
//     return true;
// }

// void objinfo_unload(ObjectInfoRegistry *reg)
// {
//     for (int i = 0; i < reg->count; i++)
//     {
//         UnloadTexture(reg->items[i].texture);
//         free((void*)reg->items[i].name);
//     }
//     free(reg->items);
//     memset(reg, 0, sizeof(*reg));
// }

// const ObjectInfo *objinfo_by_id(const ObjectInfoRegistry *reg, uint32_t id)
// {
//     for (int i = 0; i < reg->count; i++)
//         if (reg->items[i].id == id)
//             return &reg->items[i];
//     return NULL;
// }