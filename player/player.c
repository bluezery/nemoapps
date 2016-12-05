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
#include <nemobus.h>

#include <nemoplay.h>
#include <nemosound.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "nemohelper.h"

#define NEMOPLAY_POS_CMD 0x10000
#define NEMOPLAY_FF_CMD 0x10
#define NEMOPLAY_FR_CMD 0x100
#define NEMOPLAY_RESET_CMD 0x1000

#define COLOR0 0xEA562DFF
#define COLOR1 0x35FFFFFF
#define COLORBACK 0x10171E99

#define FRAME_TIMEOUT 2000
#define OVERLAY_TIMEOUT 1500

#define OBJPATH "/nemoplayer"
#define OBJPATH_REMOTE "/nemoremote"

typedef struct _PlayerView PlayerView;
struct _PlayerView
{
    struct nemobus *bus;
    char *objpath;

    int width, height;
    struct nemotool *tool;
    struct nemoshow *show;

    bool fin;
    bool is_playing;
    int repeat;  // -1, 0, 1
    char *path;

    char *screenid;


    Frame *frame;

    NemoWidget *video;

    pthread_t pth_decoder;
    pthread_t pth_audioplayer;

    struct nemoplay *play;
    struct playshader *shader;
    struct nemotimer *video_timer;

    struct nemotimer *frame_timer;
    Sketch *sketch;

    NemoWidget *overlay;
    struct nemotimer *overlay_timer;

    struct showone *overlay_group;
    struct showone *top;
    struct showone *bottom;
    struct showone *text_blur;
    double ss_w, ss_h;

    char *filename;
    Text *name_back, *name;
    struct showone *fr_back;
    Button *fr;
    Button *ff;
    Button *nf;
    Button *pf;
    Button *vol_up;
    Button *vol_mute;
    Button *vol_down;
    struct showone *vol;
    bool is_mute;
    int vol_cnt;

    struct showone *playbar;
    struct showone *ss_back;
    struct showone *ss0;
    struct showone *ss1;

    int progress_x, progress_y;
    struct showone *progress_gradient;
    struct showone *progress_circle;
    GraphBar *progress;
    GraphBarItem *progress_it0;
    GraphBarItem *progress_it1;
    double progress_pos;
    struct showone *progress_event;

    struct nemotimer *progress_timer;

    Text *cur_time_back;
    Text *cur_time;
    Text *total_time_back;
    Text *total_time;
    int32_t duration;

    DrawingBox *dbox;
    bool dbox_grab;
    double dbox_grab_diff_x, dbox_grab_diff_y;
};

