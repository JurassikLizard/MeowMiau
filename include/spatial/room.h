#ifndef ROOM_H
#define ROOM_H

#include "world.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TILE_EMPTY         = 0,
    TILE_FLOOR,
    TILE_WALL,
    TILE_FLOOR_WALL_JOIN,
    TILE_TYPE_COUNT
} TileType;

typedef struct {
    TileType type;
    uint8_t  meta;
} TileCell;

/* ============================================================
 *  ROOM
 *  Index: z*(map_w*map_d) + y*map_w + x
 * ============================================================ */
typedef struct {
    int       width, depth, height;
    int       map_w,  map_d,  map_h;
    TileCell *cells;
} Room;

static inline TileCell *room_cell(Room *r, int x, int y, int z)
{
    if (x < 0 || x >= r->map_w) return NULL;
    if (y < 0 || y >= r->map_d) return NULL;
    if (z < 0 || z >= r->map_h) return NULL;
    return &r->cells[z * (r->map_w * r->map_d) + y * r->map_w + x];
}

static inline TileType room_tile(Room *r, int x, int y, int z)
{
    TileCell *c = room_cell(r, x, y, z);
    return c ? c->type : TILE_EMPTY;
}

bool room_init    (Room *r, int width, int depth, int height);
void room_generate(Room *r);
void room_destroy (Room *r);
bool room_is_walkable(Room *r, int x, int y);

#endif /* ROOM_H */