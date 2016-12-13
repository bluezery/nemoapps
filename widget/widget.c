#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nemoglib.h>

#include "nemoutil.h"
#include "widget.h"

// ************************************** //
// NemoWidget //
// ************************************** //
struct _NemoWidgetMotionFrame
{
    double t;
    List *attrs;
};

struct _NemoWidgetMotion
{
    NemoWidget *widget;
    struct showone *ease;
    struct showtransition *trans;
    struct showone *seq;
    List *frames;
};

static int _nemowidget_contain_point(struct nemoshow *show, struct showone *canvas, float x, float y)
{
    NemoWidget *widget = nemoshow_one_get_userdata(canvas);

    RET_IF(!widget->klass->area, 0);

    if (widget->klass->area(widget, x, y)) return 1;
    return 0;
}

NemoWidget *nemowidget_get_parent(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->parent;
}

void nemowidget_set_framerate(NemoWidget *widget, int framerate)
{
    NEMOWIDGET_CHECK(widget);
    struct nemoshow *show = nemowidget_get_show(widget);
    nemoshow_view_set_framerate(show, framerate);
}

void nemowidget_set_canvas(NemoWidget *widget, struct showone *canvas)
{
    NEMOWIDGET_CHECK(widget);
    if (widget->canvas) {
        ERR("Previous widget's canvas will be deleted");
        nemoshow_canvas_destroy(widget->canvas);
    }
    widget->canvas = canvas;
}

struct showone *nemowidget_get_canvas(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->canvas;
}

NemoWidgetClass *nemowidget_get_klass(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->klass;
}

void *nemowidget_get_context(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->context;
}

NemoWidgetData *_nemowidget_search_data(NemoWidget *widget, const char *key)
{
    List *l;
    NemoWidgetData *data;
    LIST_FOR_EACH(widget->datas, l, data) {
        if (data->key && key) {
            break;
        }
    }
    return data;
}

void nemowidget_set_data(NemoWidget *widget, const char *key, void *userdata)
{
    NEMOWIDGET_CHECK(widget);
    RET_IF(!key);

    NemoWidgetData *data = _nemowidget_search_data(widget, key);
    if (!data) {
        data = calloc(sizeof(NemoWidgetData), 1);
        data->key = strdup(key);
        widget->datas = list_append(widget->datas, data);
    }
    data->data = userdata;
}

void *nemowidget_get_data(NemoWidget *widget, const char *key)
{
    NEMOWIDGET_CHECK(widget, NULL);
    RET_IF(!key, NULL);

    NemoWidgetData *data = _nemowidget_search_data(widget, key);
    return data ? data->data : NULL;
}

static void _nemowidget_win_core_init(NemoWidget *win)
{
    NEMOWIDGET_CHECK(win);
    if (win->core) free(win->core);
    win->core = calloc(sizeof(NemoWidgetCore), 1);
}

static void _nemowidget_win_core_shutdown(NemoWidget *win)
{
    NEMOWIDGET_CHECK(win);
    RET_IF(!win->core);
    free(win->core->uuid);
    free(win->core);
}

const char *nemowidget_get_uuid(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->core->uuid;
}

struct nemotool *nemowidget_get_tool(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->core->tool;
}

struct nemotale *nemowidget_get_tale(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->core->tale;
}

struct nemoshow *nemowidget_get_show(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->core->show;
}

struct showone *nemowidget_get_scene(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->core->scene;
}

uint32_t nemowidget_get_id(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, 0);
    return widget->id;
}

void nemowidget_set_id(NemoWidget *widget, uint32_t id)
{
    NEMOWIDGET_CHECK(widget);
    widget->id = id;
}

void nemowidget_set_tag(NemoWidget *widget, const char *tag)
{
    NEMOWIDGET_CHECK(widget);
    if (widget->tag) free(widget->tag);
    widget->tag = strdup(tag);
}

const char *nemowidget_get_tag(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    return widget->tag;
}

NemoWidget *nemowidget_get_top_widget(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    while (widget->parent) {
        widget = widget->parent;
    }
    return widget;
}

GLuint nemowidget_get_texture(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, 0);
    if (widget->klass->type != NEMOWIDGET_TYPE_OPENGL) {
        ERR("Widget[%p,%s] is not opengl",
                widget, widget->klass->name);
        return 0;
    }
    struct showone *canvas = nemowidget_get_canvas(widget);
    RET_IF(!canvas, 0);

    GLuint texture = nemoshow_canvas_get_texture(canvas);
    return texture;
}

void *nemowidget_get_buffer(NemoWidget *widget, int *w, int *h, int *stride)
{
    NEMOWIDGET_CHECK(widget, NULL);
    if (widget->klass->type != NEMOWIDGET_TYPE_PIXMAN &&
            widget->klass->type != NEMOWIDGET_TYPE_VECTOR) {
        ERR("Widget[%p,%s] is neither pixman nor vector",
                widget, widget->klass->name);
        return NULL;
    }
    struct showone *canvas = nemowidget_get_canvas(widget);
    RET_IF(!canvas, NULL);

    pixman_image_t *image = nemoshow_canvas_get_pixman(canvas);
    if (w) *w = pixman_image_get_width(image);
    if (h) *h = pixman_image_get_height(image);
    if (stride) *stride = pixman_image_get_stride(image);

    struct talenode *node = nemoshow_canvas_get_node(canvas);
    return nemotale_node_get_buffer(node);
}

pixman_image_t *nemowidget_get_pixman(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    if (widget->klass->type != NEMOWIDGET_TYPE_PIXMAN &&
            widget->klass->type != NEMOWIDGET_TYPE_VECTOR) {
        ERR("Widget[%p,%s] is neither pixman nor vector",
                widget, widget->klass->name);
        return NULL;
    }
    struct showone *canvas = nemowidget_get_canvas(widget);
    RET_IF(!canvas, NULL);
    return nemoshow_canvas_get_pixman(canvas);
}

void nemowidget_get_geometry(NemoWidget *widget, double *x, double *y, double *w, double *h)
{
    NEMOWIDGET_CHECK(widget);
    struct showone *canvas = nemowidget_get_canvas(widget);
    RET_IF(!canvas);

    if (x) *x = NEMOSHOW_CANVAS_AT(canvas, tx);
    if (y) *y = NEMOSHOW_CANVAS_AT(canvas, ty);
    if (w) *w = nemoshow_canvas_get_width(canvas);
    if (h) *h = nemoshow_canvas_get_height(canvas);
}

struct showone *nemowidget_pick_one(NemoWidget *widget, int x, int y)
{
    NEMOWIDGET_CHECK(widget, NULL);

    struct showone *canvas = nemowidget_get_canvas(widget);
    return nemoshow_canvas_pick_one(canvas, x, y);

}

