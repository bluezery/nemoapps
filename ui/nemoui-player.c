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
    char *path;
    int vw, vh;
    int cw, ch;
    int w, h;
    double sx, sy;
    struct nemoplay *play;
    NemoWidget *widget;

    int repeat;
    bool fin;
    bool need_stop;
    bool is_playing;
    bool no_drop;

    struct playdecoder *decoder;
    struct playaudio *audio;
    struct playvideo *video;
    bool prepared;
};

static void _player_resize(NemoWidget *widget, const char *id, void *info, void *data)
{
    PlayerUI *ui = data;
    RET_IF(!ui->video);
    RET_IF(!ui->prepared);

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
    nemowidget_show(ui->widget, 0, 0, 0);
    nemowidget_set_alpha(ui->widget, easetype, duration, delay, 1.0);
}

void nemoui_player_hide(PlayerUI *ui, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(ui->widget, 0, 0, 0);
    nemowidget_set_alpha(ui->widget, easetype, duration, delay, 0.0);
}

void nemoui_player_destroy(PlayerUI *ui)
{
    free(ui->path);
	if (ui->video) nemoplay_video_destroy(ui->video);
	if (ui->audio) nemoplay_audio_destroy(ui->audio);
	if (ui->decoder) nemoplay_decoder_destroy(ui->decoder);
    nemoplay_destroy(ui->play);
    free(ui);
}

const char *nemoui_player_get_uri(PlayerUI *ui)
{
    return ui->path;
}

bool nemoui_player_is_prepared(PlayerUI *ui)
{
    return ui->prepared;
}

double nemoui_player_get_cts(PlayerUI *ui)
{
    return nemoplay_get_clock_cts(ui->play);
}

double nemoui_player_get_duration(PlayerUI *ui)
{
    return nemoplay_get_duration(ui->play);
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
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_revoke_translate(PlayerUI *ui)
{
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "tx");
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "ty");
}

void nemoui_player_revoke_scale(PlayerUI *ui)
{
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "sx");
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "sy");
}

void nemoui_player_revoke_rotate(PlayerUI *ui)
{
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "ro");
}

void nemoui_player_revoke_alpha(PlayerUI *ui)
{
    nemoshow_revoke_transition_one(ui->show, nemowidget_get_canvas(ui->widget), "alpha");
}

void nemoui_player_translate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float x, float y)
{
    nemowidget_translate(ui->widget, easetype, duration, delay, x, y);
}

void nemoui_player_rotate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float ro)
{
    // Not supported yet
    //nemowidget_rotate(ui->widget, easetype, duration, delay, ro);
}

static void _video_update(struct nemoplay *play, void *data)
{
    PlayerUI *ui = data;

    if (ui->need_stop) {
        ui->need_stop = false;
        nemoplay_audio_stop(ui->audio);
        nemoplay_video_stop(ui->video);
        nemoplay_decoder_stop(ui->decoder);
    }

    nemowidget_callback_dispatch(ui->widget, "player,update", NULL);

    nemowidget_dirty(ui->widget);
	nemoshow_dispatch_frame(ui->show);
}

static void _video_done(struct nemoplay *play, void *data)
{
    PlayerUI *ui = data;

    ui->fin = true;
    if (ui->repeat != 0) {
        if (ui->repeat > 0) ui->repeat--;

        nemoui_player_play(ui);
    } else {
        nemowidget_callback_dispatch(ui->widget, "player,done", NULL);

    }
    nemowidget_dirty(ui->widget);
	nemoshow_dispatch_frame(ui->show);
}

// This is thread safe function
void nemoui_player_prepare(PlayerUI *ui)
{
    RET_IF(ui->prepared);

    if (nemoplay_load_media(ui->play, ui->path) < 0) {
        ERR("nemoplay load media failed: %s", ui->path);
        return;
    }
    ui->prepared = true;
}

