#include "render/renderer.h"
#include <string.h>
#include <stdlib.h>

void renderer_init(Renderer *r, const RoomLayout *layout)
{
    memset(r, 0, sizeof(*r));
    r->layout = layout;

    float sx = (float)layout->vp_w / (float)layout->img_w;
    float sy = (float)layout->vp_h / (float)layout->img_h;
    r->scale = (sx < sy ? sx : sy) * RENDER_SCALE_PAD;
}

void renderer_begin_frame(Renderer *r)
{
    r->call_count = 0;
}

void renderer_push(Renderer *r, Texture2D *tex, Rectangle src,
                   Vector2 dst, int sort_key, Color tint)
{
    if (r->call_count >= DRAWCALL_MAX) return;
    DrawCall *dc = &r->calls[r->call_count++];
    dc->tex      = tex;
    dc->src      = src;
    dc->dst      = dst;
    dc->sort_key = sort_key;
    dc->tint     = tint;
}

static int draw_call_cmp(const void *a, const void *b)
{
    int ka = ((const DrawCall *)a)->sort_key;
    int kb = ((const DrawCall *)b)->sort_key;
    return (ka > kb) - (ka < kb);
}

void renderer_flush(Renderer *r)
{
    qsort(r->calls, (size_t)r->call_count, sizeof(DrawCall), draw_call_cmp);

    const float s = r->scale;
    const float ox = r->layout->origin_sx;
    const float oy = r->layout->origin_sy;

    for (int i = 0; i < r->call_count; i++) {
        DrawCall *dc = &r->calls[i];

        float dx = ox + (dc->dst.x - ox) * s;
        float dy = oy + (dc->dst.y - oy) * s;

        DrawTexturePro(
            *dc->tex,
            dc->src,
            (Rectangle){
                dx,
                dy,
                dc->src.width  * s,
                dc->src.height * s
            },
            (Vector2){0,0},
            0.0f,
            dc->tint
        );
    }
}