//XXX: child transform is not applied!!!
void nemowidget_motion(NemoWidget *widget, uint32_t easetype, uint32_t duration, uint32_t delay, ...)
{
    NEMOWIDGET_CHECK(widget);
    struct nemoshow *show;
    show = nemowidget_get_show(widget);

    NemoMotion *m = nemomotion_create(show, easetype, duration, delay);

    const char *name;

    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 1.0, widget->canvas, name, value, NULL);
        } else {
            double value = va_arg(ap, double);
            nemomotion_attach(m, 1.0, widget->canvas, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

void nemowidget_motion_bounce(NemoWidget *widget, uint32_t easetype, uint32_t duration, uint32_t delay, ...)
{
    NEMOWIDGET_CHECK(widget);
    struct nemoshow *show;
    show = nemowidget_get_show(widget);

    NemoMotion *m = nemomotion_create(show, easetype, duration, delay);

    const char *name;
    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t bounce = va_arg(ap, uint32_t);
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 0.8, widget->canvas, name, bounce, NULL);
            nemomotion_attach(m, 1.0, widget->canvas, name, value, NULL);
        } else {
            double bounce = va_arg(ap, double);
            double value = va_arg(ap, double);
            nemomotion_attach(m, 0.8, widget->canvas, name, bounce, NULL);
            nemomotion_attach(m, 1.0, widget->canvas, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

void nemowidget_dirty(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    nemoshow_canvas_damage_all(nemowidget_get_canvas(widget));
}

void nemowidget_dispatch_frame(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    struct nemoshow *show = nemowidget_get_show(widget);
    nemoshow_dispatch_frame(show);
}

static void _nemowidget_process_scope_recursive(NemoWidget *widget)
{
    if (widget->scope) {
        nemowidget_set_scope(widget);
    }
    NemoWidget *child;
    INLIST_FOR_EACH(widget->children, child) {
        _nemowidget_process_scope_recursive(child);
    }
}

static void _nemowidget_dispatch_canvas_resize(struct nemoshow *show, struct showone *canvas, int32_t width, int32_t height)
{
	if (width == 0 || height == 0) return;
    NemoWidget *widget = nemoshow_one_get_userdata(canvas);

    if (widget->klass->type == NEMOWIDGET_TYPE_WIN) {
        nemowidget_put_scope(widget);
        _nemowidget_process_scope_recursive(widget);
    }

    NemoWidgetInfo_Resize resize;
    resize.width = width;
    resize.height = height;
    nemowidget_callback_dispatch(widget, "resize", &resize);
}

static void _nemowidget_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
    NemoWidget *widget = nemoshow_one_get_userdata(canvas);

    nemowidget_callback_dispatch(widget, "frame", NULL);
    nemowidget_update(widget);

    if (widget->klass->type == NEMOWIDGET_TYPE_VECTOR) {
        nemoshow_canvas_render_vector(show, canvas);
    }
}

NemoWidget *nemowidget_create(NemoWidgetClass *klass, NemoWidget *parent, int width, int height)
{
    RET_IF(!klass, NULL);
    RET_IF(klass->type >= NEMOWIDGET_TYPE_MAX, NULL);
    RET_IF(!klass->create, NULL);
    RET_IF(!parent, NULL);
    RET_IF(width <= 0 || height <= 0, NULL);

    NemoWidget *widget;
    widget = calloc(sizeof(NemoWidget), 1);
    widget->klass = klass;

    widget->target.width = width;
    widget->target.height = height;
    widget->target.sx = 1.0;
    widget->target.sy = 1.0;
    widget->event_enable = true;
    widget->event_grab_enable = true;
    nemowidget_call_register(widget, "event");
    nemowidget_call_register(widget, "resize");
    nemowidget_call_register(widget, "frame");

    parent->children = inlist_append(parent->children, INLIST(widget));
    widget->parent = parent;

    struct nemoshow *show;
    show = nemowidget_get_show(parent);
    widget->core = parent->core;

    struct showone *canvas;
    canvas = nemoshow_canvas_create();
    nemoshow_canvas_set_width(canvas, width);
    nemoshow_canvas_set_height(canvas, height);
    if (klass->type == NEMOWIDGET_TYPE_VECTOR) {
        nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
    } else if (klass->type == NEMOWIDGET_TYPE_PIXMAN) {
        nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
    } else if (klass->type == NEMOWIDGET_TYPE_OPENGL) {
        nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
    }

    nemoshow_canvas_pivot(canvas, 0, 0);
    nemoshow_one_set_userdata(canvas, widget);
    nemoshow_canvas_set_dispatch_redraw(canvas, _nemowidget_dispatch_canvas_redraw);
    nemoshow_canvas_set_dispatch_resize(canvas, _nemowidget_dispatch_canvas_resize);
    //nemotale_node_set_data(nemoshow_canvas_get_node(canvas), widget);
    nemoshow_one_attach(show->scene, canvas);
    nemowidget_set_canvas(widget, canvas);

    if (widget->klass->area) {
        nemoshow_canvas_set_contain_point(canvas, _nemowidget_contain_point);
    }

    widget->context = widget->klass->create(widget, width, height);

    return widget;
}

void nemowidget_destroy(NemoWidget *widget);

void _nemowidget_destroy_idle(void *data)
{
    NemoWidget *widget = data;
    NEMOWIDGET_CHECK(widget);

    nemowidget_detach_scope(widget);
    nemowidget_detach_region(widget);
    nemograbs_destroy(widget->usergrabs);
    nemograbs_destroy(widget->grabs);

    NemoWidgetContainer *container;
    INLIST_FREE(widget->containers, container) {
        free(container->name);
        free(container);
    }

    NemoWidgetCall *call;
    LIST_FREE(widget->calls, call) {
        NemoWidgetCallback *callback;
        LIST_FREE(call->callbacks, callback) {
            free(callback);
        }
        free(call->id);
        free(call);
    }

    NemoWidgetData *userdata;
    LIST_FREE(widget->datas, userdata) {
        free(userdata);
    }

    Inlist *l;
    while ((l = INLIST_FIRST(widget->items))) {
        NemoWidgetItem *it = INLIST_DATA(l, NemoWidgetItem);
        nemowidget_item_remove(it);
    }

    widget->klass->destroy(widget);
    if (widget->canvas) nemoshow_canvas_destroy(widget->canvas);

    if (widget->klass->type == NEMOWIDGET_TYPE_WIN) {
        nemoshow_destroy_view(widget->core->show);
        _nemowidget_win_core_shutdown(widget);
    }

    free(widget);
}

void nemowidget_destroy(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    if (widget->parent)
        widget->parent->children =
            inlist_remove(widget->parent->children, INLIST(widget));
    Inlist *l;
    while ((l = INLIST_FIRST(widget->children))) {
        NemoWidget *child =
            INLIST_DATA(l, NemoWidget);
        nemowidget_destroy(child);
    }

    _nemowidget_destroy_idle(widget);
    //nemotool_dispatch_idle(nemowidget_get_tool(widget), _nemowidget_destroy_idle, widget);
}

void nemowidget_show(NemoWidget *widget, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK(widget);

    widget->is_show = true;
    if (widget->klass->show) {
        widget->klass->show(widget, easetype, duration, delay);
    }
}

void nemowidget_hide(NemoWidget *widget, uint32_t easetype, int duration, int delay)
{
    NEMOWIDGET_CHECK(widget);

    widget->is_show = false;
    if (widget->klass->hide) {
        widget->klass->hide(widget, easetype, duration, delay);
    }
}

void nemowidget_update(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    if (widget->klass->update) {
        widget->klass->update(widget);
    }
}

void nemowidget_resize(NemoWidget *widget, double width, double height)
{
    /*
    if (EQUAL(widget->target.width, width) && EQUAL(widget->target.height, height))
        return;
        */

    NEMOWIDGET_CHECK(widget);
    if (widget->container) {
        ERR("Widget(%s) cannot change it's size if it's in the (%s) of (%s)",
                widget->klass->name,
                widget->container->name,
                widget->parent->klass->name);
        return;
    }

    widget->target.width = width;
    widget->target.height = height;

    if (widget->klass->resize) {
        widget->klass->resize(widget, width, height);
    } else {
        NemoWidgetContainer *container;
        INLIST_FOR_EACH(widget->containers, container) {
            if (!container->content) continue;
            nemowidget_resize(container->content,
                    container->sw * widget->target.width,
                    container->sh * widget->target.height);
        }

        struct showone *canvas;
        canvas = nemowidget_get_canvas(widget);
        //nemoshow_canvas_set_size(nemowidget_get_show(widget), canvas, width, height);
        nemoshow_canvas_set_width(canvas, width);
        nemoshow_canvas_set_height(canvas, height);
    }
}

void nemowidget_pivot(NemoWidget *widget, double ax, double ay)
{
    NEMOWIDGET_CHECK(widget);
    struct showone *canvas = nemowidget_get_canvas(widget);
    nemoshow_canvas_pivot(canvas, ax, ay);
}

void nemowidget_translate(NemoWidget *widget, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    NEMOWIDGET_CHECK(widget);
    if (widget->klass->type == NEMOWIDGET_TYPE_WIN) {
        ERR("Window is not supported");
        return;
    }
    if (widget->container) {
        ERR("Widget(%s) cannot be translated if it's in the (%s) of (%s)",
                widget->klass->name,
                widget->container->name,
                widget->parent->klass->name);
        return;
    }

    widget->target.tx = tx;
    widget->target.ty = ty;

    if (widget->klass->translate) {
        widget->klass->translate(widget, easetype, duration, delay, tx, ty);
    } else {
        NemoWidgetContainer *container;
        INLIST_FOR_EACH(widget->containers, container) {
            if (!container->content) continue;
            nemowidget_translate(container->content, easetype, duration, delay,
                    widget->target.tx + container->sx * widget->target.width,
                    widget->target.ty + container->sy * widget->target.height);
        }

        struct showone *canvas;
        canvas = nemowidget_get_canvas(widget);
        if (duration > 0) {
            _nemoshow_item_motion(canvas, easetype, duration, delay,
                    "tx", tx,
                    "ty", ty,
                    NULL);
        } else
            nemoshow_canvas_translate(canvas, tx, ty);
    }
}

void nemowidget_set_alpha(NemoWidget *widget, uint32_t easetype, int duration, int delay, double alpha)
{
    NEMOWIDGET_CHECK(widget);

    widget->target.alpha = alpha;

    if (widget->klass->set_alpha) {
        widget->klass->set_alpha(widget, easetype, duration, delay, alpha);
    } else {
        NemoWidgetContainer *container;
        INLIST_FOR_EACH(widget->containers, container) {
            if (!container->content) continue;
            nemowidget_set_alpha(container->content, easetype, duration, delay,
                    container->content->target.alpha * alpha);
        }

        if (widget->container) {
            alpha = widget->target.alpha;
            NemoWidget *parent = widget->parent;
            while (parent) {
                alpha = alpha * parent->target.alpha;
                parent = parent->parent;
            }
        }

        struct showone *canvas;
        canvas = nemowidget_get_canvas(widget);
        if (duration > 0) {
            _nemoshow_item_motion(canvas, easetype, duration, delay,
                    "alpha", alpha,
                    NULL);
        } else
            nemoshow_canvas_set_alpha(canvas, alpha);
    }
}

void nemowidget_scale(NemoWidget *widget, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    /*
    if (EQUAL(widget->target.sx, sx) && EQUAL(widget->target.sy, sy)) return;
    */

    NEMOWIDGET_CHECK(widget);

    widget->target.sx = sx;
    widget->target.sy = sy;

    if (widget->klass->scale) {
        widget->klass->scale(widget, easetype, duration, delay, sx, sy);
    } else {
        NemoWidgetContainer *container;
        INLIST_FOR_EACH(widget->containers, container) {
            if (!container->content) continue;
            nemowidget_scale(container->content,
                    easetype, duration, delay,
                    container->content->target.sx * sx,
                    container->content->target.sx * sx
                    );
        }

        if (widget->container) {
            sx = widget->target.sx;
            sy = widget->target.sy;
            NemoWidget *parent = widget->parent;
            while (parent) {
                sx = sx * parent->target.sx;
                sy = sy * parent->target.sy;
                parent = parent->parent;
            }
        }

        struct showone *canvas;
        canvas = nemowidget_get_canvas(widget);
        if (duration > 0) {
            _nemoshow_item_motion(canvas, easetype, duration, delay,
                    "sx", sx, "sy", sy,
                    NULL);
        } else
            nemoshow_canvas_scale(canvas, sx, sy);
    }
}

static NemoWidgetCall *_nemowidget_find_call(NemoWidget *widget, const char *id)
{
    List *l;
    NemoWidgetCall *call = NULL;
    LIST_FOR_EACH(widget->calls, l, call) {
        if (call->id && !strcmp(call->id, id)) {
            break;
        }
    }
    return call;
}

static NemoWidgetCallback *_nemowidget_find_callback(NemoWidget *widget, const char *id, NemoWidget_Func func, void *userdata)
{
    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (!call) {
        ERR("There is not matched call id(%s) for the widget(%p)", id, widget);
        return NULL;
    }

    List *l;
    NemoWidgetCallback *callback = NULL;
    LIST_FOR_EACH(call->callbacks, l, callback) {
        if (callback->func == func &&
                callback->userdata == userdata) {
            break;
        }
    }
    return callback;
}

bool nemowidget_callback_dispatch(NemoWidget *widget, const char *id, void *info)
{
    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (!call) return false;

    if (list_count(call->callbacks) == 0)  return false;

    List *l;
    NemoWidgetCallback *callback;
    LIST_FOR_EACH(call->callbacks, l, callback) {
        callback->func(widget, id, info, callback->userdata);
    }
    return true;
}

void nemowidget_append_callback(NemoWidget *widget, const char *id, NemoWidget_Func func, void *userdata)
{
    RET_IF(!id);

    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (!call) {
        ERR("There is not matched call id(%s) for the widget(%p)", id, widget);
        return;
    }
    NemoWidgetCallback *callback;
    callback = _nemowidget_find_callback(widget, id, func, userdata);
    if (callback) {
        ERR("callback(func:%p, userdata:%p) was already registered for the widget(%p)",
                callback->func, callback->userdata, widget);
        return;
    }

    callback = calloc(sizeof(NemoWidgetCallback), 1);
    callback->func = func;
    callback->userdata = userdata;
    call->callbacks = list_append(call->callbacks, callback);
}

void nemowidget_callback_remove(NemoWidget *widget, const char *id, NemoWidget_Func func, void *userdata)
{
    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (!call) {
        ERR("There is not matched call id(%s) for the widget(%p)", id, widget);
        return;
    }
    NemoWidgetCallback *callback;
    callback = _nemowidget_find_callback(widget, id, func, userdata);
    if (!callback) {
        ERR("There is no matche callback(func:%p, userdata:%p) for the widget(%p)",
                func, userdata, widget);
        return;
    }
    call->callbacks = list_remove(call->callbacks, callback);
    free(callback);
}

void nemowidget_call_register(NemoWidget *widget, const char *id)
{
    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (call) {
        ERR("call(id: %s) was already registered for the widget(%p)",
                id, widget);
        return;
    }

    call = calloc(sizeof(NemoWidgetCall), 1);
    call->id = strdup(id);
    widget->calls = list_append(widget->calls, call);
}

void nemowidget_call_unregister(NemoWidget *widget, const char *id)
{
    NemoWidgetCall *call;
    call = _nemowidget_find_call(widget, id);
    if (!call) {
        ERR("There is not matched call id(%s) for the widget(%p)", id, widget);
        return;
    }

    widget->calls = list_remove(widget->calls, call);
    NemoWidgetCallback *callback;
    LIST_FREE(call->callbacks, callback) {
        free(callback);
    }
    free(call->id);
    free(call);
}

static NemoWidgetContainer *_nemowidget_find_container(NemoWidget *widget, const char *name)
{
    NemoWidgetContainer *container = NULL;
    INLIST_FOR_EACH(widget->containers, container) {
        if (container->name && !strcmp(container->name, name)) {
            break;
        }
    }
    return container;
}

void nemowidget_container_register(NemoWidget *widget, const char *name, double sx, double sy, double sw, double sh)
{
    NemoWidgetContainer *container;
    container = _nemowidget_find_container(widget, name);
    if (container) {
        ERR("container(name: %s) was already registered for the widget(%p)",
                name, widget);
        return;
    }

    container = calloc(sizeof(NemoWidgetContainer), 1);
    container->name = strdup(name);
    container->sx = sx;
    container->sy = sy;
    container->sw = sw;
    container->sh = sh;
    widget->containers = inlist_append(widget->containers, INLIST(container));
}

void nemowidget_container_unregister(NemoWidget *widget, const char *name)
{
    NemoWidgetContainer *container;
    container = _nemowidget_find_container(widget, name);
    if (!container) {
        ERR("There is not matched container name(%s) for the widget(%p)", name, widget);
        return;
    }

    widget->containers = inlist_remove(widget->containers, INLIST(container));
    free(container->name);
    free(container);
}

void nemowidget_set_content(NemoWidget *widget, const char *name, NemoWidget *content)
{
    NEMOWIDGET_CHECK(widget);
    NEMOWIDGET_CHECK(content);

    if (widget != content->parent) {
        ERR("widget(%p, %s) is diferrent from content(%s)'s parent(%p,%s)",
                widget, widget->klass->name, content->klass->name,
                content->parent, content->parent->klass->name);
        return;
    }
    NemoWidgetContainer *container;
    container = _nemowidget_find_container(widget, name);
    if (!container) {
        ERR("There is not matched container name(%s) for the widget(%p)", name, widget);
        return;
    }

    content->container = container;
    if (container->content) {
        nemowidget_destroy(container->content);
        ERR("Previous content(%p(%s)) removed",
                container->content, container->content->klass->name);
    }
    container->content = content;

    nemowidget_translate(content, 0, 0, 0,
            widget->target.tx + container->sx * widget->target.width,
            widget->target.ty + container->sy * widget->target.height);
    nemowidget_resize(content,
            container->sw * widget->target.width,
            container->sh * widget->target.height);
    nemowidget_set_alpha(content, 0, 0, 0,
            widget->target.alpha * content->target.alpha);

}

void nemowidget_enable_event(NemoWidget *widget, bool enable)
{
    NEMOWIDGET_CHECK(widget);

    enable = !!enable;
    widget->event_enable = enable;

    NemoWidgetContainer *container;
    INLIST_FOR_EACH(widget->containers, container) {
        if (!container->content) continue;
        nemowidget_enable_event(container->content, enable);
    }
}

void nemowidget_enable_event_repeat(NemoWidget *widget, bool enable)
{
    NEMOWIDGET_CHECK(widget);

    enable = !!enable;
    widget->event_repeat_enable = enable;
}

void nemowidget_enable_event_grab(NemoWidget *widget, bool enable)
{
    NEMOWIDGET_CHECK(widget);

    enable = !!enable;
    widget->event_grab_enable = enable;
}

void nemowidget_stack_above(NemoWidget *widget, NemoWidget *above)
{
    NEMOWIDGET_CHECK(widget);

    if (!above)
        nemoshow_one_above(widget->canvas, NULL);
    else {
        nemoshow_one_above(widget->canvas, above->canvas);
    }
}

void nemowidget_stack_below(NemoWidget *widget, NemoWidget *below)
{
    NEMOWIDGET_CHECK(widget);

    if (!below)
        nemoshow_one_below(widget->canvas, NULL);
    else
        nemoshow_one_below(widget->canvas, below->canvas);
}

void nemowidget_put_scope(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    struct nemoshow *show = nemowidget_get_show(widget);
    nemoshow_view_put_scope(show);
}

void nemowidget_set_scope(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);
    if (!widget->scope) return;

    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *canvas = nemowidget_get_canvas(widget);

    float xx0, yy0, xx1, yy1;
    float x0, y0, x1, y1;

    if (widget->scope->type == NEMOWIDGET_SCOPE_TYPE_RECT) {
        x0 = widget->scope->params[0];
        y0 = widget->scope->params[1];
        x1 = widget->scope->params[0] + widget->scope->params[2];
        y1 = widget->scope->params[1] + widget->scope->params[3];
        if (canvas) {
            nemoshow_canvas_transform_to_global(canvas, x0, y0, &xx0, &yy0);
            nemoshow_canvas_transform_to_global(canvas, x1, y1, &xx1, &yy1);
        } else {
            xx0 = x0;
            yy0 = y0;
            xx1 = x1;
            yy1 = y1;
        }
        nemoshow_transform_to_viewport(show, xx0, yy0, &x0, &y0);
        nemoshow_transform_to_viewport(show, xx1, yy1, &x1, &y1);

        float x, y, w, h;
        x = x0;
        y = y0;
        w = x1 - x0;
        h = y1 - y0;
        nemoshow_view_set_scope(show, "r:%f,%f,%f,%f", x, y, w, h);
    } else if (widget->scope->type == NEMOWIDGET_SCOPE_TYPE_CIRCLE) {
        x0 = widget->scope->params[0] - widget->scope->params[2];
        y0 = widget->scope->params[1] - widget->scope->params[2];
        x1 = widget->scope->params[0] + widget->scope->params[2];
        y1 = widget->scope->params[1] + widget->scope->params[2];
        if (canvas) {
            nemoshow_canvas_transform_to_global(canvas, x0, y0, &xx0, &yy0);
            nemoshow_canvas_transform_to_global(canvas, x1, y1, &xx1, &yy1);
        } else {
            xx0 = x0;
            yy0 = y0;
            xx1 = x1;
            yy1 = y1;
        }
        nemoshow_transform_to_viewport(show, xx0, yy0, &x0, &y0);
        nemoshow_transform_to_viewport(show, xx1, yy1, &x1, &y1);

        float x, y, r;
        x = (x0 + x1)/2;
        y = (y0 + y1)/2;
        r = fabsf(x0 - x1)/2;

        nemoshow_view_set_scope(show, "c:%f,%f,%f", x, y, r);
    } else if (widget->scope->type == NEMOWIDGET_SCOPE_TYPE_ELLIPSE) {
        x0 = widget->scope->params[0] - widget->scope->params[2];
        y0 = widget->scope->params[1] - widget->scope->params[2];
        x1 = widget->scope->params[0] + widget->scope->params[2];
        y1 = widget->scope->params[1] + widget->scope->params[2];
        if (canvas) {
            nemoshow_canvas_transform_to_global(canvas, x0, y0, &xx0, &yy0);
            nemoshow_canvas_transform_to_global(canvas, x1, y1, &xx1, &yy1);
        } else {
            xx0 = x0;
            yy0 = y0;
            xx1 = x1;
            yy1 = y1;
        }
        nemoshow_transform_to_viewport(show, xx0, yy0, &x0, &y0);
        nemoshow_transform_to_viewport(show, xx1, yy1, &x1, &y1);

        float x, y, rx, ry;
        x = (x0 + x1)/2;
        y = (y0 + y1)/2;
        rx = fabsf(x0 - x1)/2;
        ry = fabsf(y0 - y1)/2;
        nemoshow_view_set_scope(show, "e:%f,%f,%f,%f", x, y, rx, ry);
    } else if (widget->scope->type == NEMOWIDGET_SCOPE_TYPE_POLYGON) {
        int i = 0;
        size_t size = 2;
        char *cmd = realloc(NULL, sizeof(char) * size);
        snprintf(cmd, size, "p");
        for (i = 0 ; i < widget->scope->params_num ; i+=2) {
            x0 = widget->scope->params[i];
            y0 = widget->scope->params[i+1];
            if (canvas) {
                nemoshow_canvas_transform_to_global(canvas, x0, y0, &xx0, &yy0);
            } else {
                xx0 = x0;
                yy0 = y0;
            }
            nemoshow_transform_to_viewport(show, xx0, yy0, &x0, &y0);
            char *tmp = strdup_printf(":%f,%f", x0, y0);
            size += strlen(tmp);
            cmd = realloc(cmd, sizeof(char) * size);
            strcat(cmd, tmp);
            free(tmp);
        }
        nemoshow_view_set_scope(show, cmd);
        free(cmd);
    }
}

void nemowidget_detach_scope(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);
    if (!widget->scope) return;

    nemowidget_put_scope(widget);
    free(widget->scope->params);
    free(widget->scope);
}

