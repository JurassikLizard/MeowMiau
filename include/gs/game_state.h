#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "gs/object_info.h"
#include "gs/map_info.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================
   API
   ============================ */

bool             game_state_load(const char *object_info_path, const char *map_info_path);
void             game_state_unload(void);

const ObjectInfo *game_state_objinfo_by_id(uint32_t id);
MapInfo          *game_state_map(void);

#endif // GAME_STATE_H