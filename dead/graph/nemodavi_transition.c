#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>
#include <nemoshow.h>
#include <showone.h>
#include <showhelper.h>
#include <showcolor.h>
#include <showitem.h>
#include <showmisc.h>

#include "nemodavi.h"
#include "nemodavi_selector.h"
#include "nemodavi_transition.h"

/*
static struct nemodavi_set* nemodavi_transition_find_set(struct nemodavi_transition *trans, struct nemodavi_selector *sel, uint32_t index);
static void nemodavi_transition_set_cattr_one(struct nemodavi_set *set, char *attr, uint32_t value);
static void nemodavi_transition_set_dattr_one(struct nemodavi_set *set, char *attr, double value);
static void nemodavi_transition_set_fix_dattr_one(struct nemodavi_set *set, char *attr, double value, double fix);

static struct nemodavi_actor* nemodavi_transition_create_actor(struct nemodavi_transition *trans);
static struct nemodavi_set* nemodavi_transition_create_set(struct nemodavi_transition *trans, struct nemodavi_item *item);
static void nemodavi_transition_attach_set_to_actor(struct nemodavi_set *set, int nth);

static void nemodavi_transition_execute(struct nemodavi_transition *trans);
*/

static void nemodavi_transition_set_cattr_one(struct nemodavi_set *set, char *attr, uint32_t value)
{
	double r, g, b, a;
	struct showprop *prop;

	if (!set || !attr) {
		return;
	}

	prop = nemoshow_get_property(attr);
	if (!prop || (prop->type != NEMOSHOW_COLOR_PROP)) {
		return;
	}

	a = (double) NEMOSHOW_COLOR_UINT32_A(value);
	r = (double) NEMOSHOW_COLOR_UINT32_R(value);
	g = (double) NEMOSHOW_COLOR_UINT32_G(value);
	b = (double) NEMOSHOW_COLOR_UINT32_B(value);

	nemoshow_sequence_set_cattr(set->sset, attr, r, g, b, a);
}

static void nemodavi_transition_set_dattr_one(struct nemodavi_set *set, char *attr, double value)
{
	struct showprop *prop;

	if (!set || !attr) {
		return;
	}

	prop = nemoshow_get_property(attr);
	if (!prop || (prop->type != NEMOSHOW_DOUBLE_PROP)) {
		return;
	}

	nemoshow_sequence_set_dattr(set->sset, attr, value);
}

static void nemodavi_transition_set_fix_dattr_one(struct nemodavi_set *set, char *attr, double value, double fix)
{
	int index;
	struct showprop *prop;

	if (!set || !attr) {
		return;
	}

	prop = nemoshow_get_property(attr);
	if (!prop && (prop->type != NEMOSHOW_DOUBLE_PROP)) {
		return;
	}

	index = nemoshow_sequence_set_dattr(set->sset, attr, value);
	nemoshow_sequence_fix_dattr(set->sset, index, fix);
}

static struct nemodavi_actor* nemodavi_transition_create_actor(struct nemodavi_transition *trans, int index)
{
	int size;
	struct nemodavi_actor *actor;

	size = sizeof(struct nemodavi_actor);

	actor = (struct nemodavi_actor *) malloc(size);
	if (!actor) {
		return NULL;
	}
	memset(actor, 0x0, size);

	nemolist_init(&actor->link);
	nemolist_init(&actor->frame_list);
	nemolist_insert_tail(&trans->actor_list, &actor->link);

	// init variables
	actor->index = index;
	actor->repeat = 1;

	return actor;
}

static struct nemodavi_set* nemodavi_transition_create_set(struct nemodavi_transition *trans, struct nemodavi_item *item, double timing)
{
	int size;
	struct nemodavi_set *set;

	if (!trans || !item) {
		return NULL;
	}

	size = sizeof(struct nemodavi_set);
	set = (struct nemodavi_set *) malloc(size);
	memset(set, 0x0, size);

	nemolist_init(&set->link);
	nemolist_insert_tail(&item->set_list, &set->link);

	set->sset = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set->sset, item->one);

	set->trans = trans;
	set->timing = timing;

	return set;
}

static struct nemodavi_set* nemodavi_transition_find_set(struct nemodavi_transition *trans, struct nemodavi_selector *sel, uint32_t index, double timing)
{
	struct nemodavi_set *set, *foundset;
	struct nemolist *list;

