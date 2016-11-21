#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include <nemoutil.h>
#include "nemowrapper.h"

#include "widget.h"
#include "widgets.h"

typedef struct _BarContext BarContext;
struct _BarContext {
    int w, h;
    struct showone *group;
    struct showone *one;
    double percent;

    NemoWidget_BarDirection dir;
};

static void _nemowidget_bar_destroy(NemoWidget *bar);
static void *_nemowidget_bar_create(NemoWidget *bar, int width, int height);
static void _nemowidget_bar_event(NemoWidget *bar, struct showevent *event);
static void _nemowidget_bar_hide(NemoWidget *bar, uint32_t easetype, int duration, int delay);
static void _nemowidget_bar_show(NemoWidget *bar, uint32_t easetype, int duration, int delay);

NemoWidgetClass NEMOWIDGET_BAR = {"bar", NEMOWIDGET_TYPE_VECTOR,
    _nemowidget_bar_create, _nemowidget_bar_destroy,
    NULL, _nemowidget_bar_event, NULL,
    _nemowidget_bar_show, _nemowidget_bar_hide,
    NULL
};

static void *_nemowidget_bar_create(NemoWidget *bar, int width, int height)
{
    struct showone *canvas;
    canvas = nemowidget_get_canvas(bar);
    nemowidget_call_register(bar, "clicked");

    BarContext *ctx = (BarContext *)calloc(sizeof(BarContext), 1);
    ctx->w = width;
    ctx->h = height;

    struct showone *group;
    group = GROUP_CREATE(canvas);
    ctx->group = group;

    struct showone *one;
    one = RRECT_CREATE(group, 0, 0, 5, 5);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_fill_color(one, RGBA(0xFFFFFF));
    nemoshow_item_set_alpha(one, 0.0);
    ctx->one = one;

    ctx->dir = NEMOWIDGET_BAR_DIR_T;
    nemoshow_item_translate(ctx->one, 0, ctx->h - ctx->h * ctx->percent);
    nemoshow_item_set_width(ctx->one, ctx->w);
    nemoshow_item_set_height(ctx->one, 0);

    return ctx;
}

static void _nemowidget_bar_destroy(NemoWidget *bar)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    nemoshow_one_destroy(ctx->group);
}

static void _nemowidget_bar_event(NemoWidget *bar, struct showevent *event)
{
    NEMOWIDGET_CHECK_CLASS(bar, (&NEMOWIDGET_BAR));
    struct nemoshow *show = nemowidget_get_show(bar);

    if (nemoshow_event_is_single_click(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(bar,
                nemoshow_event_get_x(event), nemoshow_event_get_y(event),
                &ex, &ey);

        struct showone *one;
        struct showone *canvas = nemowidget_get_canvas(bar);
        one = nemoshow_canvas_pick_one(canvas, ex, ey);

        if (one) {
            nemowidget_callback_dispatch(bar, "clicked", NULL);
        }
    }
}

static void _nemowidget_bar_show(NemoWidget *bar, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    _nemoshow_item_motion(ctx->one, easetype, duration, delay,
            "alpha", 1.0f,
            NULL);
}

static void _nemowidget_bar_hide(NemoWidget *bar, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    _nemoshow_item_motion(ctx->one, easetype, duration, delay,
            "alpha", 0.0f,
            NULL);
}

void _nemowidget_bar_update(NemoWidget *bar)
{
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        nemoshow_item_translate(ctx->one, 0.0, (double)(ctx->h - ctx->h * ctx->percent));
        nemoshow_item_set_width(ctx->one, (double)ctx->w);
        nemoshow_item_set_height(ctx->one, (double)ctx->h * ctx->percent);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        nemoshow_item_translate(ctx->one, 0.0, 0.0);
        nemoshow_item_set_width(ctx->one, (double)ctx->w);
        nemoshow_item_set_height(ctx->one, (double)ctx->h * ctx->percent);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        nemoshow_item_translate(ctx->one, 0.0, 0.0);
        nemoshow_item_set_width(ctx->one, (double)ctx->w * ctx->percent);
        nemoshow_item_set_height(ctx->one, (double)ctx->h);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        nemoshow_item_translate(ctx->one, (double)(ctx->w - ctx->w * ctx->percent), 0.0);
        nemoshow_item_set_width(ctx->one, (double)ctx->w * ctx->percent);
        nemoshow_item_set_height(ctx->one, (double)ctx->h);
    }
}

void _nemowidget_bar_update_motion(NemoWidget *bar, int duration, int delay)
{
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "tx",     0.0,
                "ty",     (double)(ctx->h - ctx->h * ctx->percent),
                "width",  (double)ctx->w,
                "height", (double)ctx->h * ctx->percent,
                NULL);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "tx",     0.0,
                "ty",     0.0,
                "width",  (double)ctx->w,
                "height", (double)ctx->h * ctx->percent,
                NULL);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "tx",     0.0,
                "ty",     0.0,
                "width",  (double)ctx->w * ctx->percent,
                "height", (double)ctx->h,
                NULL
                );
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "tx",     (double)(ctx->w - ctx->w * ctx->percent),
                "ty",     0.0,
                "width",  (double)ctx->w * ctx->percent,
                "height", (double)ctx->h,
                NULL
                );
    }
}

void nemowidget_bar_set_direction(NemoWidget *bar, NemoWidget_BarDirection dir)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    if (dir < NEMOWIDGET_BAR_DIR_T ||
            dir >= NEMOWIDGET_BAR_DIR_MAX) {
        ERR("direction is not correct: %d", dir);
        return;
    }
    if (ctx->dir == dir) {
        ERR("Same direction is already set: %d", dir);
        return;
    }

    ctx->dir = dir;
    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        nemoshow_item_translate(ctx->one, 0, ctx->h);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        nemoshow_item_translate(ctx->one, ctx->w, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    }
    _nemowidget_bar_update(bar);
}

void nemowidget_bar_set_direction_motion(NemoWidget *bar, int duration, int delay, NemoWidget_BarDirection dir)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    if (dir < NEMOWIDGET_BAR_DIR_T ||
            dir >= NEMOWIDGET_BAR_DIR_MAX) {
        ERR("direction is not correct: %d", dir);
        return;
    }
    if (ctx->dir == dir) {
        ERR("Same direction is already set: %d", dir);
        return;
    }

    ctx->dir = dir;
    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        nemoshow_item_translate(ctx->one, 0, ctx->h);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        nemoshow_item_translate(ctx->one, ctx->w, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    }
    _nemowidget_bar_update_motion(bar, duration, delay);
}

void nemowidget_bar_set_percent(NemoWidget *bar, double percent)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    ctx->percent = percent;
    _nemowidget_bar_update(bar);
}

void nemowidget_bar_set_percent_motion(NemoWidget *bar, int duration, int delay, double percent)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    ctx->percent = percent;
    _nemowidget_bar_update_motion(bar, duration, delay);
}

void nemowidget_bar_set_color_motion(NemoWidget *bar, int duration, int delay, uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
            "fill", color,
            NULL);
}

void nemowidget_bar_set_color(NemoWidget *bar,  uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(bar, &(NEMOWIDGET_BAR));
    BarContext *ctx = (BarContext *)nemowidget_get_context(bar);

    nemoshow_item_set_fill_color(ctx->one, RGBA(color));
}
