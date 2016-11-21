#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "log.h"
#include "nemoutil.h"
#include "config.h"
#include "color.h"
#include "nemowrapper.h"

#include "widgets.h"

const int COLOR_TABLE[] = {
    RGBA2UINT(34, 49, 64, 255),
    RGBA2UINT(242, 38, 19, 255),
    RGBA2UINT(52, 152, 219, 255),
    RGBA2UINT(27, 188, 155, 255),
    RGBA2UINT(247, 202, 24, 255),
    RGBA2UINT(255, 255, 255, 255),
};

const int THICK_TABLE[] = {
    5,
    10,
    15,
    20,
    30,
};

typedef struct _Menu Menu;
typedef struct _View View;

struct _Menu {
    View *view;
    struct showone *canvas;
    int thick_idx;
    uint32_t color_idx;

    double r;
    double thick_r;
    double color_r;

    struct showone *area;
    List *color_pickers;
    struct showone *thick_changer;

    bool moving;
};

typedef struct _Grab Grab;
struct _Grab {
    struct talegrab base;
    View *view;
    Menu *menu;
    bool moving;
    List *dots;
};

struct _View {
    NemoWidget *empty;
    struct nemoshow *show;
    struct showone *scene;
    struct showone *canvas;
    struct showone *blur;
    struct showone *font;
    struct showone *group;

    List *menus;

    bool locked;
    struct showone *back;
    struct showone *lock;
    struct showone *unlock;

    bool yn;
    struct showone *yes, *yes_txt;
    struct showone *no, *no_txt;
    struct showone *quit;

    float clicked_x, clicked_y;
    uint32_t clicked_time;

    List *dotss;
};

typedef struct _Dot Dot;
struct _Dot {
    int x;
    int y;
};

static void _grab_start_dot(Grab *grab, int x, int y, uint32_t color, int thick)
{
    Dot *dot = malloc(sizeof(Dot));
    dot->x = x;
    dot->y = y;
    grab->dots = list_append(grab->dots, dot);
}

static void _grab_add_dot(Grab *grab, int x, int y, uint32_t color, int thick)
{
    View *view = grab->view;
    struct nemoshow *show = view->show;
    struct showone *one;
    struct showone *group = view->group;

    Dot *prev = LIST_DATA(LIST_LAST(grab->dots));

    if (!prev) return;
    if (abs(x - prev->x) <= 1.0 || abs(y - prev->y) <= 1.0) return;

    Dot *dot = malloc(sizeof(Dot));
    dot->x = x;
    dot->y = y;
    grab->dots = list_append(grab->dots, dot);

    one = LINE_CREATE(group, prev->x, prev->y, dot->x, dot->y, thick);
    nemoshow_item_set_fill_color(one, RGBA(color));
    nemoshow_item_set_alpha(one, 1.0);

    nemoshow_one_attach(view->group, view->lock);
    nemoshow_one_attach(view->group, view->unlock);
    nemoshow_one_attach(view->group, view->quit);
    nemoshow_dispatch_frame(show);
}


static bool _menu_is_thick_changer(Menu *menu, int ex, int ey)
{
    double x, y;
    x = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
    y = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

    return CIRCLE_IN(x, y, menu->thick_r, ex, ey);
}

static bool _menu_is_menu(Menu *menu, int ex, int ey)
{
    double x, y;
    x = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
    y = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

    return CIRCLE_IN(x, y, menu->r, ex, ey);
}

static void _menu_thick_change(Menu *menu)
{
    menu->thick_idx++;
    if (menu->thick_idx >= (sizeof(THICK_TABLE)/sizeof(THICK_TABLE[0])))
        menu->thick_idx = 0;

    _nemoshow_item_motion(menu->thick_changer, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "r", THICK_TABLE[menu->thick_idx],
            NULL);
}

static bool _menu_is_color_chager(Menu *menu, int ex, int ey)
{
    double x, y;
    x = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
    y = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

    return CIRCLE_IN(x, y, menu->color_r, ex, ey);
}

