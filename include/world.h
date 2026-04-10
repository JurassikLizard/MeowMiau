#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  TILE / PROJECTION CONSTANTS
 *
 *  One world unit maps to:
 *    X-axis  ->  +2 tiles right,  +1 tile down  (screen)
 *    Y-axis  ->  -2 tiles left,   +1 tile down  (screen)
 *    Z-axis  ->  0 tiles horiz,   -2 tiles up   (screen)
 *
 *  TILE_W and TILE_H define the base pixel size of one tile.
 *  All projection math is in tile units; multiply by TILE_W/H
 *  to reach pixel space.
 *
 *  With TILE_SIZE = 8 pixels the rhombus face is 16 px wide
 *  and 8 px tall (classic 2:1 dimetric ratio).
 * ============================================================ */

#define TILE_SIZE    4          /* pixels per tile edge          */
#define TILE_W       (TILE_SIZE * 2)   /* face width  in pixels  */
#define TILE_H       (TILE_SIZE)       /* face height in pixels  */

/* ============================================================
 *  WORLD VECTOR TYPES
 *
 *  Vec3W   – continuous world-space position (float)
 *  Vec3Wi  – discrete world-space cell index (int)
 *  Vec2S   – 2-D screen-space pixel coordinate (int)
 * ============================================================ */

typedef struct { float x, y, z; } Vec3W;   /* world space  */
typedef struct { int   x, y, z; } Vec3Wi;  /* integer cell */
typedef struct { int   sx, sy;  } Vec2S;   /* screen pixel */

/* ============================================================
 *  ISOMETRIC PROJECTION
 *
 *  Converts a continuous world position to a screen pixel.
 *  The origin (0,0,0) maps to some reference point that the
 *  caller supplies as (origin_sx, origin_sy).
 *
 *  Screen equations (tile units, then scale by TILE_W/H):
 *
 *    screen_x  =  (wx - wy) * TILE_W/2
 *    screen_y  =  (wx + wy) * TILE_H/2  -  wz * TILE_H
 *
 *  Integer-cell variant is provided for snapping to grid.
 * ============================================================ */

static inline Vec2S world_to_screen(Vec3W w, int origin_sx, int origin_sy)
{
    Vec2S s;
    s.sx = origin_sx + (int)((w.x - w.y) * (TILE_W / 2.0f));
    s.sy = origin_sy + (int)((w.x + w.y) * (TILE_H / 2.0f) - w.z * TILE_H);
    return s;
}

static inline Vec2S world_to_screen_i(Vec3Wi w, int origin_sx, int origin_sy)
{
    Vec3W wf = { (float)w.x, (float)w.y, (float)w.z };
    return world_to_screen(wf, origin_sx, origin_sy);
}

/* Invert: given a screen pixel and a known Z plane, recover (x,y).
 * Useful for mouse picking on the floor (z == 0).                   */
static inline Vec3W screen_to_world_z(Vec2S s, float wz,
                                      int origin_sx, int origin_sy)
{
    float rx = (float)(s.sx - origin_sx) / (TILE_W / 2.0f);
    float ry = (float)(s.sy - origin_sy + (int)(wz * TILE_H)) / (TILE_H / 2.0f);
    Vec3W w;
    w.x = (rx + ry) * 0.5f;
    w.y = (ry - rx) * 0.5f;
    w.z = wz;
    return w;
}

/* ============================================================
 *  SORT KEY  (painter's algorithm depth)
 *
 *  Objects further "into" the scene (larger x+y, lower z)
 *  should be drawn first.  A single scalar captures this.
 * ============================================================ */

static inline float world_sort_key(Vec3W w)
{
    return w.x + w.y - w.z * 0.001f;   /* small z bias breaks ties */
}

#endif /* WORLD_H */