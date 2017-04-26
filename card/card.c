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

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

typedef struct _Mirror Mirror;
struct _Mirror {
    int x, y, width, height;
    char *target;
};

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
typedef struct _Card Card;
struct _Card {
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
    Card *card;
    CardItem *org;
    int width, height;
    struct showone *group;
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
    nemoshow_dispatch_frame(it->card->show);
}

CardItem *card_item_dup(CardItem *it)
{
    Card *card = it->card;
    CardItem *dup = calloc(sizeof(CardItem), 1);
    dup->card = card;
    dup->width = it->width;;
    dup->height = it->height;
    dup->type = strdup(it->type);
    dup->bg_uri = strdup(it->bg_uri);
    if (it->icon_uri) dup->icon_uri = strdup(it->icon_uri);
    if (it->icon_anim_type) dup->icon_anim_type = strdup(it->icon_anim_type);
    if (it->txt_uri) dup->txt_uri = strdup(it->txt_uri);
    dup->exec = strdup(it->exec);
    dup->resize = it->resize;
    dup->sxy = it->sxy;
    if (it->mirror) dup->mirror = strdup(it->mirror);
    dup->org = it;

    struct showone *group;
    dup->group = group = GROUP_CREATE(nemowidget_get_canvas(card->widget));
    nemoshow_item_set_width(group, dup->width);
    nemoshow_item_set_height(group, dup->height);
    nemoshow_item_pivot(group, 0.5, 0.5);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 1.0);
    nemoshow_item_rotate(group, nemoshow_item_get_rotate(it->group));
    dup->ro = nemoshow_item_get_rotate(it->group);
    nemoshow_item_translate(group,
            nemoshow_item_get_translate_x(it->group),
            nemoshow_item_get_translate_y(it->group));
    nemoshow_item_scale(group, 0.0, 0.0);

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
        dup->icon_timer = TOOL_ADD_TIMER(card->tool, 0, _card_item_icon_timeout, dup);

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

