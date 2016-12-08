#include <nemotool.h>
#include <nemoshow.h>

#include <ao/ao.h>
#include <nemoplay.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemohelper.h"
#include "nemoui.h"
#include "nemoui-player.h"

struct _PlayerUI {
    struct nemotool *tool;
    struct nemoshow *show;
    int vw, vh;
    int w, h;
    struct nemoplay *play;
    struct playshader *shader;
    NemoWidget *widget;
    struct nemotimer *video_timer;
    pthread_t pth_decode, pth_audio;
};

static void *_player_decode_thread(void *data)
{
    PlayerUI *ui = data;
    struct nemoplay *play = ui->play;
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

static void *_player_audio_thread(void *data)
{
    PlayerUI *ui = data;
	struct nemoplay *play = ui->play;
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

static void _player_video_timeout(struct nemotimer *timer, void *userdata)
{
    PlayerUI *ui = userdata;
    struct nemoplay *play = ui->play;
    struct playqueue *queue;
    struct playone *one;
    int state;

	queue = nemoplay_get_video_queue(play);

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {

#if 0
        double p0, p1;
        p0 = cts/(double)ui->duration;
        p1 = 1.0 - cts/(double)ui->duration;
        graph_bar_item_set_percent(ui->progress_it0, p0);
        graph_bar_item_set_percent(ui->progress_it1, p1);
        graph_bar_update(ui->progress, NEMOEASE_CUBIC_INOUT_TYPE, 100, 0);
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
				if (nemoplay_shader_get_texture_linesize(ui->shader) !=
                        nemoplay_queue_get_one_width(one))
					nemoplay_shader_set_texture_linesize(ui->shader,
                            nemoplay_queue_get_one_width(one));

				nemoplay_shader_update(ui->shader,
						nemoplay_queue_get_one_y(one),
						nemoplay_queue_get_one_u(one),
						nemoplay_queue_get_one_v(one));
				nemoplay_shader_dispatch(ui->shader);
                nemowidget_dirty(ui->widget);
                nemoshow_dispatch_frame(ui->show);

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
    PlayerUI *ui = data;

    if (!ui->shader) return;

	nemoplay_shader_set_viewport(ui->shader,
			nemowidget_get_texture(ui->widget),
			resize->width, resize->height);

	if (nemoplay_get_frame(ui->play) != 0)
		nemoplay_shader_dispatch(ui->shader);

    nemowidget_dirty(widget);
}

void nemoui_player_play(PlayerUI *ui, uint32_t easetype, int duration, int delay)
{
    struct nemoplay *play = ui->play;

    nemotimer_set_timeout(ui->video_timer, duration + delay);
    nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);

    nemotimer_set_timeout(ui->video_timer, duration + delay);

    pthread_t pth;
    if (pthread_create(&pth, NULL, _player_decode_thread, (void *)ui) != 0) {
        ERR("pthread frame decoder create failed");
    } else ui->pth_decode = pth;

    if (pthread_create(&pth, NULL, _player_audio_thread, (void *)ui) != 0) {
        ERR("pthread audio handler create failed");
        pthread_cancel(ui->pth_decode);
    } else ui->pth_audio = pth;
}

void nemoui_player_stop(PlayerUI *ui)
{
    nemotimer_set_timeout(ui->video_timer, 0);
    nemoplay_wait_thread(ui->play);
    pthread_cancel(ui->pth_decode);
    pthread_cancel(ui->pth_audio);

    nemotimer_set_timeout(ui->video_timer, 0);
    nemoplay_set_state(ui->play, NEMOPLAY_STOP_STATE);
}

void nemoui_player_show(PlayerUI *ui, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(ui->widget, 0, 0, 0);
    nemoui_player_play(ui, easetype, duration, delay);
}

void nemoui_player_hide(PlayerUI *ui)
{
    nemowidget_hide(ui->widget, 0, 0, 0);
    nemoui_player_stop(ui);
}

void nemoui_player_destroy(PlayerUI *ui)
{
    nemoplay_shader_destroy(ui->shader);
    nemoplay_destroy(ui->play);
    nemotimer_destroy(ui->video_timer);
    free(ui);
}

void nemoui_player_above(PlayerUI *ui, NemoWidget *above)
{
    nemowidget_stack_above(ui->widget, above);
}

void nemoui_player_scale(PlayerUI *ui, uint32_t easetype, int duration, int delay, float sx, float sy)
{
    nemowidget_scale(ui->widget, easetype, duration, delay, sx, sy);
}

void nemoui_player_translate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float x, float y)
{
    nemowidget_translate(ui->widget, easetype, duration, delay, x, y);
}

PlayerUI *nemoui_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio)
{
    int vw, vh;
    if (!nemoplay_get_video_info(path, &vw, &vh)) {
        ERR("nemoplay_get_video_info failed: %s", path);
        return NULL;
    }

    PlayerUI *ui = calloc(sizeof(PlayerUI), 1);
    ui->tool = nemowidget_get_tool(parent);
    ui->show = nemowidget_get_show(parent);
    ui->w = cw;
    ui->h = ch;

    int w, h;
    _rect_ratio_fit(vw, vh, cw, ch, &w, &h);
    ui->vw = vw;
    ui->vh = vh;

    struct nemoplay *play;
    ui->play = play = nemoplay_create();
    nemoplay_load_media(play, path);
    if (!enable_audio) nemoplay_revoke_audio(play);

    if (nemoplay_get_video_framerate(play) <= 30) {
        nemowidget_set_framerate(parent, 30);
    } else {
        nemowidget_set_framerate(parent, nemoplay_get_video_framerate(play));
    }

    struct playshader *shader;
    ui->shader = shader= nemoplay_shader_create();
    nemoplay_shader_prepare(shader,
            NEMOPLAY_TO_RGBA_VERTEX_SHADER,
            NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
    nemoplay_shader_set_texture(shader, vw, vh);

    NemoWidget *widget;
    ui->widget = widget = nemowidget_create_opengl(parent, w, h);
    nemowidget_append_callback(widget, "resize", _player_resize, ui);

    ui->video_timer = TOOL_ADD_TIMER(ui->tool, 0,  _player_video_timeout, ui);

    return ui;
}