	foundset = NULL;
	list = nemodavi_selector_get_showset_list(sel, index);
	nemolist_for_each(set, list, link) {
		// if exist
		if (set->trans == trans && set->timing == timing) {
			foundset = set;
			break;
		}
	}
	// if not exist
	if (!foundset) {
		foundset = nemodavi_transition_create_set(trans, sel->items[index], timing);
	}

	return foundset;
}

static struct nemodavi_frame* nemodavi_transition_create_frame(double timing)
{
	int size;
	struct nemodavi_frame *frame;

	size = sizeof(struct nemodavi_frame);
	frame = (struct nemodavi_frame *) malloc(size);
	memset(frame, 0x0, size);

	nemolist_init(&frame->link);
	frame->timing = timing;
	frame->sframe = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame->sframe, timing);

	return frame;
}


static void nemodavi_transition_attach_set_to_actor(struct nemodavi_set *set, int nth)
{
	int i = 0;
	struct nemodavi *davi;
	struct nemodavi_actor *actor;
	struct nemodavi_frame *frame;
	struct showone *one;

	if (!set) {
		return;
	}

	nemolist_for_each(actor, &set->trans->actor_list, link) {
		if (i != nth) {
			i++;
			continue;
		}

		nemolist_for_each(frame, &actor->frame_list, link) {
			if (frame->timing == set->timing) {
				nemoshow_one_attach(frame->sframe, set->sset);
				goto __set_actor_finished;
			}
		}

		
		frame = nemodavi_transition_create_frame(set->timing);
		nemolist_insert_tail(&actor->frame_list, &frame->link);

		nemoshow_one_attach(frame->sframe, set->sset);
		break;
	}

__set_actor_finished:

	// TODO change displayed item stack
	//davi = set->trans->davi;
	one = NEMOSHOW_SET_AT(set->sset, src);
	if (nemodavi_transition_get_layer_above(set->trans)) {
		nemoshow_one_above(one, NULL);
	}
}


static void nemodavi_transition_dispatch_done(void *userdata)
{
	struct nemodavi_actor *actor;
	void *bound;

	actor = (struct nemodavi_actor *) userdata;

	if (actor && actor->done_handler) {
		bound = nemodavi_data_get_bind_datum(actor->datum);
		actor->done_handler(actor->index, bound, actor->done_data);
		nemodavi_data_free_datum_paramptr(actor->datum);
		free(actor);
	}
}

static void nemodavi_transition_execute(struct nemodavi_transition *trans)
{
	if (!trans) {
		return;
	}

	int i = 0, count;
	struct nemoshow *show;
	struct showone **sframes;
	struct showone *sequence;
	struct showone *ease;
	struct nemodavi_actor *actor;
	struct nemodavi_frame *frame;


	show = trans->davi->canvas->show;

	nemolist_for_each(actor, &trans->actor_list, link) {
		// TODO apply actor's frames, not only one
		
		sequence = nemoshow_sequence_create();
		nemolist_for_each(frame, &actor->frame_list, link) {
			nemoshow_one_attach(sequence, frame->sframe);
		}

		ease = nemoshow_ease_create();
		nemoshow_ease_set_type(ease, actor->easetype);
		actor->strans = nemoshow_transition_create(ease, actor->duration, actor->delay);
		nemoshow_transition_attach_sequence(actor->strans, sequence);
		nemoshow_attach_transition(show, actor->strans);
		nemoshow_transition_set_repeat(actor->strans, actor->repeat);
		if (actor->done_handler) {
			nemoshow_transition_set_dispatch_done(actor->strans, nemodavi_transition_dispatch_done);
			nemoshow_transition_set_userdata(actor->strans, actor);
		}
	}
}

struct nemodavi_transition* nemodavi_transition_create(struct nemodavi *davi)
{
	int i, size;
	struct nemodavi_transition *trans;
	struct nemodavi_actor *actor;

	if (!davi) {
		return NULL;
	}

	size = sizeof(struct nemodavi_transition);
	trans = (struct nemodavi_transition *) malloc(size);
	memset(trans, 0x0, size);

	trans->davi = davi;
	nemolist_init(&trans->link);
	nemolist_init(&trans->actor_list);
	nemolist_insert_tail(&davi->trans_list, &trans->link);
	// as default, showone goes to top layer by transition declaration
	trans->above = 1;

	for (i = 0; i < nemodavi_get_datum_count(davi); i++) {
		actor = nemodavi_transition_create_actor(trans, i);
		actor->datum = nemodavi_get_datum(davi, i);
	}

	return trans;
}

