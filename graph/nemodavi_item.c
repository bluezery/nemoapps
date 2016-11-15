#include <stdio.h>
#include <stdlib.h>

#include <nemoshow.h>
#include <showone.h>

#include "nemodavi_item.h"

typedef struct showone* (*nemodavi_item_create_t) (void);

static nemodavi_item_create_t *g_creators;

static struct showone* create_rdonut()
{
	struct showone *one;
	//one = nemoshow_item_create(NEMOSHOW_DONUT_ITEM);
	return one;
}

static struct showone* create_rbar()
{
	struct showone *one;
	one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	return one;
}

static void nemodavi_item_register_creators()
{
	size_t size;
	if (g_creators) {
		return;
	}

	size = sizeof(nemodavi_item_create_t) * (NEMODAVI_LAST_ITEM - NEMOSHOW_LAST_ITEM + 1);
	g_creators = (nemodavi_item_create_t *) malloc(size); 
	memset(g_creators, 0x0, size);

	// TODO this should be maintained according to adding new davi item type
	g_creators[NEMODAVI_RDONUT_ITEM - NEMOSHOW_LAST_ITEM] = create_rdonut; 
	g_creators[NEMODAVI_RBAR_ITEM - NEMOSHOW_LAST_ITEM] = create_rbar; 
}

struct showone* nemodavi_item_create(int type)
{
	uint32_t index;

	if (type < 0 || type >= NEMODAVI_LAST_ITEM) {
		return NULL;
	}

	if (!g_creators) {
		nemodavi_item_register_creators();
	}

	if (type < NEMOSHOW_LAST_ITEM) {
		return nemoshow_item_create(type);
	}

	index = type - NEMOSHOW_LAST_ITEM;
	return g_creators[index]? g_creators[index]() : NULL;
}
