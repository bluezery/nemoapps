#ifndef __NEMOHELPER__
#define __NEMOHELPER__

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <uuid/uuid.h>

#include <nemoitem.h>
#include <nemobus.h>
#include <nemotool.h>
#include <nemoshow.h>
#include <nemoegl.h>
#include <nemomisc.h>
#include <nemocanvas.h>
#include <nemoglib.h>

#include <showhelper.h>

#include "nemoutil.h"
#include "pixmanhelper.h"

typedef struct _OneProperty OneProperty;
struct _OneProperty {
    double width, height;
    double tx, ty;
    double sx, sy;
    double ro;
    double alpha;
    uint32_t fill;
    uint32_t stroke;
    double stroke_w;
};

#define ONE_PROPERTY_INIT(property, _w, _h, _tx, _ty, _sx, _sy, _ro, _alpha, _fill, _stroke, _stroke_w) \
    do { \
        (property)->width = _w; \
        (property)->height = _h; \
        (property)->tx = _tx; \
        (property)->ty = _ty; \
        (property)->sx = _sx; \
        (property)->sy = _sy; \
        (property)->ro = _ro; \
        (property)->alpha = _alpha; \
        (property)->fill = _fill; \
        (property)->stroke = _stroke; \
        (property)->stroke_w = _stroke_w; \
    } while (0)

// ***********************************************//
// Nemo List
// ***********************************************//
#define nemolist_node3(head, type, member) \
	(head)->next == NULL ? NULL : (head)->next->next == NULL ? NULL : (head)->next->next->next == NULL ? NULL : (head)->next->next->next->next == NULL ? NULL : nemo_type_of((head)->next->next->next->next, type, member)

#define nemolist_node4(head, type, member) \
	(head)->next == NULL ? NULL : (head)->next->next == NULL ? NULL : (head)->next->next->next == NULL ? NULL : (head)->next->next->next->next == NULL ? NULL : (head)->next->next->next->next->next == NULL ? NULL : nemo_type_of((head)->next->next->next->next->next, type, member)

#define nemolist_node5(head, type, member) \
	(head)->next == NULL ? NULL : (head)->next->next == NULL ? NULL : (head)->next->next->next == NULL ? NULL : (head)->next->next->next->next == NULL ? NULL : (head)->next->next->next->next->next == NULL ? NULL : (head)->next->next->next->next->next == NULL ? NULL : nemo_type_of((head)->next->next->next->next->next->next, type, member)

#define ONE_CHILD0(one) \
    nemolist_node0(&((one)->children_list), struct showone, children_link)
#define ONE_CHILD1(one) \
    nemolist_node1(&((one)->children_list), struct showone, children_link)
#define ONE_CHILD2(one) \
    nemolist_node2(&((one)->children_list), struct showone, children_link)
#define ONE_CHILD3(one) \
    nemolist_node3(&((one)->children_list), struct showone, children_link)
#define ONE_CHILD4(one) \
    nemolist_node4(&((one)->children_list), struct showone, children_link)
#define ONE_CHILD5(one) \
    nemolist_node5(&((one)->children_list), struct showone, children_link)

static inline struct nemolist *nemolist_at(const struct nemolist *list, int idx)
{
	struct nemolist *e = list->next;
	int count;

	for (count = 0; e != list; count++) {
        if (idx == count) break;
		e = e->next;
	}
    return e;
}

static inline void _nemoshow_destroy_transition_all(struct nemoshow *show)
{
	while (nemolist_empty(&show->transition_list) == 0) {
        struct showtransition *trans;
		trans = nemolist_node0(&show->transition_list, struct showtransition, link);
		nemoshow_transition_destroy(trans);
	}
}

// ************************************** //
// TOOL //
// ************************************** //
static inline struct nemotimer *TOOL_ADD_TIMER(struct nemotool *tool, int32_t timeout, nemotimer_dispatch_t callback, void *userdata)
{
    struct nemotimer *timer = nemotimer_create(tool);
    nemotimer_set_timeout(timer, timeout);
    nemotimer_set_callback(timer, callback);
    nemotimer_set_userdata(timer, userdata);
    return timer;
}

static inline struct nemotool *TOOL_CREATE()
{
    struct nemotool *tool;
    tool = nemotool_create();
    if (!tool) {
        ERR("nemotool_create() failed");
        return NULL;
    }
    nemotool_connect_wayland(tool, NULL);
    return tool;
}

static inline void TOOL_DESTROY(struct nemotool *tool)
{
    nemotool_disconnect_wayland(tool);
    nemotool_destroy(tool);
}

static inline void TOOL_RUN_GLIB(struct nemotool *tool)
{
    GMainLoop *loop;
    loop = g_main_loop_new(NULL, FALSE);
	nemoglib_run_tool(loop, tool);
	g_main_loop_unref(loop);
}

pthread_mutex_t __worker_mutex;

static inline void nemowrapper_init()
{
    pthread_mutex_init(&__worker_mutex, NULL);
}

static inline void nemowrapper_shutdown()
{
    pthread_mutex_destroy(&__worker_mutex);
}

typedef void (*EventCallback)(const char *id, void *userdata);

typedef struct _EventListener EventListener;
struct _EventListener {
    Inlist __inlist;
    char *id;
    EventCallback callback;
    void *userdata;
};

typedef struct _Event Event;
struct _Event {
    char *id;
};

static Inlist *event_listeners = NULL;
static inline void _event_dispatch_idle(void *data)
{
    Event *event = (Event *)data;
    EventListener *tmp, *listener;
    INLIST_FOR_EACH_SAFE(event_listeners, tmp, listener) {
        if (!strcmp(listener->id, event->id) ){
            listener->callback(listener->id, listener->userdata);
        }
    }
    free(event->id);
    free(event);
}

static inline void event_call(struct nemotool *tool, const char *id)
{
    Event *event = (Event *)calloc(sizeof(Event), 1);
    event->id = strdup(id);
    if (nemotool_dispatch_idle(tool, _event_dispatch_idle, event) < 0) {
        ERR("nemotool dispatch idle failed");
    }
}

static inline void event_add_listener(const char *id, EventCallback callback, void *userdata)
{
    EventListener *listener;
    INLIST_FOR_EACH(event_listeners, listener) {
        if (!strcmp(listener->id, id) && (listener->callback == callback) &&
                (listener->userdata == userdata)) {
            ERR("There is duplicated listener already");
            return;
        }
    }

    listener = (EventListener *)calloc(sizeof(EventListener), 1);
    listener->id = strdup(id);
    listener->callback = callback;
    listener->userdata = userdata;
    event_listeners = inlist_append(event_listeners, INLIST(listener));
}

static inline void event_remove_listener(const char *id, EventCallback callback, void *userdata)
{
    RET_IF(!id || !callback);
    EventListener *listener;
    INLIST_FOR_EACH(event_listeners, listener) {
        if (!strcmp(listener->id, id) && (listener->callback == callback) &&
                (listener->userdata == userdata)) {
            event_listeners = inlist_remove(event_listeners, INLIST(listener));
            free(listener->id);
            free(listener);
            break;
        }
    }
}