void nemowidget_attach_scope_rect(NemoWidget *widget, int x, int y, int w, int h)
{
    NEMOWIDGET_CHECK(widget);

    nemowidget_detach_scope(widget);

    widget->scope = calloc(sizeof(NemoWidgetScope), 1);
    widget->scope->type = NEMOWIDGET_SCOPE_TYPE_RECT;
    widget->scope->params_num = 4;
    widget->scope->params = calloc(sizeof(int), 4);
    widget->scope->params[0] = x;
    widget->scope->params[1] = y;
    widget->scope->params[2] = w;
    widget->scope->params[3] = h;

    nemowidget_set_scope(widget);
}

void nemowidget_attach_scope_circle(NemoWidget *widget, int x0, int y0, int r)
{
    NEMOWIDGET_CHECK(widget);

    nemowidget_detach_scope(widget);

    widget->scope = calloc(sizeof(NemoWidgetScope), 1);
    widget->scope->type = NEMOWIDGET_SCOPE_TYPE_CIRCLE;
    widget->scope->params_num = 3;
    widget->scope->params = calloc(sizeof(float), 3);
    widget->scope->params[0] = x0;
    widget->scope->params[1] = y0;
    widget->scope->params[2] = r;

    nemowidget_set_scope(widget);
}

void nemowidget_attach_scope_ellipse(NemoWidget *widget, int x, int y, int rx, int ry)
{
    NEMOWIDGET_CHECK(widget);

    nemowidget_detach_scope(widget);

    widget->scope = calloc(sizeof(NemoWidgetScope), 1);
    widget->scope->type = NEMOWIDGET_SCOPE_TYPE_ELLIPSE;
    widget->scope->params_num = 4;
    widget->scope->params = calloc(sizeof(float), 4);
    widget->scope->params[0] = x;
    widget->scope->params[1] = y;
    widget->scope->params[2] = rx;
    widget->scope->params[3] = ry;

    nemowidget_set_scope(widget);
}

