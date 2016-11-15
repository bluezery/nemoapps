#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "widgets.h"

// ************************************//
// Frame Widget
// ************************************//
typedef struct _FrameContext FrameContext;
struct _FrameContext
{
    int w, h;
    int cx, cy, cw, ch;
    struct showone *blur;
    struct showone *font;
    struct showone *group;
    struct {
        struct showone *lt;
        struct showone *lb;
        struct showone *rt;
        struct showone *rb;
    } outdeco;
    struct {
        struct showone *lt;
        struct showone *lb;
        struct showone *rt;
        struct showone *rb;
    } indeco;
    struct showone *rect;

    struct {
        uint32_t inactive;
        uint32_t base;
        uint32_t active;
        uint32_t blink;
    } color;

    struct {
        struct showone *deco0;
        struct showone *deco1;
        struct showone *name;
    } title;

    struct {
        struct showone *deco;
        NemoWidget *btn;
    } lock;

    struct {
        NemoWidget *btn;
    } quit;

    struct {
        struct showone *deco0;
        struct showone *deco1;
        NemoWidget *btn;
    } state;

    struct {
        NemoWidget *add;
        NemoWidget *del;
        struct showone *deco0;
        struct showone *deco1;
    } menu0;

    struct {
        NemoWidget *scale;
        NemoWidget *rotate;
        NemoWidget *move;
        struct showone *deco;
    } menu1;

};

