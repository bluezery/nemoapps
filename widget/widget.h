#ifndef __WIDGET__
#define __WIDGET__

#include <nemotool.h>
#include <nemotale.h>
#include <nemoshow.h>
#include <nemotimer.h>
#include <showone.h>

#include <showitem.h>
#include <showhelper.h>

#include <pixman.h>

#include <nemoutil.h>
#include <nemowrapper.h>

// ************************************** //
// NemoWidget Item //
// ************************************** //
#define NEMOWIDGET_ITEM_CHECK(it, ...) do {\
    if (!(it)) { \
        ERR("Widget Item is NULL"); \
            return __VA_ARGS__; \
    } \
    if (!(it)->widget) { \
        ERR("Widget Item (%p) has no widget", it); \
        return __VA_ARGS__; \
    } \
    if (!(it)->context) { \
        ERR("Widget Item (%p) has no class", it); \
        return __VA_ARGS__; \
    } \
} while (0); \

// ************************************** //
// NemoWidget //
// ************************************** //
#define NEMOWIDGET_CHECK(widget, ...) do {\
    if (!(widget)) { \
        ERR("Widget is NULL"); \
            return __VA_ARGS__; \
    } \
    if (!(widget)->klass) { \
        ERR("Widget (%p) has no class", widget); \
        return __VA_ARGS__; \
    } \
    if (!(widget)->klass->name) { \
        ERR("Widget (%p)'s class has no name", widget); \
        return __VA_ARGS__; \
    } \
} while (0); \

#define NEMOWIDGET_CHECK_CLASS(widget, _klass, ...) do {\
    if ((widget)->klass != (_klass)) { \
        ERR("Widget's class (%p, %s) is not matched with desired: %p(%s)", \
                (widget)->klass, (widget)->klass->name, (_klass), (_klass)->name); \
        return __VA_ARGS__; \
    }; \
} while (0); \

#define NEMOWIDGET_AT(WIDGET, AT) \
    NEMOSHOW_CANVAS_AT(WIDGET->canvas, AT)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    NEMOWIDGET_EVENTTYPE_NONE = 0,
    NEMOWIDGET_EVENTTYPE_DOWN,
    NEMOWIDGET_EVENTTYPE_UP,
    NEMOWIDGET_EVENTTYPE_MOTION,
    NEMOWIDGET_EVENTTYPE_MAX,
} NemoWidgetEventType;

typedef enum {
    NEMOWIDGET_TYPE_WIN = 0,
    NEMOWIDGET_TYPE_EMPTY,
    NEMOWIDGET_TYPE_VECTOR,
    NEMOWIDGET_TYPE_PIXMAN,
    NEMOWIDGET_TYPE_OPENGL,
    NEMOWIDGET_TYPE_MAX,
} NemoWidgetType;

typedef enum {
    NEMOWIDGET_SCOPE_TYPE_RECT = 0,
    NEMOWIDGET_SCOPE_TYPE_CIRCLE,
    NEMOWIDGET_SCOPE_TYPE_ELLIPSE,
    NEMOWIDGET_SCOPE_TYPE_POLYGON,
} NemoWidgetScopeType;

typedef struct _NemoWidgetData NemoWidgetData;
typedef struct _NemoWidgetCore NemoWidgetCore;
typedef struct _NemoWidget NemoWidget;
typedef struct _NemoWidgetClass NemoWidgetClass;
typedef struct _NemoWidgetInfo_Resize NemoWidgetInfo_Resize;
typedef struct _NemoWidgetInfo_Fullscreen NemoWidgetInfo_Fullscreen;
typedef struct _NemoWidgetMotion NemoWidgetMotion;
typedef struct _NemoWidgetMotionFrame NemoWidgetMotionFrame;
typedef struct _NemoWidgetCallback NemoWidgetCallback;
typedef struct _NemoWidgetCall NemoWidgetCall;
typedef struct _NemoWidgetContainer NemoWidgetContainer;
typedef struct _NemoWidgetEraser NemoWidgetEraser;
typedef struct _NemoWidgetScope NemoWidgetScope;
typedef struct _NemoWidgetRegion NemoWidgetRegion;
typedef struct _NemoWidgetItem NemoWidgetItem;

