#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

typedef struct _Mirror Mirror;
struct _Mirror {
    int x, y, width, height;
    char *target;
};

typedef struct _MenuItem MenuItem;
struct _MenuItem {
    char *type;
    char *bg;
    char *txt;
    char *icon;
    char *icon_anim_type;
    char *exec;
    bool resize;
    double sxy;
    char *mirror;
    List *items;
};

static Mirror *parse_tag_mirror(XmlTag *tag)
{
    List *ll;
    XmlAttr *attr;
    Mirror *mirror = calloc(sizeof(Mirror), 1);
    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "target")) {
            if (attr->val) {
                 mirror->target = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "x")) {
            if (attr->val) {
                 mirror->x = atoi(attr->val);
            }
        } else if (!strcmp(attr->key, "y")) {
            if (attr->val) {
                 mirror->y = atoi(attr->val);
            }
        } else if (!strcmp(attr->key, "width")) {
            if (attr->val) {
                 mirror->width = atoi(attr->val);
            }
        } else if (!strcmp(attr->key, "height")) {
            if (attr->val) {
                 mirror->height = atoi(attr->val);
            }
        }
    }
    return mirror;
}

static MenuItem *parse_tag_menu(XmlTag *tag)
{
    List *l;
    XmlAttr *attr;
    MenuItem *mi = calloc(sizeof(MenuItem), 1);
    mi->sxy = 1.0;
    mi->resize = true;

    List *ll;
    XmlTag *child;
    LIST_FOR_EACH(tag->children, ll, child) {
        if (!child->name) continue;
        if (!strcmp(child->name, "items")) {
            MenuItem *submi = parse_tag_menu(child);
            mi->items = list_append(mi->items, submi);
        }
    }

    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                 mi->type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "bg")) {
            if (attr->val) {
                 mi->bg = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                 mi->icon = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon_anim_type")) {
            if (attr->val) {
                 mi->icon_anim_type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "txt")) {
            if (attr->val) {
                 mi->txt = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "exec")) {
            if (attr->val) {
                mi->exec = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "sxy")) {
            if (attr->val) {
                mi->sxy = atof(attr->val);
            }
        } else if (!strcmp(attr->key, "resize")) {
            if (attr->val) {
                if (!strcmp(attr->val, "on")) {
                    mi->resize = true;
                } else {
                    mi->resize = false;
                }
            }
        } else if (!strcmp(attr->key, "mirror")) {
            if (attr->val) {
                mi->mirror = strdup(attr->val);
            }
        }
    }
    return mi;
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    List *menu_items;
    List *mirrors;
    int item_cnt;
    int item_gap;
    int item_duration;
    double item_px, item_py;
    double item_ro;
    int item_width, item_height;
    double item_icon_px, item_icon_py, item_txt_px, item_txt_py;
    int item_grab_min_time;
    int guide_duration;
    char *launch_type;
    char *logfile;
};

typedef struct _Log Log;
struct _Log {
    char *file;
    int fd;
};

Log *log_create(const char *file)
{
    Log *log = malloc(sizeof(Log));
    log->file = strdup(file);

    char *filepath = strdup(file);
    char *path = dirname(filepath);
    file_mkdir_recursive(path, 0755);
    free(filepath);

    Clock clock = clock_get();
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s.%04d%02d%02d%02d%02d%02d", file,
            clock.year, clock.month, clock.day, clock.hours, clock.mins, clock.secs);

    log->fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, 0660);
    return log;
}

void log_write(Log *log, const char *str)
{
    write(log->fd, str, strlen(str));
    write(log->fd, "\n", 1);
    fsync(log->fd);
}

void log_destroy(Log *log)
{
    close(log->fd);
    free(log->file);
    free(log);
}

Log *_log;

typedef struct _CardGuide CardGuide;
typedef struct _CardView CardView;
struct _CardView {
    int log;

    List *mirrors;
    bool visible;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    const char *uuid;

	int icon_history_cnt;
	int icon_throw;
	int icon_throw_duration;
	NemoWidget *icon_widget;

    NemoWidget *widget;
    struct showone *item_group;
    int start;
    int cnt;
    int cx, cy, cw, ch;
    int ix, iy, iw, ih, igap;
    double iro;
	int item_grab_min_time;
    int item_duration;
    List *items;
    struct nemotimer *timer;

    char *launch_type;
    int guide_duration;
    int guide_idx;
    struct nemotimer *guide_timer;
    struct showone *guide_group;
    CardGuide *guide[4];

    NemoWidget *event;
};

struct _CardGuide {
    struct nemotool *tool;
    struct showone *parent;
    int ro;

    int duration;
    struct showone *group;
    struct showone *one;
};