void nemowidget_attach_scope_polygon(NemoWidget *widget, int cnt, int *vals)
{
    NEMOWIDGET_CHECK(widget);
    RET_IF(cnt <= 0);

    //nemowidget_detach_scope(widget);

    widget->scope = calloc(sizeof(NemoWidgetScope), 1);
    widget->scope->type = NEMOWIDGET_SCOPE_TYPE_POLYGON;
    widget->scope->params_num = cnt;
    widget->scope->params = malloc(sizeof(float) * cnt);

    int i;
    for (i = 0 ; i < cnt ; i++) {
        widget->scope->params[i] = vals[i];
    }

    nemowidget_set_scope(widget);
}

void nemowidget_put_region(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    struct nemoshow *show = nemowidget_get_show(widget);
    nemoshow_view_put_region(show);
}

void nemowidget_set_region(NemoWidget *widget, float x0, float y0, float x1, float y1)
{
    NEMOWIDGET_CHECK(widget);

    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *canvas = nemowidget_get_canvas(widget);

#if 0
    static struct showone *rect;
    if (rect) nemoshow_one_destroy(rect);
    rect = RECT_CREATE(canvas, x1 - x0, y1 - y0);
    nemoshow_item_set_fill_color(rect, RGBA(0xFF000050));
    nemoshow_item_translate(rect, x0, y0);
#endif

    float xx0, yy0, xx1, yy1;
    if (canvas) {
        nemoshow_canvas_transform_to_global(canvas, x0, y0, &xx0, &yy0);
        nemoshow_canvas_transform_to_global(canvas, x1, y1, &xx1, &yy1);
    } else {
        xx0 = x0;
        yy0 = y0;
        xx1 = x1;
        yy1 = y1;
    }
    nemoshow_transform_to_viewport(show, xx0, yy0, &x0, &y0);
    nemoshow_transform_to_viewport(show, xx1, yy1, &x1, &y1);

    nemoshow_view_set_region(show, x0, y0, x1 - x0, y1 - y0);
    //ERR("%f %f %f %f (%f, %f)", x0, y0, x1, y1, x1 - x0, y1 - y0);
}