// ************************************** //
// NemoGrab Interface //
// ************************************** //
typedef struct _NemoGrab NemoGrab;
typedef void (*NemoGrab_Callback)(NemoGrab *grab, struct nemoshow *show, struct showevent *event, void *userdata);
typedef void (*NemoGrab_Destroy)(void *data);
struct _NemoGrab {
    struct nemoshow *show;
    uint64_t device;
    NemoGrab_Callback callback;
    void *userdata;
    NemoGrab_Destroy destroy;
    void *destroy_data;
    void *data;
};

static inline NemoGrab *nemograb_create(struct nemoshow *show, struct showevent *event, NemoGrab_Callback callback, void *userdata, NemoGrab_Destroy destroy, void *destroy_data)
{
    NemoGrab *grab = (NemoGrab *)calloc(sizeof(NemoGrab), 1);
    grab->show = show;
    grab->device = nemoshow_event_get_device(event);
    grab->callback = callback;
    grab->userdata = userdata;
    grab->destroy = destroy;
    grab->destroy_data = destroy_data;
    return grab;
}

static inline void nemograb_set_data(NemoGrab *grab, void *data)
{
    grab->data = data;
}

static inline void *nemograb_get_data(NemoGrab *grab)
{
    return grab->data;
}

static inline void nemograb_destroy(NemoGrab *grab)
{
    if (grab->destroy) grab->destroy(grab->destroy_data);
    free(grab);
}

static inline void nemograb_dispatch(NemoGrab *grab, struct showevent *event)
{
    grab->callback(grab, grab->show, event, grab->userdata);
}

static inline List *nemograbs_append(List *grabs, struct nemoshow *show, struct showevent *event, NemoGrab_Callback callback, void *userdata, NemoGrab_Destroy destroy, void *destroy_data)
{
    NemoGrab *grab = nemograb_create(show, event, callback, userdata, destroy, destroy_data);
    grabs = list_append(grabs, grab);
    return grabs;
}

static inline List *nemograbs_remove(List *grabs, NemoGrab *grab)
{
    grabs = list_remove(grabs, grab);
    nemograb_destroy(grab);
    return grabs;
}

static inline List *nemograbs_dispatch(List *grabs, struct nemoshow *show, struct showevent *event)
{
    List *l, *ll;
    NemoGrab *grab;
    LIST_FOR_EACH_SAFE(grabs, l, ll, grab) {
        if (grab->device == nemoshow_event_get_device(event)) {
            nemograb_dispatch(grab, event);
            if (nemoshow_event_is_up(show, event)) {
                grabs = nemograbs_remove(grabs, grab);
            }
        }
    }
    return grabs;
}

static inline void nemograbs_destroy(List *grabs)
{
    NemoGrab *grab;
    LIST_FREE(grabs, grab) {
        nemograb_destroy(grab);
    }
}

/***************************************
* NEMOSHOW ITEM
* **************************************/
static inline struct showone *CANVAS_VECTOR_CREATE(struct showone *scene, int width, int height)
{
    struct showone *canvas;
    canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
    nemoshow_one_attach(scene, canvas);
    return canvas;
}

static inline struct showone *CANVAS_PIXMAN_CREATE(struct showone *scene, int width, int height)
{
    struct showone *canvas;
    canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
    nemoshow_one_attach(scene, canvas);
    return canvas;
}

static inline struct showone *CANVAS_PIXMAN_IMAGE_CREATE(struct showone *scene, const char *uri, int width, int height)
{
    if (!file_get_image_wh(uri, NULL, NULL)) {
        ERR("not image file: %s", uri);
        return NULL;
    }

    struct showone *canvas = CANVAS_PIXMAN_CREATE(scene, width, height);
    if (!canvas) {
        ERR("pixman canvas creation failed: %d, %d", width, height);
        return NULL;
    }
    pixman_image_t *piximg = pixman_load_image(uri, width, height);
    if (!piximg) {
        ERR("pixman load image failed");
        nemoshow_canvas_destroy(canvas);
        return NULL;
    }

    pixman_image_composite32(PIXMAN_OP_SRC,
            piximg,
            NULL,
            nemoshow_canvas_get_pixman(canvas),
            0, 0, 0, 0, 0, 0, width, height);
    return canvas;
}

static inline struct showone *CANVAS_VECTOR_IMAGE_CREATE(struct showone *scene, const char *uri, int width, int height)
{
    if (!file_get_image_wh(uri, NULL, NULL)) {
        ERR("not image file: %s", uri);
        return NULL;
    }

    struct showone *canvas = CANVAS_VECTOR_CREATE(scene, width, height);
    if (!canvas) {
        ERR("vector canvas creation failed: %d, %d", width, height);
        return NULL;
    }

    struct showone *one;
    one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
    nemoshow_one_attach(canvas, one);
    nemoshow_item_set_uri(one, uri);
    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);

    return canvas;
}

static inline struct showone* EASE_CREATE(uint32_t type)
{
    struct showone *ease;
    ease = nemoshow_ease_create();
    nemoshow_ease_set_type(ease, type);
    return ease;
}

static inline struct showone *BLUR_CREATE(const char *style, double r)
{
    struct showone *blur;
    blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
    nemoshow_filter_set_blur(blur, style, r);
    return blur;
}

static inline struct showone *LINEAR_GRADIENT_CREATE(double x0, double y0, double x1, double y1)
{
    struct showone *shader;
    shader = nemoshow_shader_create(NEMOSHOW_LINEAR_GRADIENT_SHADER);
    nemoshow_shader_set_x0(shader, x0);
    nemoshow_shader_set_y0(shader, y0);
    nemoshow_shader_set_x1(shader, x1);
    nemoshow_shader_set_y1(shader, y1);
    return shader;
}

static inline struct showone *RADIAL_GRADIENT_CREATE(double x0, double y0, double r)
{
    struct showone *shader;
    shader = nemoshow_shader_create(NEMOSHOW_RADIAL_GRADIENT_SHADER);
    nemoshow_shader_set_x0(shader, x0);
    nemoshow_shader_set_y0(shader, y0);
    nemoshow_shader_set_r(shader, r);
    return shader;
}

static inline struct showone *STOP_CREATE(struct showone *shader, uint32_t color, double offset)
{
    struct showone *stop;
    stop = nemoshow_stop_create();
    nemoshow_one_attach(shader, stop);
    nemoshow_stop_set_color(stop, RGBA(color));
    nemoshow_stop_set_offset(stop, offset);
    return stop;
}

static inline struct showone *EMBOSS_CREATE(double dx, double dy, double dz, double ambient, double specular, double r)
{
    struct showone *emboss;
    emboss = nemoshow_filter_create(NEMOSHOW_EMBOSS_FILTER);
    nemoshow_filter_set_ambient(emboss, ambient);
    nemoshow_filter_set_specular(emboss, specular);
    nemoshow_filter_set_direction(emboss, dx, dy, dz);
    nemoshow_filter_set_radius(emboss, r);

    return emboss;
}

