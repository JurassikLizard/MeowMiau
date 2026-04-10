#include "spatial/room.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 *  room_init
 * ============================================================ */
bool room_init(Room *r, int width, int depth, int height)
{
    if (!r || width <= 0 || depth <= 0 || height <= 0) return false;

    r->width  = width;
    r->depth  = depth;
    r->height = height;

    r->map_w = width  + 1;
    r->map_d = depth  + 1;
    r->map_h = height + 1;

    r->origin_sx = 0;
    r->origin_sy = 0;

    size_t n = (size_t)(r->map_w * r->map_d * r->map_h);
    r->cells = (TileCell *)calloc(n, sizeof(TileCell));
    return r->cells != NULL;
}

/* ============================================================
 *  room_generate
 *
 *  Populates the tile map for a standard four-walled room.
 *
 *  Layer z == 0  (floor plane):
 *    (0, 0, 0)               -> TILE_FLOOR_CORNER
 *    (x, 0, 0)  x >= 1      -> TILE_FLOOR_EDGE_Y  (along Y wall)
 *    (0, y, 0)  y >= 1      -> TILE_FLOOR_EDGE_X  (along X wall)
 *    (x, y, 0)  x,y >= 1   -> TILE_FLOOR
 *
 *  Layers z >= 1  (vertical slices):
 *    (0, 0, z)               -> TILE_CORNER_BACK
 *    (x, 0, z)  x >= 1      -> TILE_WALL_Y
 *    (0, y, z)  y >= 1      -> TILE_WALL_X
 *    interior cells stay TILE_EMPTY (air)
 * ============================================================ */
void room_generate(Room *r)
{
    /* --- floor layer (z == 0) --- */
    for (int y = 0; y <= r->depth; y++) {
        for (int x = 0; x <= r->width; x++) {
            TileCell *c = room_cell(r, x, y, 0);
            if (!c) continue;

            if (x == 0 && y == 0)       c->type = TILE_FLOOR_CORNER;
            else if (x == 0)            c->type = TILE_FLOOR_EDGE_X;
            else if (y == 0)            c->type = TILE_FLOOR_EDGE_Y;
            else                        c->type = TILE_FLOOR;
        }
    }

    /* --- wall layers (z >= 1) --- */
    for (int z = 1; z <= r->height; z++) {
        for (int y = 0; y <= r->depth; y++) {
            for (int x = 0; x <= r->width; x++) {
                TileCell *c = room_cell(r, x, y, z);
                if (!c) continue;

                if (x == 0 && y == 0)   c->type = TILE_CORNER_BACK;
                else if (x == 0)        c->type = TILE_WALL_X;
                else if (y == 0)        c->type = TILE_WALL_Y;
                else                    c->type = TILE_EMPTY;
            }
        }
    }
}

/* ============================================================
 *  room_is_walkable
 * ============================================================ */
bool room_is_walkable(Room *r, int x, int y)
{
    TileType t = room_tile(r, x, y, 0);
    return (t == TILE_FLOOR ||
            t == TILE_FLOOR_EDGE_X ||
            t == TILE_FLOOR_EDGE_Y ||
            t == TILE_FLOOR_CORNER);
}

/* ============================================================
 *  room_destroy
 * ============================================================ */
void room_destroy(Room *r)
{
    if (r && r->cells) {
        free(r->cells);
        r->cells = NULL;
    }
}