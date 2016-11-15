#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <nemoshow.h>

#include "color.h"
#include "nemowrapper.h"
#include "mapinfo.h"
#include "mapinfo_util.h"

#define PI	3.14159265

struct ui_props {
	double SCENE_W;
	double SCENE_H;

	double AREA_SUB_DONUT_NUM;

	double RUNNER_DONUT_INNER;
	double AREA_DONUT_INNER;
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
	double RUNNER_ARROW_LEN;
	double RUNNER_SUB_DONUT_NUM;

	double OUT_BOUND_DONUT_INNER;
	double IN_BOUND_DONUT_INNER;
};

struct man_ui {
	Runner *raw;
	struct showone *donut; // donut type
	struct showone *name;
	struct showone *photo;
	struct showone *vote;
	struct showone *d0;
	struct showone *d1;
	struct showone *d2;
	int endx;
	int endy;
};

struct area_ui {
	Area *raw;
	List *pparties;
	int pparties_count;

	struct showone *donut; // donut type
	struct showone *name; // name

	// paths
	struct showone *center;
	struct showone **subs;
};

static struct context {
	struct showone *canvas;
	struct showone *blur;
	struct showone *font;
	struct area_ui *area;
	struct man_ui *runners;
	struct ui_props *props;
	int width;
	int height;
	int finished;
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
	props->SCENE_W = scene_w;
	props->SCENE_H = scene_h;

	props->AREA_SUB_DONUT_NUM = 3;

	props->RUNNER_DONUT_INNER = scene_w * 0.017;
	props->AREA_DONUT_INNER = scene_w * 0.005f;
	props->AREA_SUB_DONUT_GAP = 30.0f; // angle

	props->RUNNER_DONUT_W = scene_w * 0.9f;
	props->RUNNER_DONUT_H = props->RUNNER_DONUT_W;
	props->RUNNER_SUB_DONUT_W = scene_w * 0.12f;
	props->RUNNER_SUB_DONUT_H = props->RUNNER_SUB_DONUT_W;
	props->RUNNER_SUB_DONUT_NUM = 2;
	props->AREA_DONUT_W = scene_w * 0.2f;
	props->AREA_DONUT_H = props->AREA_DONUT_W;

	props->RUNNER_PHOTO_W = scene_w * 0.1f;
	props->RUNNER_PHOTO_H = props->RUNNER_PHOTO_W;
	props->RUNNER_VOTE_SIZE = scene_w * 0.02f;
	props->RUNNER_NAME_SIZE = scene_w * 0.03f;
	props->RUNNER_ARROW_LEN = scene_w * 0.12f;
	props->OUT_BOUND_DONUT_INNER = scene_w * 0.025f;

	return props;
}

static int set_runner_center(struct context* context, struct man_ui* runner, double angle, double radius)
{
	double scene_w, scene_h, arrow_len, radian;

	if (!context || !runner) {
		return -1;
	}

	scene_w = context->props->SCENE_W;
	scene_h = context->props->SCENE_H;
	arrow_len = context->props->RUNNER_ARROW_LEN * 2;
	radian = (double) (angle * (PI / 180));

	runner->endx = scene_w / 2.0f + (radius + arrow_len) * cos(radian);
	runner->endy = scene_h / 2.0f + (radius + arrow_len) * sin(radian);

	return 0;
}

