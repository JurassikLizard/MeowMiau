#include "scene_object.h"
#include "render/renderer.h"
#include "toml/tomlc17.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 *  TOML helpers
 * ============================================================ */

static int toml_int(toml_datum_t tbl, const char *key, int def)
{
    toml_datum_t v = toml_get(tbl, key);
    return (v.type == TOML_INT64) ? (int)v.u.int64 : def;
}

static const char *toml_str(toml_datum_t tbl, const char *key)
{
    toml_datum_t v = toml_get(tbl, key);
    return (v.type == TOML_STRING) ? v.u.s : NULL;
}

/* ============================================================
 *  scene_load_toml
 *
 *  Expected TOML layout:
 *
 *  [[type]]
 *  name    = "bookshelf"
 *  texture = "assets/bookshelf.png"
 *  size    = { x=2, y=1, z=2 }
 *  render_offset = { x=0, y=0, z=0 }
 *  tex_origin    = { x=8, y=30 }
 *
 *  [[object]]
 *  type = "bookshelf"
 *  pos  = { x=3, y=0, z=0 }
 *
 *  Types must appear before objects in the file so that the
 *  pointer from SceneObject.type is valid when instances are
 *  resolved. (A two-pass load would lift this constraint but
 *  adds complexity for no practical gain here.)
 * ============================================================ */
bool scene_load_toml(Scene *s, const char *path)
{
    memset(s, 0, sizeof(*s));

    toml_result_t result = toml_parse_file_ex(path);
    if (!result.ok) {
        fprintf(stderr, "scene: TOML parse error in '%s': %s\n",
                path, result.errmsg);
        return false;
    }

    /* ---- types ---- */
    toml_datum_t type_arr = toml_get(result.toptab, "type");
    if (type_arr.type == TOML_ARRAY) {
        for (int i = 0; i < type_arr.u.arr.size; i++) {
            if (s->type_count >= SCENE_MAX_TYPES) {
                fprintf(stderr, "scene: too many types (max %d)\n", SCENE_MAX_TYPES);
                break;
            }
            toml_datum_t t = type_arr.u.arr.elem[i];
            if (t.type != TOML_TABLE) continue;

            ObjectType *ot = &s->types[s->type_count++];
            memset(ot, 0, sizeof(*ot));

            /* name */
            const char *name = toml_str(t, "name");
            if (!name) {
                fprintf(stderr, "scene: type[%d] missing 'name', skipping\n", i);
                s->type_count--;
                continue;
            }
            strncpy(ot->name, name, sizeof(ot->name) - 1);

            /* texture */
            const char *tex_path = toml_str(t, "texture");
            if (tex_path) {
                ot->texture  = LoadTexture(tex_path);
                ot->src_rect = (Rectangle){
                    0, 0,
                    (float)ot->texture.width,
                    (float)ot->texture.height
                };
            }

            /* size */
            toml_datum_t sz = toml_get(t, "size");
            if (sz.type == TOML_TABLE) {
                ot->size.x = toml_int(sz, "x", 1);
                ot->size.y = toml_int(sz, "y", 1);
                ot->size.z = toml_int(sz, "z", 1);
            } else {
                ot->size = (Vec3W){ 1, 1, 1 };
            }

            /* render_offset */
            toml_datum_t ro = toml_get(t, "render_offset");
            if (ro.type == TOML_TABLE) {
                ot->render_offset.x = toml_int(ro, "x", 0);
                ot->render_offset.y = toml_int(ro, "y", 0);
                ot->render_offset.z = toml_int(ro, "z", 0);
            }

            /* tex_origin */
            toml_datum_t to = toml_get(t, "tex_origin");
            if (to.type == TOML_TABLE) {
                ot->tex_origin.sx = toml_int(to, "x", 0);
                ot->tex_origin.sy = toml_int(to, "y", 0);
            }
        }
    }

    /* ---- instances ---- */
    toml_datum_t obj_arr = toml_get(result.toptab, "object");
    if (obj_arr.type == TOML_ARRAY) {
        for (int i = 0; i < obj_arr.u.arr.size; i++) {
            if (s->object_count >= SCENE_MAX_OBJECTS) {
                fprintf(stderr, "scene: too many objects (max %d)\n", SCENE_MAX_OBJECTS);
                break;
            }
            toml_datum_t o = obj_arr.u.arr.elem[i];
            if (o.type != TOML_TABLE) continue;

            const char *type_name = toml_str(o, "type");
            if (!type_name) {
                fprintf(stderr, "scene: object[%d] missing 'type', skipping\n", i);
                continue;
            }

            ObjectType *ot = scene_type_by_name(s, type_name);
            if (!ot) {
                fprintf(stderr, "scene: object[%d] references unknown type '%s', skipping\n",
                        i, type_name);
                continue;
            }

            SceneObject *so = &s->objects[s->object_count++];
            so->type = ot;

            toml_datum_t pos = toml_get(o, "pos");
            if (pos.type == TOML_TABLE) {
                so->pos.x = toml_int(pos, "x", 0);
                so->pos.y = toml_int(pos, "y", 0);
                so->pos.z = toml_int(pos, "z", 0);
            }
        }
    }

    toml_free(result);
    return true;
}

