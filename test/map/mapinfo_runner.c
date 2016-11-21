#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemoshow.h>

#include <nemowrapper.h>
#include <nemodavi.h>
#include <nemodavi_data.h>
#include <nemodavi_attr.h>
#include <nemodavi_selector.h>
#include <nemodavi_transition.h>

#include "mapinfo.h"
#include "mapinfo_nemoutil.h"

#define PI	3.14159265
#define RGBA2ARGB(color) \
    ((color & 0xFFFFFF00) >> 8 | (color & 0x000000FF) << 24)

struct ui_props {
	double SCENE_W;
	double SCENE_H;

	double RUNNER_NUM_MAX;
	double AREA_SUB_DONUT_NUM;

	double RUNNER_START_ANGLE;
	double RUNNER_DONUT_INNER;
	double AREA_START_ANGLE;
	double AREA_DONUT_INNER;
	double AREA_DONUT_ACTIVE_INNER;
	double AREA_SUB_DONUT_GAP;

	double RUNNER_DONUT_W;
	double RUNNER_DONUT_H;
	double RUNNER_SUB_DONUT_W;
	double RUNNER_SUB_DONUT_H;
	double AREA_DONUT_W;
	double AREA_DONUT_H;

	double RUNNER_PHOTO_W;
	double RUNNER_PHOTO_H;
	double RUNNER_VOTE_SIZE;
	double RUNNER_NAME_SIZE;
	double RUNNER_INFO_GAP;
	double RUNNER_ARROW_LEN;
	double RUNNER_SUB_DONUT_NUM;

	double OUT_BOUND_DONUT_INNER;
	double IN_BOUND_DONUT_INNER;
	int32_t TIMER_INTERVAL;
};

struct man_ui {
	int cx;
	int cy;
	int endx;
	int endy;
	int votex;
	int votey;
};

struct context {
	struct showone *canvas;
	struct showone *blur;
	struct showone *font;
	struct man_ui *runners;
	struct ui_props *props;
	Area *area;

	int width;
	int height;

	// new members
	struct nemodavi *areadv;
	struct nemodavi *mandv;
};

static struct context *g_context;

static struct ui_props* init_props(int scene_w, int scene_h)
{
	struct ui_props* props;
  props	= (struct ui_props*) malloc(sizeof(struct ui_props));

	if (!props) {
		return NULL;
	}
	memset(props, 0x0, sizeof(struct ui_props));
	props->SCENE_W = (double) scene_w;
	props->SCENE_H = (double) scene_h;

	props->RUNNER_NUM_MAX = 3;
	props->AREA_SUB_DONUT_NUM = 3;

	props->RUNNER_START_ANGLE = 0.0f;
	props->RUNNER_DONUT_INNER = scene_w * 0.017;
	props->AREA_START_ANGLE = 0.0f;
	props->AREA_DONUT_INNER = scene_w * 0.005f;
	props->AREA_DONUT_ACTIVE_INNER = scene_w * 0.03f;
	props->AREA_SUB_DONUT_GAP = 15.0f; // angle

	props->RUNNER_DONUT_W = scene_w * 0.9f;
	props->RUNNER_DONUT_H = props->RUNNER_DONUT_W;
	props->RUNNER_SUB_DONUT_W = scene_w * 0.2f;
	props->RUNNER_SUB_DONUT_H = props->RUNNER_SUB_DONUT_W;
	props->AREA_DONUT_W = scene_w * 0.2f;
	props->AREA_DONUT_H = props->AREA_DONUT_W;

	props->RUNNER_PHOTO_W = scene_w * 0.1f;
	props->RUNNER_PHOTO_H = props->RUNNER_PHOTO_W;
	props->RUNNER_VOTE_SIZE = scene_w * 0.05f;
	props->RUNNER_NAME_SIZE = scene_w * 0.03f;
	props->RUNNER_INFO_GAP = scene_w * 0.005f;
	props->RUNNER_ARROW_LEN = scene_w * 0.09f;
	props->OUT_BOUND_DONUT_INNER = scene_w * 0.025f;
	props->IN_BOUND_DONUT_INNER = scene_w * 0.005f;

	props->TIMER_INTERVAL = 500;

	return props;
}

