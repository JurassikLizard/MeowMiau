#include "gs/game_state.h"
#include <stdio.h>

typedef struct {
    ObjectInfoRegistry object_registry;
    MapInfo            map;
} GameState;

static GameState g_state = {0};

bool game_state_load(const char *object_info_path, const char *map_info_path)
{
    if (!objinfo_load_from_toml(&g_state.object_registry, object_info_path)) {
        fprintf(stderr, "game_state: failed to load object info from '%s'\n",
                object_info_path);
        return false;
    }

    if (!mapinfo_load_from_toml(&g_state.map, map_info_path)) {
        fprintf(stderr, "game_state: failed to load map info from '%s'\n",
                map_info_path);
        objinfo_unload(&g_state.object_registry);
        return false;
    }

    return true;
}

void game_state_unload(void)
{
    objinfo_unload(&g_state.object_registry);
    mapinfo_free(&g_state.map);
}

const ObjectInfo *game_state_objinfo_by_id(uint32_t id)
{
    return objinfo_by_id(&g_state.object_registry, id);
}

MapInfo *game_state_map(void)
{
    return &g_state.map;
}