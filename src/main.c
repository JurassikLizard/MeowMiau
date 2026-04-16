/* main.c
 *
 *  Pipeline:
 *    1. Load room texture; build RoomLayout from crop constants.
 *    2. Load scene (types + instances) from a single TOML file.
 *    3. Each frame: push background + all scene objects into the
 *       Renderer draw-call list, flush once (sorted back-to-front).
 *
 *  All world positions are integer tile coordinates (Vec3W).
 *  The only floating-point is inside room_layout_init() where
 *  pixel sizes are derived from image dimensions.
 */

#include "raylib.h"
#include "spatial/room_layout.h"
#include "render/renderer.h"
#include "scene_object.h"
#include <limits.h>

/* ---- viewport ----------------------------------------------- */
#define SCREEN_W  800
#define SCREEN_H  600

/* ---- assets ------------------------------------------------- */
#define ROOM_IMAGE_PATH  "assets/room_bg.png"
#define SCENE_TOML_PATH  "assets/data/scene.toml"

/* ── Room layout crop constants ────────────────────────────────
 *  Adjust to match the actual pre-drawn room image.
 */
#define RHOMBUS_H_PAD         6
#define WALL_PAD              3
#define WALL_H_PX            29
#define RHOMBUS_V_PAD_TOP     3
#define RHOMBUS_V_PAD_BOTTOM  6
#define ISO_TILE_H_PX         4

/* ---- debug circle ------------------------------------------- */
#define CIRCLE_RADIUS  4
#define CIRCLE_COLOR   ((Color){ 220, 60, 60, 200 })

/* ================================================================
 *  build_circle_texture  (debug helper, unchanged from original)
 * ================================================================ */
static Texture2D build_circle_texture(int radius)
{
    int side = radius * 2 + 2;
    RenderTexture2D rt = LoadRenderTexture(side, side);
    BeginTextureMode(rt);
        ClearBackground(BLANK);
        DrawCircle(side / 2, side / 2, (float)radius, WHITE);
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    UnloadRenderTexture(rt);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

/* ================================================================
 *  push_circle  (debug helper, unchanged from original)
 * ================================================================ */
static void push_circle(Renderer *r, Texture2D *circle_tex,
                        Vec3W world_pos, Color tint)
{
    Vec2S sc   = renderer_project(r, world_pos);
    int   side = circle_tex->width;
    Rectangle src = { 0, 0, (float)side, (float)side };
    Vector2   dst = { (float)(sc.sx - side / 2),
                      (float)(sc.sy - side / 2) };
    renderer_push(r, circle_tex, src, dst, world_sort_key(world_pos), tint);
}

/* ================================================================
 *  main
 * ================================================================ */
int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Room Demo");
    SetTargetFPS(60);

    /* ---- room image ------------------------------------------ */
    Texture2D room_tex = LoadTexture(ROOM_IMAGE_PATH);

    /* ---- room layout ----------------------------------------- */
    RoomLayout rl = {
        .rhombus_h_pad        = RHOMBUS_H_PAD,
        .wall_pad             = WALL_PAD,
        .wall_h_px            = WALL_H_PX,
        .rhombus_v_pad_top    = RHOMBUS_V_PAD_TOP,
        .rhombus_v_pad_bottom = RHOMBUS_V_PAD_BOTTOM,
        .iso_tile_h_px        = ISO_TILE_H_PX,
        .vp_w                 = SCREEN_W,
        .vp_h                 = SCREEN_H,
    };
    room_layout_init(&rl, &room_tex);

    /* ---- renderer -------------------------------------------- */
    Renderer renderer;
    renderer_init(&renderer, &rl);

    /* ---- scene (types + instances from one TOML) ------------- */
    Scene scene;
    if (!scene_load_toml(&scene, SCENE_TOML_PATH))
        return 1;

    /* ---- debug circle sprite --------------------------------- */
    Texture2D circle_tex = build_circle_texture(CIRCLE_RADIUS);

    /* ================================================================
     *  Main loop
     * ================================================================ */
    while (!WindowShouldClose())
    {
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

        /* 2. All scene objects */
        scene_push_objects(&scene, &renderer);

        /* 3. Debug: tile-corner circles at z=1 */
        for (int iy = 0; iy <= rl.room_depth; iy++)
            for (int ix = 0; ix <= rl.room_width; ix++)
                push_circle(&renderer, &circle_tex,
                            (Vec3W){ ix, iy, 1 }, CIRCLE_COLOR);

        BeginDrawing();
            ClearBackground(BLACK);
            renderer_flush(&renderer);

            /* ---- debug overlays -------------------------------- */
            {
                int ox = rl.origin_sx, oy = rl.origin_sy;
                DrawLine(ox - 6, oy,     ox + 6, oy,     MAGENTA);
                DrawLine(ox,     oy - 6, ox,     oy + 6, MAGENTA);
                DrawText("(0,0,0)", ox + 4, oy - 12, 12, MAGENTA);

                Vec2S px = renderer_project_debug(&renderer, (Vec3W){ rl.room_width, 0, 0 });
                DrawCircle(px.sx, px.sy, 4, GREEN);
                DrawText(TextFormat("(%d,0,0)", rl.room_width),
                         px.sx + 4, px.sy - 12, 12, GREEN);

                Vec2S py = renderer_project_debug(&renderer, (Vec3W){ 0, rl.room_depth, 0 });
                DrawCircle(py.sx, py.sy, 4, BLUE);
                DrawText(TextFormat("(0,%d,0)", rl.room_depth),
                         py.sx + 4, py.sy - 12, 12, BLUE);
            }

            DrawText(TextFormat(
                "tile %dpx x %dpx  |  room %d x %d x %d  |  objects %d",
                rl.iso_tile_w_px, rl.iso_tile_h_px,
                rl.room_width, rl.room_depth, rl.room_height,
                scene.object_count),
                8, 8, 16, RAYWHITE);
        EndDrawing();
    }

    UnloadTexture(circle_tex);
    UnloadTexture(room_tex);
    scene_unload(&scene);
    CloseWindow();
    return 0;
}