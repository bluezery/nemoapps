#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "util.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"
#include "view.h"

typedef struct _Context Context;
struct _Context {
    struct nemoshow *show;
    struct showone *circle;
    struct showone *stop0, *stop1, *stop2, *stop3, *stop4, *stop5, *stop6, *stop7;
    struct nemotimer *timer;
};

int idx = 0;

static void _timeout(struct nemotimer *timer, void *userdata)
{
    Context *ctx = userdata;

    if (idx == 0) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 0.0,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 0.0,
                NULL);
    } else if (idx == 1) {
        _nemoshow_item_motion(ctx->stop3, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 0.0,
                NULL);
        _nemoshow_item_motion(ctx->stop4, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 0.0,
                NULL);
    } else if (idx == 2) {
        _nemoshow_item_motion(ctx->stop5, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 0.0,
                NULL);
        _nemoshow_item_motion(ctx->stop6, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 0.0,
                NULL);
    } else if (idx == 3) {
        _nemoshow_item_motion(ctx->stop5, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 1.0,
                NULL);
        _nemoshow_item_motion(ctx->stop6, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 1.0,
                NULL);
    } else if (idx == 4) {
        _nemoshow_item_motion(ctx->stop3, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 1.0,
                NULL);
        _nemoshow_item_motion(ctx->stop4, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 1.0,
                NULL);
    } else if (idx == 5) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "offset", 1.0,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "offset", 1.0,
                NULL);
    }

    idx++;
    if (idx > 5) idx = 0;

    nemotimer_set_timeout(timer, 3000);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    struct nemoshow *show = ctx->show;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(show);

    // hide
    //spinfx_hide(ctx->spinfx, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);

    nemowidget_win_exit_if_no_trans(win);
}

#if 0
static void _timeout2(struct nemotimer *timer, void *userdata)
{
    Context *ctx = userdata;

    if (idx == 0) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", LIGHTBLUE,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", BLUE,
                NULL);
    } else if (idx == 1) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", LIGHTGREEN,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", GREEN,
                NULL);
    } else if (idx == 2) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", LIGHTYELLOW,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", YELLOW ,
                NULL);
    } else if (idx == 3) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", LIGHTGREEN,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", GREEN,
                NULL);
    } else if (idx == 4) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", LIGHTYELLOW,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", YELLOW,
                NULL);
    } else if (idx == 5) {
        _nemoshow_item_motion(ctx->stop1, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 500,
                "fill", LIGHTBLUE,
                NULL);
        _nemoshow_item_motion(ctx->stop2, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0,
                "fill", BLUE,
                NULL);
    }

    idx++;
    if (idx > 5) idx = 0;

    nemotimer_set_timeout(timer, 3000);
}
#endif


int main(int argc, char *argv[])
{
    Context *ctx = calloc(sizeof(Context), 1);

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_base(tool, APPNAME, base);
    nemowidget_append_callback(win, "exit", _win_exit, ctx);
    ctx->show = nemowidget_get_show(win);
    nemowidget_show(win, 0, 0, 0);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, base->width, base->height);
    nemowidget_show(vector, 0, 0, 0);

    struct showone *emboss;
    emboss = EMBOSS_CREATE(1, 1, 1, 128, 32, 12);

    struct showone *circle;
    circle = ARC_CREATE(nemowidget_get_canvas(vector), 400, 400, 0, 360);
    nemoshow_item_set_stroke_color(circle, RGBA(GREEN));
    nemoshow_item_set_stroke_width(circle, 50);
    nemoshow_item_set_fill_color(circle, RGBA(0x0));
    nemoshow_item_set_filter(circle, emboss);
    nemoshow_item_translate(circle, 256, 256);

    struct showone *shader;
    struct showone *stop;
    shader = LINEAR_GRADIENT_CREATE(0.0, 0.0, 500, 500);
    nemoshow_item_set_shader(circle, shader);

    stop = STOP_CREATE(shader, LIGHTRED, 0.0);
    ctx->stop0 = stop;

    stop = STOP_CREATE(shader, RED, 1.0);
    ctx->stop1 = stop;

    stop = STOP_CREATE(shader, LIGHTBLUE, 1.0);
    ctx->stop2 = stop;

    stop = STOP_CREATE(shader, BLUE, 1.0);
    ctx->stop3 = stop;

    stop = STOP_CREATE(shader, LIGHTGREEN, 1.0);
    ctx->stop4 = stop;

    stop = STOP_CREATE(shader, GREEN, 1.0);

    stop = STOP_CREATE(shader, LIGHTYELLOW, 1.0);
    ctx->stop6 = stop;

    stop = STOP_CREATE(shader, YELLOW, 1.0);
    ctx->stop7 = stop;

    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(vector));
    nemotimer_set_timeout(timer, 2200);
    nemotimer_set_callback(timer, _timeout);
    nemotimer_set_userdata(timer, ctx);
    ctx->timer = timer;

    nemotool_run(tool);

    nemowidget_destroy(vector);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    free(ctx);

    return 0;
}
