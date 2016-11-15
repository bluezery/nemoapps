#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <showone.h>
#include <nemoshow.h>

#include <nemodavi.h>

#define GRAPH_DEFAULT_DIA_RATE				0.6
#define GRAPH_DEFAULT_INNER_RATE			GRAPH_DEFAULT_DIA_RATE * 0.07
#define GRAPH_GAP_RATE								GRAPH_DEFAULT_INNER_RATE * 0.2 
#define NAME_GAP_RATE									GRAPH_DEFAULT_INNER_RATE * 0.4 
#define SUBGRAPH_DEFAULT_INNER_RATE		GRAPH_GAP_RATE
#define FONT_DEFAULT_SIZE							20

static double graph_set_width(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t width, count;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &width);
	if (ret < 0) {
		width = nemodavi_get_width(davi) * GRAPH_DEFAULT_DIA_RATE;
	}

	return (double) width;
}

static double graph_set_height(int index, void *datum, void *userdata)
{
	return graph_set_width(index, datum, userdata);
}

static double graph_set_inner(int index, void *datum, void *userdata)
{
	int ret;
	double inner;
	struct nemodavi_layout *layout;
	struct nemodavi *davi;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &inner);

	if (ret < 0) {
		inner = nemodavi_get_width(davi) * GRAPH_DEFAULT_INNER_RATE;
	}

	return inner;
}

static uint32_t graph_set_fill(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t color;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_COLOR, index, datum, &color);

	if (ret < 0) {
		color = 0xff000000;
	}

	return color;
}

static double graph_get_angle(int index, struct nemodavi_layout *layout)
{
	int i, ret;
	uint32_t total, value, count, currtotal;
	double angle;
	void *bound;
	struct nemodavi *davi;
	struct nemodavi_selector *sel;
	struct nemodavi_datum *tmp;

	if (!layout || index < 0) {
		printf("%s] %d: %f\n", __FUNCTION__, index, 0.0);
		return 0.0;
	}

	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");
	count = nemodavi_get_datum_count(davi);

	total = currtotal = 0;

	for (i = 0; i < count; i++) {
		value = 0;

		tmp = nemodavi_get_datum(davi, i);
		bound = nemodavi_data_get_bind_datum(tmp);
		nemodavi_layout_call_getter_int(
				layout, NEMODAVI_LAYOUT_GETTER_VALUE, i, bound, &value);

		total += value;

		if (i == index) {
			currtotal = total;
		} 
	}

	angle = 360 * ((double) currtotal / (double)total);

	printf("%s] %d: %f\n", __FUNCTION__, index, angle);
	return angle;
}

static double graph_set_from(int index, void *datum, void *userdata)
{
	return graph_get_angle(index - 1, (struct nemodavi_layout *) userdata);
}

static double graph_set_to(int index, void *datum, void *userdata)
{
	return graph_get_angle(index, (struct nemodavi_layout *) userdata);
}

static double graph_set_sw(int index, void *datum, void *userdata)
{
	int ret;
	double sw;
	struct nemodavi_layout *layout;
	struct nemodavi *davi;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR8, index, datum, &sw);

	if (ret < 0) {
		sw = 0.0;
	}

	return sw;
}

static double subgraph_set_width(int index, void *datum, void *userdata)
{
	int i, ret, count;
	double gap, graph_inner, graph_width;
	double width, tmp_width, sub_inner;
	void *bound;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;
	struct nemodavi_datum *tmp_datum;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");
	count = nemodavi_get_datum_count(davi);
	width = nemodavi_get_width(davi);

	for (i = 0; i < count; i++) {
		graph_width = nemodavi_get_dattr_by_index(sel, "width", i); 
		graph_inner = nemodavi_get_dattr_by_index(sel, "inner", i); 

		tmp_width = graph_width - (graph_inner * 2);
		if (tmp_width < width) {
			width = tmp_width;
		}
	}

	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR4, index, datum, &gap);
	if (ret < 0) {
		gap = nemodavi_get_width(davi) * GRAPH_GAP_RATE;
	}
	width -= (gap * 2);

	for (i = 0; i < index; i++) {
		sub_inner = 0.0;

		tmp_datum = nemodavi_get_datum(davi, i);
		bound = nemodavi_data_get_bind_datum(tmp_datum);
		nemodavi_layout_call_getter_double(
				layout, NEMODAVI_LAYOUT_GETTER_USR3, i, bound, &sub_inner);
		width -= (sub_inner * 2);	
	}

	return width;
}

