#include "spatial/room.h"
#include <stdlib.h>
#include <string.h>

bool room_init(Room *r, int width, int depth, int height)
{
    if (!r || width <= 0 || depth <= 0 || height <= 0) return false;
    r->width  = width;
    r->depth  = depth;
    r->height = height;
    r->map_w  = width  + 1;
    r->map_d  = depth  + 1;
    r->map_h  = height + 1;
    size_t n  = (size_t)(r->map_w * r->map_d * r->map_h);
    r->cells  = (TileCell *)calloc(n, sizeof(TileCell));
    return r->cells != NULL;
}

void room_generate(Room *r)
{
    for (int y = 0; y <= r->depth; y++)
        for (int x = 0; x <= r->width; x++) {
            TileCell *c = room_cell(r, x, y, 0);
            if (c) c->type = (x == 0 || y == 0) ? TILE_FLOOR_WALL_JOIN : TILE_FLOOR;
        }

    for (int z = 1; z <= r->height; z++)
        for (int y = 0; y <= r->depth; y++)
            for (int x = 0; x <= r->width; x++) {
                TileCell *c = room_cell(r, x, y, z);
                if (c) c->type = (x == 0 || y == 0) ? TILE_WALL : TILE_EMPTY;
            }
}

bool room_is_walkable(Room *r, int x, int y)
{
    TileType t = room_tile(r, x, y, 0);
    return (t == TILE_FLOOR || t == TILE_FLOOR_WALL_JOIN);
}

void room_destroy(Room *r)
{
    if (r && r->cells) { free(r->cells); r->cells = NULL; }
}