#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include "spatial/room.h"
#include "spatial/object.h"

typedef struct {
    Room           room;
    ObjectRegistry objects;
    uint32_t       cat_id;   /* 0 = no cat spawned */
    float          time;     /* seconds since sim start */
} SimWorld;

bool    sim_world_init         (SimWorld *w, int room_width, int room_depth,
                                int room_height);
void    sim_world_populate_room(SimWorld *w);
void    sim_world_update       (SimWorld *w, float dt);
void    sim_world_destroy      (SimWorld *w);

Object *sim_world_place_furniture(SimWorld *w, Vec3W origin, Vec3W size,
                                  int sprite_id);
Object *sim_world_spawn_cat      (SimWorld *w, Vec3W pos);

static inline Object *sim_world_get_cat(SimWorld *w)
{
    return obj_by_id(&w->objects, w->cat_id);
}

int sim_world_objects_at_tile(SimWorld *w, int tx, int ty,
                              Object **out, int max);

#endif /* SIM_WORLD_H */