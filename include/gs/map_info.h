#ifndef MAP_INFO_H
#define MAP_INFO_H

#include "world.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MAP_OBJ_STATIC = 0
} MapObjectType;

typedef struct {
    uint32_t      id;    // matches an ObjectInfo id
    MapObjectType type;
    Vec3W         pos;
} MapObject;

typedef struct {
    MapObject *objects;
    int        count;
    int        capacity;
} MapInfo;

/* ============================
   API
   ============================ */

// Parse TOML. Returns false on failure.
bool mapinfo_load_from_toml(MapInfo *map, const char *path);

// Free all memory.
void mapinfo_free(MapInfo *map);

#endif // MAP_INFO_H