#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timerfd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

#define PLANET_GRAB_MIN_TIME 500
#define MENU_DESTROY_TIMEOUT 6000

typedef struct _ConfigGroup ConfigGroup;
typedef struct _ConfigItem ConfigItem;

struct _ConfigGroup {
    char *name;
    List *items;
};

struct _ConfigItem {
    char *icon;
    char *type;
    char *exec;
    bool resize;
    double sxy;
};

static ConfigItem *parse_tag_item(XmlTag *tag)
{
    List *ll;
    XmlAttr *attr;
    ConfigItem *item = calloc(sizeof(ConfigItem), 1);
    item->sxy = 1.0;
    item->resize = true;

    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                 item->type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                 item->icon = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "exec")) {
            if (attr->val) {
                item->exec = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "sxy")) {
            if (attr->val) {
                item->sxy = atof(attr->val);
            }
        } else if (!strcmp(attr->key, "resize")) {
            if (attr->val) {
                if (!strcmp(attr->val, "on")) {
                    item->resize = true;
                } else {
                    item->resize = false;
                }
            }
        }
    }
    return item;
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    List *grps;
    double sxy;
    char *planet_uri;
};

typedef struct _MenuView MenuView;
typedef struct _MainItem MainItem;
typedef struct _SubItem SubItem;

struct _MenuView {
    const char *uuid;
    ConfigApp *app;
    int width, height;
    struct nemotool *tool;
    struct nemoshow *show;

    NemoWidget *widget;
    struct showone *group;

    struct showone *planet;
    struct nemotimer *planet_timer;
    struct nemotimer *timer;

    List *items;
    struct nemotimer *destroy_timer;
};

struct _SubItem {
    MainItem *main_it;
    ConfigItem *config;
    int w, h;
    MainItem *it;
    struct showone *group;
    struct showone *one;
};

struct _MainItem {
    int w, h;
    MenuView *view;
    char *name;
    struct showone *group;
    struct showone *main_group; // Only scale for main item
    struct showone *event;
    struct showone *one;
    struct showone *one_active;
    int state; // 0:normal, 1: active
    List *items;
};

SubItem *sub_item_create(MainItem *main_it, ConfigItem *config, double app_sxy)
{
    SubItem *it;
    it = calloc(sizeof(SubItem), 1);
    it->main_it = main_it;
    it->config = config;

    struct showone *group;
    it->group = group = GROUP_CREATE(main_it->group);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);

    image_get_wh(config->icon, &it->w, &it->h);
    it->w *= app_sxy;
    it->h *= app_sxy;

    struct showone *one;
    it->one = one = IMAGE_CREATE(group, it->w, it->h, config->icon);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "sub_item");
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    return it;
}

void sub_item_down(SubItem *it)
{
    _nemoshow_item_motion_bounce(it->group, NEMOEASE_CUBIC_INOUT_TYPE, 350, 0,
            "sx", 0.75, 0.85, "sy", 0.75, 0.85, NULL);
}

void sub_item_up(SubItem *it)
{
    _nemoshow_item_motion(it->group, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
            "sx", 1.0, "sy", 1.0, NULL);
}

void sub_item_translate(SubItem *it, uint32_t easetype, int duration, int delay, double x, double y)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "tx", x, "ty", y, NULL);
    } else {
        nemoshow_item_translate(it->group, x, y);
    }
}

void sub_item_show(SubItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "sx", 1.0, "sy", 1.0,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_scale(it->group, 1.0, 1.0);
        nemoshow_item_set_alpha(it->group, 1.0);
    }
}

void sub_item_hide(SubItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "sx", 0.0, "sy", 0.0,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_scale(it->group, 0.0, 0.0);
        nemoshow_item_set_alpha(it->group, 0.0);
    }
}

