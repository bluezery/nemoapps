#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemoplay.h>

#include <nemosound.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define TIMEOUT 2000

typedef struct _ScreenSaver ScreenSaver;
struct _ScreenSaver
{
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *parent;

    int width, height;

    NemoWidget *back;
    struct showone *back_one;
    NemoWidget *video;

    pthread_t pth_decoder;
    pthread_t pth_audioplayer;

    struct nemoplay *play;
    struct playshader *shader;
    struct nemotimer *video_timer;
};

static void *_nemoplay_handle_decode_frame(void *data)
{
    ScreenSaver *view = data;
    struct nemoplay *play = view->play;
    int state;

	nemoplay_enter_thread(play);

	while ((state = nemoplay_get_state(play)) != NEMOPLAY_DONE_STATE) {
		if (nemoplay_has_cmd(play, NEMOPLAY_SEEK_CMD) != 0) {
			nemoplay_seek_media(play, 0.0f);
			nemoplay_put_cmd(play, NEMOPLAY_SEEK_CMD);
		}

		if (state == NEMOPLAY_PLAY_STATE) {
			nemoplay_decode_media(play, 256, 128);
		} else if (state == NEMOPLAY_WAIT_STATE || state == NEMOPLAY_STOP_STATE) {
			nemoplay_wait_media(play);
		}
	}

	nemoplay_leave_thread(play);

    return NULL;
}

static void *_nemoplay_handle_audioplay(void *data)
{
    ScreenSaver *view = data;
	struct nemoplay *play = view->play;
	struct playqueue *queue;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;
	int state;

	nemoplay_enter_thread(play);

	ao_initialize();

	format.channels = nemoplay_get_audio_channels(play);
	format.bits = nemoplay_get_audio_samplebits(play);
	format.rate = nemoplay_get_audio_samplerate(play);
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;

	driver = ao_default_driver_id();
    device = ao_open_live(driver, &format, NULL);
    if (device == NULL) {
        nemoplay_revoke_audio(play);
        ao_shutdown();

        nemoplay_leave_thread(play);
        return NULL;
    }

    queue = nemoplay_get_audio_queue(play);

	while ((state = nemoplay_queue_get_state(queue)) != NEMOPLAY_QUEUE_DONE_STATE) {
		if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
			if (nemoplay_queue_get_count(queue) < 64)
                nemoplay_set_state(play, NEMOPLAY_WAKE_STATE);

			one = nemoplay_queue_dequeue(queue);
			if (one == NULL) {
				nemoplay_queue_wait(queue);
			} else if (nemoplay_queue_get_one_serial(one) != nemoplay_queue_get_serial(queue)) {
				nemoplay_queue_destroy_one(one);
			} else if (nemoplay_queue_get_one_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {

				nemoplay_set_audio_pts(play, nemoplay_queue_get_one_pts(one));

				ao_play(device,
						nemoplay_queue_get_one_data(one),
						nemoplay_queue_get_one_size(one));

				nemoplay_queue_destroy_one(one);
			}
		} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
			nemoplay_queue_wait(queue);
		}
	}

	ao_close(device);
	ao_shutdown();

	nemoplay_leave_thread(play);

	return NULL;
}

