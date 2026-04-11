// #include "map_info.h"
// #include <stdlib.h>
// #include <string.h>
// #include <stdio.h>
// #include "toml/tomlc17.h"

// /* ============================
//    Helpers
//    ============================ */

// static void ensure_capacity(MapInfo *m)
// {
//     if (m->count < m->capacity) return;
//     int new_cap = m->capacity ? m->capacity * 2 : 16;
//     m->objects = realloc(m->objects, new_cap * sizeof(MapObject));
//     m->capacity = new_cap;
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

// bool mapinfo_load_from_toml(MapInfo *map, const char *path)
// {
//     memset(map, 0, sizeof(*map));

//     FILE *fp = fopen(path, "r");
//     if (!fp) return false;

//     char errbuf[256];
//     toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
//     fclose(fp);
//     if (!root) return false;

//     toml_array_t *arr = toml_array_in(root, "object");
//     if (!arr) { toml_free(root); return false; }

//     int i = 0;
//     while (1)
//     {
//         toml_table_t *obj = toml_table_at(arr, i++);
//         if (!obj) break;

//         ensure_capacity(map);
//         MapObject *mo = &map->objects[map->count++];
//         memset(mo, 0, sizeof(*mo));

//         mo->id = (uint32_t)toml_get_int(obj, "id", 0);

//         const char *type = toml_get_str(obj, "type");
//         if (type && strcmp(type, "STATIC") == 0)
//             mo->type = MAP_OBJ_STATIC;

//         toml_table_t *pos = toml_table_in(obj, "pos");
//         if (pos)
//         {
//             mo->pos.x = toml_get_int(pos, "x", 0);
//             mo->pos.y = toml_get_int(pos, "y", 0);
//             mo->pos.z = toml_get_int(pos, "z", 0);
//         }
//     }

//     toml_free(root);
//     return true;
// }

// void mapinfo_free(MapInfo *map)
// {
//     free(map->objects);
//     memset(map, 0, sizeof(*map));
// }