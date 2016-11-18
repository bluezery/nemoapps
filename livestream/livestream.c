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

#include "util.h"
#include "widgets.h"
#include "view.h"
#include "graph.h"
#include "nemohelper.h"

#define COLOR0 0xEA562DFF
#define COLOR1 0x35FFFFFF
#define COLORBACK 0x10171E99

typedef struct _PlayerView PlayerView;
struct _PlayerView {
    struct nemotool *tool;
    struct nemoshow *show;
    int vw, vh;
    int w, h;
    int col, row;
    struct nemoplay *play;
    struct playshader *shader;
    NemoWidget *widget;
    struct nemotimer *video_timer;
    pthread_t pth_decoder, pth_audioplayer;
};

static void *_nemoplay_handle_decode_frame(void *data)
{
    PlayerView *view = data;
    struct nemoplay *play = view->play;
    int state;

	nemoplay_enter_thread(play);

	while ((state = nemoplay_get_state(play)) != NEMOPLAY_DONE_STATE) {
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
    PlayerView *view = data;
	struct nemoplay *play = view->play;
	struct playqueue *queue;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;
	int state;

	nemoplay_enter_thread(play);

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

	nemoplay_leave_thread(play);

	return NULL;
}

static void parse_seconds_to_hms(int64_t seconds, int *hour, int *min, int *sec)
{
    if (hour) *hour = seconds/3600;
    if (min) *min = (seconds%3600)/60;
    if (sec) *sec = (seconds%3600)%60;
}

static void _nemoplay_dispatch_video_timer(struct nemotimer *timer, void *userdata)
{
    PlayerView *view = userdata;
    struct nemoplay *play = view->play;
    struct playqueue *queue;
    struct playone *one;
    int state;

	queue = nemoplay_get_video_queue(play);

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {

#if 0
        double p0, p1;
        p0 = cts/(double)view->duration;
        p1 = 1.0 - cts/(double)view->duration;
        graph_bar_item_set_percent(view->progress_it0, p0);
        graph_bar_item_set_percent(view->progress_it1, p1);
        graph_bar_update(view->progress, NEMOEASE_CUBIC_INOUT_TYPE, 100, 0);
#endif

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
                nemowidget_dirty(view->widget);
                nemoshow_dispatch_frame(view->show);

				if (nemoplay_queue_peek_pts(queue, &pts) != 0)
					nemotimer_set_timeout(timer, MINMAX(pts > cts ? pts - cts : 1.0f, 1.0f, threshold) * 1000);
				else
					nemotimer_set_timeout(timer, threshold * 1000);

				nemoplay_queue_destroy_one(one);

				nemoplay_next_frame(play);
			}
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
        //nemotimer_set_timeout(timer, 1000 / nemoplay_get_video_framerate(play));
        nemotimer_set_timeout(timer, 0);
	}
}

static void _player_resize(NemoWidget *widget, const char *id, void *info, void *data)
{
    NemoWidgetInfo_Resize *resize = info;
    PlayerView *view = data;

    if (!view->shader) return;

	nemoplay_shader_set_viewport(view->shader,
			nemowidget_get_texture(view->widget),
			resize->width, resize->height);

	if (nemoplay_get_frame(view->play) != 0)
		nemoplay_shader_dispatch(view->shader);

    nemowidget_dirty(widget);
}

void playerview_play(PlayerView *view)
{
    struct nemoplay *play = view->play;

    nemotimer_set_timeout(view->video_timer, 10);
    nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);

    nemotimer_set_timeout(view->video_timer, 10);

    pthread_t pth;
    if (pthread_create(&pth, NULL, _nemoplay_handle_decode_frame, (void *)view) != 0) {
        ERR("pthread frame decoder create failed");
    } else view->pth_decoder = pth;

    /*
    if (pthread_create(&pth, NULL, _nemoplay_handle_audioplay, (void *)view) != 0) {
        ERR("pthread audio handler create failed");
        pthread_cancel(view->pth_decoder);
    } else view->pth_audioplayer = pth;
    */
}

void playerview_stop(PlayerView *view)
{
    ERR("STOP");
    nemotimer_set_timeout(view->video_timer, 0);
    nemoplay_wait_thread(view->play);
    pthread_cancel(view->pth_decoder);
    //pthread_cancel(view->pth_audioplayer);

    nemotimer_set_timeout(view->video_timer, 0);
    nemoplay_set_state(view->play, NEMOPLAY_STOP_STATE);
}

void playerview_show(PlayerView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);
    playerview_play(view);
}

void playerview_hide(PlayerView *view)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    playerview_stop(view);
}