static double set_man_vote_ro(int index, void *datum)
{
	int count;
	static double to = 0.0;
	Area *area = g_context->area;

	count = list_count(area_get_runners(area));
	to = mapinfo_util_get_runner_start_angle(count, index);
	return to;
}

static char* set_man_vote_text(int index, void *datum)
{
	char buff[32];
	Runner *runner;
	Area *area = g_context->area;

	runner = (Runner *) datum;
	sprintf(buff, "%.1f%c", ((double) runner->votes / (double) area->vote_total) * 100.0f, '%');
	printf("%d) vote text: %s\n", index, buff);

	return strdup(buff);
}

static double set_man_vote_xpos(int index, void *datum)
{
	return g_context->runners[index].votex;
}

static double set_man_vote_ypos(int index, void *datum)
{
	return g_context->runners[index].votey;
}

static void set_area_attr(struct nemodavi *davi)
{
	int i;
	double size;
	struct ui_props *pr;
	char buff[32];
	struct nemodavi_selector *sel;

	pr = g_context->props;

	size = pr->RUNNER_DONUT_W;
 	size += (pr->OUT_BOUND_DONUT_INNER - pr->RUNNER_DONUT_INNER);

	sel = nemodavi_selector_get(davi, "outbound");
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->OUT_BOUND_DONUT_INNER);
	nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "fill", 0x30ffffff);

	sel = nemodavi_selector_get(davi, "inbound");
	size = pr->RUNNER_DONUT_W - 7 * pr->OUT_BOUND_DONUT_INNER;
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->IN_BOUND_DONUT_INNER);
	nemodavi_set_dattr(sel, "to", 360.0f);
	nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "fill", 0xff1edcdc);
	nemodavi_set_dattr(sel, "alpha", 0.0f);

	sel = nemodavi_selector_get(davi, "center");
	size = pr->AREA_DONUT_W;
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->AREA_DONUT_INNER);
	nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "fill", 0xff1edcdc);

	sel = nemodavi_selector_get(davi, "name");
	nemodavi_set_oattr(sel, "font", g_context->font);
	nemodavi_set_dattr(sel, "font-size", pr->RUNNER_NAME_SIZE * 2);
	nemodavi_set_sattr(sel, "text", "");
	nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "fill", 0xff1edcdc);
	nemodavi_set_dattr(sel, "alpha", 0.0f);

	size = pr->AREA_DONUT_W - 4 * pr->AREA_DONUT_INNER;
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		sprintf(buff, "sub%d", i);
		sel = nemodavi_selector_get(davi, buff);
		nemodavi_set_dattr(sel, "width", size);
		nemodavi_set_dattr(sel, "height", size);
		nemodavi_set_dattr(sel, "inner", pr->AREA_DONUT_INNER);
		nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
		nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
		nemodavi_set_dattr(sel, "ax", 0.5f);
		nemodavi_set_dattr(sel, "ay", 0.5f);
		nemodavi_set_oattr(sel, "filter", g_context->blur);
		nemodavi_set_cattr(sel, "fill", 0xff1edcdc);
	}
}

static struct showone* create_runner_outpath(struct context* context, double angle, double radius)
{
	double x0, y0, x1, y1, x2, y2, w, h, r, hh;
	double scene_w, scene_h, arrow_len;
	double photo_w, radian;
	struct showone *path, *path2;

	if (!context) {
		return -1;
	}

	scene_w = context->props->SCENE_W;
	scene_h = context->props->SCENE_H;
	arrow_len = (context->props->RUNNER_DONUT_W - (context->props->RUNNER_DONUT_W - 3 * context->props->OUT_BOUND_DONUT_INNER)) / 2.0f;

	photo_w = context->props->RUNNER_PHOTO_W;
	radian = (double) (angle * (PI / 180));

	x0 = scene_w / 2.0f + radius * cos(radian);
	y0 = scene_h / 2.0f + radius * sin(radian);

	x1 = scene_w / 2.0f + (radius - arrow_len) * cos(radian);
	y1 = scene_h / 2.0f + (radius - arrow_len) * sin(radian);

	path = PATH_CREATE(context->canvas, NULL, 1, 0);

	nemoshow_item_path_clear(path);
	nemoshow_item_path_moveto(path, x0, y0, 1, 0);
	nemoshow_item_path_lineto(path, x1, y1, 1, 0);
	nemoshow_item_path_close(path, 1, 0);

	return path;
}