static void _menu_color_change(Menu *menu, int x, int y)
{
    double cx, cy;
    cx = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
    cy = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

    double radians = atan2(y - cy, x - cx);
    double r = radians * (180/M_PI);
    if (r <= 0) r = 360 + r;

    int num = 6;
    double angle = 360.0/num;
    double start = 0;
    double end = angle;
    int i = 0;
    for (i = 0 ; i < num ; i++) {
        if (start <= r && r <= end) {
            menu->color_idx = i;
            int color = COLOR_TABLE[i];
            if (menu->color_idx ==
                    (sizeof(COLOR_TABLE)/sizeof(COLOR_TABLE[0]) - 1)) {
                // XXX: To see eraser.
                color = RGBA2UINT(210, 210, 210, 255);
            }

            _nemoshow_item_motion(menu->area, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                    "fill", color, NULL);
            _nemoshow_item_motion(menu->thick_changer, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                    "fill", color, NULL);
            break;
        }
        start = end;
        end = end + angle;
    }
}

static void _menu_destroy_done(struct nemotimer *timer, void *userdata)
{
    Menu *menu = userdata;
    View *view = menu->view;
    struct showone *one;

    nemoshow_one_destroy(menu->area);
    LIST_FREE(menu->color_pickers, one) {
        nemoshow_one_destroy(one);
    }
    nemoshow_one_destroy(menu->thick_changer);
    free(menu);
    view->menus = list_remove(view->menus, menu);
}

static void _menu_destroy(Menu *menu)
{
    struct showone *one;
    one = menu->thick_changer;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
           "r", 0,
           "alpha", 0.08,
           NULL);

    int num = 6;
    double angle = 360.0/num;
    double start, end;
    start = angle/2;
    end = angle/2;
    int i;
    for (i = 0 ; i < num ; i++) {
        one = LIST_DATA(list_get_nth(menu->color_pickers, i));
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 100 * i,
                "from", start,
                "to", end,
                "alpha", 0.0,
                NULL);
        start += angle;
        end += angle;
    }

    one = menu->area;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 1000,
            "width", 0.0,
            "height", 0.0,
            "inner", 0.0,
            "alpha", 0.0,
            NULL);

    struct nemotimer *timer = nemotimer_create(nemowidget_get_tool(menu->view->empty));
    nemotimer_set_timeout(timer, 500 + ((100 * num) > 1000 ? (100 * num) : 1000));
    nemotimer_set_userdata(timer, menu);
    nemotimer_set_callback(timer, _menu_destroy_done);
}

static Menu *_menu_create(View *view, int tx, int ty)
{
    struct nemoshow *show = view->show;
    struct showone *blur = view->blur;

    Menu *menu;
    menu = calloc(sizeof(Menu), 1);
    menu->view = view;
    menu->thick_idx = 0;
    menu->color_idx = 0;
    menu->r = 80;
    menu->color_r = 60;
    menu->thick_r = THICK_TABLE[sizeof(THICK_TABLE)/sizeof(THICK_TABLE[0]) - 1];

    double r, r1, r2;
    r = menu->r;
    r1 = menu->color_r;
    r2 = THICK_TABLE[menu->thick_idx];

    int border = 20;
    struct showone *canvas;
    struct showone *one;
    canvas = CANVAS_VECTOR_CREATE(show, r * 2 + border ,  r * 2 + border);
    nemoshow_canvas_translate(canvas, tx - r, ty - r);
    menu->canvas = canvas;

    // Drawing area
    double inner = r - r1 - 2;
    double cx, cy;
    cx = cy = r + border/2;
    one = DONUT_CREATE(canvas, 0, 0, 0, 360, 0);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_TABLE[menu->color_idx]));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_filter(one, blur);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, cx, cy);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "width", r * 2,
            "height", r * 2,
            "inner", inner,
            "alpha", 0.5, NULL);
    menu->area = one;

    // Color picker
    int num = 6;
    double angle = 360.0/num;
    double start, end;
    inner =
        r1 - menu->thick_r - 2;
    start = 0;

    int i;
    for (i = 0 ; i < num ; i++) {
        if (i == num -1)
            end = 360;
        else
            end = start + angle;
        one = DONUT_CREATE(canvas, r1 * 2, r1 * 2,
                start + r1/2, start - r1/2, inner);
        nemoshow_item_set_fill_color(one, RGBA(COLOR_TABLE[i]));
        nemoshow_item_set_alpha(one, 0.0);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, cx, cy);
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 100 * i,
                "from", start,
                "to", end,
                "alpha", 1.0, NULL);
        menu->color_pickers = list_append(menu->color_pickers, one);
        start = end;
    }

    // Thick changer
    one = CIRCLE_CREATE(canvas, 0);
    nemoshow_item_set_anchor(one, -0.5, -0.5);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_TABLE[menu->color_idx]));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, cx, cy);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 750,
            "r", r2,
            "alpha", 0.8, NULL);
    menu->thick_changer = one;

    nemoshow_dispatch_frame(show);
    return menu;
}

