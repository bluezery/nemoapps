#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemoplay.h>

#include <nemosound.h>

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "nemoui-player.h"

#define TIMEOUT 2000

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
};

typedef struct _WindowItem WindowItem;
struct _WindowItem {
    const char *path;
    double x, y, w, h;
};

WindowItem items[] = {
    {APP_RES_DIR"/frame/center.mov", 1506, 315, 900, 1266},
    {APP_RES_DIR"/frame/lb.mov", 71, 1468, 1100, 620},
    {APP_RES_DIR"/frame/lt.mov", 460, 72, 710, 1090},
    {APP_RES_DIR"/frame/rb.mov", 2752, 1074, 1015, 1015},
    {APP_RES_DIR"/frame/rt.mov", 2463, 72, 945, 945},

};

typedef struct _WindowView WindowView;
struct _WindowView
{
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *parent;

    int width, height;

    NemoWidget *back;
    struct showone *back_one;

    ConfigApp *app;

    PlayerUI *back_open;
    PlayerUI *back_close;
    PlayerUI *back_frame;
    List *items;
};

static void _window_view_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    WindowView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        NemoWidget *win = nemowidget_get_top_widget(view->back);
        nemowidget_callback_dispatch(win, "exit", NULL);
    }
}

static void _player_done(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    WindowView *view = userdata;

    nemoui_player_show(view->back_frame, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    nemoui_player_play(view->back_frame);
    PlayerUI *it;
    List *l;
    LIST_FOR_EACH(view->items, l, it) {
        nemoui_player_show(it, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
        nemoui_player_play(it);
    }
}

WindowView *window_view_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    struct nemotool *tool;
    struct nemoshow *show;

    WindowView *view = calloc(sizeof(WindowView), 1);
    view->tool = tool = nemowidget_get_tool(parent);
    view->show = show = nemowidget_get_show(parent);
    view->width = width;
    view->width = height;
    view->app = app;

    NemoWidget *widget;
    view->back = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _window_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *one;
    view->back_one = one = RECT_CREATE(nemowidget_get_canvas(widget), width, height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    PlayerUI *player;
    view->back_open = player = nemoui_player_create(parent, width, height,
            APP_RES_DIR"/back/show.mp4", false, 1, false);
    nemoui_player_append_callback(player, "player,done", _player_done, view);
    nemoui_player_enable_blend(player, true);
    view->back_close = player = nemoui_player_create(parent, width, height,
            APP_RES_DIR"/back/hide.mp4", false, 1, false);
    nemoui_player_enable_blend(player, true);

    view->back_frame = player = nemoui_player_create(parent, width, height,
            APP_RES_DIR"/back/frame.mov", false, 1, false);
    nemoui_player_enable_blend(player, true);

    int i;
    for (i = 0 ; i < sizeof(items)/sizeof(items[0]) ; i++) {
        double x, y, w, h;
        x = view->app->config->sxy * items[i].x;
        y = view->app->config->sxy * items[i].y;
        w = view->app->config->sxy * items[i].w;
        h = view->app->config->sxy * items[i].h;
        player = nemoui_player_create(parent, w, h, items[i].path, false, 1, false);
        nemoui_player_translate(player, 0, 0, 0, x, y);
        nemoui_player_enable_blend(player, true);
        view->items = list_append(view->items, player);
    }

    return view;
}

static void window_view_show(WindowView *view)
{
    nemoui_player_play(view->back_open);
    nemoui_player_show(view->back_open, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemoshow_dispatch_frame(view->show);
}

static void window_view_hide(WindowView *view)
{
    nemoui_player_hide(view->back_open, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemoui_player_stop(view->back_open);

    nemoui_player_show(view->back_close, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemoui_player_play(view->back_close);
    nemoshow_dispatch_frame(view->show);
}

static void window_view_destroy(WindowView *view)
{
    nemoui_player_destroy(view->back_open);
    free(view);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    WindowView *view = userdata;

    window_view_hide(view);
    nemowidget_win_exit_after(win, TIMEOUT + 100);
}

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);

    WindowView *view = window_view_create(win, app->config->width, app->config->height, app);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    window_view_show(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    window_view_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