static void _nemowidget_frame_show(NemoWidget *frame, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK_CLASS(frame, &NEMOWIDGET_FRAME);
    FrameContext *ctx = (FrameContext *)nemowidget_get_context(frame);

    double gap = 15;
    double pad = 100;
    double tx, ty;

    // outdeco
    tx = NEMOSHOW_ITEM_AT(ctx->outdeco.lt, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->outdeco.lt, ty);
    nemoshow_item_translate(ctx->outdeco.lt, tx - pad, ty);
    _nemoshow_item_motion_bounce(ctx->outdeco.lt, easetype, 1000, 1000,
            "tx", (double)tx + gap, (double)tx,
            NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->outdeco.lb, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->outdeco.lb, ty);
    nemoshow_item_translate(ctx->outdeco.lb, tx - pad, ty);
    _nemoshow_item_motion_bounce(ctx->outdeco.lb, easetype, 1000, 1250,
            "tx", (double)tx + gap, (double)tx,
            NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->outdeco.rt, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->outdeco.rt, ty);
    nemoshow_item_translate(ctx->outdeco.rt, tx + pad, ty);
    _nemoshow_item_motion_bounce(ctx->outdeco.rt, easetype, 1000, 1500,
            "tx", (double)tx - gap, (double)tx,
            NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->outdeco.rb, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->outdeco.rb, ty);
    nemoshow_item_translate(ctx->outdeco.rb, tx + pad, ty);
    _nemoshow_item_motion(ctx->outdeco.rb, easetype, 1000, 1750,
            "tx", (double)tx - gap, (double)tx,
            NULL);

    _nemoshow_item_motion(ctx->outdeco.lt, easetype, 1000, 1000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->outdeco.lb, easetype, 1000, 1000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->outdeco.rt, easetype, 1000, 1000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->outdeco.rb, easetype, 1000, 1000,
            "alpha", 1.0f, NULL);

    // indeco
    tx = NEMOSHOW_ITEM_AT(ctx->indeco.lt, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->indeco.lt, ty);
    nemoshow_item_translate(ctx->indeco.lt, ctx->w/2, ctx->h/2);

    _nemoshow_item_motion_bounce(ctx->indeco.lt, easetype, 1000, 0,
            "tx", (double)tx - gap, (double)tx,
            "ty", (double)ty - gap, (double)ty,
            NULL);
    _nemoshow_item_motion(ctx->indeco.lt, easetype, 1000, 0,
            "alpha", 1.0f, NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->indeco.lb, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->indeco.lb, ty);
    nemoshow_item_translate(ctx->indeco.lb, ctx->w/2, ctx->h/2);
    _nemoshow_item_motion_bounce(ctx->indeco.lb, easetype, 1000, 250,
            "tx", (double)tx - gap, (double)tx,
            "ty", (double)ty - gap, (double)ty,
            NULL);
    _nemoshow_item_motion(ctx->indeco.lb, easetype, 1000, 250,
            "alpha", 1.0f, NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->indeco.rt, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->indeco.rt, ty);
    nemoshow_item_translate(ctx->indeco.rt, ctx->w/2, ctx->h/2);
    _nemoshow_item_motion_bounce(ctx->indeco.rt, easetype, 1000, 500,
            "tx", (double)tx + gap, (double)tx,
            "ty", (double)ty - gap, (double)ty,
            NULL);
    _nemoshow_item_motion(ctx->indeco.rt, easetype, 1000, 500,
            "alpha", 1.0f, NULL);

    tx = NEMOSHOW_ITEM_AT(ctx->indeco.rb, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->indeco.rb, ty);
    nemoshow_item_translate(ctx->indeco.rb, ctx->w/2, ctx->h/2);
    _nemoshow_item_motion_bounce(ctx->indeco.rb, easetype, 1000, 750,
            "tx", (double)tx + gap, (double)tx,
            "ty", (double)ty + gap, (double)ty,
            NULL);
    _nemoshow_item_motion(ctx->indeco.rb, easetype, 1000, 750,
            "alpha", 1.0f, NULL);

    // Background
    int roundx, roundy, w, h;
    roundx = 10;
    roundy = 10;
    w = NEMOSHOW_ITEM_AT(ctx->rect, width);
    h = NEMOSHOW_ITEM_AT(ctx->rect, height);
    tx = NEMOSHOW_ITEM_AT(ctx->rect, tx);
    ty = NEMOSHOW_ITEM_AT(ctx->rect, ty);
    nemoshow_item_set_width(ctx->rect, 0);
    nemoshow_item_set_height(ctx->rect, 0);
    nemoshow_item_translate(ctx->rect, ctx->w/2, ctx->h/2);
    _nemoshow_item_motion(ctx->rect, easetype, 1500, 2000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion_bounce(ctx->rect, easetype, 1500, 2000,
            "tx", (double)tx - gap, (double)tx,
            "ty", (double)ty - gap, (double)ty,
            "width",  (double)w + gap, (double)w,
            "height", (double)h + 20, (double)h,
            NULL);

    _nemoshow_item_motion_bounce(ctx->rect, easetype, 1500, 2500,
            "ox", (double)0, (double)roundx,
            "oy", (double)0, (double)roundy,
            NULL);

    _nemoshow_item_motion(ctx->rect, easetype, 1000, 3500,
            "stroke", ctx->color.active, NULL);

    // Quit
    tx = NEMOWIDGET_AT(ctx->quit.btn, tx);
    ty = NEMOWIDGET_AT(ctx->quit.btn, ty);
    nemowidget_translate(ctx->quit.btn, 0, 0, 0, tx - pad, ty);
    nemowidget_motion_bounce(ctx->quit.btn, easetype, 1000, 3000,
            "tx", (double)tx + gap,(double)tx,
            NULL);

    nemowidget_button_svg_set_fill(ctx->quit.btn,
            easetype, 1000, 3500,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->quit.btn,
            easetype, 1000, 3500,
            ctx->color.active);
    nemowidget_show(ctx->quit.btn, easetype, 1000, 3500);


    // Title
    _nemoshow_item_motion(ctx->title.name, easetype, 1000, 5500,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->title.deco0, easetype, 1000, 5000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->title.deco1, easetype, 1000, 5000,
            "alpha", 1.0f, NULL);

    // Lock
    tx = NEMOWIDGET_AT(ctx->lock.btn, tx);
    ty = NEMOWIDGET_AT(ctx->lock.btn, ty);
    nemowidget_translate(ctx->lock.btn, 0, 0, 0, tx - pad, ty);
    nemowidget_motion_bounce(ctx->lock.btn, easetype, 1000, 3250,
            "tx", (double)tx + gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->lock.btn,
            easetype, 1000, 3750,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->lock.btn,
            easetype, 1000, 3750,
            ctx->color.active);
    nemowidget_show(ctx->lock.btn, easetype, 1000, 3750);

    _nemoshow_item_motion(ctx->lock.deco, easetype, 1000, 3750,
            "alpha", 1.0f, NULL);

    // State
    tx = NEMOWIDGET_AT(ctx->state.btn, tx);
    ty = NEMOWIDGET_AT(ctx->state.btn, ty);
    nemowidget_translate(ctx->state.btn, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->state.btn, easetype, 1000, 3500,
            "tx", (double)tx - gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->state.btn,
            easetype, 1000, 4000,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->state.btn,
            easetype, 1000, 4000,
            ctx->color.active);
    nemowidget_show(ctx->state.btn, easetype, 1000, 4000);

    _nemoshow_item_motion(ctx->state.deco0, easetype, 1000, 4000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->state.deco1, easetype, 1000, 4000,
            "alpha", 1.0f, NULL);

    // Add/Del decoration
    _nemoshow_item_motion(ctx->menu0.deco0, easetype, 1000, 4000,
            "alpha", 1.0f, NULL);
    _nemoshow_item_motion(ctx->menu0.deco1, easetype, 1000, 4250,
            "alpha", 1.0f, NULL);

    // Add
    tx = NEMOWIDGET_AT(ctx->menu0.add, tx);
    ty = NEMOWIDGET_AT(ctx->menu0.add, ty);
    nemowidget_translate(ctx->menu0.add, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->menu0.add, easetype, 1000, 3500,
            "tx", (double)tx - gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->menu0.add,
            easetype, 1000, 4000,
            ctx->color.blink);
    nemowidget_button_text_set_fill(ctx->menu0.add,
            easetype, 1000, 4000,
            ctx->color.blink);
    nemowidget_show(ctx->menu0.add, easetype, 1000, 4000);

    // Delete
    tx = NEMOWIDGET_AT(ctx->menu0.del, tx);
    ty = NEMOWIDGET_AT(ctx->menu0.del, ty);
    nemowidget_translate(ctx->menu0.del, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->menu0.del, easetype, 1000, 3760,
            "tx", (double)tx - gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->menu0.del,
            easetype, 1000, 4250,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->menu0.del,
            easetype, 1000, 4250,
            ctx->color.active);
    nemowidget_show(ctx->menu0.del, easetype, 1000, 4250);

    // Scale/Rotate/Move
    _nemoshow_item_motion(ctx->menu1.deco, easetype, 1000, 4500,
            "alpha", 1.0f, NULL);

    // Scale
    tx = NEMOWIDGET_AT(ctx->menu1.scale, tx);
    ty = NEMOWIDGET_AT(ctx->menu1.scale, ty);
    nemowidget_translate(ctx->menu1.scale, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->menu1.scale, easetype, 1000, 5000,
            "tx", (double)tx - gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->menu1.scale,
            easetype, 1000, 5500,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->menu1.scale,
            easetype, 1000, 5500,
            ctx->color.active);
    nemowidget_show(ctx->menu1.scale, easetype, 1000, 5500);

    // Rotate
    tx = NEMOWIDGET_AT(ctx->menu1.rotate, tx);
    ty = NEMOWIDGET_AT(ctx->menu1.rotate, ty);
    nemowidget_translate(ctx->menu1.rotate, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->menu1.rotate, easetype, 1000, 4750,
            "tx", (double)tx- gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->menu1.rotate,
            easetype, 1000, 5250,
            ctx->color.active);
    nemowidget_button_text_set_fill(ctx->menu1.rotate,
            easetype, 1000, 5250,
            ctx->color.active);
    nemowidget_show(ctx->menu1.rotate, easetype, 1000, 5250);

    // Move
    tx = NEMOWIDGET_AT(ctx->menu1.move, tx);
    ty = NEMOWIDGET_AT(ctx->menu1.move, ty);
    nemowidget_translate(ctx->menu1.move, 0, 0, 0, tx + pad, ty);
    nemowidget_motion_bounce(ctx->menu1.move, easetype, 1000, 4500,
            "tx", (double)tx - gap, (double)tx,
            NULL);
    nemowidget_button_svg_set_fill(ctx->menu1.move,
            easetype, 1000, 5000,
            ctx->color.blink);
    nemowidget_button_text_set_fill(ctx->menu1.move,
            easetype, 1000, 5000,
            ctx->color.blink);
    nemowidget_show(ctx->menu1.move, easetype, 1000, 5000);
}

static void _nemowidget_frame_hide(NemoWidget *frame, uint32_t easetype, int duration, int delay)
{
}

static void _nemowidget_frame_button_callback(NemoWidget *button, const char *id, void *info, void *userdata)
{
    NemoWidget *frame = (NemoWidget *)userdata;
    FrameContext *ctx = (FrameContext *)nemowidget_get_context(frame);

    const char *tag = nemowidget_get_tag(button);
    RET_IF(!tag);
    if (!strcmp(tag, "scale")) {
        nemowidget_button_svg_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
        nemowidget_button_text_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
        nemowidget_button_svg_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_svg_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
    } else if (!strcmp(tag, "rotate")){
        nemowidget_button_svg_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_svg_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
        nemowidget_button_text_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
        nemowidget_button_svg_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
    } else if (!strcmp(tag, "move")) {
        nemowidget_button_svg_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.scale,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_svg_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_text_set_fill(ctx->menu1.rotate,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.active);
        nemowidget_button_svg_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
        nemowidget_button_text_set_fill(ctx->menu1.move,
                NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                ctx->color.blink);
    }

    char new_id[32];
    snprintf(new_id, 32, "%s,%s", tag, id);

    nemowidget_callback_dispatch(frame, new_id, new_id);
}

#if 0
typedef struct _Button_Creator_SVG Button_Creator_SVG;
struct _Button_Creator_SVG
{
    const char *uri;
    uint32_t color;
};

static struct showone *_nemowidget_frame_button_creator_svg(NemoWidget *widget, struct showone *group, int w, int h, void *data)
{
    Button_Creator_SVG *svg = data;

    struct showone *one;
    one = SVG_PATH_CREATE(group, w, h, svg->uri);
    nemoshow_item_set_fill_color(one, RGBA(svg->color));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_one_attach(group, one);
    return one;
}

typedef struct _Button_Creator_TXT Button_Creator_TXT;
struct _Button_Creator_TXT
{
    int fontsize;
    const char *str;
    uint32_t color;
};

static struct showone *_nemowidget_frame_button_creator_text(NemoWidget *widget, struct showone *group, int w, int h, void *data)
{
    Button_Creator_TXT *txt = data;

    struct showone *font;
    font = FONT_CREATE("NanumGothic", "Bold");

    nemowidget_eraser_register(widget, font, nemoshow_one_destroy);

    struct showone *one;
    one = TEXT_CREATE(group, font, txt->fontsize, "");
    nemoshow_item_set_fill_color(one, RGBA(txt->color));
    ERR("%p", txt->color);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_fontsize(one, txt->fontsize);
    nemoshow_item_set_text(one, txt->str);

    return one;
}
#endif

static void _nemowidget_frame_create_done(struct nemotimer *timer, void *userdata)
{
    nemotimer_destroy(timer);
}

static void *_nemowidget_frame_create(NemoWidget *frame, int width, int height)
{
    NEMOWIDGET_CHECK_CLASS(frame, &NEMOWIDGET_FRAME, NULL);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(frame);

    // XXX: For antialiasing effect of frame outline
    int x, y;
    x = y = 4;
    width -= (x * 2);
    height -= (y * 2);
    FrameContext *ctx = (FrameContext *)calloc(sizeof(FrameContext), 1);
    ctx->w = width;
    ctx->h = height;
    ctx->color.base = 0xFFFFFFFF;
    ctx->color.active = 0xFFFF00FF;
    ctx->color.blink = 0xFF0000FF;

    struct showone *group;
    NemoWidget *widget;
    struct showone *one;

    ctx->blur = BLUR_CREATE("solid", 30);
    ctx->font = FONT_CREATE("Quark Storm 3D", "Regular");

    group = GROUP_CREATE(canvas);
    nemoshow_item_translate(group, x, y);
    ctx->group = group;

    int r_baseline;
    int gap = 5;

    // Out decoration
    int rect3_w, rect3_h;
    rect3_w = 60;
    rect3_h = 4;
    r_baseline = width - rect3_w * (3/2.0);
    one = SVG_PATH_CREATE(group, rect3_w, rect3_h, ICON_DIR"/rect3.svg");
    nemoshow_item_translate(one, 0, 0);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    ctx->outdeco.lt = one;

    one = SVG_PATH_CREATE(group, rect3_w, rect3_h, ICON_DIR"/rect3.svg");
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_scale(one, 1.0, -1.0);
    nemoshow_item_translate(one, 0, height);
    ctx->outdeco.lb = one;

    one = SVG_PATH_CREATE(group, rect3_w, rect3_h, ICON_DIR"/rect3.svg");
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_scale(one, 1.0, -1.0);
    nemoshow_item_translate(one, r_baseline, rect3_h);
    ctx->outdeco.rt = one;

    one = SVG_PATH_CREATE(group, rect3_w * (3/2.0), rect3_h, ICON_DIR"/rect3.svg");
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline, height - rect3_h);
    ctx->outdeco.rb = one;

    int stroke_w;
    stroke_w = 4;
    // In decoration
    int ww, hh;
    ww = 36;
    hh = 36;
    one = SVG_PATH_CREATE(group, ww, hh, ICON_DIR"/corner.svg");
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, rect3_w + stroke_w, stroke_w);
    ctx->indeco.lt = one;
    ctx->cx = rect3_w + stroke_w;
    ctx->cy = stroke_w;
    ctx->cw = ctx->w - ctx->cx * 2;
    ctx->ch = ctx->h - ctx->cy * 2;

    one = SVG_PATH_CREATE(group, ww, hh, ICON_DIR"/corner.svg");
    nemoshow_item_pivot(one, ww/2, hh/2);
    nemoshow_item_rotate(one, -90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, rect3_w + stroke_w, height - stroke_w - hh);
    ctx->indeco.lb = one;

    one = SVG_PATH_CREATE(group, ww, hh, ICON_DIR"/corner.svg");
    nemoshow_item_pivot(one, ww/2, hh/2);
    nemoshow_item_rotate(one, 90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline - stroke_w - ww, stroke_w);
    ctx->indeco.rt = one;

    one = SVG_PATH_CREATE(group, ww, hh, ICON_DIR"/corner.svg");
    nemoshow_item_pivot(one, ww/2, hh/2);
    nemoshow_item_rotate(one, 180);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline - stroke_w - ww, height - stroke_w - hh);
    ctx->indeco.rb = one;

    // Background
    stroke_w = 4;
    one = RRECT_CREATE(group, width - rect3_w * (5/2.0), height, 0, 0);
    //nemoshow_item_set_filter(one, ctx->blur);
    nemoshow_item_set_stroke_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_stroke_width(one, stroke_w);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, rect3_w, 0);
    ctx->rect = one;

    // Quit Button
    int btn_w, btn_h;
    btn_w = 56;
    btn_h = 64;

    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "quit");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 18, "QUIT");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0, x, y + rect3_h + gap);
    nemowidget_show(widget, 0, 0, 0);
    ctx->quit.btn = widget;

    // Title
    one = TEXT_CREATE(group, ctx->font, 25, "3D-VIEWER");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, -90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.blink));
    nemoshow_item_set_alpha(one, 0.0f);
    //nemoshow_item_translate(one, btn_w, btn_h + rect3_h + gap + gap + 115);
    nemoshow_item_translate(one, btn_w, ctx->h/2);
    ctx->title.name = one;

    int rect1_w, rect1_h;
    rect1_w = 28;
    rect1_h = 8;
    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, -90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, btn_w, ctx->h/2 - 115);
    ctx->title.deco0 = one;

    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, -90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, btn_w, ctx->h/2 + 110);
    ctx->title.deco1 = one;

    // Lock button
    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "lock");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 18, "LOCK");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x, y + height - (rect3_h + gap + btn_h));
    nemowidget_show(widget, 0, 0, 0);
    ctx->lock.btn = widget;

    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, -90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, btn_w , height - (btn_h + rect3_h + gap + gap));
    ctx->lock.deco = one;

    // State check
    int chk_w, chk_h;
    chk_w = 65;
    chk_h = 25;
    widget = nemowidget_create_button(frame, chk_w, chk_h);
    nemowidget_set_tag(widget, "state");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/check.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 15, "SCENE");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap, y + rect3_h + gap);
    nemowidget_show(widget, 0, 0, 0);
    ctx->state.btn = widget;

    int rect0_w, rect0_h;
    rect0_w = 8;
    rect0_h = 15;
    one = SVG_PATH_CREATE(group, rect0_w, rect0_h, ICON_DIR"/rect0.svg");
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline + gap + chk_w + gap, rect3_h + gap);
    ctx->state.deco0 = one;

    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, 90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline + gap, rect3_h + gap + chk_h + gap + rect1_h + gap);
    ctx->state.deco1 = one;

    // Add/del Menu
    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, 90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline + gap, y + height - (rect3_h + gap + btn_h + btn_h * (2/3.0) + gap * 3));
    ctx->menu0.deco0 = one;

    int rect2_w, rect2_h;
    rect2_w = 20;
    rect2_h = 8;
    one = SVG_PATH_CREATE(group, rect2_w, rect2_h, ICON_DIR"/rect2.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, 90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline + gap, height - (rect3_h + gap + btn_h + btn_h * (2/3.0) + gap * 3) - rect2_w);
    ctx->menu0.deco1 = one;

    // Add
    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "add");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 18, "ADD");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap,
            y + height - (rect3_h + gap + btn_h + btn_h * (2/3.0) + gap * 2));
    nemowidget_show(widget, 0, 0, 0);
    ctx->menu0.add = widget;

    // Del
    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "del");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 18, "DEL");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap + (btn_w/2),
            y + height - (rect3_h + gap + btn_h));
    nemowidget_show(widget, 0, 0, 0);
    ctx->menu0.del = widget;

    // Scale/Rotate/Move menu
    one = SVG_PATH_CREATE(group, rect1_w, rect1_h, ICON_DIR"/rect1.svg");
    nemoshow_item_set_anchor(one, 0.5, 1.0);
    nemoshow_item_rotate(one, 90);
    nemoshow_item_set_fill_color(one, RGBA(ctx->color.base));
    nemoshow_item_set_alpha(one, 0.0f);
    nemoshow_item_translate(one, r_baseline + gap, y + height/2);
    ctx->menu1.deco = one;

    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "scale");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 16, "SCALE");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap,
            y + (height/2) - (btn_h/2) - btn_h * (3/4.0));
    nemowidget_show(widget, 0, 0, 0);
    ctx->menu1.scale = widget;

    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "rotate");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 14, "ROTATE");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap + gap + (btn_w/2),
            y + (height/2) - (btn_h/2));
    nemowidget_show(widget, 0, 0, 0);
    ctx->menu1.rotate = widget;

    widget = nemowidget_create_button(frame, btn_w, btn_h);
    nemowidget_set_tag(widget, "move");
    nemowidget_append_callback(widget, "clicked",
            _nemowidget_frame_button_callback, frame);
    nemowidget_button_set_svg(widget, ICON_DIR"/button.svg");
    nemowidget_button_set_text(widget, "NanumGothic", "Regular", 16, "MOVE");
    nemowidget_button_svg_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_button_text_set_fill(widget, 0, 0, 0, ctx->color.base);
    nemowidget_translate(widget, 0, 0, 0,
            x + r_baseline + gap,
            y + (height/2) - (btn_h/2) + btn_h * (3/4.0) + gap);
    nemowidget_show(widget, 0, 0, 0);
    ctx->menu1.move = widget;

    struct nemotool *tool;
    struct nemotimer *timer;
    tool = nemowidget_get_tool(frame);
    timer = nemotimer_create(tool);
    nemotimer_set_timeout(timer, 6000);
    nemotimer_set_callback(timer, _nemowidget_frame_create_done);
    nemotimer_set_userdata(timer, frame);

    nemowidget_call_register(frame, "quit,clicked");
    nemowidget_call_register(frame, "lock,clicked");
    nemowidget_call_register(frame, "state,clicked");
    nemowidget_call_register(frame, "add,clicked");
    nemowidget_call_register(frame, "del,clicked");
    nemowidget_call_register(frame, "scale,clicked");
    nemowidget_call_register(frame, "rotate,clicked");
    nemowidget_call_register(frame, "move,clicked");
    nemowidget_container_register(frame, "main",
            (double)ctx->cx/ctx->w, (double)ctx->cy/ctx->h,
            (double)ctx->cw/ctx->w, (double)ctx->ch/ctx->h);

    return ctx;
}