static CardGuide *card_create_guide(struct nemotool *tool, struct showone *parent, double rx, double ry, int duration, int ro)
{
    CardGuide *guide = calloc(sizeof(CardGuide), 1);
    guide->tool = tool;
    guide->parent = parent;
    guide->ro = ro;
    guide->duration = duration;

    const char *uri = APP_ICON_DIR"/menu_drag.svg";
    double ww, hh;
    svg_get_wh(uri, &ww, &hh);
    ww *= rx;
    hh *= ry;

    struct showone *group;
    struct showone *one;
    guide->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_set_width(group, ww);
    nemoshow_item_set_height(group, hh);

    guide->one = one = SVG_PATH_CREATE(group, ww, hh, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_rotate(one, ro);

    return guide;
}

void card_guide_translate(CardGuide *guide, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(guide->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(guide->group, tx, ty);
    }
}

void card_guide_show(CardGuide *guide, double tx0, double ty0, double tx, double ty)
{
    _nemoshow_item_motion(guide->group, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "alpha", 0.9,
            NULL);

    struct nemoshow *show = guide->group->show;
    NemoMotion *m;

    nemoshow_item_translate(guide->group, tx0, ty0);
    m = nemomotion_create(show, NEMOEASE_LINEAR_TYPE, guide->duration, 0);
    nemomotion_attach(m, 0.1,
            guide->one, "alpha", 1.0,
            NULL);
    nemomotion_attach(m, 0.4,
            guide->group, "tx", tx,
            guide->group, "ty", ty,
            NULL);
    nemomotion_attach(m, 0.45,
            guide->one, "alpha", 0.0,
            NULL);
    nemomotion_attach(m, 0.5,
            guide->group, "tx", tx0,
            guide->group, "ty", ty0,
            NULL);

    nemomotion_attach(m, 0.6,
            guide->one, "alpha", 1.0,
            NULL);
    nemomotion_attach(m, 0.9,
            guide->group, "tx", tx,
            guide->group, "ty", ty,
            NULL);
    nemomotion_attach(m, 0.95,
            guide->one, "alpha", 0.0,
            NULL);
    nemomotion_attach(m, 1.0,
            guide->group, "tx", tx0,
            guide->group, "ty", ty0,
            NULL);

    nemomotion_run(m);
}

void card_guide_hide(CardGuide *guide)
{
    _nemoshow_item_motion(guide->group, NEMOEASE_CUBIC_IN_TYPE, 1000, 0,
            "alpha", 0.0,
            NULL);
}


typedef struct _CardItem CardItem;
struct _CardItem {
    CardView *view;
    CardItem *org;
    int width, height;
    CardItem *parent;
    struct showone *parent_one;
    struct showone *group;
    struct showone *ro_group;
    struct showone *bg0;
    struct showone *bg;
    struct showone *icon;
    struct showone *txt;

    struct nemotimer *icon_timer;
    char *type;
    char *bg_uri;
    char *icon_uri;
    char *icon_anim_type;
    char *txt_uri;
    char *exec;
    bool resize;
    double sxy;
    char *mirror;

    double ro;
	uint32_t grab_time;
    NemoWidgetGrab *grab;
    bool open;

    List *children;
};

static void _card_item_icon_timeout(struct nemotimer *timer, void *userdata)
{
    CardItem *it = userdata;
    struct nemoshow *show = it->icon->show;

    RET_IF(!it->icon)
    RET_IF(!it->icon_anim_type);

    // For lotte animation
    if (strstr(it->icon_anim_type, "shopguide")) {
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
        nemomotion_attach(m, 0.5,
                it->icon, "sx", 0.5,
                it->icon, "sy", 0.5, NULL);
        nemomotion_attach(m, 1.0,
                it->icon, "sx", 1.0,
                it->icon, "sy", 1.0,
                NULL);
        nemomotion_run(m);
        nemotimer_set_timeout(timer, 2010);
    } else if (strstr(it->icon_anim_type, "shoppingnews")) {
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_INOUT_TYPE, 1500, 0);
        nemomotion_attach(m, 0.5,
                it->icon, "alpha", 0.25, NULL);
        nemomotion_attach(m, 1.0,
                it->icon, "alpha", 1.0, NULL);
        nemomotion_run(m);
        nemotimer_set_timeout(timer, 1510);
    } else if (strstr(it->icon_anim_type, "smartshopping")) {
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_LINEAR_TYPE, 2000, 0);
        nemomotion_attach(m, 0.5,
                it->icon, "ro", -45.0, NULL);
        nemomotion_attach(m, 1.0,
                it->icon, "ro", 45.0, NULL);
        nemomotion_run(m);
        nemotimer_set_timeout(timer, 2020);
    } else if (strstr(it->icon_anim_type, "culturecenter")) {
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_LINEAR_TYPE, 5000, 0);
        nemoshow_item_rotate(it->icon, 0.0);
        nemomotion_attach(m, 1.0,
                it->icon, "ro", 360.0, NULL);
        nemomotion_run(m);
        nemotimer_set_timeout(timer, 5010);
    } else if (strstr(it->icon_anim_type, "entertainment")) {
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_INOUT_TYPE, 2500, 0);
        nemomotion_attach(m, 0.5,
                it->icon, "ty", it->width/2 + it->height * 0.3 - 25,
                it->icon, "sy", 0.5, NULL);
        nemomotion_attach(m, 1.0,
                it->icon, "ty", it->width/2 + it->height * 0.3 - 50,
                it->icon, "sy", 1.0, NULL);
        nemomotion_run(m);
        nemotimer_set_timeout(timer, 2510);
    }
    nemoshow_dispatch_frame(it->view->show);
}

void card_item_get_translate(CardItem *it, double *tx, double *ty)
{
    double x = nemoshow_item_get_translate_x(it->group);
    double y = nemoshow_item_get_translate_y(it->group);

    if (it->parent) {
        x += nemoshow_item_get_translate_x(it->parent->group) - it->width/2.0;
        y += nemoshow_item_get_translate_y(it->parent->group) - it->height/2.0;
    }

    if (tx) *tx = x;
    if (ty) *ty = y;
}

void card_item_translate(CardItem *it, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(it->group, tx, ty);
    }
}

void card_item_set_alpha(CardItem *it, uint32_t easetype, int duration, int delay, double alpha)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", alpha,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, alpha);
    }
}

void card_item_scale(CardItem *it, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "sx", sx, "sy", sy,
                NULL);
    } else {
        nemoshow_item_scale(it->group, sx, sy);
    }
}

double card_item_get_rotate(CardItem *it)
{
    return nemoshow_item_get_rotate(it->ro_group);
}

