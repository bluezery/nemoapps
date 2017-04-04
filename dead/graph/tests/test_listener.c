#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemotool.h>
#include <nemotale.h>

// davi headers
#include <nemoshow.h>
#include <nemoease.h>
#include <showone.h>
#include <showhelper.h>

#include <nemodavi.h>
#include <nemodavi_attr.h>
#include <nemodavi_data.h>
#include <nemodavi_selector.h>
#include <nemodavi_transition.h>

#define SCENE_W		1000
#define SCENE_H		1000
#define WIN_W		500
#define WIN_H		500

struct context { 
	struct nemoshow *show;
	struct nemotool *tool;
	struct showone *canvas;
};

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
	char buffer[32];
	char *d = (char *) data;
	sprintf(buffer, "%dth (%s)", index, d);

	printf("%s: %d\n", __FUNCTION__, index);
	return strdup(buffer);
}

static void graph_notify(int index, void *data) 
{
	printf("%s: %d(idx), %s(value)\n", __FUNCTION__, index, (char *) data);
}

static void graph_index_notify(int index, void *data)
{
	printf("%s: %d\n", __FUNCTION__, index);
}

static void graph_range_notify(int index, void *data)
{
	printf("%s: %d\n", __FUNCTION__, index);
}

static void desc_notify(int index, void *data)
{
	printf("%s: %d\n", __FUNCTION__, index);
}

static void outline_notify(int index, void *data)
{
	printf("%s: %d\n", __FUNCTION__, index);
}

static double set_alpha_test_handler(int index, void *data, void *userdata)
{

	printf("%s: %d(idx), %d(value)\n", __FUNCTION__, index, * (uint32_t *) data);
	return index * 20 / 100;
}

static int compare_data (void *a, void *b)
{
	int da, db;

	da = * (int *) a;
	db = * (int *) b;

	printf("%d - %d = %d\n", da, db, da - db);

	return da - db;
}

void graph_click_listener(int index, void *datum)
{
  printf("click: %s\n", (char *) datum);	
}

static void display_chart(struct showone* canvas, struct context *ctx)
{
	char *mydata[5] = { "nemo", "ux", "team", "nice", "winner" };
	char *mydata2[5] = { "this", "is", "changed", "data", "array" };
	int mydata3[5] = { 100, 1000, 170, 120, 400 };

	struct nemodavi *davi, *ui, *test;
	struct nemodavi_data *data;
	struct showone *font, *one;

	struct nemodavi_selector *graph, *desc, *outline, *sort;

	if (!canvas) {
		return;
	}

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) mydata, 5, sizeof(char *));
	
	// create davi object
	davi = nemodavi_create(canvas, data);

	nemoshow_set_userdata(davi->canvas->show, davi);

	// append new selectors
	nemodavi_append_selector(davi, "graph", NEMOSHOW_ARC_ITEM);
	nemodavi_append_selector(davi, "desc", NEMOSHOW_TEXT_ITEM);

	// get selectors
	graph = nemodavi_selector_get(davi, "graph");
	desc = nemodavi_selector_get(davi, "desc");

	nemodavi_selector_attach_event_listener(graph, NEMODAVI_EVENT_CLICK, graph_click_listener);

	// font creation
	font = nemoshow_font_create();
	nemoshow_attach_one(canvas->show, font);
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
	nemodavi_set_sattr_handler(desc, "text", set_desc_text_handler, NULL);
	nemodavi_set_dattr_handler(desc, "tx", set_graph_tx_handler, NULL);
	nemodavi_set_dattr_handler(desc, "ty", set_graph_ty_handler, NULL);

	// test for data listener
	printf("--------------- start listeners test ------------------\n");
	printf("--------------- (graph, desc) new listener attach ------------------\n");
	nemodavi_selector_attach_data_listener(graph, graph_notify);
	nemodavi_selector_attach_data_listener(desc, desc_notify);
	// change already bound data
	nemodavi_data_bind_ptr_array(data, (void **) mydata2, 5, sizeof(char *));

	printf("--------------- (graph) changed listener attach ------------------\n");
	nemodavi_selector_attach_data_listener_by_index(graph, graph_index_notify, 0);
	nemodavi_selector_attach_data_listener_by_range(graph, graph_range_notify, 2, 4);
	// change already bound data
	nemodavi_data_bind_ptr_array(data, (void **) mydata, 5, sizeof(char *));
	nemodavi_data_bind_ptr_one(data, (void *) "hello, world", 4);

	printf("--------------- (graph, desc) listeners detach ------------------\n");
	nemodavi_selector_detach_data_listener(graph);
	nemodavi_selector_detach_data_listener(desc);
	// change already bound data
	nemodavi_data_bind_ptr_array(data, (void **) mydata2, 5, sizeof(char *));

	printf("--------------- end listeners test ------------------\n");

	printf("--------------- start test of davi object without data ------------------\n");
	// davi test without data
	ui = nemodavi_create(canvas, NULL);
	nemodavi_append_selector(ui, "outline", NEMOSHOW_ARC_ITEM);
	outline = nemodavi_selector_get(ui, "outline");

	nemodavi_set_dattr(outline, "width", SCENE_W / 4);
	nemodavi_set_dattr(outline, "height", SCENE_H / 4);
	nemodavi_set_cattr(outline, "fill", 0xff1edcdc);
	nemodavi_set_dattr(outline, "inner", 20.0f);
	nemodavi_set_dattr(outline, "from", 0);
	nemodavi_set_dattr(outline, "to", 360.0f);

	nemodavi_selector_attach_data_listener(outline, outline_notify);
	nemodavi_emit_data_change_signal(ui);
	printf("--------------- end test of davi object without data ------------------\n");

	printf("--------------- start sort test ------------------\n");
	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_int_array(data, mydata3, 5);
	
	// create davi object
	test = nemodavi_create(canvas, data);
	nemodavi_append_selector(test, "graph", NEMOSHOW_ARC_ITEM);
	sort = nemodavi_selector_get(test, "graph");
	nemodavi_selector_sort(sort, compare_data);
	nemodavi_set_dattr_handler(sort, "alpha", set_alpha_test_handler, NULL);
	printf("--------------- end sort test ------------------\n");
}