static inline struct showone *FONT_CREATE(const char *family, const char *style)
{
    struct showone *font;
    font = nemoshow_font_create();
    nemoshow_font_load_fontconfig(font, family, style);
    nemoshow_font_use_harfbuzz(font);
    return font;
}

#define ITEM_CREATE(ONE, PARENT, TYPE) \
    do { \
        ONE = nemoshow_item_create(TYPE); \
        if (PARENT) nemoshow_one_attach(PARENT, ONE); \
    } while (0);


static inline struct showone *GROUP_CREATE(struct showone *parent)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_GROUP_ITEM);

    return one;
}

static inline struct showone *RECT_CREATE(struct showone *parent,
        int width, int height)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_RECT_ITEM);

    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);

    return one;
}

static inline struct showone *RRECT_CREATE(struct showone *parent,
        int width, int height,
        int rx, int ry)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_RRECT_ITEM);

    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);

    nemoshow_item_set_roundx(one, rx);
    nemoshow_item_set_roundy(one, ry);

    return one;
}

static inline struct showone *CIRCLE_CREATE(struct showone *parent, double r)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_CIRCLE_ITEM);

    nemoshow_item_set_r(one, r);
    return one;
}

static inline struct showone *ARC_CREATE(struct showone *parent, int width, int height, double from, double to)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_ARC_ITEM);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);
    nemoshow_item_set_from(one, from);
    nemoshow_item_set_to(one, to);
    return one;
}

static inline struct showone *PATH_CREATE(struct showone *parent)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATH_ITEM);

    return one;
}

static inline struct showone *PATH_GROUP_CREATE(struct showone *parent)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHGROUP_ITEM);

    return one;
}

static inline struct showone *PATH_ARRAY_CREATE(struct showone *parent)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHARRAY_ITEM);

    return one;
}

static inline struct showone *SVG_PATH_GROUP_CREATE(struct showone *parent,
        int width, int height,
        const char *uri)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHGROUP_ITEM);
    nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);

    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);
    if (uri) nemoshow_item_path_load_svg(one, uri, 0, 0, width ,height);

    return one;
}


static inline struct showone *SVG_PATH_CREATE(struct showone *parent,
        int width, int height,
        const char *uri)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHTWICE_ITEM);
    nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);

    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);
    if (uri) nemoshow_item_path_load_svg(one, uri, 0, 0, width ,height);

    return one;
}

static inline struct showone *TEXT_CREATE(struct showone *parent,
        struct showone *font, int fontsize, const char *text)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_TEXT_ITEM);

    nemoshow_item_set_font(one, font);
    nemoshow_item_set_fontsize(one, fontsize);
    if (text) nemoshow_item_set_text(one, text);
    else nemoshow_item_set_text(one, "");

    return one;
}

static inline struct showone *PATH_TEXT_CREATE(struct showone *parent,
        const char *fontfamily, int fontsize, const char *text)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATH_ITEM);

    if (text)
        nemoshow_item_path_text(one, fontfamily, fontsize, text, strlen(text), 0, 0);
    else
        nemoshow_item_path_text(one, fontfamily, fontsize, "", strlen(""), 0, 0);

    return one;
}

static inline struct showone *IMAGE_CREATE(struct showone *parent,
        int width, int height, const char *uri)
{
    if (!file_is_exist(uri)) {
        ERR("file is not existed: %s", uri);
        return NULL;
    }
    if (file_is_null(uri)) {
        ERR("file size is zero: %s", uri);
        return NULL;
    }

    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_IMAGE_ITEM);

    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);
    nemoshow_item_set_uri(one, uri);

    return one;
}

static inline struct showone *LINE_CREATE(struct showone *parent,
        int x0, int y0, int x1, int y1, int width)
{
    struct showone *one;
    double x_diff = x1 - x0;
    double y_diff = y1 - y0;

    int len = sqrt(x_diff * x_diff + y_diff * y_diff);
    double r = atan2(y_diff, x_diff) * (180/M_PI);
    if (r <= 0) r = 360 + r;
    one = RRECT_CREATE(parent, len + 4, width, 3, 3);
    nemoshow_item_rotate(one, r);
    nemoshow_item_translate(one, x0, y0);
    return one;
}

static inline struct showone *PATH_LINE_CREATE(struct showone *parent,
        int x0, int y0, int x1, int y1)
{
    struct showone *one;
    one = PATH_CREATE(parent);
    nemoshow_item_path_moveto(one, x0, y0);
    nemoshow_item_path_lineto(one, x1, y1);

    return one;
}

static inline struct showone *PATH_BEZIER_CREATE(struct showone *parent,
        int x0, int y0, int x1, int y1, int x2, int y2)
{
    char buf[256];
    snprintf(buf, 256, "M %d %d Q%d %d %d %d", x0, y0, x1, y1, x2, y2);
    struct showone *one;
    one = PATH_CREATE(parent);
    nemoshow_item_path_cmd(one, buf);
    return one;
}

static inline double ease_bounce(double t, double d)
{
    double c = 1.0;
    double b = 0.0;
    if ((t/=d) < (1/2.75)) {
        return c * (7.5625 * t * t) + b;
    } else if (t < (2/2.75)) {
        double t0 = t - (1.5/2.75);
        t-=(1.5/2.75);
        return c * (7.5625* (t0) * t + .75) + b;
    } else if (t < (2.5/2.75)) {
        double t0 = t - (2.25/2.75);
        t-=(2.25/2.75);
        return c*(7.5625*(t0) * t + .9375) + b;
    } else {
        double t0 = t - (2.625/2.75);
        t-=(2.625/2.75);
        return c*(7.5625*(t0)*t + .984375) + b;
    }
}

// ***********************************************//
// Nemo Motion
// ***********************************************//
typedef struct _NemoMotion NemoMotion;
typedef void (*NemoMotion_DoneCallback)(NemoMotion *m, void *userdata);
typedef void (*NemoMotion_FrameCallback)(NemoMotion *m, uint32_t time, double t, void *userdata);

struct _NemoMotion
{
    struct nemoshow *show;
    struct showone *ease;

    struct showtransition *trans;
    struct showone *seq;
    List *frames;

    struct {
        NemoMotion_DoneCallback callback;
        void *userdata;
    } done;

    struct {
        NemoMotion_FrameCallback callback;
        void *userdata;
    } frame;
};

static inline void nemomotion_set_done_callback(NemoMotion *m, NemoMotion_DoneCallback callback, void *userdata)
{
    m->done.callback = callback;
    m->done.userdata = userdata;
}

static inline void _nemomotion_run_done(void *userdata)
{
    NemoMotion *m = (NemoMotion *)userdata;

    if (m->done.callback) {
        m->done.callback(m, m->done.userdata);
    }

    nemoshow_one_destroy(m->ease);
    free(m);
}

static inline int _nemomotion_frame(void *userdata, uint32_t time, double t)
{
    NemoMotion *m = (NemoMotion *)userdata;

    if (m->frame.callback) {
        m->frame.callback(m, time, t, m->frame.userdata);
    }
    return 0;
}

static inline void nemomotion_set_frame_callback(NemoMotion *m, NemoMotion_FrameCallback callback, void *userdata)
{
    m->frame.callback = callback;
    m->frame.userdata = userdata;
}

