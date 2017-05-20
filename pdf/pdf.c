#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>

#include <mupdf/fitz.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define COLOR0 0xEA562DFF
#define COLOR1 0x35FFFFFF
#define COLORBACK 0x10171E99

#define FRAME_TIMEOUT 2000
#define OVERLAY_TIMEOUT 1500

#define WIDGET_CNT 5

pthread_mutex_t __pdf_mutex;

typedef struct _NemoPdfPage NemoPdfPage;
struct _NemoPdfPage {
    fz_page *page;
    fz_pixmap *pixmap;
    pixman_image_t *pixman;
    int width;
    int height;
};

typedef struct _NemoPdf NemoPdf;
struct _NemoPdf {
    fz_context *ctx;
    fz_document *doc;
    int32_t cnt;
    int32_t width, height;
};

void  nemopdf_calculate_size(NemoPdf *pdf)
{
    fz_page *page;
    fz_pixmap *pixmap;
    fz_matrix matrix;
    fz_rect rect;
    fz_irect bbox;

    page = fz_load_page(pdf->ctx, pdf->doc, 0);
    if (!page) {
        ERR("fz_load_page failed");
        return;
    }

    fz_rotate(&matrix, 0);
    fz_pre_scale(&matrix, 1.0, 1.0);

    fz_bound_page(pdf->ctx, page, &rect);
    fz_transform_rect(&rect, &matrix);

    fz_round_rect(&bbox, &rect);

    pixmap = fz_new_pixmap_with_bbox(pdf->ctx, fz_device_rgb(pdf->ctx), &bbox);
    fz_clear_pixmap_with_value(pdf->ctx, pixmap, 0xff);
    pdf->width = pixmap->w;
    pdf->height = pixmap->h;
}

NemoPdfPage *nemopdf_load_page(NemoPdf *pdf, int idx, double sx, double sy)
{
    fz_page *page;
    fz_pixmap *pixmap;
    fz_matrix matrix;
    fz_rect rect;
    fz_irect bbox;

    page = fz_load_page(pdf->ctx, pdf->doc, idx);
    if (!page) return NULL;

    fz_rotate(&matrix, 0);
    fz_pre_scale(&matrix, sx, sy);

    fz_bound_page(pdf->ctx, page, &rect);
    fz_transform_rect(&rect, &matrix);

    fz_round_rect(&bbox, &rect);

    pixmap = fz_new_pixmap_with_bbox(pdf->ctx, fz_device_rgb(pdf->ctx), &bbox);
    fz_clear_pixmap_with_value(pdf->ctx, pixmap, 0xff);

    fz_device *dev;
    dev = fz_new_draw_device(pdf->ctx, pixmap);
    fz_run_page(pdf->ctx, page, dev, &matrix, NULL);
    fz_drop_device(pdf->ctx, dev);

    pixman_image_t *pixman;
    pixman = pixman_image_create_bits(PIXMAN_a8b8g8r8, pixmap->w, pixmap->h,
            (uint32_t *)(pixmap->samples), pixmap->w * pixmap->n);

    NemoPdfPage *_page = calloc(sizeof(NemoPdfPage), 1);
    _page->page = page;
    _page->pixman = pixman;
    _page->width = pixman_image_get_width(pixman);
    _page->height = pixman_image_get_height(pixman);

    return _page;
}

void nemopdf_unload_page(NemoPdf *pdf, NemoPdfPage *page)
{
    fz_drop_pixmap(pdf->ctx, page->pixmap);
    pixman_image_unref(page->pixman);
    free(page);
}

NemoPdf *nemopdf_create(const char *filepath)
{
    NemoPdf *pdf;

    pdf = calloc(sizeof(NemoPdf), 1);
    if (!pdf) return NULL;

    pdf->ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!pdf->ctx) {
        free(pdf);
        return NULL;
    }

    fz_register_document_handlers(pdf->ctx);

    pdf->doc = fz_open_document(pdf->ctx, filepath);
    if (!pdf->doc) {
        fz_drop_context(pdf->ctx);
        free(pdf);
        return NULL;
    }

    pdf->cnt = fz_count_pages(pdf->ctx, pdf->doc);
    nemopdf_calculate_size(pdf);

    return pdf;
}

void nemopdf_destroy(NemoPdf *pdf)
{
    fz_drop_document(pdf->ctx, pdf->doc);
    fz_drop_context(pdf->ctx);
    free(pdf);
}

