#include "nemoui-button.h"
#include "xemoutil.h"
#include "nemowrapper.h"

ButtonIcon *button_get_nth_icon(Button *btn, int idx)
{
    return (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
}

Button *button_create(struct showone *parent, const char *id, uint32_t tag)
{
    Button *btn = (Button *)calloc(sizeof(Button), 1);
    btn->blur = BLUR_CREATE("solid", 10);
    btn->group = GROUP_CREATE(parent);
    nemoshow_item_scale(btn->group, 0.0, 0.0);
    nemoshow_item_set_alpha(btn->group, 0.0);

    struct showone *one;
    one = btn->event = RECT_CREATE(btn->group, 0, 0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, id);
    nemoshow_one_set_tag(one, tag);
    nemoshow_one_set_userdata(one, btn);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    return btn;
}

void button_hide(Button *btn, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(btn->group, 0.0);
        nemoshow_item_scale(btn->group, 0.0, 0.0);
    }
}

void button_show(Button *btn, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
        _nemoshow_item_motion_bounce(btn->group, easetype, duration, delay,
                "sx", 1.15, 1.0, "sy", 1.15, 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(btn->group, 1.0);
        nemoshow_item_scale(btn->group, 1.0, 1.0);
    }
}

void button_enable_bg(Button *btn, int r, uint32_t color)
{
    if (btn->w < r * 2) btn->w = r * 2;
    if (btn->h < r * 2) btn->h = r * 2;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    struct showone *one;
    one = CIRCLE_CREATE(btn->group, r);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    nemoshow_item_set_alpha(one, 0.5);
    nemoshow_item_set_filter(one, btn->blur);
    btn->bg0 = one;

    one = CIRCLE_CREATE(btn->group, r);
    nemoshow_item_set_fill_color(one, RGBA(color));
    btn->bg = one;
}

void button_add_path(Button *btn)
{
    // FIXME: can not calculate btn->w, btn->h?

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);

    struct showone *one;
    icon->one = one = PATH_CREATE(btn->group);
    btn->icons = list_append(btn->icons, icon);
}


void button_add_circle(Button *btn, int r)
{
    if (btn->w < r * 2) btn->w = r * 2;
    if (btn->h < r * 2) btn->h = r * 2;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = r * 2;
    icon->h = r * 2;

    struct showone *one;
    icon->one = one = CIRCLE_CREATE(btn->group, r);
    btn->icons = list_append(btn->icons, icon);
}

void button_add_svg(Button *btn, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = w;
    icon->h = h;

    struct showone *one;
    icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, w, h, svgfile);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    btn->icons = list_append(btn->icons, icon);
}

void button_add_svg_path(Button *btn, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = w;
    icon->h = h;

    struct showone *one;
    icon->one = one = SVG_PATH_CREATE(btn->group, w, h, svgfile);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    btn->icons = list_append(btn->icons, icon);
}

void button_change_svg(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    struct showone *one;
    if (icon->old) nemoshow_one_destroy(icon->old);
    icon->old = icon->one;
    if (w >= 0 && h >= 0) {
        icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, w, h, svgfile);
        icon->w = w;
        icon->h = h;
    } else {
        icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, icon->w, icon->h, svgfile);
    }
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    _nemoshow_item_motion(icon->old, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
}

void button_change_svg_path(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    struct showone *one;
    if (icon->old) nemoshow_one_destroy(icon->old);
    icon->old = icon->one;
    if (w >= 0 && h >= 0) {
        icon->one = one = SVG_PATH_CREATE(btn->group, w, h, svgfile);
        icon->w = w;
        icon->h = h;
    } else {
        icon->one = one = SVG_PATH_CREATE(btn->group, icon->w, icon->h, svgfile);
    }
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    _nemoshow_item_motion(icon->old, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
}

void button_bg_set_color(Button *btn, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->bg, easetype, duration, delay,
                "fill", color, NULL);
    } else {
        nemoshow_item_set_fill_color(btn->bg, RGBA(color));
    }
}

void button_set_stroke(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t stroke, int stroke_width)
{
    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    if (duration > 0) {
        _nemoshow_item_motion(icon->one, easetype, duration, delay,
                "stroke", stroke, "stroke-width", (double)stroke_width,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(icon->one, RGBA(stroke));
        nemoshow_item_set_stroke_width(icon->one, stroke_width);
    }
}

void button_set_fill(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t fill)
{
    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    if (duration > 0) {
        _nemoshow_item_motion(icon->one, easetype, duration, delay,
                "fill", fill, NULL);
    } else {
        nemoshow_item_set_fill_color(icon->one, RGBA(fill));
    }
}

void button_set_userdata(Button *btn, void *userdata)
{
    btn->userdata = userdata;
}

void *button_get_userdata(Button *btn)
{
    return btn->userdata;
}

void button_destroy(Button *btn)
{
    nemoshow_one_destroy(btn->blur);
    ButtonIcon *icon;
    LIST_FREE(btn->icons, icon) {
        nemoshow_one_destroy(icon->one);
        if (icon->old) nemoshow_one_destroy(icon->old);
        free(icon);
    }
    nemoshow_one_destroy(btn->group);
    free(btn);
}

void button_translate(Button *btn, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_item_translate(btn->group, tx, ty);
    }
}

void button_down(Button *btn)
{
    if (btn->bg0)
        _nemoshow_item_motion_bounce(btn->bg0,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    if (btn->bg)
        _nemoshow_item_motion_bounce(btn->bg,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    List *l;
    ButtonIcon *icon;
    LIST_FOR_EACH(btn->icons, l, icon) {
        _nemoshow_item_motion_bounce(icon->one,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    }
}

void button_up(Button *btn)
{
    if (btn->bg0)
        _nemoshow_item_motion_bounce(btn->bg0,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                NULL);
    if (btn->bg)
        _nemoshow_item_motion_bounce(btn->bg,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                NULL);
    List *l;
    ButtonIcon *icon;
    LIST_FOR_EACH(btn->icons, l, icon) {
        _nemoshow_item_motion_bounce(icon->one,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 1.1, 1.00, "sy", 1.0, 1.00,
                NULL);
    }
}

