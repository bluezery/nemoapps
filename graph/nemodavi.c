#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>
#include <nemoshow.h>
#include <showone.h>
#include <showhelper.h>

#include "nemodavi.h"
#include "nemodavi_data.h"
#include "nemodavi_item.h"
#include "nemodavi_selector.h"

static struct nemolist* g_nemodavi_list;

struct nemodavi* nemodavi_create(struct showone *canvas, struct nemodavi_data *data) {
	static uint32_t id = 1;
	uint32_t i, count;
	struct nemodavi *davi;

	if (!canvas) {
		return NULL;
	}

	if (!data) {
		// in this case, davi without data is prepared
		int datums[1] = { 1 };
		data = nemodavi_data_create();
		nemodavi_data_bind_int_array(data, datums, 1);
	}

	count = data->data_count;

	davi = (struct nemodavi *) malloc(sizeof(struct nemodavi));
	memset(davi, 0x0, sizeof(struct nemodavi));

	davi->id = id;
	davi->tag_index = 0;
	davi->canvas = canvas;
	davi->data = data;
	davi->item_lists = (struct nemolist **) malloc(sizeof(struct nemolist *) * count);
	davi->width = NEMOSHOW_CANVAS_AT(canvas, width);
	davi->height = NEMOSHOW_CANVAS_AT(canvas, height);

	davi->group = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
	nemoshow_one_attach(canvas, davi->group);

	for (i = 0; i < count; i++) {
		davi->item_lists[i] = (struct nemolist *) malloc(sizeof(struct nemolist));
		nemolist_init(davi->item_lists[i]);
	}
	nemolist_init(&davi->trans_list);
	nemolist_init(&davi->selector_list);
	nemolist_init(&davi->link);

	if (!g_nemodavi_list) {
		g_nemodavi_list = (struct nemolist *) malloc(sizeof(struct nemolist));
		memset(g_nemodavi_list, 0x0, sizeof(struct nemolist));
		nemolist_init(g_nemodavi_list);
	}

	nemolist_insert_tail(g_nemodavi_list, &davi->link);
	nemodavi_data_reference(davi->data);

	id++;

	return davi;
}

int nemodavi_destroy(struct nemodavi *davi)
{
	struct nemodavi_selector *current, *next;
	if (!davi) {
		return -1;
	}

	nemodavi_data_unreference(davi->data);
	free(davi->item_lists);

	// TODO remove selector list
	nemolist_for_each_safe(current, next, &davi->selector_list, link) {
		nemolist_remove(&current->link);
		free(current);
	}

	nemoshow_item_destroy(davi->group);

	free(davi);

	return 0;
}

int nemodavi_append_selector(struct nemodavi *davi, char *name, int type)
{
	uint32_t i, size, found, tag;
	struct showone *one;
	struct nemodavi_item *item;
	struct nemodavi_data *data;

	if (!davi || !name) {
		return -1;
	}

	found = 0;

	size = sizeof(struct nemodavi_item);
	data = davi->data;

	for (i = 0; i < data->data_count; i++) {
		nemolist_for_each(item, davi->item_lists[i], link) {
			if (!strcmp(item->name, name)) {
				found = 1;
							goto __found;
			}
		}
		item = (struct nemodavi_item *) malloc(size);
		memset(item, 0x0, size);

		nemolist_init(&item->link);
		nemolist_init(&item->set_list);
		nemolist_insert_tail(davi->item_lists[i], &item->link);

		item->one = one = nemodavi_item_create(type);
		tag = NEMODAVI_ONE_TAGGROUP(davi->tag_index, davi->id);
		nemoshow_one_set_tag(one, tag);
		nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
		//printf("%s) davi id: %d, start tag: %d, final tag: %x\n", name, davi->id, davi->tag_index, tag);
		davi->tag_index++;

		if (nemoshow_one_is_type(one, NEMOSHOW_ITEM_TYPE)) {
			nemoshow_one_attach(davi->group, one);
		}

		// TODO we need to check if same selector exists
		item->name = strdup(name);
		item->datum = data->datums[i];
	}

__found:
	if (found) {
		return -1;
	}


	return 0;
}

