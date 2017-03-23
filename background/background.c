#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    List *icons;
    char *bgpath;
    int bgtimeout;
    int bgduration;
    int sketch_timeout;
    int sketch_min_dist;
    int sketch_dot_cnt;
    int icon_cnt;
    int icon_history_cnt;
    int icon_history_min_dist;
    int icon_throw_min_dist;
    int icon_throw;
    int icon_throw_coeff;
    int icon_throw_duration;
};

typedef struct _BackgroundView BackgroundView;
struct _BackgroundView {
    bool visible;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *parent;
    struct showone *blur;

    int gallery_timeout;
    int gallery_duration;
    NemoWidget *bg_widget;
    struct showone *bg_group;
    char *bgpath;
    Gallery *gallery;
    int gallery_idx;
    struct nemotimer *gallery_timer;

    PlayerUI *player;

    Sketch *sketch;
    int sketch_timeout;
    struct nemotimer *sketch_timer;

    int icon_history_cnt;
    int icon_history_min_dist;
    int icon_throw_min_dist;
    int icon_throw_coeff;
    int icon_throw_duration;
    NemoWidget *icon_widget;
    struct showone *icon_group;
    List *icons;
};

#define MAX_HISTORY_CNT 50
typedef struct _Grab Grab;
struct _Grab {
    uint64_t device;
    int idx;
};

Grab *grab_create(uint64_t device)
{
    Grab *grab = malloc(sizeof(Grab));
    grab->device = device;
    grab->idx = -1;

    return grab;
}

uint32_t COL[] = {
    0xFFFFFFFF,
    0xFFDAAFFF,
    0x84CFD4FF,
    0x36B7E1FF,
    0xE77A90FF,
    0xFDC36CFF,
    0xD1E778FF
};

typedef struct _Icon Icon;
struct _Icon {
    BackgroundView *view;
    List *grabs;
    uint64_t grab_dev0, grab_dev1;
    double grab_ro;
    double grab_scale_sum;
    double grab_scale_x;
    double grab_scale_y;
    int width, height;

    int prev_t[MAX_HISTORY_CNT];
    double prev_x[MAX_HISTORY_CNT], prev_y[MAX_HISTORY_CNT];
    int prev_idx;

    int grab_diff_x, grab_diff_y;
    int idx;
    char *uri;
    struct showone *group;
    struct showone *one;
    struct nemotimer *timer;
    struct nemotimer *move_timer;
    struct nemotimer *color_timer;
};

static void _icon_color_timeout(struct nemotimer *timer, void *userdata)
{
    Icon *icon = userdata;
    int idx = WELLRNG512()%(sizeof(COL)/sizeof(COL[0]));
    _nemoshow_item_motion(icon->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "fill", COL[idx],
            NULL);
    nemotimer_set_timeout(timer, 550);
}

static void _icon_move_timeout(struct nemotimer *timer, void *userdata)
{
    Icon *icon = userdata;
    BackgroundView *view = icon->view;

    double t, tx, ty, sxy, ro;
    t = (WELLRNG512() % 60) * 1000 + 20000;
    tx = WELLRNG512()%view->width;
    ty = WELLRNG512()%view->height;
    sxy = (WELLRNG512() % 200)/100.0 + 0.5;
    ro = WELLRNG512() % 360;

    _nemoshow_item_motion(icon->group, NEMOEASE_LINEAR_TYPE, t, 0,
            "tx", tx, "ty", ty,
            "sx", sxy, "sy", sxy,
            "ro", ro, NULL);

    nemotimer_set_timeout(timer, t + 100);
    nemoshow_dispatch_frame(view->show);
}

