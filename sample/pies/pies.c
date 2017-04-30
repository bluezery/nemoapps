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
    GraphPies *pies;
    struct nemotimer *timer;
};

static void _timeout(struct nemotimer *timer, void *userdata)
{
    ERR("");
    GraphPies *pies = userdata;

    List *l;
    GraphPiesItem *it;
    LIST_FOR_EACH(pies->items, l, it) {
        GraphPie *pie = graph_pies_item_get_pie(it);

        double tot = 1.0;
        List *ll;
        GraphPieItem *itt;
        LIST_FOR_EACH(pie->items, ll, itt) {
            double r = ((double)rand()/RAND_MAX) * tot;
            tot -= r;
            graph_pie_item_set_percent(itt, r);
        }
    }
    graph_pies_update(pies, NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
    nemotimer_set_timeout(timer, 2000);
}

static void _vector_event(NemoWidget *win, const char *id, void *event, void *userdata)
{
    Context *ctx = userdata;
    if (graph_pies_is(ctx->pies,
                nemoshow_event_get_x(event), nemoshow_event_get_y(event)))
        ERR("downed");
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    GraphPies *pies = ctx->pies;

    nemotimer_destroy(ctx->timer);
    _nemoshow_destroy_transition_all(pies->show);

    graph_pies_hide(pies, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

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

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, base->width, base->height);
    nemowidget_append_callback(vector, "event", _vector_event, ctx);
    nemowidget_show(vector, 0, 0, 0);

    GraphPies *pies = graph_pies_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), 500, 500, 100);
    graph_pies_set_blur(pies, "solid", 5);
    graph_pies_set_gap(pies, 10);
    ctx->pies = pies;

    GraphPie *pie;
    GraphPieItem *itt;
    GraphPiesItem *it;

    it = graph_pies_create_item(pies);
    pie = graph_pies_item_get_pie(it);
    graph_pie_set_gap(pie, 2);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, RED, PINK);
    graph_pie_item_set_percent(itt, 0.3);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, BLUE, CYAN);
    graph_pie_item_set_percent(itt, 0.2);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, GREEN, ORANGE);
    graph_pie_item_set_percent(itt, 0.3);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, WHITE, BLACK);
    graph_pie_item_set_percent(itt, 0.1);

    it = graph_pies_create_item(pies);
    pie = graph_pies_item_get_pie(it);
    graph_pie_set_gap(pie, 2);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, GREEN, LIGHTGREEN);
    graph_pie_item_set_percent(itt, 0.3);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, RED, LIGHTRED);
    graph_pie_item_set_percent(itt, 0.1);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, BLUE, LIGHTBLUE);
    graph_pie_item_set_percent(itt, 0.25);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, ORANGE, LIGHTORANGE);
    graph_pie_item_set_percent(itt, 0.1);

    it = graph_pies_create_item(pies);
    pie = graph_pies_item_get_pie(it);
    graph_pie_set_gap(pie, 2);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, BLUE, LIGHTBLUE);
    graph_pie_item_set_percent(itt, 0.1);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, ORANGE, CYAN);
    graph_pie_item_set_percent(itt, 0.1);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, GREEN, LIGHTGREEN);
    graph_pie_item_set_percent(itt, 0.2);

    itt = graph_pie_create_item(pie);
    graph_pie_item_set_color(itt, RED, PINK);
    graph_pie_item_set_percent(itt, 0.15);

    graph_pies_translate(pies, 0, 0, 0, 250, 250);
    graph_pies_show(pies, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(vector));
    nemotimer_set_timeout(timer, 3000);
    nemotimer_set_callback(timer, _timeout);
    nemotimer_set_userdata(timer, pies);
    ctx->timer = timer;

    nemotool_run(tool);

    graph_pies_destroy(pies);
    nemowidget_destroy(vector);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    free(ctx);

    return 0;
}
