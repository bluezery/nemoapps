#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define FRAME_TIMEOUT 2000
#define OVERLAY_TIMEOUT 1500

typedef enum {
    IMAGEITEM_TYPE_IMG = 0,
    IMAGEITEM_TYPE_SVG,
} ImageItemType;

typedef struct _ImageView ImageView;
typedef struct _ImageItem ImageItem;
struct _ImageItem {
    ImageView *view;
    char *path;
    char *name;

    ImageItemType type;
    Image *img;
    struct showone *one;
};

struct _ImageView {
    bool is_background;
    bool is_slideshow;
    int slideshow_timeout;
    int repeat;

    struct nemotool *tool;
    struct nemoshow *show;

    int width, height;

    Frame *frame;
    NemoWidget *widget;
    NemoWidgetGrab *grab;

    struct showone *group;
    List *items;
    int cnt;
    int idx;
    struct nemotimer *slideshow_timer;

    struct nemotimer *frame_timer;
    Sketch *sketch;

    NemoWidget *overlay;
    struct showone *overlay_group;
    struct showone *guide_group;
    struct showone *guide_left, *guide_right;

    DrawingBox *dbox;
    bool dbox_grab;
    double dbox_grab_diff_x, dbox_grab_diff_y;

    struct nemotimer *overlay_timer;
};

static void _imageview_frame_timeout(struct nemotimer *timer, void *userdata)
{
    ImageView *view = userdata;
    frame_gradient_motion(view->frame, NEMOEASE_CUBIC_INOUT_TYPE, FRAME_TIMEOUT, 0);
    nemotimer_set_timeout(timer, FRAME_TIMEOUT);
    nemoshow_dispatch_frame(view->show);
}

static void _imageview_overlay_timeout(struct nemotimer *timer, void *userdata)
{
    ImageView *view = userdata;
    nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0);
    nemowidget_hide(view->overlay, 0, 0, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _sketch_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    ImageView *view = userdata;
    if (view->sketch->enable) return;

    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_down(show, event)) {
        nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
        nemowidget_show(view->overlay, 0, 0, 0);
        nemotimer_set_timeout(view->overlay_timer, 0);
    } else if (nemoshow_event_is_up(show, event)) {
        nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);
    }
    nemoshow_dispatch_frame(view->show);
}

static void _drawingbox_adjust(ImageView *view, double tx, double ty)
{
    double gap = 5;
    double dx, dy;
    drawingbox_get_translate(view->dbox, &dx, &dy);
    if (tx < 0) tx = dx;
    if (ty < 0) ty = dy;

    double r;
    r = view->dbox->r;

    double x0, y0, x1, y1;

    // Move inside sketch area
    if (view->dbox->mode == 1) {
        r *= 3.5;
    }
    x0 = tx - r;
    x1 = tx + r;
    y0 = ty - r;
    y1 = ty + r;
    if (x0 < 0)
        tx = r + gap;
    else if (x1 > view->sketch->w)
        tx = view->sketch->w - r - gap;
    if (y0 < 0)
        ty = r + gap;
    else if (y1 > view->sketch->h)
        ty = view->sketch->h - r - gap;
    drawingbox_translate(view->dbox, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            tx, ty);
}

static void _image_load_done(bool success, void *userdata)
{
    RET_IF(!success);

    ImageItem *it = userdata;
    ImageView *view = it->view;

    image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 1.0);
    image_translate(it->img, 0, 0, 0, view->width/2, view->height/2);
    nemoshow_dispatch_frame(view->show);
}

void imageview_prev(ImageView *view)
{
    if (view->cnt <= 1) return;
    if (view->idx < 1) return;

    int prev_idx = view->idx;
    view->idx--;
    ERR("PREV page: %d", view->idx);

    ImageItem *prev_it = LIST_DATA(list_get_nth(view->items, prev_idx));
    ImageItem *it = LIST_DATA(list_get_nth(view->items, view->idx));
    if (it->type == IMAGEITEM_TYPE_IMG) {
        image_translate(it->img, 0, 0, 0, -view->width * 2, view->height/2);
        image_scale(it->img, 0, 0, 0, 2.0, 1.0);
        image_set_alpha(it->img, 0, 0, 0, 0.5);
        image_translate(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, view->width/2, view->height/2);
        image_scale(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 1.0, 1.0);
        if (!image_get_bitmap(it->img)) {
            image_load_fit(it->img, view->tool, it->path,
                    view->width, view->height, _image_load_done, it);
        } else {
            image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 1.0);
        }

        image_load_cancel(prev_it->img);
        image_translate(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, view->width, view->height/2);
        image_scale(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.001, 1.0);
        image_set_alpha(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 0.0);

    } else {
        nemoshow_item_set_alpha(prev_it->one, 0.0);
        nemoshow_item_set_alpha(it->one, 1.0);
    }
}