void nemowidget_detach_region(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget);

    if (widget->region) {
        free(widget->region);
    }
}

void nemowidget_attach_region(NemoWidget *widget, float x0, float y0, float x1, float y1)
{
    NEMOWIDGET_CHECK(widget);

    nemowidget_detach_region(widget);
    widget->region = calloc(sizeof(NemoWidgetRegion), 1);
    widget->region->x0 = x0;
    widget->region->y0 = y0;
    widget->region->x1 = x1;
    widget->region->y1 = y1;
    nemowidget_set_region(widget,
            widget->region->x0, widget->region->y0,
            widget->region->x1, widget->region->y1);
}

// ************************************** //
// NemoWidget Item //
// ************************************** //
void nemowidget_item_set_data(NemoWidgetItem *it, void *data)
{
    NEMOWIDGET_ITEM_CHECK(it);
    it->data = data;
}

void *nemowidget_item_get_data(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it, NULL);
    return it->data;
}

void nemowidget_item_set_id(NemoWidgetItem *it, uint32_t id)
{
    NEMOWIDGET_ITEM_CHECK(it);
    it->id = id;
}

uint32_t nemowidget_item_get_id(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it, 0);
    return it->id;
}

void nemowidget_item_set_tag(NemoWidgetItem *it, const char *tag)
{
    NEMOWIDGET_ITEM_CHECK(it);
    if (it->tag) free(it->tag);
    it->tag = strdup(tag);
}

const char *nemowidget_item_get_tag(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it, NULL);
    return it->tag;
}

List *nemowidget_get_items(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    NemoWidgetItem *it;
    List *items = NULL;
    INLIST_FOR_EACH(widget->items, it) {
        items = list_append(items, it);
    }

    return items;
}

NemoWidget *nemowidget_item_get_widget(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it, NULL);
    return it->widget;
}

void *nemowidget_item_get_context(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it, NULL);
    return it->context;
}

void nemowidget_item_get_geometry(NemoWidgetItem *it, int *x, int *y, int *w, int *h)
{
    NEMOWIDGET_ITEM_CHECK(it);
    NemoWidget *widget = nemowidget_item_get_widget(it);

    if (widget->klass->item_get_geometry) {
        widget->klass->item_get_geometry(it, x, y, w, h);
    } else {
        ERR("Widget (%p, %s) does not support item get geometry", widget, widget->klass->name);
    }
}

NemoWidgetItem *nemowidget_append_item(NemoWidget *widget)
{
    NEMOWIDGET_CHECK(widget, NULL);
    RET_IF(!widget->klass->item_append, NULL);

    NemoWidgetItem *it = calloc(sizeof(NemoWidgetItem), 1);
    it->widget = widget;
    widget->items = inlist_append(widget->items, INLIST(it));
    it->context = widget->klass->item_append(widget);

    return it;
}

void nemowidget_item_remove(NemoWidgetItem *it)
{
    NEMOWIDGET_ITEM_CHECK(it);
    RET_IF(!it->widget->klass->item_remove);

    it->widget->klass->item_remove(it);
    it->widget->items = inlist_remove(it->widget->items, INLIST(it));
    if (it->tag) free(it->tag);
    free(it);

    return;
}

// ************************************** //
// NemoWidget Window //
// ************************************** //
typedef struct _WinContext WinContext;
struct _WinContext
{
    int width, height;
    struct nemotimer *exit_timer;

    struct showone *scene;
    struct showone *back;

    int enable_move;
    int enable_rotate;
    int enable_scale;
    double org_ratio;
    bool is_fullscreen;

    struct {
        NemoWidget_Win_Area func;
        void *userdata;
    } area;
};

extern void nemowidget_transform_from_global(NemoWidget *widget, double gx, double gy, double *x, double *y)
{
    struct showone *canvas = nemowidget_get_canvas(widget);
    float xx, yy;
    nemoshow_canvas_transform_from_global(canvas, gx, gy, &xx, &yy);
    if (x) *x = xx;
    if (y) *y = yy;
}

extern void nemowidget_transform_to_global(NemoWidget *widget, double x, double y, double *gx, double *gy)
{
    struct showone *canvas = nemowidget_get_canvas(widget);
    float xx, yy;
    nemoshow_canvas_transform_to_global(canvas, x, y, &xx, &yy);
    if (gx) *gx = xx;
    if (gy) *gy = yy;
}

extern void nemowidget_transform_to_window(NemoWidget *widget, double x, double y, double *wx, double *wy)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double xx, yy;
    float xx0, yy0;
    nemowidget_transform_to_global(widget, x, y, &xx, &yy);
    nemoshow_transform_to_viewport(show, xx, yy, &xx0, &yy0);
    if (wx) *wx = xx0;
    if (wy) *wy = yy0;
}

