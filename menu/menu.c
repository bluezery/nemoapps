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

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    double sxy;
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
    if (width > 0) sy = (double)app->config->height/height;
    if (sx > sy) app->sxy = sy;
    else app->sxy = sx;

    xml_unload(xml);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    free(app);
}

typedef struct _AnimatorScene AnimatorScene;
struct _AnimatorScene {
    struct nemoplay *play;
    struct playbox *box;
};

void animator_scene_reverse(AnimatorScene *scene)
{
    struct playbox *box = scene->box;
    int i = 0;
    for (i = 0 ; i < box->nones ; i++) {
        if (i >= box->nones - i - 1) break;
        struct playone *temp;
        temp = box->ones[i];
        box->ones[i] = box->ones[box->nones - i - 1];
        box->ones[box->nones - i - 1] = temp;
    }
}

typedef struct _Animator Animator;
typedef void (*AnimatorDone)(Animator *anim, void *userdata);

struct _Animator {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;

    int repeat;

    struct playshader *shader;
    NemoWidget *widget;
    struct nemotimer *timer;
    struct nemotimer *timer_reverse;

    int box_idx;
    int scene_idx;
    List *scenes;
    int scene_idx_next;

    AnimatorDone done;
    void *done_userdata;
};

void animator_set_scene(Animator *anim, int idx)
{
    RET_IF(anim->scene_idx == idx);
    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, idx));
    RET_IF(!scene);

    anim->scene_idx = idx;

    nemoplay_shader_set_format(anim->shader,
            nemoplay_get_pixel_format(scene->play));
    nemoplay_shader_set_texture(anim->shader,
            nemoplay_get_video_width(scene->play),
            nemoplay_get_video_height(scene->play));

}

void animator_set_done(Animator *anim, AnimatorDone done, void *userdata)
{
    anim->done = done;
    anim->done_userdata = userdata;
}

void animator_set_repeat(Animator *anim, int repeat)
{
    anim->repeat = repeat;
}

void animator_play(Animator *anim)
{
    nemotimer_set_timeout(anim->timer, 10);
    nemotimer_set_timeout(anim->timer_reverse, 0);
}

void animator_play_reverse(Animator *anim)
{
    nemotimer_set_timeout(anim->timer, 0);
    nemotimer_set_timeout(anim->timer_reverse, 10);
}

void animator_play_scene(Animator *anim, int scene_idx)
{
    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, scene_idx));
    RET_IF(!scene);
    if (anim->scene_idx != scene_idx) {
        anim->scene_idx = scene_idx;
        nemoplay_shader_set_format(anim->shader,
                nemoplay_get_pixel_format(scene->play));
        nemoplay_shader_set_texture(anim->shader,
                nemoplay_get_video_width(scene->play),
                nemoplay_get_video_height(scene->play));

        anim->box_idx = 0;
    }
    animator_play(anim);
}

void animator_play_scene_reverse(Animator *anim, int scene_idx)
{
    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, scene_idx));
    RET_IF(!scene);
    if (anim->scene_idx != scene_idx) {
        anim->scene_idx = scene_idx;
        nemoplay_shader_set_format(anim->shader,
                nemoplay_get_pixel_format(scene->play));
        nemoplay_shader_set_texture(anim->shader,
                nemoplay_get_video_width(scene->play),
                nemoplay_get_video_height(scene->play));

        anim->box_idx = nemoplay_box_get_count(scene->box) - 1;
    }
    animator_play_reverse(anim);
}

int animator_get_box_cnt(Animator *anim, int scene_idx)
{
    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, scene_idx));
    RET_IF(!scene, -1);
    return  nemoplay_box_get_count(scene->box);
}

void animator_set_box_idx(Animator *anim, int idx)
{
    anim->box_idx = idx;
}

void animator_stop(Animator *anim)
{
    nemotimer_set_timeout(anim->timer, 0);
    nemotimer_set_timeout(anim->timer_reverse, 0);
}

void animator_show(Animator *anim, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(anim->widget, easetype, duration, delay);
    nemowidget_set_alpha(anim->widget, easetype, duration, delay, 1.0);
    nemoshow_dispatch_frame(anim->show);
}

