#ifndef RENDERER_H
#define RENDERER_H

/*  ============================================================
 *  RENDERER  –  render/renderer.h
 *
 *  The renderer is a stateless, data-driven layer that sits on
 *  top of raylib.  It holds:
 *    • loaded textures (atlas, later object sheets)
 *    • a draw-call list for the current frame (painter-sorted)
 *    • the screen origin that maps world (0,0,0) to a pixel
 *
 *  Nothing in this file touches SimWorld internals – it only
 *  reads Room and ObjectRegistry through their public APIs.
 *
 *  Extension plan:
 *    Phase 1 (now)   – room tiles only
 *    Phase 2         – object draw calls inserted into sorted list
 *    Phase 3         – cat animation frames, UI overlay
 *
 *  DRAW CALL LIST
 *  --------------
 *  Every visible thing becomes a DrawCall (sprite + screen pos +
 *  sort key).  After collection the list is sorted ascending by
 *  sort_key (back-to-front) and flushed to raylib in one pass.
 *  This naturally interleaves room tiles and objects without
 *  special-casing either.
 *  ============================================================ */

#include "raylib.h"
#include "../sim_world.h"
#include "room_atlas.h"

/* ============================================================
 *  DrawCall  –  one sprite blit, unsorted
 * ============================================================ */
typedef struct {
    Texture2D  *tex;        /* which texture atlas              */
    Rectangle   src;        /* source rect within atlas         */
    Vector2     dst;        /* top-left screen pixel (float)    */
    float       sort_key;   /* painter depth: draw low first    */
    Color       tint;       /* WHITE = unmodified               */
} DrawCall;

/* ============================================================
 *  RenderContext
 * ============================================================ */
#define DRAWCALL_MAX  4096

typedef struct {
    /* ---- atlas textures ---- */
    Texture2D room_atlas;       /* 19-tile room sprite sheet        */
    /* future:  Texture2D object_atlas; */
    /* future:  Texture2D cat_atlas;    */

    /* ---- frame draw list ---- */
    DrawCall  calls[DRAWCALL_MAX];
    int       call_count;

    /* ---- viewport / projection origin ---- */
    /* World point (0, room.depth, 0) – the back-left floor corner –
     * is mapped to (origin_sx, origin_sy) in screen pixels.
     * Update this when the window is resized.                    */
    int origin_sx;
    int origin_sy;
} RenderContext;

/* ============================================================
 *  Lifecycle
 * ============================================================ */

/* Load textures.  Call after InitWindow().
 * room_atlas_path – path to the 19-tile PNG.
 * Returns false if any texture fails to load.                  */
bool renderer_init(RenderContext *rc, const char *room_atlas_path);

/* Unload textures.  Call before CloseWindow().                 */
void renderer_destroy(RenderContext *rc);

/* Set/update the screen origin so the room is centred or
 * positioned correctly after a window resize.                  */
void renderer_set_origin(RenderContext *rc, int sx, int sy);

/* ============================================================
 *  Frame API
 *
 *  Typical usage inside the game loop:
 *
 *    BeginDrawing();
 *    ClearBackground(BLACK);
 *    renderer_begin_frame(rc);
 *    renderer_submit_room(rc, &world->room);
 *    // later: renderer_submit_objects(rc, &world->objects);
 *    // later: renderer_submit_cat(rc, cat);
 *    renderer_flush(rc);           // sort + blit everything
 *    EndDrawing();
 * ============================================================ */

/* Reset the draw-call list for this frame. */
void renderer_begin_frame(RenderContext *rc);

/* Collect draw calls for all room tiles. */
void renderer_submit_room(RenderContext *rc, Room *room);

/* Sort and blit all collected draw calls. */
void renderer_flush(RenderContext *rc);

/* ============================================================
 *  Internal helpers (exposed for testing / extension)
 * ============================================================ */

/* Push one draw call.  Silently drops if list is full. */
void renderer_push(RenderContext *rc, Texture2D *tex, Rectangle src,
                   Vector2 dst, float sort_key, Color tint);

/* Compute the top-left screen pixel for a room sprite whose
 * world anchor is `w`.  The sprite is SPRITE_W × SPRITE_H px;
 * the anchor is its bottom-centre in world space.              */
Vector2 renderer_sprite_pos(RenderContext *rc, Vec3W world_anchor);

#endif /* RENDERER_H */