void card_item_rotate(CardItem *it, uint32_t easetype, int duration, int delay, double ro)
{
    if (EQUAL(it->ro, ro)) return;
    it->ro = ro;
    if (duration > 0) {
        _nemoshow_item_motion(it->ro_group, easetype, duration, delay,
                "ro", ro,
                NULL);
    } else {
        nemoshow_item_rotate(it->group, ro);
    }

    List *l;
    CardItem *subit;
    LIST_FOR_EACH(it->children, l, subit) {
        card_item_rotate(subit, easetype, duration, delay, ro);
    }
}

CardItem *card_item_dup(CardItem *it)
{
    CardView *view = it->view;
    CardItem *dup = calloc(sizeof(CardItem), 1);
    dup->parent = NULL;
    dup->view = view;
    dup->width = it->width;;
    dup->height = it->height;
    if (it->type) dup->type = strdup(it->type);
    dup->bg_uri = strdup(it->bg_uri);
    if (it->icon_uri) dup->icon_uri = strdup(it->icon_uri);
    if (it->icon_anim_type) dup->icon_anim_type = strdup(it->icon_anim_type);
    if (it->txt_uri) dup->txt_uri = strdup(it->txt_uri);
    if (it->exec) dup->exec = strdup(it->exec);
    dup->resize = it->resize;
    dup->sxy = it->sxy;
    if (it->mirror) dup->mirror = strdup(it->mirror);
    dup->org = it;

    struct showone *group;
    dup->group = group = GROUP_CREATE(nemowidget_get_canvas(view->widget));
    nemoshow_item_set_width(group, dup->width);
    nemoshow_item_set_height(group, dup->height);
    nemoshow_item_pivot(group, 0.5, 0.5);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 1.0);
    dup->ro = card_item_get_rotate(it);
    nemoshow_item_rotate(group, dup->ro);
    nemoshow_item_scale(group, 0.0, 0.0);

    double tx, ty;
    card_item_get_translate(it, &tx, &ty);
    nemoshow_item_translate(group, tx, ty);

    struct showone *one;
    dup->bg0 = one = RRECT_CREATE(group,
            nemoshow_item_get_width(it->bg0), nemoshow_item_get_height(it->bg0),
            10, 10);
    nemoshow_item_set_fill_color(one, RGBA(RED));
    nemoshow_item_set_alpha(one, 0.0);

    dup->bg = one = IMAGE_CREATE(group,
            nemoshow_item_get_width(it->bg), nemoshow_item_get_height(it->bg),
            it->bg_uri);

    if (it->icon_uri && it->icon) {
        if (file_is_svg(it->icon_uri)) {
            dup->icon = one = SVG_PATH_CREATE(group,
                    nemoshow_item_get_width(it->icon), nemoshow_item_get_height(it->icon),
                    it->icon_uri);
            nemoshow_item_set_fill_color(one, RGBA(0xFFFFFFFF));
        } else if (file_is_image(it->icon_uri)) {
            dup->icon = one = IMAGE_CREATE(group,
                    nemoshow_item_get_width(it->icon), nemoshow_item_get_height(it->icon),
                    it->icon_uri);
        } else {
            ERR("Not supported file type: %s", it->icon_uri);
            free(dup);
            return NULL;
        }
        nemoshow_item_set_anchor(one,
                nemoshow_item_get_anchor_x(it->icon), nemoshow_item_get_anchor_y(it->icon));
        nemoshow_item_translate(one,
                nemoshow_item_get_translate_x(it->icon), nemoshow_item_get_translate_y(it->icon));
    }

    if (dup->icon && dup->icon_anim_type)
        dup->icon_timer = TOOL_ADD_TIMER(view->tool, 0, _card_item_icon_timeout, dup);

    if (it->txt_uri) {
        dup->txt = one = SVG_PATH_CREATE(group,
                nemoshow_item_get_width(it->txt), nemoshow_item_get_height(it->txt),
                it->txt_uri);
        nemoshow_item_set_anchor(one,
                nemoshow_item_get_anchor_x(it->txt), nemoshow_item_get_anchor_y(it->txt));
        nemoshow_item_translate(one,
                nemoshow_item_get_translate_x(it->txt), nemoshow_item_get_translate_y(it->txt));
        nemoshow_item_set_fill_color(one, RGBA(0xFFFFFFFF));
    }

    return dup;
}