static uint32_t prepare_party_mode(Area *areainfo)
{
	struct showone *canvas; 
	struct area_ui *area; 
	struct man_ui *runners; 
	struct showone *one, *blur, *font;
	struct ui_props *pr;

	char buffer[32];
	double x, y, w, h, r, from, to, runner_angle;
	uint32_t i, j, runner_count, diameter;

	pr = g_context->props;

	g_context->area = (struct area_ui *) malloc(sizeof(struct area_ui)); 
	memset(g_context->area, 0x0, sizeof(struct area_ui));

	g_context->area->subs = (struct showone *) malloc(sizeof(struct showone*) * pr->AREA_SUB_DONUT_NUM);
	memset(g_context->area->subs, 0x0, sizeof(struct showone*) * pr->AREA_SUB_DONUT_NUM);

	g_context->runners = (struct man_ui *) malloc(sizeof(struct man_ui) * areainfo->runners2_cnt); 
	memset(g_context->runners, 0x0, sizeof(struct man_ui) * areainfo->runners2_cnt);

	// TODO should be changed to real party count of this area
	/*
	for (i = 0; i < 3; i++) {
		g_context->runners[i].subs = (struct showone *) malloc(sizeof(struct showone *) * pr->RUNNER_SUB_DONUT_NUM);
		if (!g_context->runners[i].subs) {
			printf("fail to get runner subs\n");
			return -1;
		}
		memset(g_context->runners[i].subs, 0x0, sizeof(struct showone *) * pr->RUNNER_SUB_DONUT_NUM);
	}
	*/

	g_context->runners = (struct man_ui *) malloc(sizeof(struct man_ui) * areainfo->runners2_cnt); 
	memset(g_context->runners, 0x0, sizeof(struct man_ui) * areainfo->runners2_cnt);

	// set properties
	g_context->area->raw = areainfo;

	runner_count = g_context->area->raw->runners2_cnt;
	area = g_context->area;
	runners = g_context->runners;
	blur = g_context->blur;
	font = g_context->font;
	canvas = g_context->canvas;
	
	area->name = one = TEXT_CREATE(canvas, font, pr->RUNNER_NAME_SIZE * 2, "DANG BYUL");
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
#ifdef MAPINFO_TEST
	nemoshow_item_set_alpha(one, 1.0f);
#else
	nemoshow_item_set_alpha(one, 0.0f);
#endif
	nemoshow_item_translate(one, pr->SCENE_W / 2, (pr->SCENE_H / 4) * 3);
	nemoshow_item_set_anchor(one, 0.5f, 0.5f);

	from = 0.0;
	diameter = pr->RUNNER_DONUT_W - 7 * pr->OUT_BOUND_DONUT_INNER;
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		//to = (360 / pr->AREA_SUB_DONUT_NUM) - pr->AREA_SUB_DONUT_GAP;
		from = mapinfo_util_get_runner_start_angle(3, i);
		to = from + (360 / pr->AREA_SUB_DONUT_NUM - pr->AREA_SUB_DONUT_GAP );
		area->subs[i] = one = DONUT_CREATE(canvas, diameter, diameter, from, to, pr->AREA_DONUT_INNER);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_filter(one, blur);
		nemoshow_item_translate(one, pr->SCENE_W / 2, pr->SCENE_H / 2);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif
	}

	// TODO should be changed to real count of party 
	diameter = pr->RUNNER_DONUT_W;
	for (i = 0; i < runner_count; i++) {
		from = mapinfo_util_get_runner_start_angle(runner_count, i);
		set_runner_center(g_context, &runners[i], from, pr->AREA_DONUT_W / 2.0f);
#ifdef MAPINFO_TEST
		runners[i].donut = one = DONUT_CREATE(canvas, diameter, diameter, from, from + 50, pr->RUNNER_DONUT_INNER);
#else
		runners[i].donut = one = DONUT_CREATE(canvas, diameter, diameter, from, from, pr->RUNNER_DONUT_INNER);
#endif
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 0.0f);
    nemoshow_item_set_filter(one, blur);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
		nemoshow_item_translate(one, pr->SCENE_W / 2, pr->SCENE_H / 2);
	}

	// TODO should be changed to real count of party 
	for (i = 0; i < 3; i++) {
		/*
		diameter = pr->RUNNER_SUB_DONUT_W * 0.9f;
		for (j = 0; j < pr->RUNNER_SUB_DONUT_NUM; j++) {
			to = (360 / pr->RUNNER_SUB_DONUT_NUM) - pr->AREA_SUB_DONUT_GAP;
			//printf("runners[%d].subs[%d]: 0x%x\n", i, j, &runners[i].subs[j]); 
			runners[i].subs[j] = one = DONUT_CREATE(canvas, diameter, diameter, from, from + to, pr->AREA_DONUT_INNER * 2);
			nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_stroke_width(one, 0.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_set_anchor(one, 0.5f, 0.5f);
			nemoshow_item_translate(one, runners[i].endx, runners[i].endy);
#ifdef MAPINFO_TEST
			nemoshow_item_set_alpha(one, 1.0f);
#else
			nemoshow_item_set_alpha(one, 0.0f);
#endif
			from = from + to + pr->AREA_SUB_DONUT_GAP;
		}
		*/

		// d1 - animation
		diameter = pr->RUNNER_SUB_DONUT_W * 0.9f;
		runners[i].d1 = one = DONUT_CREATE(canvas, diameter, diameter, 0, 0, pr->AREA_DONUT_INNER * 1);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 0.0f);
		nemoshow_item_set_filter(one, blur);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
		nemoshow_item_translate(one, runners[i].endx, runners[i].endy);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif

		// d2 -animation
		runners[i].d2 = one = DONUT_CREATE(canvas, diameter, diameter, 0, 0, pr->AREA_DONUT_INNER * 1);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 0.0f);
		nemoshow_item_set_filter(one, blur);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
		nemoshow_item_translate(one, runners[i].endx, runners[i].endy);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif

		// d0
		diameter = pr->RUNNER_SUB_DONUT_W;
		runners[i].d0 = one = DONUT_CREATE(canvas, diameter, diameter, 0, 360, pr->AREA_DONUT_INNER);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 0.0f);
		nemoshow_item_set_filter(one, blur);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
		nemoshow_item_translate(one, runners[i].endx, runners[i].endy);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif

		// name
		runners[i].name = one = TEXT_CREATE(canvas, font, pr->RUNNER_NAME_SIZE * 0.8, "name!");
		nemoshow_item_set_fill_color(one, 0xff, 0xff, 0xff, 0xff);
		nemoshow_item_translate(one, runners[i].endx, runners[i].endy - (diameter / 2) * 0.3);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif
		// vote
		runners[i].vote = one = TEXT_CREATE(canvas, font, pr->RUNNER_NAME_SIZE, "vote!");
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_anchor(one, 0.5f, 0.5f);
#ifdef MAPINFO_TEST
		nemoshow_item_set_alpha(one, 1.0f);
