#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <showone.h>
#include <nemoshow.h>

#include <nemodavi.h>

#define HEIGHT_AREA_MAX		0.8
#define UNIT_MAX_COUNT		100
#define BAR_DEFAULT_WIDTH	10
#define BAR_DEFAULT_GAP 	20
#define NAME_DEFAULT_GAP 	10
#define INNER_WIDTH_RATE	0.8
#define FONT_DEFAULT_SIZE	20

// graph callbacks
static double graph_set_width(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t width, count;
	char *appdata;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR1, index, datum, &width);
	if (ret < 0) {
		count = nemodavi_get_datum_count(davi);
		width = nemodavi_get_width(davi) / count;
	}

	return (double) width;
}

static double graph_set_height(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t value, unit_h, count;
	char *appdata;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	unit_h = nemodavi_get_height(davi) / UNIT_MAX_COUNT;

	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR2, index, datum, &unit_h);
	if (ret < 0) {
		count = nemodavi_get_datum_count(davi);
		unit_h = nemodavi_get_height(davi) / count;
	}

	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_VALUE, index, datum, &value);
	if (ret < 0) {
		value = 0;
	}

	return (double) unit_h * value;
}

static double graph_set_tx(int index, void *datum, void *userdata)
{
	int ret;
	void *bound;
	uint32_t i, start, width, gap; 
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_datum *tdatum;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	start = nemodavi_get_width(davi) * 0.1;

	sel = nemodavi_selector_get(davi, "graph");
	for (i = 0; i < index; i++) {
		width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", i); 

		tdatum = nemodavi_get_datum(davi, i);
		bound = nemodavi_data_get_bind_datum(tdatum);
		ret = nemodavi_layout_call_getter_int(
				layout, NEMODAVI_LAYOUT_GETTER_USR3, i, bound, &gap);
		if (ret < 0) {
			gap = BAR_DEFAULT_GAP;
		}

		start += width + gap;
	}

	return (double) start;
}

static double graph_set_ty(int index, void *datum, void *userdata)
{
	uint32_t start, height;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	start = nemodavi_get_height(davi) * HEIGHT_AREA_MAX;
	height = (uint32_t) nemodavi_get_dattr_by_index(sel, "height", index); 

	start -= height;

	return (double) start;
}

static uint32_t graph_set_fill(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t color;
	char *appdata;
	struct nemodavi_layout *layout;
	nemodavi_layout_getter_t getter;

	layout = (struct nemodavi_layout *) userdata;
	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_COLOR, index, datum, &color);

	if (ret < 0) {
		color = 0xff000000;
	}

	return color;
}

// bucket callbacks
static double bucket_set_width(int index, void *datum, void *userdata)
{
	uint32_t width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 

	return width;
}

static double bucket_set_height(int index, void *datum, void *userdata)
{
	uint32_t height, width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 
	height = width / 2;

	return height;
}

static double bucket_set_tx(int index, void *datum, void *userdata)
{
	uint32_t tx;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	tx = (uint32_t) nemodavi_get_dattr_by_index(sel, "tx", index); 

	return tx;
}

static double bucket_set_ty(int index, void *datum, void *userdata)
{
	uint32_t ty, width, height;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	ty = (uint32_t) nemodavi_get_dattr_by_index(sel, "ty", index); 
	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 

	height = width / 2;
	ty = ty - height;

	return ty;
}

static uint32_t bucket_set_fill(int index, void *datum, void *userdata)
{
	uint32_t color;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	color = (uint32_t) nemodavi_get_cattr_by_index(sel, "fill", index); 

	return color;
}

// outerball callbacks
static double outerball_set_width(int index, void *datum, void *userdata)
{
	uint32_t width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 

	return width;
}

static double outerball_set_height(int index, void *datum, void *userdata)
{
	uint32_t height;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	height = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 

	return height;
}

static double outerball_set_r(int index, void *datum, void *userdata)
{
	uint32_t width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 
	width = width / 2;

	return width;
}

static double outerball_set_tx(int index, void *datum, void *userdata)
{
	uint32_t tx, width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	tx = (uint32_t) nemodavi_get_dattr_by_index(sel, "tx", index); 
	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 

	tx = tx + width / 2;

	return tx;
}

static double outerball_set_ty(int index, void *datum, void *userdata)
{
	uint32_t ty;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "bucket");

	ty = (uint32_t) nemodavi_get_dattr_by_index(sel, "ty", index); 

	return ty;
}

static uint32_t outerball_set_fill(int index, void *datum, void *userdata)
{
	uint32_t color;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	color = (uint32_t) nemodavi_get_cattr_by_index(sel, "fill", index); 

	return color;
}

// innerball callbacks
static double innerball_set_width(int index, void *datum, void *userdata)
{
	uint32_t width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 
	width = width * INNER_WIDTH_RATE;

	return width;
}

static double innerball_set_height(int index, void *datum, void *userdata)
{
	uint32_t height;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	height = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 
	height = height * INNER_WIDTH_RATE;

	return height;
}

static double innerball_set_r(int index, void *datum, void *userdata)
{
	uint32_t width;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);
	sel = nemodavi_selector_get(davi, "graph");

	width = (uint32_t) nemodavi_get_dattr_by_index(sel, "width", index); 
	width = (width * INNER_WIDTH_RATE) / 2;

	return width;
}