static void _icon_timeout(struct nemotimer *timer, void *userdata)
{
    Icon *icon = userdata;
    struct nemoshow *show = icon->one->show;
    int timeout = 0;
    if (strstr(icon->uri, "shopguide")) {
        timeout = 2000 * 2;
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_OUT_TYPE, timeout, 0);
        nemomotion_attach(m, 0.5,
                icon->one, "sx", 0.5,
                icon->one, "sy", 0.5, NULL);
        nemomotion_attach(m, 1.0,
                icon->one, "sx", 1.0,
                icon->one, "sy", 1.0,
                NULL);
        nemomotion_run(m);
    } else if (strstr(icon->uri, "shoppingnews")) {
        timeout = 1500 * 2;
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_INOUT_TYPE, timeout, 0);
        nemomotion_attach(m, 0.5,
                icon->one, "alpha", 0.25, NULL);
        nemomotion_attach(m, 1.0,
                icon->one, "alpha", 1.0, NULL);
        nemomotion_run(m);
    } else if (strstr(icon->uri, "smartshopping")) {
        timeout = 2000 * 2;
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_LINEAR_TYPE, timeout, 0);
        nemomotion_attach(m, 0.5,
                icon->one, "ro", -45.0, NULL);
        nemomotion_attach(m, 1.0,
                icon->one, "ro", 45.0, NULL);
        nemomotion_run(m);
    } else if (strstr(icon->uri, "culturecenter")) {
        timeout = 5000 * 2;
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_LINEAR_TYPE, timeout, 0);
        nemoshow_item_rotate(icon->one, 0.0);
        nemomotion_attach(m, 1.0,
                icon->one, "ro", 360.0, NULL);
        nemomotion_run(m);
    } else if (strstr(icon->uri, "entertainment")) {
        timeout = 2500 * 2;
        NemoMotion *m = nemomotion_create(show,
                NEMOEASE_CUBIC_INOUT_TYPE, timeout, 0);
        nemomotion_attach(m, 0.5,
                icon->one, "ty", icon->width/2 + icon->height * 0.3 - 25,
                icon->one, "sy", 0.5, NULL);
        nemomotion_attach(m, 1.0,
                icon->one, "ty", icon->width/2 + icon->height * 0.3 - 50,
                icon->one, "sy", 1.0, NULL);
        nemomotion_run(m);
    }
    nemotimer_set_timeout(timer, timeout + 100);
    nemoshow_dispatch_frame(icon->view->show);
}

Icon *backgroundview_create_icon(BackgroundView *view, const char *uri, double rx, double ry)
{
    Icon *icon = calloc(sizeof(Icon), 1);
    icon->view = view;

    icon->uri = strdup(uri);

    if (file_is_svg(uri)) {
        double ww, hh;
        svg_get_wh(uri, &ww, &hh);
        icon->width = ww * rx;
        icon->height = hh * ry;
    } else if (file_is_image(uri)) {
        int ww, hh;
        image_get_wh(uri, &ww, &hh);
        icon->width = ww * rx;
        icon->height = hh * ry;
    } else {
        ERR("Not supported file type: %s", uri);
        free(icon);
        return NULL;
    }

    struct showone *group;
    struct showone *one;
    icon->group = group = GROUP_CREATE(view->icon_group);
    nemoshow_item_set_width(group, icon->width);
    nemoshow_item_set_height(group, icon->height);
    nemoshow_item_pivot(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 0.0);

    icon->timer = TOOL_ADD_TIMER(view->tool, 0, _icon_timeout, icon);
    icon->move_timer = TOOL_ADD_TIMER(view->tool, 0, _icon_move_timeout, icon);
    icon->color_timer = TOOL_ADD_TIMER(view->tool, 0, _icon_color_timeout, icon);

    if (file_is_svg(uri)) {
        icon->one = one = SVG_PATH_GROUP_CREATE(group, icon->width, icon->height, uri);
    } else if (file_is_image(uri)) {
        icon->one = one = IMAGE_CREATE(group, icon->width, icon->height, uri);
    } else {
        ERR("Not supported file type: %s", uri);
        free(icon);
        return NULL;
    }
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_tag(one, 0x111);
    nemoshow_one_set_userdata(one, icon);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    //nemoshow_item_set_fill_color(one, RGBA(WHITE));
    return icon;
}

void icon_revoke(Icon *icon)
{
    struct nemoshow *show = icon->group->show;
    nemoshow_revoke_transition_one(show, icon->group, "tx");
    nemoshow_revoke_transition_one(show, icon->group, "ty");
    nemoshow_revoke_transition_one(show, icon->group, "sx");
    nemoshow_revoke_transition_one(show, icon->group, "sy");
    nemoshow_revoke_transition_one(show, icon->group, "ro");

    nemoshow_revoke_transition_one(show, icon->one, "sx");
    nemoshow_revoke_transition_one(show, icon->one, "sy");
    nemoshow_revoke_transition_one(show, icon->one, "tx");
    nemoshow_revoke_transition_one(show, icon->one, "ty");
    nemoshow_revoke_transition_one(show, icon->one, "alpha");
    nemoshow_revoke_transition_one(show, icon->one, "ro");
}

