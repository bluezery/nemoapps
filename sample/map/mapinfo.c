#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemolist.h>

#include "color.h"
#include "mapinfo.h"
#include "nemowrapper.h"

struct context {
	struct nemoshow *show;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct showone *canvas;
	int width;
	int height;
};

static struct context *g_context;

static struct mapinfo_mode* g_mapinfo_mode_table;

static void init_mapinfo_mode_table(struct showone* canvas, int scene_w, int scene_h)
{
	int i, size;
 	size = sizeof (struct mapinfo_mode) * MAPINFO_MODE_LAST;
	g_mapinfo_mode_table = (struct mapinfo_mode *) malloc(size); 
	memset(g_mapinfo_mode_table, 0x0, size);

	for (i = 0; i < MAPINFO_MODE_LAST; i++) {
		g_mapinfo_mode_table[i].canvas = canvas;
		g_mapinfo_mode_table[i].width = scene_w;
		g_mapinfo_mode_table[i].height = scene_h;
	}

	mapinfo_mode_runner_init(&g_mapinfo_mode_table[MAPINFO_MODE_RUNNER]);
	mapinfo_mode_party_init(&g_mapinfo_mode_table[MAPINFO_MODE_PARTY]);
}

static void dispatch_timer(struct nemotimer *timer, void *data)
{
}


int mapinfo_update_ui(Area* areainfo, MapInfoMode mode, MapInfoReq req)
{
	static int current_mode = MAPINFO_MODE_LAST;
	static int current_state = MAPINFO_REQ_LAST;
	uint32_t delay = 0;

	printf("current mode: %d\n", current_mode);
	printf("current state: %d\n", current_state);

	if (req == MAPINFO_REQ_ENTER) {
		if (current_state == MAPINFO_REQ_ENTER) {
			delay = g_mapinfo_mode_table[current_mode].exit(0);
		}
		// set area
		g_mapinfo_mode_table[mode].prepare(areainfo);
		g_mapinfo_mode_table[mode].enter(delay);
	} else if (req == MAPINFO_REQ_EXIT) {
		if (current_state == MAPINFO_REQ_EXIT) {
			printf("aleady exit mode\n");
			return -1;
		}
		g_mapinfo_mode_table[current_mode].exit(0);
	} else if (req == MAPINFO_REQ_UPDATE) {
		if (mode != current_mode)  {
			printf("not same mode");
			return -1;
		}
		if (current_state != MAPINFO_REQ_ENTER) {
			printf("not enter mode\n");
			return -1;
		}
		g_mapinfo_mode_table[mode].update(areainfo);
	} else {
		printf("invalid request\n");
		return -1;
	}

	current_mode = mode;
	if (req != MAPINFO_REQ_UPDATE) {
		current_state = req;
	}

	return 0;
}

int mapinfo_init(struct nemotool *tool, struct nemoshow* show, struct showone* canvas, int scene_w, int scene_h)
{
	if (!show || !canvas) {
		printf("mapinfo invalid arguments");
		return -1;
	}

	if (g_context) {
		printf("mapinfo already inited\n");
		return -1;
	}

	g_context = (struct context *)malloc(sizeof(struct context));
	memset(g_context, 0, sizeof(struct context));

	g_context->show = show;
	g_context->canvas = canvas;
	g_context->tool = tool;
	g_context->width = scene_w;
	g_context->height = scene_h;
	g_context->timer = nemotimer_create(tool);

	nemotimer_set_callback(g_context->timer, dispatch_timer);
	nemotimer_set_userdata(g_context->timer, g_context);

	init_mapinfo_mode_table(canvas, scene_w, scene_h);

	return 0;
}

int mapinfo_deinit()
{
	if (!g_context) {
		printf("mapinfo not inited\n");
		return -1;
	}

	free(g_context);
	return 0;
}
