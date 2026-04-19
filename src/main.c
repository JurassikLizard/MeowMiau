/* main.c */

#include "raylib.h"
#include "spatial/room_layout.h"
#include "render/renderer.h"
#include "scene_object.h"
#include <limits.h>
#include <string.h>

/* ---- viewport ----------------------------------------------- */
#define SCREEN_W  800
#define SCREEN_H  600

/* ---- assets ------------------------------------------------- */
#define ROOM_IMAGE_PATH        "assets/room_bg.png"
#define SCENE_TOML_PATH        "assets/data/scene.toml"
#define DEBUG_CIRCLE_PNG_PATH  "assets/debug_circle.png"

/* ---- room layout crop constants ----------------------------- */
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
 *  export_circle_texture
 *
 *  Builds a small circle sprite and writes it to disk as a PNG so
 *  scene.toml can reference it as a normal "debug_circle" type.
 *  Safe to call every launch — ExportImage is a no-op if unchanged.
 * ================================================================ */
static void export_circle_texture(int radius, const char *path)
{
    int side = radius * 2 + 2;
    RenderTexture2D rt = LoadRenderTexture(side, side);
    BeginTextureMode(rt);
        ClearBackground(BLANK);
        DrawCircle(side / 2, side / 2, (float)radius, CIRCLE_COLOR);
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);
    ExportImage(img, path);
    UnloadImage(img);
    UnloadRenderTexture(rt);
}

/* ================================================================
 *  register_debug_circle_type
 *
 *  Injects the "debug_circle" ObjectType directly into the scene
 *  without going through TOML. The PNG must already exist on disk.
 *  tex_origin is set to the centre of the sprite so the circle
 *  sits on the projected tile corner.
 * ================================================================ */
static void register_debug_circle_type(Scene *s, const char *png_path)
{
    if (s->type_count >= SCENE_MAX_TYPES) {
        TraceLog(LOG_ERROR, "register_debug_circle_type: type limit reached\n");
        return;
    }
    ObjectType *ot = &s->types[s->type_count++];
    memset(ot, 0, sizeof(*ot));

    strncpy(ot->name, "debug_circle", sizeof(ot->name) - 1);
    ot->texture  = LoadTexture(png_path);
    ot->src_rect = (Rectangle){ 0, 0,
                                (float)ot->texture.width,
                                (float)ot->texture.height };
    ot->size          = (Vec3W){ 1, 1, 1 };
    ot->render_offset = (Vec3W){ 0, 0, 0 };
    /* Centre of sprite lands on the projected point. */
    ot->tex_origin    = (Vec2S){ ot->texture.width  / 2,
                                 ot->texture.height / 2 };
}

/* ================================================================
 *  spawn_debug_circles
 *
 *  One "debug_circle" object per tile corner at z=1.
 * ================================================================ */
static void spawn_debug_circles(Scene *s, const RoomLayout *rl)
{
    for (int iy = 0; iy <= rl->room_depth; iy++)
        for (int ix = 0; ix <= rl->room_width; ix++)
            scene_spawn(s, "debug_circle", (Vec3W){ ix, iy, 1 });
}

/* ================================================================
 *  main
 * ================================================================ */
int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Room Demo");
    SetTargetFPS(60);

    /* ---- export debug circle PNG (once; cached on disk) ------ */
    export_circle_texture(CIRCLE_RADIUS, DEBUG_CIRCLE_PNG_PATH);

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

    /* ---- scene (types + instances from TOML) ----------------- */
    Scene scene;
    if (!scene_load_toml(&scene, SCENE_TOML_PATH))
        return 1;

    /* ---- debug circles (type registered, instances spawned) -- */
    register_debug_circle_type(&scene, DEBUG_CIRCLE_PNG_PATH);
    spawn_debug_circles(&scene, &rl);

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

        /* 2. All scene objects (auto-sorts on first frame / when dirty) */
        scene_push_objects(&scene, &renderer);

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

    UnloadTexture(room_tex);
    scene_unload(&scene);
    CloseWindow();
    return 0;
}