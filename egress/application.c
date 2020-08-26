#include "application.h"

#include "person.h"

#include <math.h>
#include <stdio.h>

struct _topology_settings_t topology_settings = {.topology_path = "topology.json", .type = TOPOLOGY_OBSTACLES, .write_enabled = false, .default_geometry = TOPOLOGY_SQUARE};
struct _abm_settings_t abm_settings = {0, _TRAVERSE, false};

static FILE *filz;

enum{
	OPT_CONFP = 128, /// this tells argp to not assign short options
	OPT_AVGAGE,
	OPT_GROUPP,
	OPT_ENVP,
	OPT_TRUSTP,
	OPT_PSYCHOP
};

const struct argp_option model_options[] = {
		{"confusion-p", OPT_CONFP, "DOUBLE", 0, NULL, 0},
		{"avg-age", 	OPT_AVGAGE, "DOUBLE", 0, NULL, 0},
		{"group-p", 	OPT_GROUPP, "DOUBLE", 0, NULL, 0},
		{"env-p", 	OPT_ENVP, "DOUBLE", 0, NULL, 0},
		{"trust-p", 	OPT_TRUSTP, "DOUBLE", 0, NULL, 0},
		{"psycho-p", 	OPT_PSYCHOP, "DOUBLE", 0, NULL, 0},
		{0}
};

#define HANDLE_ARGP_CASE(label, fmt, var)	\
	case label: \
		if(sscanf(arg, fmt, &var) != 1){ \
			return ARGP_ERR_UNKNOWN; \
		} \
	break

static error_t model_parse(int key, char *arg, struct argp_state *state) {
	(void)state;

	switch (key) {
		HANDLE_ARGP_CASE(OPT_CONFP, "%lf", confusion_p);
		HANDLE_ARGP_CASE(OPT_AVGAGE, "%lf", avg_age);
		HANDLE_ARGP_CASE(OPT_GROUPP, "%lf", group_p);
		HANDLE_ARGP_CASE(OPT_ENVP, "%lf", env_knowledge_p);
		HANDLE_ARGP_CASE(OPT_TRUSTP, "%lf", trust_p);
		HANDLE_ARGP_CASE(OPT_PSYCHOP, "%lf", psycho_infl_p);

		case ARGP_KEY_SUCCESS:
			filz = fopen("escape_stats.txt", "w");
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

struct argp model_argp = {model_options, model_parse, NULL, NULL, NULL, NULL, NULL};


// From Luc Devroye's book "Non-Uniform Random Variate Generation." p. 522
unsigned random_binomial(unsigned trials, double p) { // this is exposed since it is used also in guy.c
	if(p >= 1.0 || !trials)
		return trials;
	unsigned x = 0;
	double sum = 0, log_q = log(1.0 - p); // todo cache those logarithm value
	while(1) {
		sum += log(Random()) / (trials - x);
		if(sum < log_q || trials == x) {
			return x;
		}
		x++;
	}
	return 0;
}

// we handle infects visits move at slightly randomized timesteps 1.0, 2.0, 3.0...
// healthy people are moved at slightly randomized timesteps 0.5, 1.5, 2.5, 3.5...
// this way we preserve the order of operation as in the original model
void ProcessEvent(unsigned int me, simtime_t now, int event_type, union{agent_t *a; unsigned *cell;} pl, unsigned int event_size, region_t *state) {
	switch (event_type) {
		case INIT:
			if(GetValueTopology(me, me))
				break;

			state = malloc(sizeof(region_t));
			SetState(state);
			state->psycho_count = 0;
			state->exit = false;
			state->expl_dist = UINT_MAX;
			memset(state->group_d, UCHAR_MAX, sizeof(state->group_d));

			person_env_init(me);
			person_set_up(state, me, 120);

			unsigned i = EXPLOSIONS_CNT;
			while(i--){
				ScheduleNewEvent(me, explosions[i].time, EXPLOSION, &explosions[i].cell, sizeof(unsigned));
			}
			break;

		case VISIT:
			person_on_visit(now, state, me, *pl.a);
			break;

		case LEAVE:
			person_on_leave(now, state, me, *pl.a);
			break;

		case SAVE:
			KillAgent(*pl.a);
			asaved++;
			break;

		case REFRESH:
			person_refresh_all(state, me);
			ScheduleNewEvent(me, now + Random() * 10, REFRESH, NULL, 0);
			if(now > 10000)
				state->exit = true;
			break;

		case EXPLOSION:
			person_explosion(state, me, *pl.cell);
			break;

		case _TRAVERSE:
		default:
			printf("%s:%d: Unsupported event: %d\n", __FILE__, __LINE__, event_type);
			exit(EXIT_FAILURE);
	}
	if(asaved + adeaths == 9462){
		FILE *tfil = fopen("time.txt", "w");
		fprintf(tfil, "%lf", now);
		fclose(tfil);
	}
	if(asaved + adeaths == 9960){
		fprintf(filz, "%u %u\n", asaved, adeaths);
	}

}

int OnGVT(unsigned int me, region_t *snapshot) {
	if(!snapshot)
		return true;

	if(!CountAgents() || snapshot->exit){
		return true;
	}
	return false;
}