static double subgraph_set_height(int index, void *datum, void *userdata)
{
	return subgraph_set_width(index, datum, userdata);
}

static double subgraph_set_inner(int index, void *datum, void *userdata)
{
	int ret;
	double inner;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);
	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR3, index, datum, &inner);

	if (ret < 0) {
		inner = nemodavi_get_width(davi) * SUBGRAPH_DEFAULT_INNER_RATE;
	}

	return inner;
}

static uint32_t subgraph_set_fill(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t color;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR5, index, datum, &color);

	if (ret < 0) {
		color = 0xff000000;
	}

	return color;
}

static double subgraph_set_from(int index, void *datum, void *userdata)
{
	double from;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	from = (uint32_t) nemodavi_get_dattr_by_index(sel, "from", index); 

	return from;
}

static double subgraph_set_to(int index, void *datum, void *userdata)
{
	double to;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	to = (uint32_t) nemodavi_get_dattr_by_index(sel, "to", index); 

	return to;
}

// value callbacks
static char* value_set_text(int index, void *datum, void *userdata)
{
	int ret;
	char *text;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_string(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &text);
	if (ret < 0) {
		text = "";
	}

	return text;
}

static double value_set_fontsize(int index, void *datum, void *userdata)
{
	int ret;
	double size;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR6, index, datum, &size);
	if (ret < 0) {
		size = FONT_DEFAULT_SIZE;
	}

	return size;
}

static double value_set_tx(int index, void *datum, void *userdata)
{
	// TODO this should be replaced to proper value
	return 0;
}

static double value_set_ty(int index, void *datum, void *userdata)
{
	// TODO this should be replaced to proper value
	return 0;
}

static uint32_t value_set_fill(int index, void *datum, void *userdata)
{
	// TODO this should be replaced to proper value
	return 0xffffffff;
}

// name callbacks
static char* name_set_text(int index, void *datum, void *userdata)
{
	int ret;
	char *text;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_string(
			layout, NEMODAVI_LAYOUT_GETTER_NAME, index, datum, &text);
	if (ret < 0) {
		text = "";
	}

	return text;
}

static double name_set_fontsize(int index, void *datum, void *userdata)
{
	int ret;
	double size;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR7, index, datum, &size);
	if (ret < 0) {
		size = FONT_DEFAULT_SIZE;
	}

	return size;
}

static double name_set_tx(int index, void *datum, void *userdata)
{
	double tx, cx, w, r, gap;

	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	gap = NAME_GAP_RATE;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR9, index, datum, &gap);

	w = graph_set_width(index, datum, userdata) + gap;
	r = w / 2;
	cx = nemodavi_get_width(davi) / 2;

	tx = cx - r;

	return tx;
}

static double name_set_ty(int index, void *datum, void *userdata)
{
	double ty, cy, h, r, gap;

	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	gap = NAME_GAP_RATE;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR9, index, datum, &gap);

	h = graph_set_height(index, datum, userdata) + gap;
	r = h / 2;
	cy = nemodavi_get_height(davi) / 2;

	ty = cy - r;

	return ty;
}

static uint32_t name_set_fill(int index, void *datum, void *userdata)
{
	return graph_set_fill(index, datum, userdata);
}

static struct showone* create_font(struct nemoshow *show)
{
	struct showone *font;

	font = nemoshow_font_create();
	nemoshow_font_load_fontconfig(font, "NanumGothic", "bold");
	nemoshow_font_use_harfbuzz(font);