static struct showone* create_runner_inpath(struct context* context, struct man_ui* runner, double angle, double radius)
{
	double x0, y0, x1, y1, x2, y2, w, h, r, hh;
	double scene_w, scene_h, arrow_len;
	double photo_w, radian;
	struct showone *path, *path2;

	if (!context) {
		return -1;
	}

	scene_w = context->props->SCENE_W;
	scene_h = context->props->SCENE_H;
	arrow_len = context->props->RUNNER_ARROW_LEN * 2;
	photo_w = context->props->RUNNER_PHOTO_W;
	radian = (double) (angle * (PI / 180));

	x0 = scene_w / 2.0f + radius * cos(radian);
	y0 = scene_h / 2.0f + radius * sin(radian);

	x1 = runner->endx = scene_w / 2.0f + (radius + arrow_len) * cos(radian);
	y1 = runner->endy = scene_h / 2.0f + (radius + arrow_len) * sin(radian);

	w = h = 1 * photo_w;

	path = PATH_CREATE(context->canvas, NULL, 1, 0);

	nemoshow_item_path_clear(path);
	nemoshow_item_path_moveto(path, x0, y0, 1, 0);
	nemoshow_item_path_lineto(path, x1, y1, 1, 0);

	path2 = mapinfo_util_create_circle_path(w / 2, 1, 0);
	nemoshow_item_set_stroke_color(path2, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_stroke_width(path2, 3.5f);
	nemoshow_item_path_rotate(path2, angle - 90);

	nemoshow_item_path_translate(path2, x1, y1);
	nemoshow_item_path_append(path, path2);

	nemoshow_item_set_stroke_color(path, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_stroke_width(path, 3.5f);
	nemoshow_item_set_filter(path, context->blur);
	//nemoshow_item_set_from(path, 0.0f);
	nemoshow_item_set_to(path, 0.0f);

	runner->votex = (x0 + x1) / 2;
	runner->votey = (y0 + y1) / 2;

	return path;
}


static struct showone* set_outpath_path(int index, void *datum)
{
	double r;
	static double from = 0.0, to = 0.0;
	struct showone *one;
	Runner *runner;
	Area *area = g_context->area;

	r = (double) g_context->props->RUNNER_DONUT_W / 2;
	runner = (Runner *) datum;

	if (index == 0) {
		from = 0;
	} else {
		from += to;
	}
	to = 360 * ((double) runner->votes / (double) area->vote_total);

	one = create_runner_outpath(g_context, from + to, r);
	return one;
}

static struct showone* set_inpath_path(int index, void *datum)
{
	int count;
	double r, to;
	struct showone *one;
	Area *area = g_context->area;
	Runner *runner;

	runner = (Runner *) datum;
	r = (double) g_context->props->AREA_DONUT_W / 2;

	count = list_count(area_get_runners(area));
	to = mapinfo_util_get_runner_start_angle(count, index);
	one = create_runner_inpath(g_context, &g_context->runners[index], to, r);

	return one;
}

static char* set_man_name(int index, void *datum)
{
	Runner *runner;
	runner = (Runner *) datum;
	return runner->name;
}

static double set_man_graph_from(int index, void *datum)
{
	static double from = 0.0, to = 0.0;
	Runner *runner;
	Area *area = g_context->area;

	runner = (Runner *) datum;
	if (index == 0) {
		from = 0;
	} else {
		from += to;
	}
	to = 360 * ((double) runner->votes / (double) area->vote_total);

	printf("idx(%d) from: %f\n", index, from);	
	return from;
}

static double set_man_graph_to(int index, void *datum)
{
	static double from = 0.0, to = 0.0;
	Runner *runner;
	Area *area = g_context->area;

	runner = (Runner *) datum;
	if (index == 0) {
		from = 0;
	} else {
		from += to;
	}
	to = 360 * ((double) runner->votes / (double) area->vote_total);

	printf("idx(%d) to: %f\n", index, from + to);	
	return from + to;
}

static uint32_t set_man_party_color(int index, void *datum)
{
	Runner *runner;
	runner = (Runner *) datum;
	return RGBA2ARGB(runner->party->color);
}

static double set_man_xpos(int index, void *datum)
{
	return g_context->runners[index].endx;
}

static double set_man_ypos(int index, void *datum)
{
	return g_context->runners[index].endy;
}

static char* set_man_photo_uri(int index, void *datum)
{
	Runner *runner;
	runner = (Runner *) datum;
	return runner->uri;
}

static uint32_t set_man_photo_delay(int index, void *datum)
{
	// TODO how to get startdelay of prior mapinfo mode
	return 3000 + 500 * index;
}

static double set_man_name_xpos(int index, void *datum)
{
	double tx, gap = g_context->props->RUNNER_PHOTO_W;

	if (index == 0) {
		tx = g_context->runners[index].endx - gap;
	} else {
		tx = g_context->runners[index].endx + gap;
	}
	return tx;
}

static double set_man_name_ypos(int index, void *datum)
{
	double ty;
	ty = g_context->runners[index].endy + g_context->props->RUNNER_PHOTO_W / 2;

	return ty;
}


static void set_man_attr(struct nemodavi *davi, int count)
{
	int i;
	double size, w, h, r;
	struct ui_props *pr;
	struct showone *one;
	struct nemodavi_selector *sel;

	pr = g_context->props;

	sel = nemodavi_selector_get(davi, "graph");
	size = pr->RUNNER_DONUT_W;
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->RUNNER_DONUT_INNER);
	nemodavi_set_dattr(sel, "tx", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ty", pr->SCENE_W / 2);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr_handler(sel, "fill", set_man_party_color);

	sel = nemodavi_selector_get(davi, "photo");
	size = pr->RUNNER_PHOTO_W;
	w = h = pr->RUNNER_PHOTO_W;
	r = pr->RUNNER_PHOTO_W / 2;
	one = mapinfo_util_create_clip(g_context->canvas, w, h, r);
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_dattr(sel, "alpha", 0.0f);
	nemodavi_set_oattr(sel, "clip", one);

	sel = nemodavi_selector_get(davi, "name");
	nemodavi_set_oattr(sel, "font", g_context->font);
	nemodavi_set_dattr(sel, "font-size", pr->RUNNER_NAME_SIZE);
	nemodavi_set_dattr(sel, "alpha", 0.0f);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 1.0f);
	nemodavi_set_cattr(sel, "fill", 0xff1edcdc);
	nemodavi_set_sattr_handler(sel, "text", set_man_name);
	
	sel = nemodavi_selector_get(davi, "vote");
	nemodavi_set_oattr(sel, "font", g_context->font);
	nemodavi_set_dattr(sel, "font-size", pr->RUNNER_VOTE_SIZE);
	nemodavi_set_dattr(sel, "alpha", 0.0f);
	nemodavi_set_dattr(sel, "ax", 0.8f);
	nemodavi_set_dattr(sel, "ay", 1.0f);
	nemodavi_set_cattr(sel, "fill", 0xff1edcdc);
	nemodavi_set_dattr_handler(sel, "ro", set_man_vote_ro);
	nemodavi_set_sattr_handler(sel, "text", set_man_vote_text);

	sel = nemodavi_selector_get(davi, "d0");
	size = (double) pr->AREA_DONUT_W * 1.1 / 2;
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->AREA_DONUT_INNER);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	
	sel = nemodavi_selector_get(davi, "d1");
	size = (double) pr->AREA_DONUT_W * 0.8;
	nemodavi_set_dattr(sel, "width", size);
	nemodavi_set_dattr(sel, "height", size);
	nemodavi_set_dattr(sel, "inner", pr->AREA_DONUT_INNER);
	nemodavi_set_dattr(sel, "ax", 0.5f);
	nemodavi_set_dattr(sel, "ay", 0.5f);
	nemodavi_set_oattr(sel, "filter", g_context->blur);

