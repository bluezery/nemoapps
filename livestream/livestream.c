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
    if (height > 0) sy = (double)app->config->height/height;
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

typedef struct _Item Item;
typedef struct _View View;
struct _Item
{
    View *view;
    PlayerUI *ui;
    int x, y;
    struct showone *group;
    struct showone *event;
    struct showone *font;
    struct showone *text;
    Thread *thread;
};

struct _View {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;
    struct showone *bg;
    struct showone *item_group;

    int row;
    int max_row;
    int zoom;
    int zoom_max;
    int iw0, ih0;
    int iw, ih;
    int ix, iy;
    double scale;

    int zoom_start;

    List *items;
    Item *show_item;

    NemoWidget *event_widget;
    NemouiGesture *gesture;
};

void item_below(Item *it, NemoWidget *below)
{
    nemoui_player_below(it->ui, below);
}

void item_destroy(Item *it)
{
    thread_destroy(it->thread);
    nemoshow_one_destroy(it->event);
    nemoshow_one_destroy(it->text);
    nemoshow_one_destroy(it->font);
    nemoshow_one_destroy(it->group);
    nemoui_player_destroy(it->ui);
    free(it);
}

static void _player_done(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    PlayerUI *ui = userdata;
    nemoui_player_play(ui);
}

Item *item_create(NemoWidget *widget, struct showone *pgroup, View *view,
        int x, int y, int w, int h, const char *uri, bool enable_audio)
{
    Item *it = calloc(sizeof(Item), 1);
    it->view = view;
    it->x = x;
    it->y = y;

    PlayerUI *ui;
    it->ui = ui = nemoui_player_create(widget, w, h, uri, enable_audio, 1, true);
    if (!ui) {
        ERR("ui is NULL");
        free(it);
        return NULL;
    }
    nemoui_player_append_callback(ui, "player,done", _player_done, ui);
    struct showone *group;
    it->group = group = GROUP_CREATE(pgroup);

    struct showone *font;
    struct showone *one;
    it->font = font = FONT_CREATE("NanumGothic", "Regular");
    char *temp = strdup(uri);
    it->text = one = TEXT_CREATE(group, font, 10, basename(temp));
    free(temp);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));
    nemoshow_item_translate(one, w/2.0, h + 30);
    nemoshow_item_set_alpha(one, 0.0);

    it->event = one = RECT_CREATE(group, w, h);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, it);
    return it;
}

void item_show_text(Item *it)
{
    _nemoshow_item_motion(it->text, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            "alpha", 1.0, NULL);
}

void item_hide_text(Item *it)
{
    _nemoshow_item_motion(it->text, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            "alpha", 0.0, NULL);
}

void item_translate(Item *it, uint32_t easetype, int duration, int delay, float x, float y)
{
    nemoui_player_translate(it->ui, easetype, duration, delay, x, y);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "tx", (double)x, "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(it->group, x, y);
    }
}

void item_scale(Item *it, uint32_t easetype, int duration, int delay, float sx, float sy)
{
    nemoui_player_scale(it->ui, easetype, duration, delay, sx, sy);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "sx", sx, "sy", sy,
                NULL);
    } else {
        nemoshow_item_scale(it->group, sx, sy);
    }
}

void item_hide(Item *it, uint32_t easetype, int duration, int delay)
{
    nemoui_player_stop(it->ui);
    nemoui_player_hide(it->ui, easetype, duration, delay);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
    }
}

void item_show(Item *it, uint32_t easetype, int duration, int delay)
{
    nemoui_player_show(it->ui, easetype, duration, delay);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
    }
}

void item_play(Item *it)
{
    nemoui_player_play(it->ui);
}

// ix, iy are designated left top item's coordinates.
void _zoom(View *view, int zoom, int ix, int iy, uint32_t easetype, int duration, int delay)
{
    if (zoom < 1) zoom = 1;
    if (ix < 0) ix = 0;
    if (iy < 0) iy = 0;
    if (ix + zoom >= view->row) ix = view->row - zoom;
    if (iy + zoom >= view->row) iy = view->row - zoom;

    view->zoom = zoom;
    view->iw = view->width/view->zoom;
    view->ih = view->height/view->zoom;
    view->ix = ix;
    view->iy = iy;
    view->scale = (double)view->iw/view->iw0;

    List *l;
    Item *it;
    LIST_FOR_EACH(view->items, l, it) {
        if (!it) {
            ERR("it is NULL");
            continue;
        }
        float x, y;
        x = (it->x - view->ix) * view->iw;
        y = (it->y - view->iy) * view->ih;
        // XXX: show only visible players
        if (view->ix <= it->x && it->x < view->ix + view->zoom && view->iy <= it->y && it->y < view->iy + view->zoom) {
            item_translate(it, easetype, duration, delay, x, y);
            item_scale(it, easetype, duration, delay, view->scale, view->scale);
            item_show(it, 0, 0, 0);
            if (!nemoui_player_is_prepared(it->ui)) {
                ERR("item(%s) is not prepared", nemoui_player_get_uri(it->ui));
            } else {
                item_play(it);
            }
        } else {
            item_translate(it, easetype, duration, delay, x, y);
            item_scale(it, easetype, duration, delay, view->scale, view->scale);
            item_hide(it, easetype, duration, delay);
        }
    }
    nemoshow_dispatch_frame(view->show);
}

