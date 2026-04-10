#include "render/renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 *  SPRITE ANCHOR GEOMETRY
 *
 *  Every sprite is 16×16 px.  We need to find the screen pixel
 *  that corresponds to the sprite's top-left corner, given the
 *  world position of the sprite's "anchor" point.
 *
 *  FLOOR SPRITE
 *  ............
 *  A floor patch covers world region [wx, wx+2) × [wy, wy+2).
 *  In iso projection the screen top of the diamond corresponds
 *  to the back corner (wx+2, wy+2).  The sprite top-left is
 *  half a sprite-width to the left of that point.
 *
 *    anchor_world = (wx+STEP, wy+STEP, 0)
 *    screen_top_centre = world_to_screen(anchor_world)
 *    sprite_top_left.x = screen_top_centre.sx - SPRITE_W/2
 *    sprite_top_left.y = screen_top_centre.sy - SPRITE_H/2
 *      (the -SPRITE_H/2 accounts for the upper half of the 2:1
 *       diamond that sits above the mid-line of the sprite)
 *
 *  WALL-X SPRITE  (left wall, outward normal –X, x==0 face)
 *  ...........................................................
 *  Wall sprites sit at x==0.  A wall column covers
 *  [wy, wy+2) in Y and [wz, wz+4) in Z.
 *  The top-left of the sprite should align with:
 *    - horizontally: the right edge of the x=0 column projected
 *                    at (0, wy, wz+ZSTEP) → that's the top of the column
 *    - vertically:   the top of the wall strip at z+ZSTEP
 *
 *  In practice for wall-X tiles:
 *    anchor_world = (0, wy+STEP, wz+ZSTEP)
 *    sprite_top_left.x = screen.sx - SPRITE_W
 *    sprite_top_left.y = screen.sy
 *
 *  WALL-Y SPRITE  (right wall, outward normal –Y, y==0 face)
 *  ...........................................................
 *  Mirror of wall-X:
 *    anchor_world = (wx+STEP, 0, wz+ZSTEP)
 *    sprite_top_left.x = screen.sx
 *    sprite_top_left.y = screen.sy
 *
 *  BACK CORNER SPRITE  (x==0, y==0 column)
 *  ........................................
 *    anchor_world = (0, 0, wz+ZSTEP)
 *    sprite_top_left.x = screen.sx - SPRITE_W/2
 *    sprite_top_left.y = screen.sy
 *
 *  These offsets were derived by drawing the isometric geometry
 *  on paper and verifying that adjacent sprites share edges.
 * ============================================================ */

/* sort_key for a room tile: same painter logic as world_sort_key
 * but we want floors (low sort key, drawn first) then walls     */
static float tile_sort_key(float wx, float wy, float wz)
{
    /* walls should draw after the floor below them               */
    return wx + wy - wz * 0.5f;
}

/* ============================================================
 *  Lifecycle
 * ============================================================ */

bool renderer_init(RenderContext *rc, const char *room_atlas_path)
{
    memset(rc, 0, sizeof(*rc));

    rc->room_atlas = LoadTexture(room_atlas_path);
    if (rc->room_atlas.id == 0) {
        fprintf(stderr, "renderer: failed to load room atlas: %s\n",
                room_atlas_path);
        return false;
    }

    /* Nearest-neighbour filter so pixel art stays crisp           */
    SetTextureFilter(rc->room_atlas, TEXTURE_FILTER_POINT);

    return true;
}

void renderer_destroy(RenderContext *rc)
{
    UnloadTexture(rc->room_atlas);
    memset(rc, 0, sizeof(*rc));
}

void renderer_set_origin(RenderContext *rc, int sx, int sy)
{
    rc->origin_sx = sx;
    rc->origin_sy = sy;
}

/* ============================================================
 *  Frame API
 * ============================================================ */

void renderer_begin_frame(RenderContext *rc)
{
    rc->call_count = 0;
}

/* qsort comparator – ascending sort_key (back to front)        */
static int draw_call_cmp(const void *a, const void *b)
{
    float ka = ((const DrawCall *)a)->sort_key;
    float kb = ((const DrawCall *)b)->sort_key;
    if (ka < kb) return -1;
    if (ka > kb) return  1;
    return 0;
}

void renderer_flush(RenderContext *rc)
{
    qsort(rc->calls, (size_t)rc->call_count,
          sizeof(DrawCall), draw_call_cmp);

    for (int i = 0; i < rc->call_count; i++) {
        DrawCall *dc = &rc->calls[i];
        DrawTextureRec(*dc->tex, dc->src, dc->dst, dc->tint);
    }
}

/* ============================================================
 *  Internal helpers
 * ============================================================ */

void renderer_push(RenderContext *rc, Texture2D *tex, Rectangle src,
                   Vector2 dst, float sort_key, Color tint)
{
    if (rc->call_count >= DRAWCALL_MAX) return;
    DrawCall *dc = &rc->calls[rc->call_count++];
    dc->tex      = tex;
    dc->src      = src;
    dc->dst      = dst;
    dc->sort_key = sort_key;
    dc->tint     = tint;
}

