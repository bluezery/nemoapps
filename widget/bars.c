#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "log.h"
#include "util.h"
#include "nemowrapper.h"

#include "widget.h"
#include "widgets.h"

typedef struct _BarsItemContext BarsItemContext;
struct _BarsItemContext {
    struct showone *one;
    double percent;
};

typedef struct _BarsContext BarsContext;
struct _BarsContext {
    int w, h;
    struct showone *group;

    double gap;

    NemoWidget_BarDirection dir;
};

static void _nemowidget_bars_destroy(NemoWidget *bars);
static void *_nemowidget_bars_create(NemoWidget *bars, int width, int height);
static void _nemowidget_bars_hide(NemoWidget *bars, int duration, int delay);
static void _nemowidget_bars_show(NemoWidget *bars, int duration, int delay);
static void *_nemowidget_bar_append_item(NemoWidget *widget);
static void _nemowidget_bar_remove_item(NemoWidgetItem *widget);

NemoWidgetClass NEMOWIDGET_BARS = {"bars", NEMOWIDGET_TYPE_VECTOR,
    _nemowidget_bars_create, _nemowidget_bars_destroy,
    NULL, NULL, NULL,
    _nemowidget_bars_show, _nemowidget_bars_hide,
    NULL, NULL, NULL, NULL,
    _nemowidget_bar_append_item, _nemowidget_bar_remove_item
};

static void *_nemowidget_bars_create(NemoWidget *bars, int width, int height)
{
    struct showone *canvas;
    canvas = nemowidget_get_canvas(bars);
    nemowidget_call_register(bars, "clicked");

    BarsContext *ctx = (BarsContext *)calloc(sizeof(BarsContext), 1);
    ctx->w = width;
    ctx->h = height;

    struct showone *group;
    group = GROUP_CREATE(canvas);
    ctx->group = group;

    ctx->dir = NEMOWIDGET_BAR_DIR_T;

    return ctx;
}

static void *_nemowidget_bar_append_item(NemoWidget *bars)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS), NULL);
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    BarsItemContext *ictx = calloc(sizeof(BarsContext), 1);

    struct showone *one;
    one = RRECT_CREATE(ctx->group, 0, 0, 5, 5);
    nemoshow_item_set_fill_color(one, RGBA(0xFFFFFF));
    nemoshow_item_set_alpha(one, 0.0);
    ictx->one = one;

    return ictx;
}

static void _nemowidget_bar_remove_item(NemoWidgetItem *item)
{
    BarsItemContext *ictx = nemowidget_item_get_context(item);

    free(ictx);
}

#if 0
static void nemowidget_bars_set_bar_color(NemoWidgetItem *bars, int duration, int delay, uint32_t color)
{
    _nemoshow_item_motion(bars->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
            "fill", color,
            NULL);
}

static void nemowidget_bars_set_bar_percent(BarsContext *bars, int duration, int delay, double percent)
{
    bars->percent = percent;
}
#endif

static void _nemowidget_bars_destroy(NemoWidget *bars)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    nemoshow_one_destroy(ctx->group);
}


static void _nemowidget_bars_show(NemoWidget *bars, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

#if 0
    _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
            "alpha", 1.0f,
            NULL);
#endif
}

static void _nemowidget_bars_hide(NemoWidget *bars, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

#if 0
    _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
            "alpha", 0.0f,
            NULL);
#endif
}

#if 0
void _nemowidget_bars_update(NemoWidget *bars, int duration, int delay)
{
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "width",  (double)ctx->w,
                "height", (double)ctx->h * ctx->percent,
                NULL);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                 "width",  (double)ctx->w,
                 "height", (double)ctx->h * ctx->percent,
                NULL);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "width",  (double)ctx->w * ctx->percent,
                "height", (double)ctx->h,
                NULL
                );
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
                "width",  (double)ctx->w * ctx->percent,
                "height", (double)ctx->h,
                NULL
                );
    }
}
#endif

#if 0
void nemowidget_bars_set_direction(NemoWidget *bars, int duration, int delay, NemoWidget_BarDirection dir)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    if (ctx->dir == dir) return;

    ctx->dir = dir;
    if (ctx->dir == NEMOWIDGET_BAR_DIR_T) {
        nemoshow_item_scale(ctx->one, 1.0, -1.0);
        nemoshow_item_translate(ctx->one, 0, ctx->h);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_B) {
        nemoshow_item_scale(ctx->one, 1.0, 1.0);
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, ctx->w);
        nemoshow_item_set_height(ctx->one, 0);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_R) {
        nemoshow_item_scale(ctx->one, 1.0, 1.0);
        nemoshow_item_translate(ctx->one, 0, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    } else if (ctx->dir == NEMOWIDGET_BAR_DIR_L) {
        nemoshow_item_scale(ctx->one, -1.0, 1.0);
        nemoshow_item_translate(ctx->one, ctx->w, 0);
        nemoshow_item_set_width(ctx->one, 0);
        nemoshow_item_set_height(ctx->one, ctx->h);
    }
    _nemowidget_bars_update(bars, duration, delay);
}

void nemowidget_bars_set_percent(NemoWidget *bars, int duration, int delay, double percent)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    ctx->percent = percent;
    _nemowidget_bars_update(bars, duration, delay);
}


void nemowidget_bars_set_color(NemoWidget *bars, int duration, int delay, uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(bars, &(NEMOWIDGET_BARS));
    BarsContext *ctx = (BarsContext *)nemowidget_get_context(bars);

    ctx->color = color;
    _nemoshow_item_motion(ctx->one, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay,
            fill", color,
            NULL);
}
#endif