typedef struct _Context Context;
struct _Context {
    NemoWidget *win;
    NemoWidget *empty;
    bool noevent;
    View *view;
};

static void _view_hide_yn_done(struct nemotimer *timer, void *userdata)
{
    View *view = userdata;
    nemoshow_one_destroy(view->yes);
    nemoshow_one_destroy(view->yes_txt);
    nemoshow_one_destroy(view->no);
    nemoshow_one_destroy(view->no_txt);
    view->yes = NULL;
    view->yes_txt = NULL;
    view->no = NULL;
    view->no_txt = NULL;
    view->yn = false;
}

static void _view_hide_yn(View *view)
{
    struct nemoshow *show = view->show;
    struct showone *one;

    int x;
    x = NEMOSHOW_ITEM_AT(view->quit, tx) +
        NEMOSHOW_ITEM_AT(view->quit, width)/2;

    one = view->no_txt;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "font-size", 0.0,
            "alpha", 0.0,
            NULL);

    one = view->no;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "r", 0.0,
            "stroke-width", 0.0,
            "alpha", 0.0,
            NULL);

    one = view->yes_txt;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250,
            "tx", x,
            "font-size", 0.0,
            "alpha", 0.0,
            NULL);

    one = view->yes;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250,
            "tx", x,
            "r", 0.0,
            "stroke-width", 0.0,
            "alpha", 0.0,
            NULL);

    one = view->quit;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 500,
            "alpha", 1.0,
            NULL);

    struct nemotimer *timer = nemotimer_create(nemowidget_get_tool(view->empty));
    nemotimer_set_timeout(timer, 500 + 500);
    nemotimer_set_userdata(timer, view);
    nemotimer_set_callback(timer, _view_hide_yn_done);
}

static void _view_show_yn(View *view)
{
    struct nemoshow *show = view->show;
    struct showone *group = view->group;
    struct showone *font = view->font;

    int x, y;
    int wh;
    wh = 40;
    struct showone *one;

    x = NEMOSHOW_ITEM_AT(view->quit, tx) +
        NEMOSHOW_ITEM_AT(view->quit, width)/2;
    y = NEMOSHOW_ITEM_AT(view->quit, ty) +
        NEMOSHOW_ITEM_AT(view->quit, height)/2;

    one = view->quit;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0,
            NULL);

    one = CIRCLE_CREATE(group, 0);
    nemoshow_item_set_anchor(one, -0.5, -0.5);
    nemoshow_item_set_stroke_color(one, RGBA(RED));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, x, y);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250,
            "tx", x - wh - 12,
            "r", (double)wh/2,
            "stroke-width", 5.0,
            "alpha", 1.0,
            NULL);
    view->yes = one;

    one = TEXT_CREATE(group, font, 0, "Y");
    nemoshow_item_set_fill_color(one, RGBA(RED));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, x, y);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250,
            "tx", x - wh - 12,
            "font-size", 30.0,
            "alpha", 1.0,
            NULL);
    view->yes_txt = one;

    one = CIRCLE_CREATE(group, 0);
    nemoshow_item_set_anchor(one, -0.5, -0.5);
    nemoshow_item_set_stroke_color(one, RGBA(ORANGE));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, x, y);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 500,
            "r", (double)wh/2,
            "stroke-width", 5.0,
            "alpha", 1.0,
            NULL);
    view->no = one;

    one = TEXT_CREATE(group, font, 0, "N");
    nemoshow_item_set_fill_color(one, RGBA(ORANGE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, x, y);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 500,
            "font-size", 30.0,
            "alpha", 1.0,
            NULL);
    view->no_txt = one;

    view->yn = true;
    nemoshow_dispatch_frame(show);
}