void view_zoom(View *view, int zoom, int ix, int iy, uint32_t easetype, int duration, int delay)
{
    RET_IF(zoom <= 0);
    RET_IF(zoom > view->zoom_max);
    if (view->zoom == zoom) return;

    _zoom(view, zoom, ix, iy, easetype, duration, delay);
}

static void *_item_prepare(void *userdata)
{
    Item *it = userdata;
    nemoui_player_prepare(it->ui);
    return NULL;
}

static void _item_prepare_done(bool cancel, void *userdata)
{
    Item *it = userdata;
    View *view = it->view;
    if (!cancel) {
        float x, y;
        x = (it->x - view->ix) * view->iw;
        y = (it->y - view->iy) * view->ih;
        // XXX: show only visible players
        if (view->ix <= it->x && it->x < view->ix + view->zoom && view->iy <= it->y && it->y < view->iy + view->zoom) {
            item_translate(it, 0, 0, 0, x, y);
            item_scale(it, 0, 0, 0, view->scale, view->scale);
            item_show(it, 0, 0, 0);
            if (!nemoui_player_is_prepared(it->ui)) {
                ERR("item(%s) is not prepared", nemoui_player_get_uri(it->ui));
            } else {
                item_play(it);
            }
        } else {
            item_translate(it, 0, 0, 0, x, y);
            item_scale(it, 0, 0, 0, view->scale, view->scale);
            item_hide(it, 0, 0, 0);
        }
        nemoshow_dispatch_frame(view->show);
    }
}

void view_show(View *view, uint32_t easetype, int duration, int delay)
{
    _zoom(view, view->row, view->ix, view->iy, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

    nemowidget_show(view->widget, easetype, duration, delay);
    nemowidget_show(view->event_widget, easetype, duration, delay);
    nemoui_gesture_show(view->gesture);

    List *l;
    Item *it;
    LIST_FOR_EACH(view->items, l, it) {
        if (!it) {
            ERR("it is NULL");
            continue;
        }
        item_show(it, easetype, duration, delay);
        if (!it->thread) {
            it->thread = thread_create(view->tool, _item_prepare, _item_prepare_done, it);
        }
    }
}

static void _view_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    View *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_single_click(show, event)) {
        if (nemoshow_event_is_done(event)) return;

        double ex, ey;
        nemowidget_transform_from_global(view->widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);

        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one) {
            Item *it = nemoshow_one_get_userdata(one);

            if (view->show_item == it) {
                view_zoom(view, view->row, 0, 0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
                view->show_item = NULL;
            } else {
                int iw, ih;
                iw = view->width/(view->row + 1);
                ih = view->height/(view->row + 1);
                int ix, iy;
                ix = 0;
                iy = 0;
                List *l;
                Item *itt;
                LIST_FOR_EACH(view->items, l, itt) {
                    item_play(itt);
                    item_show(itt, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                    double scale;
                    float x, y;
                    if (itt == it) {
                        x = (view->row - 1) * iw;
                        y = (view->row - 3) * ih;
                        scale = (double)iw * 2/view->iw0;
                        item_translate(itt, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, x, y);
                        item_scale(itt, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, scale, scale);
                        item_below(itt, view->event_widget);
                        item_show_text(itt);
                        view->show_item = it;
                        continue;
                    }
                    x = ix * iw;
                    y = iy * ih;
                    scale = (double)iw/view->iw0;
                    item_translate(itt, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, x, y);
                    item_scale(itt, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, scale, scale);
                    item_hide_text(itt);
                    ix++;
                    if (ix >= view->row - 1) {
                        ix = 0;
                        iy++;
                    }
                }
                view->ix = 0;
                view->iy = 0;
                view->zoom = view->row + 1;
            }
            nemoshow_dispatch_frame(view->show);
        }
    }
}

