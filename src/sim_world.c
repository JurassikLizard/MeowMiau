#include "sim_world.h"
#include <string.h>

bool sim_world_init(SimWorld *w, int room_width, int room_depth,
                    int room_height, ObjectInfoRegistry *objinfo_reg)
{
    memset(w, 0, sizeof(*w));
    if (!room_init(&w->room, room_width, room_depth, room_height))
        return false;
    if (!obj_registry_init(&w->objects, objinfo_reg)) {
        room_destroy(&w->room);
        return false;
    }
    return true;
}

void sim_world_populate_room(SimWorld *w) { room_generate(&w->room); }

void sim_world_update(SimWorld *w, float dt) { w->time += dt; }

void sim_world_destroy(SimWorld *w)
{
    obj_registry_destroy(&w->objects);
    room_destroy(&w->room);
    memset(w, 0, sizeof(*w));
}

Object *sim_world_place_furniture(SimWorld *w, Vec3W position,
                                  Vec3W size, int sprite_id)
{
    Object *o = obj_spawn(&w->objects, sprite_id);
    if (!o) return NULL;
    o->position    = position;
    o->size      = size;
    o->flags     = OBJ_FLAG_SOLID | OBJ_FLAG_STATIC;
    obj_snap_to_floor(o);
    return o;
}

Object *sim_world_spawn_cat(SimWorld *w, Vec3W pos)
{
    /* TODO: Get CAT obj_id from config/ObjectInfoRegistry */
    uint32_t cat_obj_id = 2;  /* placeholder; should be looked up from registry */
    Object *cat = obj_spawn(&w->objects, cat_obj_id);
    if (!cat) return NULL;
    cat->position = pos;
    cat->size   = (Vec3W){ 1, 1, 1 };
    cat->flags  = OBJ_FLAG_INTERACTIVE;
    w->cat_id   = cat->id;
    return cat;
}

int sim_world_objects_at_tile(SimWorld *w, int tx, int ty,
                              Object **out, int max)
{
    int found = 0;
    for (int i = 0; i < w->objects.count && found < max; i++) {
        Object *o = &w->objects.items[i];
        if (tx + 1 > o->position.x && tx < o->position.x + o->size.x &&
            ty + 1 > o->position.y && ty < o->position.y + o->size.y)
            out[found++] = o;
    }
    return found;
}