static void _nemoplay_dispatch_video_timer(struct nemotimer *timer, void *userdata)
{
    ScreenSaver *view = userdata;
    struct nemoplay *play = view->play;
    struct playqueue *queue;
    struct playone *one;
    int state;

	queue = nemoplay_get_video_queue(play);

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
		double cts = nemoplay_get_cts(play);

		if (cts >= nemoplay_get_duration(play))
			nemoplay_set_cmd(play, NEMOPLAY_SEEK_CMD);
		if (nemoplay_queue_get_count(queue) < 64)
            nemoplay_set_state(play, NEMOPLAY_WAKE_STATE);

		one = nemoplay_queue_dequeue(queue);
		if (one == NULL) {
			nemotimer_set_timeout(timer, 1000 / nemoplay_get_video_framerate(play));
		} else if (nemoplay_queue_get_one_serial(one) != nemoplay_queue_get_serial(queue)) {
			nemoplay_queue_destroy_one(one);
			nemotimer_set_timeout(timer, 1);
		} else if (nemoplay_queue_get_one_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
			double threshold = 1.0f / nemoplay_get_video_framerate(play);
			double cts = nemoplay_get_cts(play);
			double pts;

			nemoplay_set_video_pts(play, nemoplay_queue_get_one_pts(one));

			if (cts > nemoplay_queue_get_one_pts(one) + threshold) {
				nemoplay_queue_destroy_one(one);
				nemotimer_set_timeout(timer, 1);
			} else if (cts < nemoplay_queue_get_one_pts(one) - threshold) {
				nemoplay_queue_enqueue_tail(queue, one);
				nemotimer_set_timeout(timer, threshold * 1000);
			} else {
				if (nemoplay_shader_get_texture_linesize(view->shader) !=
                        nemoplay_queue_get_one_width(one))
					nemoplay_shader_set_texture_linesize(view->shader,
                            nemoplay_queue_get_one_width(one));

				nemoplay_shader_update(view->shader,
						nemoplay_queue_get_one_y(one),
						nemoplay_queue_get_one_u(one),
						nemoplay_queue_get_one_v(one));
				nemoplay_shader_dispatch(view->shader);
                nemowidget_dirty(view->video);
                nemoshow_dispatch_frame(view->show);

				if (nemoplay_queue_peek_pts(queue, &pts) != 0)
					nemotimer_set_timeout(timer,
                            MINMAX(pts > cts ? pts - cts : 1.0f, 1.0f, threshold) * 1000);
				else
					nemotimer_set_timeout(timer, threshold * 1000);

				nemoplay_queue_destroy_one(one);

				nemoplay_next_frame(play);
			}
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
        nemotimer_set_timeout(timer, 1000 / nemoplay_get_video_framerate(play));
	}
}

static void _playerview_opengl_resize(NemoWidget *widget, const char *id, void *info, void *data)
{
    NemoWidgetInfo_Resize *resize = info;
    ScreenSaver *view = data;

    if (!view->shader) return;

	nemoplay_shader_set_viewport(view->shader,
			nemowidget_get_texture(view->video),
			resize->width, resize->height);

	if (nemoplay_get_frame(view->play) != 0)
		nemoplay_shader_dispatch(view->shader);

    nemowidget_dirty(widget);
}

static void _playerview_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    ScreenSaver *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    NemoWidget *win = nemowidget_get_top_widget(view->back);
    if (nemoshow_event_is_down(show, event)) {
        nemowidget_callback_dispatch(win, "exit", NULL);
    }
}

