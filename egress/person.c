#include "person.h"

#include "application.h"
#include "parameters.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define M_E		2.7182818284590452354	/* e */


static struct {
	unsigned nearest_exit;
	unsigned room_id;
	unsigned exit_dis;
	bool can_see_exit;
} env_info [EDGE_LEN * EDGE_LEN];

void person_set_up(region_t *region, unsigned region_id, unsigned count)
{
	if(env_info[region_id].room_id != 3)
		return;

	agent_t agent = SpawnAgent(sizeof(person_t));
	person_t *p = DataAgent(agent, NULL);
	memset(p, 0, sizeof(*p));

	p->role = role_gleader;
	p->age = MIN_AGE + (avg_age - MIN_AGE) * 2 * Random();
	p->emotional_f = Random();
	p->group_i = region_id + 1;
	p->destination = FindReceiver();
	count--;
	ScheduleNewLeaveEvent((Random() * p->age / 5), LEAVE, agent);

	double prob = PSYCHO_P + RESCUER_P + group_p;

	while(count--){
		agent = SpawnAgent(sizeof(person_t));
		p = DataAgent(agent, NULL);
		memset(p, 0, sizeof(*p));
		p->role = role_on_my_own;
		p->age = MIN_AGE + (avg_age - MIN_AGE) * 2 * Random();
		p->emotional_f = Random();
		p->destination = FindReceiver();
		if(Random() < prob){
			double r = Random();
			if(r < PSYCHO_P / prob){
				p->role = role_psycho;
				p->destination = exits[(unsigned)(Random() * EXITS_CNT)];
				region->psycho_count++;
			} else if(r < (PSYCHO_P + RESCUER_P) / prob) {
				p->destination = env_info[region_id].nearest_exit;
				p->role = role_rescuer;
				p->group_i = 0;
			} else {
				p->role = role_gfollower;
				p->group_i = region_id + 1;
			}
		}

		ScheduleNewLeaveEvent((Random() * p->age / 5), LEAVE, agent);
	}
}

void person_env_init(unsigned region_id)
{
	unsigned y = region_id / EDGE_LEN;
	unsigned x = region_id - y * EDGE_LEN;
	unsigned i = ROOMS_CNT;
	while(i--){
		if(
			x >= rooms[i].x_min &&
			x <= rooms[i].x_max &&
			y >= rooms[i].y_min &&
			y <= rooms[i].y_max){
			break;
		}
	}
	env_info[region_id].room_id = i;
	env_info[region_id].can_see_exit = false;

	const struct _r_t *r = &rooms[i];

	unsigned path[RegionsCount()];
	unsigned reg_min = EXITS_CNT;
	unsigned hops_min = UINT_MAX;

	i = EXITS_CNT;
	while(i--){
		const unsigned t_exit = exits[i];
		y = t_exit / EDGE_LEN;
		x = t_exit - y * EDGE_LEN;

		env_info[region_id].can_see_exit =
			x >= r->x_min &&
			x <= r->x_max &&
			y >= r->y_min &&
			y <= r->y_max;

		if(region_id == t_exit){
			reg_min = region_id;
			break;
		}
		unsigned hops = ComputeMinTour(region_id, t_exit, path);
		reg_min = hops_min <= hops ? reg_min : t_exit;
		hops_min = hops_min <= hops ? hops_min : hops;
	}

	env_info[region_id].nearest_exit = reg_min;
	env_info[region_id].exit_dis = hops_min;
}