void animator_hide(Animator *anim, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(anim->widget, easetype, duration, delay);
    nemowidget_set_alpha(anim->widget, easetype, duration, delay, 0.0);
    nemoshow_dispatch_frame(anim->show);
}

void animator_translate(Animator *anim, uint32_t easetype, int duration, int delay, float tx, float ty)
{
    nemowidget_translate(anim->widget, easetype, duration, delay, tx, ty);
}

static void _animator_resize(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    NemoWidgetInfo_Resize *resize = info;
    Animator *anim = userdata;

    struct showone *canvas = nemowidget_get_canvas(widget);

	nemoplay_shader_set_viewport(anim->shader,
			nemoshow_canvas_get_texture(canvas),
            resize->width, resize->height);

    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, anim->scene_idx));
    if (!scene) return;

    struct playone *one;
    one = nemoplay_box_get_one(scene->box, anim->box_idx);
    if (one) {
        nemoplay_shader_update(anim->shader, one);
        nemoplay_shader_dispatch(anim->shader);
    }
}

static void _animator_timeout(struct nemotimer *timer, void *userdata)
{
    Animator *anim = userdata;

    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, anim->scene_idx));
    RET_IF(!scene);

    int box_cnt = nemoplay_box_get_count(scene->box);

    struct playone *one;
	one = nemoplay_box_get_one(scene->box, anim->box_idx);
    ERR("[scene: %d][box: %d/%d][%p]", anim->scene_idx,
            anim->box_idx, box_cnt, one);
	if (one) {
		nemoplay_shader_update(anim->shader, one);
		nemoplay_shader_dispatch(anim->shader);
	}

    nemowidget_dirty(anim->widget);
    nemoshow_dispatch_frame(anim->show);

    anim->box_idx++;
    if (anim->box_idx >= box_cnt) {
        if (anim->repeat != 0) {
            anim->box_idx = 0;
            if (anim->repeat > 0) anim->repeat--;
            nemotimer_set_timeout(timer, 1000/60);
        } else {
            anim->box_idx = box_cnt - 1;
            if (anim->done)
                anim->done(anim, anim->done_userdata);
        }
    } else {
        nemotimer_set_timeout(timer, 1000/60);
    }
}

static void _animator_reverse_timeout(struct nemotimer *timer, void *userdata)
{
    Animator *anim = userdata;

    AnimatorScene *scene = LIST_DATA(list_get_nth(anim->scenes, anim->scene_idx));
    RET_IF(!scene);

    int box_cnt = nemoplay_box_get_count(scene->box);

    struct playone *one;
	one = nemoplay_box_get_one(scene->box, anim->box_idx);
    ERR("[scene: %d][box: %d/%d][%p]", anim->scene_idx,
            anim->box_idx, box_cnt, one);
	if (one) {
		nemoplay_shader_update(anim->shader, one);
		nemoplay_shader_dispatch(anim->shader);
	}

    nemowidget_dirty(anim->widget);
    nemoshow_dispatch_frame(anim->show);

    anim->box_idx--;
    if (anim->box_idx < 0 ) {
        if (anim->repeat != 0) {
            anim->box_idx = box_cnt - 1;
            if (anim->repeat > 0) anim->repeat--;
            nemotimer_set_timeout(timer, 1000/60);
        } else {
            anim->box_idx = 0;
            if (anim->done)
                anim->done(anim, anim->done_userdata);
        }
    } else {
        nemotimer_set_timeout(timer, 1000/60);
    }
}

Animator *animator_create(NemoWidget *parent, int width, int height)
{
    Animator *anim = calloc(sizeof(Animator), 1);
    anim->show = nemowidget_get_show(parent);
    anim->tool = nemowidget_get_tool(parent);
    anim->scene_idx = -1;

    struct playshader *shader;
    anim->shader = shader = nemoplay_shader_create();

    NemoWidget *widget;
    anim->widget = widget = nemowidget_create_opengl(parent, width, height);
    nemowidget_append_callback(widget, "resize", _animator_resize, anim);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    anim->timer = TOOL_ADD_TIMER(anim->tool, 0,
            _animator_timeout, anim);
    anim->timer_reverse = TOOL_ADD_TIMER(anim->tool, 0,
            _animator_reverse_timeout, anim);

    return anim;
}