void menu_item_set_state(MainItem *it, int state)
{
    it->state = state;
    if (it->state == 0) {
        _nemoshow_item_motion(it->one, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion(it->one_active, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                "alpha", 0.0, NULL);

        int delay = 0;
        List *l;
        SubItem *sub_it;
        LIST_FOR_EACH(it->items, l, sub_it) {
            sub_item_translate(sub_it, NEMOEASE_CUBIC_OUT_TYPE, 400, delay, 0, 0);
            sub_item_hide(sub_it, NEMOEASE_CUBIC_OUT_TYPE, 400, delay);
            delay += 100;
        }
    } else if (it->state == -1) {
        _nemoshow_item_motion(it->one, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(it->one_active, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 0.0, NULL);
    } else if (it->state == 1) {
        _nemoshow_item_motion(it->one, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(it->one_active, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 1.0, NULL);

        double r = (it->w/2) * 1.3;
        double s_angle = -90 + 12 + (list_get_idx(it->view->items, it) + 1) * 72;
        int delay = 0;
        List *l;
        SubItem *sub_it;
        LIST_FOR_EACH(it->items, l, sub_it) {
            double x, y;
            x = r * cos(s_angle * M_PI / 180.0);
            y = r * sin(s_angle * M_PI / 180.0);
            s_angle += 26;
            sub_item_translate(sub_it, NEMOEASE_CUBIC_OUT_TYPE, 500, delay, x, y);
            sub_item_show(sub_it, NEMOEASE_CUBIC_OUT_TYPE, 500, delay);
            delay += 100;
        }

    } else {
        ERR("Not supported state");
    }
}

void menu_item_down(MainItem *it)
{
    _nemoshow_item_motion_bounce(it->event, NEMOEASE_CUBIC_INOUT_TYPE, 350, 0,
            "sx", 0.75, 0.85, "sy", 0.75, 0.85, NULL);
    _nemoshow_item_motion_bounce(it->one, NEMOEASE_CUBIC_INOUT_TYPE, 350, 0,
            "sx", 0.75, 0.85, "sy", 0.75, 0.85, NULL);
}

void menu_item_up(MainItem *it)
{
    _nemoshow_item_motion(it->event, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
            "sx", 1.0, "sy", 1.0, NULL);
    _nemoshow_item_motion(it->one, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
            "sx", 1.0, "sy", 1.0, NULL);
}

void menu_item_scale(MainItem *it, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    _nemoshow_item_motion(it->main_group, easetype, duration, 0,
            "sx", sx, "sy", sy, NULL);
}

static void _menu_sub_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    SubItem *it = userdata;
    MenuView *view = it->main_it->view;
    nemotimer_set_timeout(view->destroy_timer, MENU_DESTROY_TIMEOUT);

    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;

    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        nemotimer_set_timeout(view->timer, 0);
        List *l;
        MainItem *itt;
        LIST_FOR_EACH(view->items, l, itt) {
            nemoshow_revoke_transition_one(view->show, itt->group, "ro");
        }
        sub_item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        nemotimer_set_timeout(view->timer, 100);
        sub_item_up(it);

        if (nemoshow_event_is_single_click(show, event)) {
            char path[PATH_MAX];
            char name[PATH_MAX];
            char args[PATH_MAX];
            char *buf;
            char *tok;
            buf = strdup(it->config->exec);
            tok = strtok(buf, ";");
            snprintf(name, PATH_MAX, "%s", tok);
            snprintf(path, PATH_MAX, "%s", tok);
            tok = strtok(NULL, "");
            snprintf(args, PATH_MAX, "%s", tok);
            free(buf);

            nemo_execute(view->uuid, it->config->type, path, args,
                    it->config->resize? "on" : "off",
                    ex, ey, 0.0, it->config->sxy, it->config->sxy);
        }
    }
    nemoshow_dispatch_frame(show);
}
static void _menu_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    MainItem *it = userdata;
    MenuView *view = it->view;
    nemotimer_set_timeout(view->destroy_timer, MENU_DESTROY_TIMEOUT);

    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;

    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        nemotimer_set_timeout(view->timer, 0);
        List *l;
        MainItem *itt;
        LIST_FOR_EACH(view->items, l, itt) {
            nemoshow_revoke_transition_one(view->show, itt->group, "ro");
        }
        menu_item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        nemotimer_set_timeout(view->timer, 100);
        menu_item_up(it);

        if (nemoshow_event_is_single_click(show, event)) {
            if (it->state == 0) {
                menu_item_set_state(it, 1);
                menu_item_scale(it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.25, 1.25);

                List *l;
                MainItem *itt;
                LIST_FOR_EACH(view->items, l, itt) {
                    if (it == itt) continue;
                    menu_item_set_state(itt, 0);
                    menu_item_scale(itt, NEMOEASE_CUBIC_IN_TYPE, 500, 0, 1.0, 1.0);
                }


            } else if (it->state == 1) {
                menu_item_set_state(it, 0);
                menu_item_scale(it, NEMOEASE_CUBIC_IN_TYPE, 500, 0, 1.0, 1.0);
            }
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _menu_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    MenuView *view = userdata;
    nemotimer_set_timeout(view->destroy_timer, MENU_DESTROY_TIMEOUT);
}

void menu_item_show(MainItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion(it->event, easetype, duration, delay,
                "sx", 1.0, "sy", 1.0, NULL);
        _nemoshow_item_motion(it->one, easetype, duration, delay,
                "sx", 1.0, "sy", 1.0, NULL);
        _nemoshow_item_motion(it->one_active, easetype, duration, delay,
                "sx", 1.0, "sy", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
        nemoshow_item_scale(it->one, 1.0, 1.0);
        nemoshow_item_scale(it->one_active, 1.0, 1.0);
    }
}

void menu_item_hide(MainItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(it->event, easetype, duration, delay,
                "alpha", 0.0, "sx", 0.0, "sy", 0.0,
                NULL);
        _nemoshow_item_motion(it->one, easetype, duration, delay,
                "alpha", 0.0, "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
        nemoshow_item_scale(it->event, 0.0, 0.0);
        nemoshow_item_scale(it->one, 0.0, 0.0);
    }
}

void menu_item_destroy(MainItem *it)
{
    free(it->name);
    nemoshow_one_destroy(it->event);
    nemoshow_one_destroy(it->one);
    nemoshow_one_destroy(it->main_group);
    nemoshow_one_destroy(it->group);
    free(it);
}

void menu_item_translate(MainItem *it, int x, int y)
{
    nemoshow_item_translate(it->group, x, y);
}

MainItem *menu_create_item(MenuView *view, ConfigGroup *grp)
{
    MainItem *it = calloc(sizeof(MainItem), 1);
    it->view = view;
    it->name = strdup(grp->name);

    // XXX: There should be icon with same name as group name
    char uri[PATH_MAX];
    snprintf(uri, PATH_MAX, APPDATA_ROOT"/main/%s.png", grp->name);
    file_get_image_wh(uri, &it->w, &it->h);
    it->w *= view->app->config->sxy;
    it->h *= view->app->config->sxy;

    struct showone *group;
    it->group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;
    it->main_group = group = GROUP_CREATE(it->group);
    nemoshow_item_set_width(group, it->w);
    nemoshow_item_set_height(group, it->h);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    //nemoshow_item_translate(group, it->w/2, it->h/2);

    it->one = one = IMAGE_CREATE(group, it->w, it->h, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, it->w/2, it->h/2);
    nemoshow_item_scale(one, 0.0, 0.0);

    snprintf(uri, PATH_MAX, APPDATA_ROOT"/main/%s_active.png", grp->name);
    it->one_active = one = IMAGE_CREATE(group, it->w, it->h, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, it->w/2, it->h/2);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_alpha(one, 0.0);

    snprintf(uri, PATH_MAX, APPDATA_ROOT"/main/event/%s.svg", grp->name);

    double ww, hh;
    svg_get_wh(uri, &ww, &hh);
    ww *= view->app->config->sxy;
    hh *= view->app->config->sxy;
    it->event = one = SVG_PATH_CREATE(group, ww, hh, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, ww/2, hh/2);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_id(one, "main_item");
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_alpha(one, 0.0);

    List *l;
    ConfigItem *cit;
    LIST_FOR_EACH(grp->items, l, cit) {
        SubItem *sub_it;
        sub_it = sub_item_create(it, cit, view->app->config->sxy);
        it->items = list_append(it->items, sub_it);
    }

    view->items = list_append(view->items, it);
    return it;
}

void menu_view_destroy(MenuView *view)
{
    MainItem *it;
    LIST_FREE(view->items, it) menu_item_destroy(it);
    nemoshow_one_destroy(view->group);
    free(view);
}

void menu_view_hide(MenuView *view)
{
    nemotimer_set_timeout(view->destroy_timer, 0);
    nemowidget_set_alpha(view->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.0);

    nemotimer_set_timeout(view->timer, 0);
    nemotimer_set_timeout(view->planet_timer, 0);
    _nemoshow_item_motion(view->planet, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, "sx", 0.0, "sy", 0.0, NULL);

    int delay = 0;
    List *l;
    MainItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        menu_item_hide(it, NEMOEASE_CUBIC_INOUT_TYPE, 500, delay);
        delay += 50;
    }
    nemoshow_dispatch_frame(view->show);
}

