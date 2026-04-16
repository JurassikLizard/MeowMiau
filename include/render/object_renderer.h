#ifndef OBJECT_RENDERER_H
#define OBJECT_RENDERER_H

#include "render/renderer.h"
#include "spatial/object.h"
#include "world.h"

typedef struct {
    int id;               // from Object.id, for rebaking after obj_kill
    uint32_t obj_id;      // from Object.obj_id, object info registry id

    // depth for sorting
    int depth;

    // sprite / texture info
    Texture2D *texture;   // or atlas index
    Rectangle src;        // source rect in texture
    Rectangle dest;       // final draw rect (screen space)

    Vec3W      sprite_render_offset_from_position;
    Vec2S      sprite_texture_origin;

    // flags (flip, tint, etc)
    unsigned int flags;
} BakedObject;

typedef struct {
    BakedObject *objects;
    size_t count;
    size_t capacity;
} StaticBakeCache;

typedef struct {
    StaticBakeCache static_cache;
    Renderer *renderer;  // non-owning pointer for projection during bake
} ObjectRenderer;

bool object_renderer_init(Renderer *r, ObjectRenderer *or, ObjectRegistry *reg);
void object_renderer_destroy(ObjectRenderer *or);

/// @brief  Bakes all static objects from the registry into the renderer, get pushed to renderer by `object_renderer_push_static`. Should be called after an obj_kill that changes ids of static objects, to rebake the object registry.
/// @param reg 
/// @return Returns true on successful bake, false on failure (e.g. failure to project perspective or find objects).
bool bake_static_objects(const ObjectRegistry *reg);

/// @brief Called once per frame to push all static objects into the renderer. Should be called after `bake_static_objects` has been called at least once.
/// @param or 
void object_renderer_push_static(ObjectRegistry *reg);


#endif /* OBJECT_RENDERER_H */