typedef struct _PdfView PdfView;
struct _PdfView {
    struct nemoshow *show;
    struct nemotool *tool;

    bool is_background;
    bool is_slideshow;
    int slideshow_timeout;
    int repeat;

    NemoPdf *pdf;
    int32_t pageidx;
    Worker *worker;

    NemoWidget *parent;
    int width, height;
    int max_width, max_height;
    int real_width, real_height;

    NemoWidgetGrab *grab;

    Frame *frame;

    List *pages;
    NemoWidget *widgets[WIDGET_CNT];
    int widget_idx;

    NemoWidget *event;

    struct nemotimer *slideshow_timer;
    bool is_start;

    struct nemotimer *frame_timer;
    Sketch *sketch;

    NemoWidget *overlay;
    struct showone *overlay_group;

    struct showone *guide_group;
    struct showone *guide_left;
    struct showone *guide_right;

    DrawingBox *dbox;
    bool dbox_grab;
    double dbox_grab_diff_x, dbox_grab_diff_y;

    struct nemotimer *overlay_timer;
};

typedef struct _PdfViewPage PdfViewPage;
struct _PdfViewPage {
    PdfView *view;
    NemoPdfPage *page;
};

static PdfView *pdfview_create(Config *config,
        const char *path, int slideshow_timeout, int repeat)
{
    PdfView *view = calloc(sizeof(PdfView), 1);
    view->slideshow_timeout = slideshow_timeout;
    view->repeat = repeat;
	view->pdf = nemopdf_create(path);

    if (!config->layer || strcmp(config->layer, "background")) {
        _rect_ratio_fit(view->pdf->width, view->pdf->height,
                config->width, config->height, &(config->width), &(config->height));
    }

    return view;
}

typedef struct _WorkData WorkData;
struct _WorkData {
    PdfView *view;
    int idx;
    double sx, sy;
    NemoPdfPage *page;
};

static void _pdfview_work(void *userdata)
{
    WorkData *data = userdata;

    // FIXME: When program exits, this thread cannot be clean-up correctly.
    //ERR("[%d], [%p] %lf, %lf", page->idx, page->view->pdf, page->sx, page->sy);
    pthread_mutex_lock(&__pdf_mutex);
    data->page = nemopdf_load_page(data->view->pdf, data->idx, data->sx, data->sy);
    pthread_mutex_unlock(&__pdf_mutex);
}

static void _pdfview_work_done(bool cancel, void *userdata)
{
    WorkData *data = userdata;
    PdfView *view = data->view;

    //XXX: eventhough it is canceled, show it anyway.
    if (data->idx == view->pageidx) {
        if (data->page) {
            NemoWidget *widget;
            widget = view->widgets[view->widget_idx];
            pixman_composite_fit(nemowidget_get_pixman(widget), data->page->pixman);
            nemowidget_dirty(widget);
            if (view->is_start) {
                frame_content_show(view->frame, view->widget_idx);
                view->is_start = false;
            }
            nemoshow_dispatch_frame(view->show);
        } else {
            ERR("Page is not loaded correctly");
        }
    }

    if (cancel) {
        if (data->page) nemopdf_unload_page(view->pdf, data->page);
    } else {
        PdfViewPage *page = LIST_DATA(list_get_nth(view->pages, data->idx));
        page->page = data->page;
    }
    free(data);
}

static void _pdfview_append_work(PdfView *view, int idx)
{
    WorkData *data = calloc(sizeof(WorkData), 1);
    data->view = view;
    data->idx = idx;

    int w, h;
    _rect_ratio_fit(view->pdf->width, view->pdf->height, view->width, view->height,
            &w, &h);
    data->sx = (double)w/view->pdf->width;
    data->sy = (double)h/view->pdf->height;

    worker_append_work(view->worker, _pdfview_work, _pdfview_work_done, data);
}

static void pdfview_start_work(PdfView *view)
{
    int idx0, idx1;
    idx0 = view->pageidx;
    idx1 = idx0 + 1;

    while (idx0 >= 0 || idx1 < view->pdf->cnt) {
        if (idx0 >= 0) {
            _pdfview_append_work(view, idx0--);
        }
        if (idx1 < view->pdf->cnt) {
            _pdfview_append_work(view, idx1++);
        }
    }
    worker_start(view->worker);
}

static void pdfview_stop_work(PdfView *view)
{
    worker_stop(view->worker);
}