typedef void *(*NemoWidget_Create)(NemoWidget *widget, int w, int h);
typedef void (*NemoWidget_Destroy)(NemoWidget *widget);
typedef bool (*NemoWidget_Area)(NemoWidget *widget, double x, double y);
typedef void (*NemoWidget_Event)(NemoWidget *widget, struct showevent *event);
typedef void (*NemoWidget_Update)(NemoWidget *widget);
typedef void (*NemoWidget_Show)(NemoWidget *widget, uint32_t easetype, int duration, int delay);
typedef void (*NemoWidget_Hide)(NemoWidget *widget, uint32_t easetype, int duration, int delay);
typedef void (*NemoWidget_Resize)(NemoWidget *widget, int w, int h);
typedef void (*NemoWidget_Translate)(NemoWidget *widget, uint32_t easetype, int duration, int delay, int x, int y);
typedef void (*NemoWidget_Scale)(NemoWidget *widget, uint32_t easetype, int duration, int delay, double sx, double sy);
typedef void (*NemoWidget_Rotate)(NemoWidget *widget, uint32_t easetype, int duration, int delay, double ro);
typedef void (*NemoWidget_Alpha)(NemoWidget *widget, uint32_t easetype, int duration, int delay, double alpha);

typedef void *(*NemoWidget_Item_Append)(NemoWidget *widget);
typedef void (*NemoWidget_Item_Remove)(NemoWidgetItem *it);
typedef void (*NemoWidget_Item_Get_Geometry)(NemoWidgetItem *it, int *x, int *y, int *w, int *h);

typedef void (*NemoWidget_Func)(NemoWidget *widget, const char *id, void *info, void *userdata);
typedef bool (*NemoWidget_AreaFunc)(NemoWidget *widget, double x, double y, void *userdata);

struct _NemoWidgetData
{
    char *key;
    void *data;
};

struct _NemoWidgetCore {
    struct nemotool *tool;
    struct nemotale *tale;
    struct nemoshow *show;
    struct showone *scene;
    char *uuid;
};

struct _NemoWidgetClass {
    const char *name;
    NemoWidgetType type;
    NemoWidget_Create create;
    NemoWidget_Destroy destroy;
    NemoWidget_Area area;
    NemoWidget_Event event;
    NemoWidget_Update update;
    NemoWidget_Show show;
    NemoWidget_Hide hide;
    NemoWidget_Resize resize;
    NemoWidget_Translate translate;
    NemoWidget_Scale scale;
    NemoWidget_Rotate rotate;
    NemoWidget_Alpha set_alpha;
    NemoWidget_Item_Append item_append;
    NemoWidget_Item_Remove item_remove;
    NemoWidget_Item_Get_Geometry item_get_geometry;
};

struct _NemoWidgetCallback
{
    NemoWidget_Func func;
    void *userdata;
};

struct _NemoWidgetCall
{
    char *id;
    List *callbacks;     // NemoWidgetCallback
};

struct _NemoWidgetContainer
{
    Inlist __inlist;
    char *name;
    double sx, sy, sw, sh;
    NemoWidget *content;
};

struct _NemoWidgetEraser
{
    void *data;
    void (*free)(void *);
};

struct _NemoWidgetScope
{
    NemoWidgetScopeType type;
    int params_num;
    int *params;
};

struct _NemoWidgetRegion
{
    float x0, y0, x1, y1;
};

struct _NemoWidget {
    Inlist __inlist;

    struct {
        double tx, ty;
        double width, height;
        double sx, sy;
        double alpha;
    } target;

    List *datas;
    uint32_t id;
    char *tag;

    Inlist *children;
    Inlist *items;
    NemoWidget *parent;

    struct showone *canvas; // NULL if win

    List *grabs;
    bool is_show;
    bool event_enable;
    bool event_repeat_enable;
    bool event_grab_enable;

    NemoWidgetCore *core;
    NemoWidgetClass *klass;
    void *context;

    void *data;

    List *calls;

    NemoWidgetContainer *container; // if I am a content

    Inlist *containers;
    List *usergrabs;

    NemoWidgetScope *scope;
    NemoWidgetRegion *region;
};

struct _NemoWidgetInfo_Resize {
    int width;
    int height;
};

struct _NemoWidgetInfo_Fullscreen {
    char *id;
    int32_t x, y, width, height;
};

struct _NemoWidgetItem
{
    Inlist __inlist;
    uint32_t id;
    char *tag;

    NemoWidget *widget;

    void *context;
    void *data;
};

int nemowidget_init();
void nemowidget_shutdown();
const char *nemowidget_get_uuid(NemoWidget *widget);
struct nemotool *nemowidget_get_tool(NemoWidget *widget);
struct nemotale *nemowidget_get_tale(NemoWidget *widget);
struct nemoshow *nemowidget_get_show(NemoWidget *widget);
struct showone *nemowidget_get_scene(NemoWidget *widget);
NemoWidget *nemowidget_get_top_widget(NemoWidget *widget);
NemoWidget *nemowidget_get_parent(NemoWidget *widget);
struct showone *nemowidget_get_canvas(NemoWidget *widget);
NemoWidgetClass *nemowidget_get_klass(NemoWidget *widget);
void *nemowidget_get_context(NemoWidget *widget);