static void playerview_destroy(PlayerView *view)
{
    nemoplay_shader_destroy(view->shader);
    nemoplay_destroy(view->play);
    nemotimer_destroy(view->video_timer);
    free(view);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    PlayerView *view = userdata;

    playerview_stop(view);

    nemowidget_win_exit_after(win, 500);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    List *paths;
    bool is_audio;
    int repeat;
    double sxy;
    int col, row;
};

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->is_audio = true;
    app->repeat = -1;
    app->config = config_load(domain, appname, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

    char buf[PATH_MAX];
    const char *temp;

    double sx = 1.0;
    double sy = 1.0;
    int width, height;
    snprintf(buf, PATH_MAX, "%s/size", appname);
    temp = xml_get_value(xml, buf, "width");
    if (!temp) {
        ERR("No size width in %s", appname);
    } else {
        width = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "height");
    if (!temp) {
        ERR("No size height in %s", appname);
    } else {
        height = atoi(temp);
    }
    if (width > 0) sx = (double)app->config->width/width;
    if (width > 0) sy = (double)app->config->height/height;
    if (sx > sy) app->sxy = sy;
    else app->sxy = sx;

    temp = xml_get_value(xml, buf, "col");
    if (!temp) {
        ERR("No size col in %s", appname);
    } else {
        app->col = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "row");
    if (!temp) {
        ERR("No size row in %s", appname);
    } else {
        app->row = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/play", appname);
    temp = xml_get_value(xml, buf, "repeat");
    if (temp && strlen(temp) > 0) {
        app->repeat = atoi(temp);
    }

    List *tags  = xml_search_tags(xml, APPNAME"/file");
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        List *ll;
        XmlAttr *attr;
        LIST_FOR_EACH(tag->attrs, ll, attr) {
            if (!strcmp(attr->key, "path")) {
                if (attr->val) {
                    char *path = strdup(attr->val);
                    app->paths = list_append(app->paths, path);
                    break;
                }
            }
        }
    }

    xml_unload(xml);

    struct option options[] = {
        {"repeat", required_argument, NULL, 'p'},
        {"audio", required_argument, NULL, 'a'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:p:", options, NULL)) != -1) {
        switch(opt) {
            case 'a':
                app->is_audio = !strcmp(optarg, "off") ? false : true;
                break;
            case 'p':
                app->repeat = atoi(optarg);
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    char *path;
    LIST_FREE(app->paths, path) free(path);
    free(app);
}

PlayerView *playerview_create(NemoWidget *parent, int cw, int ch, const char *path, ConfigApp *app)
{
    int vw, vh;
    if (!nemoplay_get_video_info(path, &vw, &vh)) {
        ERR("nemoplay_get_video_info failed: %s", path);
        return NULL;
    }

    PlayerView *view = calloc(sizeof(PlayerView), 1);
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = cw;
    view->h = ch;
    view->col = app->col;
    view->row = app->row;

    int w, h;
    _rect_ratio_fit(vw, vh, cw, ch, &w, &h);
    view->vw = vw;
    view->vh = vh;

    struct nemoplay *play;
    view->play = play = nemoplay_create();
    nemoplay_prepare_media(play, path);
    if (!app->is_audio)
        nemoplay_revoke_audio(play);

    if (nemoplay_get_video_framerate(play) <= 30) {
        nemowidget_win_set_framerate(parent, 30);
    } else {
        nemowidget_win_set_framerate(parent, nemoplay_get_video_framerate(play));
    }

    struct playshader *shader;
    view->shader = shader= nemoplay_shader_create();
    nemoplay_shader_prepare(shader,
            NEMOPLAY_TO_RGBA_VERTEX_SHADER,
            NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
    nemoplay_shader_set_texture(shader, vw, vh);

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_opengl(parent, w, h);
    nemowidget_append_callback(widget, "resize", _player_resize, view);

    view->video_timer = TOOL_ADD_TIMER(view->tool, 0,  _nemoplay_dispatch_video_timer, view);

    return view;
}

void playerview_translate(PlayerView *view, uint32_t easetype, int duration, int delay, float x, float y)
{
    nemowidget_translate(view->widget, easetype, duration, delay, x, y);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->paths) {
        ERR("No playable resources are provided");
        return -1;
    }

	ao_initialize();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "underlay");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    NemoWidget *widget = nemowidget_create_vector(win, app->config->width, app->config->height);
    struct showone *group;
    struct showone *one;
    group = GROUP_CREATE(nemowidget_get_canvas(widget));
    one = RECT_CREATE(group, app->config->width, app->config->height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    int cnt = list_count(app->paths);

    int w, h;
    w = app->config->width/app->col;
    h = app->config->height/app->row;

    int i = 0;
    List *l;
    char *path;
    LIST_FOR_EACH(app->paths, l, path) {
        //if (!file_is_video(path)) continue;
        PlayerView *view = playerview_create(win, w, h, path, app);
        if (!view) continue;

        playerview_translate(view, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                (i%app->col) * w, (i/app->row) * h);
        playerview_show(view);
        i++;
    }

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
	ao_shutdown();

    _config_unload(app);

    return 0;
}
