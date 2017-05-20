#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"
#include "nemoui.h"

typedef struct _Context Context;
struct _Context {
    struct nemoshow *show;
    Plexus *plexus;
    struct nemotimer *timer;
};

int g_idx = 0;
static void _timeout(struct nemotimer *timer, void *userdata)
{
    Context *ctx = userdata;

    if (g_idx == 0) {
        ERR("left top");
        plexus_translate(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 0, 0);
        plexus_set_r(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 50, 40);
        //plexus_set_wh(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 100, 100);
    } else if (g_idx == 1) {
        ERR("right top");
        plexus_translate(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 400, 0);
        plexus_set_r(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 50, 40);
        //plexus_set_wh(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 100, 100);
    } else if (g_idx == 2) {
        ERR("right bottom");
        plexus_translate(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 400, 400);
        plexus_set_r(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 50, 40);
        //plexus_set_wh(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 100, 100);
    } else if (g_idx == 3) {
        ERR("left bottom");
        plexus_translate(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 0, 400);
        plexus_set_r(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 50, 40);
        //plexus_set_wh(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 100, 100);
    } else {
        ERR("FULL");
        plexus_translate(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 0, 0);
        plexus_set_r(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 256, 200);
        //plexus_set_wh(ctx->plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0, 512, 512);
    }
    g_idx++;
    if (g_idx > 4) g_idx = 0;
    nemotimer_set_timeout(timer, 2200);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    struct nemoshow *show = ctx->show;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(show);

    plexus_hide(ctx->plexus, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    Context *ctx = calloc(sizeof(Context), 1);

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_config(tool, APPNAME, base);

    nemowidget_append_callback(win, "exit", _win_exit, ctx);
    ctx->show = nemowidget_get_show(win);
    nemowidget_show(win, 0, 0, 0);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, base->width, base->height);
    nemowidget_show(vector, 0, 0, 0);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(vector);

    Plexus *plexus;
    plexus = plexus_create(nemowidget_get_tool(vector), canvas, 512, 512, 2, 8);
    ctx->plexus = plexus;
    plexus_show(plexus, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0);

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
