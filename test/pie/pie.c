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
#include "nemoui.h"

typedef struct _Context Context;
struct _Context {
    GraphPie *pie;
    struct nemotimer *timer;
};

static void _timeout(struct nemotimer *timer, void *userdata)
{
    ERR("");
    GraphPie *pie = userdata;

    double tot = 1.0;

    List *l;
    GraphPieItem *it;
    LIST_FOR_EACH(pie->items, l, it) {
        double r = ((double)rand()/RAND_MAX) * tot;
        tot -= r;
        graph_pie_item_set_percent(it, r);
    }
    graph_pie_update(pie, NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
    nemotimer_set_timeout(timer, 2000);
}

static void _vector_event(NemoWidget *win, const char *id, void *event, void *userdata)
{
    Context *ctx = userdata;
    if (graph_pie_is(ctx->pie, nemoshow_event_get_x(event), nemoshow_event_get_y(event)))
        ERR("downed");
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    GraphPie *pie = ctx->pie;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(pie->show);

    graph_pie_hide(pie, NEMOEASE_CUBIC_OUT_TYPE, 1500, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    Context *ctx = calloc(sizeof(Context), 1);

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_base(tool, APPNAME, base);
    nemowidget_append_callback(win, "exit", _win_exit, ctx);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, base->width, base->height);
    nemowidget_append_callback(vector, "event", _vector_event, ctx);
    nemowidget_show(vector, 0, 0, 0);

    GraphPie *pie = graph_pie_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector),
            500, 500, 50);
    graph_pie_set_blur(pie, "solid", 5);
    graph_pie_set_gap(pie, 1);
    ctx->pie = pie;

    GraphPieItem *it;
    it = graph_pie_create_item(pie);
    graph_pie_item_set_color(it, RED, PINK);
    graph_pie_item_set_percent(it, 0.2);

    it = graph_pie_create_item(pie);
    graph_pie_item_set_color(it, BLUE, CYAN);
    graph_pie_item_set_percent(it, 0.3);

    it = graph_pie_create_item(pie);
    graph_pie_item_set_color(it, GREEN, ORANGE);
    graph_pie_item_set_percent(it, 0.25);

    it = graph_pie_create_item(pie);
    graph_pie_item_set_color(it, PURPLE, BROWN);
    graph_pie_item_set_percent(it, 0.1);

    it = graph_pie_create_item(pie);
    graph_pie_item_set_color(it, WHITE, BLACK);
    graph_pie_item_set_percent(it, 0.15);

    graph_pie_translate(pie, 0, 0, 0, 256, 256);
    graph_pie_show(pie, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(vector));
    nemotimer_set_timeout(timer, 3000);
    nemotimer_set_callback(timer, _timeout);
    nemotimer_set_userdata(timer, pie);
    ctx->timer = timer;

    nemotool_run(tool);

    graph_pie_destroy(pie);
    nemowidget_destroy(vector);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    free(ctx);

    return 0;
}
