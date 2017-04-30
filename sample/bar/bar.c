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
    GraphBar *bar0, *bar1, *bar2;
    struct nemotimer *timer;
};

static void _timeout(struct nemotimer *timer, void *userdata)
{
    Context *ctx = userdata;

    double tot = 1.0;

    List *l;
    GraphBarItem *it;
    LIST_FOR_EACH(ctx->bar0->items, l, it) {
        double r = ((double)rand()/RAND_MAX) * tot;
        tot -= r;
        graph_bar_item_set_percent(it, r);
    }
    tot = 1.0;
    LIST_FOR_EACH(ctx->bar1->items, l, it) {
        double r = ((double)rand()/RAND_MAX) * tot;
        tot -= r;
        graph_bar_item_set_percent(it, r);
    }
    tot = 1.0;
    LIST_FOR_EACH(ctx->bar2->items, l, it) {
        double r = ((double)rand()/RAND_MAX) * tot;
        tot -= r;
        graph_bar_item_set_percent(it, r);
    }
    graph_bar_update(ctx->bar0, NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
    graph_bar_update(ctx->bar1, NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
    graph_bar_update(ctx->bar2, NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
    nemotimer_set_timeout(timer, 1500);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    struct nemoshow *show = ctx->show;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(show);

    graph_bar_hide(ctx->bar0, NEMOEASE_CUBIC_OUT_TYPE, 1500, 0);
    graph_bar_hide(ctx->bar1, NEMOEASE_CUBIC_OUT_TYPE, 1500, 0);
    graph_bar_hide(ctx->bar2, NEMOEASE_CUBIC_OUT_TYPE, 1500, 0);

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

    // Bar0
    GraphBar *bar;
    bar = graph_bar_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), 100, 500, GRAPH_BAR_DIR_T);
    graph_bar_set_blur(bar, "solid", 5);
    graph_bar_set_gap(bar, 5);
    ctx->bar0 = bar;

    GraphBarItem *it;
    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, RED, PINK);
    graph_bar_item_set_percent(it, 0.5);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, BLUE, CYAN);
    graph_bar_item_set_percent(it, 0.33);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, GREEN, ORANGE);
    graph_bar_item_set_percent(it, 0.17);

    graph_bar_translate(bar, 0, 0, 0, 6, 6);
    graph_bar_show(bar, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    // Bar1
    bar = graph_bar_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), 100, 500, GRAPH_BAR_DIR_T);
    graph_bar_set_blur(bar, "solid", 5);
    graph_bar_set_gap(bar, 5);
    graph_bar_translate(bar, 0, 0, 0, 196, 6);
    ctx->bar1 = bar;

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, RED, PINK);
    graph_bar_item_set_percent(it, 0.2);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, BLUE, CYAN);
    graph_bar_item_set_percent(it, 0.7);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, GREEN, ORANGE);
    graph_bar_item_set_percent(it, 0.1);

    graph_bar_show(bar, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    // Bar2
    bar = graph_bar_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), 100, 500, GRAPH_BAR_DIR_T);
    graph_bar_set_blur(bar, "solid", 5);
    graph_bar_set_gap(bar, 5);
    graph_bar_translate(bar, 0, 0, 0, 386, 6);
    ctx->bar2 = bar;

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, RED, PINK);
    graph_bar_item_set_percent(it, 0.3);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, BLUE, CYAN);
    graph_bar_item_set_percent(it, 0.3);

    it = graph_bar_create_item(bar);
    graph_bar_item_set_color(it, GREEN, ORANGE);
    graph_bar_item_set_percent(it, 0.4);

    graph_bar_show(bar, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(vector));
    nemotimer_set_timeout(timer, 3000);
    nemotimer_set_callback(timer, _timeout);
    nemotimer_set_userdata(timer, ctx);
    ctx->timer = timer;

    nemotool_run(tool);

    graph_bar_destroy(bar);
    nemowidget_destroy(vector);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    free(ctx);

    return 0;
}