static void pdfview_destroy_work(PdfView *view)
{
    worker_destroy(view->worker);
}

static void _pdfview_resize(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    PdfView *view = userdata;
    NemoWidgetInfo_Resize *resize = info;
    if (resize->width == 0 || resize->height == 0) return;

    NemoWidget *cur = view->widgets[view->widget_idx];

    pixman_image_t *pixman = nemowidget_get_pixman(cur);
    view->width = pixman_image_get_width(pixman);
    view->height = pixman_image_get_height(pixman);
    _rect_ratio_fit(view->pdf->width, view->pdf->height,
            view->width , view->height, &(view->real_width), &(view->real_height));

    PdfViewPage *page = LIST_DATA(list_get_nth(view->pages, view->pageidx));
    if (page->page) {
        pixman_composite_fit(pixman, page->page->pixman);
        nemowidget_dirty(widget);
    }

    if (view->width > view->max_width || view->height > view->max_height) {
        pdfview_stop_work(view);
        pdfview_start_work(view);
        view->max_width = view->width;
        view->max_height = view->height;

    }
}

static void pdfview_prev(PdfView *view)
{
    if (view->pdf->cnt == 1) return;
    if (view->pageidx < 1) return;

    view->pageidx--;
    ERR("PREV page: %d", view->pageidx);

    PdfViewPage *page = LIST_DATA(list_get_nth(view->pages, view->pageidx));

    int cur_idx;
    int next_idx;
    cur_idx = view->widget_idx;
    if (cur_idx - 1 <= 0) {
        next_idx = WIDGET_CNT - 1;
    } else {
        next_idx = cur_idx - 1;
    }

    NemoWidget *cur = view->widgets[cur_idx];
    NemoWidget *next = view->widgets[next_idx];

    nemowidget_stack_above(next, cur);
    if (page->page) {
        pixman_composite_fit(nemowidget_get_pixman(next), page->page->pixman);
        nemowidget_dirty(next);
    }

    frame_go_widget_prev(view->frame, cur_idx, next_idx);

    view->widget_idx = next_idx;
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_next(PdfView *view)
{
    if (view->pdf->cnt == 1) return;

    if (view->pageidx + 1 >= view->pdf->cnt) {
        if (view->repeat != 0) {
            view->pageidx = 0;
            if (view->repeat > 0) view->repeat--;
        } else
            return;
    } else
        view->pageidx++;
    ERR("NEXT page: %d", view->pageidx);

    PdfViewPage *page = LIST_DATA(list_get_nth(view->pages, view->pageidx));

    int cur_idx;
    int next_idx;
    cur_idx = view->widget_idx;
    if (cur_idx + 1 >= WIDGET_CNT) {
        next_idx = 0;
    } else {
        next_idx = cur_idx + 1;
    }

    NemoWidget *cur = view->widgets[cur_idx];
    NemoWidget *next = view->widgets[next_idx];

    nemowidget_stack_above(next, cur);
    if (page->page) {
        pixman_composite_fit(nemowidget_get_pixman(next), page->page->pixman);
        nemowidget_dirty(next);
    }

    frame_go_widget_next(view->frame, cur_idx, next_idx);

    view->widget_idx = next_idx;
    nemoshow_dispatch_frame(view->show);
}

static void _pdfview_frame_timeout(struct nemotimer *timer, void *userdata)
{
    PdfView *view = userdata;
    frame_gradient_motion(view->frame, NEMOEASE_CUBIC_INOUT_TYPE, FRAME_TIMEOUT, 0);
    nemotimer_set_timeout(timer, FRAME_TIMEOUT);
    nemoshow_dispatch_frame(view->show);
}

static void _pdfview_overlay_timeout(struct nemotimer *timer, void *userdata)
{
    PdfView *view = userdata;
    nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0);
    nemowidget_hide(view->overlay, 0, 0, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _sketch_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    PdfView *view = userdata;
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

static void _drawingbox_adjust(PdfView *view, double tx, double ty)
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

static void _pdfview_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    PdfView *view = userdata;

    if (nemoshow_event_is_down(show, event)) {
    } else if (nemoshow_event_is_up(show, event)) {
        if (nemoshow_event_is_single_click(show, event)) {

            double ex;
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event), &ex, NULL);

            double width, tx;
            nemowidget_get_geometry(view->event, &tx, NULL, &width, NULL);

            if (ex < (width * 0.25)) {
                pdfview_prev(view);
            } else if (ex > (width * 0.75)) {
                pdfview_next(view);
            }
        }
        view->grab = NULL;
    }
    nemoshow_dispatch_frame(view->show);
}