static View *_view_create(NemoWidget *empty, int width, int height)
{
    struct nemoshow *show;
    struct showone *canvas;
    struct showone *group;
    struct showone *one;

    show = nemowidget_get_show(empty);
    canvas = nemowidget_get_canvas(empty);

    View *view = calloc(sizeof(View), 1);
    view->empty = empty;

    view->blur = BLUR_CREATE("solid", 10);
    view->font = FONT_CREATE("BM JUA", "Regular");

    group = GROUP_CREATE(canvas);
    view->group = group;

#if 0
    // border
    int sw = 5;
    int len = 50;
    one = PATH_CREATE(show, canvas, NULL,
            NULL,
            STYLE(true, sw, BLUE, 1.0f), COORD(0, 0, 0, 0));
    nemoshow_item_path_moveto(one, x, len);
    nemoshow_item_path_cubicto(one, x, y, x, y, len, y);
    nemoshow_item_path_moveto(one, width-len, y);
    nemoshow_item_path_cubicto(one, width, y, width, y, width, len);
    nemoshow_item_path_moveto(one, width, height-len);
    nemoshow_item_path_cubicto(one, width, height, width, height, width-len, height);
    nemoshow_item_path_moveto(one, len, height);
    nemoshow_item_path_cubicto(one, x, height, x, height, x, height - len);
    view->back = one;
#endif
    one = RRECT_CREATE(group, width, height, 5, 5);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 1.0);
    view->back = one;

    /*
    one = RRECT_CREATE(group, NULL, 2, 2,
            STYLE(false, 0, RED, 1.0f),
            COORD(0, 0, 100, 20));
    nemoshow_item_rotate(one, 45);
    nemoshow_item_translate(one, 100, 100);
    */

    int x = 12;
    int y = 12;
    width -= (x * 2);
    height -= (y * 2);

    // Pin
    int wh = 40;
    int space = 12;
    one = SVG_PATH_CREATE(group, wh, wh, ICON_DIR"/lock.svg");
    nemoshow_item_set_stroke_color(one, RGBA(ORANGE));
    nemoshow_item_set_stroke_width(one, 3);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, space, space);
    view->lock = one;
    one = SVG_PATH_CREATE(group, wh, wh, ICON_DIR"/unlock.svg");
    nemoshow_item_set_stroke_color(one, RGBA(BLUE));
    nemoshow_item_set_stroke_width(one, 3);
    nemoshow_item_set_alpha(one, 1.0);
    nemoshow_item_translate(one, space, space);
    view->unlock = one;

    one = SVG_PATH_CREATE(group, wh, wh, ICON_DIR"/quit.svg");
    nemoshow_item_set_stroke_color(one, RGBA(RED));
    nemoshow_item_set_stroke_width(one, 3);
    nemoshow_item_set_alpha(one, 1.0);
    nemoshow_item_translate(one, 1024 - wh - space, space);
    view->quit = one;

    return view;
}

static bool _is_yes(View *view, int x, int y)
{
    if (!view->yes) return false;
    struct showone *one = view->yes;
    double tx = NEMOSHOW_ITEM_AT(one, tx);
    double ty = NEMOSHOW_ITEM_AT(one, ty);
    double r = NEMOSHOW_ITEM_AT(one, r);

    if (CIRCLE_IN(tx, ty, r, x, y))  return true;
    return false;
}

static bool _is_no(View *view, int x, int y)
{
    if (!view->no) return false;
    struct showone *one = view->no;
    double tx = NEMOSHOW_ITEM_AT(one, tx);
    double ty = NEMOSHOW_ITEM_AT(one, ty);
    double r = NEMOSHOW_ITEM_AT(one, r);

    if (CIRCLE_IN(tx, ty, r, x, y))  return true;
    return false;
}

static bool _is_quit(View *view, int x, int y)
{
    struct showone *one = view->quit;
    double tx = NEMOSHOW_ITEM_AT(one, tx);
    double ty = NEMOSHOW_ITEM_AT(one, ty);
    double w = NEMOSHOW_ITEM_AT(one, width);
    double h = NEMOSHOW_ITEM_AT(one, height);

    if (RECTS_CROSS(tx, ty, w, h, x, y, 30, 30)) {
        return true;
    }
    return false;
}

