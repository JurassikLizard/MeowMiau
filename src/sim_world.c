#include "sim_world.h"
#include <string.h>

/* ============================================================
 *  sim_world_init
 * ============================================================ */
bool sim_world_init(SimWorld *w, int room_width, int room_depth,
                    int room_height)
{
    memset(w, 0, sizeof(*w));

    if (!room_init(&w->room, room_width, room_depth, room_height))
        return false;

    if (!obj_registry_init(&w->objects)) {
        room_destroy(&w->room);
        return false;
    }

    w->cat_id = 0;
    w->time   = 0.0f;
    return true;
}

/* ============================================================
 *  sim_world_populate_room
 * ============================================================ */
void sim_world_populate_room(SimWorld *w)
{
    room_generate(&w->room);
}

/* ============================================================
 *  sim_world_update
 * ============================================================ */
void sim_world_update(SimWorld *w, float dt)
{
    w->time += dt;

    /* TODO: tick subsystems in order, e.g.:
     *   needs_system_update(&w->needs, w->cat_id, dt);
     *   cat_brain_update   (&w->cat_brain, w, dt);
     *   physics_update     (&w->objects, dt);
     */
}

/* ============================================================
 *  sim_world_destroy
 * ============================================================ */
void sim_world_destroy(SimWorld *w)
{
    obj_registry_destroy(&w->objects);
    room_destroy(&w->room);
    memset(w, 0, sizeof(*w));
}

/* ============================================================
 *  sim_world_place_furniture
 * ============================================================ */
Object *sim_world_place_furniture(SimWorld *w, Vec3W origin,
                                  Vec3W size, int sprite_id)
{
    Object *o = obj_spawn(&w->objects, OBJECT_FURNITURE);
    if (!o) return NULL;

    o->origin    = origin;
    o->size      = size;
    o->sprite_id = sprite_id;
    o->flags     = OBJ_FLAG_SOLID | OBJ_FLAG_STATIC;

    /* Ensure the object sits on the floor (or on top of
     * something) rather than floating.                  */
    obj_snap_to_floor(o);

    return o;
}

/* ============================================================
 *  sim_world_spawn_cat
 * ============================================================ */
Object *sim_world_spawn_cat(SimWorld *w, Vec3W pos)
{
    Object *cat = obj_spawn(&w->objects, OBJECT_CAT);
    if (!cat) return NULL;

    cat->origin = pos;
    cat->size   = (Vec3W){ 1.0f, 1.0f, 1.0f };   /* 1×1×1 unit for now */
    cat->flags  = OBJ_FLAG_INTERACTIVE;

    w->cat_id = cat->id;

    /* TODO: allocate and attach CatState via cat->userdata */

    return cat;
}

/* ============================================================
 *  sim_world_objects_at_tile
 * ============================================================ */
int sim_world_objects_at_tile(SimWorld *w, int tx, int ty,
                              Object **out, int max)
{
    int found = 0;
    for (int i = 0; i < w->objects.count && found < max; i++) {
        Object *o = &w->objects.items[i];
        /* check if object footprint covers tile (tx, ty) */
        float ox0 = o->origin.x, ox1 = ox0 + o->size.x;
        float oy0 = o->origin.y, oy1 = oy0 + o->size.y;
        if ((float)tx + 1.0f > ox0 && (float)tx < ox1 &&
            (float)ty + 1.0f > oy0 && (float)ty < oy1) {
            out[found++] = o;
        }
    }
    return found;
}