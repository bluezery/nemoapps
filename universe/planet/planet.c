#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timerfd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

#define PLANET_GRAB_MIN_TIME 500
#define MENU_DESTROY_TIMEOUT 6000

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    double sxy;
    char *backline_path;
};

typedef struct _PlanetView PlanetView;
typedef struct _Planet Planet;
typedef struct _Menu Menu;
typedef struct _MenuItem MenuItem;

struct _PlanetView {
    const char *uuid;
    ConfigApp *app;
    int width, height;
    struct nemotool *tool;
    struct nemoshow *show;

    NemoWidget *widget;
    struct showone *group;
    struct showone *orbit[8];

    Animation *sun_anim;
    List *items;
    List *planets;

    struct nemotimer *planet_timer;
};

struct _Planet {
    int width, height;
    int orbit_idx;
    double position_t;
    ConfigApp *app;
    char *uri;
    PlanetView *view;
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *group;
    struct showone *one;
    struct showone *ring0, *ring1;
    List *shines;
    struct nemotimer *timer;

    uint32_t grab_time;

};

static void _planet_timeout(struct nemotimer *timer, void *userdata)
{
    Planet *planet = userdata;
    int duration = 30000;
    nemoshow_item_rotate(planet->ring0, 0.0);
    _nemoshow_item_motion(planet->ring0, NEMOEASE_LINEAR_TYPE, duration, 0,
            "ro", -360.0, NULL);
    nemoshow_item_rotate(planet->ring1, 0.0);
    _nemoshow_item_motion(planet->ring1, NEMOEASE_LINEAR_TYPE, duration, 0,
            "ro", -360.0, NULL);

    List *l;
    struct showone *one;
    LIST_FOR_EACH(planet->shines, l, one)  {
        nemoshow_item_rotate(one, 0.0);
        _nemoshow_item_motion(one, NEMOEASE_LINEAR_TYPE, duration, 0,
            "ro", 360.0, NULL);
    }

    nemotimer_set_timeout(timer, duration);
}

