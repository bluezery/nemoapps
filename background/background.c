#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "util.h"
#include "widgets.h"
#include "view.h"
#include "graph.h"
#include "nemohelper.h"

static int BG_TIMEOUT = 30000;
static int LINE_TIMEOUT = 15000;
static int GAP = 10;

#define GRADIENT0 0x5c8affff
#define GRADIENT1 0xEA562DFF
#define GRADIENT2 0xdf41a1ff
#define GRADIENT3 0x35FFFFFF
typedef struct _Dot Dot;
struct _Dot {
    struct nemotool *tool;
    int r;

    struct showone *shader;
    struct showone *blur;
    struct showone *group;

    struct nemotimer *timer;
    struct showone *one[3];
};

static void _dot_timeout(struct nemotimer *timer, void *userdata)
{
    Dot *dot = userdata;

    struct showone *one;
    int i;
    for (i = 0 ; i < 3; i++) {
        one = dot->one[i];
        nemoshow_item_set_alpha(one, 1.0);
        nemoshow_item_scale(one, 0.0, 0.0);
        nemoshow_item_rotate(one, 0.0);
        _nemoshow_item_motion_bounce(one, NEMOEASE_LINEAR_TYPE, 500, i * 250,
                "ro", 360.0, 360.0,
                "alpha", 0.25, 0.5,
                "sx", 1.25, 1.0,
                "sy", 1.25, 1.0, NULL);
    }

    nemotimer_set_timeout(timer, 1000);
}

static Dot *dot_create(struct nemotool *tool, struct showone *parent, int r, uint32_t color0, uint32_t color1, uint32_t color2, uint32_t color3)
{
    Dot *dot = calloc(sizeof(Dot), 1);
    dot->tool = tool;
    dot->r = r;

    struct showone *blur;
    dot->blur = blur = BLUR_CREATE("solid", 5);

    struct showone *shader;
    dot->shader = shader = LINEAR_GRADIENT_CREATE(-r/2, -r/2, r/2, r/2);
    STOP_CREATE(shader, color0, 0.0);
    STOP_CREATE(shader, color1, 0.333);
    STOP_CREATE(shader, color2, 0.666);
    STOP_CREATE(shader, color3, 1.0);

    struct showone *one;
    struct showone *group;
    group = dot->group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    int i;
    for (i = 0; i < 3 ; i++) {
        dot->one[i] = one = CIRCLE_CREATE(group, r);
        nemoshow_item_set_fill_color(one, RGBA(WHITE));
        nemoshow_item_set_shader(one, shader);
        nemoshow_item_set_filter(one, blur);
        nemoshow_item_set_alpha(one, 0.0);
    }

    dot->timer = TOOL_ADD_TIMER(tool, 0, _dot_timeout, dot);
    return dot;
}

void dot_destroy(Dot *dot)
{
    nemoshow_one_destroy(dot->group);
    nemotimer_destroy(dot->timer);
    free(dot);
}

void dot_hide(Dot *dot, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(dot->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(dot->group, 0.0);
    }
    nemotimer_set_timeout(dot->timer, 0);
}

static void _dot_destroy(struct nemotimer *timer, void *userdata)
{
    dot_destroy(userdata);
    nemotimer_destroy(timer);
}

void dot_hide_destroy(Dot *dot, uint32_t easetype, int duration, int delay)
{
    dot_hide(dot, easetype, duration, delay);
    TOOL_ADD_TIMER(dot->tool, duration + delay, _dot_destroy, dot);
}

static void dot_translate(Dot *dot, double tx, double ty)
{
    nemoshow_item_translate(dot->group, tx, ty);
}

static void dot_show(Dot *dot, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(dot->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(dot->group, 1.0);
    }
    nemotimer_set_timeout(dot->timer, 10);
}

typedef struct _Background Background;
struct _Background {
    const char *uuid;
    struct nemoshow *show;
    struct nemotool *tool;
    int w, h;
    char *exec;
    NemoWidget *bg_widget;
    struct showone *bg_group;
    Gallery *gallery;
    struct nemotimer *gallery_timer;
    int gallery_idx;

    NemoWidget *widget;
    struct showone *group;

    Sketch *sketch;
    struct nemotimer *sketch_timer;
};