typedef struct _NemoWidgetGrabData NemoWidgetGrabData;
struct _NemoWidgetGrabData {
    char *key;
    void *data;
};

// User grabs
struct _NemoWidgetGrab
{
    NemoWidget *widget;
    NemoWidgetGrab_Callback callback;
    void *userdata;
    List *data;
};

static void _nemowidget_usergrab_callback(NemoGrab *_grab, struct nemoshow *show, struct showevent *event, void *userdata)
{
    NemoWidgetGrab *grab = userdata;
    grab->callback(grab, grab->widget, event, grab->userdata);
}

void _nemowidget_grab_destroy(void *data)
{
    NemoWidgetGrab *grab = data;
    NemoWidgetGrabData *grabdata;
    LIST_FREE(grab->data, grabdata) {
        free(grabdata->key);
        free(grabdata);
    }
    free(grab);
}

NemoWidgetGrab *nemowidget_create_grab(NemoWidget *widget, struct showevent *event, NemoWidgetGrab_Callback callback, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_down(show, event)) {
        NemoWidgetGrab *grab = calloc(sizeof(NemoWidgetGrab), 1);
        grab->widget = widget;
        grab->callback = callback;
        grab->userdata = userdata;
        widget->usergrabs = nemograbs_append(widget->usergrabs, show, event,
                _nemowidget_usergrab_callback, grab, _nemowidget_grab_destroy, grab);
        return grab;
    }
    return NULL;
}

void nemowidget_grab_set_data(NemoWidgetGrab *grab, const char *key, void *data)
{
    RET_IF(!key);
    NemoWidgetGrabData *grabdata = malloc(sizeof(NemoWidgetGrabData));
    grabdata->key = strdup(key);
    grabdata->data = data;
    grab->data = list_append(grab->data, grabdata);
}

void *nemowidget_grab_get_data(NemoWidgetGrab *grab, const char *key)
{
    RET_IF(!key, NULL);
    List *l;
    NemoWidgetGrabData *grabdata;
    LIST_FOR_EACH(grab->data, l, grabdata) {
        if (!strcmp(grabdata->key, key)) {
            return grabdata->data;
        }
    }
    return NULL;
}

void *nemowidget_grab_get_userdata(NemoWidgetGrab *grab)
{
    return grab->userdata;
}

// Widget grab
static void _nemowidget_dispatch_event(NemoWidget *widget, struct showevent *event)
{
    nemowidget_callback_dispatch(widget, "event", event);
    if (widget->klass->event) {
        widget->klass->event(widget, event);
    }
}

static void _nemowidget_grab_callback(NemoGrab *grab, struct nemoshow *show, struct showevent *event, void *userdata)
{
    NemoWidget *widget = userdata;
    _nemowidget_dispatch_event(widget, event);
}

static void _nemowidget_dispatch_grabs(NemoWidget *widget, void *event)
{
    struct nemoshow *show = nemowidget_get_show(widget);

    widget->grabs = nemograbs_dispatch(widget->grabs, show, event);
    widget->usergrabs = nemograbs_dispatch(widget->usergrabs, show, event);

    NemoWidget *child;
    INLIST_FOR_EACH(widget->children, child) {
        _nemowidget_dispatch_grabs(child, event);
    }
}

static void _nemowidget_create_grab(NemoWidget *widget, struct showevent *event)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    if (nemoshow_event_is_down(show, event)) {
        widget->grabs = nemograbs_append(widget->grabs, show, event,
                _nemowidget_grab_callback, widget, NULL, NULL);

        widget->grabs = nemograbs_dispatch(widget->grabs, show, event);
        widget->usergrabs = nemograbs_dispatch(widget->usergrabs, show, event);
    }
}

static void _nemowidget_win_dispatch_show_event(struct nemoshow *show, struct showevent *event)
{
    NemoWidget *win  = nemoshow_get_userdata(show);

    nemoshow_event_update_taps(show, NULL, event);

    _nemowidget_dispatch_grabs(win, event);

    uint64_t device = nemoshow_event_get_device(event);

    float x, y;
    x = nemoshow_event_get_x(event);
    y = nemoshow_event_get_y(event);
    struct showone *one;
    struct showone *scene = nemowidget_get_scene(win);
    float sx, sy;

    if (win->event_enable) {
        nemoshow_children_for_each_reverse(one, scene) {
            NemoWidget *widget = nemoshow_one_get_userdata(one);
            if (!widget) continue;
            if (widget->klass->type == NEMOWIDGET_TYPE_WIN) break;
            if (!widget->is_show) continue;
            if (nemoshow_event_is_keyboard_enter(show, event) ||
                    nemoshow_event_is_keyboard_leave(show, event) ||
                    nemoshow_event_is_keyboard_down(show, event) ||
                    nemoshow_event_is_keyboard_up(show, event)) {
                // FIXME: keyboard events are repeated to all widgets.
                // There is not focus yet.
                if (widget->event_enable) {
                    _nemowidget_dispatch_event(widget, event);
                }
            } else {
                if (nemoshow_contain_canvas(show, one, x, y, &sx, &sy)) {
                    if (widget->event_enable) {
                        if (widget->event_grab_enable) {
                            _nemowidget_create_grab(widget, event);
                        } else {
                            _nemowidget_dispatch_event(widget, event);
                        }
                    }
                    if (!widget->event_repeat_enable) break;
                }
            }
        }
    }

    WinContext *ctx = nemowidget_get_context(win);
    if (!ctx->area.func ||
            ctx->area.func(win, x, y, ctx->area.userdata)) {
        if (win->event_enable)
            nemowidget_callback_dispatch(win, "event", event);
        if (win->klass->event) {
            win->klass->event(win, event);
        }
    } else {
        if (nemoshow_event_is_down(show, event)) {
            struct nemotool *tool = NEMOSHOW_AT(show, tool);
            float vx, vy;
            nemoshow_transform_to_viewport(show, x, y, &vx, &vy);
            nemotool_touch_bypass(tool, device, vx, vy);
        }
    }
}

static int _nemowidget_win_dispatch_show_destroy(struct nemoshow *show)
{
    NemoWidget *win = nemoshow_get_userdata(show);
    struct nemotool *tool = nemowidget_get_tool(win);

    if (!nemowidget_callback_dispatch(win, "exit", NULL)) {
        nemotool_exit(tool);
        return 1;
    }
    return 0;
}

static void _nemowidget_win_dispatch_show_layer(struct nemoshow *show, int32_t ontop)
{
    NemoWidget *win = nemoshow_get_userdata(show);
    nemowidget_callback_dispatch(win, "layer", (void *)(intptr_t)ontop);
}

static void _nemowidget_win_dispatch_fullscren(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    NemoWidget *win = nemoshow_get_userdata(show);
    WinContext *ctx = nemowidget_get_context(win);

    NemoWidgetInfo_Fullscreen fullscreen;
    if (id) fullscreen.id =  strdup(id);
    else fullscreen.id = NULL;
    fullscreen.x = x;
    fullscreen.y = y;
    fullscreen.width = width;
    fullscreen.height = height;
    if (fullscreen.id) {
        ctx->is_fullscreen = true;
        struct nemoshow *show = nemowidget_get_show(win);
        ctx->org_ratio = (double)show->width/show->height;
        nemowidget_callback_dispatch(win, "fullscreen", &fullscreen);
        free(fullscreen.id);
    } else {
        ctx->is_fullscreen = false;

        // Revoke original ratio when fullscreen mode --> normal
        double org_ratio, new_ratio;
        org_ratio = ctx->org_ratio;
        new_ratio = (double)width/height;

        double ratio = org_ratio/new_ratio;
        if (ratio > 1.0) {
            ratio = 1.0/ratio;
            nemowidget_resize(win, width, height * ratio);
        } else {
            nemowidget_resize(win, width * ratio, height);
        }
        nemowidget_callback_dispatch(win, "fullscreen", &fullscreen);
    }
}

