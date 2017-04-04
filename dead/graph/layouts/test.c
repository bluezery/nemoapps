#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemoshow.h>
#include <nemoease.h>
#include <showitem.h>
#include <showcanvas.h>
#include <showone.h>

#include <nemodavi.h>

static double set_graph_tx_handler(int index, void *data, void *userdata)
{
	printf("%s: %d\n", __FUNCTION__, index);
	return 100 * (index + 1);
}

static double set_graph_ty_handler(int index, void *data, void *userdata)
{
	printf("%s: %d\n", __FUNCTION__, index);
	return 100 * (index + 1);
}

static double set_graph_from_handler(int index, void *data, void *userdata)
{
	double part, from;

	part = 360 / 5;
	from = part * index;
		
	printf("%s: %d\n", __FUNCTION__, index);
	return from;
}

static double set_graph_to_handler(int index, void *data, void *userdata)
{
	double part, to;

	part = 360 / 5;
	to = part * (index + 1);
		
	printf("%s: %d\n", __FUNCTION__, index);
	return to;
}

static char* set_desc_text_handler(int index, void *data, void *userdata)
{
	char *appdata;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;


	layout = (struct nemodavi_layout *) userdata;
	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME);
	if (!getter) {
		return NULL;
	}

	appdata = getter(index, data);

	return strdup(appdata);
}

static int create(struct nemodavi_layout *layout)
{
	struct nemoshow *show;
	struct nemodavi *davi;
	struct showone *font, *one;
	struct nemodavi_selector *graph, *desc;

	printf("create test layout\n");

	davi = nemodavi_layout_get_davi(layout);
	//show = NEMOSHOW_CANVAS_AT(davi->canvas, show);
	show = davi->canvas->show;

	// append new selectors
	nemodavi_append_selector(davi, "graph", NEMOSHOW_ARC_ITEM);
	nemodavi_append_selector(davi, "desc", NEMOSHOW_TEXT_ITEM);

	// get selectors
	graph = nemodavi_selector_get(davi, "graph");
	desc = nemodavi_selector_get(davi, "desc");

	// font creation
	font = nemoshow_font_create();
	nemoshow_font_load_fontconfig(font, "NanumGothic", "bold");
	nemoshow_font_use_harfbuzz(font);

	// set graph attr
	nemodavi_set_dattr(graph, "width", 50.0f);
	nemodavi_set_dattr(graph, "height", 50.0f);
	nemodavi_set_dattr(graph, "ax", 0.5f);
	nemodavi_set_dattr(graph, "ay", 0.5f);
	nemodavi_set_cattr(graph, "fill", 0xff1edcdc);
	nemodavi_set_dattr(graph, "inner", 20.0f);
	nemodavi_set_dattr_handler(graph, "from", set_graph_from_handler, NULL);
	nemodavi_set_dattr_handler(graph, "to", set_graph_to_handler, NULL);
	nemodavi_set_dattr_handler(graph, "tx", set_graph_tx_handler, NULL);
	nemodavi_set_dattr_handler(graph, "ty", set_graph_ty_handler, NULL);

	// set desc attr
	nemodavi_set_oattr(desc, "font", font);
	nemodavi_set_dattr(desc, "font-size", 30.0f);
	nemodavi_set_cattr(desc, "fill", 0xff1edcdc);
	nemodavi_set_sattr_handler(desc, "text", set_desc_text_handler, layout);
	nemodavi_set_dattr_handler(desc, "tx", set_graph_tx_handler, NULL);
	nemodavi_set_dattr_handler(desc, "ty", set_graph_ty_handler, NULL);

	return 0;
}

static int show(struct nemodavi_layout *layout)
{
	struct nemodavi *davi;
	struct nemodavi_transition *trans;
	struct nemodavi_selector *graph, *desc;

	davi = nemodavi_layout_get_davi(layout);

	graph = nemodavi_selector_get(davi, "graph");
	desc = nemodavi_selector_get(davi, "desc");

  srand(time(NULL));

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_set_dattr(trans, graph, "inner", rand() % 30, 1);
	nemodavi_transition_set_dattr(trans, graph, "width", rand() % 50 + 50, 1);
	nemodavi_transition_set_dattr(trans, graph, "height", rand() % 50 + 50, 1);
	nemodavi_transition_set_cattr(trans, graph, "fill", 0xffffffff, 1);
	nemodavi_transition_run(davi);

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_delay(trans, 2000);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_LINEAR_TYPE);
	nemodavi_transition_set_fix_dattr(trans, graph, "ro", 360, 0, 1);
	nemodavi_transition_run(davi);

	return 0;
}

static int update(struct nemodavi_layout *layout)
{
	return 0;
}

static int hide(struct nemodavi_layout *layout)
{
	return 0;
}

nemodavi_layout_delegator_t* nemodavi_layout_register_test(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