	sel = nemodavi_selector_get(davi, "outpath");
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "stroke", 0xff1edcdc);
	nemodavi_set_dattr(sel, "stroke-width", 2.0f);
	nemodavi_set_dattr(sel, "to", 0.0f);

	sel = nemodavi_selector_get(davi, "inpath");
	nemodavi_set_dattr(sel, "to", 0.0f);
}

static int prepare_runner_mode(Area *areainfo)
{
	int i, count, runner_count;
	char buff[32];
	struct nemodavi *dv;
	struct ui_props *pr;
	List *l, *runner_list; 
	Runner *runner;
	struct showone *one;
	struct nemodavi_data *data;
	Runner **runners;

	if (!areainfo) {
		return -1;
	}
	g_context->area = areainfo;

	pr = g_context->props;
	runner_list = area_get_runners(g_context->area);
	count = list_count(runner_list);
	g_context->runners = (struct man_ui *) malloc(sizeof(struct man_ui) * count); 
	memset(g_context->runners, 0x0, sizeof(struct man_ui) * count);

	// for area
	g_context->areadv = dv = nemodavi_create(g_context->canvas, NULL);

	// TODO why dummy item need here?
	/*
	one = nemoshow_item_create(NEMOSHOW_DONUT_ITEM);
	nemoshow_attach_one(dv->canvas->show, one);
	nemoshow_one_attach(dv->canvas, one);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, 250, 250);
	*/

	nemodavi_append_selector(dv, "outbound", NEMOSHOW_DONUT_ITEM); 
	nemodavi_append_selector(dv, "inbound", NEMOSHOW_DONUT_ITEM); 
	nemodavi_append_selector(dv, "center", NEMOSHOW_DONUT_ITEM); 
	nemodavi_append_selector(dv, "name", NEMOSHOW_TEXT_ITEM); 

	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		sprintf(buff, "sub%d", i);
		nemodavi_append_selector(dv, buff, NEMOSHOW_DONUT_ITEM); 
	}

	set_area_attr(dv);

	// for runners
	data = nemodavi_data_create();

	runner_list = area_get_runners(areainfo);
	runner_count = list_count(runner_list);

	runners = (Runner **) malloc(sizeof(Runner *) * runner_count);

	i = 0;
	LIST_FOR_EACH(runner_list, l, runner) {
		runners[i] = runner;
		i++;
	}
	nemodavi_data_bind_ptr_array(data, (void **) runners, runner_count);
	g_context->mandv = dv = nemodavi_create(g_context->canvas, data);

	
	nemodavi_append_selector(dv, "graph", NEMOSHOW_DONUT_ITEM);
	nemodavi_append_selector(dv, "photo", NEMOSHOW_IMAGE_ITEM);
	nemodavi_append_selector(dv, "name", NEMOSHOW_TEXT_ITEM);
	nemodavi_append_selector(dv, "vote", NEMOSHOW_TEXT_ITEM);
	nemodavi_append_selector(dv, "d0", NEMOSHOW_DONUT_ITEM);
	nemodavi_append_selector(dv, "d1", NEMOSHOW_DONUT_ITEM);
	nemodavi_append_selector_by_handler(dv, "outpath", set_outpath_path);
	nemodavi_append_selector_by_handler(dv, "inpath", set_inpath_path);

	set_man_attr(dv, runner_count);

	return 0;
}

