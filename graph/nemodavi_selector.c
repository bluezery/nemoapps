#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>
#include <nemolistener.h>

#include "nemodavi.h"
#include "nemodavi_data.h"
#include "nemodavi_selector.h"

static struct nemodavi_selector* nemodavi_selector_create()
{
	struct nemodavi_selector *sel;

	sel = (struct nemodavi_selector *) malloc(sizeof(struct nemodavi_selector));
	memset(sel, 0x0, sizeof(struct nemodavi_selector));

	return sel;
}

static void nemodavi_selector_create_items(struct nemodavi_selector* sel, uint32_t count)
{
	uint32_t size;

	size = sizeof(struct nemodavi_item *) * count;
	sel->items = (struct nemodavi_item **) malloc(size); 
	memset(sel->items, 0x0, size);
}

static void nemodavi_selector_create_item_events(struct nemodavi_selector* sel, uint32_t index)
{
	uint32_t i, size;

	if (sel->items[index]->events) {
		return;
	}

	size = sizeof(struct nemodavi_event *) * NEMODAVI_EVENT_LAST;
	sel->items[index]->events = (struct nemodavi_event **) malloc(size); 
	memset(sel->items[index]->events, 0x0, size);

	size = sizeof(struct nemodavi_event);
	for (i = 0; i < NEMODAVI_EVENT_LAST; i++) {
		sel->items[index]->events[i] = (struct nemodavi_event *) malloc(size); 
	}
}


static int nemodavi_selector_insert_filter_index(struct nemolist *filter_list, uint32_t index)
{
	uint32_t size;
	struct nemodavi_selector_filter *filter; 

	if (!filter_list) {
		return -1;
	}

	size = sizeof(struct nemodavi_selector_filter);
	filter = (struct nemodavi_selector_filter *) malloc(size); 
	memset(filter, 0x0, size);
	filter->index = index; 

	nemolist_insert_tail(filter_list, &filter->link);

	return 0;
}

static void nemodavi_selector_data_listener(struct nemolistener *listener, void *index)
{
	void *bound;
	struct nemodavi_item *item;

	item = (struct nemodavi_item *) container_of(listener, struct nemodavi_item, listener); 

	bound = nemodavi_data_get_bind_datum(item->datum);
	item->ulistener(* (int *) index, bound, item->userdata);
	nemodavi_data_free_datum_paramptr(item->datum);

}

struct nemodavi_selector* nemodavi_selector_get(struct nemodavi *davi, char *name)
{
	int i, count;
	struct nemodavi_selector *sel;
	struct nemodavi_item *item;
	struct nemolist *list;

	if (!davi || !name) {
		return NULL;
	}

	nemolist_for_each(sel, &davi->selector_list, link) {
		if(!strcmp(sel->name, name)) {
			//printf("this selector exists!: %s\n", name);
			return sel;
		}
	}

	count = davi->data->data_count;

	sel = nemodavi_selector_create();
	sel->name = strdup(name);
	sel->count = count;

	nemodavi_selector_create_items(sel, count);
	nemolist_init(&sel->link);

	for (i = 0; i < count; i++) {
		list = nemodavi_get_item_list(davi, i);
		nemolist_for_each(item, list, link) {
			if (strcmp(item->name, name)) {
				continue;
			}

			nemodavi_selector_set_item(sel, i, item);
			break;
		}
	}
	//TODO need to check if matched selector name exists
	// if not, we need to return NULL
	

	nemolist_insert_tail(&davi->selector_list, &sel->link);

	return sel;
}

struct nemodavi_selector* nemodavi_selector_get_by_index(struct nemodavi *davi, char *name, uint32_t index)
{
	int max;
	struct nemodavi_selector *sel;
	struct nemodavi_item *item;
	struct nemolist *list;

	if (!davi || !name) {
		return NULL;
	}

	//TODO check if this selector exists
	//


	max = davi->data->data_count - 1;
	if (index < 0 || index > max) {
		return NULL;
	}

	sel = nemodavi_selector_create();
	sel->name = strdup(name);
	sel->count = 1;

	nemodavi_selector_create_items(sel, 1);
	nemolist_init(&sel->link);

	list = nemodavi_get_item_list(davi, index);
	nemolist_for_each(item, list, link) {
		if (strcmp(item->name, name)) {
			continue;
		}

		nemodavi_selector_set_item(sel, 0, item);
		break;
	}
	//TODO need to check if matched selector name exists
	// if not, we need to return NULL