void icon_show(Icon *icon, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(icon->group, 0.5);
    }
    nemotimer_set_timeout(icon->timer, 100 + delay);
    nemotimer_set_timeout(icon->move_timer, 100 + delay);
}

void icon_hide(Icon *icon, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(icon->group, 0.0);
    }
    nemotimer_set_timeout(icon->timer, 0);
    nemotimer_set_timeout(icon->move_timer, 0);
}

void icon_rotate(Icon *icon, uint32_t easetype, int duration, int delay, double ro)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "ro", ro,
                NULL);
    } else {
        nemoshow_item_rotate(icon->group, ro);
    }
}

void icon_scale(Icon *icon, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "sx", sx, "sy", sy,
                NULL);
    } else {
        nemoshow_item_scale(icon->group, sx, sy);
    }
}

void icon_translate(Icon *icon, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(icon->group, tx, ty);
    }
}

void icon_get_center(Icon *icon, void *event, double *cx, double *cy)
{
    RET_IF(!icon);
    int cnt = list_count(icon->grabs);
    RET_IF(cnt <= 0);

    double sumx = 0, sumy = 0;
    List *l;
    Grab *g;
    LIST_FOR_EACH(icon->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                sumx += nemoshow_event_get_x_on(event, i);
                sumy += nemoshow_event_get_y_on(event, i);
                break;
            }
        }
    }
    if (cx) *cx = sumx/cnt;
    if (cy) *cy = sumy/cnt;
}

void icon_rotate_init(Icon *icon, NemoWidget *widget, void *event)
{
    if (list_count(icon->grabs) < 2) return;

    // Update index
    List *l;
    Grab *g;
    LIST_FOR_EACH(icon->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                break;
            }
        }
        if (i >= nemoshow_event_get_tapcount(event)) {
            ERR("grab is lost!!!!!");
            continue;
        }
        g->idx = i;
    }

    // Find far distance
    double x0, x1, y0, y1;
    double dist = 0;
    LIST_FOR_EACH(icon->grabs, l, g) {
        List *ll;
        Grab *gg;
        LIST_FOR_EACH(icon->grabs, ll, gg) {
            double _x0, _y0, _x1, _y1;
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, g->idx),
                    nemoshow_event_get_y_on(event, g->idx), &_x0, &_y0);
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, gg->idx),
                    nemoshow_event_get_y_on(event, gg->idx), &_x1, &_y1);
            double _dist = DISTANCE(_x0, _y0, _x1, _y1);
            if (dist < _dist) {
                dist = _dist;
                x0 = _x0;
                y0 = _y0;
                x1 = _x1;
                y1 = _y1;
                icon->grab_dev0 = g->device;
                icon->grab_dev1 = gg->device;
            }
        }
    }
    icon->grab_ro = nemoshow_item_get_rotate(icon->group) + atan2f(x0 - x1, y0 - y1) * 180/M_PI;
}

void icon_do_rotate(Icon *icon, NemoWidget *widget, void *event)
{
    if (list_count(icon->grabs) < 2) return;

    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int i;
    for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
        if (icon->grab_dev0 == nemoshow_event_get_device_on(event, i)) {
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, i),
                    nemoshow_event_get_y_on(event, i), &x0, &y0);
            continue;
        } else if (icon->grab_dev1 == nemoshow_event_get_device_on(event, i)) {
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, i),
                    nemoshow_event_get_y_on(event, i), &x1, &y1);
            continue;
        }
    }
    double r1 = atan2f(x0 - x1, y0 - y1);
    icon_rotate(icon, 0, 0, 0, icon->grab_ro - r1 * 180/M_PI);
}

