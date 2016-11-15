#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "widgets.h"

// ************************************//
// Dim Widget
// ************************************//
typedef struct _DimContext DimContext;
struct _DimContext
{
    double x, y, w, h;

    uint32_t color;

    struct showone *group;
    struct showone *bg;
};

static void *_nemowidget_dim_create(NemoWidget *dim, int width, int height)
{
    struct showone *canvas;
    canvas = nemowidget_get_canvas(dim);
    nemowidget_call_register(dim, "clicked");

    DimContext *ctx = (DimContext *)calloc(sizeof(DimContext), 1);
    ctx->w = width;
    ctx->h = height;

    struct showone *group;

    group = GROUP_CREATE(canvas);
    ctx->group = group;

    return ctx;
}

static void _nemowidget_dim_destroy(NemoWidget *dim)
{
    NEMOWIDGET_CHECK_CLASS(dim, (&NEMOWIDGET_DIM));
    DimContext *ctx = (DimContext *)nemowidget_get_context(dim);

    nemoshow_one_destroy(ctx->group);
    free(ctx);
}

static void _nemowidget_dim_show(NemoWidget *dim, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(dim, (&NEMOWIDGET_DIM));
    DimContext *ctx = (DimContext *)nemowidget_get_context(dim);

    if (ctx->bg) {
        _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                "alpha", 1.0f,
                NULL);
    }
}

static void _nemowidget_dim_hide(NemoWidget *dim, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(dim, (&NEMOWIDGET_DIM));
    DimContext *ctx = (DimContext *)nemowidget_get_context(dim);

    if (ctx->bg) {
        _nemoshow_item_motion(ctx->bg, easetype, duration, delay,
                "alpha", 0.0f,
                NULL);
    }
}

static void _nemowidget_dim_event(NemoWidget *dim, struct showevent *event)
{
    NEMOWIDGET_CHECK_CLASS(dim, (&NEMOWIDGET_DIM));
    struct nemoshow *show = nemowidget_get_show(dim);

    if (nemoshow_event_is_single_click(show, event)) {
        nemowidget_callback_dispatch(dim, "clicked", "clicked");
    }
}

void nemowidget_dim_set_bg(NemoWidget *dim, struct showone *bg)
{
    NEMOWIDGET_CHECK_CLASS(dim, (&NEMOWIDGET_DIM));
    DimContext *ctx = (DimContext *)nemowidget_get_context(dim);

    if (ctx->bg) {
        nemoshow_one_destroy(ctx->bg);
    }

    nemoshow_attach_one(ctx->group->show, bg);
    nemoshow_one_attach(ctx->group, bg);
    ctx->bg = bg;

    nemowidget_dirty(dim);
}

NemoWidgetClass NEMOWIDGET_DIM = {"dim", NEMOWIDGET_TYPE_VECTOR,
    _nemowidget_dim_create, _nemowidget_dim_destroy,
    NULL, _nemowidget_dim_event, NULL,
    _nemowidget_dim_show, _nemowidget_dim_hide,
    NULL
};

