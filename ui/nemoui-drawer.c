#include <nemoutil.h>
#include <nemowrapper.h>
#include <widget.h>

#include "nemoui-misc.h"
#include "nemoui-drawer.h"
#include "nemoui-button.h"

#define DBOX_CYAN 0x00D8DEFF
#define DBOX_WHITE WHITE
#define DBOX_BLACK 0x5D5D5DFF
#define DBOX_MAGENTA  MAGENTA
#define DBOX_YELLOW YELLOW

uint32_t DRAWING_BOX_COLOR[] = {
    DBOX_BLACK, DBOX_CYAN, DBOX_MAGENTA, DBOX_YELLOW, DBOX_WHITE
};

double DRAWING_BOX_SIZE[] = {
    1, 2, 3, 5, 8,
};

DrawingBox *drawingbox_create(struct showone *parent, int r)
{
    DrawingBox *dbox = (DrawingBox *)calloc(sizeof(DrawingBox), 1);

    dbox->r = r;
    dbox->size = 1;
    dbox->color = DRAWING_BOX_COLOR[0];

    struct showone *group;
    group = dbox->group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    int wh = r * 2;
    Button *btn;

    btn = dbox->pencil_btn = button_create(group, "dbox", 10);
    button_enable_bg(btn, r, DBOX_CYAN);
    button_add_svg_path(btn, RES_DIR"/pencil.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_WHITE);

    btn = dbox->quit_btn = button_create(group, "dbox", 11);
    button_enable_bg(btn, r, DBOX_CYAN);
    button_add_svg_path(btn, RES_DIR"/quit.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_WHITE);

    btn = dbox->share_btn = button_create(group, "dbox", 12);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, RES_DIR"/share.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->undo_btn = button_create(group, "dbox", 13);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, RES_DIR"/undo.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->undo_all_btn = button_create(group, "dbox", 14);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, RES_DIR"/undo_all.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->stroke_btn = button_create(group, "dbox", 15);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, RES_DIR"/stroke_1.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->color_btn = button_create(group, "dbox", 16);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg(btn, RES_DIR"/rainbow.svg", wh, wh);
    button_add_circle(btn, r * 0.75);
    button_set_fill(btn, 0, 0, 0, 1, dbox->color);

    ButtonIcon *icon = button_get_nth_icon(btn, 1);
    nemoshow_item_set_alpha(icon->one, 0.0);
    nemoshow_item_scale(icon->one, 0.0, 0.0);

    return dbox;
}

void drawingbox_destroy(DrawingBox *dbox)
{
    button_destroy(dbox->pencil_btn);
    button_destroy(dbox->quit_btn);
    button_destroy(dbox->share_btn);
    button_destroy(dbox->undo_btn);
    button_destroy(dbox->undo_all_btn);
    button_destroy(dbox->stroke_btn);
    button_destroy(dbox->color_btn);

    nemoshow_one_destroy(dbox->group);
    free(dbox);
}

void drawingbox_change_color(DrawingBox *dbox, uint32_t easetype, int duration, int delay)
{
    dbox->color_idx++;
    if (dbox->color_idx >= (sizeof(DRAWING_BOX_COLOR)/sizeof(DRAWING_BOX_COLOR[0]))) {
        dbox->color_idx = 0;
    }
    dbox->color = DRAWING_BOX_COLOR[dbox->color_idx];

    button_set_fill(dbox->color_btn, easetype, duration, delay,
            1, dbox->color);
    button_set_fill(dbox->stroke_btn, easetype, duration, delay,
            0, dbox->color);

    ButtonIcon *rainbow = button_get_nth_icon(dbox->color_btn, 0);
    ButtonIcon *color = button_get_nth_icon(dbox->color_btn, 1);
    _nemoshow_item_motion(rainbow->one, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0, NULL);
    _nemoshow_item_motion(color->one, easetype, duration, delay,
            "sx", 1.0, "sy", 1.0, "alpha", 1.0, NULL);
}

void drawingbox_change_stroke(DrawingBox *dbox, uint32_t easetype, int duration, int delay)
{
    dbox->size_idx++;
    if (dbox->size_idx >= (sizeof(DRAWING_BOX_SIZE)/sizeof(DRAWING_BOX_SIZE[0]))) {
        dbox->size_idx = 0;
    }
    dbox->size = DRAWING_BOX_SIZE[dbox->size_idx];

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, RES_DIR"/stroke_%d.svg", dbox->size_idx);
    button_change_svg_path(dbox->stroke_btn, easetype, duration, delay,
            0, buf, -1, -1);
    button_set_fill(dbox->stroke_btn, easetype, duration, delay,
            0, dbox->color);
}

void drawingbox_show_pencil(DrawingBox *dbox)
{
    dbox->mode = 0;

    button_bg_set_color(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            DBOX_CYAN);
    button_set_fill(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            0, DBOX_WHITE);
    button_show(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);
}

void drawingbox_show_menu(DrawingBox *dbox)
{
    dbox->mode = 1;
    int r = dbox->r * 2.5;

    int delay = 100;

    button_hide(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_show(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);

    button_show(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay);
    button_translate(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay,
           r * sin(2 * M_PI * 0.4), r *cos(2 * M_PI * 0.4));

    button_show(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2);
    button_translate(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
           r * sin(2 * M_PI * 0.6), r *cos(2 * M_PI * 0.6));
    ButtonIcon *rainbow = button_get_nth_icon(dbox->color_btn, 0);
    ButtonIcon *color = button_get_nth_icon(dbox->color_btn, 1);
    _nemoshow_item_motion(rainbow->one, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
            "sx", 1.0, "sy", 1.0, "alpha", 1.0, NULL);
    _nemoshow_item_motion(color->one, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0, NULL);

    button_show(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 3);
    button_translate(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 3,
           r * sin(2 * M_PI * 0.2), r *cos(2 * M_PI * 0.2));

    button_show(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 4);
    button_translate(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 4,
           r * sin(2 * M_PI * 0.8), r *cos(2 * M_PI * 0.8));

    button_show(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 5);
    button_translate(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 5,
           r * sin(2 * M_PI * 0.0), r *cos(2 * M_PI * 0.0));
}

void drawingbox_show(DrawingBox *dbox)
{
    _nemoshow_item_motion(dbox->group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);

    button_show(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 350);
}

void drawingbox_hide(DrawingBox *dbox)
{
    _nemoshow_item_motion(dbox->group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);

    button_hide(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 150);
}

void drawingbox_get_translate(DrawingBox *dbox, double *tx, double *ty)
{
    if (tx) *tx = nemoshow_item_get_translate_x(dbox->group);
    if (ty) *ty = nemoshow_item_get_translate_y(dbox->group);
}

void drawingbox_translate(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(dbox->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "tx");
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "ty");
        nemoshow_item_translate(dbox->group, tx, ty);
    }
}

void drawingbox_scale(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(dbox->group, easetype, duration, delay,
                "sx", sx, "sy", sy, NULL);
    } else {
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "sx");
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "sy");
        nemoshow_item_scale(dbox->group, sx, sy);
    }
}
