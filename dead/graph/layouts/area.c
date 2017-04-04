#include <stdio.h>

#include "nemodavi.h"
#include "nemodavi_layout.h"

#define HIGHT_MAX_RATE	0.5
#define X_START_RATE 0.0

struct showone* fgraph_append_handler(int index, void *datum, void *userdata)
{
	uint32_t x0, y0, x1, y1, xidx, count, xnum, xunit, ymax, ynum, yunit, value;
	void *bound;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct showone *one;
	struct nemodavi_datum *tdatum;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &xnum);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &xunit);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR3, index, datum, &ymax);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR4, index, datum, &ynum);

	yunit = nemodavi_get_height(davi) * HIGHT_MAX_RATE / ymax;

	nemoshow_item_path_use(one, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_clear(one);

	if (index == 0) {
		x0 = 0;
		y0 = nemodavi_get_height(davi) * HIGHT_MAX_RATE;
		nemoshow_item_path_moveto(one, x0, y0);
	} else {
		x0 = index * xunit;

		tdatum = nemodavi_get_datum(davi, index - 1);
		bound = nemodavi_data_get_bind_datum(tdatum);
		nemodavi_layout_call_getter_int(
				layout, NEMODAVI_LAYOUT_GETTER_VALUE, index - 1, bound, &value);
		y0 = nemodavi_get_height(davi) * HIGHT_MAX_RATE - value * yunit; 
		nemoshow_item_path_moveto(one, x0, y0);
	}

	x1 = (index + 1) * xunit;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &value);
	y1 = nemodavi_get_height(davi) * HIGHT_MAX_RATE - value * yunit; 

	nemoshow_item_path_lineto(one, x1, y1);

	nemoshow_item_path_close(one);

	return one;
}

struct showone* bgraph_append_handler(int index, void *datum, void *userdata)
{
	uint32_t x0, y0, x1, y1, xidx, count, xnum, xunit, ymax, ynum, yunit, value;
	void *bound;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct showone *one;
	struct nemodavi_datum *tdatum;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &xnum);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &xunit);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR3, index, datum, &ymax);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR4, index, datum, &ynum);

	yunit = nemodavi_get_height(davi) * HIGHT_MAX_RATE / ymax;
	count = nemodavi_get_datum_count(davi);

	nemoshow_item_path_use(one, NEMOSHOW_ITEM_FILL_PATH);
	nemoshow_item_path_clear(one);

	if (index - 1 < 0) {
		x0 = 0;
		y0 = nemodavi_get_height(davi) * HIGHT_MAX_RATE;

		nemoshow_item_path_moveto(one, count * xunit , y0);
		nemoshow_item_path_lineto(one, x0, y0);

	} else {
		x0 = index * xunit;

		tdatum = nemodavi_get_datum(davi, index - 1);
		bound = nemodavi_data_get_bind_datum(tdatum);
		nemodavi_layout_call_getter_int(
				layout, NEMODAVI_LAYOUT_GETTER_VALUE, index - 1, bound, &value);
		y0 = nemodavi_get_height(davi) * HIGHT_MAX_RATE - value * yunit; 
		nemoshow_item_path_moveto(one, x0, y0);
	}

	x1 = (index + 1) * xunit;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &value);
	y1 = nemodavi_get_height(davi) * HIGHT_MAX_RATE - value * yunit; 

	nemoshow_item_path_lineto(one, x1, y1);

	// last index
	x0 = count * xunit;
	y0 = nemodavi_get_height(davi) * HIGHT_MAX_RATE;

	nemoshow_item_path_lineto(one, x0, y0);

	nemoshow_item_path_close(one);

	return one;
}

uint32_t bgraph_set_fill_color(int index, void *datum, void *userdata)
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

	color |= 0xffdddddd;

	return color;
}

uint32_t fgraph_set_stroke_color(int index, void *datum, void *userdata)
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

double fgraph_set_stroke_width(int index, void *datum, void *userdata)
{
	int ret;
	double width;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	ret = nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR6, index, datum, &width);

	if (ret < 0) {
		width = 1.0f;
	}

	return width;
}

double point_set_tx(int index, void *datum, void *userdata)
{
	double tx;
	uint32_t xunit;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &xunit);


	tx = (index + 1) * xunit;

	return tx;
}

double point_set_ty(int index, void *datum, void *userdata)
{
	double ty;
	uint32_t value, yunit, ymax;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR3, index, datum, &ymax);

	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &value);

	yunit = nemodavi_get_height(davi) * HIGHT_MAX_RATE / ymax;

	ty = nemodavi_get_height(davi) * HIGHT_MAX_RATE - value * yunit; 

	return ty;
}

double point_set_width(int index, void *datum, void *userdata)
{
	uint32_t num;
	double width, sw;

	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	sw = fgraph_set_stroke_width(index, datum, userdata);
	width  = sw * 2;

	num = nemodavi_get_datum_count(davi);
	if (index == num - 1) {
		width  = sw * 5;
	}

	return width;
}

double point_set_height(int index, void *datum, void *userdata)
{
	return point_set_width(index, datum, userdata); 
}

double point_set_inner(int index, void *datum, void *userdata)
{
	double inner, width;

	width = point_set_width(index, datum, userdata);
	inner = width / 2;

	return inner;
}

double point_set_stroke_width(int index, void *datum, void *userdata)
{
	double sw;

	sw = fgraph_set_stroke_width(index, datum, userdata);

	return sw;
}

double point_set_stroke_color(int index, void *datum, void *userdata)
{
	return fgraph_set_stroke_color(index, datum, userdata);
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
	uint32_t count;
	double unitw, unith;
	struct showone *font;
	struct nemodavi *davi;
	struct nemodavi_selector *sel;

	davi = nemodavi_layout_get_davi(layout);
	count = nemodavi_get_datum_count(davi);
	font = create_font(davi->canvas->show);

	nemodavi_append_selector_by_handler(davi, "bgraph", bgraph_append_handler, layout);
	sel = nemodavi_selector_get(davi, "bgraph");
	nemodavi_set_cattr_handler(sel, "fill", bgraph_set_fill_color, layout);
	nemodavi_set_cattr_handler(sel, "stroke", bgraph_set_fill_color, layout);
	nemodavi_set_dattr(sel, "stroke-width", 1.0f);

	nemodavi_append_selector_by_handler(davi, "fgraph", fgraph_append_handler, layout);
	sel = nemodavi_selector_get(davi, "fgraph");
	nemodavi_set_cattr_handler(sel, "stroke", fgraph_set_stroke_color, layout);
	nemodavi_set_dattr_handler(sel, "stroke-width", fgraph_set_stroke_width, layout);

	nemodavi_append_selector(davi, "point", NEMOSHOW_CIRCLE_ITEM);
	sel = nemodavi_selector_get(davi, "point");
	nemodavi_set_dattr(sel, "from", 0.0);
	nemodavi_set_dattr(sel, "to", 360.0);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xffffffff);
	nemodavi_set_dattr_handler(sel, "tx", point_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", point_set_ty, layout);
	nemodavi_set_dattr_handler(sel, "width", point_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", point_set_height, layout);
	nemodavi_set_dattr_handler(sel, "inner", point_set_inner, layout);
	nemodavi_set_cattr_handler(sel, "stroke", point_set_stroke_color, layout);
	nemodavi_set_dattr_handler(sel, "stroke-width", point_set_stroke_width, layout);

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

nemodavi_layout_delegator_t* nemodavi_layout_register_area(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
