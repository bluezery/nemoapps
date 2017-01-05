#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemoplay.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

#define FPS_TIMEOUT 1000/60

typedef struct _MovieBox MovieBox;

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    char *type;
    Config *config;
    double sxy;
    List *groups;
};

typedef struct _MenuGroup MenuGroup;
struct _MenuGroup {
    int id;
    char *mov_dir;
    List *items;
    MovieBox *box_click;
    MovieBox *box_click_effect;
    MovieBox *box_down;
    MovieBox *box_effect0;
    MovieBox *box_effect1;
    MovieBox *box_show;
};

typedef struct _MenuItem MenuItem;
struct _MenuItem {
    int group_id;
    char *type;
    char *mov_dir;
    char *exec;
    double sxy;
    bool resize;
    MovieBox *box_down;
    MovieBox *box_effect;
    MovieBox *box_show;
};

static MenuItem *parse_tag_item(XmlTag *tag)
{
    MenuItem *item = calloc(sizeof(MenuItem), 1);
    item->sxy = 1.0;
    item->resize = true;

    List *l;
    XmlAttr *attr;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, "group")) {
            if (attr->val) {
                 item->group_id = atoi(attr->val);
            }
        } else if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                 item->type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "mov")) {
            if (attr->val) {
                 item->mov_dir = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "exec")) {
            if (attr->val) {
                item->exec = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "sxy")) {
            if (attr->val) {
                item->sxy = atof(attr->val);
            }
        } else if (!strcmp(attr->key, "resize")) {
            if (attr->val) {
                if (!strcmp(attr->val, "on")) {
                    item->resize = true;
                } else {
                    item->resize = false;
                }
            }
        }
    }
    if (!item->type) item->type = strdup("xapp");
    return item;
}

static MenuGroup *parse_tag_group(XmlTag *tag)
{
    MenuGroup *grp = calloc(sizeof(MenuItem), 1);

    List *l;
    XmlAttr *attr;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, "id")) {
            if (attr->val) {
                grp->id = atoi(attr->val);
            }
        } else if (!strcmp(attr->key, "mov")) {
            if (attr->val) {
                grp->mov_dir = strdup(attr->val);
            }
        }
    }
    return grp;
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

    double sx = 1.0;
    double sy = 1.0;
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
    if (width > 0) sx = (double)app->config->width/width;
    if (height > 0) sy = (double)app->config->height/height;
    if (sx > sy) app->sxy = sy;
    else app->sxy = sx;

    List *tags;
    tags = xml_search_tags(xml, APPNAME"/group");
    List *l;
    XmlTag *tag;
    LIST_FOR_EACH(tags, l, tag) {
        MenuGroup *grp = parse_tag_group(tag);
        if (!grp) continue;
        app->groups = list_append(app->groups, grp);
    }

    tags = xml_search_tags(xml, APPNAME"/group/item");
    LIST_FOR_EACH(tags, l, tag) {
        MenuItem *it = parse_tag_item(tag);
        if (!it) continue;
        List *ll;
        MenuGroup *grp;
        LIST_FOR_EACH(app->groups, ll, grp) {
            if (grp->id == it->group_id) {
                grp->items = list_append(grp->items, it);
                break;
            }
        }
    }

    xml_unload(xml);

    struct option options[] = {
        {"type", required_argument, NULL, 't'},
        { NULL }
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "t:", options, NULL)) != -1) {
        switch(opt) {
            case 't':
                app->type = strdup(optarg);
                break;
            default:
                break;
        }
    }
    if (!app->type) app->type = strdup("table");

    return app;
}

static void _config_unload(ConfigApp *app)
{
    MenuGroup *grp;
    LIST_FREE(app->groups, grp) {
        free(grp->mov_dir);
        free(grp);
        MenuItem *it;
        LIST_FREE(grp->items, it) {
            free(it->type);
            free(it->mov_dir);
            free(it->exec);
            free(it);
        }
    }

    config_unload(app->config);
    free(app->type);
    free(app);
}

struct _MovieBox {
    int width, height;
    char *path;
    struct playbox *box;
    int cnt;
    int pixel_format;
};

List *__movie_boxes = NULL;

// FIXME: consider asynchronous later
MovieBox *movie_box_create(const char *path)
{
    if (!file_is_exist(path)) {
        ERR("file does not exist: %s", path);
        return NULL;
    }

    MovieBox *mbox = calloc(sizeof(MovieBox), 1);
    mbox->path = strdup(path);

    // XXX: caches
    List *l;
    MovieBox *_mbox;
    LIST_FOR_EACH(__movie_boxes, l, _mbox) {
        if (!strcmp(path, _mbox->path)) {
            break;
        }
    }
    if (_mbox) {
        mbox->box = _mbox->box;
        mbox->pixel_format = _mbox->pixel_format;
        mbox->width = _mbox->width;
        mbox->height = _mbox->height;
    } else {
        struct nemoplay *play;
        play = nemoplay_create();
        nemoplay_load_media(play, path);

        struct playbox *box;
        mbox->box = box = nemoplay_box_create(nemoplay_get_video_framecount(play));
        if (!box) {
            ERR("No box!!: %s", path);
            nemoplay_destroy(play);
            free(mbox);
            return NULL;
        }
        nemoplay_extract_video(play, box, nemoplay_get_video_framecount(play));

        mbox->cnt = nemoplay_box_get_count(box);

        mbox->pixel_format = nemoplay_get_pixel_format(play);
        mbox->width = nemoplay_get_video_width(play);
        mbox->height = nemoplay_get_video_height(play);
        nemoplay_destroy(play);
    }

    return mbox;
}

