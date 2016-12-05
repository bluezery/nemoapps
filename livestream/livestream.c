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

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "nemohelper.h"

#define COLOR0 0xEA562DFF
#define COLOR1 0x35FFFFFF
#define COLORBACK 0x10171E99

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    List *paths;
    bool enable_audio;
    int repeat;
    double sxy;
    int col, row;
};

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    PlayerUI *ui = userdata;

    nemoui_player_stop(ui);

    nemowidget_win_exit_after(win, 500);
}

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->enable_audio = true;
    app->repeat = -1;
    app->config = config_load(domain, appname, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

    char buf[PATH_MAX];
    const char *temp;

    double sx = 1.0;
    double sy = 1.0;
    int width, height;
    snprintf(buf, PATH_MAX, "%s/size", appname);
    temp = xml_get_value(xml, buf, "width");
    if (!temp) {
        ERR("No size width in %s", appname);
    } else {
        width = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "height");
    if (!temp) {
        ERR("No size height in %s", appname);
    } else {
        height = atoi(temp);
    }
    if (width > 0) sx = (double)app->config->width/width;
    if (width > 0) sy = (double)app->config->height/height;
    if (sx > sy) app->sxy = sy;
    else app->sxy = sx;

    temp = xml_get_value(xml, buf, "col");
    if (!temp) {
        ERR("No size col in %s", appname);
    } else {
        app->col = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "row");
    if (!temp) {
        ERR("No size row in %s", appname);
    } else {
        app->row = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/play", appname);
    temp = xml_get_value(xml, buf, "repeat");
    if (temp && strlen(temp) > 0) {
        app->repeat = atoi(temp);
    }

    List *tags  = xml_search_tags(xml, APPNAME"/file");
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        List *ll;
        XmlAttr *attr;
        LIST_FOR_EACH(tag->attrs, ll, attr) {
            if (!strcmp(attr->key, "path")) {
                if (attr->val) {
                    char *path = strdup(attr->val);
                    app->paths = list_append(app->paths, path);
                    break;
                }
            }
        }
    }

    xml_unload(xml);

    struct option options[] = {
        {"repeat", required_argument, NULL, 'p'},
        {"audio", required_argument, NULL, 'a'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:p:", options, NULL)) != -1) {
        switch(opt) {
            case 'a':
                app->enable_audio = !strcmp(optarg, "off") ? false : true;
                break;
            case 'p':
                app->repeat = atoi(optarg);
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    char *path;
    LIST_FREE(app->paths, path) free(path);
    free(app);
}

typedef struct _Tile Tile;
struct _Tile
{
    PlayerUI *ui;
    int tx, ty;
};

typedef struct _View View;
struct _View {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;
    struct showone *group;
    struct showone *bg;

    int row;
    int max_row;
    int zoom;
    int max_zoom;
    int fw, fh;
    int iw, ih;

    List *tiles;
};

View *view_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    View *view = calloc(sizeof(View), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;
    view->max_row = app->row;
    view->row = app->row;
    view->max_zoom = app->row;
    view->zoom = app->row;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, app->config->width, app->config->height);
    //nemowidget_append_callback(widget, "event", _win_event, widget);

    struct showone *group;
    struct showone *one;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->bg = one = RECT_CREATE(group, width, height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    view->iw = view->width/view->zoom;
    view->ih = view->height/view->zoom;
    view->fw = view->iw;
    view->fh = view->ih;

    int i = 0;
    List *l;
    char *path;
    LIST_FOR_EACH(app->paths, l, path) {
        //if (!file_is_video(path)) continue;
        Tile *tile = calloc(sizeof(Tile), 1);
        tile->tx = i%app->row;
        tile->ty = i/app->row;

        PlayerUI *ui;
        tile->ui = ui = nemoui_player_create(widget, view->iw, view->ih, path, app->enable_audio);
        if (!ui) {
            ERR("ui is NULL");
            free(tile);
            continue;
        }

        view->tiles = list_append(view->tiles, tile);
        i++;
        if (i >= app->row * app->row) break;
    }

    return view;
}

void view_zoom(View *view, int zoom, int gx, int gy, uint32_t easetype, int duration, int delay)
{
    view->zoom = zoom;
    int w, h;
    view->iw = view->width/view->zoom;
    view->ih = view->height/view->zoom;

    double scale;
    scale = (double)view->iw/view->fw;

    List *l;
    Tile *tile;
    LIST_FOR_EACH(view->tiles, l, tile) {
        if (!tile) {
            ERR("tile is NULL");
            continue;
        }

        float x, y;
        nemoui_player_translate(tile->ui, easetype, duration, delay,
                tile->tx * view->iw, tile->ty * view->ih);
        nemoui_player_scale(tile->ui, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, scale, scale);
    }
}

void view_show(View *view, uint32_t easetype, int duration, int delay)
{
    List *l;
    Tile *tile;
    LIST_FOR_EACH(view->tiles, l, tile) {
        if (!tile) {
            ERR("tile is NULL");
            continue;
        }
        nemoui_player_show(tile->ui);
    }
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->paths) {
        ERR("No playable resources are provided");
        return -1;
    }

	ao_initialize();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "underlay");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    View *view = view_create(win, app->config->width, app->config->height, app);
    view_zoom(view, view->zoom - 3, 0, 0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    view_show(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
	ao_shutdown();

    _config_unload(app);

    return 0;
}
