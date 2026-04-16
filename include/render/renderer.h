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

#define RENDER_SCALE_PAD 0.7f

typedef struct Renderer {
    DrawCall         calls[DRAWCALL_MAX];
    int              call_count;
    const RoomLayout *layout;
    float            scale;
} Renderer;

void renderer_init      (Renderer *r, const RoomLayout *layout);
void renderer_set_layout(Renderer *r, const RoomLayout *layout);

void renderer_begin_frame(Renderer *r);
void renderer_push       (Renderer *r, Texture2D *tex, Rectangle src,
                          Vector2 dst, int sort_key, Color tint);
void renderer_flush      (Renderer *r);

static inline Vec2S renderer_project(const Renderer *r, Vec3W w)
{
    const RoomLayout *L = r->layout;

    float sx = (float)L->origin_sx +
        (w.x - w.y) * L->iso_tile_w_px +
        (w.z * 0); // keep structure clean

    float sy = (float)L->origin_sy +
        (w.x + w.y) * L->iso_tile_h_px -
        (w.z * L->iso_tile_w_px);

    return (Vec2S){ (int)sx, (int)sy };
}

static inline Vec2S renderer_project_debug(const Renderer *r, Vec3W w)
{
    const RoomLayout *L = r->layout;
    float s = r->scale;

    float sx = (float)L->origin_sx +
        ((w.x - w.y) * L->iso_tile_w_px) * s;

    float sy = (float)L->origin_sy +
        ((w.x + w.y) * L->iso_tile_h_px) * s -
        ( w.z * L->iso_tile_w_px) * s;

    return (Vec2S){ (int)sx, (int)sy };
}

#endif /* RENDERER_H */