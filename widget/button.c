#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "widgets.h"
#include "nemoui.h"

// ************************************//
// Widget Interface
// ************************************//
typedef struct showone *(*NemoWidgetInterface_Create)(NemoWidget *widget, void *data);
typedef void (*NemoWidgetInterface_Update)(NemoWidget *widget, void *data);
typedef void (*NemoWidgetInterface_Motion)(NemoWidget *widget, void *data);

struct _NemoWidgetInterface {
    NemoWidgetInterface_Create create;
    NemoWidgetInterface_Update update;
    void *data;
};

// ************************************//
// Button Widget
// ************************************//
typedef struct _ButtonOne ButtonOne;
struct _ButtonOne {
    struct showone *one;
    uint32_t color;
    double alpha;
};

typedef struct _ButtonContext ButtonContext;
struct _ButtonContext
{
    double x, y, w, h;

    uint32_t color;
    struct showone *emboss;
    struct showone *group;

    bool hide;

    struct showone *bg;
    struct showone *svg;
    Text *text;
};

static void *_nemowidget_button_create(NemoWidget *button, int width, int height)
{
    struct showone *canvas = nemowidget_get_canvas(button);

    ButtonContext *ctx = (ButtonContext *)calloc(sizeof(ButtonContext), 1);
    ctx->w = width;
    ctx->h = height;
    ctx->emboss = EMBOSS_CREATE(1.0, 1.0, 1.0, 64, 10.1, 10);
    ctx->group = GROUP_CREATE(canvas);

    nemowidget_call_register(button, "clicked");

    return ctx;
}

static void _nemowidget_button_destroy(NemoWidget *button)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    if (ctx->bg) nemoshow_one_destroy(ctx->bg);
    if (ctx->svg) nemoshow_one_destroy(ctx->svg);
    if (ctx->text) text_destroy(ctx->text);

    nemoshow_one_destroy(ctx->group);
    free(ctx);
}

static void _nemowidget_button_show(NemoWidget *button, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    ctx->hide = false;

    if (duration > 0) {
        if (ctx->bg) {
            _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                    "alpha", 1.0f,
                    NULL);
        }
        if (ctx->svg) {
            _nemoshow_item_motion(ctx->svg, easetype, duration, delay,
                    "alpha", 1.0f,
                    NULL);
        }
    } else {
        if (ctx->bg) nemoshow_item_set_alpha(ctx->bg, 1.0f);
        if (ctx->svg) nemoshow_item_set_alpha(ctx->svg, 1.0f);
    }
    if (ctx->text) text_set_alpha(ctx->text, easetype, duration, delay, 1.0);
}

static void _nemowidget_button_hide(NemoWidget *button, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    ctx->hide = true;

    if (duration > 0) {
        if (ctx->bg) {
            _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                    "alpha", 0.0f,
                    NULL);
        }
        if (ctx->svg) {
            _nemoshow_item_motion(ctx->svg, easetype, duration, delay,
                    "alpha", 0.0f,
                    NULL);
        }
    } else {
        if (ctx->bg) nemoshow_item_set_alpha(ctx->bg, 0.0f);
        if (ctx->svg) nemoshow_item_set_alpha(ctx->svg, 0.0f);
    }
    if (ctx->text) text_set_alpha(ctx->text, easetype, duration, delay, 0.0);
}

static void _nemowidget_button_event(NemoWidget *button, struct showevent *event)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    struct nemoshow *show = nemowidget_get_show(button);

    if (nemoshow_event_is_down(show, event)) {
        if (ctx->hide) return;

        double scale = 0.8;
        double x = NEMOSHOW_ITEM_AT(ctx->group, tx);
        double y = NEMOSHOW_ITEM_AT(ctx->group, ty);
        double dx = x + (ctx->w/2) * (NEMOSHOW_ITEM_AT(ctx->group, sx) - scale);
        double dy = y + (ctx->h/2) * (NEMOSHOW_ITEM_AT(ctx->group, sy) - scale);

        _nemoshow_item_motion(ctx->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", scale,
                "sy", scale,
                "tx", dx,
                "ty", dy,
                NULL);
    } else if (nemoshow_event_is_up(show, event)) {
        double scale = 1.0;
        double x = NEMOSHOW_ITEM_AT(ctx->group, tx);
        double y = NEMOSHOW_ITEM_AT(ctx->group, ty);
        double dx = x + (ctx->w/2) * (NEMOSHOW_ITEM_AT(ctx->group, sx) - scale);
        double dy = y + (ctx->h/2) * (NEMOSHOW_ITEM_AT(ctx->group, sy) - scale);

        _nemoshow_item_motion(ctx->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", scale,
                "sy", scale,
                "tx", dx,
                "ty", dy,
                NULL);
    }
    if (nemoshow_event_is_single_click(show, event)) {
        nemowidget_callback_dispatch(button, "clicked", NULL);
    }
}

