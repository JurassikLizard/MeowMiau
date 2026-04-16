#ifndef OBJECT_H
#define OBJECT_H

#include "world.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OBJECT_GENERIC      = 0,
    OBJECT_FURNITURE,
    OBJECT_TOY,
    OBJECT_FOOD_BOWL,
    OBJECT_WATER_BOWL,
    OBJECT_CAT,
    OBJECT_TYPE_COUNT
} ObjectType;

typedef enum {
    OBJ_FLAG_NONE        = 0,
    OBJ_FLAG_SOLID       = 1 << 0,
    OBJ_FLAG_CLIMBABLE   = 1 << 1,
    OBJ_FLAG_INTERACTIVE = 1 << 2,
    OBJ_FLAG_HIDDEN      = 1 << 3,
    OBJ_FLAG_STATIC      = 1 << 4,
} ObjectFlags;

typedef struct Object {
    uint32_t     id;              /* unique object instance id in registry */
    uint32_t     obj_id;          /* object info registry id (type reference) */
    ObjectType   type;
    uint32_t     flags;

    Vec3W        position;         /* tile position, back-bottom corner  */
    Vec3W        size;           /* AABB extents in world tiles         */

    void        *userdata;
} Object;

/* ============================================================
 *  ObjectRegistry
 * ============================================================ */
#define OBJECT_POOL_INITIAL 64

typedef struct {
    Object  *items;
    int      count;
    int      capacity;
    uint32_t next_id;
    ObjectRenderer *obj_renderer; // non-owning pointer for rebaking after obj_kill
    // TODO: PROPERLY CALL REBAKE WHEN SPAWNING OBJ, ALONG WITH PROPERLY PARSING MAP
} ObjectRegistry;

bool    obj_registry_init   (ObjectRegistry *reg);
void    obj_registry_destroy(ObjectRegistry *reg);

Object *obj_spawn  (ObjectRegistry *reg, ObjectType type);
void    obj_kill   (ObjectRegistry *reg, uint32_t id);
Object *obj_by_id  (ObjectRegistry *reg, uint32_t id);

typedef void (*ObjIterFn)(Object *obj, void *ctx);
void    obj_foreach(ObjectRegistry *reg, ObjIterFn cb, void *ctx);

bool obj_aabb_overlaps_solid(ObjectRegistry *reg, const Object *obj);
void obj_snap_to_floor      (Object *obj);

static inline int obj_sort_key(const Object *o)
{
    Vec3W centre = {
        o->position.x + o->size.x / 2,
        o->position.y + o->size.y / 2,
        o->position.z
    };
    return world_sort_key(centre);
}

#endif /* OBJECT_H */