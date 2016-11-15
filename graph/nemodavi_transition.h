#ifndef __NEMODAVI_TRANS_H__
#define __NEMODAVI_TRANS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nemolist.h>
#include <showone.h>
#include <showtransition.h>

#include "nemodavi.h"
#include "nemodavi_selector.h"

struct nemodavi_transition {
	struct nemodavi *davi;

	struct nemolist actor_list; 
	struct nemolist link;

	// flag for layer setting 
	uint32_t above;
};

struct nemodavi_frame  {
	struct showone *sframe;
	double timing;

	struct nemolist link;
};

typedef void (*nemodavi_transition_set_done_t)(int index, void *datum, void *userdata);

struct nemodavi_actor {
	uint32_t index;
	struct nemolist frame_list;

	struct showtransition *strans;
	struct nemodavi_datum *datum;

	uint32_t duration;
	uint32_t delay;
	uint32_t easetype;
	uint32_t repeat;

	nemodavi_transition_set_done_t done_handler;
	void *done_data;	

	struct nemolist link;
};

struct nemodavi_set {
	struct showone *sset;
	double timing;
	struct nemodavi_transition *trans;

	struct nemolist link;
};

typedef uint32_t (*nemodavi_transition_set_cattr_handler_t)(int index, void *datum, void *userdata);
typedef double (*nemodavi_transition_set_dattr_handler_t)(int index, void *datum, void *userdata);

typedef uint32_t (*nemodavi_transition_set_delay_handler_t)(int index, void *datum, void *userdata);
typedef uint32_t (*nemodavi_transition_set_duration_handler_t)(int index, void *datum, void *userdata);
typedef uint32_t (*nemodavi_transition_set_repeat_handler_t)(int index, void *datum, void *userdata);
typedef uint32_t (*nemodavi_transition_set_ease_handler_t)(int index, void *datum, void *userdata);

// create, run animation
extern struct nemodavi_transition* nemodavi_transition_create(struct nemodavi *davi);
extern int nemodavi_transition_run(struct nemodavi *davi);
extern void nemodavi_transition_clear(struct nemodavi *davi);

// set delay, repeat, ease, done callback 
extern int nemodavi_transition_set_delay(struct nemodavi_transition *trans, uint32_t delay);
extern int nemodavi_transition_set_duration(struct nemodavi_transition *trans, uint32_t duration);
extern int nemodavi_transition_set_repeat(struct nemodavi_transition *trans, int repeat);
extern int nemodavi_transition_set_ease(struct nemodavi_transition *trans, int ease);
extern int nemodavi_transition_set_done(struct nemodavi_transition *trans, nemodavi_transition_set_done_t done, void *userdata);

// set delay, repeat, ease, done callback handler
extern int nemodavi_transition_set_delay_handler(struct nemodavi_transition *trans, nemodavi_transition_set_delay_handler_t handler, void *userdata);
extern int nemodavi_transition_set_duration_handler(struct nemodavi_transition *trans, nemodavi_transition_set_duration_handler_t handler, void *userdata);
extern int nemodavi_transition_set_repeat_handler(struct nemodavi_transition *trans, nemodavi_transition_set_repeat_handler_t handler, void *userdata);
extern int nemodavi_transition_set_ease_handler(struct nemodavi_transition *trans, nemodavi_transition_set_ease_handler_t handler, void *userdata);

// set ani attrs for selector
extern int nemodavi_transition_set_cattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t value, double timing);
extern int nemodavi_transition_set_dattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, double value, double timing);
extern int nemodavi_transition_set_fix_dattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, double value, double fix, double timing);

extern int nemodavi_transition_set_cattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, uint32_t value, double timing);
extern int nemodavi_transition_set_dattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, double value, double timing);
extern int nemodavi_transition_set_fix_dattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, double value, double fix, double timing);

// set ani attrs handler for selector
extern int nemodavi_transition_set_cattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_cattr_handler_t handler, void *userdata, double timing);
extern int nemodavi_transition_set_dattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_dattr_handler_t handler, void *userdata, double timing);
extern int nemodavi_transition_set_fix_dattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_dattr_handler_t  handler, nemodavi_transition_set_dattr_handler_t fixhandler, void *userdata, double timing);

static void nemodavi_transition_set_layer_above(struct nemodavi_transition *trans, uint32_t above)
{
	trans->above = above;
}

static uint32_t nemodavi_transition_get_layer_above(struct nemodavi_transition *trans)
{
	return trans->above;
}
#ifdef __cplusplus
}
#endif

#endif
