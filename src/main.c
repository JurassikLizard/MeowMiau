/* main.c
 *
 *  Full pipeline:
 *    1. Load room texture; build RoomLayout from 5 crop numbers
 *       + image dimensions read from the texture.
 *    2. Init SimWorld using the derived room dimensions.
 *    3. Each frame: push background + one circle per integer (x,y)
 *       at z=1 through the Renderer draw-call list, flush once.
 *
 *  All world positions are integer tile coordinates (Vec3W).
 *  The only floating-point is inside room_layout_init() where
 *  pixel sizes are derived.
 */

#include "raylib.h"
#include "spatial/room_layout.h"
#include "render/renderer.h"
#include "sim_world.h"
#include <limits.h>
#include "render/object_renderer.h"

/* ---- viewport ----------------------------------------------- */
#define SCREEN_W  800
#define SCREEN_H  600

/* ---- room image --------------------------------------------- */
#define ROOM_IMAGE_PATH  "assets/room_bg.png"

/* ── The 5 layout numbers ──────────────────────────────────────
 *
 *  Adjust these to match your actual pre-drawn room image.
 *  rhombus_w_px and rhombus_v_px are derived from the image
 *  dimensions together with the padding values below.
 *
 *  Vertical image layout (top → bottom):
 *    | WALL_PAD | WALL_H_PX | RHOMBUS_V_PAD | rhombus_v_px | RHOMBUS_V_PAD |
 *
 *  Horizontal image layout:
 *    | RHOMBUS_H_PAD | rhombus_w_px | RHOMBUS_H_PAD |
 */
#define RHOMBUS_H_PAD    6   /* horizontal padding each side          */
#define WALL_PAD         3   /* dead pixels above usable wall area    */
#define WALL_H_PX       29   /* usable wall pixel height              */
#define RHOMBUS_V_PAD_TOP     3   /* dead pixels above floor rhombus       */
#define RHOMBUS_V_PAD_BOTTOM  6   /* dead pixels below floor rhombus       */
#define ISO_TILE_H_PX     4   /* iso tile face pixel height            */
/* iso_tile_w_px = ISO_TILE_H_PX * 2 = 16 px                          */

/* ---- circle appearance -------------------------------------- */
#define CIRCLE_RADIUS  4
#define CIRCLE_COLOR   ((Color){ 220, 60, 60, 200 })

/* ================================================================
 *  build_circle_texture
 *
 *  Renders a white filled circle on a transparent background into
 *  a (2*radius+2) × (2*radius+2) texture.
 * ================================================================ */