	nemolist_insert_tail(&davi->selector_list, &sel->link);

	return sel;
}

struct nemodavi_selector* nemodavi_selector_get_by_range(struct nemodavi *davi, char *name, uint32_t from, uint32_t to)
{
	int i, count, max;
	struct nemodavi_selector *sel;
	struct nemodavi_item *item;
	struct nemolist *list;

	if (!davi || !name) {
		return NULL;
	}

	//TODO check if this selector exists
	//


	// check from, to index value
	max = davi->data->data_count - 1;
	from = from < 0 ? 0 : from;
	to = to < from ? from : to;
	to = to > max ? max : to;

	count = to - from + 1;

	sel = nemodavi_selector_create();
	sel->name = strdup(name);
	sel->count = count;

	nemodavi_selector_create_items(sel, count);
	nemolist_init(&sel->link);

	for (i = 0; i < count; i++) {
		list = nemodavi_get_item_list(davi, i + from);
		nemolist_for_each(item, list, link) {
			if (strcmp(item->name, name)) {
				continue;
			}

			nemodavi_selector_set_item(sel, i, item);
			break;
		}
	}
	//TODO need to check if matched selector name exists
	// if not, we need to return NULL
	

	nemolist_insert_tail(&davi->selector_list, &sel->link);

	return sel;
}

struct nemodavi_selector* nemodavi_selector_get_by_filter(struct nemodavi *davi, char *name, nemodavi_selector_filter_t filter, void *userdata)
{
	uint32_t i, count, found;
	void *bound;
	struct nemodavi_selector *sel;
	struct nemodavi_item *item;
	struct nemodavi_datum *datum;
	struct nemodavi_selector_filter *current;
	struct nemolist filtered_list, *list;

	if (!davi || !name || !filter) {
		return NULL;
	}

	//TODO check if this selector exists
	//


	nemolist_init(&filtered_list);

	for (i = 0; i < nemodavi_get_datum_count(davi); i++) {
		datum = nemodavi_get_datum(davi, i);
		bound = nemodavi_data_get_bind_datum(datum);
		found = filter(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);
		if (found) {
			nemodavi_selector_insert_filter_index(&filtered_list, i);
		}
	}

	count = nemolist_length(&filtered_list);

	sel = nemodavi_selector_create();
	sel->name = strdup(name);
	sel->count = count;

	nemodavi_selector_create_items(sel, count);
	nemolist_init(&sel->link);

	i = 0;
	nemolist_for_each(current, &filtered_list, link) {
		list = nemodavi_get_item_list(davi, current->index);
		nemolist_for_each(item, list, link) {
			if (strcmp(item->name, name)) {
				continue;
			}

			nemodavi_selector_set_item(sel, i, item);
			break;
		}
		i++;
	}


	//TODO need to check if matched selector name exists
	// if not, we need to return NULL
	

	nemolist_insert_tail(&davi->selector_list, &sel->link);

	return sel;
}

int nemodavi_selector_destory(struct nemodavi_selector* sel)
{
	int i;

	if (!sel) {
		return -1;
	}

	nemolist_remove(&sel->link);

	for (i = 0; i < sel->count; i++) {
		nemolist_remove(&sel->items[i]->link);
		// TODO remove this items's set list
		free(sel->items[i]);
	}

	free(sel);

	return 0;
}

int nemodavi_selector_attach_data_listener(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, void *userdata)
{
	int i;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel || !ulistener) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		nemodavi_selector_set_data_listener(sel, i, nemodavi_selector_data_listener);
		datum = nemodavi_selector_get_datum(sel, i);
		listener = nemodavi_selector_get_data_listener(sel, i);
		nemodavi_data_attach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i, ulistener, userdata);
	}

	return 0;
}

int nemodavi_selector_attach_data_listener_by_index(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, uint32_t index, void *userdata)
{
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel || !ulistener) {
		return -1;
	}

	if (index < 0 || index > sel->count) {
		return -1;
	}

	nemodavi_selector_set_data_listener(sel, index, nemodavi_selector_data_listener);
	datum = nemodavi_selector_get_datum(sel, index);
	listener = nemodavi_selector_get_data_listener(sel, index);
	nemodavi_data_attach_datum_listener(datum, listener);

	nemodavi_selector_set_data_ulistener(sel, index, ulistener, userdata);

	return 0;
}