void movie_box_destroy(MovieBox *mbox)
{
    nemoplay_box_destroy(mbox->box);
    free(mbox->path);
    free(mbox);
}

typedef struct _Movie Movie;
typedef void (*MovieAfter)(Movie *movie, void *userdata);
struct _Movie {
    int width, height;
    NemoWidget *widget;
    struct playshader *shader;

    struct nemotool *tool;
    MovieBox *mbox;
    bool repeat;
    int idx;
    struct showtransition *trans;

    struct nemotimer *after_timer;
    void *after_userdata;
    MovieAfter after;
};

static void _movie_resize(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Resize *resize = info;
    Movie *movie = userdata;
    if (!movie->mbox) return;

    nemoplay_shader_set_viewport(movie->shader,
            nemowidget_get_texture(movie->widget),
            resize->width, resize->height);

    if (movie->mbox) {
        struct playone *one;
        one = nemoplay_box_get_one(movie->mbox->box, movie->idx);
        if (!one) {
            ERR("nemoplay_box_get_one failed: %d", movie->idx);
        } else {
            nemoplay_shader_update(movie->shader, one);
            nemoplay_shader_dispatch(movie->shader);
            nemowidget_dirty(movie->widget);
            struct nemoshow *show = nemowidget_get_show(widget);
            nemoshow_dispatch_frame(show);
        }
    }
}

static int _movie_frame_rev(void *userdata, uint32_t time, double t)
{
    Movie *movie = userdata;
    struct playone *one;
	one = nemoplay_box_get_one(movie->mbox->box, movie->idx);
    if (!one) {
        ERR("nemoplay_box_get_one failed: %d", movie->idx);
        movie->trans = NULL;
        return 1;
    }

    //ERR("%s, %d, %lf, (%d)", movie->mbox->path, time, t, movie->idx);
    nemoplay_shader_update(movie->shader, one);
    nemoplay_shader_dispatch(movie->shader);
    nemowidget_dirty(movie->widget);
    struct nemoshow *show = nemowidget_get_show(movie->widget);
    nemoshow_dispatch_frame(show);

    if (movie->idx > 0) {
        movie->idx--;
    } else {
        if (movie->repeat) {
            movie->idx = movie->mbox->cnt - 1;
        } else {
            movie->trans = NULL;
            return 1;
        }
    }

    return 0;
}

static int _movie_frame(void *userdata, uint32_t time, double t)
{
    Movie *movie = userdata;
    struct playone *one;
	one = nemoplay_box_get_one(movie->mbox->box, movie->idx);
    if (!one) {
        ERR("nemoplay_box_get_one failed: %d", movie->idx);
        movie->trans = NULL;
        return 1;
    }

    //ERR("%s, %d, %lf, (%d)", movie->mbox->path, time, t, movie->idx);
    nemoplay_shader_update(movie->shader, one);
    nemoplay_shader_dispatch(movie->shader);
    nemowidget_dirty(movie->widget);
    struct nemoshow *show = nemowidget_get_show(movie->widget);
    nemoshow_dispatch_frame(show);

    if (movie->idx >= 0) {
        movie->idx++;
        if (movie->idx >= movie->mbox->cnt) {
            if (movie->repeat) {
                movie->idx = 0;
            } else {
                movie->idx = movie->mbox->cnt - 1;
                movie->trans = NULL;
                return 1;
            }
        }
    }

    return 0;
}

static int _movie_frame_rewind(void *userdata, uint32_t time, double t)
{
    Movie *movie = userdata;
    struct playone *one;
	one = nemoplay_box_get_one(movie->mbox->box, movie->idx);
    if (!one) {
        ERR("nemoplay_box_get_one failed: %d", movie->idx);
        movie->trans = NULL;
        return 1;
    }

    nemoplay_shader_update(movie->shader, one);
    nemoplay_shader_dispatch(movie->shader);
    nemowidget_dirty(movie->widget);
    struct nemoshow *show = nemowidget_get_show(movie->widget);
    nemoshow_dispatch_frame(show);

    if (movie->idx > 0) {
        movie->idx--;
    } else {
        movie->trans = NULL;
        return 1;
    }

    return 0;
}

void movie_rotate(Movie *movie, double ro)
{
    struct showone *canvas = nemowidget_get_canvas(movie->widget);
    nemoshow_canvas_rotate(canvas, ro);
}

void movie_translate(Movie *movie, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    nemowidget_translate(movie->widget, easetype, duration, delay, tx, ty);
}

void movie_destroy(Movie *movie)
{
    if (movie->trans) nemoshow_transition_destroy(movie->trans);
    if (movie->after_timer) nemotimer_destroy(movie->after_timer);
    movie->after_timer = NULL;
    nemoplay_shader_destroy(movie->shader);
    nemowidget_destroy(movie->widget);
    free(movie);
}

static void _movie_after_timeout(struct nemotimer *timer, void *userdata)
{
    Movie *movie = userdata;
    RET_IF(!movie->after);
    RET_IF(!movie->after_timer);

    movie->after(movie, movie->after_userdata);

    nemotimer_destroy(timer);
    movie->after = NULL;
    movie->after_userdata = NULL;
    movie->after_timer = NULL;
}