void icon_do_scale(Icon *icon, NemoWidget *widget, void *event)
{
    if (list_count(icon->grabs) < 2) return;

    double cx, cy;
    icon_get_center(icon, event, &cx, &cy);

    // Scale
    int k = 0;
    double sumdiff = 0;

    List *l;
    Grab *g;
    LIST_FOR_EACH(icon->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                double xx, yy;
                xx = nemoshow_event_get_x_on(event, i);
                yy= nemoshow_event_get_y_on(event, i);
                sumdiff += DISTANCE(cx, cy, xx, yy);
                k++;
                break;
            }
        }
    }

    double coeff = 0.02;
    double scale = sumdiff/k;
    if (icon->grab_scale_x + (double)(scale - icon->grab_scale_sum) * coeff <= 5.0 &&
            icon->grab_scale_x + (double)(scale - icon->grab_scale_sum) * coeff <= 5.0 &&
            icon->grab_scale_x + (double)(scale - icon->grab_scale_sum) * coeff >= 0.5 &&
            icon->grab_scale_x + (double)(scale - icon->grab_scale_sum) * coeff >= 0.5) {
        icon_scale(icon, 0, 0, 0,
                icon->grab_scale_x + (double)(scale - icon->grab_scale_sum) * coeff,
                icon->grab_scale_y + (double)(scale - icon->grab_scale_sum) * coeff);
    }
}

void icon_scale_init(Icon *icon, NemoWidget *widget, void *event)
{
    if (list_count(icon->grabs) < 2) return;
    //nemoshow_item_set_anchor(icon->group, cx, cy);

    double cx, cy;
    icon_get_center(icon, event, &cx, &cy);

    int k = 0;
    double sum = 0;
    List *l;
    Grab *g;
    LIST_FOR_EACH(icon->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                sum += DISTANCE(cx, cy,
                        nemoshow_event_get_x_on(event, i),
                        nemoshow_event_get_y_on(event, i));
                k++;
                break;
            }
        }
    }
    icon->grab_scale_sum = sum/k;
    icon->grab_scale_x = nemoshow_item_get_scale_x(icon->group);
    icon->grab_scale_y = nemoshow_item_get_scale_y(icon->group);
}

void icon_do_move(Icon *icon, NemoWidget *widget, void *event)
{
    double cx, cy;
    icon_get_center(icon, event, &cx, &cy);

    // history update
    icon->prev_t[icon->prev_idx] = nemoshow_event_get_time(event);
    icon->prev_x[icon->prev_idx] = cx;
    icon->prev_y[icon->prev_idx] = cy;
    icon->prev_idx++;
    if (icon->prev_idx >= icon->view->icon_history_cnt) {
        icon->prev_idx = 0;
    }

    icon_translate(icon, 0, 0, 0,
            cx + icon->grab_diff_x,
            cy + icon->grab_diff_y);
}

void icon_do_move_up(Icon *icon, NemoWidget *widget, void *event)
{
    BackgroundView *view = icon->view;
    int history_cnt = icon->view->icon_history_cnt;
    // Move
    if (list_count(icon->grabs) >= 1) {
        double cx = 0, cy = 0;
        icon_get_center(icon, event, &cx, &cy);

        // diff update
        double tx, ty;
        tx = nemoshow_item_get_translate_x(icon->group);
        ty = nemoshow_item_get_translate_y(icon->group);
        icon->grab_diff_x = tx - cx;
        icon->grab_diff_y = ty - cy;

        // history update
        icon->prev_t[icon->prev_idx] = nemoshow_event_get_time(event);
        icon->prev_x[icon->prev_idx] = cx;
        icon->prev_y[icon->prev_idx] = cy;
        icon->prev_idx++;
        if (icon->prev_idx >= history_cnt) {
            icon->prev_idx = 0;
        }
    } else {
        icon->grab_diff_x = 0;
        icon->grab_diff_y = 0;

        double sum = 0;
        int i, j, k = 0;
        int start = -9999, end;
        i = icon->prev_idx;

        do {
            if (icon->prev_t[i] == 0) {
                i++;
                if (i >= history_cnt) i = 0;
                continue;
            }

            j = i + 1;
            if (j >= history_cnt) j = 0;

            if (icon->prev_t[j] == 0) break;

            if (abs(icon->prev_x[i] - icon->prev_x[j]) <= view->icon_history_min_dist &&
                    abs(icon->prev_y[i] - icon->prev_y[i] <= view->icon_history_min_dist)) {
                i++;
                if (i >= history_cnt) i = 0;
                continue;
            }

            int tdiff = icon->prev_t[j] - icon->prev_t[i];
            if (tdiff <= 0) {
                tdiff = 8;
            }

            if (start < -100) start = i;
            end = i;

            sum += DISTANCE(icon->prev_x[j], icon->prev_y[j],
                    icon->prev_x[i], icon->prev_y[i])/tdiff;
            k++;
            i++;
            if (i >= history_cnt) i = 0;
        } while (i != icon->prev_idx);

        if (k < 5) {
            nemotimer_set_timeout(icon->move_timer, 100);
        } else {
            double dir = atan2f((icon->prev_y[start] - icon->prev_y[end]),
                    (icon->prev_x[start] - icon->prev_x[end]));
            double avg = (sum/k) * view->icon_throw_coeff;

            double tx = -avg * cos(dir) + icon->prev_x[end];
            double ty = -avg * sin(dir) + icon->prev_y[end];
            if (tx <= 0) tx = 0;
            if (tx >= view->width) tx = view->width;
            if (ty <= 0) ty = 0;
            if (ty >= view->height) ty = view->height;

            if (abs(nemoshow_item_get_translate_x(icon->group) - tx) > view->icon_throw_min_dist ||
                    abs(nemoshow_item_get_translate_y(icon->group) - ty) > view->icon_throw_min_dist) {
                _nemoshow_item_motion(icon->group, NEMOEASE_CUBIC_OUT_TYPE, view->icon_throw_duration, 0,
                        "tx", tx, "ty", ty, NULL);
                if (WELLRNG512()%2 == 0) {
                    nemosound_play(BACKGROUND_SOUND_DIR"/boom.wav");
                } else {
                    nemosound_play(BACKGROUND_SOUND_DIR"/throwing.wav");
                }
            }
            nemotimer_set_timeout(icon->move_timer, view->icon_throw_duration + 100);
        }
        nemotimer_set_timeout(icon->color_timer, 0);
    }
}

