#ifndef OBJECT_INFO_H
#define OBJECT_INFO_H

#include "raylib.h"
#include "world.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OBJINFO_SPRITE_NORMAL = 0
} ObjSpriteType;

typedef struct {
    uint32_t      id;
    const char   *name;

    Texture2D     texture;

    Vec3W         sprite_render_offset_from_position;
    Vec2S         sprite_texture_origin;

    Rectangle     src_rect;       // full rect when NORMAL
    ObjSpriteType sprite_type;
} ObjectInfo;

typedef struct {
    ObjectInfo *items;
    int         count;
    int         capacity;
} ObjectInfoRegistry;

/* ============================
   API
   ============================ */

// Parse TOML and load textures. Returns false on failure.
bool              objinfo_load_from_toml(ObjectInfoRegistry *reg, const char *path);

// Unload textures and free all memory.
void              objinfo_unload(ObjectInfoRegistry *reg);

// Lookup by numeric id. Returns NULL if not found.
const ObjectInfo *objinfo_by_id(const ObjectInfoRegistry *reg, uint32_t id);

#endif // OBJECT_INFO_H