	return font;
}

static struct showone* name_append_handler(int index, void *datum, void *userdata)
{
	uint32_t total;
	char *name;
	double percent, diameter, width, from, to, gap;
	struct showone *one, *path;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	nemodavi_layout_call_getter_string(
		layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &name);
	
	from = graph_get_angle(index - 1, layout);
	to = graph_get_angle(index, layout);
	width = graph_set_width(index, datum, userdata);

	gap = NAME_GAP_RATE;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR9, index, datum, &gap);

	diameter = width + gap;

	one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	path = nemoshow_item_create(NEMOSHOW_PATH_ITEM);

	nemoshow_item_path_use(path, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_arc(path, 0, 0, diameter, diameter, from, to - from);
	nemoshow_item_set_path(one, path);

	return one;
}

static int create(struct nemodavi_layout *layout)
{
	uint32_t count;
	double tx, ty;
	struct showone *font;
	struct nemodavi *davi;
	struct nemodavi_selector *sel;

	davi = nemodavi_layout_get_davi(layout);
	count = nemodavi_get_datum_count(davi);
	font = create_font(davi->canvas->show);

	tx = nemodavi_get_width(davi) / 2;
	ty = nemodavi_get_height(davi) / 2;

	nemodavi_append_selector(davi, "graph", NEMOSHOW_ARC_ITEM);
	nemodavi_append_selector(davi, "subgraph", NEMOSHOW_ARC_ITEM);
	nemodavi_append_selector(davi, "value", NEMOSHOW_TEXT_ITEM);
	nemodavi_append_selector_by_handler(davi, "name", name_append_handler, layout);
	//nemodavi_append_selector(davi, "name", NEMOSHOW_TEXT_ITEM);

	sel = nemodavi_selector_get(davi, "graph");
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_dattr(sel, "tx", tx);
	nemodavi_set_dattr(sel, "ty", ty);
	nemodavi_set_cattr(sel, "stroke", 0xffffffff);
	nemodavi_set_dattr_handler(sel, "width", graph_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", graph_set_height, layout);
	nemodavi_set_dattr_handler(sel, "inner", graph_set_inner, layout);
	nemodavi_set_cattr_handler(sel, "fill", graph_set_fill, layout);
	nemodavi_set_dattr_handler(sel, "from", graph_set_from, layout);
	nemodavi_set_dattr_handler(sel, "to", graph_set_to, layout);
	nemodavi_set_dattr_handler(sel, "stroke-width", graph_set_sw, layout);

	sel = nemodavi_selector_get(davi, "subgraph");
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_dattr(sel, "tx", tx);
	nemodavi_set_dattr(sel, "ty", ty);
	nemodavi_set_dattr_handler(sel, "width", subgraph_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", subgraph_set_height, layout);
	nemodavi_set_dattr_handler(sel, "inner", subgraph_set_inner, layout);
	nemodavi_set_cattr_handler(sel, "fill", subgraph_set_fill, layout);
	nemodavi_set_dattr_handler(sel, "from", subgraph_set_from, layout);
	nemodavi_set_dattr_handler(sel, "to", subgraph_set_to, layout);

	sel = nemodavi_selector_get(davi, "value");
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_sattr_handler(sel, "text", value_set_text, layout);
	nemodavi_set_dattr_handler(sel, "font-size", value_set_fontsize, layout);
	nemodavi_set_dattr_handler(sel, "tx", value_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", value_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", value_set_fill, layout);

	sel = nemodavi_selector_get(davi, "name");
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_sattr_handler(sel, "text", name_set_text, layout);
	nemodavi_set_dattr_handler(sel, "font-size", name_set_fontsize, layout);
	nemodavi_set_dattr_handler(sel, "tx", name_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", name_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", name_set_fill, layout);

	return 0;
}

static int show(struct nemodavi_layout *layout)
{
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

nemodavi_layout_delegator_t* nemodavi_layout_register_donut(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
