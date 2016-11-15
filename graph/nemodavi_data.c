#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>
#include <nemolistener.h>

#include "nemodavi_data.h"

struct nemodavi_data* nemodavi_data_create()
{
	struct nemodavi_data *data;

	data = (struct nemodavi_data *) malloc(sizeof(struct nemodavi_data));
	memset(data, 0x0, sizeof(struct nemodavi_data));

	return data;
}

int nemodavi_data_destory(struct nemodavi_data *data)
{
	if (!data) {
		return -1;
	}

	if (data->ref) {
		return -1;
	}

	free(data);

	return 0;
}

static void nemodavi_data_initialize(struct nemodavi_data *data, NemoDaviDatumType type, uint32_t count)
{
	uint32_t i;

	data->data_count = count;
	data->datums = (struct nemodavi_datum **) malloc(sizeof(struct nemodavi_datum *) * count);

	for (i = 0; i < count; i++) {
		data->datums[i] = (struct nemodavi_datum *) malloc(sizeof(struct nemodavi_datum));
		memset(data->datums[i], 0x0, sizeof(struct nemodavi_datum));
		data->datums[i]->type = type;
		nemosignal_init(&data->datums[i]->signal);
	}
}


int nemodavi_data_bind_ptr_array(struct nemodavi_data *data, void **datums, uint32_t count, size_t onesize)
{
	int i;
	void *addr;

	if (!data || !datums) {
		return -1;
	}

	if (!data->datums) {
		nemodavi_data_initialize(data, NEMODAVI_DATUM_TYPE_PTR, count);
	}


	for (i = 0; i < data->data_count; i++) {
		addr = datums + (onesize * i) / sizeof(void *);
		printf("nemodavi data: 0x%x\n", addr);
		nemodavi_data_set_ptr_datum(data, addr, i);
		nemosignal_emit(&data->datums[i]->signal, &i);
	}

	return 0;
}

int nemodavi_data_bind_int_array(struct nemodavi_data *data, int *datums, uint32_t count)
{
	int i;

	if (!data || !datums) {
		return -1;
	}

	if (!data->datums) {
		nemodavi_data_initialize(data, NEMODAVI_DATUM_TYPE_INT, count);
	}

	for (i = 0; i < data->data_count; i++) {
		nemodavi_data_set_int_datum(data, datums[i], i);
		nemosignal_emit(&data->datums[i]->signal, &i);
	}

	return 0;
}

int nemodavi_data_bind_double_array(struct nemodavi_data *data, double *datums, uint32_t count)
{
	int i;

	if (!data || !datums) {
		return -1;
	}

	if (!data->datums) {
		nemodavi_data_initialize(data, NEMODAVI_DATUM_TYPE_DOUBLE, count);
	}

	for (i = 0; i < data->data_count; i++) {
		nemodavi_data_set_double_datum(data, datums[i], i);
		nemosignal_emit(&data->datums[i]->signal, &i);
	}

	return 0;
}

int nemodavi_data_bind_ptr_one(struct nemodavi_data *data, void *datum, uint32_t index)
{
	if (!data || !data->datums || !datum) {
		return -1;
	}

	if (index < 0 || index > (data->data_count - 1)) {
		return -1;
	}

	nemodavi_data_set_ptr_datum(data, datum, index);
	nemosignal_emit(&data->datums[index]->signal, &index);

	return 0;
}

int nemodavi_data_bind_int_one(struct nemodavi_data *data, int datum, uint32_t index)
{
	if (!data || !data->datums) {
		return -1;
	}

	if (index < 0 || index > (data->data_count - 1)) {
		return -1;
	}

	nemodavi_data_set_int_datum(data, datum, index);
	nemosignal_emit(&data->datums[index]->signal, &index);

	return 0;
}

int nemodavi_data_bind_double_one(struct nemodavi_data *data, double datum, uint32_t index)
{
	if (!data || !data->datums) {
		return -1;
	}

	if (index < 0 || index > (data->data_count - 1)) {
		return -1;
	}

	nemodavi_data_set_double_datum(data, datum, index);
	nemosignal_emit(&data->datums[index]->signal, &index);

	return 0;
}

int nemodavi_data_attach_datum_listener(struct nemodavi_datum *datum, struct nemolistener *listener)
{
	struct nemolistener *current, *next;
	struct nemolist *listener_list;

	if (!datum || !listener) {
		return -1;
	}

	listener_list = nemodavi_data_get_datum_listener_list(datum);
	nemolist_for_each_safe(current, next, listener_list, link) {
		if (current == listener) {
			printf("already attached listener removed, and new one is attached\n");
			nemolist_remove(&current->link);
			break;
		}
	}

	nemolist_insert_tail(listener_list, &listener->link);

	return 0;
}

int nemodavi_data_detach_datum_listener(struct nemodavi_datum *datum, struct nemolistener *listener)
{
	struct nemolistener *current, *next;
	struct nemolist *listener_list;

	if (!datum || !listener) {
		return -1;
	}

	listener_list = nemodavi_data_get_datum_listener_list(datum);
	nemolist_for_each_safe(current, next, listener_list, link) {
		if (current == listener) {
			nemolist_remove(&current->link);
			break;
		}
	}

	return 0;
}


int nemodavi_data_reference(struct nemodavi_data *data)
{
	if (!data) {
		return -1;
	}

	data->ref++;

	// TODO how to keep davi list in davi data
	//
	//
	return 0;
}


int nemodavi_data_unreference(struct nemodavi_data *data)
{
	if (!data) {
		return -1;
	}

	if (!data->ref) {
		return -1;
	}

	data->ref--;
	// TODO how to keep davi list in davi data
	//
	return 0;
}

int nemodavi_data_emit_all_signals(struct nemodavi_data *data)
{
	int i;
	struct nemosignal *signal;

	if (!data) {
		return -1;
	}

	for (i = 0; i < data->data_count; i++) {
		signal = nemodavi_data_get_signal(data, i);
		nemosignal_emit(signal, &i);
	}

	return 0;
}

void* nemodavi_data_get_bind_datum(struct nemodavi_datum *datum)
{
	void *pbound = NULL;

	if (!datum) {
		return NULL;
	}

	nemodavi_data_free_datum_paramptr(datum);

	if (datum->type == NEMODAVI_DATUM_TYPE_PTR) {
		pbound = nemodavi_data_get_ptr_one(datum);
	} else if (datum->type == NEMODAVI_DATUM_TYPE_INT) {
		datum->paramptr = pbound = (int *) malloc(sizeof(int));
		* (int *) pbound = nemodavi_data_get_int_one(datum);
	} else if (datum->type == NEMODAVI_DATUM_TYPE_DOUBLE) {
		datum->paramptr = pbound = (double *) malloc(sizeof(double));
		* (double *) pbound = nemodavi_data_get_double_one(datum);
	}

	return (void *) pbound;
}

void nemodavi_data_free_datum_paramptr(struct nemodavi_datum *datum)
{
	if (!datum || !datum->paramptr) {
		return;
	}

	free(datum->paramptr);
	datum->paramptr = NULL;
}