static void _view_scale(NemouiGesture *gesture, NemoWidget *widget, void *event, double scale, void *userdata)
{
    View *view = userdata;
    // XXX: only increase/decrease by one step
    if (scale >= 1.0) scale = 1.0;
    double zoom = (int)(view->zoom_start + scale + 0.5);
    if (view->zoom == zoom) return;
    if (view->zoom_start - zoom > 1) zoom = view->zoom_start - 1;
    else if (view->zoom_start - view->zoom > 1) zoom = view->zoom_start + 1;

    double cx, cy;
    nemoui_gesture_get_center(gesture, event, &cx, &cy);
    int ix, iy;
    if (view->zoom > zoom) {
        if (cx < view->width/2) {
            if (cy < view->height/2) {
                ix = view->ix;
                iy = view->iy;
            } else {
                ix = view->ix;
                iy = view->iy + 1;
            }
        } else {
            if (cy < view->height/2) {
                ix = view->ix + 1;
                iy = view->iy;
            } else {
                ix = view->ix + 1;
                iy = view->iy + 1;
            }
        }
    } else {
        if (cx < view->width/2) {
            if (cy < view->height/2) {
                ix = view->ix - 1;
                iy = view->iy - 1;
            } else {
                ix = view->ix - 1;
                iy = view->iy;
            }
        } else {
            if (cy < view->height/2) {
                ix = view->ix;
                iy = view->iy - 1;
            } else {
                ix = view->ix;
                iy = view->iy;
            }
        }
    }

    view_zoom(view, zoom, ix, iy, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    view->show_item = NULL;

    nemoshow_event_set_done_all(event);
}

static void _view_scale_start(NemouiGesture *gesture, NemoWidget *widget, void *event, void *userdata)
{
    View *view = userdata;
    view->zoom_start = view->zoom;
}

static void _view_scale_stop(NemouiGesture *gesture, NemoWidget *widget, void *event, void *userdata)
{
}

void view_hide(View *view, uint32_t easetype, int duration, int delay)
{
    nemoui_gesture_hide(view->gesture);
    nemowidget_hide(view->event_widget, easetype, duration, delay);

    List *l;
    Item *it;
    LIST_FOR_EACH(view->items, l, it) {
        item_hide(it, easetype, duration, delay);
    }
    _nemoshow_item_motion(view->item_group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    nemowidget_hide(view->widget, easetype, duration, delay);
}

void view_destroy(View *view)
{
    nemoui_gesture_destroy(view->gesture);
    nemowidget_destroy(view->event_widget);
    List *l;
    Item *it;
    LIST_FOR_EACH(view->items, l, it) {
        item_destroy(it);
    }
    nemoshow_one_destroy(view->bg);
    nemoshow_one_destroy(view->item_group);
    nemowidget_destroy(view->widget);
    free(view);
}

View *view_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    View *view = calloc(sizeof(View), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;
    view->row = app->row;
    view->max_row = view->row;
    view->zoom_max = view->row;
    view->zoom = view->row;
    view->ix = 0;
    view->iy = 0;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, app->config->width, app->config->height);

    struct showone *group;
    struct showone *one;
    view->item_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->bg = one = RECT_CREATE(group, width, height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    view->iw = view->width/view->zoom;
    view->ih = view->height/view->zoom;
    view->iw0 = view->iw;
    view->ih0 = view->ih;

    int i = 0;
    List *l;
    char *path;
    LIST_FOR_EACH(app->paths, l, path) {
        //if (!file_is_video(path)) continue;
        int ix = i%app->row;
        int iy = i/app->row;

        Item *it;
        it = item_create(widget, group, view, ix, iy, view->iw, view->ih, path, app->enable_audio);
        if (!it) {
            ERR("item create failed: %d/%d", i, app->row * app->row);
            continue;
        }
        view->items = list_append(view->items, it);
        i++;
        if (i >= app->row * app->row) break;
    }

    view->event_widget = widget = nemowidget_create_vector(parent,
            app->config->width, app->config->height);
    nemowidget_append_callback(widget, "event", _view_event, view);

    NemouiGesture *gesture;
    gesture = view->gesture = nemoui_gesture_create(parent, width, height);
    nemoui_gesture_set_scale(gesture, _view_scale, _view_scale_start, _view_scale_stop, view);
    nemoui_gesture_set_max_taps(gesture, 2);

    return view;
}

static void _win_fullscreen_callback(NemoWidget *win, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Fullscreen *fs = info;
    View *view = userdata;

    // FIXME: scaling all textures for fitting in the fullscreen
	if (fs->id) {
    } else {
    }

    nemoshow_dispatch_frame(view->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    View *view = userdata;
    view_hide(view, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    nemowidget_win_exit_after(win, 1000);
}

int main(int argc, char *argv[])
{
    // FIXME
    avformat_network_init();
    ao_initialize();

    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->paths) {
        ERR("No playable resources are provided");
        return -1;
    }

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_enable_fullscreen(win, true);
    nemowidget_win_enable_move(win, 3);
    nemowidget_win_enable_rotate(win, 3);
    nemowidget_win_enable_scale(win, 3);

    View *view = view_create(win, app->config->width, app->config->height, app);
    nemowidget_append_callback(win, "fullscreen", _win_fullscreen_callback, view);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    view_show(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
	ao_shutdown();

    _config_unload(app);

    return 0;
}