void person_refresh_all(region_t *region, unsigned region_id)
{
	unsigned hops = region->expl_dist;
	agent_t agent;
	IterAgents(NULL);

	double psy_p = pow(1 - psycho_infl_p, region->psycho_count);

	while(IterAgents(&agent)){

		person_t *p = DataAgent(agent, NULL);

		if(Random() < 10.0 / hops / hops){
			KillAgent(agent);
			adeaths++;
			if(p->role == role_psycho)
				region->psycho_count--;
			continue;
		}

		switch(p->role){
		case role_psycho:
		case role_rescuer:
			break;

		case role_on_my_own:
		case role_gfollower:
		case role_gleader:
			;
			double f;
			if(Random() > psy_p){
				p->destination = FindReceiver();
				f = 1;
			} else {
				if(env_info->can_see_exit){
					f = p->emotional_f *
						env_info[region_id].exit_dis / (double)(CountAgents() * hops);
					f = pow(1 + f, 1/f) / M_E;
				} else {
					f = 1;
				}
			}
			double emo = EMO_MVG_AVG * f + (1 - EMO_MVG_AVG) * p->emotion;
			p->emotion = emo;
			break;
		}
	}
}

void person_on_visit(simtime_t now, region_t *region, unsigned region_id, agent_t agent)
{
	person_t *p = DataAgent(agent, NULL);
	ScheduleNewLeaveEvent(now + (Random() * p->age / 5), LEAVE, agent);

	if(p->role == role_psycho)
		region->psycho_count++;
}

static void check_memory(person_t *p, unsigned region_id)
{
	unsigned r_id = env_info[region_id].room_id;
	bool memory = Random() < MEMORY_P;
	while (memory && p->destination == region_id){
		p->destination = rooms[r_id].doors[(unsigned)(Random() * DOORS_LEN)];
	}
}

void person_on_leave(simtime_t now, region_t *region, unsigned region_id, agent_t agent)
{
	person_t *p = DataAgent(agent, NULL);

	if(Random() < (p->age * p->age / (10 * MAX_AGE * MAX_AGE))){
		ScheduleNewLeaveEvent(now + (Random() * p->age / 5), LEAVE, agent);
		return;
	}

	if(Random() < confusion_p){
		ScheduleNewLeaveEvent(now + (Random() * p->age / 5), LEAVE, agent);
		return;
	}

	unsigned dest;
	if(p->emotion > EMO_THRESH){
		dest = FindReceiver();
		if(p->role == role_gleader){
			region->group_d[p->group_i] = dest;
		}
	} else {
		switch(p->role){
		case role_psycho:
			if(Random() < 0.3)
				dest = FindReceiver();
			else
				dest = FindReceiverToward(p->destination);
			region->psycho_count--;
			break;
		case role_gleader:
			if(region->group_d[0] != UINT_MAX && Random() < trust_p){
				p->destination = region->group_d[0];
			} else {
				if(p->destination == region_id){
					if(Random() < env_knowledge_p)
						p->destination = exits[(unsigned)(Random() * EXITS_CNT)];
					else
						check_memory(p, region_id);
				}
			}
			/* fallthrough */
		case role_rescuer:
			dest = FindReceiverToward(p->destination);
			region->group_d[p->group_i] = dest;
			break;
		case role_on_my_own:
			if(region->group_d[0] != UINT_MAX && Random() < trust_p){
				p->destination = region->group_d[0];
			} else {
				if(p->destination == region_id){
					if(Random() < env_knowledge_p)
						p->destination = exits[(unsigned)(Random() * EXITS_CNT)];
					else
						check_memory(p, region_id);
				}
			}
			dest = FindReceiverToward(p->destination);
			break;
		case role_gfollower:
			dest = region->group_d[p->group_i];
			p->role = role_on_my_own;
		}
	}

	if(dest == UINT_MAX)
		dest = FindReceiver();

	unsigned i = EXITS_CNT;
	if (region->expl_dist != UINT_MAX)
	while(i--){
		if(exits[i] == dest){
			EnqueueVisit(agent, dest, SAVE);
			return;
		}
	}
	EnqueueVisit(agent, dest, VISIT);
}

void person_explosion(region_t *region, unsigned region_id, unsigned explosion_cell)
{
	unsigned path[RegionsCount()];
	unsigned hops = (region_id != explosion_cell) ? ComputeMinTour(region_id, explosion_cell, path) : 0;
	hops = (hops < region->expl_dist) ? hops : region->expl_dist;
	region->expl_dist = hops;

	person_refresh_all(region, region_id);
}

