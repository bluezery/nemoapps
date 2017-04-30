#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"
#include "nemoui.h"

typedef struct _Context Context;
struct _Context {
    struct nemoshow *show;
    SpinFx *spinfx;
    struct nemotimer *timer;
};

static void _timeout(struct nemotimer *timer, void *userdata)
{
    ERR("");
    Context *ctx = userdata;

    spinfx_show(ctx->spinfx, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);
    nemotimer_set_timeout(timer, 2200);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    struct nemoshow *show = ctx->show;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(show);

    // hide
    spinfx_hide(ctx->spinfx, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);

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

    SpinFx *spinfx;
    spinfx = spinfx_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), 500, 500, 50, 4, 2);
    spinfx_translate(spinfx, 0, 0, 0, 250, 250);
    spinfx_show(spinfx, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);
    ctx->spinfx = spinfx;

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