static void _nemowidget_frame_destroy(NemoWidget *frame)
{
    NEMOWIDGET_CHECK_CLASS(frame, &NEMOWIDGET_FRAME);
    FrameContext *ctx = (FrameContext *)nemowidget_get_context(frame);

    nemoshow_one_destroy(ctx->blur);
    nemoshow_one_destroy(ctx->font);

#if 1
    nemoshow_one_destroy(ctx->outdeco.lt);
    nemoshow_one_destroy(ctx->outdeco.lb);
    nemoshow_one_destroy(ctx->outdeco.rt);
    nemoshow_one_destroy(ctx->outdeco.rb);
    nemoshow_one_destroy(ctx->indeco.lt);
    nemoshow_one_destroy(ctx->indeco.lb);
    nemoshow_one_destroy(ctx->indeco.rt);
    nemoshow_one_destroy(ctx->indeco.rb);
    nemoshow_one_destroy(ctx->rect);
    nemoshow_one_destroy(ctx->title.deco0);
    nemoshow_one_destroy(ctx->title.deco1);
    nemoshow_one_destroy(ctx->title.name);
    nemoshow_one_destroy(ctx->lock.deco);
    nemoshow_one_destroy(ctx->state.deco0);
    nemoshow_one_destroy(ctx->state.deco1);
    nemoshow_one_destroy(ctx->menu0.deco0);
    nemoshow_one_destroy(ctx->menu0.deco1);
    nemoshow_one_destroy(ctx->menu1.deco);
#endif
    nemoshow_one_destroy(ctx->group);
    free(ctx);
}