void animator_append_callback(Animator *anim, const char *id, NemoWidget_Func func, void *userdata)
{
    nemowidget_append_callback(anim->widget, id, func, userdata);
}

AnimatorScene *animator_append_scene(Animator *anim, const char *path)
{
    AnimatorScene *scene = calloc(sizeof(AnimatorScene), 1);

    struct nemoplay *play;
    scene->play = play = nemoplay_create();
    nemoplay_load_media(play, path);

    struct playbox *box;
    scene->box = box = nemoplay_box_create(nemoplay_get_video_framecount(play));
	nemoplay_extract_video(play, box);

    anim->scenes = list_append(anim->scenes, scene);

    return scene;
}

typedef enum {
    ICON_STATE_HIDE = 0,
    ICON_STATE_NORMAL,
    ICON_STATE_DOWN
} IconState;

typedef struct _Icon Icon;
struct _Icon {
    Animator *anim;
    IconState state;
};

Icon *icon_create(NemoWidget *parent, int w, int h,
        const char *normal_path,
        const char *show_path, const char *down_path)
{
    Icon *icon = calloc(sizeof(Icon), 1);

    Animator *anim;
    icon->anim = anim = animator_create(parent, w, h);
    animator_append_scene(anim, show_path);
    animator_append_scene(anim, down_path);
    animator_append_scene(anim, normal_path);
    icon->state = ICON_STATE_HIDE;
    return icon;
}

void icon_append_callback(Icon *icon, const char *id, NemoWidget_Func func, void *userdata)
{
    animator_append_callback(icon->anim, id, func, userdata);
}

void icon_show(Icon *icon, uint32_t easetype, int duration, int delay)
{
    animator_show(icon->anim, easetype, duration, delay);
}

void icon_hide(Icon *icon, uint32_t easetype, int duration, int delay)
{
    animator_hide(icon->anim, easetype, duration, delay);
}

void _icon_play_down(Animator *anim, void *userdata)
{
    Icon *icon = userdata;
    animator_set_done(icon->anim, NULL, NULL);
    animator_play_scene(icon->anim, 1);
}

void _icon_play_hide(Animator *anim, void *userdata)
{
    Icon *icon = userdata;
    animator_set_done(icon->anim, NULL, NULL);
    animator_play_scene_reverse(icon->anim, 0);
}

void icon_set_state(Icon *icon, IconState state)
{
    RET_IF(icon->state < ICON_STATE_HIDE || icon->state > ICON_STATE_DOWN);

    ERR("%d --> %d", icon->state, state);
    if (icon->state == ICON_STATE_HIDE) {
        if (state == ICON_STATE_NORMAL) {
            animator_play_scene(icon->anim, 0);
        } else if (state == ICON_STATE_DOWN) {
            animator_play_scene(icon->anim, 0);
            animator_set_done(icon->anim, _icon_play_down, icon);
        } else {
            ERR("Already state: %d", state);
        }
    } else if (icon->state == ICON_STATE_NORMAL) {
        if (state == ICON_STATE_HIDE) {
            animator_play_scene_reverse(icon->anim, 0);
            animator_set_done(icon->anim, _icon_play_down, icon);
        } else if (state == ICON_STATE_DOWN) {
            ERR("XXXXXXXXXXXX");
            animator_play_scene(icon->anim, 1);
        } else {
            ERR("Already state: %d", state);
        }
    } else if (icon->state == ICON_STATE_DOWN) {
        if (state == ICON_STATE_NORMAL) {
            animator_play_scene_reverse(icon->anim, 1);
        } else if (state == ICON_STATE_HIDE) {
            animator_play_scene_reverse(icon->anim, 1);
            animator_set_done(icon->anim, _icon_play_hide, icon);
        } else {
            ERR("Already state: %d", state);
        }
    }
    icon->state = state;
}

