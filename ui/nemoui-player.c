#include <nemotool.h>
#include <nemoshow.h>

#include <ao/ao.h>
#include <nemoplay.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemohelper.h"
#include "nemoui.h"
#include "nemoui-player.h"

// TODO: Retry when network is disconnected!
// TODO: Create thread to call avformat_open_input()!!
// For network stream, it takes long times.
int _nemoplay_load_media(struct nemoplay *play, const char *mediapath)
{
	AVFormatContext *container;
	AVCodecContext *context;
	AVCodec *codec;
	AVCodecContext *video_context = NULL;
	AVCodecContext *audio_context = NULL;
	AVCodecContext *subtitle_context = NULL;
	int video_stream = -1;
	int audio_stream = -1;
	int subtitle_stream = -1;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return -1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto err1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto err1;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			video_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
			audio_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			subtitle_stream = i;
			subtitle_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		}
	}

	video_stream = av_find_best_stream(container, AVMEDIA_TYPE_VIDEO, video_stream, -1, NULL, 0);
	audio_stream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, audio_stream, video_stream, NULL, 0);
	subtitle_stream = av_find_best_stream(container, AVMEDIA_TYPE_SUBTITLE, subtitle_stream, audio_stream >= 0 ? audio_stream : video_stream, NULL, 0);

	if (video_context != NULL) {
		play->video_width = video_context->width;
		play->video_height = video_context->height;
		play->video_timebase = av_q2d(container->streams[video_stream]->time_base);
	}

	if (audio_context != NULL) {
		SwrContext *swr;

		play->audio_channels = audio_context->channels;
		play->audio_samplerate = audio_context->sample_rate;
		play->audio_samplebits = 16;
		play->audio_timebase = av_q2d(container->streams[audio_stream]->time_base);

		swr = swr_alloc();
		av_opt_set_int(swr, "in_channel_layout", audio_context->channel_layout, 0);
		av_opt_set_int(swr, "out_channel_layout", audio_context->channel_layout, 0);
		av_opt_set_int(swr, "in_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_int(swr, "out_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_sample_fmt(swr, "in_sample_fmt", audio_context->sample_fmt, 0);
		av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		swr_init(swr);

		play->swr = swr;
	}

	play->container = container;

	play->duration = container->duration / AV_TIME_BASE;

	play->video_context = video_context;
	play->audio_context = audio_context;
	play->subtitle_context = subtitle_context;
	play->video_stream = video_stream;
	play->audio_stream = audio_stream;
	play->subtitle_stream = subtitle_stream;

	play->video_framerate = av_q2d(av_guess_frame_rate(container, container->streams[video_stream], NULL));

	play->state = NEMOPLAY_PLAY_STATE;
	play->cmd = 0x0;
	play->frame = 0;

	return 0;

err1:
	avformat_close_input(&container);

	return -1;
}

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

void nemoui_player_play(PlayerUI *ui)
{
    struct nemoplay *play = ui->play;

    nemotimer_set_timeout(ui->video_timer, 10);

    pthread_t pth;
    if (!ui->pth_decode) {
        if (pthread_create(&pth, NULL, _player_decode_thread, (void *)ui) != 0) {
            ERR("pthread frame decoder create failed");
        } else
            ui->pth_decode = pth;
    }

    if (!ui->pth_audio) {
        if (pthread_create(&pth, NULL, _player_audio_thread, (void *)ui) != 0) {
            ERR("pthread audio handler create failed");
        } else
            ui->pth_audio = pth;
    }
    nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);
}

void nemoui_player_stop(PlayerUI *ui)
{
    nemotimer_set_timeout(ui->video_timer, 0);
    nemoplay_set_state(ui->play, NEMOPLAY_STOP_STATE);

    // TODO: refactoring thread
    /*
    nemoplay_wait_thread(ui->play);
    if (ui->pth_decode) pthread_cancel(ui->pth_decode);
    if (ui->pth_audio) pthread_cancel(ui->pth_audio);
    */

}

void nemoui_player_show(PlayerUI *ui, uint32_t easetype, int duration, int delay)
{
    nemoui_player_play(ui);
    nemowidget_show(ui->widget, 0, 0, 0);
    nemowidget_set_alpha(ui->widget, easetype, duration, delay, 1.0);
}

void nemoui_player_hide(PlayerUI *ui, uint32_t easetype, int duration, int delay)
{
    nemoui_player_stop(ui);
    nemowidget_hide(ui->widget, 0, 0, 0);
    nemowidget_set_alpha(ui->widget, easetype, duration, delay, 0.0);
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

void nemoui_player_below(PlayerUI *ui, NemoWidget *below)
{
    nemowidget_stack_below(ui->widget, below);
}

static void _player_scale_done(NemoMotion *m, void *userdata)
{
    PlayerUI *ui = userdata;


    struct showone *canvas;
    canvas = nemowidget_get_canvas(ui->widget);
    double sx, sy;
    sx = NEMOSHOW_CANVAS_AT(canvas, sx);
    sy = NEMOSHOW_CANVAS_AT(canvas, sy);
    nemowidget_resize(ui->widget, ui->w * sx, ui->h * sy);
    nemowidget_scale(ui->widget, 0, 0, 0, 1.0, 1.0);
}

void nemoui_player_scale(PlayerUI *ui, uint32_t easetype, int duration, int delay, float sx, float sy)
{
    struct showone *canvas = nemowidget_get_canvas(ui->widget);
    struct nemoshow *show = nemowidget_get_show(ui->widget);

    if (duration > 0) {
        NemoMotion *m = nemomotion_create(show, easetype, duration, delay);
        nemomotion_set_done_callback(m, _player_scale_done, ui);
        nemomotion_attach(m, 1.0,
                canvas, "sx", sx, canvas, "sy", sy,
                NULL);
        nemomotion_run(m);
    } else {
        ui->w = ui->w * sx;
        ui->h = ui->h * sy;
        nemowidget_resize(ui->widget, ui->w, ui->h);
    }
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