Planet *planet_dup(Planet *planet)
{
    PlanetView *view = planet->view;
    Planet *dup = calloc(sizeof(Planet), 1);
    dup->view = view;
    dup->tool = view->tool;
    dup->show = view->show;
    dup->app = view->app;
    dup->uri = strdup(planet->uri);

    ConfigApp *app = view->app;

    const char *uri;
    uri = planet->uri;
    struct showone *group;
    struct showone *one;
    dup->group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);

    int iw, ih;
    file_get_image_wh(uri, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;
    dup->one = one = IMAGE_CREATE(group, iw, ih, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    const char *uri0, *uri1;
    int ran = rand()%3;
    if (ran == 0) {
        uri0 = UNIVERSE_IMG_DIR"/ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/ring1.png";
    } else if (ran == 1) {
        uri0 = UNIVERSE_IMG_DIR"/b_ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/b_ring1.png";
    } else {
        uri0 = UNIVERSE_IMG_DIR"/p_ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/p_ring1.png";
    }

    file_get_image_wh(uri0, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;
    dup->ring0 = one = IMAGE_CREATE(group, iw, ih, uri0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    int iww, ihh;
    file_get_image_wh(uri1, &iww, &ihh);
    iww *= app->sxy;
    ihh *= app->sxy;
    dup->ring1 = one = IMAGE_CREATE(group, iww, ihh, uri1);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    dup->width = iww;
    dup->height = ihh;

    uri = UNIVERSE_IMG_DIR"/shine.png";
    file_get_image_wh(uri, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;
    int r = iww/2;
    int cnt = rand()%3 + 1;
    double rad = 360.0/cnt;

    int i = 0;
    for (i = 0 ; i < cnt ; i++) {
        int tx, ty;
        tx = r * cos(rad * i * (M_PI/180.0));
        ty = r * sin(rad * i * (M_PI/180.0));
        one = IMAGE_CREATE(group, iw, ih, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_pivot(one, -tx, -ty);
        nemoshow_item_translate(one, tx, ty);
        dup->shines = list_append(dup->shines, one);
    }

    dup->timer = TOOL_ADD_TIMER(dup->tool, 0, _planet_timeout, dup);

    return dup;
}

void planet_destroy(Planet *planet)
{
    nemotimer_destroy(planet->timer);
    struct showone *one;
    List *l;
    LIST_FOR_EACH(planet->shines, l, one) {
        nemoshow_one_destroy(one);
    }
    nemoshow_one_destroy(planet->group);
    free(planet);
}

void planet_hide(Planet *planet, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(planet->timer, 0);
    if (duration > 0) {
        _nemoshow_item_motion(planet->group, easetype, duration, delay,
                "alpha", 0.0, "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(planet->group, 0.0);
        nemoshow_item_scale(planet->group, 0.0, 0.0);
    }
}

static void _planet_destroy(struct nemotimer *timer, void *userdata)
{
    Planet *planet = userdata;
    planet_destroy(planet);
}

void planet_hide_destroy(Planet *planet)
{
    planet_hide(planet, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    TOOL_ADD_TIMER(planet->tool, 1000, _planet_destroy, planet);
}

Planet *planet_create(PlanetView *view, ConfigApp *app, const char *uri)
{
    Planet *planet = calloc(sizeof(Planet), 1);
    planet->view = view;
    planet->tool = view->tool;
    planet->show = view->show;
    planet->app = view->app;
    planet->uri = strdup(uri);

    struct showone *group;
    struct showone *one;
    planet->group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);

    int iw, ih;
    file_get_image_wh(uri, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;
    planet->one = one = IMAGE_CREATE(group, iw, ih, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, planet);

    const char *uri0, *uri1;
    int ran = rand()%3;
    if (ran == 0) {
        uri0 = UNIVERSE_IMG_DIR"/ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/ring1.png";
    } else if (ran == 1) {
        uri0 = UNIVERSE_IMG_DIR"/b_ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/b_ring1.png";
    } else {
        uri0 = UNIVERSE_IMG_DIR"/p_ring0.png";
        uri1 = UNIVERSE_IMG_DIR"/p_ring1.png";
    }

    planet->ring0 = one = IMAGE_CREATE(group, iw, ih, uri0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    double iww, ihh;
    iww = iw;
    ihh = ih;
    planet->ring1 = one = IMAGE_CREATE(group, iww, ihh, uri1);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    planet->width = iww;
    planet->height = ihh;

    uri = UNIVERSE_IMG_DIR"/shine.png";
    file_get_image_wh(uri, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;
    int r = iww/2;
    int cnt = rand()%3 + 1;
    double rad = 360.0/cnt;

    int i = 0;
    for (i = 0 ; i < cnt ; i++) {
        int tx, ty;
        tx = r * cos(rad * i * (M_PI/180.0));
        ty = r * sin(rad * i * (M_PI/180.0));
        one = IMAGE_CREATE(group, iw, ih, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_pivot(one, -tx, -ty);
        nemoshow_item_translate(one, tx, ty);
        planet->shines = list_append(planet->shines, one);
    }

    planet->timer = TOOL_ADD_TIMER(planet->tool, 0, _planet_timeout, planet);

    return planet;
}

void planet_show(Planet *planet, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(planet->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(planet->group, 1.0);
    }
    nemotimer_set_timeout(planet->timer, 100 + delay);
}

void planet_translate(Planet *planet, uint32_t easetype, int duration, int delay, float tx, float ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(planet->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_item_translate(planet->group, tx, ty);
    }
}

void planet_down(Planet *planet)
{
    _nemoshow_item_motion_bounce(planet->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.5, 0.75, "sy", 0.5, 0.75, NULL);
    _nemoshow_item_motion_bounce(planet->ring0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 50,
            "sx", 0.5, 0.75, "sy", 0.5, 0.75, NULL);
    _nemoshow_item_motion_bounce(planet->ring1, NEMOEASE_CUBIC_INOUT_TYPE, 500, 100,
            "sx", 0.5, 0.75, "sy", 0.5, 0.75, NULL);
    List *l;
    struct showone *one;
    LIST_FOR_EACH(planet->shines, l, one) {
        _nemoshow_item_motion_bounce(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150,
                "sx", 0.5, 0.75, "sy", 0.5, 0.75, NULL);
    }
}

void planet_up(Planet *planet)
{
    _nemoshow_item_motion_bounce(planet->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0, NULL);
    _nemoshow_item_motion_bounce(planet->ring0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 100,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0, NULL);
    _nemoshow_item_motion_bounce(planet->ring1, NEMOEASE_CUBIC_INOUT_TYPE, 500, 50,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0, NULL);
    List *l;
    struct showone *one;
    LIST_FOR_EACH(planet->shines, l, one) {
        _nemoshow_item_motion_bounce(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0, NULL);
    }
}

static void _planet_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    Planet *planet = userdata;
    PlanetView *view = planet->view;
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;

    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
		uint32_t grab_time = nemoshow_event_get_time(event);
		if (grab_time - planet->grab_time >= PLANET_GRAB_MIN_TIME) {
			planet->grab_time = nemoshow_event_get_time(event);
            Planet *dup = planet_dup(planet);
            planet_show(dup, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
			nemowidget_grab_set_data(grab, "item", dup);
		}
    } else if (nemoshow_event_is_motion(show, event)) {
        Planet *dup = nemowidget_grab_get_data(grab, "item");
		if (!dup) return;
        planet_translate(dup, 0, 0, 0, ex, ey);
    } else if (nemoshow_event_is_up(show, event)) {
        Planet *dup = nemowidget_grab_get_data(grab, "item");
		if (!dup) return;
        planet_hide_destroy(dup);

        bool over = false;
        List *l;
        Planet *_planet;
        LIST_FOR_EACH(view->planets, l, _planet) {
            int x, y, w, h;
            x = nemoshow_item_get_translate_x(_planet->group);
            y = nemoshow_item_get_translate_y(_planet->group);
            w = nemoshow_item_get_width(_planet->ring1);
            h = nemoshow_item_get_height(_planet->ring1);
            if (RECTS_CROSS(x - w/2, y - h/2, w, h, ex, ey, w, h)) {
                over = true;
                break;
            }
        }
        if (!over) {
            char args[PATH_MAX];
            snprintf(args, PATH_MAX, "--width;%d;--height;%d;--planet;%s",
                    (int)(636 * view->app->sxy), (int)(636 * view->app->sxy),
                    planet->uri);
            nemo_execute(view->uuid, "app", "/usr/bin/nemouniverse-menu", args, "off",
                    ex, ey, 0.0, 1.0, 1.0);
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _planet_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    //PlanetView *view = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            Planet *planet = nemoshow_one_get_userdata(one);
            nemowidget_create_grab(widget, event,
                    _planet_grab_event, planet);
        } else {
            struct nemotool *tool = nemowidget_get_tool(widget);
            uint64_t device = nemoshow_event_get_device(event);
            float vx, vy;
            nemoshow_transform_to_viewport(show, ex, ey, &vx, &vy);
            nemotool_touch_bypass(tool, device, vx, vy);
        }
    }
}

static void _planet_viewplanet_timeout(struct nemotimer *timer, void *userdata)
{
    PlanetView *view = userdata;
    List *l;
    Planet *planet;
    LIST_FOR_EACH(view->planets, l, planet) {
        planet->position_t += 0.00025;
        if (planet->position_t >= 1.0) planet->position_t = 0.0;

        if (planet->position_t >= 0.5) {
            nemoshow_one_above_one(planet->group, view->sun_anim->group);
        } else if (planet->position_t >= 0.0) {
            nemoshow_one_below_one(planet->group, view->sun_anim->group);
        }
        double px, py, tx, ty;
        nemoshow_item_path_get_position(view->orbit[planet->orbit_idx],
                planet->position_t, &px, &py, &tx, &ty);
        planet_translate(planet, 0, 0, 0, px, py);

    }
    nemotimer_set_timeout(timer, 100);
}

PlanetView *planet_viewcreate(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    PlanetView *view = calloc(sizeof(PlanetView), 1);
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->width = width;
    view->height = height;
    view->app = app;

    NemoWidget *widget;
    struct showone *one;
    struct showone *group;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _planet_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    int i;

    int iw, ih;
    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, UNIVERSE_IMG_DIR"/sun/%05d.png", 0);
    file_get_image_wh(buf, &iw, &ih);
    iw *= app->sxy;
    ih *= app->sxy;

    int backline_cnt = 0;
    List *files = fileinfo_readdir(app->backline_path);
    FileInfo *file;
    LIST_FREE(files, file) {
        if (fileinfo_is_svg(file)) {
            view->orbit[backline_cnt] = one = SVG_PATH_CREATE(group, width, height/1.5, file->path);
            nemoshow_item_set_fill_color(one, RGBA(WHITE));
            nemoshow_item_set_anchor(one, 0.5, 0.5);
            nemoshow_item_translate(one, width/2, height/2);
            backline_cnt++;
        }
        fileinfo_destroy(file);
    }

    Animation *anim;
    view->sun_anim = anim = animation_create(view->tool, group, iw, ih);
    animation_set_fps(anim, 10);
    animation_translate(anim, 0, 0, 0, width/2, height/2);

    i = 0;
    do {
        snprintf(buf, PATH_MAX, UNIVERSE_IMG_DIR"/sun/%05d.png", i++);
        if (!file_is_exist(buf)) break;
        animation_append_item(anim, buf);
    } while (1);
    animation_set_anchor(anim, 0.5, 0.5);


    files = fileinfo_readdir(UNIVERSE_IMG_DIR"/planet");
    int cnt = 0;
    List *l;
    LIST_FOR_EACH(files, l, file) {
        if (file->is_dir) continue;
        cnt++;
    }

    int ran = 0;
    double gap = 1.0/cnt;
    double start = 0.0;
    LIST_FREE(files, file) {
        if (file->is_dir) {
            fileinfo_destroy(file);
            continue;
        }
        Planet *planet = planet_create(view, app, file->path);
        planet->orbit_idx = ran;
        planet->position_t = start;
        view->planets = list_append(view->planets, planet);
        start += gap;
        ran++;
        if (ran >= backline_cnt) ran = 0;
        fileinfo_destroy(file);
    }
    view->planet_timer = TOOL_ADD_TIMER(view->tool, 0, _planet_viewplanet_timeout, view);

    return view;
}

void planet_viewshow(PlanetView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    int i = 0;
    List *l;
    Planet *planet;
    LIST_FOR_EACH(view->planets, l, planet) {
        planet_show(planet, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay  + i * 250);
        i++;
    }
    animation_show(view->sun_anim, NEMOEASE_CUBIC_INOUT_TYPE, duration, delay);
    nemotimer_set_timeout(view->planet_timer, 100);

    nemoshow_dispatch_frame(view->show);
}

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
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

    char buf[PATH_MAX];
    const char *temp;

    int width, height;
    snprintf(buf, PATH_MAX, "%s/size", appname);
    temp = xml_get_value(xml, buf, "width");
    if (!temp) {
        ERR("No size width in %s", appname);
    } else {
        width = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "height");
    if (!temp) {
        ERR("No size height in %s", appname);
    } else {
        height = atoi(temp);
    }

    double sx = 1.0;
    double sy = 1.0;
    if (width > 0) sx = (double)app->config->width/width;
    if (width > 0) sy = (double)app->config->height/height;
    if (sx > sy) app->sxy = sy;
    else app->sxy = sx;

    xml_unload(xml);

    int id = 0;
    struct option options[] = {
        {"backline_id", required_argument, NULL, 'b'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "b", options, NULL)) != -1) {
        switch(opt) {
            case 'b':
                id = atoi(optarg);
                break;
            default:
                break;
        }
    }

    app->backline_path = strdup_printf("%s/backline%d", UNIVERSE_ICON_DIR, id);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->backline_path) free(app->backline_path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "underlay");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    PlanetView *view = planet_viewcreate(win, app->config->width, app->config->height, app);
    planet_viewshow(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 2000);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
