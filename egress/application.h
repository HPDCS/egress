#pragma once

#include <ROOT-Sim.h>

#include "parameters.h"

typedef struct _region_t{
	unsigned saved;
	unsigned deaths;
	unsigned psycho_count;
	unsigned expl_dist;
	unsigned group_d[GROUPS_CNT];
	bool exit;
} region_t;

enum _event_t{
	VISIT = INIT + 1,
	LEAVE,
	SAVE,
	REFRESH,
	EXPLOSION,
	_TRAVERSE
};