void movie_add_after(Movie *movie, MovieAfter after, void *userdata)
{
    if (!movie->mbox->cnt) return;
    movie->after = after;
    movie->after_userdata = userdata;

    if (movie->after_timer) nemotimer_destroy(movie->after_timer);
    uint32_t time = movie->mbox->cnt * (1000/60);
    movie->after_timer = TOOL_ADD_TIMER(movie->tool, time,
            _movie_after_timeout, movie);
}

Movie *movie_create(NemoWidget *parent, int width, int height, MovieBox *mbox)
{
    Movie *movie = calloc(sizeof(Movie), 1);
    movie->tool = nemowidget_get_tool(parent);
    movie->width = width;
    movie->height = height;
    movie->mbox = mbox;

    movie->shader = nemoplay_shader_create();
    nemoplay_shader_set_format(movie->shader,
            mbox->pixel_format);
    nemoplay_shader_resize(movie->shader,
            mbox->width, mbox->height);

    NemoWidget *widget;
    movie->widget = widget = nemowidget_create_opengl(parent, width, height);
    nemowidget_append_callback(widget, "resize", _movie_resize, movie);
    nemowidget_set_alpha(movie->widget, 0, 0, 0, 0.0);

    return movie;
}

void movie_rewind(Movie *movie)
{
    movie->repeat = false;

    struct nemoshow *show = nemowidget_get_show(movie->widget);
    if (movie->trans) nemoshow_transition_destroy(movie->trans);
    struct showtransition *trans;
    movie->trans = trans = nemoshow_transition_create(NEMOEASE_LINEAR_TYPE, 0, 0);
    nemoshow_transition_set_dispatch_frame(trans, _movie_frame_rewind);
    nemoshow_transition_set_repeat(trans, 0);
    nemoshow_transition_set_userdata(trans, movie);
    nemoshow_attach_transition(show, trans);

    nemoshow_dispatch_frame(show);
}

static void _movie_destroy(struct nemotimer *timer, void *userdata)
{
    Movie *movie = userdata;
    movie_destroy(movie);
    nemotimer_destroy(timer);
}

void movie_rewind_destroy(Movie *movie)
{
    movie_rewind(movie);
    // xxx: do not use movie_add_after
    // it will be broken after callback call
    uint32_t time = movie->mbox->cnt * (1000/60);
    TOOL_ADD_TIMER(movie->tool, time, _movie_destroy, movie);
}

void movie_play_rev(Movie *movie, MovieBox *mbox, bool repeat)
{
    movie->mbox = mbox;
    movie->repeat = repeat;
    movie->idx = mbox->cnt - 1;
    nemoplay_shader_set_format(movie->shader,
            mbox->pixel_format);
    nemoplay_shader_resize(movie->shader,
            mbox->width, mbox->height);

    struct nemoshow *show = nemowidget_get_show(movie->widget);
    if (movie->trans) nemoshow_transition_destroy(movie->trans);
    struct showtransition *trans;
    movie->trans = trans = nemoshow_transition_create(NEMOEASE_LINEAR_TYPE, 0, 0);
    nemoshow_transition_set_dispatch_frame(trans, _movie_frame_rev);
    nemoshow_transition_set_repeat(trans, 0);
    nemoshow_transition_set_userdata(trans, movie);
    nemoshow_attach_transition(show, trans);

    nemoshow_dispatch_frame(show);
}

void movie_show(Movie *movie)
{
    nemowidget_show(movie->widget, 0, 0, 0);
    nemowidget_set_alpha(movie->widget, 0, 0, 0, 1.0);
}

void movie_hide(Movie *movie)
{
    nemowidget_hide(movie->widget, 0, 0, 0);
    nemowidget_set_alpha(movie->widget, 0, 0, 0, 0.0);
}

void movie_play(Movie *movie, MovieBox *mbox, bool repeat)
{
    movie->mbox = mbox;
    movie->repeat = repeat;
    movie->idx = 0;
    nemoplay_shader_set_format(movie->shader,
            mbox->pixel_format);
    nemoplay_shader_resize(movie->shader,
            mbox->width, mbox->height);

    struct nemoshow *show = nemowidget_get_show(movie->widget);
    if (movie->trans) nemoshow_transition_destroy(movie->trans);
    struct showtransition *trans;
    movie->trans = trans = nemoshow_transition_create(NEMOEASE_LINEAR_TYPE, 0, 0);
    nemoshow_transition_set_dispatch_frame(trans, _movie_frame);
    nemoshow_transition_set_repeat(trans, 0);
    nemoshow_transition_set_userdata(trans, movie);
    nemoshow_attach_transition(show, trans);

    nemoshow_dispatch_frame(show);
}

void movie_play_destroy(Movie *movie, MovieBox *mbox, bool repeat)
{
    ERR("");
    movie_play(movie, mbox, repeat);
    // XXX: do not use movie_add_after
    // it will be broken after callback call
    uint32_t time = movie->mbox->cnt * (1000/60);
    TOOL_ADD_TIMER(movie->tool, time,
            _movie_destroy, movie);
}

typedef struct _LauncherView LauncherView;
typedef struct _LauncherGrab LauncherGrab;
struct _LauncherGrab
{
    struct nemotool *tool;
    Movie *movie;
    LauncherView *view;
    int ex, ey;

    struct nemotimer *timer;
};

struct _LauncherView {
    ConfigApp *app;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    const char *uuid;

    NemoWidget *widget;
    struct showone *group;
    struct showone *bg;

