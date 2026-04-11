#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "world.h"
#include "spatial/room_layout.h"

/* ============================================================
 *  DrawCall
 * ============================================================ */
typedef struct {
    Texture2D *tex;
    Rectangle  src;
    Vector2    dst;
    int        sort_key;
    Color      tint;
} DrawCall;

/* ============================================================
 *  Renderer
 *
 *  Accumulates draw calls each frame, sorts by sort_key
 *  (ascending = back to front), then flushes in one pass.
 *  Holds a non-owning pointer to the active RoomLayout.
 * ============================================================ */
#define DRAWCALL_MAX 4096

typedef struct {
    DrawCall         calls[DRAWCALL_MAX];
    int              call_count;
    const RoomLayout *layout;
} Renderer;

void renderer_init      (Renderer *r, const RoomLayout *layout);
void renderer_set_layout(Renderer *r, const RoomLayout *layout);

void renderer_begin_frame(Renderer *r);
void renderer_push       (Renderer *r, Texture2D *tex, Rectangle src,
                          Vector2 dst, int sort_key, Color tint);
void renderer_flush      (Renderer *r);

static inline Vec2S renderer_project(const Renderer *r, Vec3W w)
{
    Vec2S s;
    s.sx = r->layout->origin_sx + (w.x - w.y) * r->layout->iso_tile_w_px;
    s.sy = r->layout->origin_sy + (w.x + w.y) * r->layout->iso_tile_h_px
                               -  w.z         * r->layout->iso_tile_w_px;
    return s;
}

#endif /* RENDERER_H */