#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <showone.h>
#include <nemoshow.h>

#include <nemodavi.h>

#define START_ANGLE	270.0f

static struct showone* graph_append_handler(int index, void *datum, void *userdata)
{
	uint32_t total;
	char *appdata;
	double percent, diameter, w, h, from, to;
	struct showone *one;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE);
	if (!getter) {
		return NULL;
	}

	appdata = getter(index, datum);

	if (!appdata) {
		percent = 0.0;
	} else {
		percent = atof(appdata);
	}

	from = START_ANGLE;
	to = 360 * percent * 0.01;

	total = nemodavi_get_datum_count(davi);

	w = nemodavi_get_width(davi);
	h = nemodavi_get_height(davi);
	diameter = (w - h) > 0 ? h : w;
	diameter = diameter * 0.5 * ((double) (total - index) / (double) total);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_arc(one, 0, 0, diameter, diameter, from, to);

	return one;
}

static struct showone* name_append_handler(int index, void *datum, void *userdata)
{
	uint32_t total;
	char *appdata;
	double percent, diameter, w, h, from, to;
	struct showone *one, *path;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE);
	if (!getter) {
		return NULL;
	}

	appdata = getter(index, datum);

	if (!appdata) {
		percent = 0.0;
	} else {
		percent = atof(appdata);
	}

	from = 240.0f;
	to = 270.0f;

	total = nemodavi_get_datum_count(davi);

	w = nemodavi_get_width(davi);
	h = nemodavi_get_height(davi);
	diameter = (w - h) > 0 ? h : w;
	diameter = diameter * 0.5 * ((double) (total - index) / (double) total);

	one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	path = nemoshow_item_create(NEMOSHOW_PATH_ITEM);

	nemoshow_item_path_use(path, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_arc(path, 0, 0, diameter, diameter, from, to);
	nemoshow_item_set_path(one, path);

	return one;
}

static double graph_set_x(int index, void *datum, void *userdata)
{
	uint32_t total;
	double diameter, w, h, x;
	struct nemodavi *davi;

	davi = (struct nemodavi *) userdata;

	w = nemodavi_get_width(davi);
	h = nemodavi_get_height(davi);
	total = nemodavi_get_datum_count(davi);

	diameter = (w - h) > 0 ? h : w;
	diameter = diameter * 0.5 * ((double) (total - index) / (double) total);

	x = (w / 2) - (diameter / 2); 

	return x;
}

static double graph_set_y(int index, void *datum, void *userdata)
{
	uint32_t total;
	double diameter, w, h, y;
	struct nemodavi *davi;

	davi = (struct nemodavi *) userdata;

	w = nemodavi_get_width(davi);
	h = nemodavi_get_height(davi);
	total = nemodavi_get_datum_count(davi);

	diameter = (w - h) > 0 ? h : w;
	diameter = diameter * 0.5 * ((double) (total - index) / (double) total);

	y = (h / 2) - (diameter / 2); 

	return y;
}

static double name_set_x(int index, void *datum, void *userdata)
{
	double w, gap;
	char *appdata;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2);
	if (!getter) {
		gap = 0.0;
		goto __finished;
	}

	appdata = getter(index, datum);

	if (!appdata) {
		gap = 0.0;
	} else {
		gap = atof(appdata);
	}

__finished:
	w = nemodavi_get_width(davi);

	return (w / 2.0f) - gap;
}

static double name_set_y(int index, void *datum, void *userdata)
{
	return graph_set_y(index, datum, userdata);
}

static char* name_set_text(int index, void *datum, void *userdata)
{
	char *appdata;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME);

	if (!getter) {
		return NULL;
	}

	appdata = getter(index, datum);
	if (!appdata) {
		appdata = "";
	}

	return appdata;
}

static uint32_t graph_set_color(int index, void *datum, void *userdata)
{
	uint32_t color;
	char *appdata;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR);
	if (!getter) {
		return NULL;
	}

	appdata = getter(index, datum);
	if (!appdata) {
		color = 0xffffffff;
	} else {
		color = (uint32_t) atoi(appdata);
		printf("%s: %x\n", __FUNCTION__, color);
	}

	return color;
}