    MovieBox *box_main_bg;
    MovieBox *box_sub_bg;
    MovieBox *box_feedback0;
    MovieBox *box_feedback1;
    MovieBox *box_feedback2;
    MovieBox *box_feedback3;
    MovieBox *box_longpress;

    List *grabs;
    List *launchers;
};

typedef struct _LauncherItemSub LauncherItemSub;
typedef struct _LauncherItem LauncherItem;
typedef struct _Launcher Launcher;

struct _LauncherItemSub {
    Launcher *launcher;
    int width, height;

    MenuItem *menu_it;
    Movie *movie;
    struct nemotimer *delay_timer;
};

struct _LauncherItem {
    Launcher *launcher;
    int width, height;

    MenuGroup *grp;
    Movie *movie;

    List *items;
    struct nemotimer *delay_timer;
};

struct _Launcher {
    LauncherView *view;
    int x, y;
    int width, height;

    Movie *main_bg;
    Movie *sub_bg;
    List *items;
    LauncherItem *it_cur;

    struct nemotimer *destroy_timer;
};

void launcher_item_rotate(LauncherItem *it, double ro)
{
    movie_rotate(it->movie, ro);
}

void launcher_item_translate(LauncherItem *it, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    movie_translate(it->movie, easetype, duration, delay, tx , ty);
}

static void _launcher_item_grab_up_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_play(it->movie, it->grp->box_effect1, true);
}

static void _launcher_item_click_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_play(it->movie, it->grp->box_click_effect, true);
}

static void _launcher_item_unclick_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_play(it->movie, it->grp->box_effect1, true);
}

void _launcher_item_sub_show_done(Movie *movie, void *userdata)
{
    LauncherItemSub *sub = userdata;
    movie_play(sub->movie, sub->menu_it->box_effect, true);
}

static void _launcher_item_sub_show(LauncherItemSub *sub)
{
    movie_show(sub->movie);
    movie_play(sub->movie, sub->menu_it->box_show, false);
    movie_add_after(sub->movie, _launcher_item_sub_show_done, sub);
}

static void _item_sub_show_delay_timeout(struct nemotimer *timer, void *userdata)
{
    LauncherItemSub *sub = userdata;
    _launcher_item_sub_show(sub);

    nemotimer_destroy(sub->delay_timer);
    sub->delay_timer = NULL;
}

void launcher_item_sub_show(LauncherItemSub *sub, int delay)
{
    if (sub->delay_timer) {
        nemotimer_destroy(sub->delay_timer);
        sub->delay_timer = NULL;
    }
    if (delay > 0) {
        sub->delay_timer = TOOL_ADD_TIMER(sub->launcher->view->tool, delay,
                _item_sub_show_delay_timeout, sub);
    } else {
        _launcher_item_sub_show(sub);
    }
}

void _launcher_item_sub_hide_done(Movie *movie, void *userdata)
{
    LauncherItemSub *sub = userdata;
    movie_hide(sub->movie);
}

void launcher_item_sub_hide(LauncherItemSub *sub)
{
    movie_play_rev(sub->movie, sub->menu_it->box_show, false);
    movie_add_after(sub->movie, _launcher_item_sub_hide_done, sub);
}

void launcher_item_sub_translate(LauncherItemSub *sub, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    movie_translate(sub->movie, easetype, duration, delay, tx , ty);
}

static void _launcher_item_sub_grab_up_done(Movie *movie, void *userdata)
{
    LauncherItemSub *sub = userdata;
    movie_play(sub->movie, sub->menu_it->box_effect, true);
}

static void launcher_item_sub_exec(LauncherItemSub *sub)
{
    Launcher *launcher = sub->launcher;
    LauncherView *view = launcher->view;

    MenuItem *it = sub->menu_it;

    double x, y;
    nemowidget_get_geometry(sub->movie->widget, &x, &y, NULL, NULL);
    x = x + sub->width + 10;

    double ro = 0.0;

    // FIXME: not working
    struct nemoshow *show = launcher->view->show;
    nemoshow_view_set_anchor(show, 1.0, 0.0);

    char path[PATH_MAX];
    char name[PATH_MAX];
    char args[PATH_MAX];
    char *buf;
    char *tok;
    buf = strdup(it->exec);
    tok = strtok(buf, ";");
    snprintf(name, PATH_MAX, "%s", tok);
    snprintf(path, PATH_MAX, "%s", tok);
    tok = strtok(NULL, "");
    snprintf(args, PATH_MAX, "%s", tok);
    free(buf);

    struct nemobus *bus = NEMOBUS_CREATE();
    struct busmsg *msg = NEMOMSG_CREATE_CMD(it->type, path);
    nemobus_msg_set_attr(msg, "owner", view->uuid);
    nemobus_msg_set_attr(msg, "args", args);
    nemobus_msg_set_attr(msg, "resize", it->resize? "on" : "off");
    nemobus_msg_set_attr_format(msg, "x", "%f", x);
    nemobus_msg_set_attr_format(msg, "y", "%f", y);
    nemobus_msg_set_attr_format(msg, "r", "%f", ro);
    nemobus_msg_set_attr_format(msg, "sx", "%f", it->sxy);
    nemobus_msg_set_attr_format(msg, "sy", "%f", it->sxy);
    // FIXME: only xapp can use dx, dy
    nemobus_msg_set_attr_format(msg, "dx", "%f", 0.0);
    nemobus_msg_set_attr_format(msg, "dy", "%f", 0.0);
    NEMOMSG_SEND(bus, msg);
}