static bool _is_lock(View *view, int x, int y)
{
    struct showone *one;
    one = view->lock;
    double tx = NEMOSHOW_ITEM_AT(one, tx);
    double ty = NEMOSHOW_ITEM_AT(one, ty);
    double w = NEMOSHOW_ITEM_AT(one, width);
    double h = NEMOSHOW_ITEM_AT(one, height);

    if (RECTS_CROSS(tx, ty, w, h, x, y, 30, 30)) {
        return true;
    }
    return false;
}

static void _lock_change(View *view)
{
    struct nemoshow *show = view->show;
    if (view->locked) {
        view->locked = false;
        _nemoshow_item_motion(view->lock, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.0,
                NULL);
        _nemoshow_item_motion(view->unlock, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 1.0,
                NULL);
    } else {
        view->locked = true;
        _nemoshow_item_motion(view->lock, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 1.0,
                NULL);
        _nemoshow_item_motion(view->unlock, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.0,
                NULL);
    }
    nemoshow_dispatch_frame(show);
}

static int _grab_dispatch(struct talegrab *tgrab, uint32_t type, struct taleevent *event)
{
    Grab *grab = (Grab *)container_of(tgrab, Grab, base);
    View *view = grab->view;
    struct nemoshow *show = view->show;
    Menu *menu = grab->menu;

    if (type & NEMOTALE_UP_EVENT) {
        if (menu) {
            if (menu->moving && grab->moving) {
                grab->moving = false;
                menu->moving = false;
            }
        }
        Dot *dot;
        nemotale_finish_grab(&grab->base);
        LIST_FREE(grab->dots, dot) {
            free(dot);
        }
        free(grab);
        return 0;
    } else if (type & NEMOTALE_DOWN_EVENT) {
        if (menu && !menu->moving && _menu_is_thick_changer(menu, event->x, event->y)) {
            grab->moving = true;
            menu->moving = true;
        } else if (menu && !_menu_is_menu(menu, event->x, event->y)) {
            _grab_start_dot(grab, event->x, event->y,
                    COLOR_TABLE[menu->color_idx],
                    THICK_TABLE[menu->thick_idx]);
        } else {
            _grab_start_dot(grab, event->x, event->y,
                    COLOR_TABLE[0],
                    THICK_TABLE[0]);
        }
    } else if (type & NEMOTALE_MOTION_EVENT) {
        if (menu) {
            if (menu->moving && grab->moving) {
                double x, y;
                x = event->x - menu->r;
                y = event->y - menu->r;
                nemoshow_canvas_translate(menu->canvas, x, y);
                if (event->x < -10 || event->x > (1024 + 10) ||
                        event->y < -10 || event->y > (512 + 10)) {
                    grab->moving = false;
                    menu->moving = false;
                    _menu_destroy(menu);
                }
                nemoshow_dispatch_frame(show);
            } else if (!_menu_is_menu(menu, event->x, event->y)) {
                _grab_add_dot(grab, event->x, event->y,
                        COLOR_TABLE[menu->color_idx],
                        THICK_TABLE[menu->thick_idx]);
            }
        } else {
            _grab_add_dot(grab, event->x, event->y,
                    COLOR_TABLE[0],
                    THICK_TABLE[0]);
        }
    }

    return 1;
}

static Grab *_grab_create(View *view, Menu *menu, struct nemotale *tale, uint32_t type, struct taleevent *event)
{
    Grab *grab = calloc(sizeof(Grab), 1);
    grab->view = view;
    grab->menu = menu;

    nemotale_prepare_grab(&grab->base, tale, event->device, _grab_dispatch);
    nemotale_dispatch_grab(tale, event->device, type, event);
    return grab;
}

static void _quit(Context *ctx)
{
    if (ctx->noevent) return;
    ctx->noevent = true;
    nemowidget_win_exit(ctx->win);
}

