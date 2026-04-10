#ifndef SIM_WORLD_H
#define SIM_WORLD_H

/*  ============================================================
 *  sim_world.h  –  top-level simulation context
 *
 *  SimWorld is the single "god object" the game loop holds on
 *  to.  It owns:
 *    • the Room          (tile map + dimensions)
 *    • the ObjectRegistry (all furniture, cat, toys, …)
 *    • future subsystems are added here, never as globals
 *
 *  Usage sketch:
 *
 *    SimWorld w;
 *    sim_world_init(&w, 10, 8, 5);      // 10×8 floor, 5 tall
 *    sim_world_populate_room(&w);        // fill tiles
 *
 *    // place a sofa
 *    Object *sofa = sim_world_place_furniture(
 *        &w,
 *        (Vec3W){2.0f, 3.0f, 0.0f},    // origin
 *        (Vec3W){2.0f, 1.0f, 1.5f},    // size
 *        SPRITE_SOFA                    // sprite id
 *    );
 *
 *    // spawn the cat
 *    w.cat_id = sim_world_spawn_cat(&w, (Vec3W){5.0f, 4.0f, 0.0f});
 *
 *    // game loop
 *    while (!WindowShouldClose()) {
 *        float dt = GetFrameTime();
 *        sim_world_update(&w, dt);
 *        // render…
 *    }
 *
 *    sim_world_destroy(&w);
 *  ============================================================ */

#include "spatial/room.h"
#include "spatial/object.h"

/* ============================================================
 *  SimWorld
 * ============================================================ */
typedef struct {
    Room           room;
    ObjectRegistry objects;

    /* Convenience handle to the (single) cat object.
     * 0 means no cat has been spawned yet.              */
    uint32_t       cat_id;

    /* Wall-clock time since simulation start (seconds).
     * Updated by sim_world_update().                    */
    float          time;

    /* ---- extension point ----
     * Add subsystem pointers here as the simulation grows,
     * e.g.:
     *   CatBrain     *cat_brain;
     *   NeedsSystem  *needs;
     *   AudioSystem  *audio;
     */
} SimWorld;

/* ============================================================
 *  Lifecycle
 * ============================================================ */

/* Initialise room + object registry.  Does NOT generate tiles
 * or spawn any objects – call the helpers below for that.     */
bool sim_world_init(SimWorld *w, int room_width, int room_depth,
                    int room_height);

/* Run room_generate() and apply default tile layout. */
void sim_world_populate_room(SimWorld *w);

/* Tick all subsystems by `dt` seconds. */
void sim_world_update(SimWorld *w, float dt);

/* Free all owned memory. */
void sim_world_destroy(SimWorld *w);

/* ============================================================
 *  Object helpers
 * ============================================================ */

/* Place a piece of furniture.  The object is marked SOLID and
 * STATIC by default.  Returns the new Object* or NULL.        
 In theory should be rarely used, as everything should be loaded from map*/
Object *sim_world_place_furniture(SimWorld *w,
                                  Vec3W origin,
                                  Vec3W size,
                                  int   sprite_id);

/* Spawn the cat at `pos`.  Sets w->cat_id.  Returns Object*.  */
Object *sim_world_spawn_cat(SimWorld *w, Vec3W pos);

/* Convenience: get the cat object (NULL if not spawned). */
static inline Object *sim_world_get_cat(SimWorld *w)
{
    return obj_by_id(&w->objects, w->cat_id);
}

/* ============================================================
 *  Spatial query helpers  (used by rendering + AI)
 * ============================================================ */

/* Fill `out` (capacity `max`) with pointers to objects whose
 * footprint overlaps the tile column (tx, ty).
 * Returns the number of objects written.                      */
int sim_world_objects_at_tile(SimWorld *w, int tx, int ty,
                              Object **out, int max);

#endif /* SIM_WORLD_H */