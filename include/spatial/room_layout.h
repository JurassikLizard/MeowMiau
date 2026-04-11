#ifndef ROOM_LAYOUT_H
#define ROOM_LAYOUT_H

/* ============================================================
 *  ROOM LAYOUT  –  room_layout.h
 *
 *  Converts 5 image-crop numbers + a loaded Texture2D into all
 *  tile pixel sizes, room dimensions, and the projection origin.
 *
 *  ── The 5 input numbers ─────────────────────────────────────
 *
 *  1. rhombus_h_pad  Horizontal padding on EACH side of the
 *                    floor rhombus.
 *                    rhombus_w_px = img_w - 2 * rhombus_h_pad.
 *  2. wall_pad       Dead pixels at the very top of the image
 *                    (above the usable wall area).
 *  3. wall_h_px      Pixel height of the usable wall area.
 *                    room_height = wall_h_px / iso_tile_w_px.
 *  4. rhombus_v_pad_top     Dead pixels ABOVE the usable floor rhombus.
 *  5. rhombus_v_pad_bottom  Dead pixels BELOW the usable floor rhombus.
 *                    rhombus_v_px = img_h - wall_pad - wall_h_px
 *                                         - rhombus_v_pad_top
 *                                         - rhombus_v_pad_bottom.
 *  5. iso_tile_h_px  Pixel height of one iso tile face.
 *                    iso_tile_w_px = iso_tile_h_px * 2.
 *
 *  Plus:
 *    img_w, img_h    Read directly from the loaded texture.
 *    vp_w, vp_h      Viewport size in pixels (for centring).
 *
 *  ── Vertical image layout (top → bottom) ────────────────────
 *
 *    | wall_pad | wall_h_px | rhombus_v_pad_top | rhombus_v_px | rhombus_v_pad_bottom |
 *
 *  ── Derived quantities ──────────────────────────────────────
 *
 *    iso_tile_w_px = iso_tile_h_px * 2
 *    rhombus_w_px  = img_w - 2 * rhombus_h_pad
 *    rhombus_v_px  = img_h - wall_pad - wall_h_px - 2 * rhombus_v_pad
 *    iso_span      = rhombus_w_px / iso_tile_w_px
 *    room_width    = iso_span / 2  (square room)
 *    room_depth    = iso_span / 2
 *    room_height   = wall_h_px / iso_tile_w_px
 *
 *  ── Projection ──────────────────────────────────────────────
 *
 *  World origin (0,0,0) = back corner of the rhombus.
 *  In image-local coordinates:
 *
 *    local_ox = rhombus_h_pad + rhombus_w_px / 2
 *    local_oy = wall_pad + wall_h_px + rhombus_v_pad
 *
 *  The image is centred in the viewport:
 *
 *    img_offset_x = (vp_w - img_w) / 2
 *    img_offset_y = (vp_h - img_h) / 2
 *    origin_sx    = img_offset_x + local_ox
 *    origin_sy    = img_offset_y + local_oy
 *
 *  Projection formula (one world unit = one full tile face):
 *    +x moves  +iso_tile_w_px  screen-x,  +iso_tile_h_px  screen-y
 *    +y moves  -iso_tile_w_px  screen-x,  +iso_tile_h_px  screen-y
 *    +z moves   0              screen-x,  -iso_tile_w_px  screen-y
 *
 *    sx = origin_sx + (wx - wy) * iso_tile_w_px
 *    sy = origin_sy + (wx + wy) * iso_tile_h_px  -  wz * iso_tile_w_px
 *
 * ============================================================ */

#include "world.h"
#include "raylib.h"
#include <assert.h>

typedef struct {
    /* ---- caller-supplied inputs -------------------------------- */
    int rhombus_h_pad;  /* horizontal padding each side             */
    int wall_pad;       /* dead pixels above usable wall area       */
    int wall_h_px;      /* usable wall pixel height                 */
    int rhombus_v_pad_top;    /* dead pixels above floor rhombus          */
    int rhombus_v_pad_bottom; /* dead pixels below floor rhombus          */
    int iso_tile_h_px;  /* iso tile face pixel height               */

    int vp_w, vp_h;    /* viewport dimensions for centring         */

    /* ---- derived by room_layout_init() ------------------------ */
    int img_w, img_h;         /* from texture                       */
    int rhombus_w_px;         /* = img_w - 2 * rhombus_h_pad        */
    int rhombus_v_px;         /* = img_h - wall_pad - wall_h_px
                                        - 2 * rhombus_v_pad         */
    int iso_tile_w_px;        /* = iso_tile_h_px * 2                */
    int iso_span;             /* = rhombus_w_px / iso_tile_w_px     */
    int room_width;           /* = iso_span / 2                     */
    int room_depth;           /* = iso_span / 2                     */
    int room_height;          /* = wall_h_px / iso_tile_w_px        */

    int img_offset_x;         /* image top-left x in viewport       */
    int img_offset_y;         /* image top-left y in viewport       */
    int origin_sx, origin_sy; /* screen pixel of world (0,0,0)      */
} RoomLayout;

/* ---- init ---------------------------------------------------- */

static inline void room_layout_init(RoomLayout *rl, const Texture2D *tex)
{
    rl->img_w = tex->width;
    rl->img_h = tex->height;

    rl->iso_tile_w_px = rl->iso_tile_h_px * 2;

    /* Derive rhombus pixel extents from image dimensions */
    rl->rhombus_w_px = rl->img_w - 2 * rl->rhombus_h_pad;
    rl->rhombus_v_px = rl->img_h - rl->wall_pad - rl->wall_h_px
                                 - rl->rhombus_v_pad_top
                                 - rl->rhombus_v_pad_bottom;
    assert(rl->rhombus_w_px > 0 && "rhombus_h_pad too large for image width");
    assert(rl->rhombus_v_px > 0 && "vertical pads + wall exceed image height");

    /* Tile span — discard any partial-tile remainder at the edges */
    rl->iso_span = rl->rhombus_w_px / rl->iso_tile_w_px;
    assert(rl->iso_span % 2 == 0 && "iso_span must be even for a square room");
    rl->room_width  = rl->iso_span / 2;
    rl->room_depth  = rl->iso_span / 2;
    rl->room_height = rl->wall_h_px / rl->iso_tile_w_px;

    /* Centre image in viewport */
    rl->img_offset_x = (rl->vp_w - rl->img_w) / 2;
    rl->img_offset_y = (rl->vp_h - rl->img_h) / 2;

    /* World origin = back corner = top-centre of usable rhombus */
    int local_ox  = rl->rhombus_h_pad + rl->rhombus_w_px / 2;
    int local_oy  = rl->wall_pad + rl->wall_h_px + rl->rhombus_v_pad_top;
    rl->origin_sx = rl->img_offset_x + local_ox;
    rl->origin_sy = rl->img_offset_y + local_oy;
}

/* ---- draw background centred in viewport -------------------- */

static inline void room_layout_draw_background(const RoomLayout *rl,
                                                Texture2D        *tex)
{
    DrawTexture(*tex, rl->img_offset_x, rl->img_offset_y, WHITE);
}

#endif /* ROOM_LAYOUT_H */