void nemowidget_set_framerate(NemoWidget *widget, int framerate);
void nemowidget_set_data(NemoWidget *widget, const char *key, void *data);
void *nemowidget_get_data(NemoWidget *widget, const char *key);
void nemowidget_set_id(NemoWidget *widget, uint32_t id);
uint32_t nemowidget_get_id(NemoWidget *widget);
void nemowidget_set_tag(NemoWidget *widget, const char *tag);
const char *nemowidget_get_tag(NemoWidget *widget);
void nemowidget_get_geometry(NemoWidget *widget, double *x, double *y, double *w, double *h);
GLuint nemowidget_get_texture(NemoWidget *widget);
void *nemowidget_get_buffer(NemoWidget *widget, int *w, int *h, int *stride);
pixman_image_t *nemowidget_get_pixman(NemoWidget *widget);

struct showone *nemowidget_pick_one(NemoWidget *widget, int x, int y);

//XXX: child transform is not applied!!!
void nemowidget_motion(NemoWidget *widget, uint32_t easetype, uint32_t dur, uint32_t delay, ...);
void nemowidget_motion_bounce(NemoWidget *widget, uint32_t easetype, uint32_t duration, uint32_t delay, ...);
void nemowidget_dirty(NemoWidget *widget);
void nemowidget_dispatch_frame(NemoWidget *widget);
NemoWidget *nemowidget_create(NemoWidgetClass *klass, NemoWidget *parent, int width, int height);
void nemowidget_destroy(NemoWidget *widget);
void nemowidget_show(NemoWidget *widget, uint32_t easetype, int duration, int delay);
void nemowidget_hide(NemoWidget *widget, uint32_t easetype, int duration, int delay);
void nemowidget_update(NemoWidget *widget);
void nemowidget_resize(NemoWidget *widget, double width, double height);
void nemowidget_set_content(NemoWidget *widget, const char *name, NemoWidget *content);

void nemowidget_pivot(NemoWidget *widget, double ax, double ay);
void nemowidget_translate(NemoWidget *widget, uint32_t easetype, int duration, int delay, double tx, double ty);
void nemowidget_get_translate(NemoWidget *widget, double *tx, double *ty);
void nemowidget_scale(NemoWidget *widget, uint32_t easetype, int duration, int delay, double sx, double sy);
void nemowidget_get_scale(NemoWidget *widget, double *sx, double *sy);
void nemowidget_rotate(NemoWidget *widget, uint32_t easetype, int duration, int delay, double ro);
double nemowidget_get_rotate(NemoWidget *widget);
void nemowidget_set_alpha(NemoWidget *widget, uint32_t easetype, int duration, int delay, double alpha);
double nemowidget_get_alpha(NemoWidget *widget);
void nemowidget_set_event_callback(NemoWidget *widget, NemoWidget_Func func, void *userdata);
void nemowidget_set_frame_callback(NemoWidget *widget, NemoWidget_Func func, void *userdata);
void nemowidget_append_callback(NemoWidget *widget, const char *id, NemoWidget_Func func, void *userdata);
void nemowidget_remove_callback(NemoWidget *widget, const char *id, NemoWidget_Func func, void *userdata);
void nemowidget_call_register(NemoWidget *widget, const char *id);
void nemowidget_call_unregister(NemoWidget *widget, const char *id);
bool nemowidget_callback_dispatch(NemoWidget *widget, const char *id, void *info);

void nemowidget_container_register(NemoWidget *widget, const char *name, double sx, double sy, double sw, double sh);
void nemowidget_container_unregister(NemoWidget *widget, const char *name);
void nemowidget_enable_event(NemoWidget *widget, bool enable);
void nemowidget_enable_event_repeat(NemoWidget *widget, bool repeat);
void nemowidget_enable_event_grab(NemoWidget *widget, bool grab);

void nemowidget_stack_above(NemoWidget *widget, NemoWidget *above);
void nemowidget_stack_below(NemoWidget *widget, NemoWidget *below);

void nemowidget_eraser_register(NemoWidget *widget, void *data, void (*free)(void *));
void nemowidget_eraser_unregister(NemoWidget *widget, void *data, void (*free)(void *));

void nemowidget_transform_from_global(NemoWidget *widget, double gx, double gy, double *x, double *y);
extern void nemowidget_transform_to_global(NemoWidget *widget, double x, double y, double *gx, double *gy);
extern void nemowidget_transform_to_window(NemoWidget *widget, double x, double y, double *wx, double *wy);