bool imageview_next(ImageView *view)
{
    if (view->cnt <= 1) return false;

    int prev_idx = view->idx;
    if (view->idx + 1 >= view->cnt) {
        if (view->repeat != 0) {
            view->idx = 0;
            if (view->repeat > 0) view->repeat--;
        } else
            return false;
    } else
        view->idx++;

    ERR("NEXT page: %d", view->idx);

    ImageItem *prev_it = LIST_DATA(list_get_nth(view->items, prev_idx));
    ImageItem *it = LIST_DATA(list_get_nth(view->items, view->idx));
    if (it->type == IMAGEITEM_TYPE_IMG) {
        image_translate(it->img, 0, 0, 0, view->width + view->width/2, view->height/2);
        image_scale(it->img, 0, 0, 0, 1.0, 1.0);
        image_set_alpha(it->img, 0, 0, 0, 0.25);
        image_translate(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, view->width/2, view->height/2);
        if (!image_get_bitmap(it->img)) {
            image_load_fit(it->img, view->tool, it->path,
                    view->width, view->height, _image_load_done, it);
        } else {
            image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 1.0);
        }

        image_load_cancel(prev_it->img);
        image_translate(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0, view->height/2);
        image_scale(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.001, 1.0);
        image_set_alpha(prev_it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 0.0);
    } else {
        nemoshow_item_set_alpha(prev_it->one, 0.0);
        nemoshow_item_set_alpha(it->one, 1.0);
    }
    return true;
}

static void _imageview_slideshow_timeout(struct nemotimer *timer, void *userdata)
{
    ImageView *view = userdata;

    if (imageview_next(view)) {
        nemotimer_set_timeout(view->slideshow_timer, view->slideshow_timeout * 1000);
    } else {
        view->is_slideshow = false;
    }
    nemoshow_dispatch_frame(view->show);
}

static void imageview_start_slideshow(ImageView *view)
{
    if (view->cnt <= 1) return;

    view->is_slideshow = true;
    nemotimer_set_timeout(view->slideshow_timer, view->slideshow_timeout * 1000);
    nemoshow_dispatch_frame(view->show);
}