static void _nemowidget_win_dispatch_resize(struct nemoshow *show, int width, int height)
{
    NemoWidget *win = nemoshow_get_userdata(show);
    WinContext *ctx = nemowidget_get_context(win);

    double org_ratio, new_ratio;
    if (!ctx->is_fullscreen) {
        // XXX: if ratio is differnt, do not resize if it is not fullscreen mode
        // because up callback can be occured before resize
        org_ratio = (double)show->width/show->height;
        new_ratio = (double)width/height;
#define R_EPSILON 0.1
#define R_EQUAL(a, b) (((a) > (b)) ? (((a) - (b)) < R_EPSILON) : (((b) - (a)) < R_EPSILON))

        if (!R_EQUAL(org_ratio, new_ratio)) return;
    }
    nemoshow_view_resize(show, width, height);
    nemoshow_view_redraw(show);
}

void _nemowidget_win_destroy(NemoWidget *win)
{
    WinContext *ctx = nemowidget_get_context(win);
    if (ctx->exit_timer) nemotimer_destroy(ctx->exit_timer);
    free(ctx);
}

static void _nemowidget_win_event(NemoWidget *win, struct showevent *event)
{
    WinContext *ctx = nemowidget_get_context(win);
    struct nemoshow *show = nemowidget_get_show(win);

    if (ctx->exit_timer) return;

    if (nemoshow_event_is_down(show, event) ||
            nemoshow_event_is_up(show, event)) {
        if (nemoshow_event_is_single_tap(show, event)) {
            if (ctx->enable_move == 1) {
                nemoshow_view_move(show,
                        nemoshow_event_get_serial_on(event, 0));
            }
        } else if (nemoshow_event_is_many_taps(show, event)) {
            uint32_t picktype = 0;
            if ((ctx->enable_rotate > 0) &&
                    (nemoshow_event_get_tapcount(event) >= ctx->enable_rotate)) {
                picktype |= NEMOSHOW_VIEW_PICK_ROTATE_TYPE;
            }
            if ((ctx->enable_scale > 0) &&
                    (nemoshow_event_get_tapcount(event) >= ctx->enable_scale)) {
                picktype |= NEMOSHOW_VIEW_PICK_SCALE_TYPE;
            }
            if ((ctx->enable_move > 0) &&
                    (nemoshow_event_get_tapcount(event) >= ctx->enable_move)) {
                picktype |= NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE;
            }
            uint32_t serial0, serial1;
            nemoshow_event_get_distant_tapserials(show, event,
                    &serial0, &serial1);
            if (picktype != 0)
                nemoshow_view_pick(show, serial0, serial1, picktype);
        }
    }
}

static void _nemowidget_win_show(NemoWidget *win, uint32_t easetype, int duration, int delay)
{
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_render_one(show);
}

static void _nemowidget_win_resize(NemoWidget *win, int width, int height)
{
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_resize(show, width, height);
}

NemoWidgetClass NEMOWIDGET_WIN = {"Win", NEMOWIDGET_TYPE_WIN, NULL, _nemowidget_win_destroy, NULL, _nemowidget_win_event, NULL, _nemowidget_win_show, NULL, _nemowidget_win_resize, NULL};

NemoWidget *nemowidget_create_win(struct nemotool *tool, const char *name, int width, int height)
{
    RET_IF(width <= 0 || height <= 0, NULL);

    NemoWidget *win;
    win = calloc(sizeof(NemoWidget), 1);
    win->klass = &NEMOWIDGET_WIN;

    win->target.width = width;
    win->target.height = height;
    win->target.sx = 1.0;
    win->target.sy = 1.0;
    win->event_enable = true;
    win->event_grab_enable = true;
    nemowidget_call_register(win, "event");
    nemowidget_call_register(win, "resize");
    nemowidget_call_register(win, "frame");

    nemowidget_call_register(win, "exit");
    nemowidget_call_register(win, "layer");
    nemowidget_call_register(win, "fullscreen");

    struct nemoshow *show;
    show = nemoshow_create_view(tool, width, height);
    nemoshow_set_name(show, name);
    nemoshow_view_set_anchor(show, -0.5, -0.5);
    nemoshow_view_set_state(show, "layer");
    nemoshow_view_put_state(show, "keypad");
    nemoshow_view_put_state(show, "sound");
    nemoshow_set_state(show, NEMOSHOW_ONTIME_STATE);
    nemoshow_set_userdata(show, win);

    nemoshow_set_dispatch_resize(show, _nemowidget_win_dispatch_resize);
    nemoshow_set_dispatch_destroy(show, _nemowidget_win_dispatch_show_destroy);
    nemoshow_set_dispatch_event(show, _nemowidget_win_dispatch_show_event);
    nemoshow_set_dispatch_layer(show, _nemowidget_win_dispatch_show_layer);
    nemoshow_set_dispatch_fullscreen(show, _nemowidget_win_dispatch_fullscren);

    _nemowidget_win_core_init(win);
    win->core->tool = tool;
    win->core->tale = NEMOSHOW_AT(show, tale);
    win->core->show = show;
    win->core->uuid = uuid_gen();
    nemoshow_view_set_uuid(show, win->core->uuid);

    WinContext *ctx = calloc(sizeof(WinContext), 1);
    ctx->enable_move = 1;
    ctx->enable_scale = 2;
    ctx->enable_rotate = 2;
    win->context = ctx;

    return win;
}

NemoWidget *nemowidget_create_win_base(struct nemotool *tool, const char *name, Config *base)
{
    NemoWidget *win;

    win = nemowidget_create_win(tool, name, base->width, base->height);
    RET_IF(!win, NULL);

    if (base->layer) {
        nemowidget_win_set_layer(win, base->layer);
        if (!strcmp(base->layer, "background")) {
            nemowidget_win_set_anchor(win, 0.0, 0.0);
            nemowidget_win_enable_move(win, 0);
            nemowidget_win_enable_rotate(win, 0);
            nemowidget_win_enable_scale(win, 0);
        }
    }

    if (base->min_width > 0 && base->min_height > 0) {
        nemowidget_win_set_min_size(win, base->min_width, base->min_height);
    }
    if (base->max_width > 0 && base->max_height > 0) {
        nemowidget_win_set_max_size(win, base->max_width, base->max_height);
    }
    if (base->scene_width > 0 && base->scene_height > 0) {
        nemowidget_win_load_scene(win, base->scene_width, base->scene_height);
    } else {
        nemowidget_win_load_scene(win, base->width, base->height);
    }
    if (base->framerate > 0) {
        nemowidget_set_framerate(win, base->framerate);
    }

    return win;
}

void nemowidget_win_set_anchor(NemoWidget *win, double ax, double ay)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemoshow *show = nemowidget_get_show(win);
	nemoshow_view_set_anchor(show, ax, ay);
}

void nemowidget_win_set_max_size(NemoWidget *win, double width, double height)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_set_max_size(show, width, height);
}

void nemowidget_win_set_min_size(NemoWidget *win, double width, double height)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_set_min_size(show, width, height);
}

void nemowidget_win_set_fullscreen(NemoWidget *win)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_set_fullscreen(show, "fullscreen0");
}

void nemowidget_win_set_layer(NemoWidget *win, const char *layer)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    RET_IF(!layer);
    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_set_layer(show, layer);
    if (!strcmp(layer, "background"))
        nemoshow_view_set_opaque(show, 0, 0, show->width, show->height);
}

void nemowidget_win_set_event_area(NemoWidget *win, NemoWidget_Win_Area func, void *userdata)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    WinContext *ctx = nemowidget_get_context(win);
    ctx->area.func = func;
    ctx->area.userdata = userdata;
}

void nemowidget_win_enable_move(NemoWidget *win, int tapcount)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    WinContext *ctx = nemowidget_get_context(win);
    ctx->enable_move = tapcount;
}

void nemowidget_win_enable_rotate(NemoWidget *win, int tapcount)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    WinContext *ctx = nemowidget_get_context(win);
    ctx->enable_rotate = tapcount;
}