CardItem *card_create_item(CardView *view, CardItem *parent, MenuItem *mi, ConfigApp *app)
{
    CardItem *it = calloc(sizeof(CardItem), 1);
    it->parent = parent;
    it->view = view;
    it->width = view->iw;
    it->height = view->ih;
    if (mi->type) it->type = strdup(mi->type);
    it->bg_uri = strdup(mi->bg);
    if (mi->icon) it->icon_uri = strdup(mi->icon);
    if (mi->icon_anim_type) it->icon_anim_type = strdup(mi->icon_anim_type);
    if (mi->txt) it->txt_uri = strdup(mi->txt);
    if (mi->exec) it->exec = strdup(mi->exec);
    it->resize = mi->resize;
    it->sxy = mi->sxy;
    if (mi->mirror) it->mirror = strdup(mi->mirror);
	it->grab_time = 0;

    double global_sxy = app->config->sxy;

    struct showone *group;
    if (it->parent) {
        it->group = group = GROUP_CREATE(it->parent->group);
    } else {
        it->group = group = GROUP_CREATE(view->item_group);
    }
    it->ro_group = GROUP_CREATE(it->group);

    MenuItem *submi;
    List *l;
    LIST_FOR_EACH(mi->items, l, submi) {
        CardItem *subit = card_create_item(view, it, submi, app);
        if (subit) {
            nemoshow_one_above(it->ro_group, subit->group);
            card_item_set_alpha(subit, 0, 0, 0, 1.0);
            card_item_scale(subit, 0, 0, 0, 1.0, 1.0);
            card_item_translate(subit, 0, 0, 0, subit->width/2, subit->height/2);
            it->children = list_append(it->children, subit);
        }
    }

    nemoshow_item_set_width(group, it->width);
    nemoshow_item_set_height(group, it->height);
    nemoshow_item_pivot(group, 0.5, 0.5);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);

    struct showone *one;
    it->bg0 = one = RECT_CREATE(it->ro_group, it->width, it->height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_tag(one, 0x999);
    nemoshow_one_set_userdata(one, it);

    it->bg = one = IMAGE_CREATE(it->ro_group, it->width, it->height, it->bg_uri);

    if (it->icon_uri) {
        if (file_is_svg(it->icon_uri)) {
            double w, h;
            double ww, hh;
            svg_get_wh(it->icon_uri, &ww, &hh);
            w = ww * global_sxy;
            h = hh * global_sxy;
            it->icon = one = SVG_PATH_CREATE(it->ro_group, w, h, it->icon_uri);
            nemoshow_item_set_fill_color(one, RGBA(0xFFFFFFFF));
        } else if (file_is_image(it->icon_uri)) {
            double w, h;
            int ww, hh;
            image_get_wh(it->icon_uri, &ww, &hh);
            w = ww * global_sxy;
            h = hh * global_sxy;
            it->icon = one = IMAGE_CREATE(it->ro_group, w, h, it->icon_uri);
        } else {
            ERR("Not supported icon file type: %s", it->icon_uri);
        }
        nemoshow_item_translate(one, it->width * app->item_icon_px, it->height * app->item_icon_py);

        if (it->icon_anim_type) {
            // XXX: for Lotte animation
            if (strstr(it->icon_anim_type, "smartshopping")) {
                nemoshow_item_set_anchor(one, 0.5, 0.0);
                nemoshow_item_translate(one, it->width * app->item_icon_px, it->height * (app->item_icon_py - 0.15));
            } else if (strstr(it->icon_anim_type, "entertainment")) {
                nemoshow_item_set_anchor(one, 0.5, 1.0);
                nemoshow_item_translate(one, it->width * app->item_icon_px, it->height * app->item_icon_py);
            } else {
                nemoshow_item_set_anchor(one, 0.5, 0.5);
                nemoshow_item_translate(one, it->width * app->item_icon_px, it->height * app->item_icon_py);
            }
            it->icon_timer = TOOL_ADD_TIMER(view->tool, 0, _card_item_icon_timeout, it);
        } else {
            nemoshow_item_set_anchor(one, 0.5, 0.5);
            nemoshow_item_translate(one, it->width * app->item_icon_px, it->height * app->item_icon_py);
        }
    }

    if (it->txt_uri) {
        double w, h;
        double ww, hh;
        svg_get_wh(it->txt_uri, &ww, &hh);
        w = ww * global_sxy;
        h = hh * global_sxy;
        it->txt = one = SVG_PATH_GROUP_CREATE(it->ro_group, w, h, it->txt_uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one,
                it->width * app->item_txt_px,
                it->height * app->item_txt_py);
    }

    return it;
}

void card_item_destroy(CardItem *it)
{
    if (it->icon_timer) nemotimer_destroy(it->icon_timer);
    nemoshow_one_destroy(it->group);
    free(it->bg_uri);
    if (it->icon_uri) free(it->icon_uri);
    if (it->icon_anim_type) free(it->icon_anim_type);
    if (it->txt_uri) free(it->txt_uri);
    if (it->exec) free(it->exec);
    free(it);
}

CardItem *view_append_item(CardView *view, MenuItem *mi, ConfigApp *app)
{
    CardItem *it;
    it = card_create_item(view, NULL, mi, app);
    RET_IF(!it, NULL);
    view->items = list_append(view->items, it);

    return it;
}

void card_item_show(CardItem *it, uint32_t easetype, int duration, int delay)
{
    nemoshow_one_above(it->group, NULL);
    if (it->icon_anim_type) nemotimer_set_timeout(it->icon_timer, 100 + delay);
}

void card_item_hide(CardItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->icon_timer) nemotimer_set_timeout(it->icon_timer, 0);
}

static void _card_item_hide_destroy(struct nemotimer *timer, void *userdata)
{
    CardItem *it = userdata;
    card_item_destroy(it);
    // Do not remove from lists;
    nemotimer_destroy(timer);
}

void card_item_hide_destroy(CardItem *it, uint32_t easetype, int duration, int delay)
{
    card_item_hide(it, easetype, duration, delay);
    TOOL_ADD_TIMER(it->view->tool, duration + delay + 100, _card_item_hide_destroy, it);
}