CardItem *card_create_item(Card *card, double global_sxy, const char *type, const char *bg_uri, char *icon_uri,  const char *icon_anim_type, const char *txt_uri, const char *exec, bool resize, double sxy, char *mirror, ConfigApp *app)
{
    CardItem *it = calloc(sizeof(CardItem), 1);
    it->card = card;
    it->width = card->iw;
    it->height = card->ih;
    it->type = strdup(type);
    it->bg_uri = strdup(bg_uri);
    if (icon_uri) it->icon_uri = strdup(icon_uri);
    if (icon_anim_type) it->icon_anim_type = strdup(icon_anim_type);
    if (txt_uri) it->txt_uri = strdup(txt_uri);
    it->exec = strdup(exec);
    it->resize = resize;
    it->sxy = sxy;
    if (mirror) it->mirror = strdup(mirror);
	it->grab_time = 0;

    struct showone *group;
    it->group = group = GROUP_CREATE(card->item_group);
    nemoshow_item_set_width(group, it->width);
    nemoshow_item_set_height(group, it->height);
    nemoshow_item_pivot(group, 0.5, 0.5);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);

    struct showone *one;
    it->bg0 = one = RECT_CREATE(group, it->width, it->height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_tag(one, 0x999);
    nemoshow_one_set_userdata(one, it);

    it->bg = one = IMAGE_CREATE(group, it->width, it->height, it->bg_uri);

    if (it->icon_uri) {
        if (file_is_svg(it->icon_uri)) {
            double w, h;
            double ww, hh;
            svg_get_wh(it->icon_uri, &ww, &hh);
            w = ww * global_sxy;
            h = hh * global_sxy;
            it->icon = one = SVG_PATH_CREATE(group, w, h, it->icon_uri);
            nemoshow_item_set_fill_color(one, RGBA(0xFFFFFFFF));
        } else if (file_is_image(it->icon_uri)) {
            double w, h;
            int ww, hh;
            image_get_wh(it->icon_uri, &ww, &hh);
            w = ww * global_sxy;
            h = hh * global_sxy;
            it->icon = one = IMAGE_CREATE(group, w, h, it->icon_uri);
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
            it->icon_timer = TOOL_ADD_TIMER(card->tool, 0, _card_item_icon_timeout, it);
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
        it->txt = one = SVG_PATH_GROUP_CREATE(group, w, h, it->txt_uri);
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
    free(it->exec);
    free(it);
}

CardItem *card_append_item(Card *card, double global_sxy, const char *type, const char *bg_uri, char *icon_uri, char *icon_anim_type, const char *txt_uri, const char *exec, bool resize, double sxy, char *mirror, ConfigApp *app)
{
    CardItem *it;
    it = card_create_item(card, global_sxy, type, bg_uri, icon_uri, icon_anim_type, txt_uri, exec, resize, sxy, mirror, app);
    RET_IF(!it, NULL);
    card->items = list_append(card->items, it);

    return it;
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
    TOOL_ADD_TIMER(it->card->tool, duration + delay + 100, _card_item_hide_destroy, it);
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
void card_item_rotate(CardItem *it, uint32_t easetype, int duration, int delay, double ro)
{
    if (EQUAL(it->ro, ro)) return;
    it->ro = ro;
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "ro", ro,
                NULL);
    } else {
        nemoshow_item_rotate(it->group, ro);
    }
}

void card_show(Card *card)
{
    nemowidget_show(card->widget, 0, 0, 0);
    nemowidget_show(card->event, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_OUT_TYPE;
    int duration = 1000;
    int delay = 150;

    card->start = 0;

    int i;
    for (i = 0; i < card->cnt; i++) {
        CardItem *it = LIST_DATA(list_get_nth(card->items, i));
        if (!it) {
            ERR("item is NULL at index(%d)", i);
            break;
        }

        if (i != 0) {
            card_item_rotate(it, easetype, duration, delay * i, card->iro);
            card_item_show(it, easetype, duration, delay * i);
            card_item_set_alpha(it, easetype, duration, delay * i, 1.0);
            card_item_scale(it, easetype, duration, delay * i, 1.0, 1.0);
        }
        card_item_translate(it, easetype, duration, delay * i,
                card->ix + (card->iw + card->igap) * i, card->iy);
    }

    nemotimer_set_timeout(card->timer, duration + delay * (i - 1) + 10);
    nemotimer_set_timeout(card->guide_timer, delay * (i - 1) + 10);

    nemoshow_dispatch_frame(card->show);
}

static void card_item_revoke(CardItem *it)
{
    struct nemoshow *show = it->group->show;
    nemoshow_revoke_transition_one(show, it->group, "alpha");
    nemoshow_revoke_transition_one(show, it->group, "ro");
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

void card_hide(Card *card)
{
    nemowidget_hide(card->widget, 0, 0, 0);
    nemowidget_hide(card->event, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_IN_TYPE;
    int duration = 500;
    List *l;
    CardItem *it;
    LIST_FOR_EACH(card->items, l, it) {
        card_item_revoke(it);
        card_item_hide(it, easetype, duration, 0);
        card_item_rotate(it, easetype, duration, 0, 0);
        card_item_scale(it, easetype, duration, 0, 0.0, 0.0);
        card_item_translate(it, easetype, duration, 0, card->ix, card->iy);
    }

    nemotimer_set_timeout(card->timer, 0);

    nemotimer_set_timeout(card->guide_timer, 0);
    card_guide_hide(card->guide[0]);
    card_guide_hide(card->guide[1]);
    card_guide_hide(card->guide[2]);
    card_guide_hide(card->guide[3]);

    nemoshow_dispatch_frame(card->show);
}

static void _card_timeout(struct nemotimer *timer, void *userdata)
{
    Card *card = userdata;

    int cnt = list_count(card->items);
    if (cnt <= card->cnt) {
        ERR("Number(%d) of appended items should be bigger than number(%d) of viewable items",
                card->cnt, list_count(card->items));
        return;
    }

    List *l;
    l = list_get_nth(card->items, card->start);
    RET_IF(!l);

    int i, j;
    j = card->start;
    for (i = 0; i < card->cnt ; i++) {
        uint32_t easetype = NEMOEASE_LINEAR_TYPE;
        int duration = card->item_duration - 20;

        CardItem *it = LIST_DATA(list_get_nth(card->items, j));
        if (!it) {
            ERR("item is NULL at index(%d)", j);
            break;
        }

        card_item_show(it, NEMOEASE_CUBIC_OUT_TYPE, duration, 0);
        if (i == 0) {
            card_item_rotate(it, 0, 0, 0, 0.0);
            card_item_translate(it, 0, 0, 0, card->ix, card->iy);

            card_item_rotate(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, card->iro);
            card_item_set_alpha(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 1.0);
            card_item_scale(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 1.0, 1.0);
        } else if (i == card->cnt - 1) {
            card_item_rotate(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0);
            card_item_set_alpha(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0);
            card_item_scale(it, NEMOEASE_CUBIC_IN_TYPE, duration, 0, 0.0, 0.0);
        }
        card_item_translate(it, easetype, duration, 0,
                card->ix + (card->iw + card->igap) * (i + 1), card->iy);
        j++;
        if ( j >= cnt ) {
            j = 0;
        }
    }

    card->start--;
    if (card->start < 0) {
        card->start = cnt - 1;
    }

    nemotimer_set_timeout(timer, card->item_duration);
    nemoshow_dispatch_frame(card->show);
}

static void _card_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    CardItem *it = userdata;
    Card *card = it->card;

    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
		uint32_t grab_time = nemoshow_event_get_time(event);
		if (grab_time - it->grab_time >= card->item_grab_min_time) {
			it->grab_time = nemoshow_event_get_time(event);
			CardItem *dup = card_item_dup(it);
			card_item_show(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
			card_item_scale(dup, NEMOEASE_CUBIC_OUT_TYPE, 250, 0, 1.0, 1.0);
			nemowidget_grab_set_data(grab, "item", dup);
		}
    } else if (nemoshow_event_is_motion(show, event)) {
        CardItem *dup = nemowidget_grab_get_data(grab, "item");
		if (!dup) return;
        card_item_translate(dup, 0, 0, 0, ex, ey);

        double ro = 0.0;
        if (!strcmp(card->launch_type, "table")) {
            if (!RECTS_CROSS(card->cx - card->cw/2.0, card->cy - card->ch/2.0,
                        card->cw, card->ch, ex, ey, 5, 5)) {

                int rw, rh;
                rw = (card->width - card->cw)/2;
                rh = (card->height - card->ch)/2;
                if (RECTS_CROSS(0, rh + card->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 45;
                } else if (RECTS_CROSS(0, rh, rw, card->ch, ex, ey, 1, 1)) {
                    ro = 90;
                } else if (RECTS_CROSS(0, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 135;
                } else if (RECTS_CROSS(rw, 0, card->cw, rh, ex, ey, 1, 1)) {
                    ro = 180;
                } else if (RECTS_CROSS(rw + card->cw, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 225;
                } else if (RECTS_CROSS(rw + card->cw, rh, rw, card->ch, ex, ey, 1, 1)) {
                    ro = 270;
                } else if (RECTS_CROSS(rw + card->cw, rh + card->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 315;
                }
            }
            card_item_rotate(dup, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, ro);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        CardItem *itt = nemowidget_grab_get_data(grab, "item");
		if (!itt) return;
        if (RECTS_CROSS(card->cx - card->cw/2.0, card->cy - card->ch/2.0,
                    card->cw, card->ch, ex, ey, 5, 5)) {
            card_item_hide_destroy(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            card_item_scale(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0, 0.0);
            if (itt->org) {
                int tx, ty;
                tx = nemoshow_item_get_translate_x(itt->org->group);
                ty = nemoshow_item_get_translate_y(itt->org->group);
                card_item_translate(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, tx, ty);
            }
        } else {
            card_item_hide_destroy(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            card_item_scale(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0, 0.0);
            card_item_rotate(itt, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0);

            float x, y;
            nemoshow_transform_to_viewport(show,
                    NEMOSHOW_ITEM_AT(itt->group, tx),
                    NEMOSHOW_ITEM_AT(itt->group, ty),
                    &x, &y);

            double ro = 0.0;

            if (!strcmp(card->launch_type, "table")) {
                int rw, rh;
                rw = (card->width - card->cw)/2;
                rh = (card->height - card->ch)/2;
                if (RECTS_CROSS(0, rh + card->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 45;
                } else if (RECTS_CROSS(0, rh, rw, card->ch, ex, ey, 1, 1)) {
                    ro = 90;
                } else if (RECTS_CROSS(0, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 135;
                } else if (RECTS_CROSS(rw, 0, card->cw, rh, ex, ey, 1, 1)) {
                    ro = 180;
                } else if (RECTS_CROSS(rw + card->cw, 0, rw, rh, ex, ey, 1, 1)) {
                    ro = 225;
                } else if (RECTS_CROSS(rw + card->cw, rh, rw, card->ch, ex, ey, 1, 1)) {
                    ro = 270;
                } else if (RECTS_CROSS(rw + card->cw, rh + card->ch, rw, rh, ex, ey, 1, 1)) {
                    ro = 315;
                }
            }

            nemoshow_view_set_anchor(show, 0.5, 0.5);

            char path[PATH_MAX];
            char name[PATH_MAX];
            char args[PATH_MAX];
            char *buf;
            char *tok;
            buf = strdup(itt->exec);
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
            msg = NEMOMSG_CREATE_CMD(itt->type, path);

            ERR("%lf %lf %s %s %s", x, y, itt->type, path, args);
            one = nemoitem_one_create();
            nemoitem_one_set_attr_format(one, "x", "%f", x);
            nemoitem_one_set_attr_format(one, "y", "%f", y);
            nemoitem_one_set_attr_format(one, "r", "%f", ro);
            nemoitem_one_set_attr_format(one, "sx", "%f", itt->sxy);
            nemoitem_one_set_attr_format(one, "sy", "%f", itt->sxy);
            nemoitem_one_set_attr(one, "owner", card->uuid);
            nemoitem_one_set_attr(one, "resize", itt->resize? "on" : "off");
            nemoitem_one_save_attrs(one, states, ';');
            nemoitem_one_destroy(one);

            nemobus_msg_set_attr(msg, "args", args);
            nemobus_msg_set_attr(msg, "states", states);

            if (itt->mirror) {
                if (strstr(itt->mirror, "/nemoshell/fullscreen"))
                    nemobus_msg_set_attr_format(msg, "mirrorscreen", itt->mirror);
            } else {
                List *l;
                Mirror *mirror;
                LIST_FOR_EACH(card->mirrors, l, mirror) {
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
    }
    nemoshow_dispatch_frame(show);
}

static void _card_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    Card *card = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(card->widget, ex, ey);
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
    Card *card = userdata;
    if (card->guide_idx == 0) {
        card_guide_show(card->guide[0],
                card->cx, card->cy,
                card->cx, card->cy + card->ih/2);

        card_guide_hide(card->guide[1]);
        card_guide_hide(card->guide[2]);
        card_guide_hide(card->guide[3]);
    } else if (card->guide_idx == 1) {
        card_guide_show(card->guide[1],
                card->cx, card->cy,
                card->cx, card->cy - card->ih/2);

        card_guide_hide(card->guide[0]);
        card_guide_hide(card->guide[2]);
        card_guide_hide(card->guide[3]);
    } else if (card->guide_idx == 2) {
        card_guide_show(card->guide[2],
                card->cx - card->cw/2 + card->iw/2, card->cy,
                card->cx - card->cw/2, card->cy);

        card_guide_hide(card->guide[0]);
        card_guide_hide(card->guide[1]);
        card_guide_hide(card->guide[3]);
    } else if (card->guide_idx == 3) {
        card_guide_show(card->guide[3],
                card->cx + card->cw/2 - card->iw/2, card->cy,
                card->cx + card->cw/2, card->cy);

        card_guide_hide(card->guide[0]);
        card_guide_hide(card->guide[1]);
        card_guide_hide(card->guide[2]);
    }
    card->guide_idx++;
    if (card->guide_idx >= 4) card->guide_idx = 0;
    nemotimer_set_timeout(timer, card->guide_duration + 1000);
    nemoshow_dispatch_frame(card->show);
}

Card *card_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    Card *card = calloc(sizeof(Card), 1);
    card->show = nemowidget_get_show(parent);
    card->tool = nemowidget_get_tool(parent);
    card->uuid = nemowidget_get_uuid(parent);
    card->width = width;
    card->height = height;
    card->cnt = app->item_cnt;
    card->iw = app->item_width;
    card->ih = app->item_height;
    card->iro = app->item_ro;
    card->igap = app->item_gap;
    card->item_duration = app->item_duration;
	card->item_grab_min_time = app->item_grab_min_time;
    card->guide_duration = app->guide_duration;
    card->guide_idx = 0;
    card->mirrors = app->mirrors;
    if (app->launch_type) {
        card->launch_type = strdup(app->launch_type);
    } else {
        card->launch_type = strdup("table");
    }

    card->cw = card->iw * card->cnt + card->igap * card->cnt;
    card->ch = card->ih;

    card->cx = card->width * app->item_px;
    card->cy = card->height * app->item_py;

    card->ix = card->cx - card->cw/2.0;
    card->iy = card->cy - card->ch/2.0 + card->ih/2.0;
    double rx, ry;
    rx = (double)app->config->sxy;
    ry = (double)app->config->sxy;

    NemoWidget *widget;
    card->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_enable_event(widget, false);

    struct showone *group;
    card->item_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    struct nemotimer *timer;
    card->guide_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    card->guide[0] = card_create_guide(card->tool, group, rx, ry, card->guide_duration, 0);
    card->guide[1] = card_create_guide(card->tool, group, rx, ry, card->guide_duration, 180);
    card->guide[2] = card_create_guide(card->tool, group, rx, ry, card->guide_duration, 90);
    card->guide[3] = card_create_guide(card->tool, group, rx, ry, card->guide_duration, 270);

    card->guide_timer = timer = TOOL_ADD_TIMER(card->tool, 0, _card_guide_timeout, card);

    card->event = widget = nemowidget_create_pixman(parent, width, height);
    nemowidget_append_callback(widget, "event", _card_event, card);
    nemowidget_enable_event_repeat(widget, true);

    card->timer = timer = TOOL_ADD_TIMER(card->tool, card->item_duration,
            _card_timeout, card);

    return card;
}

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
    List *ll;
    XmlAttr *attr;
    MenuItem *item = calloc(sizeof(MenuItem), 1);
    item->sxy = 1.0;
    item->resize = true;

    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                 item->type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "bg")) {
            if (attr->val) {
                 item->bg = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                 item->icon = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon_anim_type")) {
            if (attr->val) {
                 item->icon_anim_type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "txt")) {
            if (attr->val) {
                 item->txt = strdup(attr->val);
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
        } else if (!strcmp(attr->key, "mirror")) {
            if (attr->val) {
                item->mirror = strdup(attr->val);
            }
        }
    }
    if (!item->type) item->type = strdup("xapp");
    return item;
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
        MenuItem *it = parse_tag_menu(tag);
        if (!it) continue;
        app->menu_items = list_append(app->menu_items, it);
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
    if (!temp) {
        ERR("No item px in %s", prefix);
    } else {
        app->item_px = atof(temp);
    }
    temp = xml_get_value(xml, buf, "py");
    if (!temp) {
        ERR("No item py in %s", prefix);
    } else {
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
    MenuItem *it;
    LIST_FREE(app->menu_items, it) {
        free(it->type);
        free(it->bg);
        if (it->icon) if (it->icon) free(it->icon);
        if (it->icon_anim_type) free(it->icon_anim_type);
        if (it->txt) free(it->txt);
        free(it->exec);
        free(it);
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
    Card *card = userdata;
    int32_t visible = (intptr_t)info;
    if (visible == -1) {
        if (card->visible) {
            card_hide(card);
            card->visible = false;
        }
    } else {
        if (!card->visible) {
            card_show(card);
            card->visible = true;
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
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "underlay");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    Card *card = card_create(win, app->config->width, app->config->height, app);

    List *l;
    MenuItem *it;
    LIST_FOR_EACH(app->menu_items, l, it) {
        CardItem *_it = card_append_item(card, app->config->sxy,
                it->type, it->bg, it->icon, it->icon_anim_type, it->txt, it->exec,
                it->resize, it->sxy, it->mirror, app);
        card_item_translate(_it, 0, 0, 0, card->ix, card->iy);
    }
    card_show(card);
    nemowidget_append_callback(win, "layer", _card_win_layer, card);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    if (_log) log_destroy(_log);

    return 0;
}
