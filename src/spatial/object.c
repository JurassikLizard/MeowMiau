#include "spatial/object.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 *  Registry lifecycle
 * ============================================================ */

bool obj_registry_init(ObjectRegistry *reg)
{
    reg->items    = (Object *)calloc(OBJECT_POOL_INITIAL, sizeof(Object));
    if (!reg->items) return false;
    reg->count    = 0;
    reg->capacity = OBJECT_POOL_INITIAL;
    reg->next_id  = 1;   /* 0 is reserved as "null id" */
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

/* ============================================================
 *  obj_spawn
 *  Appends a zeroed Object, assigns a unique id, returns ptr.
 * ============================================================ */
Object *obj_spawn(ObjectRegistry *reg, ObjectType type)
{
    /* grow if needed */
    if (reg->count >= reg->capacity) {
        int new_cap = reg->capacity * 2;
        Object *buf = (Object *)realloc(reg->items, new_cap * sizeof(Object));
        if (!buf) return NULL;
        reg->items    = buf;
        reg->capacity = new_cap;
    }

    Object *o  = &reg->items[reg->count++];
    memset(o, 0, sizeof(Object));
    o->id        = reg->next_id++;
    o->type      = type;
    o->sprite_id = -1;
    return o;
}

/* ============================================================
 *  obj_kill
 *  Swap-remove by id (order changes – callers must not cache
 *  raw pointers across a kill).
 * ============================================================ */
void obj_kill(ObjectRegistry *reg, uint32_t id)
{
    for (int i = 0; i < reg->count; i++) {
        if (reg->items[i].id == id) {
            reg->items[i] = reg->items[--reg->count];
            return;
        }
    }
}

/* ============================================================
 *  obj_by_id
 * ============================================================ */
Object *obj_by_id(ObjectRegistry *reg, uint32_t id)
{
    for (int i = 0; i < reg->count; i++)
        if (reg->items[i].id == id) return &reg->items[i];
    return NULL;
}

/* ============================================================
 *  obj_foreach
 * ============================================================ */
void obj_foreach(ObjectRegistry *reg, ObjIterFn cb, void *ctx)
{
    for (int i = 0; i < reg->count; i++)
        cb(&reg->items[i], ctx);
}

/* ============================================================
 *  obj_aabb_overlaps_solid  (stub)
 *
 *  TODO: replace the O(n) scan with a spatial hash once the
 *  scene grows beyond ~100 objects.
 * ============================================================ */
bool obj_aabb_overlaps_solid(ObjectRegistry *reg, const Object *query)
{
    for (int i = 0; i < reg->count; i++) {
        const Object *o = &reg->items[i];
        if (o->id == query->id)         continue;
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;

        /* AABB overlap test on x and y; ignore z for floor placement */
        bool ox = (query->origin.x < o->origin.x + o->size.x) &&
                  (query->origin.x + query->size.x > o->origin.x);
        bool oy = (query->origin.y < o->origin.y + o->size.y) &&
                  (query->origin.y + query->size.y > o->origin.y);
        bool oz = (query->origin.z < o->origin.z + o->size.z) &&
                  (query->origin.z + query->size.z > o->origin.z);
        if (ox && oy && oz) return true;
    }
    return false;
}

/* ============================================================
 *  obj_snap_to_floor  (stub)
 *
 *  Currently just zeros z.  A fuller version would raycast
 *  downward and land on top of furniture below the object.
 * ============================================================ */
void obj_snap_to_floor(Object *obj)
{
    obj->origin.z = 0.0f;
    /* TODO: check for furniture below and adjust z accordingly */
}