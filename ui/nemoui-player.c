#include <nemotool.h>
#include <nemoshow.h>

#include <ao/ao.h>
#include <nemoplay.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "nemoui-player.h"
#include "playback.h"

struct _PlayerUI {
    struct nemotool *tool;
    struct nemoshow *show;
    int vw, vh;
    int w, h;
    double sx, sy;
    struct nemoplay *play;
    NemoWidget *widget;

    int duration;
    bool fin;
    bool need_stop;
    bool is_playing;

    struct playdecoder *decoder;
    struct playaudio *audio;
    struct playvideo *video;
};

static void _player_resize(NemoWidget *widget, const char *id, void *info, void *data)
{
    PlayerUI *ui = data;

    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

	nemoplay_video_set_texture(ui->video,
			nemoshow_canvas_get_texture(canvas),
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

    nemowidget_dirty(widget);
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
	nemoplay_video_destroy(ui->video);
	nemoplay_audio_destroy(ui->audio);
	nemoplay_decoder_destroy(ui->decoder);

	nemoplay_set_state(ui->play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(ui->play);
    nemoplay_destroy(ui->play);
    free(ui);
}

double nemoui_player_get_cts(PlayerUI *ui)
{
    return nemoplay_get_cts(ui->play);
}

double nemoui_player_get_duration(PlayerUI *ui)
{
    return ui->duration;
}

bool nemoui_player_is_playing(PlayerUI *ui)
{
    return ui->is_playing;
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

    // TODO: fix scale resize timing for smoothing scaling
    nemowidget_scale(ui->widget, 0, 0, 0, 1.0, 1.0);
    nemowidget_resize(ui->widget, ui->w * ui->sx, ui->h * ui->sy);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_scale(PlayerUI *ui, uint32_t easetype, int duration, int delay, float sx, float sy)
{
    if (EQUAL(ui->sx, sx) && EQUAL(ui->sy, sy)) return;
    ui->sx = sx;
    ui->sy = sy;

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

void nemoui_player_play(PlayerUI *ui)
{
    struct nemoplay *play = ui->play;

    ui->is_playing = true;
    if (ui->fin) {
        nemoplay_decoder_seek(ui->decoder, 0.0f);
        ui->fin = false;
    } else {
    }
    nemoplay_set_state(play, NEMOPLAY_PLAY_STATE);

    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_stop(PlayerUI *ui)
{
    ui->is_playing = false;
    nemoplay_set_state(ui->play, NEMOPLAY_STOP_STATE);

    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_seek(PlayerUI *ui, double time)
{
    nemoplay_decoder_seek(ui->decoder, time);
    if (!ui->is_playing) {
        ui->need_stop = true;
        nemoplay_set_state(ui->play, NEMOPLAY_PLAY_STATE);
    }
    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

static void _video_update(struct nemoplay *play, void *data)
{
    PlayerUI *ui = data;

    if (ui->need_stop) {
        ui->need_stop = false;
        nemoplay_set_state(ui->play, NEMOPLAY_STOP_STATE);
    }

    nemowidget_callback_dispatch(ui->widget, "player,update", NULL);

    nemowidget_dirty(ui->widget);
	nemoshow_dispatch_frame(ui->show);
}

static void _video_done(struct nemoplay *play, void *data)
{
    PlayerUI *ui = data;

    ui->fin = true;

    nemowidget_callback_dispatch(ui->widget, "player,done", NULL);

    nemowidget_dirty(ui->widget);
	nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_redraw(PlayerUI *ui)
{
    nemoplay_video_redraw(ui->video);
}

void nemoui_player_append_callback(PlayerUI *ui, const char *id, NemoWidget_Func func, void *userdata)
{
    nemowidget_append_callback(ui->widget, id, func, userdata);
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
    ui->sx = 1.0;
    ui->sy = 1.0;

    int w, h;
    _rect_ratio_fit(vw, vh, cw, ch, &w, &h);
    ui->vw = vw;
    ui->vh = vh;

    struct nemoplay *play;
    ui->play = play = nemoplay_create();

    // XXX: it's state is PLAY as default
    ui->is_playing = true;
    nemoplay_load_media(play, path);
    if (!enable_audio) nemoplay_revoke_audio(play);
    ui->duration = nemoplay_get_duration(play);

    if (nemoplay_get_video_framerate(play) <= 30) {
        nemowidget_set_framerate(parent, 30);
    } else {
        nemowidget_set_framerate(parent, nemoplay_get_video_framerate(play));
    }

    NemoWidget *widget;
    ui->widget = widget = nemowidget_create_opengl(parent, w, h);
    nemowidget_call_register(widget, "player,update");
    nemowidget_call_register(widget, "player,done");
    nemowidget_append_callback(widget, "resize", _player_resize, ui);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct playvideo *video;
    ui->decoder = nemoplay_decoder_create(play);
    ui->audio = nemoplay_audio_create_by_ao(play);
    ui->video = video = nemoplay_video_create_by_timer(play, ui->tool);
	nemoplay_video_set_texture(video, nemowidget_get_texture(widget), vw, vh);
	nemoplay_video_set_update(video, _video_update);
	nemoplay_video_set_done(video, _video_done);
	nemoplay_video_set_data(video, ui);

    return ui;
}