NemoWidgetClass NEMOWIDGET_FRAME = {"frame", NEMOWIDGET_TYPE_VECTOR,
    _nemowidget_frame_create, _nemowidget_frame_destroy,
    NULL, NULL, NULL,
    _nemowidget_frame_show, _nemowidget_frame_hide,
    NULL};

void nemowidget_frame_set_color(NemoWidget *frame, uint32_t inactive, uint32_t base, uint32_t active, uint32_t blink)
{
    NEMOWIDGET_CHECK_CLASS(frame, &NEMOWIDGET_FRAME);
    FrameContext *ctx = (FrameContext *)nemowidget_get_context(frame);

    ctx->color.inactive = inactive;
    ctx->color.base = base;
    ctx->color.active = active;
    ctx->color.blink = blink;

    // Outdeco
    nemoshow_item_set_fill_color(ctx->outdeco.lt, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->outdeco.lb, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->outdeco.rt, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->outdeco.rb, RGBA(ctx->color.base));

    // Indeco
    nemoshow_item_set_fill_color(ctx->indeco.lt, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->indeco.lb, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->indeco.rt, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->indeco.rb, RGBA(ctx->color.base));

    // Background
    nemoshow_item_set_stroke_color(ctx->rect, RGBA(ctx->color.base));

    // Title
    nemoshow_item_set_fill_color(ctx->title.name, RGBA(ctx->color.blink));
    nemoshow_item_set_fill_color(ctx->title.deco0, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->title.deco1, RGBA(ctx->color.base));

    // Quit
    nemowidget_button_svg_set_fill(ctx->quit.btn, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->quit.btn, 0, 0, 0, ctx->color.inactive);

    nemowidget_button_svg_set_fill(ctx->lock.btn, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->lock.btn, 0, 0, 0, ctx->color.inactive);

    nemoshow_item_set_fill_color(ctx->lock.deco, RGBA(ctx->color.base));

    nemowidget_button_svg_set_fill(ctx->state.btn, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->state.btn, 0, 0, 0, ctx->color.inactive);

    nemoshow_item_set_fill_color(ctx->state.deco0, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->state.deco1, RGBA(ctx->color.base));

    nemowidget_button_svg_set_fill(ctx->menu0.add, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->menu0.add, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_svg_set_fill(ctx->menu0.del, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->menu0.del, 0, 0, 0, ctx->color.inactive);
    nemoshow_item_set_fill_color(ctx->menu0.deco0, RGBA(ctx->color.base));
    nemoshow_item_set_fill_color(ctx->menu0.deco1, RGBA(ctx->color.base));

    nemowidget_button_svg_set_fill(ctx->menu1.scale, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->menu1.scale, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_svg_set_fill(ctx->menu1.rotate, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->menu1.rotate, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_svg_set_fill(ctx->menu1.move, 0, 0, 0, ctx->color.inactive);
    nemowidget_button_text_set_fill(ctx->menu1.move, 0, 0, 0, ctx->color.inactive);
    nemoshow_item_set_fill_color(ctx->menu1.deco, RGBA(ctx->color.base));
}