static uint32_t enter_runner_mode(uint32_t startdelay)
{
	int i, count;
	char buff[32];
	double size, delay, duration, r, from, to;
	struct ui_props *pr;
	struct nemodavi *dv;
	struct nemodavi_transition *trans;
	struct nemodavi_selector *sel;
	List *l, *runners; 
	Runner *runner;
	Area *area;

	pr = g_context->props;
	area = g_context->area;
	runners = area_get_runners(area);
	count = list_count(runners);
	delay = startdelay;

	dv = g_context->areadv;
	trans = nemodavi_transition_create(g_context->areadv);
	size = pr->RUNNER_DONUT_W - 3 * pr->OUT_BOUND_DONUT_INNER;
	duration = 300;

	sel = nemodavi_selector_get(dv, "inbound");
	nemodavi_transition_set_dattr(trans, sel, "width", size);
	nemodavi_transition_set_dattr(trans, sel, "height", size);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay += duration; 

	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "outbound");
	nemodavi_transition_set_dattr(trans, sel, "to", 360);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay += duration - 500;
	nemodavi_transition_run(g_context->areadv);

	dv = g_context->mandv;
	trans = nemodavi_transition_create(dv);
	duration = 1500;
	sel = nemodavi_selector_get(dv, "graph");
	nemodavi_transition_set_dattr_handler(trans, sel, "from", set_man_graph_from);
	nemodavi_transition_set_dattr_handler(trans, sel, "to", set_man_graph_to);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay += duration - 500;

	trans = nemodavi_transition_create(dv);
	duration = 2000;
	sel = nemodavi_selector_get(dv, "outpath");
	nemodavi_transition_set_dattr(trans, sel, "to", 1.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay = 1000 + startdelay;
	nemodavi_transition_run(g_context->mandv);


	dv = g_context->areadv;
	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "center");
	nemodavi_transition_set_dattr(trans, sel, "to", 360.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay = 1500 + startdelay;

	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "name");
	nemodavi_set_sattr(sel, "text", area->name);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay = 1500 + startdelay;

	from = 0.0f;
	duration = 1000;
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		to = (360 / pr->AREA_SUB_DONUT_NUM) - pr->AREA_SUB_DONUT_GAP;
		trans = nemodavi_transition_create(dv);
		sprintf(buff, "sub%d", i);
		printf("sub from:%f, to:%f\n", from, to);
		sel = nemodavi_selector_get(dv, buff);
		nemodavi_transition_set_dattr(trans, sel, "from", from);
		nemodavi_transition_set_dattr(trans, sel, "to", from + to);
		nemodavi_transition_set_delay(trans, delay);
		nemodavi_transition_set_duration(trans, duration);
		nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

		from = from + to + pr->AREA_SUB_DONUT_GAP;
	}
	delay = 2000 + startdelay;
	nemodavi_transition_run(g_context->areadv);

	dv = g_context->mandv;
	trans = nemodavi_transition_create(dv);
	duration = 1500;
	sel = nemodavi_selector_get(dv, "inpath");
	nemodavi_transition_set_dattr(trans, sel, "to", 1.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay = 2500 + startdelay;

	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "d0");
	nemodavi_set_cattr_handler(sel, "fill", set_man_party_color);
	nemodavi_set_dattr_handler(sel, "tx", set_man_xpos);
	nemodavi_set_dattr_handler(sel, "ty", set_man_ypos);
	nemodavi_transition_set_dattr(trans, sel, "to", 360.0f);
	sel = nemodavi_selector_get(dv, "d1");
	nemodavi_set_dattr_handler(sel, "tx", set_man_xpos);
	nemodavi_set_dattr_handler(sel, "ty", set_man_ypos);
	nemodavi_transition_set_dattr(trans, sel, "to", 360.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);


	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "photo");
	nemodavi_set_sattr_handler(sel, "image-uri", set_man_photo_uri);
	nemodavi_set_dattr_handler(sel, "tx", set_man_xpos);
	nemodavi_set_dattr_handler(sel, "ty", set_man_ypos);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);
	nemodavi_transition_set_delay_handler(trans, set_man_photo_delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	delay = 3500 + startdelay;

	trans = nemodavi_transition_create(dv);
	duration = 1000;
	sel = nemodavi_selector_get(dv, "name");
	nemodavi_set_dattr_handler(sel, "tx", set_man_name_xpos);
	nemodavi_set_dattr_handler(sel, "ty", set_man_name_ypos);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);

	sel = nemodavi_selector_get(dv, "vote");
	nemodavi_set_dattr_handler(sel, "tx", set_man_vote_xpos);
	nemodavi_set_dattr_handler(sel, "ty", set_man_vote_ypos);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);

	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_run(g_context->mandv);

	delay = 3000 + startdelay;
	dv = g_context->areadv;
	trans = nemodavi_transition_create(dv);
	duration = 10000;
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		sprintf(buff, "sub%d", i);
		sel = nemodavi_selector_get(dv, buff);
		nemodavi_transition_set_fix_dattr(trans, sel, "ro", 360.0f, 0);
	}
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_LINEAR_TYPE);
	nemodavi_transition_run(g_context->areadv);

	return delay + 4500;
}

