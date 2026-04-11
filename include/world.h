#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  WORLD VECTOR TYPES
 *
 *  All world positions are integer tile coordinates.
 *
 *  Vec3W  – world tile position (int x, y, z)
 *  Vec2S  – screen pixel coordinate (int sx, sy)
 *
 *  Axis convention (matches pre-drawn room image):
 *    +x  →  down-right on screen
 *    +y  →  down-left  on screen
 *    +z  →  up         on screen
 *    (0,0,0) is the back corner of the room
 * ============================================================ */

typedef struct { int x, y, z; } Vec3W;
typedef struct { int sx, sy;  } Vec2S;

/* ============================================================
 *  SORT KEY  (painter's algorithm, back-to-front)
 *
 *  Higher x+y = closer to camera = drawn later.
 *  z tiebreaks: higher z = drawn later (on top of floor).
 *  Scaled so z differences don't swamp x+y differences.
 * ============================================================ */
static inline int world_sort_key(Vec3W w)
{
    return (w.x + w.y) * 1000 + w.z;
}

#endif /* WORLD_H */