void nemoui_player_play(PlayerUI *ui)
{
    struct nemoplay *play = ui->play;

    if (!ui->prepared) {
        if (nemoplay_load_media(play, ui->path) < 0) {
            ERR("nemoplay load media failed: %s", ui->path);
            return;
        }
        ui->prepared = true;
    }

    if (!ui->video) {
        int vw, vh;
        vw = nemoplay_get_video_width(play);
        vh = nemoplay_get_video_height(play);
        _rect_ratio_fit(vw, vh, ui->cw, ui->ch, &ui->w, &ui->h);
        nemowidget_resize(ui->widget, ui->w, ui->h);

        if (nemoplay_get_video_framerate(play) <= 30) {
            nemowidget_set_framerate(ui->widget, 30);
        } else {
            nemowidget_set_framerate(ui->widget, nemoplay_get_video_framerate(play));
        }

        struct playvideo *video;
        ui->video = video = nemoplay_video_create_by_timer(play);
        if (ui->no_drop) nemoplay_video_set_drop_rate(video, 0.0);
        nemoplay_video_stop(ui->video);
        nemoplay_video_set_texture(video,
                nemowidget_get_texture(ui->widget),
                ui->w, ui->h);
        nemoplay_video_set_update(video, _video_update);
        nemoplay_video_set_done(video, _video_done);
        nemoplay_video_set_data(video, ui);
    }
    if (!ui->audio) {
        ui->audio = nemoplay_audio_create_by_ao(play);
        nemoplay_audio_stop(ui->audio);
    }
    if (!ui->decoder) {
        ui->decoder = nemoplay_decoder_create(play);
        nemoplay_decoder_stop(ui->decoder);
    }

    ui->is_playing = true;
    if (ui->fin) {
        nemoplay_decoder_seek(ui->decoder, 0.0f);
        ui->fin = false;
    } else {
    }
    nemoplay_audio_play(ui->audio);
    nemoplay_video_play(ui->video);
    nemoplay_decoder_play(ui->decoder);

    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_stop(PlayerUI *ui)
{
    RET_IF(!ui->audio || !ui->video || !ui->decoder)
    ui->is_playing = false;
    nemoplay_audio_stop(ui->audio);
    nemoplay_video_stop(ui->video);
    nemoplay_decoder_stop(ui->decoder);

    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_seek(PlayerUI *ui, double time)
{
    RET_IF(!ui->decoder)
    nemoplay_decoder_seek(ui->decoder, time);
    if (!ui->is_playing) {
        ui->need_stop = true;
        nemoplay_audio_play(ui->audio);
        nemoplay_video_play(ui->video);
        nemoplay_decoder_play(ui->decoder);
    }
    nemowidget_dirty(ui->widget);
    nemoshow_dispatch_frame(ui->show);
}

void nemoui_player_redraw(PlayerUI *ui)
{
    if (ui->video) nemoplay_video_redraw(ui->video);
}

void nemoui_player_append_callback(PlayerUI *ui, const char *id, NemoWidget_Func func, void *userdata)
{
    nemowidget_append_callback(ui->widget, id, func, userdata);
}

void nemoui_player_set_repeat(PlayerUI *ui, int repeat)
{
    ui->repeat = repeat;
}

PlayerUI *nemoui_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio, int num_threads, bool no_drop)
{
    RET_IF(cw <= 0 || ch <= 0, NULL);
    RET_IF(!path, NULL);

    PlayerUI *ui = calloc(sizeof(PlayerUI), 1);
    ui->tool = nemowidget_get_tool(parent);
    ui->show = nemowidget_get_show(parent);
    ui->path = strdup(path);
    ui->cw = cw;
    ui->ch = ch;
    ui->sx = 1.0;
    ui->sy = 1.0;
    ui->no_drop = no_drop;

    struct nemoplay *play;
    ui->play = play = nemoplay_create();
    if (!enable_audio) nemoplay_revoke_audio(play);
    if (num_threads > 0) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%d", num_threads);
        nemoplay_set_video_stropt(play, "threads", buf);
    }

    NemoWidget *widget;
    ui->widget = widget = nemowidget_create_opengl(parent, cw, ch);
    nemowidget_call_register(widget, "player,update");
    nemowidget_call_register(widget, "player,done");
    nemowidget_append_callback(widget, "resize", _player_resize, ui);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    return ui;
}