static void display_chart_transition(struct nemodavi *davi)
{
	struct nemodavi_transition *trans;
	struct nemodavi_selector *graph, *desc;

	graph = nemodavi_selector_get(davi, "graph");
	desc = nemodavi_selector_get(davi, "desc");

  srand(time(NULL));

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_CUBIC_INOUT_TYPE);
	nemodavi_transition_set_dattr(trans, graph, "inner", rand() % 30);
	nemodavi_transition_set_dattr(trans, graph, "width", rand() % 50 + 50);
	nemodavi_transition_set_dattr(trans, graph, "height", rand() % 50 + 50);
	nemodavi_transition_set_cattr(trans, graph, "fill", 0xffffffff);
	nemodavi_transition_run(davi);

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_delay(trans, 2000);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_LINEAR_TYPE);
	nemodavi_transition_set_fix_dattr(trans, graph, "ro", 360, 0);
	nemodavi_transition_run(davi);
}

static uint32_t set_transition_delay(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	return index * 1000;
}

static uint32_t set_transition_duration(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	return index * 1000;
}

static uint32_t set_transition_ease(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	return index;
}

static double set_transition_inner(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	return index * 10 + 30;
}

static double set_transition_size(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	return index * 20 + 30;
}

static uint32_t set_transition_fill(int index, void *datum)
{
	char *d = (char *) datum;
	printf("(%s) %dth: %s\n", __FUNCTION__, index, datum);
	uint32_t colors[5] = {
	 	0xffffffff,
	 	0xfffbfbcc,
	 	0xffababff,
	 	0xff0a0a01,
	 	0xff0bff00
 	}; 
	return colors[index];
}

