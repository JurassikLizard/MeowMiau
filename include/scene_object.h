#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "raylib.h"
#include "world.h"
#include "render/renderer.h"
#include <stdbool.h>
#include <stdint.h>

/* ============================================================
 *  ObjectType
 *
 *  Defines the visual and spatial template for a class of object.
 *  Loaded once from the [[type]] sections of scene.toml.
 *  Instances reference their type by name.
 *
 *  Fields that belong to the *class* of thing, not the instance:
 *    - texture, src_rect          what to draw
 *    - size                       AABB in tile units (for sorting)
 *    - render_offset              world-tile offset from instance pos
 *                                 to the texture anchor point
 *    - tex_origin                 pixel in the texture that lands on
 *                                 the projected render_offset point
 * ============================================================ */
typedef struct {
    char       name[64];         /* unique key; instances reference this */

    Texture2D  texture;
    Rectangle  src_rect;         /* sub-rect within texture (full tex if type has one sprite) */

    Vec3W      size;             /* AABB extents in tile units */
    Vec3W      render_offset;    /* world-tile offset: anchor = pos + render_offset */
    Vec2S      tex_origin;       /* pixel in texture that sits at the projected anchor */
} ObjectType;

/* ============================================================
 *  SceneObject
 *
 *  A placed instance. Owns nothing — the ObjectType owns the texture.
 *  pos is mutable every frame for dynamic objects (cat, etc.).
 * ============================================================ */
typedef struct {
    const ObjectType *type;      /* non-owning; points into Scene.types[] */
    Vec3W             pos;       /* tile position, back-bottom corner */
} SceneObject;

/* ============================================================
 *  Scene
 *
 *  Flat arrays; no heap allocation after load.
 *  To remove an instance: swap with last, decrement count.
 * ============================================================ */
#define SCENE_MAX_TYPES    64
#define SCENE_MAX_OBJECTS  256

typedef struct {
    ObjectType  types  [SCENE_MAX_TYPES];
    int         type_count;

    SceneObject objects[SCENE_MAX_OBJECTS];
    int         object_count;
} Scene;

/* ============================================================
 *  API
 * ============================================================ */

/* Load types and instances from a single TOML file.
 * Textures are loaded here via raylib LoadTexture().
 * Returns false and prints to stderr on failure. */
bool scene_load_toml(Scene *s, const char *path);

/* Unload all textures and zero the scene. */
void scene_unload(Scene *s);

/* Push all visible objects into the renderer this frame.
 * No sorting here — renderer_flush() handles that. */
void scene_push_objects(const Scene *s, Renderer *r);

/* Lookup helpers */
ObjectType  *scene_type_by_name  (Scene *s, const char *name);
SceneObject *scene_object_at_tile(Scene *s, int tx, int ty); /* first hit, or NULL */


/* Instance management */
SceneObject *scene_spawn (Scene *s, const char *type_name, Vec3W pos);
void         scene_remove(Scene *s, SceneObject *obj); /* swap-remove */

#endif /* SCENE_OBJECT_H */