static void _overlay_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    PdfView *view = userdata;
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

static void _overlay_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    PdfView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (!view->sketch->enable) {
        if (nemoshow_event_is_down(show, event)) {
            nemotimer_set_timeout(view->overlay_timer, 0);
            nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
            nemowidget_show(view->overlay, 0, 0, 0);
        } else if (nemoshow_event_is_up(show, event)) {
            nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);
        }
    }

    if (nemoshow_event_is_down(show, event)) {
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
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void _pdfview_page_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    PdfView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);
    if (view->sketch->enable) return;


    if (nemoshow_event_is_down(show, event)) {
        nemowidget_set_alpha(view->overlay, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
        nemowidget_show(view->overlay, 0, 0, 0);
        nemotimer_set_timeout(view->overlay_timer, 0);
        nemoshow_dispatch_frame(view->show);
    } else if (nemoshow_event_is_up(show, event)) {
        nemotimer_set_timeout(view->overlay_timer, OVERLAY_TIMEOUT);
    }

    if (nemoshow_event_is_down(show, event)) {
        if (nemoshow_event_get_tapcount(event) >= 2) {
            nemoshow_event_set_done_all(event);
            return;
        }

        if (view->grab) return;
        view->grab = nemowidget_create_grab(widget, event,
                _pdfview_grab_event, view);
    }
}