// TODO we need to provide trans bv index selectively later.
int nemodavi_transition_destroy(struct nemodavi_transition* trans)
{
	if (!trans) {
		return -1;
	}

	struct nemodavi_actor *actor, *tmp;

	printf("%s: actor len: %d\n", __FUNCTION__, nemolist_length(&trans->actor_list));
	nemolist_for_each_safe(actor, tmp, &trans->actor_list, link) {
		printf("%s: %d\n", __FUNCTION__, actor->index);
		nemoshow_transition_destroy(actor->strans, 0);
		if (!actor->done_handler) {
			free(actor);
		}
	}
	free(trans);

	return 0;
}

void nemodavi_transition_clear(struct nemodavi *davi)
{
	int i, is_infinite_actor = 0;
	struct nemodavi_transition *trans;
	struct nemodavi_actor *actor;
	struct nemodavi_item *item;
	struct nemodavi_set *set;

	if (!davi) {
		return;
	}

	for (i = 0; i < nemodavi_get_datum_count(davi); i++) {
		nemolist_for_each(item, davi->item_lists[i], link) {
	        struct nemodavi_set *tmp;
			nemolist_for_each_safe(set, tmp, &item->set_list, link) {
				free(set);
			}
			nemolist_init(&item->set_list);
		}
	}

	nemolist_for_each(trans, &davi->trans_list, link) {
		nemolist_for_each(actor, &trans->actor_list, link) {
			if (actor->repeat == 0) {
				is_infinite_actor = 1;
			}
		}

		if (!is_infinite_actor) {
			nemolist_for_each(actor, &trans->actor_list, link) {
				if (!actor->done_handler) {
					free(actor);
				}
			}
			free(trans);
		}
	}
	nemolist_init(&davi->trans_list);
}

int nemodavi_transition_run(struct nemodavi *davi)
{
	int i;
	struct nemodavi_transition *trans;
	struct nemodavi_item *item;
	struct nemodavi_set *set;

	if (!davi) {
		return -1;
	}

	nemolist_for_each(trans, &davi->trans_list, link) {
		for (i = 0; i < nemodavi_get_datum_count(davi); i++) {
			nemolist_for_each(item, davi->item_lists[i], link) {
				nemolist_for_each(set, &item->set_list, link) {
					if (set->trans == trans) {
						nemodavi_transition_attach_set_to_actor(set, i);
					}
					//break;
				}
			}
		}
		nemodavi_transition_execute(trans);
	}
	nemodavi_transition_clear(davi);
	nemoshow_dispatch_frame(davi->canvas->show);

	return 0;
}

int nemodavi_transition_set_delay(struct nemodavi_transition *trans, uint32_t delay)
{
	struct nemodavi_actor *actor;
	if (!trans) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		actor->delay = delay;
	}

	return 0;
}

int nemodavi_transition_set_duration(struct nemodavi_transition *trans, uint32_t duration)
{
	struct nemodavi_actor *actor;

	if (!trans) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		actor->duration = duration;
	}

	return 0;
}

int nemodavi_transition_set_repeat(struct nemodavi_transition *trans, int repeat)
{
	struct nemodavi_actor *actor;

	if (!trans) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		actor->repeat = repeat;
	}

	return 0;
}

int nemodavi_transition_set_ease(struct nemodavi_transition *trans, int ease)
{
	struct nemodavi_actor *actor;

	if (!trans) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		actor->easetype = ease;
	}

	return 0;
}

int nemodavi_transition_set_done(struct nemodavi_transition *trans, nemodavi_transition_set_done_t done, void *userdata)
{
	struct nemodavi_actor *actor;

	if (!trans|| !done) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		actor->done_handler = done;
		if (userdata) {
			actor->done_data = userdata;
		}
	}

	return 0;
}

int nemodavi_transition_set_delay_handler(struct nemodavi_transition *trans, nemodavi_transition_set_delay_handler_t handler, void *userdata)
{
	int i = 0;
	void *bound;
	struct nemodavi_actor *actor;

	if (!trans || !handler) {
		return -1;
	}


	nemolist_for_each(actor, &trans->actor_list, link) {
		bound = nemodavi_data_get_bind_datum(actor->datum);
		actor->delay = handler(i++, bound, userdata);
		nemodavi_data_free_datum_paramptr(actor->datum);
	}

	return 0;
}

int nemodavi_transition_set_duration_handler(struct nemodavi_transition *trans, nemodavi_transition_set_duration_handler_t handler, void *userdata)
{
	int i = 0;
	void *bound;
	struct nemodavi_actor *actor;

	if (!trans || !handler) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		bound = nemodavi_data_get_bind_datum(actor->datum);
		actor->duration = handler(i++, bound, userdata);
		nemodavi_data_free_datum_paramptr(actor->datum);
	}

	return 0;
}