typedef struct _MenuView MenuView;
struct _MenuView {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;

    NemoWidget *widget;
    struct showone *group;
    struct showone *bg;

    List *menus;
    Animator *menu_bg;
    Animator *menu_item;
    Icon *icon;
};

typedef struct _Menu Menu;
struct _Menu {
    Animator *bg;
    List *items;
};


static void _menu_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    MenuView *view = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        ERR("down");
        //animator_play(view->menu_item);
        icon_set_state(view->icon, ICON_STATE_DOWN);
    } else if (nemoshow_event_is_up(show, event)) {
            //;icon_set_state(view->icon, ICON_STATE_NORMAL);
        if (nemoshow_event_is_single_click(show, event)) {
            ERR("click");
            icon_set_state(view->icon, ICON_STATE_HIDE);
            /*
            animator_set_box_idx(view->menu_item, 59);
            animator_set_scene(view->menu_item, 0);
            animator_play_reverse(view->menu_item);
            */
        } else {
            ERR("up");
            icon_set_state(view->icon, ICON_STATE_NORMAL);
            //animator_play_reverse(view->menu_item);
        }
    }
}

#if 0
static void _menu_icon_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    MenuView *view = userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        animator_set_scene(view->menu_item0, 0);
        animator_set_reverse(view->menu_item0, false);
        animator_play(view->menu_item0);
        animator_show(view->menu_item0, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);
        animator_hide(view->menu_item1, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);
        animator_hide(view->menu_item, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);

    } else if (nemoshow_event_is_up(show, event)) {
        animator_set_scene(view->menu_item1, 0);
        animator_set_reverse(view->menu_item1, true);
        animator_play(view->menu_item1);
        animator_hide(view->menu_item0, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);
        animator_show(view->menu_item1, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);
        animator_hide(view->menu_item, NEMOEASE_CUBIC_OUT_TYPE, 150, 0);

        if (nemoshow_event_is_single_click(show, event)) {
        }
    }
}
#endif

MenuView *menuview_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    MenuView *view = calloc(sizeof(MenuView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    NemoWidget *widget;
    struct showone *one;
    struct showone *group;

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->bg = one = RECT_CREATE(group, width, height);
    nemoshow_item_set_fill_color(one, RGBA(RED));

    Animator *anim;
    const char *path = MENU_ANIM_DIR"/contents_show.mov";
    int w, h;
    nemoplay_get_video_info(path, &w, &h);
    /*
    w *= app->sxy;
    h *= app->sxy;
    */

#if 0
    AnimatorScene *scene;
    view->menu_item = anim = animator_create(widget, w, h);
    animator_append_callback(anim, "event", _menu_event, view);
    animator_append_scene(anim, MENU_ANIM_DIR"/contents_show.mov");
    scene = animator_append_scene(anim, MENU_ANIM_DIR"/contents_click.mov");
#endif
    Icon *icon;
    view->icon = icon = icon_create(widget, w, h,
            MENU_ANIM_DIR"/contents_show.mov",
            MENU_ANIM_DIR"/contents_click.mov",
            MENU_ANIM_DIR"/shine.mov"
            );
    icon_append_callback(icon, "event", _menu_event, view);

    return view;
}

void _menuview_show_done(Animator *anim, void *userdata)
{
    animator_set_done(anim, NULL, NULL);
    animator_set_scene(anim, 1);
    animator_set_box_idx(anim, 0);
}

void menuview_show(MenuView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 1.0);

    /*
    animator_show(view->menu_item, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    animator_set_done(view->menu_item, _menuview_show_done, view);
    animator_set_scene(view->menu_item, 0);
    animator_play(view->menu_item);
    */
    icon_show(view->icon, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    icon_set_state(view->icon, ICON_STATE_NORMAL);

    nemoshow_dispatch_frame(view->show);
}

void menuview_hide(MenuView *view)
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
    /*
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "background");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);
    */

    MenuView *view = menuview_create(win, app->config->width, app->config->height, app);
    menuview_show(view);

    nemoshow_dispatch_frame(view->show);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