int nemodavi_append_selector_by_handler(struct nemodavi *davi, char *name, nemodavi_append_selector_handler_t handler, void *userdata)
{
	uint32_t i, size, found, tag;
	void *bound;
	struct showone *one;
	struct nemodavi_item *item;
	struct nemodavi_data *data;

	if (!davi || !name) {
		return -1;
	}

	found = 0;

	size = sizeof(struct nemodavi_item);
	data = davi->data;

	for (i = 0; i < data->data_count; i++) {
		nemolist_for_each(item, davi->item_lists[i], link) {
			if (!strcmp(item->name, name)) {
				found = 1;
				goto __found;
			}
		}
		item = (struct nemodavi_item *) malloc(size);
		memset(item, 0x0, size);

		nemolist_init(&item->link);
		nemolist_init(&item->set_list);
		nemolist_insert_tail(davi->item_lists[i], &item->link);

		bound = nemodavi_data_get_bind_datum(data->datums[i]);
		item->one = one = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(data->datums[i]);

		tag = NEMODAVI_ONE_TAGGROUP(davi->tag_index, davi->id);
		nemoshow_one_set_tag(one, tag);
		nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
		//printf("%s) davi id: %d, start tag: %d, final tag: %x\n", name, davi->id, davi->tag_index, tag);
		davi->tag_index++;

		if (nemoshow_one_is_type(one, NEMOSHOW_ITEM_TYPE)) {
			nemoshow_one_attach(davi->group, one);
		}

		// TODO we need to check if same selector exists
		item->name = strdup(name);
		item->datum = data->datums[i];
	}

__found:
	if (found) {
		return -1;
	}


	return 0;
}

struct nemodavi_datum* nemodavi_get_datum(struct nemodavi *davi, uint32_t index)
{
	if (!davi) {
		return NULL;
	}

	if (index >= davi->data->data_count) {
		return NULL;
	}

	return davi->data->datums[index];
}

void nemodavi_emit_data_change_signal(struct nemodavi *davi)
{
	if (!davi) {
		return;
	}

	nemodavi_data_emit_all_signals(davi->data);
}

void nemodavi_emit_event(uint32_t event, uint32_t tag)
{
	uint32_t i;
	void *bound;
	struct nemodavi *davi;
	struct nemodavi_item *item;
	struct nemodavi_datum *datum;

	if (!g_nemodavi_list) {
		return;
	}

	nemolist_for_each(davi, g_nemodavi_list, link) {
		if (davi->id != NEMODAVI_ONE_GROUP(tag)) {
			continue;
		}
		for (i = 0; i < nemodavi_get_datum_count(davi); i++) {
			datum = nemodavi_get_datum(davi, i);
			nemolist_for_each(item, davi->item_lists[i], link) {
				if (nemoshow_one_get_tag(item->one) != tag) {
					continue;
				}

				if (item->events && item->events[event]->listener) {
					bound = nemodavi_data_get_bind_datum(datum);
					item->events[event]->listener(i, bound, item->events[event]->userdata);
					nemodavi_data_free_datum_paramptr(datum);
					break;
				}
			}
		}
	}
}

void nemodavi_translate(struct nemodavi *davi, double tx, double ty)
{
	if (!davi) {
		return;
	}

	nemoshow_item_translate(davi->group, tx, ty);
}

void nemodavi_scale(struct nemodavi *davi, double sx, double sy)
{
	if (!davi) {
		return;
	}

	nemoshow_item_scale(davi->group, sx, sy);
}

void nemodavi_rotate(struct nemodavi *davi, double angle)
{
	if (!davi) {
		return;
	}

	nemoshow_item_rotate(davi->group, angle);
}