static void _launcher_item_sub_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    LauncherItemSub *sub = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        movie_play(sub->movie, sub->menu_it->box_down, false);
    } else if (nemoshow_event_is_up(show, event)) {
        movie_rewind(sub->movie);
        movie_add_after(sub->movie, _launcher_item_sub_grab_up_done, sub);
        if (nemoshow_event_is_single_click(show, event)) {
            launcher_item_sub_exec(sub);
        }
    }
}

static void _launcher_item_sub_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    LauncherItemSub *sub = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        nemowidget_create_grab(widget, event, _launcher_item_sub_grab_event, sub);
    }
}

void launcher_item_sub_destroy(LauncherItemSub *sub)
{
    if (sub->delay_timer) nemotimer_destroy(sub->delay_timer);
    movie_destroy(sub->movie);
    free(sub);
}

LauncherItemSub *launcher_item_sub_create(Launcher *launcher, MenuItem *menu_it)
{
    int w, h ;
    LauncherItemSub *sub = calloc(sizeof(LauncherItemSub), 1);
    sub->launcher = launcher;
    sub->width = w = menu_it->box_show->width;
    sub->height = h = menu_it->box_show->height;
    sub->menu_it = menu_it;
    sub->movie = movie_create(launcher->view->widget, w, h, sub->menu_it->box_show);

    nemowidget_append_callback(sub->movie->widget, "event", _launcher_item_sub_event, sub);

    return sub;
}

static void _launcher_item_hide_done_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_hide(it->movie);
}

static void _launcher_item_hide_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_play_rev(it->movie, it->grp->box_show, false);
    movie_add_after(it->movie, _launcher_item_hide_done_done, it);
}

void _launcher_item_hide(LauncherItem *it)
{
    LauncherItemSub *sub;
    sub = LIST_DATA(LIST_LAST(it->items));
    movie_add_after(sub->movie, _launcher_item_hide_done, it);

    List *l;
    LIST_FOR_EACH(it->items, l, sub) {
        launcher_item_sub_hide(sub);
    }
}

static void _item_hide_delay_timeout(struct nemotimer *timer, void *userdata)
{
    LauncherItem *it = userdata;
    _launcher_item_hide(it);

    nemotimer_destroy(it->delay_timer);
    it->delay_timer = NULL;
}

void launcher_item_hide(LauncherItem *it, int delay)
{
    if (it->delay_timer) {
        nemotimer_destroy(it->delay_timer);
        it->delay_timer = NULL;
    }
    if (delay > 0) {
        it->delay_timer = TOOL_ADD_TIMER(it->launcher->view->tool, delay,
                _item_hide_delay_timeout, it);
    } else {
        _launcher_item_hide(it);
    }
}

static void _launcher_item_show_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    movie_play(it->movie, it->grp->box_effect1, true);
}

void _launcher_item_show(LauncherItem *it)
{
    movie_show(it->movie);
    movie_play(it->movie, it->grp->box_show, false);
    movie_add_after(it->movie, _launcher_item_show_done, it);
}

static void _item_show_delay_timeout(struct nemotimer *timer, void *userdata)
{
    LauncherItem *it = userdata;
    _launcher_item_show(it);

    nemotimer_destroy(it->delay_timer);
    it->delay_timer = NULL;
}

void launcher_item_show(LauncherItem *it, int delay)
{
    if (it->delay_timer) {
        nemotimer_destroy(it->delay_timer);
        it->delay_timer = NULL;
    }
    if (delay > 0) {
        it->delay_timer = TOOL_ADD_TIMER(it->launcher->view->tool, delay,
                _item_show_delay_timeout, it);
    } else {
        _launcher_item_show(it);
    }
}

static void _launcher_sub_hide_done(Movie *moview, void *userdata)
{
    LauncherItem *it = userdata;
    movie_hide(it->launcher->sub_bg);
}

void launcher_sub_hide(LauncherItem *it)
{
    Launcher *launcher = it->launcher;

    movie_play_rev(launcher->sub_bg, launcher->view->box_sub_bg, false);
    movie_add_after(launcher->sub_bg, _launcher_sub_hide_done, it);

    List *l;
    LauncherItemSub *sub;
    LIST_FOR_EACH(it->items, l, sub) {
        launcher_item_sub_hide(sub);
    }

}

static void _launcher_sub_show_done(Movie *movie, void *userdata)
{
    LauncherItem *it = userdata;
    Launcher *launcher = it->launcher;

    int gap = 10;

    int i = 0;
    List *l;
    LauncherItemSub *sub;
    LIST_FOR_EACH(it->items, l, sub) {
        int x, y;
        x = launcher->x + launcher->width + (launcher->width - it->width)/2;
        y = launcher->y + gap + (gap + it->height) * i;
        launcher_item_sub_translate(sub, 0, 0, 0, x, y);
        launcher_item_sub_show(sub, 100 * i);
        i++;
    }
}