#else
		nemoshow_item_set_alpha(one, 0.0f);
#endif
		nemoshow_item_translate(one, runners[i].endx, runners[i].endy + (diameter / 2) * 0.3);

	}

	area->pparties = area_calculate_parties(NULL, areainfo->parent);
	area->pparties_count = list_count(area->pparties);
	/*
	LIST_FREE(parties, party) {
			ERR("%s, %d", party->name, party->predominate_cnt);
			ERR("%s, %d", party->name, party->cnt);
	}
	*/
}

static void enter_transition_done_callback(void *userdata) {
	struct context *context = (struct context *) userdata;
	context->finished = 1;
}

static uint32_t enter_party_mode(uint32_t delay)
{
	struct nemoshow *show;
	struct showone *frame;
	struct showone *set;
	struct showone *canvas; 
	struct showtransition *trans;

	struct area_ui *area; 
	struct man_ui *runners; 
	struct showone *one, *blur, *font;
	struct ui_props *pr;
	struct context *context = g_context;

	double from, to, tx, ty, diameter;
	double r, runner_r, ruuner_angle;
	uint32_t i, color, exetime;
  List *l;
  Party *party;

	char buffer[32];

	/*
	if (!context->finished) {
		printf("enter not finished\n");
		return -1;
	}	
	context->finished = 0;
	*/

	runners = g_context->runners;
	blur = g_context->blur;
	font = g_context->font;
	canvas = g_context->canvas;
	pr = g_context->props;
	show = NEMOSHOW_CANVAS_AT(g_context->canvas, show);
	area = g_context->area;

	// area inbound
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		frame = mapinfo_util_create_transition();
		diameter = pr->RUNNER_DONUT_W - 2 * pr->OUT_BOUND_DONUT_INNER;

		nemoshow_item_set_alpha(area->subs[i], 0.0f);
		set = nemoshow_sequence_create_set();
		nemoshow_one_attach(frame, set);
		nemoshow_sequence_set_source(set, area->subs[i]);
		nemoshow_sequence_set_dattr(set, "width", diameter, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_sequence_set_dattr(set, "height", diameter, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 300, 0 + delay, 1);
	}

	// outline donut per party
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
    nemoshow_item_set_fill_color(runners[i].donut, RGBA(party->color));
		from = NEMOSHOW_ITEM_AT(runners[i].donut, from);
		to = (360 / area->pparties_count ) * ((double) party->predominate_cnt / (double) party->cnt);

		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].donut);
		//nemoshow_sequence_set_dattr(set, "from", from, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_sequence_set_dattr(set, "to", from + to, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);
		from = from + to;
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1000 + delay, 1);

	// title
	frame = mapinfo_util_create_transition();
	set = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set, area->name);
	nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1000 + delay, 1);

	// circle per runner
	frame = mapinfo_util_create_transition();
	for (i = 0; i < area->pparties_count; i++) {
		// set donut
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d0);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1500 + delay, 1);


	// d1, d2 
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
    nemoshow_item_set_fill_color(runners[i].d1, RGBA(party->color));
		from = 0.0;
		to = (360 / pr->RUNNER_SUB_DONUT_NUM) - pr->AREA_SUB_DONUT_GAP;
		// set donut
		nemoshow_item_set_from(runners[i].d1, from);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d1);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set, "to", from + to, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);

		from = from + to + pr->AREA_SUB_DONUT_GAP;
		nemoshow_item_set_from(runners[i].d2, from);
    nemoshow_item_set_fill_color(runners[i].d2, RGBA(party->color));
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d2);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set, "to", from + to, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 2000 + delay, 1);


	// name and value per runner
	i = 0;
	LIST_FOR_EACH(area->pparties, l, party) {
		frame = mapinfo_util_create_transition();
		nemoshow_item_set_text(runners[i].name, party->name);
    nemoshow_item_set_fill_color(runners[i].name, RGBA(party->color));
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].name);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);

		sprintf(buffer, "%ld", ((uint32_t) party->predominate_cnt));
		nemoshow_item_set_text(runners[i].vote, buffer);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].vote);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);
		mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 2500 + 500 * i + delay, 1);
		i++;
	}

	// inbound animation
	int attr;
	frame = mapinfo_util_create_transition();
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		set = nemoshow_sequence_create_set();
		nemoshow_one_attach(frame, set);
		nemoshow_sequence_set_source(set, area->subs[i]);
		attr = nemoshow_sequence_set_dattr(set, "ro", 360.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_fix_dattr(set, attr, 0.0f);
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_LINEAR_TYPE, 20000, 3000 + delay, 0);

	frame = mapinfo_util_create_transition();
	for (i = 0; i < area->pparties_count; i++) {
		set = nemoshow_sequence_create_set();
		nemoshow_one_attach(frame, set);
		nemoshow_sequence_set_source(set, runners[i].d1);
		attr = nemoshow_sequence_set_dattr(set, "ro", 360.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_fix_dattr(set, attr, 0.0f);

		set = nemoshow_sequence_create_set();
		nemoshow_one_attach(frame, set);
		nemoshow_sequence_set_source(set, runners[i].d2);
		attr = nemoshow_sequence_set_dattr(set, "ro", 360.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_fix_dattr(set, attr, 0.0f);
	}
	trans = mapinfo_util_execute_transition(show, frame, NEMOEASE_LINEAR_TYPE, 10000, 3000 + delay, 0);
	nemoshow_transition_set_dispatch_done(trans, enter_transition_done_callback);
	nemoshow_transition_set_userdata(trans, g_context);

	return delay + 3000 + 1000;
}

static uint32_t exit_party_mode(uint32_t delay)
{
	struct nemoshow *show;
	struct showone *frame;
	struct showone *set;
	struct showone *canvas; 
	struct area_ui *area; 
	struct man_ui *runners; 
	struct showone *one, *blur, *font;
	struct ui_props *pr;

	double from, to, tx, ty, diameter;
	double r, runner_r, ruuner_angle;
	uint32_t i, color, exetime;
  List *l;
  Party *party;

	char buffer[32];

	printf("%s\n", __FUNCTION__);
	runners = g_context->runners;
	blur = g_context->blur;
	font = g_context->font;
	canvas = g_context->canvas;
	pr = g_context->props;
	show = NEMOSHOW_CANVAS_AT(g_context->canvas, show);
	area = g_context->area;
	

	// name and value per runner
	i = 0;
	LIST_FOR_EACH(area->pparties, l, party) {
		frame = mapinfo_util_create_transition();
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].name);
		nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);

		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].vote);
		nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);
		mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000,  delay, 1);
		i++;
	}

	// d1, d2 
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
		from = 0.0;
		to = (360 / pr->RUNNER_SUB_DONUT_NUM) - pr->AREA_SUB_DONUT_GAP;
		// set donut
		nemoshow_item_set_from(runners[i].d1, from);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d1);
		nemoshow_sequence_set_dattr(set, "to", from, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);

		from = from + to + pr->AREA_SUB_DONUT_GAP;
		nemoshow_item_set_from(runners[i].d2, from);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d2);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set, "to", from, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500 + delay, 1);

	// circle per runner
	frame = mapinfo_util_create_transition();
	for (i = 0; i < area->pparties_count; i++) {
		// set donut
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].d0);
		nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1000 + delay, 1);

	// title
	frame = mapinfo_util_create_transition();
	set = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set, area->name);
	nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1000 + delay, 1);

	// outline donut per party
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
		// set donut
		from = NEMOSHOW_ITEM_AT(runners[i].donut, from);
		to = (360 / area->pparties_count ) * ((double) party->predominate_cnt / (double) party->cnt);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].donut);
		nemoshow_sequence_set_dattr(set, "to", from, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);
		from = from + to;
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1000 + delay, 1);


	// area inbound
	frame = mapinfo_util_create_transition();
	for (i = 0; i < pr->AREA_SUB_DONUT_NUM; i++) {
		set = nemoshow_sequence_create_set();
		nemoshow_one_attach(frame, set);
		from = NEMOSHOW_ITEM_AT(area->subs[i], from);
		nemoshow_sequence_set_source(set, area->subs[i]);
		nemoshow_sequence_set_dattr(set, "to", from, NEMOSHOW_SHAPE_DIRTY);
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 1500 + delay, 1);

	return 2500 + delay; 
}