static void _pdfview_slideshow_timeout(struct nemotimer *timer, void *userdata)
{
    PdfView *view = userdata;

    pdfview_next(view);
    if (view->is_slideshow) {
        nemotimer_set_timeout(view->slideshow_timer, view->slideshow_timeout * 1000);
    }
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_start_slideshow(PdfView *view)
{
    view->is_slideshow = true;
    nemotimer_set_timeout(view->slideshow_timer, view->slideshow_timeout * 1000);
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_stop_slideshow(PdfView *view)
{
    view->is_slideshow = false;
    nemotimer_set_timeout(view->slideshow_timer, 0);
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_attach(PdfView *view, NemoWidget *parent, int width, int height, const char *layer)
{
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->worker = worker_create(view->tool);
    view->is_background = layer && !strcmp(layer, "background") ? true : false;

    view->width = width;
    view->height = height;
    _rect_ratio_fit(view->pdf->width, view->pdf->height,
            view->width , view->height, &(view->real_width), &(view->real_height));

    int pdf_gap = 2;
    view->frame = frame_create(parent, width, height, pdf_gap);
    int cw, ch;
    cw = view->frame->content_width;
    ch = view->frame->content_height;

    int i = 0;
    for (i = 0 ; i < view->pdf->cnt ; i++) {
        PdfViewPage *page = calloc(sizeof(PdfViewPage), 1);
        page->view = view;
        view->pages = list_append(view->pages, page);
    }

    view->widget_idx = 0;

    NemoWidget *widget;
    struct showone *group;
    struct showone *one;

    for (i = 0 ; i < WIDGET_CNT ; i++) {
        widget = view->widgets[i] = nemowidget_create_pixman(parent, cw, ch);
        frame_append_widget(view->frame, widget);
        nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    }

    view->slideshow_timer = TOOL_ADD_TIMER(view->tool, 0, _pdfview_slideshow_timeout, view);

    // Sketch
    view->frame_timer = TOOL_ADD_TIMER(view->tool, 0, _pdfview_frame_timeout, view);
    view->sketch = sketch_create(parent, cw, ch);
    sketch_enable_blur(view->sketch, false);
    nemowidget_append_callback(view->sketch->widget, "event", _sketch_event, view);
    frame_append_widget_group(view->frame, view->sketch->widget,
            view->sketch->group);

    // Event
    view->event = widget = nemowidget_create_pixman(parent, cw, ch);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_append_callback(widget, "resize", _pdfview_resize, view);
    nemowidget_append_callback(widget, "event", _pdfview_page_event, view);
    frame_append_widget(view->frame, widget);

    // Overlay
    view->overlay = widget = nemowidget_create_vector(parent, cw, ch);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _overlay_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    view->overlay_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    frame_append_widget_group(view->frame, widget, group);

    view->overlay_timer = TOOL_ADD_TIMER(view->tool, 0, _pdfview_overlay_timeout,
            view);

    int gap = 5;
    view->guide_group = group = GROUP_CREATE(group);
    nemoshow_item_set_alpha(group, 0.0);

    view->guide_left = one = RRECT_CREATE(group, cw * 0.25 - gap * 2, ch - gap * 2, 4, 4);
    nemoshow_item_translate(one, gap, gap);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    if (view->pdf->cnt <= 1 || view->is_background) {
        nemoshow_item_set_alpha(one, 0.0);
    } else {
        nemoshow_item_set_alpha(one, 0.5);
    }

    view->guide_right = one = RRECT_CREATE(group, cw * 0.25 - gap * 2, ch - gap, 4, 4);
    nemoshow_item_translate(one, cw * 0.75 + gap, gap);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    if (view->pdf->cnt <= 1 || view->is_background) {
        nemoshow_item_set_alpha(one, 0.0);
    } else {
        nemoshow_item_set_alpha(one, 0.5);
    }

    int r = 20;
    DrawingBox *dbox;
    view->dbox = dbox = drawingbox_create(view->overlay_group, r);
    drawingbox_translate(dbox, 0, 0, 0, width/2, height - r * 2);

}

static void pdfview_hide(PdfView *view)
{
    pdfview_stop_work(view);
    pdfview_stop_slideshow(view);

    nemotimer_set_timeout(view->overlay_timer, 0);
    nemowidget_hide(view->overlay, 0, 0, 0);
    sketch_hide(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    _nemoshow_item_motion(view->guide_group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    drawingbox_hide(view->dbox);

    frame_hide(view->frame);
    frame_content_hide(view->frame, WIDGET_CNT);
    frame_content_hide(view->frame, view->widget_idx);
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_show(PdfView *view)
{
    view->is_start = true;
    frame_show(view->frame);
    frame_content_show(view->frame, WIDGET_CNT);
    frame_content_show(view->frame, WIDGET_CNT + 1);
    frame_content_show(view->frame, WIDGET_CNT + 2);

    sketch_show(view->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    _nemoshow_item_motion(view->guide_group, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500,
            "alpha", 1.0, NULL);
    drawingbox_show(view->dbox);
    nemotimer_set_timeout(view->overlay_timer, 1000 + 400 + OVERLAY_TIMEOUT);

    pdfview_start_work(view);
    nemoshow_dispatch_frame(view->show);
}

static void pdfview_destroy(PdfView *view)
{
    pdfview_destroy_work(view);
    nemotimer_destroy(view->slideshow_timer);

    PdfViewPage *page;
    LIST_FREE(view->pages, page) {
        if (page->page) nemopdf_unload_page(view->pdf, page->page);
        free(page);
    }
    pthread_mutex_lock(&__pdf_mutex);
    nemopdf_destroy(view->pdf);
    pthread_mutex_unlock(&__pdf_mutex);

    frame_remove_widget(view->frame, view->sketch->widget);
    sketch_destroy(view->sketch);
    nemotimer_destroy(view->overlay_timer);
    drawingbox_destroy(view->dbox);

    frame_destroy(view->frame);

    free(view);
}

static void _win_fullscreen_callback(NemoWidget *win, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Fullscreen *fs = info;
    PdfView *view = userdata;

	if (fs->id) {
        frame_go_full(view->frame, fs->width, fs->height);
        pdfview_start_slideshow(view);
    } else {
        frame_go_normal(view->frame);
        pdfview_stop_slideshow(view);
    }
    nemoshow_dispatch_frame(view->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    PdfView *view = userdata;

    pdfview_hide(view);

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
    app->slideshow_timeout = 0;
    app->repeat = 0;
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

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

    pthread_mutex_init(&__pdf_mutex, NULL);
    nemowrapper_init();

    PdfView *view = pdfview_create(app->config,
            app->path, app->slideshow_timeout, app->repeat);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);
    if (app->config->layer && !strcmp(app->config->layer, "background")) {
        nemowidget_enable_event(win, false);
    }

    pdfview_attach(view, win, app->config->width, app->config->height, app->config->layer);
    nemowidget_append_callback(win, "fullscreen", _win_fullscreen_callback, view);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    pdfview_show(view);
    if (app->is_slideshow) pdfview_start_slideshow(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    pdfview_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);

    nemowrapper_shutdown();
    pthread_mutex_destroy(&__pdf_mutex);

    _config_unload(app);

    return 0;
}