ScreenSaver *playerview_create(NemoWidget *parent, int width, int height, int glw, int glh, int vw, int vh, const char *path, bool is_audio)
{
    struct nemotool *tool;
    struct nemoshow *show;
    ScreenSaver *view = calloc(sizeof(ScreenSaver), 1);
    view->tool = tool = nemowidget_get_tool(parent);
    view->show = show = nemowidget_get_show(parent);
    view->width = width;
    view->width = height;

    NemoWidget *widget;
    view->back = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *one;
    view->back_one = one = RECT_CREATE(nemowidget_get_canvas(widget), width, height);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));

    struct nemoplay *play = nemoplay_create();
    nemoplay_load_media(play, path);
    if (!is_audio) nemoplay_revoke_audio(play);
    view->play = play;

    struct playshader *shader;
    view->shader = shader= nemoplay_shader_create();
    nemoplay_shader_prepare(shader,
            NEMOPLAY_TO_RGBA_VERTEX_SHADER,
            NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
    nemoplay_shader_set_texture(shader, vw, vh);

    struct nemotimer *timer;

    view->video = widget = nemowidget_create_opengl(parent, glw, glh);
    nemowidget_append_callback(widget, "event", _playerview_event, view);
    nemowidget_append_callback(widget, "resize", _playerview_opengl_resize, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_translate(widget, 0, 0, 0, (width - glw)/2, (height - glh)/2);

    view->video_timer = timer = TOOL_ADD_TIMER(tool, 0,
            _nemoplay_dispatch_video_timer, view);

    return view;
}

static void playerview_show(ScreenSaver *view)
{
    nemowidget_set_alpha(view->back, NEMOEASE_CUBIC_OUT_TYPE, TIMEOUT, 0, 1.0);
    nemowidget_show(view->back, 0, 0, 0);

    nemowidget_set_alpha(view->video, NEMOEASE_CUBIC_OUT_TYPE, TIMEOUT, 0, 1.0);
    nemowidget_show(view->video, 0, 0, 0);

    nemotimer_set_timeout(view->video_timer, 10);

    pthread_t pth;
	if (pthread_create(&pth, NULL, _nemoplay_handle_decode_frame, (void *)view) != 0) {
        ERR("pthread frame decoder create failed");
    } else
        view->pth_decoder = pth;

	if (pthread_create(&pth, NULL, _nemoplay_handle_audioplay, (void *)view) != 0) {
        ERR("pthread audio handler create failed");
        pthread_cancel(view->pth_decoder);
    } else
        view->pth_audioplayer = pth;

    nemoshow_dispatch_frame(view->show);
}

static void playerview_hide(ScreenSaver *view)
{
    nemowidget_set_alpha(view->back, NEMOEASE_CUBIC_OUT_TYPE, TIMEOUT, 0, 0.0);
    nemowidget_hide(view->back, 0, 0, 0);

    nemowidget_set_alpha(view->video, NEMOEASE_CUBIC_OUT_TYPE, TIMEOUT, 0, 0.0);
    nemowidget_hide(view->video, 0, 0, 0);

    nemoplay_set_state(view->play, NEMOPLAY_DONE_STATE);
    nemoplay_wait_thread(view->play);
    pthread_cancel(view->pth_decoder);
    pthread_cancel(view->pth_audioplayer);

    nemotimer_set_timeout(view->video_timer, 0);
    nemoplay_set_state(view->play, NEMOPLAY_STOP_STATE);

    nemoshow_dispatch_frame(view->show);
}

static void playerview_destroy(ScreenSaver *view)
{
    nemoplay_shader_destroy(view->shader);
    nemoplay_destroy(view->play);
    nemotimer_destroy(view->video_timer);
    free(view);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    ScreenSaver *view = userdata;

    playerview_hide(view);
    nemowidget_win_exit_after(win, TIMEOUT + 100);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *path;
    bool is_audio;
};

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->is_audio = true;
    app->config = config_load(domain, appname, filename, argc, argv);

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        {"audio", required_argument, NULL, 'a'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:a:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                app->path = strdup(optarg);
                break;
            case 'a':
                app->is_audio = !strcmp(optarg, "off") ? false : true;
                break;
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->path) free(app->path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->path) {
        ERR("Usage: %s -f FILENAME [-a off]", APPNAME);
        return -1;
    }

    int vw, vh;
    if (!nemoplay_get_video_info(app->path, &vw, &vh)) {
        ERR("nemoplay_get_video_info failed: %s", app->path);
        return -1;
    }

    int glw, glh;
    _rect_ratio_fit(vw, vh, app->config->width, app->config->height, &glw, &glh);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    // XXX: all windows are not visible under screensaver
    nemowidget_win_enable_state(win, "opaque", true);
    nemowidget_win_set_anchor(win, 0.0, 0.0);
    nemowidget_win_set_layer(win, "overlay");

    ScreenSaver *view = playerview_create(win,
            app->config->width, app->config->height, glw, glh, vw, vh, app->path, app->is_audio);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    playerview_show(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    playerview_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