void card_item_execute(CardItem *it, double ex, double ey)
{
    CardView *view = it->view;
    struct nemoshow *show = it->view->show;
    card_item_hide_destroy(it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    card_item_scale(it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0, 0.0);
    card_item_rotate(it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0);

    float x, y;
    nemoshow_transform_to_viewport(show,
            NEMOSHOW_ITEM_AT(it->group, tx),
            NEMOSHOW_ITEM_AT(it->group, ty),
            &x, &y);

    double ro = 0.0;
    if (!strcmp(view->launch_type, "table")) {
        int rw, rh;
        rw = (view->width - view->cw)/2;
        rh = (view->height - view->ch)/2;
        if (RECTS_CROSS(0, rh + view->ch, rw, rh, ex, ey, 1, 1)) {
            ro = 45;
        } else if (RECTS_CROSS(0, rh, rw, view->ch, ex, ey, 1, 1)) {
            ro = 90;
        } else if (RECTS_CROSS(0, 0, rw, rh, ex, ey, 1, 1)) {
            ro = 135;
        } else if (RECTS_CROSS(rw, 0, view->cw, rh, ex, ey, 1, 1)) {
            ro = 180;
        } else if (RECTS_CROSS(rw + view->cw, 0, rw, rh, ex, ey, 1, 1)) {
            ro = 225;
        } else if (RECTS_CROSS(rw + view->cw, rh, rw, view->ch, ex, ey, 1, 1)) {
            ro = 270;
        } else if (RECTS_CROSS(rw + view->cw, rh + view->ch, rw, rh, ex, ey, 1, 1)) {
            ro = 315;
        }
    }

    nemoshow_view_set_anchor(show, 0.5, 0.5);

    char path[PATH_MAX];
    char name[PATH_MAX];
    char args[PATH_MAX];
    char *buf;
    char *tok;
    buf = strdup(it->exec);
    tok = strtok(buf, ";");
    snprintf(name, PATH_MAX, "%s", tok);
    snprintf(path, PATH_MAX, "%s", tok);
    tok = strtok(NULL, "");
    snprintf(args, PATH_MAX, "%s", tok);
    free(buf);

    struct nemobus *bus;
    struct busmsg *msg;
    struct itemone *one;
    char states[512];

    bus = NEMOBUS_CREATE();

    const char *type = it->type;
    if (!type) {
        type="app";
    }
    msg = NEMOMSG_CREATE_CMD(type, path);

    ERR("%lf %lf %s %s %s", x, y, type, path, args);
    one = nemoitem_one_create();
    nemoitem_one_set_attr_format(one, "x", "%f", x);
    nemoitem_one_set_attr_format(one, "y", "%f", y);
    nemoitem_one_set_attr_format(one, "r", "%f", ro);
    nemoitem_one_set_attr_format(one, "sx", "%f", it->sxy);
    nemoitem_one_set_attr_format(one, "sy", "%f", it->sxy);
    nemoitem_one_set_attr(one, "owner", view->uuid);
    nemoitem_one_set_attr(one, "resize", it->resize? "on" : "off");
    nemoitem_one_save_attrs(one, states, ';');
    nemoitem_one_destroy(one);

    nemobus_msg_set_attr(msg, "args", args);
    nemobus_msg_set_attr(msg, "states", states);

    if (it->mirror) {
        if (strstr(it->mirror, "/nemoshell/fullscreen"))
            nemobus_msg_set_attr_format(msg, "mirrorscreen", it->mirror);
    } else {
        List *l;
        Mirror *mirror;
        LIST_FOR_EACH(view->mirrors, l, mirror) {
            if (RECTS_CROSS(x, y, 1, 1, mirror->x, mirror->y, mirror->width, mirror->height)) {
                nemobus_msg_set_attr_format(msg, "mirrorscreen", mirror->target);
                break;
            }
        }
    }
    NEMOMSG_SEND(bus, msg);

    nemosound_play(APP_SOUND_DIR"/show.wav");

    char buff[PATH_MAX];
    Clock now;
    now = clock_get();
    snprintf(buff, PATH_MAX, "%d %d %d %d %d %d %s %s", now.year, now.month, now.day,
            now.hours, now.mins, now.secs, path, args);
    if (_log) log_write(_log, buff);
}

void card_show(CardView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_show(view->event, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_OUT_TYPE;
    int duration = 1000;
    int delay = 150;

    view->start = 0;

    int i;
    for (i = 0; i < view->cnt; i++) {
        CardItem *it = LIST_DATA(list_get_nth(view->items, i));
        if (!it) {
            ERR("item is NULL at index(%d)", i);
            break;
        }

        if (i != 0) {
            card_item_rotate(it, easetype, duration, delay * i, view->iro);
            card_item_show(it, easetype, duration, delay * i);
            card_item_set_alpha(it, easetype, duration, delay * i, 1.0);
            card_item_scale(it, easetype, duration, delay * i, 1.0, 1.0);
        }
        double tx, ty;
        tx = view->ix + (view->iw + view->igap) * i;
        ty = view->iy;
        card_item_translate(it, easetype, duration, delay * i, tx, ty);
    }

    nemotimer_set_timeout(view->timer, duration + delay * (i - 1) + 10);
    nemotimer_set_timeout(view->guide_timer, delay * (i - 1) + 10);

    nemoshow_dispatch_frame(view->show);
}

static void card_item_revoke(CardItem *it)
{
    struct nemoshow *show = it->group->show;
    nemoshow_revoke_transition_one(show, it->group, "alpha");
    nemoshow_revoke_transition_one(show, it->ro_group, "ro");
    nemoshow_revoke_transition_one(show, it->group, "sx");
    nemoshow_revoke_transition_one(show, it->group, "sy");
    nemoshow_revoke_transition_one(show, it->group, "tx");
    nemoshow_revoke_transition_one(show, it->group, "ty");

    if (it->icon) {
        nemoshow_revoke_transition_one(show, it->icon, "alpha");
        nemoshow_revoke_transition_one(show, it->icon, "ro");
        nemoshow_revoke_transition_one(show, it->icon, "sx");
        nemoshow_revoke_transition_one(show, it->icon, "sy");
        nemoshow_revoke_transition_one(show, it->icon, "tx");
        nemoshow_revoke_transition_one(show, it->icon, "ty");
    }
}

void card_hide(CardView *view)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_hide(view->event, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_IN_TYPE;
    int duration = 500;
    List *l;
    CardItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        card_item_revoke(it);
        card_item_hide(it, easetype, duration, 0);
        card_item_rotate(it, easetype, duration, 0, 0);
        card_item_scale(it, easetype, duration, 0, 0.0, 0.0);
        card_item_translate(it, easetype, duration, 0, view->ix, view->iy);

        /*
        List *l;
        CardItem *subit;
        LIST_FOR_EACH(it->children, l, subit) {
            card_item_translate(subit, easetype, duration, 0, view->ix, view->iy);
        }
        */
    }

    nemotimer_set_timeout(view->timer, 0);

    nemotimer_set_timeout(view->guide_timer, 0);
    card_guide_hide(view->guide[0]);
    card_guide_hide(view->guide[1]);
    card_guide_hide(view->guide[2]);
    card_guide_hide(view->guide[3]);

    nemoshow_dispatch_frame(view->show);
}

static void _card_timeout(struct nemotimer *timer, void *userdata)
{
    CardView *view = userdata;

    int cnt = list_count(view->items);
    if (cnt <= view->cnt) {
        ERR("Number(%d) of appended items should be bigger than number(%d) of viewable items",
                view->cnt, list_count(view->items));
        return;
    }

    List *l;
    l = list_get_nth(view->items, view->start);
    RET_IF(!l);

    int i, j;
    j = view->start;
    for (i = 0; i < view->cnt ; i++) {
        uint32_t easetype = NEMOEASE_LINEAR_TYPE;
        int duration = view->item_duration - 20;

        CardItem *it = LIST_DATA(list_get_nth(view->items, j));
        if (!it) {
            ERR("item is NULL at index(%d)", j);
            break;
        }

        card_item_show(it, NEMOEASE_CUBIC_OUT_TYPE, duration, 0);
        if (i == 0) {
            card_item_rotate(it, 0, 0, 0, 0.0);
            card_item_translate(it, 0, 0, 0, view->ix, view->iy);

            card_item_rotate(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, view->iro);
            card_item_set_alpha(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 1.0);
            card_item_scale(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 1.0, 1.0);
        } else if (i == view->cnt - 1) {
            card_item_rotate(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0);
            card_item_set_alpha(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0);
            card_item_scale(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0, 0.0);
            int delay = 0;
            List *l;
            CardItem *subit;
            LIST_FOR_EACH_REVERSE(it->children, l, subit) {
                card_item_translate(subit, NEMOEASE_CUBIC_INOUT_TYPE, 500, delay, it->width/2, it->height/2);
                delay += 150;
            }
            it->open = false;

            if (it->grab) {
                nemowidget_grab_set_done(it->grab, true);
                it->grab = NULL;
            }
        }

        double tx, ty;
        tx = view->ix + (view->iw + view->igap) * (i + 1);
        ty = view->iy;
        card_item_translate(it, easetype, duration, 0, tx, ty);

        j++;
        if ( j >= cnt ) {
            j = 0;
        }
    }

    view->start--;
    if (view->start < 0) {
        view->start = cnt - 1;
    }

    nemotimer_set_timeout(timer, view->item_duration);
    nemoshow_dispatch_frame(view->show);
}

static void _card_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemowidget_grab_is_done(grab)) return;

    CardItem *it = userdata;
    CardView *view = it->view;

    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        if (it->exec) {
            uint32_t grab_time = nemoshow_event_get_time(event);
            if (grab_time - it->grab_time >= view->item_grab_min_time) {
                it->grab_time = nemoshow_event_get_time(event);
                CardItem *dup = card_item_dup(it);
                card_item_show(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
                card_item_scale(dup, NEMOEASE_CUBIC_OUT_TYPE, 250, 0, 1.0, 1.0);
                nemowidget_grab_set_data(grab, "item", dup);
            }
        } else {
            if (it->grab) return;
            it->grab = grab;
            card_item_scale(it, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.75, 0.75);
        }
    } else if (nemoshow_event_is_motion(show, event)) {
        CardItem *dup = nemowidget_grab_get_data(grab, "item");
		if (!dup) return;
        card_item_translate(dup, 0, 0, 0, ex, ey);

        double ro = 0.0;
        if (!strcmp(view->launch_type, "table")) {
            if (!RECTS_CROSS(view->cx - view->cw/2.0, view->cy - view->ch/2.0,
                        view->cw, view->ch, ex, ey, 5, 5)) {

                int rw, rh;
                rw = (view->width - view->cw)/2;
                rh = (view->height - view->ch)/2;
                if (RECTS_CROSS(0, rh + view->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 45;
                } else if (RECTS_CROSS(0, rh, rw, view->ch, ex, ey, 1, 1)) {
                    ro = 90;
                } else if (RECTS_CROSS(0, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 135;
                } else if (RECTS_CROSS(rw, 0, view->cw, rh, ex, ey, 1, 1)) {
                    ro = 180;
                } else if (RECTS_CROSS(rw + view->cw, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 225;
                } else if (RECTS_CROSS(rw + view->cw, rh, rw, view->ch, ex, ey, 1, 1)) {
                    ro = 270;
                } else if (RECTS_CROSS(rw + view->cw, rh + view->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 315;
                }
            }
            card_item_rotate(dup, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, ro);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        if (it->exec) {
            CardItem *dup = nemowidget_grab_get_data(grab, "item");
            if (!dup || !dup->org) return;

            double px = 0, py = 0;
            int cnt = 0;
            if (dup->org->parent) {
                card_item_get_translate(dup->org->parent, &px, &py);
                cnt = list_count(dup->org->parent->children);
            }

            if (dup->org->parent && RECTS_CROSS(px - it->width/2.0, py + it->height/2.0,
                        cnt * it->width, it->height, ex, ey, 5, 5)) {
                card_item_hide_destroy(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
                card_item_set_alpha(dup, NEMOEASE_CUBIC_OUT_TYPE, 900, 0, 0);
                card_item_scale(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.25, 0.25);

                double tx, ty;
                card_item_get_translate(dup->org, &tx, &ty);
                card_item_translate(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, tx, ty);
            } else if (RECTS_CROSS(view->cx - view->cw/2.0, view->cy - view->ch/2.0,
                        view->cw, view->ch, ex, ey, 5, 5)) {
                card_item_hide_destroy(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
                card_item_set_alpha(dup, NEMOEASE_CUBIC_OUT_TYPE, 900, 0, 0);
                card_item_scale(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.25, 0.25);

                double tx, ty;
                card_item_get_translate(dup->org, &tx, &ty);
                card_item_translate(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, tx, ty);
            } else {
                card_item_execute(dup, ex, ey);
            }
        } else {
            if (!it->grab || it->grab != grab) return;
            card_item_scale(it, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0, 1.0, 1.0);
            it->grab = NULL;
            int i = 0;
            if (nemoshow_event_is_single_click(show, event)) {
                if (!it->open) {
                    int delay = 0;
                    List *l;
                    CardItem *subit;
                    LIST_FOR_EACH(it->children, l, subit) {
                        card_item_translate(subit, NEMOEASE_CUBIC_INOUT_TYPE, 500, delay,
                                it->width/2 + it->width * i, it->height + it->height/2);
                        i++;
                        delay += 150;
                    }
                    it->open = true;
                } else {
                    int delay = 0;
                    List *l;
                    CardItem *subit;
                    LIST_FOR_EACH(it->children, l, subit) {
                        card_item_translate(subit, NEMOEASE_CUBIC_INOUT_TYPE, 500, delay,
                                subit->width/2, subit->height/2);
                        delay += 150;
                    }
                    it->open = false;
                }
            }
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _card_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    CardView *view = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && 0x999 == nemoshow_one_get_tag(one)) {
            CardItem *it = nemoshow_one_get_userdata(one);
            nemowidget_create_grab(widget, event,
                    _card_item_grab_event, it);
        } else {
            struct nemotool *tool = nemowidget_get_tool(widget);
            uint64_t device = nemoshow_event_get_device(event);
            float vx, vy;
            nemoshow_transform_to_viewport(show, ex, ey, &vx, &vy);
            nemotool_touch_bypass(tool, device, vx, vy);
        }
    }
}

static void _card_guide_timeout(struct nemotimer *timer, void *userdata)
{
    CardView *view = userdata;
    if (view->guide_idx == 0) {
        card_guide_show(view->guide[0],
                view->cx, view->cy,
                view->cx, view->cy + view->ih/2);

        card_guide_hide(view->guide[1]);
        card_guide_hide(view->guide[2]);
        card_guide_hide(view->guide[3]);
    } else if (view->guide_idx == 1) {
        card_guide_show(view->guide[1],
                view->cx, view->cy,
                view->cx, view->cy - view->ih/2);

        card_guide_hide(view->guide[0]);
        card_guide_hide(view->guide[2]);
        card_guide_hide(view->guide[3]);
    } else if (view->guide_idx == 2) {
        card_guide_show(view->guide[2],
                view->cx - view->cw/2 + view->iw/2, view->cy,
                view->cx - view->cw/2, view->cy);

        card_guide_hide(view->guide[0]);
        card_guide_hide(view->guide[1]);
        card_guide_hide(view->guide[3]);
    } else if (view->guide_idx == 3) {
        card_guide_show(view->guide[3],
                view->cx + view->cw/2 - view->iw/2, view->cy,
                view->cx + view->cw/2, view->cy);

        card_guide_hide(view->guide[0]);
        card_guide_hide(view->guide[1]);
        card_guide_hide(view->guide[2]);
    }
    view->guide_idx++;
    if (view->guide_idx >= 4) view->guide_idx = 0;
    nemotimer_set_timeout(timer, view->guide_duration + 1000);
    nemoshow_dispatch_frame(view->show);
}

CardView *card_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    CardView *view = calloc(sizeof(CardView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->width = width;
    view->height = height;
    view->cnt = app->item_cnt;
    view->iw = app->item_width;
    view->ih = app->item_height;
    view->iro = app->item_ro;
    view->igap = app->item_gap;
    view->item_duration = app->item_duration;
	view->item_grab_min_time = app->item_grab_min_time;
    view->guide_duration = app->guide_duration;
    view->guide_idx = 0;
    view->mirrors = app->mirrors;
    if (app->launch_type) {
        view->launch_type = strdup(app->launch_type);
    } else {
        view->launch_type = strdup("table");
    }

    view->cw = view->iw * view->cnt + view->igap * view->cnt;
    view->ch = view->ih;

    view->cx = view->width * app->item_px;
    view->cy = view->height * app->item_py;

    view->ix = view->cx - view->cw/2.0;
    view->iy = view->cy - view->ch/2.0 + view->ih/2.0;
    double rx, ry;
    rx = (double)app->config->sxy;
    ry = (double)app->config->sxy;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_enable_event(widget, false);

    struct showone *group;
    view->item_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    struct nemotimer *timer;
    view->guide_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->guide[0] = card_create_guide(view->tool, group, rx, ry, view->guide_duration, 0);
    view->guide[1] = card_create_guide(view->tool, group, rx, ry, view->guide_duration, 180);
    view->guide[2] = card_create_guide(view->tool, group, rx, ry, view->guide_duration, 90);
    view->guide[3] = card_create_guide(view->tool, group, rx, ry, view->guide_duration, 270);

    view->guide_timer = timer = TOOL_ADD_TIMER(view->tool, 0, _card_guide_timeout, view);

    view->event = widget = nemowidget_create_pixman(parent, width, height);
    nemowidget_append_callback(widget, "event", _card_event, view);
    nemowidget_enable_event_repeat(widget, true);

    view->timer = timer = TOOL_ADD_TIMER(view->tool, view->item_duration,
            _card_timeout, view);

    return view;
}

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);
    app->item_px = 0.5;
    app->item_py = 0.5;
    app->item_icon_px = 0.5;
    app->item_icon_py = 0.35;
    app->item_txt_px = 0.5;
    app->item_txt_py = 0.7;

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
    const char *temp;

    snprintf(buf, PATH_MAX, "%s/items", prefix);
    List *tags  = xml_search_tags(xml, buf);
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        MenuItem *mi = parse_tag_menu(tag);
        if (!mi) continue;
        app->menu_items = list_append(app->menu_items, mi);
    }

    snprintf(buf, PATH_MAX, "%s/mirror", prefix);
    tags = xml_search_tags(xml, buf);
    LIST_FOR_EACH(tags, l, tag) {
        Mirror *mirror = parse_tag_mirror(tag);
        if (!mirror) continue;
        app->mirrors = list_append(app->mirrors, mirror);
    }

    snprintf(buf, PATH_MAX, "%s/item", prefix);
    temp = xml_get_value(xml, buf, "count");
    if (!temp) {
        ERR("No item count in %s", prefix);
    } else {
        app->item_cnt = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "gap");
    if (!temp) {
        ERR("No item area width in %s", prefix);
    } else {
        app->item_gap = atoi(temp);
    }
    app->item_gap *= app->config->sxy;

    temp = xml_get_value(xml, buf, "duration");
    if (!temp) {
        ERR("No item duration in %s", prefix);
    } else {
        app->item_duration = atoi(temp);
    }

    temp = xml_get_value(xml, buf, "px");
    if (temp) {
        app->item_px = atof(temp);
    }
    temp = xml_get_value(xml, buf, "py");
    if (temp) {
        app->item_py = atof(temp);
    }
    temp = xml_get_value(xml, buf, "ro");
    if (!temp) {
        ERR("No item py in %s", prefix);
    } else {
        app->item_ro = atof(temp);
    }
    temp = xml_get_value(xml, buf, "width");
    if (!temp) {
        ERR("No item width in %s", prefix);
    } else {
        app->item_width = atoi(temp);
    }
    app->item_width *= app->config->sxy;
    temp = xml_get_value(xml, buf, "height");
    if (!temp) {
        ERR("No item height in %s", prefix);
    } else {
        app->item_height = atoi(temp);
    }
    app->item_height *= app->config->sxy;

    // Position
    temp = xml_get_value(xml, buf, "icon_px");
    if (temp) {
        app->item_icon_px = atof(temp);
    }
    temp = xml_get_value(xml, buf, "icon_py");
    if (temp) {
        app->item_icon_py = atof(temp);
    }
    temp = xml_get_value(xml, buf, "txt_px");
    if (temp) {
        app->item_txt_px = atof(temp);
    }
    temp = xml_get_value(xml, buf, "txt_py");
    if (temp) {
        app->item_txt_py = atof(temp);
    }

    temp = xml_get_value(xml, buf, "grab_min_time");
    if (!temp) {
        ERR("No item grab_min_time in %s", prefix);
    } else {
        app->item_grab_min_time = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/guide", prefix);
    temp = xml_get_value(xml, buf, "duration");
    if (!temp) {
        ERR("No guide duration in %s", prefix);
    } else {
        app->guide_duration = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/launch", prefix);
    temp = xml_get_value(xml, buf, "type");
    if (!temp) {
        ERR("No launch type in %s", prefix);
    } else {
        app->launch_type = strdup(temp);
    }

    snprintf(buf, PATH_MAX, "%s/log", prefix);
    temp = xml_get_value(xml, buf, "file");
    if (temp) {
        app->logfile = strdup(temp);
    }

    xml_unload(xml);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    MenuItem *mi;
    LIST_FREE(app->menu_items, mi) {
        if (mi->type) free(mi->type);
        free(mi->bg);
        if (mi->icon) if (mi->icon) free(mi->icon);
        if (mi->icon_anim_type) free(mi->icon_anim_type);
        if (mi->txt) free(mi->txt);
        if (mi->exec) free(mi->exec);
        free(mi);
    }
    Mirror *mirror;
    LIST_FREE(app->mirrors, mirror) {
        free(mirror->target);
        free(mirror);
    }
    if (app->launch_type) free(app->launch_type);
    if (app->logfile) free(app->logfile);
    free(app);
}

static void _card_win_layer(NemoWidget *win, const char *id, void *info, void *userdata)
{
    CardView *view = userdata;
    int32_t visible = (intptr_t)info;
    if (visible == -1) {
        if (view->visible) {
            card_hide(view);
            view->visible = false;
        }
    } else {
        if (!view->visible) {
            card_show(view);
            view->visible = true;
        }
    }
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    if (app->logfile) _log = log_create(app->logfile);
    WELLRNG512_INIT();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);

    CardView *view = card_create(win, app->config->width, app->config->height, app);

    List *l;
    MenuItem *mi;
    LIST_FOR_EACH(app->menu_items, l, mi) {
        CardItem *it = view_append_item(view, mi, app);
        card_item_translate(it, 0, 0, 0, view->ix, view->iy);
    }
    card_show(view);
    nemowidget_append_callback(win, "layer", _card_win_layer, view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    if (_log) log_destroy(_log);

    return 0;
}