static uint32_t exit_runner_mode(uint32_t delay)
{
	int i;
	double size;
	char buff[32];
	struct nemodavi *dv;
	struct nemodavi_transition *trans;
	struct nemodavi_selector *sel;
	struct ui_props *pr;

	pr = g_context->props;

	dv = g_context->mandv;
	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "vote");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "name");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "photo");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "d1");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 1000);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "d0");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 1500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "inpath");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 2000);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	dv = g_context->areadv;
	trans = nemodavi_transition_create(dv);
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		sprintf(buff, "sub%d", i);
		sel = nemodavi_selector_get(dv, buff);
		nemodavi_transition_set_dattr(trans, sel, "from", 0.0f);
		nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
		nemodavi_transition_set_dattr(trans, sel, "ro", 0.0f);
	}
	nemodavi_transition_set_delay(trans, delay + 2500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "name");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	sel = nemodavi_selector_get(dv, "center");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 2500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	dv = g_context->mandv;
	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "outpath");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 1000);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	sel = nemodavi_selector_get(dv, "graph");
	nemodavi_transition_set_dattr(trans, sel, "from", 0.0f);
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 1500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	dv = g_context->areadv;
	trans = nemodavi_transition_create(dv);
	size = pr->RUNNER_DONUT_W - 7 * pr->OUT_BOUND_DONUT_INNER;
	sel = nemodavi_selector_get(dv, "inbound");
	nemodavi_transition_set_dattr(trans, sel, "width", size);
	nemodavi_transition_set_dattr(trans, sel, "height", size);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	sel = nemodavi_selector_get(dv, "outbound");
	nemodavi_transition_set_dattr(trans, sel, "to", 0.0f);
	nemodavi_transition_set_delay(trans, delay + 2500);
	nemodavi_transition_set_duration(trans, 1000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	nemodavi_transition_run(g_context->mandv);
	nemodavi_transition_run(g_context->areadv);
}