void icon_move_init(Icon *icon, NemoWidget *widget, void *event)
{
    // diff update
    double cx, cy;
    icon_get_center(icon, event, &cx, &cy);

    double tx, ty;
    tx = nemoshow_item_get_translate_x(icon->group);
    ty = nemoshow_item_get_translate_y(icon->group);
    icon->grab_diff_x = tx - cx;
    icon->grab_diff_y = ty - cy;

    // history init
    icon->prev_idx = 0;
    int i;
    for (i = 0 ; i < icon->view->icon_history_cnt ; i++) {
        icon->prev_t[i] = 0;
        icon->prev_x[i] = 0;
        icon->prev_y[i] = 0;
    }
    nemotimer_set_timeout(icon->color_timer, 20);
}

void icon_append_grab(Icon *icon, NemoWidget *widget, void *event)
{
    Grab *g = grab_create(nemoshow_event_get_device(event));
    icon->grabs = list_append(icon->grabs, g);

    icon_move_init(icon, widget, event);
    icon_rotate_init(icon, widget, event);
    icon_scale_init(icon, widget, event);
}

void icon_remove_grab(Icon *icon, NemoWidget *widget, void *event)
{
    List *l;
    Grab *g;
    LIST_FOR_EACH(icon->grabs, l, g) {
        if (g->device == nemoshow_event_get_device(event)) {
            break;
        }
    }
    RET_IF(!g);
    icon->grabs = list_remove(icon->grabs, g);

    icon_rotate_init(icon, widget, event);
    icon_scale_init(icon, widget, event);
    icon_do_move_up(icon, widget, event);
}

static void _icon_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    Icon *icon = userdata;
    BackgroundView *view = icon->view;

    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        icon_append_grab(icon, widget, event);
        if (list_count(icon->grabs) == 1) {
            // Revoke all transitions
            nemoshow_revoke_transition_one(show, icon->group, "tx");
            nemoshow_revoke_transition_one(show, icon->group, "ty");
            nemoshow_revoke_transition_one(show, icon->group, "sx");
            nemoshow_revoke_transition_one(show, icon->group, "sy");
            nemoshow_revoke_transition_one(show, icon->group, "ro");
            nemotimer_set_timeout(icon->move_timer, 0);
        }
    } else if (nemoshow_event_is_motion(show, event)) {
        // Move
        Grab *g = LIST_DATA(LIST_FIRST(icon->grabs));
        // XXX: To reduce duplicated caclulation
        if (g->device == nemoshow_event_get_device(event)) {
            icon_do_move(icon, widget, event);
            icon_do_scale(icon, widget, event);
            icon_do_rotate(icon, widget, event);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        icon_remove_grab(icon, widget, event);
    }
    nemoshow_dispatch_frame(view->show);
}