int nemodavi_transition_set_repeat_handler(struct nemodavi_transition *trans, nemodavi_transition_set_repeat_handler_t handler, void *userdata)
{
	int i = 0;
	void *bound;
	struct nemodavi_actor *actor;

	if (!trans || !handler) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		bound = nemodavi_data_get_bind_datum(actor->datum);
		actor->repeat = handler(i++, bound, userdata);
		nemodavi_data_free_datum_paramptr(actor->datum);
	}

	return 0;
}

int nemodavi_transition_set_ease_handler(struct nemodavi_transition *trans, nemodavi_transition_set_ease_handler_t handler, void *userdata)
{
	int i = 0;
	void *bound;
	struct nemodavi_actor *actor;

	if (!trans || !handler) {
		return -1;
	}

	nemolist_for_each(actor, &trans->actor_list, link) {
		bound = nemodavi_data_get_bind_datum(actor->datum);
		actor->easetype = handler(i++, bound, userdata);
		nemodavi_data_free_datum_paramptr(actor->datum);
	}

	return 0;
}

int nemodavi_transition_set_cattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t value, double timing)
{
	// color attr
	int i;
	struct nemodavi_set *set;

	if (!trans || !sel || !attr) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		nemodavi_transition_set_cattr_one(set, attr, value);
	}

	return 0;
}

int nemodavi_transition_set_dattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, double value, double timing)
{
	int i;
	struct nemodavi_set *set;

	if (!trans || !sel || !attr) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		nemodavi_transition_set_dattr_one(set, attr, value);
	}

	return 0;
}

int nemodavi_transition_set_fix_dattr(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, double value, double fix, double timing)
{
	int i;
	struct nemodavi_set *set;

	if (!trans || !sel || !attr) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		nemodavi_transition_set_fix_dattr_one(set, attr, value, fix);
	}

	return 0;
}

int nemodavi_transition_set_cattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, uint32_t value, double timing)
{
	// color attr
	int i;
	struct nemodavi_set *set;

	if (!trans || !sel || !attr) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	set = nemodavi_transition_find_set(trans, sel, index, timing);
	nemodavi_transition_set_cattr_one(set, attr, value);

	return 0;
}

int nemodavi_transition_set_dattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, double value, double timing)
{
	int i;
	struct nemodavi_set *set;

	if (!trans || !sel || !attr) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	set = nemodavi_transition_find_set(trans, sel, index, timing);
	nemodavi_transition_set_dattr_one(set, attr, value);

	return 0;
}

int nemodavi_transition_set_fix_dattr_by_index(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, uint32_t index, double value, double fix, double timing)
{
	int i;
	struct nemodavi_set *set;
	struct nemodavi_set *foundset;
	struct nemolist *list;

	if (!trans || !sel || !attr) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	set = nemodavi_transition_find_set(trans, sel, index, timing);
	nemodavi_transition_set_fix_dattr_one(set, attr, value, fix);

	return 0;
}

int nemodavi_transition_set_cattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_cattr_handler_t handler, void *userdata, double timing)
{
	// color attr
	int i;
	uint32_t value;
	void *bound;
	struct nemodavi_set *set;
	struct nemodavi_datum *datum;

	if (!trans || !sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		nemodavi_transition_set_cattr_one(set, attr, value);
	}

	return 0;
}

int nemodavi_transition_set_dattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_dattr_handler_t handler, void *userdata, double timing)
{
	int i;
	double value;
	void *bound;
	struct nemodavi_set *set;
	struct nemodavi_datum *datum;

	if (!trans || !sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		nemodavi_transition_set_dattr_one(set, attr, value);
	}

	return 0;
}

int nemodavi_transition_set_fix_dattr_handler(struct nemodavi_transition *trans, struct nemodavi_selector *sel, char *attr, nemodavi_transition_set_dattr_handler_t  handler, nemodavi_transition_set_dattr_handler_t fixhandler, void *userdata, double timing)
{
	int i;
	double value, fix;
	void *bound;
	struct nemodavi_set *set;
	struct nemodavi_datum *datum;

	if (!trans || !sel || !attr || !handler || !fixhandler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		set = nemodavi_transition_find_set(trans, sel, i, timing);
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		fix = fixhandler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		nemodavi_transition_set_fix_dattr_one(set, attr, value, fix);
	}

	return 0;
}
