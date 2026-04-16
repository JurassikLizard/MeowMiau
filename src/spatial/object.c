#include "spatial/object.h"
#include <render/object_renderer.h>
#include <stdlib.h>
#include <string.h>

bool obj_registry_init(ObjectRegistry *reg, ObjectInfoRegistry *objinfo_reg)
{
    reg->items    = (Object *)calloc(OBJECT_POOL_INITIAL, sizeof(Object));
    if (!reg->items) return false;
    reg->count    = 0;
    reg->capacity = OBJECT_POOL_INITIAL;
    reg->next_id  = 1;
    reg->objinfo_reg = objinfo_reg;
    return true;
}

void obj_registry_destroy(ObjectRegistry *reg)
{
    if (reg && reg->items) {
        free(reg->items);
        reg->items    = NULL;
        reg->count    = 0;
        reg->capacity = 0;
    }
}

Object *obj_spawn(ObjectRegistry *reg, uint32_t obj_id)
{
    if (reg->count >= reg->capacity) {
        int new_cap = reg->capacity * 2;
        Object *buf = (Object *)realloc(reg->items, new_cap * sizeof(Object));
        if (!buf) return NULL;
        reg->items    = buf;
        reg->capacity = new_cap;
    }
    Object *o    = &reg->items[reg->count++];
    memset(o, 0, sizeof(Object));
    o->id        = reg->next_id++;
    o->obj_id    = obj_id;
    o->type      = OBJECT_GENERIC;  /* type determined by ObjectInfo now */

    return o;
}

void obj_kill(ObjectRegistry *reg, uint32_t id)
{
    for (int i = 0; i < reg->count; i++) {
        if (reg->items[i].id == id) {
            uint32_t killed_item_flags = reg->items[i].flags;
            reg->items[i] = reg->items[--reg->count];
            if (killed_item_flags & OBJ_FLAG_STATIC || reg->items[i].flags & OBJ_FLAG_STATIC) {
                // If the killed or swapped item is static, we need to rebake the object registry to update the renderer's baked static objects.
                bake_static_objects(reg);
            }
            return;
        }
    }
}

Object *obj_by_id(ObjectRegistry *reg, uint32_t id)
{
    for (int i = 0; i < reg->count; i++)
        if (reg->items[i].id == id) return &reg->items[i];
    return NULL;
}

void obj_foreach(ObjectRegistry *reg, ObjIterFn cb, void *ctx)
{
    for (int i = 0; i < reg->count; i++)
        cb(&reg->items[i], ctx);
}

bool obj_aabb_overlaps_solid(ObjectRegistry *reg, const Object *query)
{
    for (int i = 0; i < reg->count; i++) {
        const Object *o = &reg->items[i];
        if (o->id == query->id)           continue;
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;
        bool ox = (query->position.x < o->position.x + o->size.x) &&
                  (query->position.x + query->size.x > o->position.x);
        bool oy = (query->position.y < o->position.y + o->size.y) &&
                  (query->position.y + query->size.y > o->position.y);
        bool oz = (query->position.z < o->position.z + o->size.z) &&
                  (query->position.z + query->size.z > o->position.z);
        if (ox && oy && oz) return true;
    }
    return false;
}

void obj_snap_to_floor(Object *obj)
{
    obj->position.z = 0;
}