static void _background_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    BackgroundView *view = userdata;
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->icon_widget, ex, ey);
        if (one && 0x111 == nemoshow_one_get_tag(one)) {
            Icon *icon = nemoshow_one_get_userdata(one);
            nemowidget_create_grab(widget, event,
                    _icon_grab_event, icon);
            // XXX: To prevent drawing in the sketch
            nemoshow_event_set_done(event);
        } else if (!view->sketch) {
            struct nemotool *tool = nemowidget_get_tool(widget);
            uint64_t device = nemoshow_event_get_device(event);
            float vx, vy;
            nemoshow_transform_to_viewport(show, ex, ey, &vx, &vy);
            nemotool_touch_bypass(tool, device, vx, vy);
        }
    }

#if 0
    struct showone *one;
    if (nemoshow_event_is_pointer_enter(show, event)) {
        one = CIRCLE_CREATE(nemowidget_get_canvas(widget), 100);
        nemoshow_item_set_fill_color(one, RGBA(WHITE));
        nemoshow_item_translate(one, ex, ey);
    } else if (nemoshow_event_is_pointer_motion(show, event)) {
        nemoshow_item_translate(one, ex, ey);
    } else if (nemoshow_event_is_pointer_leave(show, event)) {
        nemoshow_item_set_alpha(one, 0.0);
    }
#endif
}

static void _sketch_timeout(struct nemotimer *timer, void *userdata)
{
    BackgroundView *view = userdata;
    sketch_remove_old(view->sketch, view->sketch_timeout);
    nemotimer_set_timeout(timer, view->sketch_timeout);
    nemoshow_dispatch_frame(view->show);
}

static void _background_gallery_timeout(struct nemotimer *timer, void *userdata)
{
    BackgroundView *view = userdata;

    GalleryItem *gallery_it;
    gallery_it = LIST_DATA(list_get_nth(view->gallery->items, view->gallery_idx));
    gallery_item_hide(gallery_it, NEMOEASE_LINEAR_TYPE, view->gallery_duration, 0);

    view->gallery_idx++;
    if (view->gallery_idx >= list_count(view->gallery->items))
        view->gallery_idx = 0;

    gallery_it = LIST_DATA(list_get_nth(view->gallery->items, view->gallery_idx));
    gallery_item_show(gallery_it, NEMOEASE_LINEAR_TYPE, view->gallery_duration, 0);

    nemotimer_set_timeout(timer, view->gallery_timeout);
    nemoshow_dispatch_frame(view->show);
}

