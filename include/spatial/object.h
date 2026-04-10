#ifndef OBJECT_H
#define OBJECT_H

/*  ============================================================
 *  OBJECT SYSTEM
 *
 *  An "object" is any non-tile entity that occupies world space:
 *  furniture, interactive props, the cat, food bowls, toys, etc.
 *
 *  Design decisions
 *  ----------------
 *  • Objects live in *continuous* world space (Vec3W), not the
 *    integer tile grid.  This lets the cat walk smoothly and
 *    lets furniture be nudged to sub-tile positions if desired.
 *
 *  • Each object has an "origin" – the world point that
 *    corresponds to the (0,0) of its sprite/hitbox.  By
 *    convention the origin is the front-bottom corner of the
 *    object's bounding box.
 *
 *  • size.x / size.y / size.z are in world units and define
 *    the axis-aligned bounding box (AABB) starting from origin.
 *
 *  • sprite_id is an index into the game's sprite registry
 *    (defined separately).  The renderer uses it to look up
 *    the atlas region.  -1 means "no sprite" (invisible marker).
 *
 *  • sprite_offset is in screen pixels and shifts the sprite
 *    relative to the projected origin point.  This lets artists
 *    draw sprites that don't align exactly with the tile grid.
 *
 *  • flags is a bitmask of ObjectFlags (see below).
 *
 *  • type distinguishes broad categories so subsystems can
 *    filter quickly (e.g. the hunger system only cares about
 *    OBJECT_FOOD_BOWL).
 *  ============================================================ */

#include "world.h"
#include <stdint.h>
#include <stdbool.h>

/* ---- object categories ---- */
typedef enum {
    OBJECT_GENERIC      = 0,
    OBJECT_FURNITURE,           /* sofa, desk, shelf, litter box … */
    OBJECT_TOY,                 /* ball, feather wand, laser dot … */
    OBJECT_FOOD_BOWL,
    OBJECT_WATER_BOWL,
    OBJECT_CAT,                 /* the cat itself                  */
    OBJECT_TYPE_COUNT
} ObjectType;

/* ---- per-object behaviour flags ---- */
typedef enum {
    OBJ_FLAG_NONE       = 0,
    OBJ_FLAG_SOLID      = 1 << 0,   /* blocks cat movement          */
    OBJ_FLAG_CLIMBABLE  = 1 << 1,   /* cat can jump/walk on top     */
    OBJ_FLAG_INTERACTIVE= 1 << 2,   /* cat can interact with it     */
    OBJ_FLAG_HIDDEN     = 1 << 3,   /* skip rendering               */
    OBJ_FLAG_STATIC     = 1 << 4,   /* never moves (optimisation)   */
} ObjectFlags;

/* ---- screen-pixel sprite offset ---- */
typedef struct { int dx, dy; } SpriteOffset;

/* ---- the object descriptor ---- */
typedef struct Object {
    /* identity */
    uint32_t    id;             /* unique handle                   */
    ObjectType  type;
    uint32_t    flags;          /* OR of ObjectFlags               */

    /* world-space geometry */
    Vec3W       origin;         /* front-bottom-left corner        */
    Vec3W       size;           /* bounding box extents            */

    /* rendering */
    int         sprite_id;      /* index into sprite registry      */
    SpriteOffset sprite_offset; /* pixel nudge from projected origin*/

    /* ---- extension pointer ----
     * Subsystems hang their own data here.
     * Furniture:  FurnitureData *
     * Cat:        CatState *
     * Food bowl:  FoodBowlData *
     * etc.
     * The object system does NOT own this memory; the subsystem
     * that allocates it is responsible for freeing it.           */
    void        *userdata;
} Object;

/* ============================================================
 *  OBJECT REGISTRY
 *
 *  A simple growable pool of objects owned by the simulation.
 *  All live objects are stored here and iterated each frame.
 * ============================================================ */

#define OBJECT_POOL_INITIAL 64

typedef struct {
    Object  *items;
    int      count;
    int      capacity;
    uint32_t next_id;
} ObjectRegistry;

/* lifecycle */
bool     obj_registry_init   (ObjectRegistry *reg);
void     obj_registry_destroy(ObjectRegistry *reg);

/* add / remove */
Object  *obj_spawn (ObjectRegistry *reg, ObjectType type);
void     obj_kill  (ObjectRegistry *reg, uint32_t id);

/* lookup */
Object  *obj_by_id (ObjectRegistry *reg, uint32_t id);

/* iteration helper – call cb(obj, userdata) for every live object */
typedef void (*ObjIterFn)(Object *obj, void *ctx);
void     obj_foreach(ObjectRegistry *reg, ObjIterFn cb, void *ctx);

/* ============================================================
 *  COLLISION / PLACEMENT HELPERS  (stubs)
 * ============================================================ */

/* Returns true if the AABB of `obj` overlaps any SOLID object
 * in `reg` other than itself.                                   */
bool obj_aabb_overlaps_solid(ObjectRegistry *reg, const Object *obj);

/* Snap an object's origin so it sits flush on the floor (z=0)
 * or on top of another object directly beneath it.             */
void obj_snap_to_floor(Object *obj);

/* ============================================================
 *  SORT KEY for painter's algorithm
 *
 *  Use the centre of the object's base footprint.              */
static inline float obj_sort_key(const Object *o)
{
    Vec3W centre = {
        o->origin.x + o->size.x * 0.5f,
        o->origin.y + o->size.y * 0.5f,
        o->origin.z
    };
    return world_sort_key(centre);
}

#endif /* OBJECT_H */