static Texture2D build_circle_texture(int radius)
{
    int side = radius * 2 + 2;
    RenderTexture2D rt = LoadRenderTexture(side, side);
    BeginTextureMode(rt);
        ClearBackground(BLANK);
        DrawCircle(side / 2, side / 2, (float)radius, WHITE);
    EndTextureMode();

    /* OpenGL UV is bottom-up; bake a corrected image. */
    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    UnloadRenderTexture(rt);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

/* ================================================================
 *  push_circle
 *
 *  Projects world_pos to screen via the layout, then pushes a
 *  tinted circle draw call into the renderer.
 * ================================================================ */
static void push_circle(Renderer *r, Texture2D *circle_tex,
                        const RoomLayout *rl,
                        Vec3W world_pos, Color tint)
{
    Vec2S sc   = renderer_project(r, world_pos);
    int   side = circle_tex->width;
    Rectangle src = { 0, 0, (float)side, (float)side };
    Vector2   dst = { (float)(sc.sx - side / 2),
                      (float)(sc.sy - side / 2) };
    renderer_push(r, circle_tex, src, dst,
                  world_sort_key(world_pos), tint);
}

/* ================================================================
 *  main
 * ================================================================ */
int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Room Demo");
    SetTargetFPS(60);

    /* ---- load image ------------------------------------------ */
    Texture2D room_tex = LoadTexture(ROOM_IMAGE_PATH);

    /* ---- build layout (img dimensions come from the texture) - */
    RoomLayout rl = {
        .rhombus_h_pad = RHOMBUS_H_PAD,
        .wall_pad      = WALL_PAD,
        .wall_h_px     = WALL_H_PX,
        .rhombus_v_pad_top    = RHOMBUS_V_PAD_TOP,
        .rhombus_v_pad_bottom = RHOMBUS_V_PAD_BOTTOM,
        .iso_tile_h_px = ISO_TILE_H_PX,
        .vp_w          = SCREEN_W,
        .vp_h          = SCREEN_H,
    };
    
    room_layout_init(&rl, &room_tex);

    /* ---- init simulation ------------------------------------- */
    SimWorld world;
    sim_world_init(&world, rl.room_width, rl.room_depth, rl.room_height);
    sim_world_populate_room(&world);

    /* ---- renderer + circle sprite ---------------------------- */
    Renderer  renderer;
    renderer_init(&renderer, &rl);

    ObjectRenderer obj_renderer;
    object_renderer_init(&obj_renderer, &renderer);

    Texture2D circle_tex = build_circle_texture(CIRCLE_RADIUS);

    /* ================================================================
     *  Main loop
     * ================================================================ */
    while (!WindowShouldClose())
    {
        sim_world_update(&world, GetFrameTime());

        renderer_begin_frame(&renderer);

        /* 1. Background — INT_MIN sort key so it is always first */
        {
            Rectangle src = { 0, 0,
                              (float)room_tex.width,
                              (float)room_tex.height };
            Vector2 dst   = { (float)rl.img_offset_x,
                              (float)rl.img_offset_y };
            renderer_push(&renderer, &room_tex, src, dst, INT_MIN, WHITE);
        }

        /* 2. Circles at every integer (x,y) on z=1
         *      x : 0 .. room_width   (inclusive)
         *      y : 0 .. room_depth   (inclusive)
         *    +x goes down-right, +y goes down-left, z=1 is one tile up */
        for (int iy = 0; iy <= rl.room_depth; iy++)
            for (int ix = 0; ix <= rl.room_width; ix++)
                push_circle(&renderer, &circle_tex, &rl,
                            (Vec3W){ ix, iy, 1 }, CIRCLE_COLOR);

        BeginDrawing();
            ClearBackground(BLACK);
            renderer_flush(&renderer);

            /* ---- debug overlays --------------------------------
             *  All drawn at z=0 (floor level) so any z-lift in
             *  the circles shows up as a clear vertical offset.
             *
             *  Magenta cross : raw screen origin (origin_sx, origin_sy)
             *                  = world (0,0,0) projected
             *  Green  dot    : world (room_width, 0, 0)  — +x tip
             *  Blue   dot    : world (0, room_depth, 0)  — +y tip
             */
            {
                /* Magenta cross at origin */
                int ox = rl.origin_sx, oy = rl.origin_sy;
                DrawLine(ox - 6, oy,     ox + 6, oy,     MAGENTA);
                DrawLine(ox,     oy - 6, ox,     oy + 6, MAGENTA);
                DrawText("(0,0,0)", ox + 4, oy - 12, 12, MAGENTA);

                /* +x tip */
                Vec2S px = renderer_project(&renderer, (Vec3W){ rl.room_width, 0, 0 });
                DrawCircle(px.sx, px.sy, 4, GREEN);
                DrawText(TextFormat("(%d,0,0)", rl.room_width),
                         px.sx + 4, px.sy - 12, 12, GREEN);

                /* +y tip */
                Vec2S py = renderer_project(&renderer, (Vec3W){ 0, rl.room_depth, 0 });
                DrawCircle(py.sx, py.sy, 4, BLUE);
                DrawText(TextFormat("(0,%d,0)", rl.room_depth),
                         py.sx + 4, py.sy - 12, 12, BLUE);
            }

            DrawText(TextFormat(
                "tile %dpx x %dpx  |  room %d x %d x %d",
                rl.iso_tile_w_px, rl.iso_tile_h_px,
                rl.room_width, rl.room_depth, rl.room_height),
                8, 8, 16, RAYWHITE);
        EndDrawing();
    }

    UnloadTexture(circle_tex);
    UnloadTexture(room_tex);
    sim_world_destroy(&world);
    CloseWindow();
    return 0;
}