void nemowidget_win_enable_scale(NemoWidget *win, int tapcount)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    WinContext *ctx = nemowidget_get_context(win);
    ctx->enable_scale = tapcount;
}

void nemowidget_win_enable_fullscreen(NemoWidget *win, bool enable)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemoshow *show = nemowidget_get_show(win);

    if (enable) {
        nemoshow_view_set_fullscreen_type(show, "pick");
        nemoshow_view_set_fullscreen_type(show, "pitch");
    } else {
        nemoshow_view_put_fullscreen_type(show, "pick");
        nemoshow_view_put_fullscreen_type(show, "pitch");
    }
}

void nemowidget_win_set_type(NemoWidget *win, const char *type)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    RET_IF(!type);

    struct nemoshow *show = nemowidget_get_show(win);
    nemoshow_view_set_type(show, type);
}

void nemowidget_win_enable_state(NemoWidget *win, const char *state, bool enable)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    RET_IF(!state);

    struct nemoshow *show = nemowidget_get_show(win);
    if (enable) {
        nemoshow_view_set_state(show, state);
    } else {
        nemoshow_view_put_state(show, state);
    }
}

void nemowidget_win_load_scene(NemoWidget *win, int width, int height)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    WinContext *ctx = nemowidget_get_context(win);

    struct showone *scene;
    struct showone *canvas;
    struct showone *back;
    struct nemoshow *show =  nemowidget_get_show(win);

    ctx->width = width;
    ctx->height = height;

    scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
    nemoshow_set_scene(show, scene);
    ctx->scene = scene;

    back = nemoshow_canvas_create();
	nemoshow_canvas_set_width(back, width);
	nemoshow_canvas_set_height(back, height);
	nemoshow_canvas_set_type(back, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(back, 0.0f, 0.0f, 0.0f, 0.0f);
    nemoshow_one_attach(scene, back);
    ctx->back = back;

    // XXX: void canvas only for redraw/resize event callback
    canvas = nemoshow_canvas_create();
    nemoshow_canvas_set_width(canvas, width);
    nemoshow_canvas_set_height(canvas, height);
    nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
    nemoshow_canvas_pivot(canvas, 0, 0);
    nemoshow_one_set_userdata(canvas, win);
    nemoshow_canvas_set_dispatch_redraw(canvas, _nemowidget_dispatch_canvas_redraw);
    nemoshow_canvas_set_dispatch_resize(canvas, _nemowidget_dispatch_canvas_resize);
    nemoshow_one_attach(scene, canvas);
    nemowidget_set_canvas(win, canvas);
    if (win->klass->area) {
        nemoshow_canvas_set_contain_point(canvas, _nemowidget_contain_point);
    }

    win->core->scene = scene;
}

void nemowidget_win_exit(NemoWidget *win)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    struct nemotool *tool = nemowidget_get_tool(win);
    nemotool_exit(tool);
}

static void _nemowidget_win_exit_after_done(struct nemotimer *timer, void *userdata)
{
    NemoWidget *win = userdata;
    struct nemotool *tool = nemowidget_get_tool(win);
    nemotool_exit(tool);
}

void nemowidget_win_exit_after(NemoWidget *win, int timeout)
{
    nemowidget_enable_event(win, false);
    WinContext *ctx = nemowidget_get_context(win);

    if (ctx->exit_timer) return;

    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(win));
    nemotimer_set_timeout(timer, timeout);
    nemotimer_set_callback(timer, _nemowidget_win_exit_after_done);
    nemotimer_set_userdata(timer, win);
    ctx->exit_timer = timer;
}

static void _nemowidget_win_check_trans(struct nemotimer *timer, void *userdata)
{
    NemoWidget *win = userdata;
    WinContext *ctx = nemowidget_get_context(win);

    if (nemoshow_has_transition(nemowidget_get_show(win))) {
        nemotimer_set_timeout(timer, 150);
    } else {
        nemotimer_destroy(ctx->exit_timer);
        ctx->exit_timer = NULL;
        nemowidget_win_exit(win);
    }
}

void nemowidget_win_exit_if_no_trans(NemoWidget *win)
{
    NEMOWIDGET_CHECK_CLASS(win, &NEMOWIDGET_WIN);
    nemowidget_enable_event(win, false);

    WinContext *ctx = nemowidget_get_context(win);

    if (!nemoshow_has_transition(nemowidget_get_show(win))) {
        nemowidget_win_exit(win);
    }

    if (ctx->exit_timer) nemotimer_destroy(ctx->exit_timer);
    struct nemotimer *timer;
    timer = nemotimer_create(nemowidget_get_tool(win));
    nemotimer_set_timeout(timer, 20);
    nemotimer_set_callback(timer, _nemowidget_win_check_trans);
    nemotimer_set_userdata(timer, win);
    ctx->exit_timer = timer;
}


// ************************************** //
// NemoWidget Vector //
// ************************************** //
typedef struct _VectorContext VectorContext;
struct _VectorContext
{
};

void *_nemowidget_vector_create(NemoWidget *vector, int width, int height)
{
    NEMOWIDGET_CHECK_CLASS(vector, &NEMOWIDGET_VECTOR, NULL);
    VectorContext *ctx = calloc(sizeof(VectorContext), 1);
    return ctx;
}

static void _nemowidget_vector_destroy(NemoWidget *vector)
{
    NEMOWIDGET_CHECK_CLASS(vector, &NEMOWIDGET_VECTOR);
    VectorContext *ctx = nemowidget_get_context(vector);
    free(ctx);
}

NemoWidgetClass NEMOWIDGET_VECTOR = {"vector", NEMOWIDGET_TYPE_VECTOR, _nemowidget_vector_create, _nemowidget_vector_destroy, NULL, NULL, NULL, NULL, NULL};

NemoWidget *nemowidget_create_vector(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_VECTOR, parent, width, height);
}

// ************************************** //
// NemoWidget Opengl //
// ************************************** //
typedef struct _OpenglContext OpenglContext;
struct _OpenglContext
{
};

void *_nemowidget_opengl_create(NemoWidget *opengl, int width, int height)
{
    NEMOWIDGET_CHECK_CLASS(opengl, &NEMOWIDGET_OPENGL, NULL);
    OpenglContext *ctx = calloc(sizeof(OpenglContext), 1);
    return ctx;
}

void _nemowidget_opengl_destroy(NemoWidget *opengl)
{
    NEMOWIDGET_CHECK_CLASS(opengl, &NEMOWIDGET_OPENGL);
    OpenglContext *ctx = nemowidget_get_context(opengl);
    free(ctx);
}

NemoWidgetClass NEMOWIDGET_OPENGL = {"opengl", NEMOWIDGET_TYPE_OPENGL, _nemowidget_opengl_create, _nemowidget_opengl_destroy, NULL, NULL, NULL, NULL, NULL};

NemoWidget *nemowidget_create_opengl(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_OPENGL, parent, width, height);
}

// ************************************** //
// NemoWidget Pixman //
// ************************************** //
typedef struct _PixmanContext PixmanContext;
struct _PixmanContext
{
};

void *_nemowidget_pixman_create(NemoWidget *pixman, int width, int height)
{
    NEMOWIDGET_CHECK_CLASS(pixman, &NEMOWIDGET_PIXMAN, NULL);
    PixmanContext *ctx = calloc(sizeof(PixmanContext), 1);
    return ctx;
}

void _nemowidget_pixman_destroy(NemoWidget *pixman)
{
    NEMOWIDGET_CHECK_CLASS(pixman, &NEMOWIDGET_PIXMAN);
    PixmanContext *ctx = nemowidget_get_context(pixman);
    free(ctx);
}

NemoWidgetClass NEMOWIDGET_PIXMAN = {"pixman", NEMOWIDGET_TYPE_PIXMAN, _nemowidget_pixman_create, _nemowidget_pixman_destroy, NULL, NULL, NULL, NULL, NULL};

NemoWidget *nemowidget_create_pixman(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_PIXMAN, parent, width, height);
}