Vector2 renderer_sprite_pos(RenderContext *rc, Vec3W world_anchor)
{
    Vec2S s = world_to_screen(world_anchor, rc->origin_sx, rc->origin_sy);
    return (Vector2){ (float)s.sx, (float)s.sy };
}

/* ============================================================
 *  ROOM SUBMISSION
 *
 *  We iterate the room in sprite-sized steps rather than
 *  world-tile steps.  Each sprite covers:
 *    STEP  = SPRITE_WORLD_STEP = 2  world units in X or Y
 *    ZSTEP = SPRITE_Z_STEP     = 4  world units in Z
 *
 *  Render order (submitted low→high sort key):
 *    1. Floor patches           (z == 0, all x,y interior)
 *    2. Wall-floor transitions  (bottom of each wall column)
 *    3. Wall body rows          (mid of each wall column)
 *    4. Wall top-edge rows      (top of each wall column)
 *    5. Back corner column      (x==0, y==0)
 *
 *  The sort key ensures floors draw before walls, and lower
 *  wall rows draw before upper ones (painter safety).
 * ============================================================ */

/* Helper: push one room sprite */
static void push_room_sprite(RenderContext *rc, RoomSpriteID id,
                             float wx, float wy, float wz,
                             int dx_px, int dy_px,
                             float sort_key)
{
    /* world_to_screen gives us the screen position of the world
     * anchor; then we apply the per-sprite pixel offset.        */
    Vec3W anchor = { wx, wy, wz };
    Vec2S s = world_to_screen(anchor, rc->origin_sx, rc->origin_sy);
    Vector2 dst = {
        (float)(s.sx + dx_px),
        (float)(s.sy + dy_px)
    };
    renderer_push(rc, &rc->room_atlas,
                  room_atlas_src(id), dst, sort_key, WHITE);
}

