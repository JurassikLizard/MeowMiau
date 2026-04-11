#include "render/object_renderer.h"
#include <stdlib.h>
#include <string.h>

/* ============================
   Internal helpers
   ============================ */

// TODO: replace with your actual lookup system
static Texture2D *get_object_texture(const Object *o)
{
    (void)o;
    return NULL;
}

static Rectangle get_object_src(const Object *o)
{
    // For now just get full texture
    (void)o;
    return (Rectangle){0,0,0,0};
}

static int baked_cmp(const void *a, const void *b)
{
    const BakedObject *A = a;
    const BakedObject *B = b;
    return (A->depth - B->depth);
}

static bool ensure_capacity(StaticBakeCache *c, size_t needed)
{
    if (needed <= c->capacity) return true;

    size_t new_cap = c->capacity ? c->capacity * 2 : 64;
    if (new_cap < needed) new_cap = needed;

    BakedObject *n = realloc(c->objects, new_cap * sizeof(BakedObject));
    if (!n) return false;

    c->objects = n;
    c->capacity = new_cap;
    return true;
}

/* ============================
   Lifecycle
   ============================ */

bool object_renderer_init(ObjectRenderer *or, Renderer *r)
{
    or->static_cache.objects = NULL;
    or->static_cache.count = 0;
    or->static_cache.capacity = 0;
    or->renderer = r;
    return true;
}

void object_renderer_destroy(ObjectRenderer *or)
{
    if (or->static_cache.objects)
    {
        free(or->static_cache.objects);
        or->static_cache.objects = NULL;
    }
    or->static_cache.count = 0;
    or->static_cache.capacity = 0;
}

/* ============================
   Bake
   ============================ */

bool bake_static_objects(ObjectRenderer *or, const ObjectRegistry *reg)
{
    StaticBakeCache *cache = &or->static_cache;
    cache->count = 0;

    if (!reg || reg->count == 0) return true;

    if (!ensure_capacity(cache, reg->count))
        return false;

    for (int i = 0; i < reg->count; i++)
    {
        const Object *o = &reg->items[i];

        if (!(o->flags & OBJ_FLAG_STATIC)) continue;
        if (o->flags & OBJ_FLAG_HIDDEN)   continue;

        BakedObject *bo = &cache->objects[cache->count++];
        bo->id = o->id;

        // depth
        bo->depth = obj_sort_key(o);

        // texture
        bo->texture = get_object_texture(o);
        bo->src     = get_object_src(o);

        Vec3W world_pos = {
            o->position.x + bo->sprite_render_offset_from_position.x,
            o->position.y + bo->sprite_render_offset_from_position.y,
            o->position.z + bo->sprite_render_offset_from_position.z
        };
        Vec2S pos_at_render_offset = renderer_project(or->renderer, world_pos);

        Vec2S render_pos = {
            pos_at_render_offset.sx - bo->sprite_texture_origin.sx,
            pos_at_render_offset.sy - bo->sprite_texture_origin.sy
        };

        // screen dest (deferred projection → use origin directly for now)
        // final projection happens implicitly via renderer_push usage
        bo->dest = (Rectangle){
            render_pos.sx, 
            render_pos.sy,
            bo->src.width,
            bo->src.height
        };

        bo->flags = o->flags;
    }

    return true;
}

/* ============================
   Push per-frame
   ============================ */

void object_renderer_push_static(ObjectRenderer *or)
{
    const StaticBakeCache *cache = &or->static_cache;

    for (size_t i = 0; i < cache->count; i++)
    {
        const BakedObject *bo = &cache->objects[i];

        if (!bo->texture) continue;

        Vector2 dst = { bo->dest.x, bo->dest.y };

        renderer_push(
            or->renderer,
            bo->texture,
            bo->src,
            dst,
            bo->depth,
            WHITE
        );
    }
}