void launcher_sub_show(LauncherItem *it)
{
    Launcher *launcher = it->launcher;

    movie_show(launcher->sub_bg);
    movie_play(launcher->sub_bg, launcher->view->box_sub_bg, false);
    movie_add_after(launcher->sub_bg, _launcher_sub_show_done, it);
}
static void _launcher_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    LauncherItem *it = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        movie_play(it->movie, it->grp->box_down, false);
    } else if (nemoshow_event_is_up(show, event)) {
        if (nemoshow_event_is_single_click(show, event)) {
            List *l;
            LauncherItem *itt;
            LIST_FOR_EACH(it->launcher->items, l, itt) {
                if (itt->movie->mbox == itt->grp->box_click ||
                    itt->movie->mbox == itt->grp->box_click_effect) {
                    movie_play_rev(itt->movie, itt->grp->box_click, false);
                    movie_add_after(itt->movie, _launcher_item_unclick_done, itt);
                }
            }

            movie_play(it->movie, it->grp->box_click, false);
            movie_add_after(it->movie, _launcher_item_click_done, it);

            if (it->launcher->it_cur) {
                launcher_sub_hide(it->launcher->it_cur);
            }
            launcher_sub_show(it);
            it->launcher->it_cur = it;
            /*
            int i = 1;
            List *l;
            LauncherItemSub *sub;
            LIST_FOR_EACH(it->items, l, sub) {
                int x, y;
                x = launcher->x + gap * i;
                y = launcher->y + (launcher->height - sub->height)/2;
                launcher_item_translate(sub, 0, 0, 0, x, y);
                movie_play(sub->movie, sub->grp->box_show, false);

                uint32_t time = sub->movie->mbox->cnt * (1000/60);
                TOOL_ADD_TIMER(sub->movie->tool, time, _launcher_item_show_done, sub);
                i++;
            }
            */

        } else {
            movie_rewind(it->movie);
            movie_add_after(it->movie, _launcher_item_grab_up_done, it);
        }
    }
}

static void _launcher_item_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    LauncherItem *it = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        nemowidget_create_grab(widget, event, _launcher_item_grab_event, it);
    }
}

void launcher_item_destroy(LauncherItem *it)
{
    if (it->delay_timer) nemotimer_destroy(it->delay_timer);
    LauncherItemSub *sub;
    LIST_FREE(it->items, sub) {
        launcher_item_sub_destroy(sub);
    }
    movie_destroy(it->movie);
    free(it);
}

LauncherItem *launcher_item_create(Launcher *launcher, MenuGroup *grp)
{
    int w, h ;
    LauncherItem *it = calloc(sizeof(LauncherItem), 1);
    it->launcher = launcher;
    it->width = w = grp->box_show->width;
    it->height = h = grp->box_show->height;
    it->grp = grp;
    it->movie = movie_create(launcher->view->widget, w, h, it->grp->box_show);

    nemowidget_append_callback(it->movie->widget, "event", _launcher_item_event, it);

    List *l;
    MenuItem *menu_it;
    LIST_FOR_EACH(grp->items, l, menu_it) {
        LauncherItemSub *sub = launcher_item_sub_create(launcher, menu_it);
        it->items = list_append(it->items, sub);
    }

    return it;
}

// XXX: should call before show
void launcher_translate(Launcher *launcher, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    movie_translate(launcher->main_bg, easetype, duration, delay, tx, ty);
    movie_translate(launcher->sub_bg, easetype, duration, delay,
            tx + launcher->width, ty);
    launcher->x = tx;
    launcher->y = ty;
}

static void _launcher_show_done(Movie *movie, void *userdata)
{
    Launcher *launcher = userdata;
    int gap = 10;

    int i = 0;
    List *l;
    LauncherItem *it;
    LIST_FOR_EACH(launcher->items, l, it) {
        int x, y;
        x = launcher->x + (launcher->width - it->width)/2;
        y = launcher->y + gap + (gap + it->height) * i;
        launcher_item_translate(it, 0, 0, 0, x, y);
        launcher_item_show(it, 150 * i);
        i++;
    }
}

void launcher_show(Launcher *launcher)
{
    movie_show(launcher->main_bg);
    movie_play(launcher->main_bg, launcher->view->box_main_bg, false);
    movie_add_after(launcher->main_bg, _launcher_show_done, launcher);
}

static void _launcher_hide_done_done(Movie *movie, void *userdata)
{
    Launcher *launcher = userdata;
    movie_hide(launcher->main_bg);
    movie_hide(launcher->sub_bg);
    LauncherItem *it;
    LIST_FREE(launcher->items, it) {
        launcher_item_destroy(it);
    }
    if (launcher->destroy_timer) nemotimer_destroy(launcher->destroy_timer);
    free(launcher);
}

void _launcher_hide_done(Movie *movie, void *userdata)
{
    Launcher *launcher = userdata;

    movie_play_rev(launcher->main_bg, launcher->view->box_main_bg, false);
    movie_add_after(launcher->main_bg, _launcher_hide_done_done, launcher);
}

void launcher_hide_destroy(Launcher *launcher)
{
    LauncherItem *it;
    it = LIST_DATA(LIST_LAST(launcher->items));
    movie_add_after(it->movie, _launcher_hide_done, launcher);

    int i = 0;
    List *l;
    LIST_FOR_EACH(launcher->items, l, it) {
        launcher_item_hide(it, 150 * i);
        i++;
    }

}

static void _launcher_destroy_timeout(struct nemotimer *timer, void *userdata)
{
    ERR("");
    Launcher *launcher = userdata;
    launcher_hide_destroy(launcher);
}