static void display_chart_transition_with_handler(struct nemodavi *davi)
{
	struct nemodavi_transition *trans;
	struct nemodavi_selector *graph, *desc;

	trans= nemodavi_transition_create(davi);

	graph = nemodavi_selector_get(davi, "graph");
	desc = nemodavi_selector_get(davi, "desc");

	nemodavi_transition_set_delay_handler(trans, set_transition_delay, NULL);
	nemodavi_transition_set_duration_handler(trans, set_transition_duration, NULL);
	nemodavi_transition_set_ease_handler(trans, set_transition_ease, NULL);
	nemodavi_transition_set_dattr_handler(trans, graph, "inner", set_transition_inner, NULL);
	nemodavi_transition_set_dattr_handler(trans, graph, "width", set_transition_size, NULL);
	nemodavi_transition_set_dattr_handler(trans, graph, "height", set_transition_size, NULL);
	nemodavi_transition_set_cattr_handler(trans, graph, "fill", set_transition_fill, NULL);

	trans = nemodavi_transition_create(davi);
	nemodavi_transition_set_delay(trans, 5000);
	nemodavi_transition_set_duration(trans, 2000);
	nemodavi_transition_set_ease(trans, NEMOEASE_LINEAR_TYPE);
	nemodavi_transition_set_fix_dattr(trans, graph, "ro", 360, 0);

	nemodavi_transition_run(davi);
}

static void dispatch_show_event(struct nemoshow *show, void *event)
{
	// TODO this is the variable just for test
	uint32_t i, serial0, serial1;
	struct nemodavi* davi = (struct nemodavi *)nemoshow_get_userdata(show);

	nemoshow_event_update_taps(show, NULL, event);
	if (nemoshow_event_is_down(show, event)) {
		printf("touch down\n");	

		if (nemoshow_event_is_single_tap(show, event)) {
			printf("one down\n");
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			printf("many down on canvas\n");
			nemoshow_event_get_distant_tapserials(show, event, &serial0, &serial1);
			nemoshow_view_pick(show, serial0, serial1, 
					(1 << NEMOSHOW_VIEW_PICK_ROTATE_TYPE) | (1 << NEMOSHOW_VIEW_PICK_SCALE_TYPE) | (1 << NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE));
		}
		return;
	} 

	if (nemoshow_event_is_single_click(show, event)) {
		uint32_t tag = nemoshow_canvas_pick_tag(davi->canvas, 
				nemoshow_event_get_x_on(event, 0),
				nemoshow_event_get_y_on(event, 0));

		printf("%s: %x\n", __FUNCTION__, tag);
		nemodavi_emit_event(NEMODAVI_EVENT_CLICK, tag);
		display_chart_transition(davi);
		//display_chart_transition_with_handler(davi);
	}
}


static struct context* init_app() {
	struct context *ctx;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *back_canvas;
	struct showone *front_canvas;

	ctx = (struct context *)malloc(sizeof(struct context));
	memset(ctx, 0, sizeof(struct context));

	tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);

	show = nemoshow_create_view(tool, SCENE_W, SCENE_H);
	nemoshow_view_set_input(show, "touch");
	nemoshow_set_dispatch_event(show, dispatch_show_event);

	scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, SCENE_W);
	nemoshow_scene_set_height(scene, SCENE_H);
	nemoshow_attach_one(show, scene);

	back_canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(back_canvas, SCENE_W);
	nemoshow_canvas_set_height(back_canvas, SCENE_H);
	nemoshow_canvas_set_type(back_canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(back_canvas, 0xff, 0xff, 0xff, 0x00);
	nemoshow_canvas_set_alpha(back_canvas, 0x00);
	nemoshow_attach_one(show, back_canvas);
	nemoshow_one_attach(scene, back_canvas);

	front_canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(front_canvas, SCENE_W);
	nemoshow_canvas_set_height(front_canvas, SCENE_H);
	nemoshow_canvas_set_type(front_canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	//nemoshow_canvas_set_event(front_canvas, 1);
	nemoshow_attach_one(show, front_canvas);
	nemoshow_one_attach(scene, front_canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, WIN_W, WIN_H);

	ctx->tool = tool;
	ctx->show = show;
	ctx->canvas = front_canvas;

	//nemoshow_set_userdata(show, ctx);
	return ctx;
}

static void deinit_app(struct context* ctx) {
	nemoshow_destroy_view(ctx->show);
	nemotool_disconnect_wayland(ctx->tool);
	nemotool_destroy(ctx->tool);
	free(ctx);
}

int main () {

	struct context *ctx;
	struct showone *one;

	ctx = init_app();

	display_chart(ctx->canvas, ctx);

	nemoshow_dispatch_frame(ctx->show);
	nemotool_run(ctx->tool);

	deinit_app(ctx);

	return 0;
}
