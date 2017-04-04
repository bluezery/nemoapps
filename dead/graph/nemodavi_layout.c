#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>

#include "nemodavi.h"
#include "nemodavi_layout.h"

// forword declaration for layout register functions
extern nemodavi_layout_delegator_t* nemodavi_layout_register_histogram(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_donut(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_donutbar(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_donutarray(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_arc(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_area(void);
extern nemodavi_layout_delegator_t* nemodavi_layout_register_test(void);

static struct nemolist *g_layout_list;

static nemodavi_layout_delegator_t **g_delegators;

static nemodavi_layout_register_t g_registers[] = {
	nemodavi_layout_register_histogram,
	nemodavi_layout_register_donut,
	nemodavi_layout_register_donutbar,
	nemodavi_layout_register_donutarray,
	nemodavi_layout_register_arc,
	nemodavi_layout_register_area,
	nemodavi_layout_register_test
};

static void nemodavi_layout_register_plugins()
{
	uint32_t i;
	size_t size;

	if (g_layout_list) {
		return;
	}

	g_layout_list = (struct nemolist *) malloc(sizeof(struct nemolist));
	nemolist_init(g_layout_list);

	size = sizeof(nemodavi_layout_delegator_t *) * NEMODAVI_LAYOUT_LAST;
	g_delegators = (nemodavi_layout_delegator_t **) malloc(size);
	memset(g_delegators, 0x0, size);

	for (i = 0; i < NEMODAVI_LAYOUT_LAST; i++) {
		g_delegators[i] = g_registers[i]();
	}
}

static char* nemodavi_layout_call_getter(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum)
{
	nemodavi_layout_getter_t getter;
	getter = nemodavi_layout_get_getter(layout, type); 
	if (!getter) {
		return NULL;
	}

	return getter(index, datum);
}

struct nemodavi_layout* nemodavi_layout_append(struct nemodavi *davi, NemoDaviLayoutType type)
{
	struct nemodavi_layout *layout;

	if (!davi || davi->layout) {
		return NULL;
	}

	if (type < 0 || type > NEMODAVI_LAYOUT_LAST) {
		return NULL;
	}

	if (!g_layout_list) {
		nemodavi_layout_register_plugins();
	}

	if (!g_delegators[type]) {
		return NULL;
	}

	layout = (struct nemodavi_layout *) malloc(sizeof(struct nemodavi_layout));
	memset(layout, 0x0, sizeof(struct nemodavi_layout));

	layout->type = type;
	layout->davi = davi;
	layout->delegators = g_delegators[type];

	nemolist_insert_tail(g_layout_list, &layout->link);
	nemodavi_set_layout(davi, layout);

	return layout;
}

int nemodavi_layout_call_delegator(struct nemodavi_layout *layout, NemoDaviLayoutDelegatorType type)
{
	struct nemodavi_layout *current;

	if (!layout) {
		return -1;
	}

	if (layout->created && (type == NEMODAVI_LAYOUT_DELEGATOR_CREATE)) {
		return -1;
	}

	nemolist_for_each(current, g_layout_list, link) {
		if (current->davi == layout->davi) {
			if (layout->delegators[type]) {
				layout->delegators[type](layout);
			}
			break;
		}
	}

	if (type == NEMODAVI_LAYOUT_DELEGATOR_CREATE) {
		layout->created = 1;
	}

	return 0;
}

int nemodavi_layout_set_getter(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, nemodavi_layout_getter_t getter)
{
	if (!layout) {
		return -1;
	}

	if (type < 0 || type >= NEMODAVI_LAYOUT_GETTER_LAST) {
		return -1;
	}

	layout->getters[type] = getter;

	return 0;
}

nemodavi_layout_getter_t nemodavi_layout_get_getter(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type)
{
	if (!layout) {
		return NULL;
	}

	if (type < 0 || type >= NEMODAVI_LAYOUT_GETTER_LAST) {
		return NULL;
	}

	return layout->getters[type];
}

int nemodavi_layout_call_getter_int(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, int *out)
{
	char *result;

	result = nemodavi_layout_call_getter(layout, type, index, datum);

	if (!result) {
		return -1;
	}

	*out = strtoul(result, (char **) NULL, 0);
	return 0;
}

int nemodavi_layout_call_getter_double(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, double *out)
{
	char *result;

	result = nemodavi_layout_call_getter(layout, type, index, datum);

	if (!result) {
		return -1;
	}

	*out = atof(result);
	return 0;
}

int nemodavi_layout_call_getter_string(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, char **out)
{
	char *result;

	result = nemodavi_layout_call_getter(layout, type, index, datum);

	if (!result) {
		return -1;
	}

	*out = result;
	return 0;
}

nemodavi_layout_delegator_t* nemodavi_layout_allocate_delegators()
{
	size_t size;
	nemodavi_layout_delegator_t *delegators;

	size = sizeof(nemodavi_layout_delegator_t) * NEMODAVI_LAYOUT_DELEGATOR_LAST;
	delegators = (nemodavi_layout_delegator_t *) malloc(size);
	memset(delegators, 0x0, size);

	return delegators;
}