Launcher *launcher_create(LauncherView *view)
{
    int w, h;
    Launcher *launcher = calloc(sizeof(Launcher), 1);
    launcher->view = view;
    launcher->width = w = view->box_main_bg->width;
    launcher->height = h = view->box_main_bg->height;

    launcher->main_bg = movie_create(view->widget, w, h, launcher->view->box_main_bg);
    launcher->sub_bg = movie_create(view->widget, w, h, launcher->view->box_sub_bg);

    List *l;
    MenuGroup *grp;
    LIST_FOR_EACH(view->app->groups, l, grp) {
        LauncherItem *it = launcher_item_create(launcher, grp);
        launcher->items = list_append(launcher->items, it);
    }

    launcher->destroy_timer = TOOL_ADD_TIMER(view->tool, 5000,
            _launcher_destroy_timeout, launcher);

    return launcher;
}

typedef struct _Longpress Longpress;
struct _Longpress {
    LauncherView *view;
    float ex, ey;
    Movie *del_movie;
};

static void _longpress_done(struct nemotimer *timer, void *userdata)
{
    Longpress *longpress = userdata;
    LauncherView *view = longpress->view;
    movie_destroy(longpress->del_movie);

    Launcher *launcher;
    launcher = launcher_create(view);
    launcher_translate(launcher, 0, 0, 0,
            longpress->ex - view->box_main_bg->width/2, 0);
    launcher_show(launcher);

    view->launchers = list_append(view->launchers, launcher);
    free(longpress);
}

static void _longpress_timeout(struct nemotimer *timer, void *userdata)
{
    LauncherGrab *lgrab = userdata;
    Movie *movie = lgrab->movie;
    LauncherView *view = lgrab->view;

    nemotimer_destroy(timer);
    lgrab->timer = NULL;

    Longpress *longpress = calloc(sizeof(Longpress), 1);
    longpress->view = view;
    longpress->ex = lgrab->ex;
    longpress->ey = lgrab->ey;
    longpress->del_movie = movie;

    movie_play(movie, view->box_longpress, false);
    // xxx: do not use movie_add_after
    // it will be broken after callback call
    uint32_t time = movie->mbox->cnt * (1000/60);
    TOOL_ADD_TIMER(movie->tool, time, _longpress_done, longpress);
}

static void _launcherview_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);
    LauncherGrab *lgrab = userdata;
    Movie *movie = lgrab->movie;
    LauncherView *view = lgrab->view;

    double gx = nemoshow_event_get_grab_x(event);
    double gy = nemoshow_event_get_grab_y(event);

    if (nemoshow_event_is_down(show, event)) {
        int r = rand()%4;
        if (r == 0) {
            movie_play(movie, view->box_feedback0, true);
        } else if (r == 1) {
            movie_play(movie, view->box_feedback1, true);
        } else if (r == 2) {
            movie_play(movie, view->box_feedback2, true);
        } else {
            movie_play(movie, view->box_feedback3, true);
        }
        movie_translate(movie, 0, 0, 0, ex - movie->width/2, ey - movie->height/2);
    } else {
        if (lgrab->timer) {
            if (nemoshow_event_is_motion(show, event)) {
                if ((abs(gx - ex) > 20) || (abs(gy - ey) > 20)) {
                    nemotimer_set_timeout(lgrab->timer, 500);
                }
                lgrab->ex = ex;
                lgrab->ey = ey;
                movie_translate(movie, 0, 0, 0, ex - movie->width/2, ey - movie->height/2);
            } else if (nemoshow_event_is_up(show, event)) {
                movie_rewind_destroy(movie);
                nemotimer_destroy(lgrab->timer);
                view->grabs = list_remove(view->grabs, lgrab);
                free(lgrab);
            }
        } else {
            if (nemoshow_event_is_up(show, event)) {
                view->grabs = list_remove(view->grabs, lgrab);
                free(lgrab);
            }
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _launcherview_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);
    LauncherView *view = userdata;

    if (nemoshow_event_is_down(show, event)) {
        struct nemotool *tool = nemowidget_get_tool(widget);

        LauncherGrab *lgrab = calloc(sizeof(LauncherGrab), 1);
        lgrab->tool = nemowidget_get_tool(widget);
        lgrab->movie = movie_create(widget,
                view->box_feedback0->width, view->box_feedback0->height,
                view->box_feedback0);
        movie_show(lgrab->movie);
        lgrab->view = view;
        nemowidget_create_grab(widget, event, _launcherview_grab_event, lgrab);
        lgrab->timer = TOOL_ADD_TIMER(tool, 500, _longpress_timeout, lgrab);
        view->grabs = list_append(view->grabs, lgrab);
    }
}