static void *_nemoplay_handle_decode_frame(void *data)
{
    PlayerView *view = data;
    struct nemoplay *play = view->play;
    int state;

	nemoplay_enter_thread(play);

	while ((state = nemoplay_get_state(play)) != NEMOPLAY_DONE_STATE) {
        if (nemoplay_has_cmd(play, NEMOPLAY_POS_CMD)) {
            nemoplay_put_cmd(play, NEMOPLAY_POS_CMD);
            nemoplay_seek_media(play, view->progress_pos);

            if (state == NEMOPLAY_STOP_STATE) {
                nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
                nemoplay_decode_media(play, 256, 128);
                nemoplay_set_state(play, NEMOPLAY_STOP_STATE);
            }
        } else if (nemoplay_has_cmd(play, NEMOPLAY_FR_CMD)) {
            nemoplay_put_cmd(play, NEMOPLAY_FR_CMD);
            double cts = nemoplay_get_cts(view->play);
            if (cts - 10 < 0) {
                cts = 0;
            } else
                cts = cts - 10;
            nemoplay_seek_media(play, cts);

            if (state == NEMOPLAY_STOP_STATE) {
                nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
                nemoplay_decode_media(play, 256, 128);
                nemoplay_set_state(play, NEMOPLAY_STOP_STATE);
            }
        } else if (nemoplay_has_cmd(play, NEMOPLAY_FF_CMD)) {
            nemoplay_put_cmd(play, NEMOPLAY_FF_CMD);
            double cts = nemoplay_get_cts(view->play);
            if (cts + 10 > view->duration) {
                cts = view->duration;
            } else
                cts = cts + 10;
            nemoplay_seek_media(play, cts);

            if (state == NEMOPLAY_STOP_STATE) {
                nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
                nemoplay_decode_media(play, 256, 128);
                nemoplay_set_state(play, NEMOPLAY_STOP_STATE);
            }
        } else if (nemoplay_has_cmd(play, NEMOPLAY_RESET_CMD)) {
			nemoplay_put_cmd(play, NEMOPLAY_RESET_CMD);
			nemoplay_seek_media(play, 0.0);
            nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
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
    PlayerView *view = data;
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

static void _nemoplay_dispatch_progress_timeout(struct nemotimer *timer, void *userdata)
{
    PlayerView *view = userdata;
    bool showit = false;

    double cts = nemoplay_get_cts(view->play);
    if (cts < view->duration - 0.1) {
        nemotimer_set_timeout(view->progress_timer, 200);
    } else if (view->repeat != 0) {
        ERR("REPEAT");
        view->progress_pos = 0.0;
        nemoplay_set_cmd(view->play, NEMOPLAY_POS_CMD);
        nemotimer_set_timeout(view->progress_timer, 10);
        if (view->repeat > 0) view->repeat--;
        showit = true;
    } else {
        ERR("FIN");
        view->fin = true;
        view->is_playing = false;
        nemotimer_set_timeout(view->progress_timer, 0);
        nemotimer_set_timeout(view->video_timer, 0);
        nemoplay_set_state(view->play, NEMOPLAY_STOP_STATE);

        _nemoshow_item_motion(view->ss0,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(view->ss1,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 1.0, NULL);

        cts = view->duration;
        showit = true;
    }

    if (view->overlay->show || showit) {
        char buf[16];
        int hour, min, sec;
        parse_seconds_to_hms(cts, &hour, &min, &sec);
        snprintf(buf, 16, "%02d:%02d:%02d", hour, min, sec);
        text_update(view->cur_time_back, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, buf);
        text_update(view->cur_time, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, buf);

        double p0, p1;
        p0 = cts/(double)view->duration;
        p1 = 1.0 - cts/(double)view->duration;
        graph_bar_item_set_percent(view->progress_it0, p0);
        graph_bar_item_set_percent(view->progress_it1, p1);
        graph_bar_update(view->progress, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0);
    }

    if (!view->is_playing) {
        nemotimer_set_timeout(timer, 0);
    }
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
                nemowidget_dirty(view->video);
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

static void _playerview_resize(NemoWidget *widget, const char *id, void *info, void *data)
{
    NemoWidgetInfo_Resize *resize = info;
    PlayerView *view = data;

    if (!view->shader) return;

	nemoplay_shader_set_viewport(view->shader,
			nemowidget_get_texture(view->video),
			resize->width, resize->height);

	if (nemoplay_get_frame(view->play) != 0)
		nemoplay_shader_dispatch(view->shader);

    nemowidget_dirty(widget);
}

static void _drawingbox_adjust(PlayerView *view, double tx, double ty)
{
    double gap = 5;
    double dx, dy;
    drawingbox_get_translate(view->dbox, &dx, &dy);
    if (tx < 0) tx = dx;
    if (ty < 0) ty = dy;

    double r;
    r = view->dbox->r;

    double x0, y0, x1, y1;

    // Move inside sketch area
    if (view->dbox->mode == 1) {
        r *= 3.5;
    }
    x0 = tx - r;
    x1 = tx + r;
    y0 = ty - r;
    y1 = ty + r;
    if (x0 < 0)
        tx = r + gap;
    else if (x1 > view->sketch->w)
        tx = view->sketch->w - r - gap;
    if (y0 < 0)
        ty = r + gap;
    else if (y1 > view->sketch->h)
        ty = view->sketch->h - r - gap;
    drawingbox_translate(view->dbox, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            tx, ty);
}

static void playerview_overlay_show(PlayerView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->overlay, 0, 0, 0);
    nemowidget_set_alpha(view->overlay, easetype, duration, delay, 1.0);
    nemotimer_set_timeout(view->overlay_timer, duration + delay + OVERLAY_TIMEOUT);
}

static void playerview_overlay_hide(PlayerView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->overlay, 0, 0, 0);
    nemowidget_set_alpha(view->overlay, easetype, duration, delay, 0.0);
    nemotimer_set_timeout(view->overlay_timer, 0);
}

static void playerview_overlay_down(PlayerView *view)
{
    nemotimer_set_timeout(view->overlay_timer, 0);
    nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
    nemowidget_show(view->overlay, 0, 0, 0);
    nemoshow_dispatch_frame(view->show);
}

static void playerview_overlay_up(PlayerView *view)
{
    nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);
}

void playerview_play(PlayerView *view)
{
    struct nemoplay *play = view->play;
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    nemotimer_set_timeout(view->progress_timer, 10);
    nemotimer_set_timeout(view->video_timer, 10);

    _nemoshow_item_motion(view->ss0,
            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
    _nemoshow_item_motion(view->ss1,
            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    if (view->fin) {
        ERR("RESET");
        view->is_playing = true;
        view->fin = false;

        nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
        nemoplay_set_cmd(play, NEMOPLAY_RESET_CMD);

    } else {
        ERR("PLAY");
        view->is_playing = true;
        nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
    }
}

void playerview_stop(PlayerView *view)
{
    ERR("STOP");
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->is_playing = false;
    nemotimer_set_timeout(view->progress_timer, 0);
    nemotimer_set_timeout(view->video_timer, 0);

    _nemoshow_item_motion(view->ss0,
            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(view->ss1,
            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);

    nemoplay_set_state(view->play, NEMOPLAY_STOP_STATE);
}

void playerview_fr(PlayerView *view)
{
    ERR("FR");
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->fin = false;
    nemotimer_set_timeout(view->progress_timer, 10);
    nemotimer_set_timeout(view->video_timer, 10);

    nemoplay_set_cmd(view->play, NEMOPLAY_FR_CMD);
}

void playerview_ff(PlayerView *view)
{
    ERR("FF");
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    if (!view->fin) {
        //view->fin = false;
        nemotimer_set_timeout(view->progress_timer, 10);
        nemotimer_set_timeout(view->video_timer, 10);
        nemoplay_set_cmd(view->play, NEMOPLAY_FF_CMD);
    }
}

void playerview_volume_up(PlayerView *view)
{
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->vol_cnt += 10;
    if (view->vol_cnt > 100) view->vol_cnt = 100;
    ERR("%d", view->vol_cnt);
    nemosound_set_current_volume(view->tool, view->vol_cnt);

    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
            "to", (360/100.0) * view->vol_cnt - 90.0, NULL);
}

void playerview_volume_down(PlayerView *view)
{
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->vol_cnt -= 10;
    if (view->vol_cnt < 0) view->vol_cnt = 0;
    ERR("%d", view->vol_cnt);
    nemosound_set_current_volume(view->tool, view->vol_cnt);

    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
            "to", (360/100.0) * view->vol_cnt - 90.0, NULL);
}

void playerview_volume_enable(PlayerView *view)
{
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->is_mute = false;
    nemosound_set_current_mute(view->tool, view->is_mute);
    button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            0, PLAYER_ICON_DIR"/volume.svg", -1, -1);
    button_set_fill(view->vol_mute, 0, 0, 0, 0, COLOR1);
    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "stroke", COLOR1,
            NULL);
}

void playerview_volume_disable(PlayerView *view)
{
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    view->is_mute = true;
    nemosound_set_current_mute(view->tool, view->is_mute);
    button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            0, PLAYER_ICON_DIR"/volume_mute.svg", -1, -1);
    button_set_fill(view->vol_mute, 0, 0, 0, 0, GRAY);
    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "stroke", GRAY,
            NULL);
}