static uint32_t update_party_mode(Area * areainfo)
{
	struct nemoshow *show;
	struct showone *frame;
	struct showone *set;
	struct showone *canvas; 
	struct area_ui *area; 
	struct man_ui *runners; 
	struct showone *one, *blur, *font;
	struct ui_props *pr;
	struct showtransition *trans;
	struct context *context = g_context;

	double from, to, tx, ty, diameter;
	double r, runner_r, ruuner_angle;
	uint32_t i, color, exetime, delay = 0;

  List *l;
  Party *party;

	char buffer[32];

	/*
	if (!context->finished) {
		printf("enter not finished\n");
		return -1;
	}	
	context->finished = 0;
	*/

	runners = g_context->runners;
	canvas = g_context->canvas;
	pr = g_context->props;
	show = NEMOSHOW_CANVAS_AT(g_context->canvas, show);
	area = g_context->area;

	area->pparties = area_calculate_parties(NULL, areainfo->parent);
	area->pparties_count = list_count(area->pparties);

	printf("%s\n", __FUNCTION__);
	// outline donut per party
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
		// set donut
		from = NEMOSHOW_ITEM_AT(runners[i].donut, from);
		to = (360 / area->pparties_count) * ((double) party->predominate_cnt / (double) party->cnt);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].donut);
		nemoshow_sequence_set_dattr(set, "to", from + to, NEMOSHOW_SHAPE_DIRTY);
		nemoshow_one_attach(frame, set);
		from = from + to;
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0 + delay, 1);

	// name and value per runner
	i = 0;
	frame = mapinfo_util_create_transition();
	LIST_FOR_EACH(area->pparties, l, party) {
		sprintf(buffer, "%ld", ((uint32_t) party->predominate_cnt));
		nemoshow_item_set_text(runners[i].vote, buffer);
		set = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set, runners[i].vote);
		nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set);
		i++;
	}
	mapinfo_util_execute_transition(show, frame, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500 + delay, 1);

	return delay + 1500;
}

int mapinfo_mode_party_init(struct mapinfo_mode *mode)
{
	struct nemoshow *show;

	if (!mode) { return -1; }

	mode->prepare = prepare_party_mode; mode->enter = enter_party_mode;
	mode->exit = exit_party_mode; mode->update = update_party_mode;
	
	g_context = (struct context *) malloc(sizeof(struct context));
	g_context->props = init_props(mode->width, mode->height); g_context->width =
		mode->width; g_context->height = mode->height; g_context->canvas =
		mode->canvas;

	// commoly used font and blur items are created
	show = NEMOSHOW_CANVAS_AT(mode->canvas, show); g_context->font =
		FONT_CREATE(show, "NanumGothic", "bold"); g_context->blur =
		BLUR_CREATE(show, "solid", 5);

	g_context->finished = 1;

	return 0; }

