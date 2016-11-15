#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemotool.h>
#include <nemotale.h>

// davi headers
#include <nemoshow.h>
#include <showone.h>
#include <showhelper.h>

#include <nemodavi.h>

#define SCENE_W		1000
#define SCENE_H		1000
#define WIN_W		1000	
#define WIN_H		1000	
#define XNUM		7	

struct context { 
	struct nemoshow *show;
	struct nemotool *tool;
	struct showone *canvas;
};

struct realdata {
	uint32_t color;
	char *name;
	uint32_t value;
};

static char* value_getter(uint32_t index, void *data)
{
	char buffer[32];
	struct realdata *d = (struct realdata *) data;
	sprintf(buffer, "%d", d->value);

	return strdup(buffer);
}

static char* name_getter(uint32_t index, void *data)
{
	char buffer[32];
	struct realdata *d = (struct realdata *) data;
	sprintf(buffer, "%s", d->name);

	return strdup(buffer);
}

static char* color_getter(uint32_t index, void *data)
{
	char buffer[32];
	struct realdata *d = (struct realdata *) data;
	sprintf(buffer, "%u", d->color);

	return strdup(buffer);
}

static char* xnum_getter(uint32_t index, void *data)
{
	return "3";
}

static char* xunit_getter(uint32_t index, void *data)
{
	return "150";
}

static char* ymax_getter(uint32_t index, void *data)
{
	return "100";
}

static char* ynum_getter(uint32_t index, void *data)
{
	return "4";
}

static char* sw_getter(uint32_t index, void *data)
{
	return "3";
}

static void create_chart(struct showone* canvas, struct context *ctx)
{
	struct nemodavi *davi;
	struct nemodavi_data *data;
	struct nemodavi_layout *layout;

	struct realdata rds1[5] = {
		{ 0xFFF1C40F, "pusan", 13 },
		{ 0xFFF1C40F, "pusan", 33 },
		{ 0xFFF1C40F, "pusan", 53 },
		{ 0xFFF1C40F, "pusan", 60 },
		{ 0xFFF1C40F, "pusan", 72 }
	};

	struct realdata rds2[5] = {
		{ 0xFFC0392B, "seoul",  10 },
		{ 0xFFC0392B, "seoul",  25 },
		{ 0xFFC0392B, "seoul",  35 },
		{ 0xFFC0392B, "seoul",  40 },
		{ 0xFFC0392B, "seoul",  60 }
	};

	struct realdata rds3[5] = {
		{ 0xFF27AE60, "daegu", 5 },
		{ 0xFF27AE60, "daegu", 15 },
		{ 0xFF27AE60, "daegu", 20 },
		{ 0xFF27AE60, "daegu", 31 },
		{ 0xFF27AE60, "daegu", 40 }
	};

	struct realdata rds4[5] = {
		{ 0xFF2980B9, "gwangju", 2 },
		{ 0xFF2980B9, "gwangju", 10 },
		{ 0xFF2980B9, "gwangju", 14 },
		{ 0xFF2980B9, "gwangju", 20 },
		{ 0xFF2980B9, "gwangju", 30 }
	};

	struct realdata rds5[5] = {
		{ 0xFF34495E, "daejeon", 3 },
		{ 0xFF34495E, "daejeon", 5 },
		{ 0xFF34495E, "daejeon", 6 },
		{ 0xFF34495E, "daejeon", 10 },
		{ 0xFF34495E, "daejeon", 15 }
	};

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) rds1, 5, sizeof(struct realdata));

	// create davi object
	davi = nemodavi_create(canvas, data);
	nemoshow_set_userdata(davi->canvas->show, davi);

	layout = nemodavi_layout_append(davi, NEMODAVI_LAYOUT_AREA);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE, value_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR, color_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME, name_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1, xnum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2, xunit_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR3, ymax_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR4, ynum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR6, sw_getter);

	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_CREATE);

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) rds2, 5, sizeof(struct realdata));

	// create davi object
	davi = nemodavi_create(canvas, data);
	nemoshow_set_userdata(davi->canvas->show, davi);

	layout = nemodavi_layout_append(davi, NEMODAVI_LAYOUT_AREA);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE, value_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR, color_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME, name_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1, xnum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2, xunit_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR3, ymax_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR4, ynum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR6, sw_getter);

	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_CREATE);

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) rds3, 5, sizeof(struct realdata));

	// create davi object
	davi = nemodavi_create(canvas, data);
	nemoshow_set_userdata(davi->canvas->show, davi);

	layout = nemodavi_layout_append(davi, NEMODAVI_LAYOUT_AREA);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE, value_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR, color_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME, name_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1, xnum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2, xunit_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR3, ymax_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR4, ynum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR6, sw_getter);

	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_CREATE);

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) rds4, 5, sizeof(struct realdata));

	// create davi object
	davi = nemodavi_create(canvas, data);
	nemoshow_set_userdata(davi->canvas->show, davi);

	layout = nemodavi_layout_append(davi, NEMODAVI_LAYOUT_AREA);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE, value_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR, color_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME, name_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1, xnum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2, xunit_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR3, ymax_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR4, ynum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR6, sw_getter);

	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_CREATE);

	// bind data array
	data = nemodavi_data_create();
	nemodavi_data_bind_ptr_array(data, (void **) rds5, 5, sizeof(struct realdata));

	// create davi object
	davi = nemodavi_create(canvas, data);
	nemoshow_set_userdata(davi->canvas->show, davi);

	layout = nemodavi_layout_append(davi, NEMODAVI_LAYOUT_AREA);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_VALUE, value_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_COLOR, color_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_NAME, name_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR1, xnum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR2, xunit_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR3, ymax_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR4, ynum_getter);
	nemodavi_layout_set_getter(layout, NEMODAVI_LAYOUT_GETTER_USR6, sw_getter);

	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_CREATE);
}

static void show_chart(struct nemodavi *davi)
{
	struct nemodavi_layout *layout;

	layout = nemodavi_get_layout(davi);
	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_SHOW);
}

static void hide_chart(struct nemodavi *davi)
{
	struct nemodavi_layout *layout;

	layout = nemodavi_get_layout(davi);
	nemodavi_layout_call_delegator(layout, NEMODAVI_LAYOUT_DELEGATOR_HIDE);
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
		nemoshow_dispatch_frame(davi->canvas->show);
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

	create_chart(ctx->canvas, ctx);

	nemoshow_dispatch_frame(ctx->show);
	nemotool_run(ctx->tool);

	deinit_app(ctx);

	return 0;
}