static void _background_gallery_timeout(struct nemotimer *timer, void *userdata)
{
    Background *bg = userdata;

    GalleryItem *gallery_it;
    gallery_it = LIST_DATA(list_get_nth(bg->gallery->items, bg->gallery_idx));
    gallery_item_hide(gallery_it, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);

    bg->gallery_idx++;
    if (bg->gallery_idx >= list_count(bg->gallery->items))
        bg->gallery_idx = 0;

    gallery_it = LIST_DATA(list_get_nth(bg->gallery->items, bg->gallery_idx));
    gallery_item_show(gallery_it, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    nemotimer_set_timeout(timer, BG_TIMEOUT);
    nemoshow_dispatch_frame(bg->show);
}

static void _background_sketch_timeout(struct nemotimer *timer, void *userdata)
{
    Background *bg = userdata;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sketch_remove_old(bg->sketch, LINE_TIMEOUT);
    nemotimer_set_timeout(timer, LINE_TIMEOUT);
    nemoshow_dispatch_frame(bg->show);
}

static void _background_exe_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    Background *bg = userdata;

    if (nemoshow_event_is_down(show, event)) {
        Dot *dot = dot_create(bg->tool, bg->group, 50, GRADIENT0, GRADIENT1, GRADIENT2, GRADIENT3);
        dot_show(dot, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        nemowidget_grab_set_data(grab, "dot", dot);
    } else if (nemoshow_event_is_motion(show, event)) {
        Dot *dot = nemowidget_grab_get_data(grab, "dot");
        dot_translate(dot, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
    } else if (nemoshow_event_is_up(show, event)) {
        Dot *dot = nemowidget_grab_get_data(grab, "dot");
        dot_hide_destroy(dot, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

        if (bg->exec) {
            float x, y;
            x = nemoshow_event_get_x(event);
            y = nemoshow_event_get_y(event);
            nemoshow_transform_to_viewport(show, x, y, &x, &y);
            struct nemoshow *show = nemowidget_get_show(widget);
            nemoshow_view_set_anchor(show, 0.5, 0.5);

            nemo_execute(bg->uuid, "app", bg->exec, NULL, NULL, x, y, 0, 1, 1);
        }
    }
    nemoshow_dispatch_frame(bg->show);
}

static void _background_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    Background *bg = userdata;

    if (nemoshow_event_is_down(show, event)) {
        float x, y;
        x = nemoshow_event_get_x(event);
        y = nemoshow_event_get_y(event);
        if ((x < GAP) || (y < GAP) || (x > bg->w - GAP) || (y > bg->h - GAP)) {
            nemowidget_create_grab(widget, event, _background_exe_grab_event, bg);
            nemowidget_enable_event_repeat(widget, false);
            nemowidget_enable_event_repeat(widget, true);
        } else {
            nemowidget_enable_event_repeat(widget, true);
        }
    }
}

Background *background_create(NemoWidget *parent, int w, int h, const char *exec)
{
    Background *bg = calloc(sizeof(Background), 1);
    bg->show = nemowidget_get_show(parent);
    bg->tool = nemowidget_get_tool(parent);
    bg->uuid = nemowidget_get_uuid(parent);
    bg->w = w;
    bg->h = h;
    if (exec) bg->exec = strdup(exec);
    bg->bg_widget = nemowidget_create_vector(parent, w, h);
    bg->bg_group = GROUP_CREATE(nemowidget_get_canvas(bg->bg_widget));
    bg->gallery = gallery_create(bg->tool, bg->bg_group, w, h);

    FileInfo *file;
    List *bgfiles = fileinfo_readdir(BG_IMG_DIR);
    LIST_FREE(bgfiles, file) {
        if (file->is_dir) continue;
        gallery_append_item(bg->gallery, file->path);
        fileinfo_destroy(file);
    }

    bg->gallery_timer = TOOL_ADD_TIMER(bg->tool, 0, _background_gallery_timeout, bg);

    bg->sketch = sketch_create(parent, w, h);

    bg->widget = nemowidget_create_vector(parent, w, h);
    nemowidget_append_callback(bg->widget, "event", _background_event, bg);
    bg->group = GROUP_CREATE(nemowidget_get_canvas(bg->widget));

    bg->sketch_timer = TOOL_ADD_TIMER(bg->tool, 0, _background_sketch_timeout, bg);

    return bg;
}

void background_destroy(Background *bg)
{
    nemotimer_destroy(bg->gallery_timer);
    gallery_destroy(bg->gallery);
    nemowidget_destroy(bg->bg_widget);
    nemowidget_destroy(bg->widget);

    nemotimer_destroy(bg->sketch_timer);
    sketch_destroy(bg->sketch);
    free(bg);
}

void background_show(Background *bg)
{
    nemowidget_show(bg->bg_widget, 0, 0, 0);
    gallery_show(bg->gallery, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    int cnt = list_count(bg->gallery->items);

    if (cnt == 1) {
        gallery_item_show(LIST_DATA(LIST_FIRST(bg->gallery->items)), NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    } else if (cnt > 1) {
        nemotimer_set_timeout(bg->gallery_timer, 10);
    }

    nemowidget_show(bg->widget, 0, 0, 0);
    sketch_show(bg->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    sketch_enable(bg->sketch, true);
    nemotimer_set_timeout(bg->sketch_timer, 10);

    nemoshow_dispatch_frame(bg->show);
}

void background_hide(Background *bg)
{
    nemowidget_hide(bg->bg_widget, 0, 0, 0);
    gallery_hide(bg->gallery, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemotimer_set_timeout(bg->gallery_timer, 0);

    nemowidget_hide(bg->widget, 0, 0, 0);
    sketch_hide(bg->sketch, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemotimer_set_timeout(bg->sketch_timer, 0);

    nemoshow_dispatch_frame(bg->show);
}

void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Background *bg = userdata;
    background_hide(bg);
    nemowidget_win_exit_after(win, 1000);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    int bg_timeout;
    int line_timeout;
    int border_gap;
    char *border_exec;
};

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, appname, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/bg", appname);
        temp = xml_get_value(xml, buf, "timeout");
        if (temp && strlen(temp) > 0) {
            app->bg_timeout = atoi(temp);
        }
        snprintf(buf, PATH_MAX, "%s/line", appname);
        temp = xml_get_value(xml, buf, "timeout");
        if (temp && strlen(temp) > 0) {
            app->line_timeout = atoi(temp);
        }
        snprintf(buf, PATH_MAX, "%s/border", appname);
        temp = xml_get_value(xml, buf, "gap");
        if (temp && strlen(temp) > 0) {
            app->border_gap = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "exec");
        if (temp && strlen(temp) > 0) {
            app->border_exec = strdup(temp);
        }

        xml_unload(xml);
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->border_exec) free(app->border_exec);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    LINE_TIMEOUT = app->line_timeout;
    BG_TIMEOUT = app->bg_timeout;
    GAP = app->border_gap;

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "background");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    Background *bg = background_create(win, app->config->width, app->config->height, app->border_exec);
    nemowidget_append_callback(win, "exit", _win_exit, bg);
    background_show(bg);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    background_hide(bg);
    background_destroy(bg);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