static inline NemoMotion *nemomotion_create(struct nemoshow *show,
        uint32_t easetype,  uint32_t dur, uint32_t delay)
{
    NemoMotion *m = (NemoMotion *)calloc(sizeof(NemoMotion), 1);
    m->show = show;

    struct showone *ease;
    ease = nemoshow_ease_create();
    nemoshow_ease_set_type(ease, easetype);
    m->ease = ease;

    m->trans = nemoshow_transition_create(ease, dur, delay);
    m->seq = nemoshow_sequence_create();

    nemoshow_transition_set_userdata(m->trans, m);
    nemoshow_transition_set_dispatch_done(m->trans, _nemomotion_run_done);
    nemoshow_transition_set_dispatch_frame(m->trans, _nemomotion_frame);

    return m;
}

static inline NemoMotion *nemomotion_create_custom(struct nemoshow *show,
        nemoease_progress_t dispatch, uint32_t dur, uint32_t delay)
{
    NemoMotion *m = (NemoMotion *)calloc(sizeof(NemoMotion), 1);
    m->show = show;

    struct showone *ease;
    ease = nemoshow_ease_create();
    m->ease = ease;

	struct showease *_ease = NEMOSHOW_EASE(ease);
    (&(_ease->ease))->dispatch = dispatch;

    m->trans = nemoshow_transition_create(ease, dur, delay);
    m->seq = nemoshow_sequence_create();

    nemoshow_transition_set_userdata(m->trans, m);
    nemoshow_transition_set_dispatch_done(m->trans, _nemomotion_run_done);
    nemoshow_transition_set_dispatch_frame(m->trans, _nemomotion_frame);

    return m;
}

static inline void nemomotion_run(NemoMotion *m)
{
    nemoshow_transition_attach_sequence(m->trans, m->seq);
    nemoshow_attach_transition(m->show, m->trans);
}

static inline void nemomotion_attach(NemoMotion *m, double t, ...)
{
    bool found = false;
    struct showone *frame;
    struct showone *set;

	nemoshow_children_for_each(frame, m->seq) {
		if (frame->type != NEMOSHOW_FRAME_TYPE) continue;
        if (EQUAL(NEMOSHOW_FRAME(frame)->t, t)) {
            found = true;
            break;
        }
    }

    if (!found) {
        frame = nemoshow_sequence_create_frame();
        nemoshow_sequence_set_timing(frame, t);
        nemoshow_one_attach(m->seq, frame);
    }

    va_list ap;
    va_start(ap, t);

    struct showone *src;
	while ((src = va_arg(ap, struct showone *))) {
        found = false;
        nemoshow_children_for_each(set, frame) {
            if (set->type != NEMOSHOW_SET_TYPE) continue;
            if (NEMOSHOW_SET(set)->src == src) {
                found = true;
                break;
            }
        }
        if (!found) {
            set = nemoshow_sequence_create_set();
            nemoshow_sequence_set_source(set, src);
            nemoshow_one_attach(frame, set);
            nemoshow_transition_check_one(m->trans, src);
        }

        const char *name;
        name = va_arg(ap, const char *);

        if (strstr(name, ":")) {
            double value = va_arg(ap, double);

            char *tmp;
            const char *tok;
            const char *_name;
            int offset;

            tmp = strdup(name);
            tok = strtok(tmp, ":");
            _name = tok;
            tok = strtok(NULL, ":");
            offset = atoi(tok);
            nemoshow_sequence_set_dattr_offset(set, _name, offset, value);
            free(tmp);
        } else if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t value = va_arg(ap, uint32_t);
            nemoshow_sequence_set_cattr(set, name, RGBA(value));
        } else {
            double value = va_arg(ap, double);
            nemoshow_sequence_set_dattr(set, name, value);
        }
    }

    va_end(ap);
}