static double graph_set_stroke_w(int index, void *datum, void *userdata)
{
	double w;
	char *appdata;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	getter = nemodavi_layout_get_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1);
	if (!getter) {
		return 1.0f;
	}

	appdata = getter(index, datum);
	if (!appdata) {
		w = 1.0f;
	} else {
		w = atof(appdata);
	}

	return w;
}

static struct showone* create_font(struct nemoshow *show)
{
	struct showone *font;

	font = nemoshow_font_create();
	nemoshow_font_load_fontconfig(font, "NanumGothic", "bold");
	nemoshow_font_use_harfbuzz(font);

	return font;
}


static int create(struct nemodavi_layout *layout)
{
	struct nemoshow *show;
	struct showone *font;
	struct nemodavi *davi;
	struct nemodavi_selector *graph, *name;
	struct nemodavi_transition *trans;

	printf("create donut layout\n");

	davi = nemodavi_layout_get_davi(layout);
	show = davi->canvas->show;

	font = create_font(show);

	nemodavi_append_selector_by_handler(davi, "graph", graph_append_handler, layout);
	nemodavi_append_selector(davi, "name", NEMOSHOW_TEXT_ITEM);
	/*
	 * FIXME text with path doesn't align well 
	 * 
	nemodavi_append_selector_by_handler(davi, "name", name_append_handler, layout);
	*/

	graph = nemodavi_selector_get(davi, "graph");
	nemodavi_set_dattr_handler(graph, "tx", graph_set_x, davi);
	nemodavi_set_dattr_handler(graph, "ty", graph_set_y, davi);
	nemodavi_set_dattr_handler(graph, "stroke-width", graph_set_stroke_w, layout);
	nemodavi_set_cattr_handler(graph, "stroke", graph_set_color, layout);

	name = nemodavi_selector_get(davi, "name");
	nemodavi_set_dattr(name, "ax", 1.0f);
	nemodavi_set_dattr(name, "ay", 0.5f);
	nemodavi_set_oattr(name, "font", font);
	nemodavi_set_dattr(name, "font-size", 20.0f);
	nemodavi_set_dattr_handler(name, "tx", name_set_x, layout);
	nemodavi_set_dattr_handler(name, "ty", name_set_y, davi);
	nemodavi_set_sattr_handler(name, "text", name_set_text, layout);
	nemodavi_set_cattr_handler(name, "fill", graph_set_color, layout);
	nemodavi_set_cattr_handler(name, "stroke", graph_set_color, layout);

	/*
	 * FIXME arc path isn't be applied to transition... 
	 *
	trans = nemodavi_transition_create(davi);
	nemodavi_set_dattr(graph, "to", 0.0f);
	nemodavi_transition_set_dattr(trans, graph, "to", 1.0f);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_run(davi);
	*/

	return 0;
}

static int show(struct nemodavi_layout *layout)
{
	struct nemodavi *davi;
	struct nemodavi_selector *graph, *name;
	struct nemodavi_transition *trans;

	davi = nemodavi_layout_get_davi(layout);
	graph = nemodavi_selector_get(davi, "graph");
	name = nemodavi_selector_get(davi, "name");

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_dattr(trans, graph, "alpha", 1.0f, 1);
	nemodavi_transition_set_dattr(trans, name, "alpha", 1.0f, 1);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_run(davi);

	return 0;
}

static int update(struct nemodavi_layout *layout)
{
	return 0;
}

static int hide(struct nemodavi_layout *layout)
{
	struct nemodavi *davi;
	struct nemodavi_selector *graph, *name;
	struct nemodavi_transition *trans;

	davi = nemodavi_layout_get_davi(layout);
	graph = nemodavi_selector_get(davi, "graph");
	name = nemodavi_selector_get(davi, "name");

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_dattr(trans, graph, "alpha", 0.0f, 1);
	nemodavi_transition_set_dattr(trans, name, "alpha", 0.0f, 1);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_run(davi);

	return 0;
}

nemodavi_layout_delegator_t* nemodavi_layout_register_donutbar(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