void renderer_submit_room(RenderContext *rc, Room *room)
{
    const int   STEP  = SPRITE_WORLD_STEP;   /* 2 */
    const int   ZSTEP = SPRITE_Z_STEP;       /* 4 */

    /* --------------------------------------------------------
     *  1. FLOOR  –  interior patches
     *
     *  Iterate x from 1..width and y from 1..depth in STEP
     *  increments.  Anchor = back corner of the patch
     *  (wx+STEP, wy+STEP, 0).
     *
     *  Screen top-left:
     *    The projected point of (wx+STEP, wy+STEP, 0) is the
     *    top diamond vertex of the tile.  The sprite top-left
     *    is SPRITE_W/2 to the left and 0 down from there.
     *    The sprite then extends downward to cover the lower
     *    half of the diamond (the front vertex).
     * -------------------------------------------------------- */
    for (int wy = STEP; wy <= room->depth; wy += STEP) {
        for (int wx = STEP; wx <= room->width; wx += STEP) {

            float sk = tile_sort_key((float)wx, (float)wy, 0.0f);

            /* Pick floor sprite variant based on position */
            RoomSpriteID id;
            bool on_x_edge = (wx <= STEP);
            bool on_y_edge = (wy <= STEP);

            if (on_x_edge && on_y_edge)
                id = SPR_FLOOR_CORNER_BACK;
            else if (on_x_edge)
                id = SPR_FLOOR_EDGE_LEFT;
            else if (on_y_edge)
                id = SPR_FLOOR_EDGE_RIGHT;
            else
                id = SPR_FLOOR_CORNER_BACK; /* plain floor – see note below */

            /*  NOTE: The 19-tile sheet doesn't include a "plain
             *  floor interior" tile in the listing above.
             *  If your sheet has one, add SPR_FLOOR_PLAIN and use
             *  it here.  For now we reuse FLOOR_CORNER_BACK as a
             *  placeholder.  This is the only place to change.  */
            (void)on_x_edge; (void)on_y_edge;

            /* Determine real floor sprite by position */
            {
                bool edge_x = (wx == STEP);          /* touches X wall */
                bool edge_y = (wy == STEP);          /* touches Y wall */
                bool front_x = (wx == room->width);  /* open front X edge */
                bool front_y = (wy == room->depth);  /* open front Y edge */

                if (edge_x && edge_y)
                    id = SPR_FLOOR_CORNER_BACK;
                else if (front_x && front_y)
                    id = SPR_FLOOR_CORNER_FRONT;
                else if (front_x)
                    id = SPR_FLOOR_EDGE_LEFT;
                else if (front_y)
                    id = SPR_FLOOR_EDGE_RIGHT;
                else
                    id = SPR_FLOOR_CORNER_BACK;  /* placeholder for plain */
            }

            /* Anchor = top of diamond = (wx, wy, 0)
             * Offset to sprite top-left: -SPRITE_W/2 in x, 0 in y
             * (the "top" of the iso rhombus sits at the projected
             *  back corner; our sprite top-left is half-width left) */
            push_room_sprite(rc, id,
                             (float)wx, (float)wy, 0.0f,
                             -SPRITE_W / 2, 0,
                             sk);
        }
    }

    /* --------------------------------------------------------
     *  2–4. WALLS  –  X-wall (left), Y-wall (right), back corner
     *
     *  Each wall column is ZSTEP world units tall per sprite row.
     *  We render from bottom to top so that upper rows draw
     *  over lower rows' edges.
     *
     *  Wall-X  (x == 0 face, visible from +X side)
     *  -------------------------------------------
     *  For each y in [STEP..depth] and each z layer:
     *    Anchor = (0, wy, wz+ZSTEP)  — top-left of wall face
     *    Sprite top-left offset: (-SPRITE_W, 0)
     *
     *  Wall-Y  (y == 0 face, visible from +Y side)
     *  -------------------------------------------
     *  For each x in [STEP..width] and each z layer:
     *    Anchor = (wx+STEP, 0, wz+ZSTEP)
     *    Sprite top-left offset: (0, 0)
     *
     *  Back corner  (x==0, y==0)
     *  -------------------------
     *    Anchor = (0, 0, wz+ZSTEP)
     *    Sprite top-left offset: (-SPRITE_W/2, 0)
     * -------------------------------------------------------- */

    /* We render each z layer bottom (wz=0) to top (wz=height-ZSTEP).
     * Within a layer: corner first, then X-wall, then Y-wall.
     * (Corner has the highest screen depth and draws over both walls.) */

    for (int wz = 0; wz < room->height; wz += ZSTEP) {
        float z_top = (float)(wz + ZSTEP);

        /* --- 2a. Wall-floor transition vs wall body vs wall top --- */
        /* For a room of height H we have H/ZSTEP z-rows.
         * Row 0 (wz==0):           wall-floor transition sprites
         * Row H/ZSTEP-1 (top row): wall-top-edge sprites
         * Middle rows:             wall-body sprites              */
        bool is_bottom = (wz == 0);
        bool is_top    = (wz + ZSTEP >= room->height);

        /* --- X-wall (left): iterate y --- */
        for (int wy = STEP; wy <= room->depth; wy += STEP) {
            bool y_end  = (wy >= room->depth);
            bool y_wall_near_corner = (wy == STEP);

            RoomSpriteID id;
            if (is_bottom) {
                id = y_end              ? SPR_WFLOOR_RIGHT_CORNER :
                     y_wall_near_corner ? SPR_WFLOOR_LEFT_CORNER  :
                                         SPR_WFLOOR_LEFT_EDGE;
            } else if (is_top) {
                id = y_end              ? SPR_WALL_TOP_RIGHT_CORNER :
                     y_wall_near_corner ? SPR_WALL_TOP_LEFT_CORNER  :
                                         SPR_WALL_TOP_LEFT_EDGE;
            } else {
                id = y_end              ? SPR_WALL_RIGHT_CORNER :
                     y_wall_near_corner ? SPR_WALL_LEFT_CORNER  :
                                         SPR_WALL_LEFT_EDGE;
            }

            float sk = tile_sort_key(0.0f, (float)wy, z_top);
            /* Anchor at top-left of wall face in world space.
             * x=0, y=wy, z=wz+ZSTEP → projects to screen left
             * edge of the wall face. Sprite offset: -SPRITE_W px left. */
            push_room_sprite(rc, id,
                             0.0f, (float)wy, z_top,
                             -SPRITE_W, 0,
                             sk);
        }

        /* --- Y-wall (right): iterate x --- */
        for (int wx = STEP; wx <= room->width; wx += STEP) {
            bool x_end  = (wx >= room->width);
            bool x_wall_near_corner = (wx == STEP);

            RoomSpriteID id;
            if (is_bottom) {
                id = x_end              ? SPR_WFLOOR_RIGHT_CORNER :
                     x_wall_near_corner ? SPR_WFLOOR_LEFT_CORNER  :
                                         SPR_WFLOOR_RIGHT_EDGE;
            } else if (is_top) {
                id = x_end              ? SPR_WALL_TOP_RIGHT_CORNER :
                     x_wall_near_corner ? SPR_WALL_TOP_LEFT_CORNER  :
                                         SPR_WALL_TOP_RIGHT_EDGE;
            } else {
                id = x_end              ? SPR_WALL_RIGHT_CORNER :
                     x_wall_near_corner ? SPR_WALL_LEFT_CORNER  :
                                         SPR_WALL_RIGHT_EDGE;
            }

            float sk = tile_sort_key((float)wx, 0.0f, z_top);
            /* Anchor at top of Y-wall face.
             * x=wx+STEP, y=0, z=wz+ZSTEP → sprite offset: (0, 0) */
            push_room_sprite(rc, id,
                             (float)(wx), 0.0f, z_top,
                             0, 0,
                             sk);
        }

        /* --- Back corner column (x==0, y==0) --- */
        {
            RoomSpriteID id;
            if (is_bottom)     id = SPR_WFLOOR_LEFT_CORNER;
            else if (is_top)   id = SPR_WALL_TOP_LEFT_CORNER;
            else               id = SPR_WALL_LEFT_CORNER;

            float sk = tile_sort_key(0.0f, 0.0f, z_top) - 1.0f; /* behind walls */
            push_room_sprite(rc, id,
                             0.0f, 0.0f, z_top,
                             -SPRITE_W / 2, 0,
                             sk);
        }
    }
}