static inline void nemomotion_do(struct nemoshow *show,
        uint32_t easetype,  uint32_t dur, uint32_t delay, double t, ...)
{
    NemoMotion *m = nemomotion_create(show, easetype, dur, delay);

    struct showone *src;
    const char *name;

    va_list ap;
    va_start(ap, t);
	while ((src = va_arg(ap, struct showone *))) {
        name = va_arg(ap, const char *);

        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, t, src, name, value, NULL);
        } else {
            double value = va_arg(ap, double);
            nemomotion_attach(m, t, src, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

static inline void _nemoshow_item_motion(struct showone *src,
        uint32_t easetype,  uint32_t dur, uint32_t delay, ...)
{
    RET_IF(!src || !src->show);
    NemoMotion *m = nemomotion_create(src->show, easetype, dur, delay);

    const char *name;

    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        } else {
            double value = va_arg(ap, double);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

static inline void _nemoshow_item_motion_with_callback(struct showone *src,
        NemoMotion_FrameCallback callback, void *userdata,
        uint32_t easetype,  uint32_t dur, uint32_t delay, ...)
{
    RET_IF(!src || !src->show);
    NemoMotion *m = nemomotion_create(src->show, easetype, dur, delay);
    nemomotion_set_frame_callback(m, callback, userdata);

    const char *name;

    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        } else {
            double value = va_arg(ap, double);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

static inline void _nemoshow_item_motion_bounce(struct showone *src,
        uint32_t easetype,  uint32_t dur, uint32_t delay, ...)
{
    RET_IF(!src || !src->show);
    NemoMotion *m = nemomotion_create(src->show, easetype, dur, delay);

    const char *name;

    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t bounce = va_arg(ap, uint32_t);
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 0.6, src, name, bounce, NULL);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        } else {
            double bounce = va_arg(ap, double);
            double value = va_arg(ap, double);
            nemomotion_attach(m, 0.6, src, name, bounce, NULL);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

static inline void _nemoshow_item_motion_bounce_with_callback(struct showone *src,
        NemoMotion_FrameCallback callback, void *userdata,
        uint32_t easetype,  uint32_t dur, uint32_t delay, ...)
{
    RET_IF(!src || !src->show);
    NemoMotion *m = nemomotion_create(src->show, easetype, dur, delay);
    nemomotion_set_frame_callback(m, callback, userdata);

    const char *name;

    va_list ap;
    va_start(ap, delay);
	while ((name = va_arg(ap, const char *))) {
        if (!strcmp(name, "fill") || !strcmp(name, "stroke")) {
            uint32_t bounce = va_arg(ap, uint32_t);
            uint32_t value = va_arg(ap, uint32_t);
            nemomotion_attach(m, 0.6, src, name, bounce, NULL);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        } else {
            double bounce = va_arg(ap, double);
            double value = va_arg(ap, double);
            nemomotion_attach(m, 0.6, src, name, bounce, NULL);
            nemomotion_attach(m, 1.0, src, name, value, NULL);
        }
    }
    va_end(ap);
    nemomotion_run(m);
}

/*****************************************/
/* _NEMOSHOW ITEM
 * **************************************/
static inline void _nemoshow_item_get_geometry(struct showone *one, int *_x, int *_y, int *_w, int *_h)
{
    // FIXME:  scale, rotate
    int px = 0, py = 0;
    if (one->parent) {
        _nemoshow_item_get_geometry(one->parent, &px, &py, NULL, NULL);
    }

    int x, y, w, h;
    if (one->type == NEMOSHOW_CANVAS_TYPE) {
        x = NEMOSHOW_CANVAS_AT(one, ty);
        y = NEMOSHOW_CANVAS_AT(one, ty);
        w = NEMOSHOW_CANVAS_AT(one, width);
        h = NEMOSHOW_CANVAS_AT(one, height);
    } else if (one->type == NEMOSHOW_ITEM_TYPE) {
        x = NEMOSHOW_ITEM_AT(one, ty);
        y = NEMOSHOW_ITEM_AT(one, ty);
        w = NEMOSHOW_ITEM_AT(one, width);
        h = NEMOSHOW_ITEM_AT(one, height);
    } else {
        return;
    }

    x = x + px;
    y = y + py;

    if (_x) *_x = x;
    if (_y) *_y = y;
    if (_w) *_w = w;
    if (_h) *_h = h;
}

typedef struct _NumberPath NumberPath;
struct _NumberPath {
    double mx, my;
    double cx0_0, cy0_0, cx0_1, cy0_1, cx0_2, cy0_2;
    double cx1_0, cy1_0, cx1_1, cy1_1, cx1_2, cy1_2;
    double cx2_0, cy2_0, cx2_1, cy2_1, cx2_2, cy2_2;
    double cx3_0, cy3_0, cx3_1, cy3_1, cx3_2, cy3_2;
};

static inline struct showone *NUM_CREATE(struct showone *parent, uint32_t color, double stroke_w, double size, int num)
{
    RET_IF((num > 9) || (num < 0), NULL);

    NumberPath NUMBERS[] = {
        {
            0.245856, 0.552486,
            0.245856, 0.331492, 0.370166, 0.099448, 0.552486, 0.099448,
            0.734807, 0.099448, 0.861878, 0.331492, 0.861878, 0.552486,
            0.861878, 0.773481, 0.734807, 0.994475, 0.552486, 0.994475,
            0.370166, 0.994475, 0.245856, 0.773481, 0.245856, 0.552486
        },
        {
            0.425414, 0.113260,
            0.425414, 0.113260, 0.577348, 0.113260, 0.577348, 0.113260,
            0.577348, 0.113260, 0.577348, 1.000000, 0.577348, 1.000000,
            0.577348, 1.000000, 0.577348, 1.000000, 0.577348, 1.000000,
            0.577348, 1.000000, 0.577348, 1.000000, 0.577348, 1.000000
        },
        {
            0.309392, 0.331492,
            0.325967, 0.011050, 0.790055, 0.022099, 0.798343, 0.337017,
            0.798343, 0.430939, 0.718232, 0.541436, 0.596685, 0.674033,
            0.519337, 0.762431, 0.408840, 0.856354, 0.314917, 0.977901,
            0.314917, 0.977901, 0.812155, 0.977901, 0.812155, 0.977901
        },
        {
            0.361878, 0.298343,
            0.348066, 0.149171, 0.475138, 0.099448, 0.549724, 0.099448,
            0.861878, 0.099448, 0.806630, 0.530387, 0.549724, 0.530387,
            0.872928, 0.530387, 0.828729, 0.994475, 0.552486, 0.994475,
            0.298343, 0.994475, 0.309392, 0.828729, 0.312155, 0.790055
        },
        {
            0.856354, 0.806630,
            0.856354, 0.806630, 0.237569, 0.806630, 0.237569, 0.806630,
            0.237569, 0.806630, 0.712707, 0.138122, 0.712707, 0.138122,
            0.712707, 0.138122, 0.712707, 0.806630, 0.712707, 0.806630,
            0.712707, 0.806630, 0.712707, 0.988950, 0.712707, 0.988950
        },
        {
            0.806630, 0.110497,
            0.502762, 0.110497, 0.502762, 0.110497, 0.502762, 0.110497,
            0.397790, 0.430939, 0.397790, 0.430939, 0.397790, 0.430939,
            0.535912, 0.364641, 0.801105, 0.469613, 0.801105, 0.712707,
            0.773481, 1.011050, 0.375691, 1.093923, 0.248619, 0.850829
        },
        {
            0.607735, 0.110497,
            0.607735, 0.110497, 0.607735, 0.110497, 0.607735, 0.110497,
            0.392265, 0.436464, 0.265193, 0.508287, 0.254144, 0.696133,
            0.287293, 1.130171, 0.872928, 1.060773, 0.845304, 0.696133,
            0.806630, 0.364641, 0.419890, 0.353591, 0.295580, 0.552486
        },
        {
            0.259668, 0.116022,
            0.259668, 0.116022, 0.872928, 0.116022, 0.872928, 0.116022,
            0.872928, 0.116022, 0.700000, 0.422099, 0.700000, 0.422099,
            0.700000, 0.422099, 0.477348, 0.733149, 0.477348, 0.733149,
            0.477348, 0.733149, 0.254144, 1.000000, 0.254144, 1.000000
        },
        {
            0.558011, 0.530387,
            0.243094, 0.524862, 0.243094, 0.104972, 0.558011, 0.104972,
            0.850829, 0.104972, 0.850829, 0.530387, 0.558011, 0.530387,
            0.243094, 0.530387, 0.198895, 0.988950, 0.558011, 0.988950,
            0.850829, 0.988950, 0.850829, 0.530387, 0.558011, 0.530387
        },
        {
            0.809392, 0.552486,
            0.685083, 0.751381, 0.298343, 0.740331, 0.259668, 0.408840,
            0.232044, 0.044199, 0.817680, -0.044199, 0.850829, 0.408840,
            0.839779, 0.596685, 0.712707, 0.668508, 0.497238, 0.994475,
            0.497238, 0.994475, 0.497238, 0.994475, 0.497238, 0.994475
        }
    };

    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHARRAY_ITEM);

    double mx = NUMBERS[num].mx;
    double my = NUMBERS[num].my;
    double cx0_0 = NUMBERS[num].cx0_0;
    double cy0_0 = NUMBERS[num].cy0_0;
    double cx0_1 = NUMBERS[num].cx0_1;
    double cy0_1 = NUMBERS[num].cy0_1;
    double cx0_2 = NUMBERS[num].cx0_2;
    double cy0_2 = NUMBERS[num].cy0_2;
    double cx1_0 = NUMBERS[num].cx1_0;
    double cy1_0 = NUMBERS[num].cy1_0;
    double cx1_1 = NUMBERS[num].cx1_1;
    double cy1_1 = NUMBERS[num].cy1_1;
    double cx1_2 = NUMBERS[num].cx1_2;
    double cy1_2 = NUMBERS[num].cy1_2;
    double cx2_0 = NUMBERS[num].cx2_0;
    double cy2_0 = NUMBERS[num].cy2_0;
    double cx2_1 = NUMBERS[num].cx2_1;
    double cy2_1 = NUMBERS[num].cy2_1;
    double cx2_2 = NUMBERS[num].cx2_2;
    double cy2_2 = NUMBERS[num].cy2_2;
    double cx3_0 = NUMBERS[num].cx3_0;
    double cy3_0 = NUMBERS[num].cy3_0;
    double cx3_1 = NUMBERS[num].cx3_1;
    double cy3_1 = NUMBERS[num].cy3_1;
    double cx3_2 = NUMBERS[num].cx3_2;
    double cy3_2 = NUMBERS[num].cy3_2;

    nemoshow_item_path_moveto(one, mx, my);
    nemoshow_item_path_cubicto(one, cx0_0, cy0_0, cx0_1, cy0_1, cx0_2, cy0_2);
    nemoshow_item_path_cubicto(one, cx1_0, cy1_0, cx1_1, cy1_1, cx1_2, cy1_2);
    nemoshow_item_path_cubicto(one, cx2_0, cy2_0, cx2_1, cy2_1, cx2_2, cy2_2);
    nemoshow_item_path_cubicto(one, cx3_0, cy3_0, cx3_1, cy3_1, cx3_2, cy3_2);

    nemoshow_item_scale(one, size, size);
    nemoshow_item_set_stroke_color(one, RGBA(color));
    nemoshow_item_set_stroke_width(one, stroke_w/size);
    return one;
}

static inline void NUM_SET_COLOR(struct showone *one, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(one, easetype, duration, delay,
                "stroke", color,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(one, RGBA(color));
    }
}

static inline void NUM_UPDATE(struct showone *one,
        uint32_t easetype, int duration, int delay, int num)
{
    RET_IF((num > 9) || (num < 0));

    NumberPath NUMBERS[] = {
        {
            0.245856, 0.552486,
            0.245856, 0.331492, 0.370166, 0.099448, 0.552486, 0.099448,
            0.734807, 0.099448, 0.861878, 0.331492, 0.861878, 0.552486,
            0.861878, 0.773481, 0.734807, 0.994475, 0.552486, 0.994475,
            0.370166, 0.994475, 0.245856, 0.773481, 0.245856, 0.552486
        },
        {
            0.425414, 0.113260,
            0.425414, 0.113260, 0.577348, 0.113260, 0.577348, 0.113260,
            0.577348, 0.113260, 0.577348, 1.000000, 0.577348, 1.000000,
            0.577348, 1.000000, 0.577348, 1.000000, 0.577348, 1.000000,
            0.577348, 1.000000, 0.577348, 1.000000, 0.577348, 1.000000
        },
        {
            0.309392, 0.331492,
            0.325967, 0.011050, 0.790055, 0.022099, 0.798343, 0.337017,
            0.798343, 0.430939, 0.718232, 0.541436, 0.596685, 0.674033,
            0.519337, 0.762431, 0.408840, 0.856354, 0.314917, 0.977901,
            0.314917, 0.977901, 0.812155, 0.977901, 0.812155, 0.977901
        },
        {
            0.361878, 0.298343,
            0.348066, 0.149171, 0.475138, 0.099448, 0.549724, 0.099448,
            0.861878, 0.099448, 0.806630, 0.530387, 0.549724, 0.530387,
            0.872928, 0.530387, 0.828729, 0.994475, 0.552486, 0.994475,
            0.298343, 0.994475, 0.309392, 0.828729, 0.312155, 0.790055
        },
        {
            0.856354, 0.806630,
            0.856354, 0.806630, 0.237569, 0.806630, 0.237569, 0.806630,
            0.237569, 0.806630, 0.712707, 0.138122, 0.712707, 0.138122,
            0.712707, 0.138122, 0.712707, 0.806630, 0.712707, 0.806630,
            0.712707, 0.806630, 0.712707, 0.988950, 0.712707, 0.988950
        },
        {
            0.806630, 0.110497,
            0.502762, 0.110497, 0.502762, 0.110497, 0.502762, 0.110497,
            0.397790, 0.430939, 0.397790, 0.430939, 0.397790, 0.430939,
            0.535912, 0.364641, 0.801105, 0.469613, 0.801105, 0.712707,
            0.773481, 1.011050, 0.375691, 1.093923, 0.248619, 0.850829
        },
        {
            0.607735, 0.110497,
            0.607735, 0.110497, 0.607735, 0.110497, 0.607735, 0.110497,
            0.392265, 0.436464, 0.265193, 0.508287, 0.254144, 0.696133,
            0.287293, 1.130171, 0.872928, 1.060773, 0.845304, 0.696133,
            0.806630, 0.364641, 0.419890, 0.353591, 0.295580, 0.552486
        },
        {
            0.259668, 0.116022,
            0.259668, 0.116022, 0.872928, 0.116022, 0.872928, 0.116022,
            0.872928, 0.116022, 0.700000, 0.422099, 0.700000, 0.422099,
            0.700000, 0.422099, 0.477348, 0.733149, 0.477348, 0.733149,
            0.477348, 0.733149, 0.254144, 1.000000, 0.254144, 1.000000
        },
        {
            0.558011, 0.530387,
            0.243094, 0.524862, 0.243094, 0.104972, 0.558011, 0.104972,
            0.850829, 0.104972, 0.850829, 0.530387, 0.558011, 0.530387,
            0.243094, 0.530387, 0.198895, 0.988950, 0.558011, 0.988950,
            0.850829, 0.988950, 0.850829, 0.530387, 0.558011, 0.530387
        },
        {
            0.809392, 0.552486,
            0.685083, 0.751381, 0.298343, 0.740331, 0.259668, 0.408840,
            0.232044, 0.044199, 0.817680, -0.044199, 0.850829, 0.408840,
            0.839779, 0.596685, 0.712707, 0.668508, 0.497238, 0.994475,
            0.497238, 0.994475, 0.497238, 0.994475, 0.497238, 0.994475
        }
    };

    double mx = NUMBERS[num].mx;
    double my = NUMBERS[num].my;
    double cx0_0 = NUMBERS[num].cx0_0;
    double cy0_0 = NUMBERS[num].cy0_0;
    double cx0_1 = NUMBERS[num].cx0_1;
    double cy0_1 = NUMBERS[num].cy0_1;
    double cx0_2 = NUMBERS[num].cx0_2;
    double cy0_2 = NUMBERS[num].cy0_2;
    double cx1_0 = NUMBERS[num].cx1_0;
    double cy1_0 = NUMBERS[num].cy1_0;
    double cx1_1 = NUMBERS[num].cx1_1;
    double cy1_1 = NUMBERS[num].cy1_1;
    double cx1_2 = NUMBERS[num].cx1_2;
    double cy1_2 = NUMBERS[num].cy1_2;
    double cx2_0 = NUMBERS[num].cx2_0;
    double cy2_0 = NUMBERS[num].cy2_0;
    double cx2_1 = NUMBERS[num].cx2_1;
    double cy2_1 = NUMBERS[num].cy2_1;
    double cx2_2 = NUMBERS[num].cx2_2;
    double cy2_2 = NUMBERS[num].cy2_2;
    double cx3_0 = NUMBERS[num].cx3_0;
    double cy3_0 = NUMBERS[num].cy3_0;
    double cx3_1 = NUMBERS[num].cx3_1;
    double cy3_1 = NUMBERS[num].cy3_1;
    double cx3_2 = NUMBERS[num].cx3_2;
    double cy3_2 = NUMBERS[num].cy3_2;

    if (duration > 0) {
        NemoMotion *m;
        m = nemomotion_create(one->show, easetype, duration, delay);
        nemomotion_attach(m, 1.0,
                one, "points:0", mx,
                one, "points:1", my,
                one, "points:2", 0.0,
                one, "points:3", 0.0,
                one, "points:4", 0.0,
                one, "points:5", 0.0,
                one, "points:6", cx0_0,
                one, "points:7", cy0_0,
                one, "points:8", cx0_1,
                one, "points:9", cy0_1,
                one, "points:10", cx0_2,
                one, "points:11", cy0_2,
                one, "points:12", cx1_0,
                one, "points:13", cy1_0,
                one, "points:14", cx1_1,
                one, "points:15", cy1_1,
                one, "points:16", cx1_2,
                one, "points:17", cy1_2,
                one, "points:18", cx2_0,
                one, "points:19", cy2_0,
                one, "points:20", cx2_1,
                one, "points:21", cy2_1,
                one, "points:22", cx2_2,
                one, "points:23", cy2_2,
                one, "points:24", cx3_0,
                one, "points:25", cy3_0,
                one, "points:26", cx3_1,
                one, "points:27", cy3_1,
                one, "points:28", cx3_2,
                one, "points:29", cy3_2,
                NULL);

        nemomotion_run(m);
    } else {
        nemoshow_item_path_clear(one);
        nemoshow_item_path_moveto(one, mx, my);
        nemoshow_item_path_cubicto(one, cx0_0, cy0_0, cx0_1, cy0_1, cx0_2, cy0_2);
        nemoshow_item_path_cubicto(one, cx1_0, cy1_0, cx1_1, cy1_1, cx1_2, cy1_2);
        nemoshow_item_path_cubicto(one, cx2_0, cy2_0, cx2_1, cy2_1, cx2_2, cy2_2);
        nemoshow_item_path_cubicto(one, cx3_0, cy3_0, cx3_1, cy3_1, cx3_2, cy3_2);
    }
}

typedef struct _Coords Coords;
struct _Coords
{
    double x;
    double y;
};

//Ref: http://stackoverflow.com/questions/1734745/how-to-create-circle-with-b%C3%A9zier-curves
//Ref: http://spencermortensen.com/articles/bezier-circle/
static inline Coords *_bezier_circle_get(int n, double r)
{
    double c = r * ((double)4/3) * tan(M_PI/(2 * n));

    int num = n * 3;
    Coords *p = (Coords *)calloc(sizeof(Coords), num + 1);

    double cosv = cos(2 * M_PI / n);
    double sinv = sin(2 * M_PI / n);

    p[0].x = r;
    p[0].y = 0;
    p[num - 1].x = p[0].x;
    p[num - 1].y = p[0].y - c;
    p[1].x = p[0].x;
    p[1].y = p[0].y + c;
    p[2].x = p[num - 1].x * cosv - p[num - 1].y * sinv;
    p[2].y = p[num - 1].x * sinv - p[num - 1].y * cosv;

    int i = 0 ;
    for (i = 3 ; i < num ; i+=3) {
        p[i].x = p[i-3].x * cosv - p[i-3].y * sinv;
        p[i].y = p[i-3].x * sinv + p[i-3].y * cosv;
        p[i+1].x = p[i-2].x * cosv - p[i-2].y * sinv;
        p[i+1].y = p[i-2].x * sinv + p[i-2].y * cosv;
        p[i+2].x = p[i-1].x * cosv - p[i-1].y * sinv;
        p[i+2].y = p[i-1].x * sinv + p[i-1].y * cosv;
    }
    p[num].x = p[0].x;
    p[num].y = p[0].y;

    return p;
}

static inline struct showone *PATH_CIRCLE_CREATE(struct showone *parent, int r)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHARRAY_ITEM);

    int n = 4;
    nemoshow_item_set_r(one, r);
    Coords *p = _bezier_circle_get(n, r);

    nemoshow_item_path_moveto(one, p[0].x, p[0].y);

    int i = 0;
    for (i = 0 ; i+2 < n*3 ; i+=3) {
        nemoshow_item_path_cubicto(one,
                p[i+1].x, p[i+1].y,
                p[i+2].x, p[i+2].y,
                p[i+3].x, p[i+3].y);
    }
    free(p);

    return one;

}

static inline struct showone *PATH_CIRCLE_CREATE_FT(struct showone *parent, int r, double from, double to)
{
    struct showone *one;
    ITEM_CREATE(one, parent, NEMOSHOW_PATHARRAY_ITEM);

    int n = 4;
    nemoshow_item_set_r(one, r);
    Coords *p = _bezier_circle_get(n, r);

    nemoshow_item_path_moveto(one, p[0].x, p[0].y);

    int i = 0;
    for (i = 0 ; i+2 < n*3 ; i+=3) {
        nemoshow_item_path_cubicto(one,
                p[i+1].x, p[i+1].y,
                p[i+2].x, p[i+2].y,
                p[i+3].x, p[i+3].y);
    }
    free(p);

    return one;

}

static inline void PATH_CIRCLE_TO_CIRCLE(struct showone *one, uint32_t easetype,
        int duration, int delay)
{
    RET_IF(!one || !one->show);

    int n = 4;
    double r = NEMOSHOW_ITEM_AT(one, r);

    Coords *p = _bezier_circle_get(n, r);

    // FIXME: n should be 4!!!
    NemoMotion *m;
    m = nemomotion_create(one->show, easetype, duration, delay);
    nemomotion_attach(m, 1.0,
            one, "points:0",  p[0].x,
            one, "points:1",  p[0].y,
            one, "points:2",  0.0,
            one, "points:3",  0.0,
            one, "points:4",  0.0,
            one, "points:5",  0.0,
            one, "points:6",  p[1].x,
            one, "points:7",  p[1].y,
            one, "points:8",  p[2].x,
            one, "points:9",  p[2].y,
            one, "points:10",  p[3].x,
            one, "points:11", p[3].y,
            one, "points:12", p[4].x,
            one, "points:13", p[4].y,
            one, "points:14", p[5].x,
            one, "points:15", p[5].y,
            one, "points:16", p[6].x,
            one, "points:17", p[6].y,
            one, "points:18", p[7].x,
            one, "points:19", p[7].y,
            one, "points:20", p[8].x,
            one, "points:21", p[8].y,
            one, "points:22", p[9].x,
            one, "points:23", p[9].y,
            one, "points:24", p[10].x,
            one, "points:25", p[10].y,
            one, "points:26", p[11].x,
            one, "points:27", p[11].y,
            one, "points:28", p[12].x,
            one, "points:29", p[12].y,
            NULL);
    nemomotion_run(m);
}

static inline void PATH_CIRCLE_TO_RECT(struct showone *one, uint32_t easetype,
        int duration, int delay)
{
    RET_IF(!one || !one->show);
    double r = NEMOSHOW_ITEM_AT(one, r);
    double stroke_w = NEMOSHOW_ITEM_AT(one, stroke_width);
    // XXX: adjust for stroke width/2
    // stroking is not correct
    double adjust = stroke_w/2;

    // FIXME: n should be 4!!!
    NemoMotion *m;
    m = nemomotion_create(one->show, easetype, duration, delay);
    nemomotion_attach(m, 1.0f,
            one, "points:0", r + adjust,
            one, "points:1", r,
            one, "points:2", 0.0,
            one, "points:3", 0.0,
            one, "points:4", 0.0,
            one, "points:5", 0.0,
            one, "points:6", 0.0,
            one, "points:7", r,
            one, "points:8", 0.0,
            one, "points:9", r,
            one, "points:10", -r,
            one, "points:11", r,
            one, "points:12", -r,
            one, "points:13", 0.0,
            one, "points:14", -r,
            one, "points:15", 0.0,
            one, "points:16", -r,
            one, "points:17", -r,
            one, "points:18", 0.0,
            one, "points:19", -r,
            one, "points:20", 0.0,
            one, "points:21", -r,
            one, "points:22", r,
            one, "points:23", -r,
            one, "points:24", r,
            one, "points:25", 0.0,
            one, "points:26", r,
            one, "points:27", 0.0,
            one, "points:28", r,
            one, "points:29", r,
            NULL);
    nemomotion_run(m);
}

#if 0
static inline void file_info_execute(struct nemoshow *show, FileInfo *fileinfo)
{
    char name[PATH_MAX];
    char cmds[PATH_MAX];

    if (!fileinfo->exec) {
        snprintf(name, PATH_MAX, "%s", fileinfo->path);
        snprintf(cmds, PATH_MAX, "%s", fileinfo->path);
    } else {
        char *buf;
        char *tok;
        buf = strdup(fileinfo->exec);
        tok = strtok(buf, ":");
        snprintf(name, PATH_MAX, "%s", tok);
        snprintf(cmds, PATH_MAX, "%s:%s", fileinfo->exec, fileinfo->path);
    }

    nemoshow_view_set_anchor(show, 0.5, 0.5);
    nemocanvas_execute_command(NEMOSHOW_AT(show, canvas), name, cmds);

}
#endif

#if 0
const char *SEGMENT7[] = {
    "M0.05 0.05 L0.05 0.05 L0.10 0.00 L0.40 0.00 L0.45 0.05 L0.40 0.10 L0.10 0.10 L0.05 0.05",
    "M0.45 0.05 L0.45 0.05 L0.50 0.10 L0.50 0.40 L0.45 0.45 L0.40 0.40 L0.40 0.10 L0.45 0.05",
    "M0.45 0.45 L0.45 0.45 L0.50 0.50 L0.50 0.80 L0.45 0.85 L0.40 0.80 L0.40 0.50 L0.45 0.45",
    "M0.45 0.85 L0.45 0.85 L0.40 0.90 L0.10 0.90 L0.05 0.85 L0.10 0.80 L0.40 0.80 L0.45 0.85",
    "M0.05 0.85 L0.05 0.85 L0.00 0.80 L0.00 0.50 L0.05 0.45 L0.10 0.50 L0.10 0.80 L0.05 0.85",
    "M0.05 0.45 L0.05 0.45 L0.00 0.40 L0.00 0.10 L0.05 0.05 L0.10 0.10 L0.10 0.40 L0.05 0.45",
    "M0.05 0.45 L0.05 0.45 L0.10 0.40 L0.40 0.40 L0.45 0.45 L0.40 0.50 L0.10 0.50 L0.05 0.45",
};

static inline bool showxml_load(struct nemoshow *show, const char *data_dir, const char *filename)
{
    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%s/%s", data_dir, filename);

    if (file_is_exist(filename))
        nemoshow_load_xml(show, filename);
    else if (file_is_exist(buf))
        nemoshow_load_xml(show, buf);
    else  {
        ERR("file not exist: %s or %s", filename, buf);
        return false;
    }
    return true;
}
#endif

static inline char *uuid_gen()
{
    uuid_t uuid;
    char uuidstr[37];
    uuid_generate_time_safe(uuid);
    uuid_unparse_lower(uuid, uuidstr);
    uuid_clear(uuid);

    return strdup(uuidstr);
}

static inline struct nemobus *NEMOBUS_CREATE()
{
    struct nemobus *bus;
    bus = nemobus_create();
    nemobus_connect(bus, NULL);
    return bus;
}

static inline struct busmsg *NEMOMSG_CREATE_CMD(const char *type, const char *path)
{
    struct busmsg *msg;
    msg = nemobus_msg_create();
    nemobus_msg_set_name(msg, "command");
    nemobus_msg_set_attr(msg, "type", type);
    nemobus_msg_set_attr(msg, "path", path);
    return msg;
}

static inline char *NEMOMSG_SEND(struct nemobus *bus, struct busmsg *msg)
{
    char *uuid = uuid_gen();
    nemobus_msg_set_attr(msg, "uuid", uuid);
    nemobus_send_msg(bus, "", "/nemoshell", msg);
    nemobus_msg_destroy(msg);

    nemobus_destroy(bus);
    return uuid;
}

static inline char *nemo_execute(const char *owner, const char *type, const char *path, const char *args, const char *resize, float x, float y, float r, float sx, float sy)
{
    RET_IF(!type, NULL);
    RET_IF(!path, NULL);

    struct nemobus *bus;

    struct busmsg *msg;
    struct itemone *one;
    char states[512];

    one = nemoitem_one_create();
    nemoitem_one_set_attr_format(one, "x", "%f", x);
    nemoitem_one_set_attr_format(one, "y", "%f", y);
    nemoitem_one_set_attr_format(one, "r", "%f", r);
    nemoitem_one_set_attr_format(one, "sx", "%f", sx);
    nemoitem_one_set_attr_format(one, "sy", "%f", sy);
    if (owner) nemoitem_one_set_attr(one, "owner", owner);
    if (resize) nemoitem_one_set_attr(one, "resize", resize);
    nemoitem_one_save_attrs(one, states, ';');
    nemoitem_one_destroy(one);

    msg = NEMOMSG_CREATE_CMD(type, path);
    if (args) nemobus_msg_set_attr(msg, "args", args);
    nemobus_msg_set_attr(msg, "states", states);

    bus = NEMOBUS_CREATE();
    return NEMOMSG_SEND(bus, msg);
}

static inline void nemo_close(const char *uuid)
{
    struct nemobus *bus;
    bus = nemobus_create();
    nemobus_connect(bus, NULL);

    struct busmsg *msg;
    msg = nemobus_msg_create();
    nemobus_msg_set_name(msg, "command");
    nemobus_msg_set_attr(msg, "type", "close");
    nemobus_msg_set_attr(msg, "uuid", uuid);
    nemobus_send_msg(bus, "", "/nemoshell", msg);
    nemobus_msg_destroy(msg);

    nemobus_destroy(bus);
}

#endif
