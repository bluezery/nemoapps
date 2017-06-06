#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoui-text.h"

#include <xemoutil.h>
#include <nemowrapper.h>
#include <widget.h>

#define TEXT_PATH_VERSION 1

Text *text_create(struct nemotool *tool, struct showone *parent, const char *family, const char *style,  int fontsize)
{
    struct nemoshow *show = parent->show;
    Text *txt = (Text *)calloc(sizeof(Text), 1);
    txt->tool = tool;
    txt->show = show;
    txt->parent = parent;
    txt->fontfamily = strdup(family);
    if (!style) txt->fontstyle = strdup("Regular");
    else txt->fontstyle = strdup(style);
    txt->fontsize = fontsize;
    txt->ax = 0.5;
    txt->ay = 0.5;

#if !TEXT_PATH_VERSION
    struct showone *font;
    txt->font = font = FONT_CREATE(family, style);
#endif

    struct showone *blur;
    txt->blur.one = blur = BLUR_CREATE("", 0);

    struct showone *group;
    txt->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);
    txt->idx = 0;

#if TEXT_PATH_VERSION
    txt->one[0] = PATH_TEXT_CREATE(group, family, fontsize, "");
#else
    txt->one[0] = TEXT_CREATE(group, font, fontsize, "");
#endif
    nemoshow_item_set_anchor(txt->one[0], 0.5, 0.5);
    nemoshow_item_set_fill_color(txt->one[0], RGBA(0xFFFFFFFF));
    nemoshow_item_set_alpha(txt->one[0], 1.0);

#if TEXT_PATH_VERSION
    txt->one[1] = PATH_TEXT_CREATE(group, family, fontsize, "");
#else
    txt->one[1] = TEXT_CREATE(group, font, fontsize, "");
#endif
    nemoshow_item_set_anchor(txt->one[1], 0.5, 0.5);
    nemoshow_item_set_fill_color(txt->one[1], RGBA(0xFFFFFFFF));
    nemoshow_item_set_alpha(txt->one[1], 1.0);

    return txt;
}

void text_set_filter(Text *txt, struct showone *filter)
{
    nemoshow_item_set_filter(txt->one[0], filter);
    nemoshow_item_set_filter(txt->one[1], filter);
}

void text_set_anchor(Text *txt, double ax, double ay)
{
    nemoshow_item_set_anchor(txt->one[0], ax, ay);
    nemoshow_item_set_anchor(txt->one[1], ax, ay);
}

void text_get_size(Text *txt, double *tw, double *th)
{
    if (!txt->str || (strlen(txt->str) == 0)) {
        ERR("No strings, it's size will be ZERO!");
    }
    nemoshow_one_update(txt->one[txt->idx]);
#if TEXT_PATH_VERSION
    if (tw) *tw = nemoshow_item_get_width(txt->one[txt->idx]);
    if (th) *th = nemoshow_item_get_height(txt->one[txt->idx]);
#else
    if (tw) *tw = NEMOSHOW_ITEM_AT(txt->one[txt->idx], textwidth);
    if (th) *th = NEMOSHOW_ITEM_AT(txt->one[txt->idx], textheight);
#endif
}

void text_set_align(Text *txt, double ax, double ay)
{
#if TEXT_PATH_VERSION
    // default is 0.5, 0.5
    txt->ax = ax;
    txt->ay = ay;
    nemoshow_one_update(txt->one[0]);
    nemoshow_one_update(txt->one[1]);
    nemoshow_item_translate(txt->one[0],
            -nemoshow_item_get_x(txt->one[0]) * (1.0 - 2.0 * ax),
            -nemoshow_item_get_y(txt->one[0]) * (1.0 - 2.0 * ay));
    nemoshow_item_translate(txt->one[1],
            -nemoshow_item_get_x(txt->one[1]) * (1.0 - 2.0 * ax),
            -nemoshow_item_get_y(txt->one[1]) * (1.0 - 2.0 * ay));
#else
    ERR("normal txt item does not support set align");
#endif
}

void text_destroy(Text *txt)
{
    if (txt->clip) nemoshow_one_destroy(txt->clip);

    free(txt->fontfamily);
    nemoshow_one_destroy(txt->one[0]);
    nemoshow_one_destroy(txt->one[1]);
    nemoshow_one_destroy(txt->group);
#if !TEXT_PATH_VERSION
    nemoshow_one_destroy(txt->font);
#endif
    nemoshow_one_destroy(txt->blur.one);
    if (txt->motion_timer) {
        nemotimer_destroy(txt->motion_timer);
    }
    free(txt);
}

void text_set_blur(Text *txt, const char *style, int r)
{
    /*
     * FIXME: blur should be created before txt->one[0] or [1]
     * because of dirty order.
    if (txt->blur.one) {
        nemoshow_one_destroy(txt->blur.one);
        txt->blur.one = NULL;
    }
    */

    RET_IF(!txt->blur.one);
    if (!style || r <= 0) return;

    struct showone *blur;
    blur = txt->blur.one;
    nemoshow_filter_set_blur(blur, style, r);
    txt->blur.r = r;

    nemoshow_item_set_filter(txt->one[0], blur);
    nemoshow_item_set_filter(txt->one[1], blur);
}