static double innerball_set_tx(int index, void *datum, void *userdata)
{
	return outerball_set_tx(index, datum, userdata);
}

static double innerball_set_ty(int index, void *datum, void *userdata)
{
	return outerball_set_ty(index, datum, userdata);
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
			layout, NEMODAVI_LAYOUT_GETTER_USR4, index, datum, &size);
	if (ret < 0) {
		size = FONT_DEFAULT_SIZE;
	}

	return size;
}

static double value_set_tx(int index, void *datum, void *userdata)
{
	return outerball_set_tx(index, datum, userdata);
}

static double value_set_ty(int index, void *datum, void *userdata)
{
	return outerball_set_ty(index, datum, userdata);
}

static uint32_t value_set_fill(int index, void *datum, void *userdata)
{
	return outerball_set_fill(index, datum, userdata);
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
			layout, NEMODAVI_LAYOUT_GETTER_USR6, index, datum, &size);
	if (ret < 0) {
		size = FONT_DEFAULT_SIZE;
	}

	return size;
}

static double name_set_tx(int index, void *datum, void *userdata)
{
	return outerball_set_tx(index, datum, userdata);
}

static double name_set_ty(int index, void *datum, void *userdata)
{
	int ret;
	uint32_t ty, width, height;
	struct nemodavi *davi;
	struct nemodavi_layout *layout;
	struct nemodavi_selector *sel;

	layout = (struct nemodavi_layout *) userdata; 
	davi = nemodavi_layout_get_davi(layout);

	ty = nemodavi_get_height(davi) * HEIGHT_AREA_MAX;
	ret = nemodavi_layout_call_getter_int(
			layout, NEMODAVI_LAYOUT_GETTER_USR5, index, datum, &height);
	if (ret < 0) {
		height = NAME_DEFAULT_GAP;
	}

	ty = ty + height;

	return ty;
}

static uint32_t name_set_fill(int index, void *datum, void *userdata)
{
	return outerball_set_fill(index, datum, userdata);
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

	nemodavi_append_selector(davi, "graph", NEMOSHOW_RECT_ITEM);
	nemodavi_append_selector(davi, "bucket", NEMOSHOW_RECT_ITEM);
	nemodavi_append_selector(davi, "outerball", NEMOSHOW_CIRCLE_ITEM);
	nemodavi_append_selector(davi, "innerball", NEMOSHOW_CIRCLE_ITEM);
	nemodavi_append_selector(davi, "value", NEMOSHOW_TEXT_ITEM);
	nemodavi_append_selector(davi, "name", NEMOSHOW_TEXT_ITEM);

	sel = nemodavi_selector_get(davi, "graph");
	nemodavi_set_dattr_handler(sel, "width", graph_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", graph_set_height, layout);
	nemodavi_set_dattr_handler(sel, "tx", graph_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", graph_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", graph_set_fill, layout);

	sel = nemodavi_selector_get(davi, "bucket");
	nemodavi_set_dattr_handler(sel, "width", bucket_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", bucket_set_height, layout);
	nemodavi_set_dattr_handler(sel, "tx", bucket_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", bucket_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", bucket_set_fill, layout);

	sel = nemodavi_selector_get(davi, "outerball");
	nemodavi_set_dattr_handler(sel, "width", outerball_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", outerball_set_height, layout);
	nemodavi_set_dattr_handler(sel, "r", outerball_set_r, layout);
	nemodavi_set_dattr_handler(sel, "tx", outerball_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", outerball_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", outerball_set_fill, layout);

	sel = nemodavi_selector_get(davi, "innerball");
	nemodavi_set_dattr_handler(sel, "width", innerball_set_width, layout);
	nemodavi_set_dattr_handler(sel, "height", innerball_set_height, layout);
	nemodavi_set_dattr_handler(sel, "r", innerball_set_r, layout);
	nemodavi_set_dattr_handler(sel, "tx", innerball_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", innerball_set_ty, layout);
	nemodavi_set_cattr(sel, "fill", 0xffffffff);

	sel = nemodavi_selector_get(davi, "value");
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_sattr_handler(sel, "text", value_set_text, layout);
	nemodavi_set_dattr_handler(sel, "font-size", value_set_fontsize, layout);
	nemodavi_set_dattr_handler(sel, "tx", value_set_tx, layout);
	nemodavi_set_dattr_handler(sel, "ty", value_set_ty, layout);
	nemodavi_set_cattr_handler(sel, "fill", value_set_fill, layout);

	sel = nemodavi_selector_get(davi, "name");
	nemodavi_set_oattr(sel, "font", font);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
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

nemodavi_layout_delegator_t* nemodavi_layout_register_histogram(void)
{
	nemodavi_layout_delegator_t *delegators;

	delegators = nemodavi_layout_allocate_delegators();
	
	delegators[NEMODAVI_LAYOUT_DELEGATOR_CREATE] = create;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_SHOW] = show;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_UPDATE] = update;
	delegators[NEMODAVI_LAYOUT_DELEGATOR_HIDE] = hide;

	return delegators;
}