void _menu_view_destroy(struct nemotimer *timer, void *userdata)
{
    MenuView *view = userdata;
    menu_view_destroy(view);
}

void _menu_destroy_timeout(struct nemotimer *timer, void *userdata)
{
    MenuView *view = userdata;
    menu_view_hide(view);
    TOOL_ADD_TIMER(view->tool, 1000, _menu_view_destroy, view);
}

static void _menu_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    MenuView *view = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            const char *id = nemoshow_one_get_id(one);
            if (id) {
                if (!strcmp(id, "main_item")) {
                    MainItem *it = nemoshow_one_get_userdata(one);
                    if (it) {
                        nemowidget_create_grab(widget, event,
                                _menu_item_grab_event, it);
                    }
                } else if (!strcmp(id, "sub_item")) {
                    SubItem *it = nemoshow_one_get_userdata(one);
                    if (it) {
                        nemowidget_create_grab(widget, event,
                                _menu_sub_item_grab_event, it);
                    }
                }
            } else {
                nemowidget_create_grab(widget, event,
                        _menu_view_grab_event, view);
            }
        } else {
            struct nemotool *tool = nemowidget_get_tool(widget);
            uint64_t device = nemoshow_event_get_device(event);
            float vx, vy;
            nemoshow_transform_to_viewport(show, ex, ey, &vx, &vy);
            nemotool_touch_bypass(tool, device, vx, vy);
        }
    }
}

