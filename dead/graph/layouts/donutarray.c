#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <showone.h>
#include <nemoshow.h>

#include <nemodavi.h>

#define START_ANGLE						270.0f
#define RADIUS_DEFAULT				30.0f
#define GAP_DEFAULT					  10.0f	
#define STROKE_WIDTH_DEFAULT	3.0f
#define BACK_COLOR					  0xFFD3D3D3	
#define FORE_COLOR					  0xFF000000
#define FONTSIZE_DEFAULT			10.0f

static struct showone* bgraph_append_handler(int index, void *datum, void *userdata)
{
	uint32_t total, radius;
	double value, from, to;
	struct showone *one;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	from = START_ANGLE;
	to = 360.0;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_arc(one, 0, 0, radius * 2, radius * 2, from, to);

	return one;
}

static struct showone* fgraph_append_handler(int index, void *datum, void *userdata)
{
	uint32_t total, radius;
	double value, from, to;
	struct showone *one;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	value = 0.0;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &value);

	from = START_ANGLE;
	to = 360 * value * 0.01;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_STROKE_PATH);
	nemoshow_item_path_arc(one, 0, 0, radius * 2, radius * 2, from, to);

	return one;
}

static double bgraph_set_x(int index, void *datum, void *userdata)
{
	double tx, radius, gap;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	gap = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &gap);

	tx = gap + ((radius * 2) + gap) * index;

	return tx;
}

static double bgraph_set_y(int index, void *datum, void *userdata)
{
	double ty, radius, gap;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	gap = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &gap);

	ty = gap;

	return ty;
}

static double bgraph_set_stroke_w(int index, void *datum, void *userdata)
{
	double sw;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	sw = STROKE_WIDTH_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR6, index, datum, &sw);

	return sw;
}

static uint32_t bgraph_set_stroke_color(int index, void *datum, void *userdata)
{
	uint32_t color;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	color = BACK_COLOR;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR4, index, datum, &color);

	return color;
}

static double fgraph_set_x(int index, void *datum, void *userdata)
{
	return bgraph_set_x(index, datum, userdata);
}

static double fgraph_set_y(int index, void *datum, void *userdata)
{
	return bgraph_set_y(index, datum, userdata);
}

static double fgraph_set_stroke_w(int index, void *datum, void *userdata)
{
	return bgraph_set_stroke_w(index, datum, userdata);
}

static uint32_t fgraph_set_stroke_color(int index, void *datum, void *userdata)
{
	uint32_t color;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	color = FORE_COLOR;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_COLOR, index, datum, &color);

	return color;
}

static double percent_set_size(int index, void *datum, void *userdata)
{
	double size;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	size = FONTSIZE_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR7, index, datum, &size);

	return size;
}

static double percent_set_x(int index, void *datum, void *userdata)
{
	double tx, radius, gap;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	gap = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &gap);

	tx = gap + radius + (radius * 2 + gap) * index;
}

static double percent_set_y(int index, void *datum, void *userdata)
{
	double ty, radius, gap;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	gap = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &gap);

	ty = gap + radius;

	return ty;
}

static char* percent_set_text(int index, void *datum, void *userdata)
{
	char *text;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	text = "";
	nemodavi_layout_call_getter_string(
			layout, NEMODAVI_LAYOUT_GETTER_USR9, index, datum, &text);

	return text;
}

static uint32_t percent_set_color(int index, void *datum, void *userdata)
{
	return fgraph_set_stroke_color(index, datum, userdata);
}

static double name_set_size(int index, void *datum, void *userdata)
{
	double size;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	size = FONTSIZE_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR8, index, datum, &size);

	return size;
}

static double name_set_x(int index, void *datum, void *userdata)
{
	return percent_set_x(index, datum, userdata);
}

static double name_set_y(int index, void *datum, void *userdata)
{
	double ty, radius, gap1, gap2;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;

	radius = RADIUS_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &radius);

	gap1 = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &gap1);

	gap2 = GAP_DEFAULT;
	nemodavi_layout_call_getter_double(
			layout, NEMODAVI_LAYOUT_GETTER_USR3, index, datum, &gap2);

	ty = gap1 + (radius * 2) + gap2;

	return ty;
}

static char* name_set_text(int index, void *datum, void *userdata)
{
	char *text;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	text = "";
	nemodavi_layout_call_getter_string(
			layout, NEMODAVI_LAYOUT_GETTER_NAME, index, datum, &text);

	return text;
}

static uint32_t name_set_color(int index, void *datum, void *userdata)
{
	uint32_t color;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata;
	davi = nemodavi_layout_get_davi(layout);

	color = FORE_COLOR;
	nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR5, index, datum, &color);

	return color;
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
	struct nemodavi_selector *sel;
	struct nemodavi_transition *trans;

	printf("create donut layout\n");

	davi = nemodavi_layout_get_davi(layout);
	show = davi->canvas->show;

	font = create_font(show);

	nemodavi_append_selector_by_handler(davi, "bgraph", bgraph_append_handler, layout);
	nemodavi_append_selector_by_handler(davi, "fgraph", fgraph_append_handler, layout);
	nemodavi_append_selector(davi, "percent", NEMOSHOW_TEXT_ITEM);
	nemodavi_append_selector(davi, "name", NEMOSHOW_TEXT_ITEM);

	sel = nemodavi_selector_get(davi, "bgraph");
	nemodavi_set_dattr_handler(sel, "tx", bgraph_set_x, layout);
	nemodavi_set_dattr_handler(sel, "ty", bgraph_set_y, layout);
	nemodavi_set_dattr_handler(sel, "stroke-width", bgraph_set_stroke_w, layout);
	nemodavi_set_cattr_handler(sel, "stroke", bgraph_set_stroke_color, layout);

	sel = nemodavi_selector_get(davi, "fgraph");
	nemodavi_set_dattr_handler(sel, "tx", fgraph_set_x, layout);
	nemodavi_set_dattr_handler(sel, "ty", fgraph_set_y, layout);
	nemodavi_set_dattr_handler(sel, "stroke-width", fgraph_set_stroke_w, layout);
	nemodavi_set_cattr_handler(sel, "stroke", fgraph_set_stroke_color, layout);

	sel = nemodavi_selector_get(davi, "percent");
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_dattr_handler(sel, "font-size", percent_set_size, layout);
	nemodavi_set_dattr_handler(sel, "tx", percent_set_x, layout);
	nemodavi_set_dattr_handler(sel, "ty", percent_set_y, layout);
	nemodavi_set_sattr_handler(sel, "text", percent_set_text, layout);
	nemodavi_set_cattr_handler(sel, "fill", percent_set_color, layout);

	sel = nemodavi_selector_get(davi, "name");
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_dattr_handler(sel, "font-size", name_set_size, layout);
	nemodavi_set_dattr_handler(sel, "tx", name_set_x, layout);
	nemodavi_set_dattr_handler(sel, "ty", name_set_y, layout);
	nemodavi_set_sattr_handler(sel, "text", name_set_text, layout);
	nemodavi_set_cattr_handler(sel, "fill", name_set_color, layout);

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

nemodavi_layout_delegator_t* nemodavi_layout_register_donutarray(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
