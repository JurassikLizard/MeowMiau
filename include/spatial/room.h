#ifndef ROOM_H
#define ROOM_H

#include "world.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  TILE TYPES
 *
 *  Each integer cell in the map carries a TileType that tells
 *  the renderer which sprite/atlas region to use, and tells
 *  the simulation about passability / surface kind.
 *
 *  Naming convention:
 *    TILE_FLOOR_*        – flat ground surfaces
 *    TILE_WALL_*         – vertical wall faces (exposed side)
 *    TILE_CORNER_*       – wall–wall junction columns
 *    TILE_CEIL_*         – ceiling tiles (if ever needed)
 *    TILE_EMPTY          – air / out-of-bounds
 *
 *  Wall direction suffixes:
 *    _X   – wall whose outward normal faces the –X direction
 *           (visible from the +X side; "left" wall in iso view)
 *    _Y   – wall whose outward normal faces the –Y direction
 *           (visible from the +Y side; "right" wall in iso view)
 * ============================================================ */

typedef enum {
    TILE_EMPTY          = 0,

    /* ---- floors ---- */
    TILE_FLOOR,             /* plain interior floor            */
    TILE_FLOOR_EDGE_X,      /* floor tile along the X wall     */
    TILE_FLOOR_EDGE_Y,      /* floor tile along the Y wall     */
    TILE_FLOOR_CORNER,      /* floor tile at the corner        */

    /* ---- walls (vertical faces) ---- */
    TILE_WALL_X,            /* left  wall face (normal –X)     */
    TILE_WALL_Y,            /* right wall face (normal –Y)     */

    /* ---- corners ---- */
    TILE_CORNER_BACK,       /* back vertical column            */

    /* ---- ceiling (stub, not rendered by default) ---- */
    TILE_CEIL,

    TILE_TYPE_COUNT
} TileType;

/* ============================================================
 *  TILE CELL  (one integer voxel in the map)
 *
 *  Keeps the tile type plus a small metadata field for
 *  future use (decals, moisture level, scratch marks, etc.)
 * ============================================================ */

typedef struct {
    TileType type;
    uint8_t  meta;      /* reserved – free for future data     */
} TileCell;

/* ============================================================
 *  ROOM DEFINITION
 *
 *  The entire simulation lives inside a single rectangular
 *  room defined by three dimensions in world units:
 *
 *    width   – extent along the X axis
 *    depth   – extent along the Y axis
 *    height  – extent along the Z axis
 *
 *  World-space layout:
 *    Floor:     z == 0,  0 <= x < width,  0 <= y < depth
 *    X-walls:   x == 0,  0 <= y < depth,  0 <= z < height
 *    Y-walls:   y == 0,  0 <= x < width,  0 <= z < height
 *    Back col:  x == 0,  y == 0,  0 <= z < height
 *
 *  The tile map is a flat array addressed by:
 *      index = layer * (width * depth)  +  row * width  +  col
 *  where col = x, row = y, layer = z.
 *
 *  For the floor plane (z == 0) x and y run over the interior.
 *  Wall tiles occupy x == 0 (X-face) and y == 0 (Y-face).
 *
 *  Map dimensions stored:
 *    map_w  = width  + 1   (includes the x==0 wall column)
 *    map_d  = depth  + 1   (includes the y==0 wall column)
 *    map_h  = height + 1   (includes the z==0 floor layer)
 * ============================================================ */

typedef struct {
    /* ---- logical dimensions (in world/tile units) ---- */
    int width;          /* X extent of the interior              */
    int depth;          /* Y extent of the interior              */
    int height;         /* Z extent (wall/ceiling height)        */

    /* ---- derived map array dimensions ---- */
    int map_w;          /* = width  + 1                          */
    int map_d;          /* = depth  + 1                          */
    int map_h;          /* = height + 1                          */

    /* ---- flat voxel array (heap-allocated) ---- */
    TileCell *cells;    /* size: map_w * map_d * map_h           */

    /* ---- screen-space origin ---- */
    /* The world point (0, depth, 0) – back-left floor corner –
       maps to this pixel; caller sets it after window sizing.   */
    int origin_sx;
    int origin_sy;
} Room;

/* ============================================================
 *  MAP ACCESSORS
 * ============================================================ */

/* Returns NULL if out of bounds. */
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

/* ============================================================
 *  ROOM LIFECYCLE
 * ============================================================ */

/* Allocate and zero-initialise the room. Does NOT populate tiles.
 * Returns false on allocation failure.                           */
bool room_init(Room *r, int width, int depth, int height);

/* Fill the tile map with the standard room geometry:
 *   – floor tiles across the base plane
 *   – wall tiles on the X==0 and Y==0 faces
 *   – corner column at X==0, Y==0
 *   – wall-edge floor tiles along the wall bases
 * This is the only function that "knows" about room aesthetics. */
void room_generate(Room *r);

/* Free heap memory owned by the room. */
void room_destroy(Room *r);

/* ============================================================
 *  PASSABILITY QUERY
 *
 *  Returns true if a world cell is walkable by the cat.
 *  A cell is walkable if z==0 AND its tile is a floor type
 *  AND it is not occupied by an opaque furniture object.
 *  (Furniture blocking is handled by the object system;
 *   this function only checks the tile layer.)
 * ============================================================ */
bool room_is_walkable(Room *r, int x, int y);

#endif /* ROOM_H */