void nemowidget_button_set_bg_circle(NemoWidget *button)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    if (ctx->bg) {
        nemoshow_one_destroy(ctx->bg);
    }

    struct showone *one;
    one = CIRCLE_CREATE(ctx->group, ctx->w < ctx->h ? ctx->w/2 : ctx->h/2);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, ctx->w/2, ctx->h/2);
    ctx->bg = one;

    nemowidget_dirty(button);
}

void nemowidget_button_set_bg_rect(NemoWidget *button)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    if (ctx->bg) {
        nemoshow_one_destroy(ctx->bg);
    }

    struct showone *one;
    one = RECT_CREATE(ctx->group, ctx->w, ctx->h);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 0.0);
    ctx->bg = one;

    nemowidget_dirty(button);
}

void nemowidget_button_bg_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->bg);

    if (duration > 0) {
        _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                "fill", color,
                NULL);
    } else {
        nemoshow_item_set_fill_color(ctx->bg, RGBA(color));
    }

    nemowidget_dirty(button);
}

void nemowidget_button_bg_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->bg);

    if (duration > 0) {
        _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                "stroke",       color,
                "stroke-width", (double)stroke_w,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(ctx->bg, RGBA(color));
        nemoshow_item_set_stroke_width(ctx->bg, stroke_w);
    }

    nemowidget_dirty(button);
}

void nemowidget_button_set_svg(NemoWidget *button, const char *uri)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);

    if (ctx->svg) {
        nemoshow_one_destroy(ctx->svg);
    }

    struct showone *one;
    one = SVG_PATH_CREATE(ctx->group, ctx->w, ctx->h, uri);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_stroke_color(one, RGBA(WHITE));
    nemoshow_item_set_stroke_width(one, 1);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, ctx->w/2, ctx->h/2);
    ctx->svg = one;

    nemowidget_dirty(button);
}

void nemowidget_button_set_text(NemoWidget *button, const char *family, const char *style, int fontsize, const char *str)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    struct nemotool *tool = nemowidget_get_tool(button);

    if (ctx->text) {
        text_destroy(ctx->text);
    }

    ctx->text = text_create(tool, ctx->group, family, style, fontsize);
    text_update(ctx->text, 0, 0, 0, str);
    text_translate(ctx->text, 0, 0, 0, ctx->w/2, ctx->h/2);

    nemowidget_dirty(button);
}

void nemowidget_button_svg_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->svg);

    if (duration > 0) {
        _nemoshow_item_motion(ctx->svg, easetype, duration, delay,
                "fill", color,
                NULL);
    } else {
        nemoshow_item_set_fill_color(ctx->svg, RGBA(color));
    }

    nemowidget_dirty(button);
}

void nemowidget_button_svg_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->svg);

    if (duration > 0) {
        _nemoshow_item_motion(ctx->svg, easetype, duration, delay,
                "stroke",       color,
                "stroke-width", (double)stroke_w,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(ctx->svg, RGBA(color));
        nemoshow_item_set_stroke_width(ctx->svg, stroke_w);
    }

    nemowidget_dirty(button);
}

void nemowidget_button_text_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay,  uint32_t color)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->text);

    text_set_fill_color(ctx->text, easetype, duration, delay, color);

    nemowidget_dirty(button);
}

void nemowidget_button_text_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w)
{
    NEMOWIDGET_CHECK_CLASS(button, (&NEMOWIDGET_BUTTON));
    ButtonContext *ctx = (ButtonContext *)nemowidget_get_context(button);
    RET_IF(!ctx->text);

    text_set_stroke_color(ctx->text, easetype, duration, delay, color, stroke_w);

    nemowidget_dirty(button);
}

NemoWidgetClass NEMOWIDGET_BUTTON = {"button", NEMOWIDGET_TYPE_VECTOR,
    _nemowidget_button_create, _nemowidget_button_destroy,
    NULL, _nemowidget_button_event, NULL,
    _nemowidget_button_show, _nemowidget_button_hide,
    NULL
};