int nemodavi_selector_attach_data_listener_by_range(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, uint32_t from, uint32_t to, void *userdata)
{
	uint32_t i, max, count;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel || !ulistener) {
		return -1;
	}

	max = sel->count - 1;
	from = from < 0 ? 0 : from;
	to = to < from ? from : to;
	to = to > max ? max : to;

	count = to - from + 1;

	for (i = 0; i < count; i++) {
		nemodavi_selector_set_data_listener(sel, i + from, nemodavi_selector_data_listener);
		datum = nemodavi_selector_get_datum(sel, i + from);
		listener = nemodavi_selector_get_data_listener(sel, i + from);
		nemodavi_data_attach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i + from, ulistener, userdata);
	}

	return 0;
}

int nemodavi_selector_attach_data_listener_by_filter(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, void *luserdata, nemodavi_selector_filter_t filter, void *fuserdata)
{
	uint32_t i, found;
	void *bound;
	struct nemolist list;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;
	struct nemodavi_selector_filter *current;

	if (!sel || !filter || !ulistener) {
		return -1;
	}

	nemolist_init(&list);

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);
		bound = nemodavi_data_get_bind_datum(datum);
		found = filter(i, bound, fuserdata);
		nemodavi_data_free_datum_paramptr(datum);

		if (found) {
			nemodavi_selector_insert_filter_index(&list, i);
		}
	}

	nemolist_for_each(current, &list, link) {
		i = current->index;

		nemodavi_selector_set_data_listener(sel, i, nemodavi_selector_data_listener);
		datum = nemodavi_selector_get_datum(sel, i);
		listener = nemodavi_selector_get_data_listener(sel, i);
		nemodavi_data_attach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i, ulistener, luserdata);
	}

	return 0;
}

int nemodavi_selector_detach_data_listener(struct nemodavi_selector *sel)
{
	uint32_t i;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);
		listener = nemodavi_selector_get_data_listener(sel, i);
		nemodavi_data_detach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_detach_data_listener_by_index(struct nemodavi_selector *sel, uint32_t index)
{
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel) {
		return -1;
	}

	datum = nemodavi_selector_get_datum(sel, index);
	listener = nemodavi_selector_get_data_listener(sel, index);
	nemodavi_data_detach_datum_listener(datum, listener);

	nemodavi_selector_set_data_ulistener(sel, index, NULL, NULL);

	return 0;
}

int nemodavi_selector_detach_data_listener_by_range(struct nemodavi_selector *sel, uint32_t from, uint32_t to)
{
	uint32_t i, max, count;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;

	if (!sel) { 
		return -1;
	}

	max = sel->count - 1;
	from = from < 0 ? 0 : from;
	to = to < from ? from : to;
	to = to > max ? max : to;

	count = to - from + 1;

	for (i = 0; i < count; i++) {
		datum = nemodavi_selector_get_datum(sel, i + from);
		listener = nemodavi_selector_get_data_listener(sel, i + from);
		nemodavi_data_detach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i + from, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_detach_data_listener_by_filter(struct nemodavi_selector *sel, nemodavi_selector_filter_t filter, void *userdata)
{
	uint32_t i, found;
	struct nemolist list;
	struct nemodavi_datum *datum;
	struct nemolistener *listener;
	struct nemodavi_selector_filter *current, *next;

	if (!sel || !filter) {
		return -1;
	}

	nemolist_init(&list);

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);
		found = filter(i, datum, userdata);
		nemodavi_data_free_datum_paramptr(datum);
		if (found) {
			nemodavi_selector_insert_filter_index(&list, i);
		}
	}

	nemolist_for_each_safe(current, next, &list, link) {
		i = current->index;

		datum = nemodavi_selector_get_datum(sel, i);
		listener = nemodavi_selector_get_data_listener(sel, i);
		nemodavi_data_detach_datum_listener(datum, listener);

		nemodavi_selector_set_data_ulistener(sel, i, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_attach_event_listener(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, void *userdata)
{
	uint32_t i;

	if (!sel || !listener) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		nemodavi_selector_create_item_events(sel, i);
		nemodavi_selector_set_event_listener(sel, i, event, listener, userdata);
	}

	return 0;
}

int nemodavi_selector_attach_event_listener_by_index(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, uint32_t index, void *userdata)
{
	if (!sel || !listener) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	nemodavi_selector_create_item_events(sel, index);
	nemodavi_selector_set_event_listener(sel, index, event, listener, userdata);

	return 0;
}

int nemodavi_selector_attach_event_listener_by_range(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, uint32_t from, uint32_t to, void *userdata)
{
	uint32_t i, count, max;

	if (!sel || !listener) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	// check from, to index value
	max = sel->count - 1;
	from = from < 0 ? 0 : from;
	to = to < from ? from : to;
	to = to > max ? max : to;

	count = to - from + 1;

	for (i = 0; i < count; i++) {
		nemodavi_selector_create_item_events(sel, i + from);
		nemodavi_selector_set_event_listener(sel, i + from, event, listener, userdata);
	}

	return 0;
}

int nemodavi_selector_attach_event_listener_by_filter(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, void *luserdata, nemodavi_selector_filter_t filter, void *fuserdata)
{
	uint32_t i, count, found;
	struct nemolist filtered_list;
	struct nemodavi_datum *datum;
	struct nemodavi_selector_filter *current;

	if (!sel || !listener || !filter) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	nemolist_init(&filtered_list);

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);
		found = filter(i, nemodavi_data_get_bind_datum(datum), fuserdata);
		nemodavi_data_free_datum_paramptr(datum);
		if (found) {
			nemodavi_selector_insert_filter_index(&filtered_list, i);
		}
	}

	count = nemolist_length(&filtered_list);

	nemolist_for_each(current, &filtered_list, link) {
		nemodavi_selector_create_item_events(sel, current->index);
		nemodavi_selector_set_event_listener(sel, current->index, event, listener, luserdata);
	}

	return 0;
}