BackgroundView *background_create(NemoWidget *parent, ConfigApp *app)
{
    BackgroundView *view = calloc(sizeof(BackgroundView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->parent = parent;
    view->width = app->config->width;
    view->height = app->config->height;
    view->gallery_timeout = app->bgtimeout;
    view->gallery_duration = app->bgduration;
    view->sketch_timeout = app->sketch_timeout;
    view->icon_history_cnt = app->icon_history_cnt;
    view->icon_history_min_dist = app->icon_history_min_dist;
    view->icon_throw_min_dist = app->icon_throw_min_dist;
    view->icon_throw_coeff = app->icon_throw_coeff;
    view->icon_throw_duration = app->icon_throw_duration;

    NemoWidget *widget;
    struct showone *group;

    if (file_is_dir(app->bgpath)) {
        view->bg_widget = widget = nemowidget_create_vector(parent, view->width, view->height);
        nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

        struct showone *canvas = nemowidget_get_canvas(view->bg_widget);

        FileInfo *file;
        List *bgfiles = fileinfo_readdir(app->bgpath);

        if (list_count(bgfiles) > 0) {
            Gallery *gallery;
            view->gallery = gallery = gallery_create(view->tool, canvas, view->width, view->height);
            if (app->config->layer && !strcmp(app->config->layer, "background")) {
                gallery_set_bg_color(view->gallery, WHITE);
            }
            view->gallery_timer = TOOL_ADD_TIMER(view->tool, 0, _background_gallery_timeout, view);

            LIST_FREE(bgfiles, file) {
                if (file->is_dir) continue;
                gallery_append_item(gallery, file->path);
                fileinfo_destroy(file);
            }

            view->sketch = sketch_create(parent, view->width, view->height);
            sketch_set_min_distance(view->sketch, app->sketch_min_dist);
            sketch_set_dot_count(view->sketch, app->sketch_dot_cnt);
            sketch_set_size(view->sketch, 3);
            view->sketch_timer = TOOL_ADD_TIMER(view->tool, view->sketch_timeout, _sketch_timeout, view);
        }
    } else if (file_is_image(app->bgpath)) {
        view->bg_widget = widget = nemowidget_create_vector(parent, view->width, view->height);
        nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

        struct showone *canvas = nemowidget_get_canvas(view->bg_widget);

        Gallery *gallery;
        view->gallery = gallery = gallery_create(view->tool, canvas, view->width, view->height);
        if (app->config->layer && !strcmp(app->config->layer, "background")) {
            gallery_set_bg_color(view->gallery, WHITE);
        }
        view->gallery_timer = TOOL_ADD_TIMER(view->tool, 0, _background_gallery_timeout, view);

        gallery_append_item(gallery, app->bgpath);

        view->sketch = sketch_create(parent, view->width, view->height);
        sketch_set_min_distance(view->sketch, app->sketch_min_dist);
        sketch_set_dot_count(view->sketch, app->sketch_dot_cnt);
        sketch_set_size(view->sketch, 3);
        view->sketch_timer = TOOL_ADD_TIMER(view->tool, view->sketch_timeout, _sketch_timeout, view);
    } else if (file_is_video(app->bgpath)) {
        PlayerUI *player;
        view->player = player = nemoui_player_create(parent,
                view->width, view->height, app->bgpath, false, -1, false);
    }

    if (app->icons) {
        view->icon_widget = widget = nemowidget_create_vector(parent, view->width, view->height);
        nemowidget_append_callback(widget, "event", _background_event, view);
        nemowidget_enable_event_repeat(widget, true);
        view->icon_group = GROUP_CREATE(nemowidget_get_canvas(widget));

        double rx, ry;
        rx = (double)view->width/3240;
        ry = (double)view->height/1920;

        int cnt = list_count(app->icons);
        int i;
        for (i = 0 ; i < app->icon_cnt ; i++) {
            char *path = LIST_DATA(list_get_nth(app->icons, i%cnt));

            ERR("%s", path);
            int tx, ty;
            tx = WELLRNG512()%view->width;
            ty = WELLRNG512()%view->height;

            Icon *icon;
            icon = backgroundview_create_icon(view, path, rx, ry);
            icon_translate(icon, 0, 0, 0, tx, ty);
            view->icons = list_append(view->icons, icon);
        }
    }

    return view;
}

void background_show(BackgroundView *view)
{
    if (view->bg_widget) {
        nemowidget_show(view->bg_widget, 0, 0, 0);
        nemowidget_set_alpha(view->bg_widget, 0, 0, 0, 1.0);
    }
    if (view->gallery) {
        int easetype = NEMOEASE_CUBIC_OUT_TYPE;
        gallery_show(view->gallery, easetype, 1000, 0);
        int cnt = list_count(view->gallery->items);
        if (cnt == 1) {
            gallery_item_show(LIST_DATA(LIST_FIRST(view->gallery->items)), easetype, 500, 0);
        } else if (cnt > 1)
            nemotimer_set_timeout(view->gallery_timer, 100);
    }
    if (view->sketch) {
        sketch_show(view->sketch, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
        sketch_enable(view->sketch, true);
        nemotimer_set_timeout(view->sketch_timer, 1000 + 100);
    }

    if (view->icon_widget)
        nemowidget_show(view->icon_widget, 0, 0, 0);

    int delay = 0;
    List *l;
    Icon *icon;
    LIST_FOR_EACH(view->icons, l, icon) {
        icon_show(icon, NEMOEASE_CUBIC_OUT_TYPE, 1000, delay);
        delay += 250;
    }

    nemoshow_dispatch_frame(view->show);
}

void background_hide(BackgroundView *view)
{
    if (view->bg_widget) {
        nemowidget_hide(view->bg_widget, 0, 0, 0);
        // Do not hide for compositing, it's background
        //nemowidget_set_alpha(view->bg_widget, 0, 0, 0, 0.0);
    }
    if (view->gallery) {
        gallery_hide(view->gallery, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
        nemotimer_set_timeout(view->gallery_timer, 0);
    }
    if (view->sketch) {
        sketch_hide(view->sketch, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
        nemotimer_set_timeout(view->sketch_timer, 0);
    }

    if (view->icon_widget)
        nemowidget_show(view->icon_widget, 0, 0, 0);

    List *l;
    Icon *icon;
    LIST_FOR_EACH(view->icons, l, icon) {
        icon_revoke(icon);
        icon_hide(icon, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    }

    nemoshow_dispatch_frame(view->show);
}

static void _background_win_layer(NemoWidget *win, const char *id, void *info, void *userdata)
{
    BackgroundView *view = userdata;
    int32_t visible = (intptr_t)info;
    if (visible == -1) {
        if (view->visible) {
            background_hide(view);
            view->visible = false;
        }
    } else {
        if (!view->visible) {
            background_show(view);
            view->visible = true;
        }
    }
}

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->bgtimeout = 60000;
    app->bgduration = 2000;
    app->sketch_timeout = 3000;
    app->sketch_min_dist = 10.0;
    app->sketch_dot_cnt = 60;
    app->icon_cnt = 0;
    app->icon_history_cnt = 5;
    app->icon_history_min_dist = 10;
    app->icon_throw_min_dist = 100;
    app->icon_throw_coeff = 200;
    app->icon_throw_duration = 1000;

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

    snprintf(buf, PATH_MAX, "%s/background", appname);
    temp = xml_get_value(xml, buf, "path");
    if (!temp) {
        ERR("No background path in %s", appname);
    } else {
        app->bgpath = strdup(temp);
    }
    temp = xml_get_value(xml, buf, "timeout");
    if (!temp) {
        ERR("No background timeout in %s", appname);
    } else {
        app->bgtimeout = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "duration");
    if (!temp) {
        ERR("No background duration in %s", appname);
    } else {
        app->bgduration = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/sketch", appname);
    temp = xml_get_value(xml, buf, "timeout");
    if (!temp) {
        ERR("No sketch timeout in %s", appname);
    } else {
        app->sketch_timeout = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "min_distance");
    if (!temp) {
        ERR("No sketch min_distance in %s", appname);
    } else {
        app->sketch_min_dist = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "dot_count");
    if (!temp) {
        ERR("No sketch dot_count in %s", appname);
    } else {
        app->sketch_dot_cnt = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/icon", appname);
    temp = xml_get_value(xml, buf, "count");
    if (!temp) {
        ERR("No icon count in %s", appname);
    } else {
        app->icon_cnt = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "history_count");
    if (!temp) {
        ERR("No icon history_count in %s", appname);
    } else {
        app->icon_history_cnt = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "history_min_distance");
    if (!temp) {
        ERR("No icon history_min_distance in %s", appname);
    } else {
        app->icon_history_min_dist = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "throw");
    if (!temp) {
        ERR("No icon throw in %s", appname);
    } else {
        app->icon_throw = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "throw_min_distance");
    if (!temp) {
        ERR("No icon throw_min_distance in %s", appname);
    } else {
        app->icon_throw_min_dist = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "throw_coefficient");
    if (!temp) {
        ERR("No icon throw_coeff in %s", appname);
    } else {
        app->icon_throw_coeff = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "throw_duration");
    if (!temp) {
        ERR("No icon throw_duration in %s", appname);
    } else {
        app->icon_throw_duration = atoi(temp);
    }

    snprintf(buf, PATH_MAX, "%s/icons", appname);
    List *tags  = xml_search_tags(xml, buf);
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        List *ll;
        XmlAttr *attr;
        LIST_FOR_EACH(tag->attrs, ll, attr) {
            if (!strcmp(attr->key, "path")) {
                if (attr->val) {
                    char *path = strdup(attr->val);
                    app->icons = list_append(app->icons, path);
                }
            }
        }
    }

    xml_unload(xml);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    char *path;
    LIST_FREE(app->icons, path) free(path);
    if (app->bgpath) free(app->bgpath);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    WELLRNG512_INIT();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);

    BackgroundView *view = background_create(win, app);
    nemowidget_append_callback(win, "layer", _background_win_layer, view);
    background_show(view);

    nemoshow_dispatch_frame(view->show);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