static void _overlay_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    PlayerView *view = userdata;
    struct nemoplay *play = view->play;
    struct nemoshow *show = nemowidget_get_show(widget);

    struct showone *one = nemowidget_grab_get_data(grab, "button");
    const char *id = one->id;
    uint32_t tag = nemoshow_one_get_tag(one);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    frame_util_transform_to_group(view->overlay_group, ex, ey, &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        if (id && !strcmp(id, "dbox")) {
            Button *btn = nemoshow_one_get_userdata(one);
            button_down(btn);
            nemowidget_enable_event_repeat(widget, false);

            double cx, cy;
            drawingbox_get_translate(view->dbox, &cx, &cy);
            view->dbox_grab_diff_x = cx - ex;
            view->dbox_grab_diff_y = cy - ey;
            view->dbox_grab = true;

            NemoWidget *win = nemowidget_get_top_widget(widget);
            nemowidget_win_enable_move(win, false);
            nemowidget_win_enable_rotate(win, false);
            nemowidget_win_enable_scale(win, false);
            nemotimer_set_timeout(view->frame_timer, 10);
        } else if (!view->sketch->enable && id && !strcmp(id, "player")) {
            if (tag == 1) {
                _nemoshow_item_motion_bounce(view->progress_circle,
                        NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                        "sx", 0.7, 0.75,
                        "sy", 0.7, 0.75,
                        NULL);
                graph_bar_translate(view->progress, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                        view->progress_x - 5, view->progress_y);
            } else if (2 <= tag && tag <= 6) {
                Button *btn = nemoshow_one_get_userdata(one);
                button_down(btn);
                if (tag == 5) {
                    _nemoshow_item_motion_bounce(view->vol,
                            NEMOEASE_CUBIC_INOUT_TYPE, 150, 100,
                            "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                            NULL);
                }
            } else if (tag == 50) {
                graph_bar_item_set_color(view->progress_it0, COLOR0, COLOR0);
                nemotimer_set_timeout(view->progress_timer, 10);
            }
        }
    } else if (nemoshow_event_is_motion(show, event)) {
        if (view->dbox_grab) {
            drawingbox_translate(view->dbox, 0, 0, 0,
                    ex + view->dbox_grab_diff_x,
                    ey + view->dbox_grab_diff_y);
            nemowidget_dirty(widget);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        if (!view->sketch->enable) {
            NemoWidget *win = nemowidget_get_top_widget(widget);
            nemowidget_win_enable_move(win, true);
            nemowidget_win_enable_rotate(win, true);
            nemowidget_win_enable_scale(win, true);
            nemotimer_set_timeout(view->frame_timer, 0);
        }
        nemowidget_enable_event_repeat(widget, true);
        if (view->dbox_grab) {
            double tx, ty;
            tx = ex + view->dbox_grab_diff_x;
            ty = ey + view->dbox_grab_diff_y;

            _drawingbox_adjust(view, tx, ty);
            view->dbox_grab_diff_x = 0;
            view->dbox_grab_diff_y = 0;
            view->dbox_grab = false;
        }

        if (id && !strcmp(id, "dbox")) {
            if (one) {
                Button *btn = nemoshow_one_get_userdata(one);
                button_up(btn);
            }
            if (nemoshow_event_is_single_click(show, event)) {
                uint32_t tag = nemoshow_one_get_tag(one);
                if (tag == 10) {
                    nemotimer_set_timeout(view->overlay_timer, 0);

                    drawingbox_show_menu(view->dbox);
                    _drawingbox_adjust(view, -1, -1);

                    sketch_set_color(view->sketch, view->dbox->color);
                    sketch_set_size(view->sketch, view->dbox->size);
                    sketch_enable(view->sketch, true);
                    _nemoshow_item_motion(view->top,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);
                    _nemoshow_item_motion(view->bottom,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);

                    NemoWidget *win = nemowidget_get_top_widget(widget);
                    nemowidget_win_enable_move(win, false);
                    nemowidget_win_enable_rotate(win, false);
                    nemowidget_win_enable_scale(win, false);
                    nemotimer_set_timeout(view->frame_timer, 10);
                } else if (tag == 11) {
                    nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);

                    drawingbox_show_pencil(view->dbox);
                    _drawingbox_adjust(view, -1, -1);

                    sketch_enable(view->sketch, false);
                    _nemoshow_item_motion(view->top,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);
                    _nemoshow_item_motion(view->bottom,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);

                    NemoWidget *win = nemowidget_get_top_widget(widget);
                    nemowidget_win_enable_move(win, true);
                    nemowidget_win_enable_rotate(win, true);
                    nemowidget_win_enable_scale(win, true);
                    nemotimer_set_timeout(view->frame_timer, 0);
                } else if (tag == 12) {
                    // share
                } else if (tag == 13) {
                    sketch_undo(view->sketch, 1);
                } else if (tag == 14) {
                    sketch_undo(view->sketch, -1);
                } else if (tag == 15) {
                    drawingbox_change_stroke(view->dbox, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
                    sketch_set_size(view->sketch, view->dbox->size);
                } else if (tag == 16) {
                    drawingbox_change_color(view->dbox, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
                    sketch_set_color(view->sketch, view->dbox->color);
                }
            }
        } else if (!view->sketch->enable && id && !strcmp(id, "player")) {
            if (tag == 1) {
                _nemoshow_item_motion_bounce(view->progress_circle,
                        NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                        "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                        NULL);
                graph_bar_translate(view->progress, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                        view->progress_x, view->progress_y);
            } else if (2 <= tag && tag <= 6) {
                Button *btn = nemoshow_one_get_userdata(one);
                button_up(btn);
                if (tag == 5) {
                    _nemoshow_item_motion_bounce(view->vol,
                            NEMOEASE_CUBIC_INOUT_TYPE, 150, 100,
                            "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                            NULL);
                }
            } else if (tag == 100) {
                graph_bar_item_set_color(view->progress_it0, COLOR1, COLOR1);
                nemotimer_set_timeout(view->progress_timer, 10);
            }

            if (nemoshow_event_is_single_click(show, event)) {
                if (tag == 1) {
                    if (view->play->state == NEMOPLAY_WAIT_STATE) {
                        playerview_stop(view);
                    } else {
                        playerview_play(view);
                    }
                } else if (tag == 2) {
                    playerview_fr(view);
                } else if (tag == 3) {
                    playerview_ff(view);
                } else if (tag == 4) {
                    playerview_volume_up(view);
                } else if (tag == 5) {
                    if (view->is_mute) {
                        playerview_volume_enable(view);
                    } else {
                        playerview_volume_disable(view);
                    }
                } else if (tag == 6) {
                    playerview_volume_down(view);
                } else if (tag == 100) {
                    view->fin = false;
                    int x, w;
                    graph_bar_get_geometry(view->progress, &x, NULL, &w, NULL);

                    double ex, ey;
                    nemowidget_transform_from_global(widget,
                            nemoshow_event_get_x(event),
                            nemoshow_event_get_y(event), &ex, &ey);
                    ERR("POS: %lf", ((ex - x)/w) * view->duration);
                    view->progress_pos = ((ex - x)/w) * view->duration;
                    nemoplay_set_cmd(play, NEMOPLAY_POS_CMD);
                    nemotimer_set_timeout(view->progress_timer, 10);
                }
            }
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void _playerview_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    PlayerView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_down(show, event)) {
        playerview_overlay_down(view);
    } else if (nemoshow_event_is_up(show, event)) {
        playerview_overlay_up(view);
    }
}

static void _overlay_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    PlayerView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (!view->sketch->enable) {
        if (nemoshow_event_is_down(show, event)) {
            playerview_overlay_down(view);
        } else if (nemoshow_event_is_up(show, event)) {
            playerview_overlay_up(view);
        }
    }

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            NemoWidgetGrab *grab;
            grab = nemowidget_create_grab(widget, event,
                        _overlay_grab_event, view);
            nemowidget_grab_set_data(grab, "button", one);
        }
    }
}

static void _playerview_frame_timeout(struct nemotimer *timer, void *userdata)
{
    PlayerView *view = userdata;
    frame_gradient_motion(view->frame, NEMOEASE_CUBIC_INOUT_TYPE, FRAME_TIMEOUT, 0);
    nemotimer_set_timeout(timer, FRAME_TIMEOUT);
    nemoshow_dispatch_frame(view->show);
}


static void _playerview_overlay_timeout(struct nemotimer *timer, void *userdata)
{
    PlayerView *view = userdata;
    nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0);
    nemowidget_hide(view->overlay, 0, 0, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _sketch_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    PlayerView *view = userdata;
    if (view->sketch->enable) return;

    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_down(show, event)) {
        playerview_overlay_down(view);
    } else if (nemoshow_event_is_up(show, event)) {
        playerview_overlay_up(view);
    }
}

static void _bus_callback(void *data, const char *events)
{
    RET_IF(!events);
    if (strchr(events, 'u')) {
        ERR("SIGHUP received");
        return;
    } else if (strchr(events, 'e')) {
        ERR("Error received");
        return;
    }

    PlayerView *view = data;

	struct nemoitem *it;
	struct itemone *one;
    it = nemobus_recv_item(view->bus);
    RET_IF(!it);

	nemoitem_for_each(one, it) {
        const char *from = nemoitem_one_get_attr(one, "from");
        if (!from){
            ERR("No from in the attribute");
            continue;
        }

        const char *path = nemoitem_one_get_path(one);
        const char *name = strrchr(path, '/');
        if (!path) continue;
        if (!name) continue;


        if (!strcmp(name, "/property")) {
            struct busmsg *msg;
            msg = nemobus_msg_create();
            nemobus_msg_set_name(msg, "property");
            nemobus_msg_set_attr(msg, "from", view->objpath);
            nemobus_msg_set_attr(msg, "filename", view->filename);
            nemobus_msg_set_attr_format(msg, "isplaying", "%s",
                    view->is_playing ? "true" : "false");
            nemobus_msg_set_attr_format(msg, "ismute", "%s",
                    view->is_mute ? "true" : "false");
            nemobus_msg_set_attr_format(msg, "volume", "%d", view->vol_cnt);
            nemobus_msg_set_attr(msg, "from", view->objpath);
            nemobus_send(view->bus, view->objpath, from, msg);
            nemobus_msg_destroy(msg);
        } else if (!strcmp(name, "/command")) {
            const char *type = nemoitem_one_get_attr(one, "type");
            ERR("%s", type);
            if (!type) continue;
            if (!strcmp(type, "exit")) {
                NemoWidget *win = nemowidget_get_top_widget(view->video);
                nemowidget_callback_dispatch(win, "exit", NULL);
            } else if (!strcmp(type, "play")) {
                playerview_play(view);
            } else if (!strcmp(type, "stop")) {
                playerview_stop(view);
            } else if (!strcmp(type, "fr")) {
                playerview_fr(view);
            } else if (!strcmp(type, "ff")) {
                playerview_ff(view);
            } else if (!strcmp(type, "volume-up")) {
                playerview_volume_up(view);
            } else if (!strcmp(type, "volume-down")) {
                playerview_volume_down(view);
            } else if (!strcmp(type, "volume-enable")) {
                playerview_volume_enable(view);
            } else if (!strcmp(type, "volume-disable")) {
                playerview_volume_disable(view);
            }

            struct busmsg *msg;
            msg = nemobus_msg_create();
            nemobus_msg_set_name(msg, "property");
            nemobus_msg_set_attr(msg, "from", view->objpath);
            nemobus_msg_set_attr(msg, "filename", view->filename);
            nemobus_msg_set_attr_format(msg, "isplaying", "%s",
                    view->is_playing ? "true" : "false");
            nemobus_msg_set_attr_format(msg, "ismute", "%s",
                    view->is_mute ? "true" : "false");
            nemobus_msg_set_attr_format(msg, "volume", "%d", view->vol_cnt);
            nemobus_msg_set_attr(msg, "from", view->objpath);
            nemobus_send(view->bus, view->objpath, from, msg);
            nemobus_msg_destroy(msg);
        }
	}
	nemoitem_destroy(it);
}

PlayerView *playerview_create(NemoWidget *parent, int width, int height, int vw, int vh, const char *path, bool is_audio, int repeat)
{
    struct nemotool *tool;
    struct nemoshow *show;
    PlayerView *view = calloc(sizeof(PlayerView), 1);
    view->tool = tool = nemowidget_get_tool(parent);
    view->show = show = nemowidget_get_show(parent);
    view->width = width;
    view->height = height;
    view->repeat = repeat;
    view->path = strdup(path);

    struct nemobus *bus;
    view->bus = bus = nemobus_create();
    if (!bus) {
        ERR("nemo bus create failed");
        free(view);
        return NULL;
    }
    view->objpath = strdup_printf("/%s/%s", APPNAME,
            nemowidget_get_uuid(parent));
    nemobus_connect(bus, NULL);
    nemobus_advertise(bus, "set", view->objpath);
	nemotool_watch_source(tool,
			nemobus_get_socket(bus),
			"reh",
            _bus_callback,
			view);

    struct nemoplay *play = nemoplay_create();
    nemoplay_load_media(play, path);
    if (!is_audio)
        nemoplay_revoke_audio(play);
    view->play = play;


    NemoWidget *win = nemowidget_get_top_widget(parent);
    if (nemoplay_get_video_framerate(play) <= 30) {
        nemowidget_set_framerate(win, 30);
    } else {
        nemowidget_set_framerate(win, nemoplay_get_video_framerate(play));
    }

    struct playshader *shader;
    view->shader = shader= nemoplay_shader_create();
    nemoplay_shader_prepare(shader,
            NEMOPLAY_TO_RGBA_VERTEX_SHADER,
            NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
    nemoplay_shader_set_texture(shader, vw, vh);

    struct showone *one;
    struct showone *group;
    struct nemotimer *timer;
    NemoWidget *widget;

    int video_gap = 2;

    view->frame = frame_create(parent, width, height, video_gap);
    int cw, ch;
    cw = view->frame->content_width;
    ch = view->frame->content_height;

    view->video = widget = nemowidget_create_opengl(parent, cw, ch);
    nemowidget_append_callback(widget, "event", _playerview_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_scale(widget, 0, 0, 0, 0.0, 0.0);
    nemowidget_append_callback(widget, "resize", _playerview_resize, view);
    frame_append_widget(view->frame, widget);

    view->video_timer = timer = TOOL_ADD_TIMER(tool, 0,
            _nemoplay_dispatch_video_timer, view);

    // Sketch
    view->frame_timer = TOOL_ADD_TIMER(view->tool, 0, _playerview_frame_timeout, view);
    view->sketch = sketch_create(parent, cw, ch);
    sketch_enable_blur(view->sketch, false);
    nemowidget_append_callback(view->sketch->widget, "event", _sketch_event, view);
    frame_append_widget_group(view->frame, view->sketch->widget,
            view->sketch->group);

    // Overlay
    view->overlay = widget = nemowidget_create_vector(parent, cw, ch);
    nemowidget_append_callback(widget, "event", _overlay_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    view->overlay_timer = TOOL_ADD_TIMER(view->tool, 0, _playerview_overlay_timeout, view);

    view->overlay_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    frame_append_widget_group(view->frame, widget, group);

    struct showone *top;
    view->top = top = GROUP_CREATE(group);

    int fontsize = height/30;

    // Top file name
    struct showone *blur;
    blur = BLUR_CREATE("solid", 1);
    view->text_blur = blur;

    view->filename = file_get_basename(path);

    Text *text;
    view->name_back = text = text_create(tool, top, "NanumGothic", "Regular", fontsize);
    text_set_stroke_color(text, 0, 0, 0, COLORBACK, 1);
    text_set_fill_color(text, 0, 0, 0, COLORBACK);
    text_set_filter(text, blur);
    text_translate(text, 0, 0, 0, width/2 + 1, fontsize + video_gap + 1);
    text_update(text, 0, 0, 0, view->filename);
    text_show(text, 0, 0, 0);

    view->name = text = text_create(tool, top, "NanumGothic", "Regular", fontsize);
    text_set_fill_color(text, 0, 0, 0, WHITE);
    text_translate(text, 0, 0, 0, width/2, fontsize + video_gap);
    text_update(text, 0, 0, 0, view->filename);
    text_show(text, 0, 0, 0);


    struct showone *bottom;
    view->bottom = bottom = GROUP_CREATE(group);

    // Play bar
    int r, rx, ry;
    r = 30;
    rx = r * 2.0;
    ry = height - r * 2.0;
    one = CIRCLE_CREATE(bottom, r);
    nemoshow_item_set_fill_color(one, RGBA(COLORBACK));
    nemoshow_item_translate(one, rx, ry);
    view->ss_back = one;

    int ih, ixx;
    ih = r/3;
    ixx = width - (rx + r);

    one = PATH_CREATE(bottom);
    nemoshow_item_path_arc(one, 0, 0, r * 2, r * 2, -10, 20);
    nemoshow_item_path_translate(one, -r, -r);
    nemoshow_item_path_lineto(one, ixx, ih/2);
    nemoshow_item_path_cubicto(one, ixx + 5, ih/2, ixx + 5, -ih/2, ixx, -ih/2);
    nemoshow_item_path_close(one);
    nemoshow_item_set_fill_color(one, RGBA(COLORBACK));
    nemoshow_item_translate(one, rx, ry);
    view->playbar = one;

    one = SVG_PATH_CREATE(bottom, r/2, r/2, PLAYER_ICON_DIR"/stop.svg");
    nemoshow_item_set_fill_color(one, RGBA(COLOR0));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, rx, ry);
    view->ss0 = one;

    one = SVG_PATH_CREATE(bottom, r/2, r/2, PLAYER_ICON_DIR"/play.svg");
    nemoshow_item_set_fill_color(one, RGBA(COLOR0));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, rx, ry);
    nemoshow_item_set_alpha(one, 0.0);
    view->ss1 = one;

    // Inner play bar
    double rr;
    rr = r * 0.7;

    int bar_x, bar_y, bar_w, bar_h;
    bar_w = ixx - rr;
    bar_h = ih/3;
    bar_x = rx + rr;
    bar_y = ry - bar_h/2;

    view->progress_x = bar_x;
    view->progress_y = bar_y;

    GraphBarItem *it;
    GraphBar *bar;
    bar = graph_bar_create(tool, bottom, bar_w, bar_h, GRAPH_BAR_DIR_R);
    graph_bar_translate(bar, 0, 0, 0, bar_x, bar_y);
    view->progress = bar;

    it = graph_bar_create_item(bar);
    graph_bar_item_set_percent(it, 0.0);
    graph_bar_item_set_color(it, COLOR1, COLOR1);
    view->progress_it0 = it;

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, GRAY, GRAY);
    graph_bar_item_set_percent(it, 0.0);
    view->progress_it1 = it;

    graph_bar_show(bar, 0, 0, 0);

    view->progress_timer = timer = TOOL_ADD_TIMER(tool, 0,
            _nemoplay_dispatch_progress_timeout, view);

    one = RECT_CREATE(bottom, bar_w, bar_h * 3);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "player");
    nemoshow_one_set_tag(one, 100);
    nemoshow_item_set_fill_color(one, RGBA(RED));
    nemoshow_item_translate(one, bar_x, ry - bar_h * 3/2);
    nemoshow_item_set_alpha(one, 0.0);
    view->progress_event = one;

    struct showone *progress_gradient;
    view->progress_gradient = progress_gradient =
        LINEAR_GRADIENT_CREATE(0, 0, rr/3, rr/3);
    STOP_CREATE(progress_gradient, COLOR0, 0.0);
    STOP_CREATE(progress_gradient, COLOR1, 1.0);

    one = CIRCLE_CREATE(bottom, rr);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "player");
    nemoshow_one_set_tag(one, 1);
    nemoshow_item_set_stroke_color(one, RGBA(COLOR1));
    nemoshow_item_set_stroke_width(one, 5);
    nemoshow_item_set_shader(one, progress_gradient);
    nemoshow_item_translate(one, rx, ry);
    view->progress_circle = one;

    text = text_create(tool, bottom, "NanumGothic", "Regular", 10);
    text_set_stroke_color(text, 0, 0, 0, COLORBACK, 1);
    text_set_fill_color(text, 0, 0, 0, COLORBACK);
    text_set_filter(text, blur);
    text_set_anchor(text, 0.0, 0.0);
    text_update(text, 0, 0, 0, "00:00:00");
    text_translate(text, 0, 0, 0, rx + r + 1, ry - ih - 5 + 1);
    text_show(text, 0, 0, 0);
    view->cur_time_back = text;

    text = text_create(tool, bottom, "NanumGothic", "Regular", 10);
    text_set_fill_color(text, 0, 0, 0, WHITE);
    text_update(text, 0, 0, 0, "00:00:00");
    text_set_anchor(text, 0.0, 0.0);
    text_translate(text, 0, 0, 0, rx + r, ry - ih - 5);
    text_show(text, 0, 0, 0);
    view->cur_time = text;

    int32_t duration = nemoplay_get_duration(play);
    char buf[16];
    int hour, min, sec;
    parse_seconds_to_hms(duration, &hour, &min, &sec);
    snprintf(buf, 16, "%02d:%02d:%02d", hour, min, sec);
    view->duration = duration;

    text = text_create(tool, bottom, "NanumGothic", "Regular", 10);
    text_set_stroke_color(text, 0, 0, 0, COLORBACK, 1);
    text_set_fill_color(text, 0, 0, 0, COLORBACK);
    text_update(text, 0, 0, 0, buf);
    text_set_anchor(text, 1.0, 0.0);
    text_translate(text, 0, 0, 0, bar_x + bar_w + 1, ry - ih - 5 + 1);
    text_show(text, 0, 0, 0);
    view->total_time_back = text;

    text = text_create(tool, bottom, "NanumGothic", "Regular", 10);
    text_set_fill_color(text, 0, 0, 0, WHITE);
    text_update(text, 0, 0, 0, buf);
    text_set_anchor(text, 1.0, 0.0);
    text_translate(text, 0, 0, 0, bar_x + bar_w, ry - ih - 5);
    text_show(text, 0, 0, 0);
    view->total_time = text;

    int ww, hh;
    ww = 30;
    hh = 30;
    int gap, pos_y;
    // FR, FF, PF, NF
    gap = 4;
    pos_y = height - hh - video_gap;

    Button *btn;
    btn = button_create(bottom, "player", 2);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/fr.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, width/2 - ww/2 - gap/2, pos_y);
    view->fr = btn;

    btn = button_create(bottom, "player", 3);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/ff.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, width/2 + ww/2 + gap/2, pos_y);
    view->ff = btn;

    btn = button_create(bottom, "player", 0);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/pf.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, GRAY);
    button_translate(btn, 0, 0, 0, width/2 - ww/2 - ww - gap, pos_y);
    view->pf = btn;

    btn = button_create(bottom, "player", 0);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/nf.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, GRAY);
    button_translate(btn, 0, 0, 0, width/2 + ww/2 + ww + gap, pos_y);
    view->nf = btn;

    // Voume
    btn = button_create(bottom, "player", 4);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/volume_up.svg", ww * 0.75, hh * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, bar_x + bar_w - ww/2, pos_y);
    view->vol_up = btn;

    int www, hhh;
    www = ww * 1.25;
    hhh = hh * 1.25;
    btn = button_create(bottom, "player", 5);
    button_enable_bg(btn, www/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/volume.svg", www * 0.75, www * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, bar_x + bar_w - ww/2 - ww * 1.5, pos_y);
    view->vol_mute = btn;

    view->vol_cnt = 50;
    nemosound_set_current_mute(view->tool, view->is_mute);
    nemosound_set_current_volume(view->tool, view->vol_cnt);

    one = ARC_CREATE(bottom, www - 4, hhh - 4, -90, -90);
    nemoshow_item_set_stroke_color(one, RGBA(COLOR1));
    nemoshow_item_set_stroke_width(one, 4);
    nemoshow_item_translate(one, bar_x + bar_w - ww/2 - ww * 1.5, pos_y);
    view->vol = one;

    btn = button_create(bottom, "player", 6);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, PLAYER_ICON_DIR"/volume_down.svg", ww * 0.75, hh * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, bar_x + bar_w - ww/2 - ww * 3, pos_y);
    view->vol_down = btn;

    r = 20;
    DrawingBox *dbox;
    view->dbox = dbox = drawingbox_create(group, r);
    drawingbox_translate(dbox, 0, 0, 0, bar_x + bar_w - ww/2, r * 2);

    return view;
}