/* ============================================================
 *  scene_unload
 * ============================================================ */
void scene_unload(Scene *s)
{
    for (int i = 0; i < s->type_count; i++)
        UnloadTexture(s->types[i].texture);
    memset(s, 0, sizeof(*s));
}

/* ============================================================
 *  scene_push_objects
 *
 *  For each instance:
 *    1. Compute anchor world pos = instance.pos + type.render_offset
 *    2. Project anchor to screen
 *    3. Subtract tex_origin to get top-left draw position
 *    4. Compute sort key from instance pos + type size centre
 *    5. Push draw call
 * ============================================================ */
void scene_push_objects(const Scene *s, Renderer *r)
{
    for (int i = 0; i < s->object_count; i++) {
        const SceneObject *so = &s->objects[i];
        const ObjectType  *ot = so->type;
        if (!ot || ot->texture.id == 0) continue;

        Vec3W anchor = {
            so->pos.x + ot->render_offset.x,
            so->pos.y + ot->render_offset.y,
            so->pos.z + ot->render_offset.z,
        };
        Vec2S screen = renderer_project(r, anchor);

        Vector2 dst = {
            (float)(screen.sx - ot->tex_origin.sx),
            (float)(screen.sy - ot->tex_origin.sy),
        };

        /* Sort by the back corner of the object's AABB */
        Vec3W corner = {
            so->pos.x,
            so->pos.y,
            so->pos.z,
        };
        int sort_key = world_sort_key(corner);

        renderer_push(r, (Texture2D *)&ot->texture, ot->src_rect, dst, sort_key, WHITE);
    }
}

/* ============================================================
 *  Lookup helpers
 * ============================================================ */

ObjectType *scene_type_by_name(Scene *s, const char *name)
{
    for (int i = 0; i < s->type_count; i++)
        if (strncmp(s->types[i].name, name, sizeof(s->types[i].name)) == 0)
            return &s->types[i];
    return NULL;
}

SceneObject *scene_object_at_tile(Scene *s, int tx, int ty)
{
    for (int i = 0; i < s->object_count; i++) {
        SceneObject *so = &s->objects[i];
        const ObjectType *ot = so->type;
        if (tx >= so->pos.x && tx < so->pos.x + ot->size.x &&
            ty >= so->pos.y && ty < so->pos.y + ot->size.y)
            return so;
    }
    return NULL;
}

/* ============================================================
 *  Instance management
 * ============================================================ */

SceneObject *scene_spawn(Scene *s, const char *type_name, Vec3W pos)
{
    if (s->object_count >= SCENE_MAX_OBJECTS) {
        fprintf(stderr, "scene_spawn: object limit reached\n");
        return NULL;
    }
    ObjectType *ot = scene_type_by_name(s, type_name);
    if (!ot) {
        fprintf(stderr, "scene_spawn: unknown type '%s'\n", type_name);
        return NULL;
    }
    SceneObject *so = &s->objects[s->object_count++];
    so->type = ot;
    so->pos  = pos;
    return so;
}

void scene_remove(Scene *s, SceneObject *obj)
{
    int idx = (int)(obj - s->objects);
    if (idx < 0 || idx >= s->object_count) return;
    s->objects[idx] = s->objects[--s->object_count];
}