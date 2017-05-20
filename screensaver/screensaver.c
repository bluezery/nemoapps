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

typedef struct _SaverView SaverView;
struct _SaverView
{
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *parent;

    int width, height;

    NemoWidget *back;
    struct showone *back_one;

    PlayerUI *player;
};

static void _saver_view_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    SaverView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        NemoWidget *win = nemowidget_get_top_widget(view->back);
        nemowidget_callback_dispatch(win, "exit", NULL);
    }
}

SaverView *saver_view_create(NemoWidget *parent, int width, int height, const char *path, bool enable_audio)
{
    struct nemotool *tool;
    struct nemoshow *show;

    SaverView *view = calloc(sizeof(SaverView), 1);
    view->tool = tool = nemowidget_get_tool(parent);
    view->show = show = nemowidget_get_show(parent);
    view->width = width;
    view->width = height;

    NemoWidget *widget;
    view->back = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _saver_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *one;
    view->back_one = one = RECT_CREATE(nemowidget_get_canvas(widget), width, height);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));

    PlayerUI *player;
    view->player = player = nemoui_player_create(parent, width, height, path, enable_audio, -1, false);

    return view;
}

static void saver_view_show(SaverView *view)
{
    nemoui_player_play(view->player);
    nemoui_player_show(view->player, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemoshow_dispatch_frame(view->show);
}

static void saver_view_hide(SaverView *view)
{
    nemoui_player_stop(view->player);
    nemoui_player_hide(view->player, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemoshow_dispatch_frame(view->show);
}

static void saver_view_destroy(SaverView *view)
{
    nemoui_player_destroy(view->player);
    free(view);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    SaverView *view = userdata;

    saver_view_hide(view);
    nemowidget_win_exit_after(win, TIMEOUT + 100);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *path;
    bool enable_audio;
};

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->enable_audio = true;
    app->config = config_load(domain, filename, argc, argv);

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        {"audio", required_argument, NULL, 'a'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:a:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                app->path = strdup(optarg);
                break;
            case 'a':
                app->enable_audio = !strcmp(optarg, "off") ? false : true;
                break;
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->path) free(app->path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->path) {
        ERR("Usage: %s -f FILENAME [-a off]", APPNAME);
        return -1;
    }

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);
    // XXX: all windows are not visible under screensaver
    nemowidget_win_enable_state(win, "opaque", true);

    SaverView *view = saver_view_create(win, app->config->width, app->config->height,
            app->path, app->enable_audio);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    saver_view_show(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    saver_view_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
