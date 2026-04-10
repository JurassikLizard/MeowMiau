/*  test_world.c
 *  Compile:  gcc test_world.c room.c object.c sim_world.c -o test_world
 *  Exercises world init, tile generation, object placement, and
 *  projection math without needing raylib.
 */

#include "sim_world.h"
#include <stdio.h>
#include <assert.h>

static void print_floor(SimWorld *w)
{
    Room *r = &w->room;
    printf("\nFloor tile map (z=0):\n");
    printf("  y\\x");
    for (int x = 0; x <= r->width; x++) printf("  %2d", x);
    printf("\n");

    for (int y = 0; y <= r->depth; y++) {
        printf("  %2d ", y);
        for (int x = 0; x <= r->width; x++) {
            static const char *names[] = {
                " . ",   /* EMPTY       */
                " F ",   /* FLOOR       */
                "FEX",   /* FLOOR_EDGE_X*/
                "FEY",   /* FLOOR_EDGE_Y*/
                "FCR",   /* FLOOR_CORNER*/
                "WLX",   /* WALL_X      */
                "WLY",   /* WALL_Y      */
                "CBK",   /* CORNER_BACK */
                "CEL",   /* CEIL        */
            };
            TileType t = room_tile(r, x, y, 0);
            printf("%s ", names[t < TILE_TYPE_COUNT ? t : 0]);
        }
        printf("\n");
    }
}

/* ---- tunables ---- */
#define SCREEN_W      640
#define SCREEN_H      480
#define TARGET_FPS     60
 
/* Room dimensions in world units.
 * Width/depth must be multiples of SPRITE_WORLD_STEP (=2).
 * Height must be a multiple of SPRITE_Z_STEP (=4).          */
#define ROOM_WIDTH    12    /* world units, multiple of 2   */
#define ROOM_DEPTH     8    /* world units, multiple of 2   */
#define ROOM_HEIGHT    8    /* world units, multiple of 4   */
 
#define ATLAS_PATH    "assets/room_tiles.png"

static void recalc_origin(RenderContext *rc, Room *room)
{
    const int W = room->width;
    const int D = room->depth;
    const int H = room->height;

    /* Total pixel size of the room's isometric bounding box. */
    int bbox_w = (W + D) * (TILE_W / 2);
    int bbox_h = (W + D) * (TILE_H / 2) + H * TILE_H;

    /* Screen pixel for the back corner (0, 0, 0).
     * Derived above; centres the bounding box on screen. */
    int ox = SCREEN_W / 2 + (D - W) * (TILE_W / 4);
    int oy = (SCREEN_H - bbox_h) / 2 + H * TILE_H + 4 /* px padding */;

    renderer_set_origin(rc, ox, oy);

    /* Keep room's own copy in sync (used by object tile queries). */
    room->origin_sx = ox;
    room->origin_sy = oy;
}

int main(void)
{
    /* --- build a 6×4, 3-tall room --- */
    SimWorld w;
    assert(sim_world_init(&w, 6, 4, 3));
    sim_world_populate_room(&w);

    print_floor(&w);

    /* --- verify tile counts --- */
    int floors = 0, walls_x = 0, walls_y = 0;
    for (int y = 0; y <= w.room.depth; y++)
        for (int x = 0; x <= w.room.width; x++) {
            TileType t = room_tile(&w.room, x, y, 0);
            if (t == TILE_FLOOR)        floors++;
            if (t == TILE_WALL_X)       walls_x++;
            if (t == TILE_WALL_Y)       walls_y++;
        }
    printf("\nFloor cells : %d (expected %d)\n", floors,
           w.room.width * w.room.depth);

    /* --- projection round-trip check --- */
    w.room.origin_sx = 400;
    w.room.origin_sy = 100;

    Vec3W world_pt = { 3.0f, 2.0f, 1.0f };
    Vec2S screen   = world_to_screen(world_pt,
                                     w.room.origin_sx,
                                     w.room.origin_sy);
    Vec3W recovered = screen_to_world_z(screen, world_pt.z,
                                        w.room.origin_sx,
                                        w.room.origin_sy);
    printf("\nProjection round-trip:\n");
    printf("  world  (%.1f, %.1f, %.1f)\n",
           world_pt.x, world_pt.y, world_pt.z);
    printf("  screen (%d, %d)\n", screen.sx, screen.sy);
    printf("  back   (%.2f, %.2f, %.2f)\n",
           recovered.x, recovered.y, recovered.z);
    assert((recovered.x - world_pt.x) < 0.01f);
    assert((recovered.y - world_pt.y) < 0.01f);

    /* --- place a sofa --- */
    Object *sofa = sim_world_place_furniture(
        &w, (Vec3W){2.0f, 1.0f, 0.0f},
            (Vec3W){2.0f, 1.0f, 1.0f}, 42);
    assert(sofa);
    printf("\nSofa id=%u at (%.0f,%.0f,%.0f) size (%.0f,%.0f,%.0f)\n",
           sofa->id,
           sofa->origin.x, sofa->origin.y, sofa->origin.z,
           sofa->size.x,   sofa->size.y,   sofa->size.z);

    /* --- spawn the cat --- */
    Object *cat = sim_world_spawn_cat(&w, (Vec3W){4.0f, 3.0f, 0.0f});
    assert(cat);
    printf("Cat   id=%u at (%.0f,%.0f,%.0f)\n",
           cat->id, cat->origin.x, cat->origin.y, cat->origin.z);
    assert(w.cat_id == cat->id);

    /* --- query objects at sofa tile --- */
    Object *hits[8];
    int n = sim_world_objects_at_tile(&w, 2, 1, hits, 8);
    printf("Objects at tile (2,1): %d found\n", n);
    assert(n >= 1);

    /* --- run a few ticks --- */
    for (int i = 0; i < 60; i++) sim_world_update(&w, 1.0f / 60.0f);
    printf("Sim time after 60 ticks: %.3f s\n", w.time);

    sim_world_destroy(&w);
    printf("\nAll assertions passed.\n");
    return 0;
}