static void playerview_show(PlayerView *view)
{
    frame_show(view->frame);
    frame_content_show(view->frame, -1);

    button_show(view->fr, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->ff, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->pf, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->nf, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->vol_up, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->vol_mute, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_show(view->vol_down, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            "to", (360/100.0) * view->vol_cnt - 90.0, NULL);
    playerview_overlay_show(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 400);

    sketch_show(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    drawingbox_show(view->dbox);

    view->is_playing = true;
    nemotimer_set_timeout(view->video_timer, 10);
    nemotimer_set_timeout(view->progress_timer, 10);

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

static void playerview_hide(PlayerView *view)
{
    nemoplay_set_state(view->play, NEMOPLAY_DONE_STATE);
    nemoplay_wait_thread(view->play);
    pthread_cancel(view->pth_decoder);
    pthread_cancel(view->pth_audioplayer);

    view->is_playing = false;
    nemotimer_set_timeout(view->progress_timer, 0);
    nemotimer_set_timeout(view->video_timer, 0);
    nemoplay_set_state(view->play, NEMOPLAY_STOP_STATE);

    playerview_overlay_hide(view, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    sketch_hide(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    drawingbox_hide(view->dbox);

    frame_hide(view->frame);
    frame_content_hide(view->frame, -1);
    nemoshow_dispatch_frame(view->show);
}

static void playerview_destroy(PlayerView *view)
{
    nemoplay_shader_destroy(view->shader);
    nemoplay_destroy(view->play);
    nemotimer_destroy(view->video_timer);
    nemotimer_destroy(view->progress_timer);

    // XXX: remove from frame to destroy
    frame_remove_widget(view->frame, view->sketch->widget);
    sketch_destroy(view->sketch);

    nemotimer_destroy(view->overlay_timer);
    drawingbox_destroy(view->dbox);

    button_destroy(view->fr);
    button_destroy(view->ff);
    button_destroy(view->pf);
    button_destroy(view->nf);
    button_destroy(view->vol_up);
    button_destroy(view->vol_mute);
    button_destroy(view->vol_down);

    // XXX: Do not destroy view->video, view->overlay
    // it will be destroyed automatically by frame_destroy
    frame_destroy(view->frame);

    nemobus_destroy(view->bus);
    free(view->filename);
    free(view->objpath);
    free(view);
}

static void _win_fullscreen_callback(NemoWidget *win, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Fullscreen *fs = info;
    PlayerView *view = userdata;

	if (fs->id) {
        /*
        nemoenvs_send(view->envs,
                "%s /nemoshell set /nemolink/control id %s src %s type video",
                nemoenvs_get_name(view->envs),
                id,
                nemoenvs_get_name(view->envs));
                */
        if (view->screenid) free(view->screenid);
        view->screenid = strdup(id);
        frame_go_full(view->frame, fs->width, fs->height);
    } else {
        /*
		nemoenvs_send(view->envs,
                "%s /nemoshell put /nemolink/control id %s",
				nemoenvs_get_name(view->envs),
				view->screenid);
                */
        free(view->screenid);
        view->screenid = NULL;
        frame_go_normal(view->frame);
    }

	if (nemoplay_get_frame(view->play) != 0) {
		nemoplay_shader_dispatch(view->shader);
	}
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    PlayerView *view = userdata;

    playerview_hide(view);

    nemowidget_win_exit_after(win, 500);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *path;
    bool is_audio;
    int repeat;
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
    if (xml) {
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/play", appname);
        temp = xml_get_value(xml, buf, "repeat");
        if (temp && strlen(temp) > 0) {
            app->repeat = atoi(temp);
        }
        xml_unload(xml);
    }

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        {"repeat", required_argument, NULL, 'p'},
        {"audio", required_argument, NULL, 'a'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:a:p:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                app->path = strdup(optarg);
                break;
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
    if (app->path) free(app->path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->path) {
        ERR("Usage: %s -f FILENAME [-a off] [-r -1/0/1]", APPNAME);
        return -1;
    }

    int vw, vh;
    if (!nemoplay_get_video_info(app->path, &vw, &vh)) {
        ERR("nemoplay_get_video_info failed: %s", app->path);
        return -1;
    }
    _rect_ratio_fit(vw, vh, app->config->width, app->config->height,
            &(app->config->width), &(app->config->height));

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_enable_fullscreen(win, true);

    PlayerView *view = playerview_create(win, app->config->width, app->config->height, vw, vh,
            app->path, app->is_audio, app->repeat);
    nemowidget_append_callback(win, "fullscreen", _win_fullscreen_callback, view);
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