void nemowidget_put_scope(NemoWidget *widget);
void nemowidget_set_scope(NemoWidget *widget);
void nemowidget_detach_scope(NemoWidget *widget);
void nemowidget_attach_scope_rect(NemoWidget *widget, int x, int y, int w, int h);
void nemowidget_attach_scope_circle(NemoWidget *widget, int x0, int y0, int r);
void nemowidget_attach_scope_ellipse(NemoWidget *widget, int x, int y, int rx, int ry);
void nemowidget_attach_scope_polygon(NemoWidget *widget, int cnt, int *vals);
void nemowidget_put_region(NemoWidget *widget);
void nemowidget_set_region(NemoWidget *widget, float x0, float y0, float x1, float y1);
void nemowidget_detach_region(NemoWidget *widget);
void nemowidget_attach_region(NemoWidget *widget, float x0, float y0, float x1, float y1);

// **************************** //
// NemoWideget Grab
// *************************** //
typedef struct _NemoWidgetGrab NemoWidgetGrab;
typedef void (*NemoWidgetGrab_Callback)(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata);

NemoWidgetGrab *nemowidget_create_grab(NemoWidget *widget, struct showevent *event, NemoWidgetGrab_Callback callback, void *userdata);
void nemowidget_grab_set_data(NemoWidgetGrab *grab, const char *key, void *data);
void *nemowidget_grab_get_data(NemoWidgetGrab *grab, const char *key);
void *nemowidget_grab_get_userdata(NemoWidgetGrab *grab);
void nemowidget_grab_set_done(NemoWidgetGrab *grab, bool done);
bool nemowidget_grab_is_done(NemoWidgetGrab *grab);

// **************************** //
// NemoWideget Item
// *************************** //
void nemowidget_item_set_id(NemoWidgetItem *item, uint32_t id);
uint32_t nemowidget_item_get_id(NemoWidgetItem *it);
void nemowidget_item_set_tag(NemoWidgetItem *it, const char *tag);
const char *nemowidget_item_get_tag(NemoWidgetItem *it);

List *nemowidget_get_items(NemoWidget *widget);
void nemowidget_item_set_data(NemoWidgetItem *it, void *data);
void *nemowidget_item_get_data(NemoWidgetItem *it);
void nemowidget_item_get_geometry(NemoWidgetItem *it, int *x, int *y, int *w, int *h);
NemoWidgetItem *nemowidget_append_item(NemoWidget *widget);
void nemowidget_item_remove(NemoWidgetItem *it);
NemoWidget *nemowidget_item_get_widget(NemoWidgetItem *it);
void *nemowidget_item_get_context(NemoWidgetItem *it);

// ************************************** //
// NemoWidget Window //
// ************************************** //
typedef bool (*NemoWidget_Win_Area)(NemoWidget *win, double x, double y, void *userdata);
extern NemoWidgetClass NEMOWIDGET_WIN;

NemoWidget *nemowidget_create_win(struct nemotool *tool, const char *name, int width, int height);
NemoWidget *nemowidget_create_win_config(struct nemotool *tool, const char *name, Config *base);
void nemowidget_win_load_scene(NemoWidget *widget, int width, int height);

void nemowidget_win_set_anchor(NemoWidget *win, double ax, double ay);
void nemowidget_win_set_max_size(NemoWidget *win, double width, double height);
void nemowidget_win_set_min_size(NemoWidget *win, double width, double height);

void nemowidget_win_set_fullscreen(NemoWidget *win);
void nemowidget_win_set_layer(NemoWidget *win, const char *layer);
void nemowidget_win_set_type(NemoWidget *win, const char *type);
void nemowidget_win_set_event_area(NemoWidget *win, NemoWidget_Win_Area func, void *userdata);
void nemowidget_win_enable_move(NemoWidget *win, int tapcount);
void nemowidget_win_enable_rotate(NemoWidget *win, int tapcount);
void nemowidget_win_enable_scale(NemoWidget *win, int tapcount);
void nemowidget_win_enable_fullscreen(NemoWidget *win, bool enable);
void nemowidget_win_enable_state(NemoWidget *win, const char *state, bool enable);
void nemowidget_win_exit(NemoWidget *widget);
void nemowidget_win_exit_if_no_trans(NemoWidget *win);
void nemowidget_win_exit_after(NemoWidget *win, int timeout);

// ************************************//
// Empty Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_VECTOR;
NemoWidget *nemowidget_create_vector(NemoWidget *parent, int width, int height);

// ************************************//
// Empty Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_OPENGL;
NemoWidget *nemowidget_create_opengl(NemoWidget *parent, int width, int height);

extern NemoWidgetClass NEMOWIDGET_PIXMAN;
NemoWidget *nemowidget_create_pixman(NemoWidget *parent, int width, int height);
#ifdef __cplusplus
}
#endif

#endif