static uint32_t update_runner_mode(Area *areainfo)
{
	/*
	int i, count;
	char buff[32];
	double size, delay, duration, r, from, to;
	struct ui_props *pr;
	struct nemodavi *dv;
	struct nemodavi_transition *trans;
	struct nemodavi_selector *sel;
	List *runners; 
	Runner *runner;
	Area *area;

	pr = g_context->props;
	area = g_context->area = areainfo;
	runners = area_get_runners(area);
	count = list_count(runners);
	delay = 0.0;

	dv = g_context->mandv;
	trans = nemodavi_transition_create(dv);
	duration = 1500;
	sel = nemodavi_selector_get(dv, "graph");
	nemodavi_transition_set_dattr_handler(trans, sel, "from", set_man_graph_from);
	nemodavi_transition_set_dattr_handler(trans, sel, "to", set_man_graph_to);
	sel = nemodavi_selector_get(dv, "vote");
	nemodavi_set_sattr_handler(sel, "text", set_man_vote_text);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 1.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	duration = 1500;
	delay = 2000;
	sel = nemodavi_selector_get(dv, "vote");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	trans = nemodavi_transition_create(dv);
	duration = 2000;
	delay = 0;
	sel = nemodavi_selector_get(dv, "outpath");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0f);
	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_run(g_context->mandv);

	trans = nemodavi_transition_create(dv);
	duration = 500;
	delay = 500;
	sel = nemodavi_selector_get(dv, "outpath");
	nemodavi_selector_destory(sel);

	//nemodavi_append_selector();
	nemodavi_append_selector_by_handler(dv, "outpath", set_outpath_path);
	sel = nemodavi_selector_get(dv, "outpath");
	nemodavi_set_oattr(sel, "filter", g_context->blur);
	nemodavi_set_cattr(sel, "stroke", 0xff1edcdc);
	nemodavi_set_dattr(sel, "stroke-width", 2.0f);
	nemodavi_set_dattr(sel, "to", 0.0f);
	nemodavi_transition_set_dattr(trans, sel, "to", 1.0f);

	nemodavi_transition_set_delay(trans, delay);
	nemodavi_transition_set_duration(trans, duration);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);

	nemodavi_transition_run(g_context->mandv);
	
	return 1500 + delay;
	*/
}

int mapinfo_mode_runner_init(struct mapinfo_mode *mode)
{
	struct nemoshow *show;

	if (!mode) {
		return -1;
	}

	mode->prepare = prepare_runner_mode;
	mode->enter = enter_runner_mode;
	mode->exit = exit_runner_mode;
	mode->update = update_runner_mode;
	
	g_context = (struct context *) malloc(sizeof(struct context));
	g_context->props = init_props(mode->width, mode->height);
	g_context->width = mode->width;
	g_context->height = mode->height;
	g_context->canvas = mode->canvas;

	// commoly used font and blur items are created
	show = NEMOSHOW_CANVAS_AT(mode->canvas, show);
	g_context->font = FONT_CREATE(show, "NanumGothic", "bold");
	g_context->blur = BLUR_CREATE(show, "solid", 5);

	return 0;
}