void _text_motion_blur_timeout(struct nemotimer *timer, void *userdata)
{
    Text *txt = (Text *)userdata;

    NemoMotion *m;
    m = nemomotion_create(txt->show, txt->blur.easetype,
            txt->blur.duration, txt->blur.delay);
    nemomotion_attach(m, 0.5, txt->blur.one, "r", 1.0, NULL);
    nemomotion_attach(m, 1.0, txt->blur.one, "r", txt->blur.r, NULL);
    nemomotion_run(m);
    nemotimer_set_timeout(timer, txt->blur.duration + txt->blur.delay);
}

void text_motion_blur(Text *txt, uint32_t easetype, int duration, int delay)
{
    RET_IF(!txt->blur.one);

    if (txt->motion_timer) {
        nemotimer_destroy(txt->motion_timer);
        txt->motion_timer = NULL;
    }
    if (duration <= 0) return;

    struct nemotimer *timer;
    timer = nemotimer_create(txt->tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _text_motion_blur_timeout);
    nemotimer_set_userdata(timer, txt);

    txt->motion_timer = timer;
    txt->blur.easetype = easetype;
    txt->blur.duration = duration;
    txt->blur.delay = delay;
}

void text_set_fill_color(Text *txt, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(txt->one[0], easetype, duration, delay,
                "fill", color,
                NULL);
        _nemoshow_item_motion(txt->one[1], easetype, duration, delay,
                "fill", color,
                NULL);
    } else {
        nemoshow_item_set_fill_color(txt->one[0], RGBA(color));
        nemoshow_item_set_fill_color(txt->one[1], RGBA(color));
    }
}

void text_set_stroke_color(Text *txt, uint32_t easetype, int duration, int delay, uint32_t color, int stroke_w)
{
    if (duration > 0) {
        _nemoshow_item_motion(txt->one[0], easetype, duration, delay,
                "stroke", color,
                "stroke-width", (double)stroke_w,
                NULL);
        _nemoshow_item_motion(txt->one[1], easetype, duration, delay,
                "stroke", color,
                "stroke-width", (double)stroke_w,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(txt->one[0], RGBA(color));
        nemoshow_item_set_stroke_width(txt->one[0], stroke_w);
        nemoshow_item_set_stroke_color(txt->one[1], RGBA(color));
        nemoshow_item_set_stroke_width(txt->one[1], stroke_w);
    }
}

void text_get_translate(Text *txt, double *tx, double *ty)
{
    if (tx) *tx = nemoshow_item_get_translate_x(txt->group);
    if (ty) *ty = nemoshow_item_get_translate_y(txt->group);
}

void text_translate(Text *txt, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(txt->group, easetype, duration, delay,
                "tx", (double)x, "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(txt->group, x, y);
    }
}

void text_translate_with_callback(Text *txt, uint32_t easetype, int duration, int delay, int x, int y,
        NemoMotion_FrameCallback callback, void *userdata)
{
    if (duration > 0) {
        _nemoshow_item_motion_with_callback(txt->group,
                callback, userdata,
                easetype, duration, delay,
                "tx", (double)x, "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(txt->group, x, y);
    }
}

void text_set_scale(Text *txt, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(txt->group, easetype, duration, delay,
                "sx", sx, "sy", sy, NULL);
    } else {
        nemoshow_item_scale(txt->group, sx, sy);
    }
}

void text_set_alpha(Text *txt, uint32_t easetype, int duration, int delay, double alpha)
{
    if (duration > 0) {
        _nemoshow_item_motion(txt->group, easetype, duration, delay,
                "alpha", alpha, NULL);
    } else {
        nemoshow_item_set_alpha(txt->group, alpha);
    }
}

void text_update(Text *txt, uint32_t easetype, int duration, int delay, const char *str)
{
    RET_IF(!str);

    if (txt->str && !strcmp(txt->str, str)) return;
    if (txt->str) free(txt->str);
    if (str) txt->str = strdup(str);
    if (txt->idx == 0) txt->idx = 1;
    else txt->idx = 0;

    if (duration > 0) {
        _nemoshow_item_motion(txt->one[txt->idx], easetype, duration, delay,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion(txt->one[!txt->idx], easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(txt->one[txt->idx], 1.0);
        nemoshow_item_set_alpha(txt->one[!txt->idx], 0.0);
    }

#if TEXT_PATH_VERSION
    nemoshow_item_path_clear(txt->one[txt->idx]);
    if (str) {
        nemoshow_item_path_text(txt->one[txt->idx],
                txt->fontfamily, txt->fontsize, str, strlen(str), 0, 0);
    }
#else
    nemoshow_item_set_text(txt->one[txt->idx], str);
#endif
    text_set_align(txt, txt->ax, txt->ay);
}

void text_set_clip(Text *txt, struct showone *clip)
{
    if (txt->clip) nemoshow_one_destroy(txt->clip);
    txt->clip = clip;
    nemoshow_item_set_clip(txt->one[0], clip);
    nemoshow_item_set_clip(txt->one[1], clip);
}