void menu_view_show(MenuView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    nemotimer_set_timeout(view->timer, 100);
    nemotimer_set_timeout(view->planet_timer, 100);
    _nemoshow_item_motion(view->planet, easetype, duration, delay,
            "alpha", 1.0, "sx", 1.0, "sy", 1.0, NULL);
    int _delay = 0;
    List *l;
    MainItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        menu_item_show(it, NEMOEASE_CUBIC_INOUT_TYPE, 1000, _delay + delay);
        _delay += 150;
    }

    nemoshow_dispatch_frame(view->show);
    nemotimer_set_timeout(view->destroy_timer, MENU_DESTROY_TIMEOUT);
}

static void _menu_view_timeout(struct nemotimer *timer, void *userdata)
{
    MenuView *view = userdata;
    return;

    int duration = 0;
    List *l;
    MainItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        double ro = nemoshow_item_get_rotate(it->group);
        if (ceil(ro) == 360) {
            ro = 0.0;
            nemoshow_item_rotate(it->group, 0.0);
        }
        duration = 40000 * ((360 - ro)/360.0);
        _nemoshow_item_motion(it->group, NEMOEASE_LINEAR_TYPE, duration, 0,
                "ro", 360.0, NULL);
    }
    nemoshow_dispatch_frame(view->show);
    nemotimer_set_timeout(timer, duration);
}

static void _menu_view_planet_timeout(struct nemotimer *timer, void *userdata)
{
    return;

    MenuView *view = userdata;
    int duration = 15000;
    nemoshow_item_rotate(view->planet, 0.0);
    _nemoshow_item_motion(view->planet, NEMOEASE_LINEAR_TYPE, duration, 0,
            "ro", -360.0, NULL);
    nemotimer_set_timeout(timer, duration);
}

MenuView *menu_view_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    MenuView *view = calloc(sizeof(MenuView), 1);
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->width = width;
    view->height = height;
    view->app = app;

    NemoWidget *widget;
    struct showone *group;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _menu_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    struct showone *one;
    int w, h;
    file_get_image_wh(app->planet_uri, &w, &h);
    w *= app->config->sxy;
    h *= app->config->sxy;
    view->planet = one = IMAGE_CREATE(group, w, h, app->planet_uri);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, width/2, height/2);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_scale(one, 0.0, 0.0);

    List *l;
    ConfigGroup *grp;
    LIST_FOR_EACH(app->grps, l, grp) {
        MainItem *it;
        it = menu_create_item(view, grp);
        menu_item_translate(it, width/2, height/2);
    }

    view->timer = TOOL_ADD_TIMER(view->tool, 0, _menu_view_timeout, view);
    view->planet_timer = TOOL_ADD_TIMER(view->tool, 0, _menu_view_planet_timeout, view);
    view->destroy_timer = TOOL_ADD_TIMER(view->tool, 0, _menu_destroy_timeout, view);

    return view;
}

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

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

    const char *prefix = "config";
    char buf[PATH_MAX];

    snprintf(buf, PATH_MAX, "%s/items", prefix);
    List *tags  = xml_search_tags(xml, buf);
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        ConfigGroup *grp = NULL;
        List *ll;
        XmlAttr *attr;
        LIST_FOR_EACH(tag->attrs, ll, attr) {
            if (!strcmp(attr->key, "group")) {
                if (attr->val) {
                    List *lll;
                    ConfigGroup *ggrp;
                    LIST_FOR_EACH(app->grps, lll, ggrp) {
                        if (!strcmp(ggrp->name, attr->val)) {
                            grp = ggrp;
                            break;
                        }
                    }
                }
                if (!grp) {
                    grp = calloc(sizeof(ConfigGroup), 1);
                    grp->name = strdup(attr->val);
                    app->grps = list_append(app->grps, grp);
                }
                break;
            }
        }
        if (!grp) continue;
        ConfigItem *it = parse_tag_item(tag);
        if (!it) continue;
        if (!it->type) it->type = strdup("app");
        grp->items = list_append(grp->items, it);
    }

    xml_unload(xml);

    struct option options[] = {
        {"planet", required_argument, NULL, 'p'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:", options, NULL)) != -1) {
        switch(opt) {
            case 'p':
                app->planet_uri = strdup(optarg);
                break;
            default:
                break;
        }
    }
    RET_IF(!app->planet_uri, NULL);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    free(app->planet_uri);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    ERR("%d %d %lf", app->config->width, app->config->height, app->config->sxy);
    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);
    nemowidget_win_enable_scale(win, -1);
    nemowidget_win_enable_rotate(win, -1);

    MenuView *view = menu_view_create(win, app->config->width, app->config->height, app);
    menu_view_show(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