static void imageview_stop_slideshow(ImageView *view)
{
    view->is_slideshow = false;
    nemotimer_set_timeout(view->slideshow_timer, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _imageview_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    ImageView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_single_click(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        double width, tx;
        nemowidget_get_geometry(view->widget, &tx, NULL, &width, NULL);
        if (ex < (width * 0.25)) {
            imageview_prev(view);
        } else if (ex > (width * 0.75)) {
            imageview_next(view);
        }
        view->grab = NULL;
    }
    nemoshow_dispatch_frame(view->show);
}

static void _overlay_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    ImageView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    struct showone *one = nemowidget_grab_get_data(grab, "button");
    const char *id = one->id;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    frame_util_transform_to_group(view->overlay_group, ex, ey, &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        if (one) {
            Button *btn = nemoshow_one_get_userdata(one);
            button_down(btn);
            nemowidget_enable_event_repeat(widget, false);

            double cx, cy;
            drawingbox_get_translate(view->dbox, &cx, &cy);
            view->dbox_grab_diff_x = cx - ex;
            view->dbox_grab_diff_y = cy - ey;
            view->dbox_grab = true;

            NemoWidget *win = nemowidget_get_top_widget(widget);
            nemowidget_win_enable_move(win, false);
            nemowidget_win_enable_rotate(win, false);
            nemowidget_win_enable_scale(win, false);
            nemotimer_set_timeout(view->frame_timer, 10);
        }

    } else if (nemoshow_event_is_motion(show, event)) {
        if (view->dbox_grab) {
            drawingbox_translate(view->dbox, 0, 0, 0,
                    ex + view->dbox_grab_diff_x,
                    ey + view->dbox_grab_diff_y);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        if (!view->sketch->enable) {
            NemoWidget *win = nemowidget_get_top_widget(widget);
            nemowidget_win_enable_move(win, true);
            nemowidget_win_enable_rotate(win, true);
            nemowidget_win_enable_scale(win, true);
            nemotimer_set_timeout(view->frame_timer, 0);
        }

        nemowidget_enable_event_repeat(widget, true);
        if (view->dbox_grab) {
            double tx, ty;
            tx = ex + view->dbox_grab_diff_x;
            ty = ey + view->dbox_grab_diff_y;

            _drawingbox_adjust(view, tx, ty);
            view->dbox_grab_diff_x = 0;
            view->dbox_grab_diff_y = 0;
            view->dbox_grab = false;
        }
        if (one) {
            Button *btn = nemoshow_one_get_userdata(one);
            button_up(btn);
        }

        if (id && !strcmp(id, "dbox")) {
            if (nemoshow_event_is_single_click(show, event)) {
                uint32_t tag = nemoshow_one_get_tag(one);
                if (tag == 10) {
                    nemotimer_set_timeout(view->overlay_timer, 0);

                    drawingbox_show_menu(view->dbox);
                    _drawingbox_adjust(view, -1, -1);

                    sketch_set_color(view->sketch, view->dbox->color);
                    sketch_set_size(view->sketch, view->dbox->size);
                    sketch_enable(view->sketch, true);
                    _nemoshow_item_motion(view->guide_group,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);

                    NemoWidget *win = nemowidget_get_top_widget(widget);
                    nemowidget_win_enable_move(win, false);
                    nemowidget_win_enable_rotate(win, false);
                    nemowidget_win_enable_scale(win, false);
                    nemotimer_set_timeout(view->frame_timer, 10);
                } else if (tag == 11) {
                    nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);

                    drawingbox_show_pencil(view->dbox);
                    _drawingbox_adjust(view, -1, -1);

                    sketch_enable(view->sketch, false);
                    _nemoshow_item_motion(view->guide_group,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);

                    NemoWidget *win = nemowidget_get_top_widget(widget);
                    nemowidget_win_enable_move(win, true);
                    nemowidget_win_enable_rotate(win, true);
                    nemowidget_win_enable_scale(win, true);
                    nemotimer_set_timeout(view->frame_timer, 0);
                } else if (tag == 12) {
                    // share
                } else if (tag == 13) {
                    sketch_undo(view->sketch, 1);
                } else if (tag == 14) {
                    sketch_undo(view->sketch, -1);
                } else if (tag == 15) {
                    drawingbox_change_stroke(view->dbox, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
                    sketch_set_size(view->sketch, view->dbox->size);
                } else if (tag == 16) {
                    drawingbox_change_color(view->dbox, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
                    sketch_set_color(view->sketch, view->dbox->color);
                }
            }
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void _overlay_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    ImageView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (!view->sketch->enable) {
        if (nemoshow_event_is_down(show, event)) {
            nemotimer_set_timeout(view->overlay_timer, 0);
            nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
            nemowidget_show(view->overlay, 0, 0, 0);
            nemoshow_dispatch_frame(view->show);
        } else if (nemoshow_event_is_up(show, event)) {
            nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);
        }
    }

    if (nemoshow_event_is_down(show, event)) {
        if (nemoshow_event_get_tapcount(event) >= 2) {
            nemoshow_event_set_done_all(event);
            return;
        }

        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            NemoWidgetGrab *grab = NULL;
            grab = nemowidget_create_grab(widget, event,
                    _overlay_grab_event, view);
            nemowidget_grab_set_data(grab, "button", one);
        } else {
            nemowidget_create_grab(widget, event,
                    _imageview_grab_event, view);
        }
    }
}

ImageView *imageview_create(NemoWidget *parent, int width, int height,
        const char *layer,
        List *files, int slideshow_timeout, int repeat)
{
    ImageView *view = calloc(sizeof(ImageView), 1);
    view->is_background = layer && !strcmp(layer, "background") ? true : false;
    view->slideshow_timeout = slideshow_timeout;
    view->repeat = repeat;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    view->frame = frame_create(parent, width, height, 2);

    int cw, ch;
    cw = view->frame->content_width;
    ch = view->frame->content_height;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, cw, ch);
    struct showone *group;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    frame_append_widget_group(view->frame, widget, group);

    List *l;
    FileInfo *file;
    LIST_FOR_EACH(files, l, file) {
        ImageItem *it = malloc(sizeof(ImageItem));
        it->view = view;
        it->path = strdup(file->path);
        it->name = strdup(file->name);
        if (file_is_svg(file->path)) {
            it->type = IMAGEITEM_TYPE_SVG;
            it->one = SVG_PATH_GROUP_CREATE(group, cw, ch, it->path);
            nemoshow_item_set_alpha(it->one, 0.0);
            it->img = NULL;
        } else {
            it->type = IMAGEITEM_TYPE_IMG;
            it->one = NULL;
            it->img = image_create(group);
            image_set_anchor(it->img, 0.5, 0.5);
            image_set_alpha(it->img, 0, 0, 0, 0.0);
        }
        view->items = list_append(view->items, it);
    }
    view->cnt = list_count(view->items);

    view->slideshow_timer = TOOL_ADD_TIMER(view->tool, 0,
            _imageview_slideshow_timeout, view);

    // Sketch
    view->frame_timer = TOOL_ADD_TIMER(view->tool, 0, _imageview_frame_timeout, view);
    view->sketch = sketch_create(parent, cw, ch);
    sketch_enable_blur(view->sketch, false);
    nemowidget_append_callback(view->sketch->widget, "event", _sketch_event, view);
    frame_append_widget_group(view->frame, view->sketch->widget,
            view->sketch->group);

    // Overlay
    view->overlay = widget = nemowidget_create_vector(parent, cw, ch);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _overlay_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    view->overlay_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    frame_append_widget_group(view->frame, widget, group);

    view->overlay_timer = TOOL_ADD_TIMER(view->tool, 0, _imageview_overlay_timeout,
            view);

    struct showone *one;
    int gap = 5;
    view->guide_group = group = GROUP_CREATE(group);
    nemoshow_item_set_alpha(group, 0.0);

    view->guide_left = one = RRECT_CREATE(group, cw * 0.25 - gap * 2, ch - gap * 2, 4, 4);
    nemoshow_item_translate(one, gap, gap);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    if (view->cnt <= 1 || view->is_background) {
        nemoshow_item_set_alpha(one, 0.0);
    } else {
        nemoshow_item_set_alpha(one, 0.5);
    }

    view->guide_right = one = RRECT_CREATE(group, cw * 0.25 - gap * 2, ch - gap, 4, 4);
    nemoshow_item_translate(one, cw * 0.75 + gap, gap);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    if (view->cnt <= 1 || view->is_background) {
        nemoshow_item_set_alpha(one, 0.0);
    } else {
        nemoshow_item_set_alpha(one, 0.5);
    }

    int r = 20;
    DrawingBox *dbox;
    view->dbox = dbox = drawingbox_create(view->overlay_group, r);
    drawingbox_translate(dbox, 0, 0, 0, width/2, height - r * 2);

    return view;
}

void imageview_destroy(ImageView *view)
{
    ImageItem *it;
    LIST_FREE(view->items, it) {
        free(it->path);
        free(it->name);
        if (it->one) nemoshow_one_destroy(it->one);
        if (it->img) image_destroy(it->img);
        free(it);
    }

    // XXX: remove from frame to destroy
    frame_remove_widget(view->frame, view->sketch->widget);
    sketch_destroy(view->sketch);

    nemotimer_destroy(view->overlay_timer);
    drawingbox_destroy(view->dbox);

    // XXX: Do not destroy view->widget, view->overlay
    // it will be destroyed automatically by frame_destroy
    frame_destroy(view->frame);
}

void imageview_hide(ImageView *view)
{
    imageview_stop_slideshow(view);

    nemotimer_set_timeout(view->overlay_timer, 0);
    sketch_hide(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    _nemoshow_item_motion(view->guide_group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    drawingbox_hide(view->dbox);

    frame_content_hide(view->frame, -1);
    frame_hide(view->frame);
    nemoshow_dispatch_frame(view->show);
}

void imageview_show(ImageView *view)
{
    frame_show(view->frame);
    frame_content_show(view->frame, -1);

    ImageItem *it = LIST_DATA(LIST_FIRST(view->items));
    if (it->type == IMAGEITEM_TYPE_IMG) {
        image_load_fit(it->img, view->tool, it->path,
                view->width, view->height, _image_load_done, it);
    } else {
        nemoshow_item_set_alpha(it->one, 1.0);
    }

    sketch_show(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    _nemoshow_item_motion(view->guide_group, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500,
            "alpha", 1.0, NULL);
    drawingbox_show(view->dbox);
    nemotimer_set_timeout(view->overlay_timer, 1000 + 400 + OVERLAY_TIMEOUT);
    nemoshow_dispatch_frame(view->show);
}

static void _win_fullscreen_callback(NemoWidget *win, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Fullscreen *fs = info;
    ImageView *view = userdata;

	if (fs->id) {
        frame_go_full(view->frame, fs->width, fs->height);
        imageview_start_slideshow(view);
    } else {
        frame_go_normal(view->frame);
        imageview_stop_slideshow(view);
    }
    nemoshow_dispatch_frame(view->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    ImageView *view = userdata;

    imageview_hide(view);

    nemowidget_win_exit_after(win, 500);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *path;
    bool is_slideshow;
    int slideshow_timeout;
    int repeat;
};

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->is_slideshow = false;
    app->repeat = -1;
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        const char *prefix = "config";
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/slideshow", prefix);
        temp = xml_get_value(xml, buf, "slideshow");
        if (temp && strlen(temp) > 0) {
            app->is_slideshow = !strcmp(temp, "on") ? true : false;
        }
        temp = xml_get_value(xml, buf, "timeout");
        if (temp && strlen(temp) > 0) {
            app->slideshow_timeout = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "repeat");
        if (temp && strlen(temp) > 0) {
            app->repeat = atoi(temp);
        }

        xml_unload(xml);
    }

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        {"slideshow", required_argument, NULL, 's'},
        {"timeout", required_argument, NULL, 't'},
        {"repeat", required_argument, NULL, 'p'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:s:t:p:", options, NULL))) {
        if (opt == -1) break;
        switch(opt) {
            case 'f':
                app->path = strdup(optarg);
                break;
            case 's':
                app->is_slideshow = !strcmp(optarg, "on") ? true : false;
                break;
            case 't':
                app->slideshow_timeout  = atoi(optarg);
                break;
            case 'p':
                app->repeat = atoi(optarg);
                break;
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->path) free(app->path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->path) {
        ERR("Usage: %s [-f FILENAME | DIRNAME] [-s on] [-t seconds] [-p -1/0/1]", APPNAME);
        return -1;
    }

    nemowrapper_init();

    List *files = NULL;
    FileInfo *file;
    if (file_is_dir(app->path)) {
        files = fileinfo_readdir(app->path);
        List *l, *ll;
        LIST_FOR_EACH_SAFE(files, l, ll, file) {
            if (file->is_dir) continue;
            if (!file_is_image(file->path) ||
                    file_is_svg(file->path)) {
                files = list_remove(files, file);
                fileinfo_destroy(file);
            }
        }
    } else {
        char *filename = file_get_basename(app->path);
        file = fileinfo_create(false, app->path, filename);
        files = list_append(files, file);

    }

    file = LIST_DATA(LIST_FIRST(files));
    RET_IF(!file, -1);
    if (!app->config->layer || strcmp(app->config->layer, "background")) {
        if (file_is_image(file->path)) {
            int w, h;
            if (!image_get_wh(file->path, &w, &h)) {
                ERR("image get wh failed: %s", file->path);
            } else {
                _rect_ratio_fit(w, h, app->config->width, app->config->height,
                        &(app->config->width), &(app->config->height));
            }
        } else if (file_is_svg(file->path)) {
            double w, h;
            if (!svg_get_wh(file->path, &w, &h)) {
                ERR("svg get wh failed: %s", file->path);
            } else {
                ERR("%lf %lf", w, h);
                _rect_ratio_fit(w, h, app->config->width, app->config->height,
                        &(app->config->width), &(app->config->height));
            }
        } else {
            ERR("file type is not an image or a svg");
        }
    }
    if (app->config->width <= 0 || app->config->height <= 0) {
        ERR("image or svg size is not correct: %d %d",
                app->config->width, app->config->height);
        return -1;
    }

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win;
    win =nemowidget_create_win_config(tool, APPNAME, app->config);
    RET_IF(!win, -1);
    if (app->config->layer && !strcmp(app->config->layer, "background")) {
        nemowidget_enable_event(win, false);
    }

    ImageView *view = imageview_create(win, app->config->width, app->config->height, app->config->layer,
            files, app->slideshow_timeout, app->repeat);
    nemowidget_append_callback(win, "fullscreen", _win_fullscreen_callback, view);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    imageview_show(view);
    if (app->is_slideshow) imageview_start_slideshow(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    imageview_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);

    LIST_FREE(files, file) fileinfo_destroy(file);
    nemowrapper_shutdown();

    _config_unload(app);

    return 0;
}