LauncherView *launcherview_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    LauncherView *view = calloc(sizeof(LauncherView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->width = width;
    view->height = height;
    view->app = app;

    NemoWidget *widget;
    struct showone *one;
    struct showone *group;

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _launcherview_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 1.0);

    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    const char *main_bg_path, *sub_bg_path;
    const char *feedback0, *feedback1, *feedback2, *feedback3, *longpress;

    if (!strcmp(app->type, "wall")) {
        if (width == 2160) {
            main_bg_path = LAUNCHER_MOV_DIR"/bg/wall/4k/main.mov";
            sub_bg_path = LAUNCHER_MOV_DIR"/bg/wall/4k/sub.mov";
            feedback0 = LAUNCHER_MOV_DIR"/effect/4k/feedback0.mov";
            feedback1 = LAUNCHER_MOV_DIR"/effect/4k/feedback1.mov";
            feedback2 = LAUNCHER_MOV_DIR"/effect/4k/feedback2.mov";
            feedback3 = LAUNCHER_MOV_DIR"/effect/4k/feedback3.mov";
            longpress = LAUNCHER_MOV_DIR"/effect/4k/longpress.mov";

        } else {
            main_bg_path = LAUNCHER_MOV_DIR"/bg/wall/1k/main.mov";
            sub_bg_path = LAUNCHER_MOV_DIR"/bg/wall/1k/sub.mov";
            feedback0 = LAUNCHER_MOV_DIR"/effect/1k/feedback0.mov";
            feedback1 = LAUNCHER_MOV_DIR"/effect/1k/feedback1.mov";
            feedback2 = LAUNCHER_MOV_DIR"/effect/1k/feedback2.mov";
            feedback3 = LAUNCHER_MOV_DIR"/effect/1k/feedback3.mov";
            longpress = LAUNCHER_MOV_DIR"/effect/1k/longpress.mov";
        }
    } else {
        if (width == 2160) {
            main_bg_path = LAUNCHER_MOV_DIR"/bg/table/4k/main.mov";
            sub_bg_path = LAUNCHER_MOV_DIR"/bg/table/4k/sub.mov";
            feedback0 = LAUNCHER_MOV_DIR"/effect/4k/feedback0.mov";
            feedback1 = LAUNCHER_MOV_DIR"/effect/4k/feedback1.mov";
            feedback2 = LAUNCHER_MOV_DIR"/effect/4k/feedback2.mov";
            feedback3 = LAUNCHER_MOV_DIR"/effect/4k/feedback3.mov";
            longpress = LAUNCHER_MOV_DIR"/effect/4k/longpress.mov";
        } else {
            main_bg_path = LAUNCHER_MOV_DIR"/bg/table/1k/main.mov";
            sub_bg_path = LAUNCHER_MOV_DIR"/bg/table/1k/sub.mov";
            feedback0 = LAUNCHER_MOV_DIR"/effect/1k/feedback0.mov";
            feedback1 = LAUNCHER_MOV_DIR"/effect/1k/feedback1.mov";
            feedback2 = LAUNCHER_MOV_DIR"/effect/1k/feedback2.mov";
            feedback3 = LAUNCHER_MOV_DIR"/effect/1k/feedback3.mov";
            longpress = LAUNCHER_MOV_DIR"/effect/1k/longpress.mov";
        }
    }

    if (width == 2160) {
        List *l;
        MenuGroup *grp;
        LIST_FOR_EACH(app->groups, l, grp) {
            char buf[PATH_MAX];
            snprintf(buf, PATH_MAX, "%s/4k/click.mov", grp->mov_dir);
            grp->box_click = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/4k/click-effect.mov", grp->mov_dir);
            grp->box_click_effect = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/4k/down.mov", grp->mov_dir);
            grp->box_down = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/4k/effect0.mov", grp->mov_dir);
            grp->box_effect0 = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/4k/effect1.mov", grp->mov_dir);
            grp->box_effect1 = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/4k/show.mov", grp->mov_dir);
            grp->box_show = movie_box_create(buf);

            List *ll;
            MenuItem *it;
            LIST_FOR_EACH(grp->items, ll, it) {
                snprintf(buf, PATH_MAX, "%s/4k/down.mov", it->mov_dir);
                it->box_down = movie_box_create(buf);
                snprintf(buf, PATH_MAX, "%s/4k/effect.mov", it->mov_dir);
                it->box_effect = movie_box_create(buf);
                snprintf(buf, PATH_MAX, "%s/4k/show.mov", it->mov_dir);
                it->box_show = movie_box_create(buf);
            }
        }
    } else {
        List *l;
        MenuGroup *grp;
        LIST_FOR_EACH(app->groups, l, grp) {
            char buf[PATH_MAX];
            snprintf(buf, PATH_MAX, "%s/1k/click.mov", grp->mov_dir);
            grp->box_click = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/1k/click-effect.mov", grp->mov_dir);
            grp->box_click_effect = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/1k/down.mov", grp->mov_dir);
            grp->box_down = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/1k/effect0.mov", grp->mov_dir);
            grp->box_effect0 = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/1k/effect1.mov", grp->mov_dir);
            grp->box_effect1 = movie_box_create(buf);
            snprintf(buf, PATH_MAX, "%s/1k/show.mov", grp->mov_dir);
            grp->box_show = movie_box_create(buf);

            List *ll;
            MenuItem *it;
            LIST_FOR_EACH(grp->items, ll, it) {
                snprintf(buf, PATH_MAX, "%s/1k/down.mov", it->mov_dir);
                it->box_down = movie_box_create(buf);
                snprintf(buf, PATH_MAX, "%s/1k/effect.mov", it->mov_dir);
                it->box_effect = movie_box_create(buf);
                snprintf(buf, PATH_MAX, "%s/1k/show.mov", it->mov_dir);
                it->box_show = movie_box_create(buf);
            }
        }
    }

    view->box_main_bg = movie_box_create(main_bg_path);
    view->box_sub_bg = movie_box_create(sub_bg_path);
    view->box_feedback0 = movie_box_create(feedback0);
    view->box_feedback1 = movie_box_create(feedback1);
    view->box_feedback2 = movie_box_create(feedback2);
    view->box_feedback3 = movie_box_create(feedback3);
    view->box_longpress = movie_box_create(longpress);

    return view;
}

void launcherview_show(LauncherView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 1.0);
    nemoshow_dispatch_frame(view->show);
}

void launcherview_hide(LauncherView *view)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 0.0);
    nemoshow_dispatch_frame(view->show);
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

    LauncherView *view = launcherview_create(win, app->config->width, app->config->height, app);
    launcherview_show(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
