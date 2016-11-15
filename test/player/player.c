#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <gst/gst.h>

#include <nemotool.h>
#include <nemoglib.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemoshow.h>
#include <nemotimer.h>
#include <nemosound.h>

#include <showeasy.h>
#include <talegesture.h>

#include "log.h"
#include "util.h"
#include "config.h"
#include "color.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "gsthelper.h"
#include "widget.h"

pthread_mutex_t __mutex;

typedef struct _Context Context;
struct _Context {
    bool noevent;
    NemoWidget *win;

    struct nemogst *gst;

	guint8 *video_buffer;
    gint video_width;
    gint video_height;

    struct showone *one;
    int width, height;
};

static void _nemoplayer_dispatch_video_frame(GstElement *base, guint8 *data, gint width, gint height, GstVideoFormat format, gpointer userdata)
{
    Context *ctx = userdata;

	pthread_mutex_lock(&__mutex);

	ctx->video_buffer = data;
	ctx->video_width = width;
	ctx->video_height = height;

	pthread_mutex_unlock(&__mutex);
}

static void nemoplayer_dispatch_tale_timer(struct nemotimer *timer, void *data)
{
    Context *ctx = data;
    NemoWidget *win = ctx->win;
    struct nemoshow *show = nemowidget_get_show(win);
    struct nemotale *tale = NEMOSHOW_AT(show, tale);

	nemotimer_set_timeout(timer, 500);
	nemotale_push_timer_event(tale, time_current_msecs());
}

static void _win_frame(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    struct nemogst *gst = ctx->gst;

	if (nemogst_is_done_media(gst) != 0)
		nemogst_replay_media(gst);

    int width, height;
    void *buffer;

	pthread_mutex_lock(&__mutex);
	width = ctx->video_width;
	height = ctx->video_height;
    buffer = ctx->video_buffer;
	ctx->video_buffer = NULL;
	pthread_mutex_unlock(&__mutex);

	if (buffer != NULL) {
		if (ctx->width != width || ctx->height != height) {
            ERR("%d %d", width, height);
            nemoshow_canvas_resize_pixman(ctx->one, width, height);
			ctx->width = width;
			ctx->height = height;
		}
        nemoshow_canvas_attach_pixman(ctx->one, buffer, width, height);

		nemogst_sink_set_property(gst, "frame-done", 1);
	}
    nemoshow_dispatch_frame(win->show);
    //nemowidget_frame_feedback(win);
}

int main(int argc, char *argv[])
{
    if ((argc < 2) || !(argv[1]) ) {
        ERR("Usage: nemogst [filename]\n");
        return -1;
    }

    int width = 512, height = 512;
    int height2 = 512;

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    Context *ctx;
    ctx = calloc(sizeof(Context), 1);
    RET_IF(!ctx, -1);

    pthread_mutex_init(&__mutex, NULL);

    gst_init(&argc, &argv);

    char *uri;
    uri = _strdup_printf( "file://%s", argv[1]);

    ERR("%s", argv[1]);
    ERR("filename: %s", uri);
    struct nemogst *gst = nemogst_create();
    nemogst_load_media_info(gst, uri);

    ERR("%d %d", nemogst_get_video_width(gst), nemogst_get_video_height(gst));

    if (nemogst_get_video_width(gst) == 0 || nemogst_get_video_height(gst) == 0) {
		nemogst_prepare_audio_sink(gst);
    } else {
        nemogst_prepare_mini_sink(gst, _nemoplayer_dispatch_video_frame, ctx);
        height2 = width / nemogst_get_video_aspect_ratio(gst);
    }
    nemogst_set_media_path(gst, uri);
    nemogst_resize_video(gst, width, height2);
    base->width = ctx->width = width;
    base->height = ctx->height = height;

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, base);
    nemowidget_append_callback(win, "frame", _win_frame, ctx);
    ctx->win = win;

    struct nemoshow *show;
    struct nemotool *tool;
    struct showone *scene;
    show = nemowidget_get_show(win);
    tool = nemowidget_get_tool(win);
    scene = show->scene;


    struct showone *canvas;
    canvas = nemoshow_canvas_create();
    nemoshow_canvas_set_width(canvas, width);
    nemoshow_canvas_set_height(canvas, height);
    nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
    nemoshow_canvas_pivot(canvas, 0.0, 0.0);
    nemoshow_attach_canvas(show, canvas);
    nemoshow_one_attach(scene, canvas);

    struct showone *one;
    one = RRECT_CREATE(canvas, width, height, 2, 2);
    nemoshow_item_set_fill_color(one, RGBA(RED));
    nemoshow_item_set_alpha(one, 1.0);

    canvas = nemoshow_canvas_create();
    nemoshow_canvas_set_width(canvas, width);
    nemoshow_canvas_set_height(canvas, height2);
    nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
    nemoshow_canvas_pivot(canvas, 0.0, 0.0);
    nemoshow_attach_canvas(show, canvas);
    nemoshow_one_attach(scene, canvas);
    ctx->one = canvas;

#if 0
    struct nemotimer *timer;
	timer = nemotimer_create(win->tool);
	nemotimer_set_callback(timer, nemoplayer_dispatch_tale_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, ctx);
#endif

    uint32_t pid = getpid();
    nemosound_set_volume(tool, pid, 5);
    nemogst_play_media(gst);
    ctx->gst = gst;

    TOOL_RUN_GLIB(tool);

    pthread_mutex_destroy(&__mutex);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    free(ctx);

    return 0;
}