static void _tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
    nemotale_event_update_node_taps(tale, node, event, type);
    //int id = nemotale_node_get_id(node);
    //if (id != 1) return;

    struct nemoshow *show = nemotale_get_userdata(tale);
    struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);
    Context *ctx = nemoshow_get_userdata(show);
    View *view = ctx->view;
    if (ctx->noevent) return;

    if (nemoshow_event_is_single_click(tale, event, type)) {
        if (_is_lock(view, event->x, event->y)) {
            _lock_change(view);
            return;
        } else if (view->yn) {
            if (_is_yes(view, event->x, event->y)) {
                _quit(ctx);
                return;
            } else if (_is_no(view, event->x, event->y)) {
                _view_hide_yn(view);
                return;
            }
        } else if (_is_quit(view, event->x, event->y)) {
            _view_show_yn(view);
            return;
        }
    }

    if (!view->locked) {
        if (nemoshow_event_is_down(tale, event, type) ||
                nemoshow_event_is_up(tale, event, type)) {
            if (nemoshow_event_is_single_tap(tale, event, type)) {
                nemocanvas_move(canvas, event->taps[0]->serial);
            } else if (nemotale_is_double_taps(tale, event, type)) {
                nemocanvas_pick(canvas,
                        event->taps[0]->serial,
                        event->taps[1]->serial,
                        (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_SCALE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
            } else if (nemotale_is_many_taps(tale, event, type)) {
                nemotale_event_update_faraway_taps(tale, event);
                nemocanvas_pick(canvas,
                        event->tap0->serial,
                        event->tap1->serial,
                        (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
            }
        }
    }

    if (view->locked) {
        if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
            if (nemoshow_event_is_down(tale, event, type)) {
                Menu *menu;
                List *l;
                LIST_FOR_EACH(view->menus, l, menu) {
                    if (menu->moving) continue;
                    double cx, cy;
                    cx = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
                    cy = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

                    if (CIRCLE_IN(cx, cy, menu->r + 10, event->x, event->y)) {
                        _grab_create(view, menu, tale, type, event);
                        return;
                    }
                }

                _grab_create(view, NULL, tale, type, event);
                return;
            } else if (nemoshow_event_is_single_click(tale, event, type)) {
                Menu *menu;
                List *l;
                LIST_FOR_EACH(view->menus, l, menu) {
                    double cx, cy;
                    cx = NEMOSHOW_CANVAS_AT(menu->canvas, tx) + menu->r;
                    cy = NEMOSHOW_CANVAS_AT(menu->canvas, ty) + menu->r;

                    if (CIRCLE_IN(cx, cy, menu->r * 2 + 10, event->x, event->y)) {
                        if (_menu_is_thick_changer(menu, event->x, event->y)) {
                            _menu_thick_change(menu);
                        } else if (_menu_is_color_chager(menu, event->x, event->y)) {
                            _menu_color_change(menu, event->x, event->y);
                        }
                        return;
                    }
                }

                if (((event->time - view->clicked_time) < 300) &&
                        (fabs(event->x - view->clicked_x) < 100) &&
                        (fabs(event->y - view->clicked_y) < 100)) {
                    Menu *menu = _menu_create(view, event->x, event->y);
                    view->menus = list_append(view->menus, menu);
                }
                view->clicked_time = event->time;
                view->clicked_x = event->x;
                view->clicked_y = event->y;
            } else if (nemotale_is_motion_event(tale, event, type)) {
            }
        }
    }
}

static bool _config_load(Context *ctx, int *w, int *h)
{
    Config *config;
    config = config_load(PROJECT_NAME, CONFXML);
    if (!config) return false;

    config_get_wh(config, APPNAME, w, h);

    config_unload(config);

    return true;
}

int main(int argc, char *argv[])
{
    Context *ctx;
    int width= 1024, height = 512;

    ctx = calloc(sizeof(Context), 1);
    RET_IF(!ctx, -1);

    if (!_config_load(ctx, &width, &height)) return -1;

    NemoWidget *win;
    win = nemowidget_create_win(width, height);
    nemowidget_win_load_scene(win, width, height);
    //nemowidget_append_callback(win, "exit", _win_exit, ctx);
    ctx->win = win;

    NemoWidget *empty;
    empty = nemowidget_create_empty(win, width, height);
    nemowidget_empty_set_type(empty, NEMOWIDGET_EMPTY_TYPE_VECTOR);
    ctx->empty = empty;

    ctx->view = _view_create(empty, width, height);

    nemowidget_show(win, 0, 0, 0);
    nemowidget_win_run(win);
    nemowidget_destroy(win);

    free(ctx);

    return 0;
}