int nemodavi_selector_detach_event_listener(struct nemodavi_selector *sel, uint32_t event)
{
	uint32_t i;

	if (!sel) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		nemodavi_selector_create_item_events(sel, i);
		nemodavi_selector_set_event_listener(sel, i, event, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_detach_event_listener_by_index(struct nemodavi_selector *sel, uint32_t event, uint32_t index)
{
	if (!sel) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	nemodavi_selector_create_item_events(sel, index);
	nemodavi_selector_set_event_listener(sel, index, event, NULL, NULL);

	return 0;
}

int nemodavi_selector_detach_event_listener_by_range(struct nemodavi_selector *sel, uint32_t event, uint32_t from, uint32_t to)
{
	uint32_t i, count, max;

	if (!sel) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	// check from, to index value
	max = sel->count - 1;
	from = from < 0 ? 0 : from;
	to = to < from ? from : to;
	to = to > max ? max : to;

	count = to - from + 1;

	for (i = 0; i < count; i++) {
		nemodavi_selector_create_item_events(sel, i + from);
		nemodavi_selector_set_event_listener(sel, i + from, event, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_detach_event_listener_by_filter(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_filter_t filter, void *userdata)
{
	uint32_t i, count, found;
	struct nemolist filtered_list;
	struct nemodavi_datum *datum;
	struct nemodavi_selector_filter *current;

	if (!sel || !filter) {
		return -1;
	}

	if (event >= NEMODAVI_EVENT_LAST) {
		return -1;
	}

	nemolist_init(&filtered_list);

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);
		found = filter(i, nemodavi_data_get_bind_datum(datum), userdata);
		nemodavi_data_free_datum_paramptr(datum);
		if (found) {
			nemodavi_selector_insert_filter_index(&filtered_list, i);
		}
	}

	count = nemolist_length(&filtered_list);

	nemolist_for_each(current, &filtered_list, link) {
		nemodavi_selector_create_item_events(sel, current->index);
		nemodavi_selector_set_event_listener(sel, current->index, event, NULL, NULL);
	}

	return 0;
}

int nemodavi_selector_sort(struct nemodavi_selector *sel, nemodavi_selector_comparator_t comparator, void *userdata)
{
	int result;
	uint32_t i, j;
	struct nemodavi_item *tmp;
	struct nemodavi_datum *a, *b;

	if (!sel || !comparator) {
		return -1;
	}

	// we apply insertion sort algorithm
	for (i = 1; i < sel->count; i++) {
		j = i;
		while (j > 0) {
			a = nemodavi_selector_get_datum(sel, j - 1);
			b = nemodavi_selector_get_datum(sel, j);
			result = comparator(
					nemodavi_data_get_bind_datum(a),
					nemodavi_data_get_bind_datum(b),
					userdata
			);
			nemodavi_data_free_datum_paramptr(a);
			nemodavi_data_free_datum_paramptr(b);

			if (result > 0) {
				tmp = sel->items[j];
				sel->items[j] = sel->items[j - 1];
				sel->items[j - 1] = tmp;
				j--;
			} else {
				break;
			}
		}
	}
	return 0;
}
