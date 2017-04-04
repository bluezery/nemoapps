#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include ""
#include "nemohelper.h"
#include "sound.h"


typedef struct _Icons Icons;
struct _Icons {
    const char *uri;
    const char *uri2;
    double x, y;
};

Icons BELLS[] = {
    {BISTRO_ICON_DIR"/bell/waiter.svg",  NULL, 80, 58},
    {BISTRO_ICON_DIR"/bell/payment.svg", NULL, 80, 163},
    {BISTRO_ICON_DIR"/bell/menu.svg",    NULL, 80, 269},
    {BISTRO_ICON_DIR"/bell/tissue.svg",  NULL, 80, 450},
    {BISTRO_ICON_DIR"/bell/water.svg",   NULL, 80, 556},
    {BISTRO_ICON_DIR"/bell/napkin.svg",  NULL, 80, 662}
};

Icons CATEGORIES[] = {
    {BISTRO_ICON_DIR"/menu/category0.svg", NULL, 80, 58},
    {BISTRO_ICON_DIR"/menu/category1.svg", NULL, 80, 163},
    {BISTRO_ICON_DIR"/menu/category2.svg", NULL, 80, 269},
    {BISTRO_ICON_DIR"/menu/category3.svg", NULL, 80, 450},
    {BISTRO_ICON_DIR"/menu/category4.svg", NULL, 80, 556},
    {BISTRO_ICON_DIR"/menu/category5.svg", NULL, 80, 662}
};

Icons MENUS[6][6] = {
    {{0,}, },
    {{0,}, },
    {
        {BISTRO_IMG_DIR"/menu/menu0-0.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu0-1.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu0-2.png", BISTRO_IMG_DIR"/menu/dish2-2/", 80, 58},
        {BISTRO_IMG_DIR"/menu/menu0-3.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu0-4.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu0-5.png", NULL, 80, 58},
    },
    {
        {BISTRO_IMG_DIR"/menu/menu3-0.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu3-1.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu3-2.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu3-3.png", NULL, 80, 58},
        {BISTRO_IMG_DIR"/menu/menu3-4.png", BISTRO_IMG_DIR"/menu/dish3-4/", 80, 58},
        {NULL, },
    },
    {{0,}, },
    {{0,}, }
};

typedef struct _MenuView MenuView;
typedef struct _BellView BellView;
typedef struct _BistroView BistroView;
typedef struct _IntroView IntroView;
typedef struct _Dish Dish;
typedef struct _Menu Menu;
typedef struct _Category Category;

struct _Dish {
    struct nemotool *tool;
    int w, h;
    char *uri;
    struct showone *group;
    Animation *anim;
};

void dish_destroy(Dish *dish)
{
    animation_destroy(dish->anim);
    nemoshow_one_destroy(dish->group);
    free(dish);
}

Dish *dish_create(struct nemotool *tool, struct showone *parent, int w, int h, const char *uri, int ro)
{
    if (!file_is_dir(uri)) {
        ERR("file is not directory: %s", uri);
        return NULL;
    }

    Dish *dish = calloc(sizeof(Dish), 1);
    dish->tool = tool;
    dish->w = w;
    dish->h = h;

    struct showone *group;
    dish->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_rotate(group, ro);

    Animation *animation;
    dish->anim = animation = animation_create(tool, group, w, h);
    animation_set_fps(animation, 24);

    int i = 0;
    do {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%s/%05d.png", uri, i++);
        if (!file_is_exist(buf)) break;
        animation_append_item(animation, buf);
    } while (1);
    animation_set_anchor(animation, 0.5, 0.5);
    return dish;
}

void dish_hide(Dish *dish, uint32_t easetype, int duration, int delay)
{
    animation_hide(dish->anim, easetype, duration, delay);
    if (duration > 0) {
        _nemoshow_item_motion(dish->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(dish->group, 0.0);
    }
}

static void _dish_destroy(struct nemotimer *timer, void *data)
{
    Dish *dish = data;
    dish_destroy(dish);
}

void dish_hide_destroy(Dish *dish, uint32_t easetype, int duration, int delay)
{
    dish_hide(dish, easetype, duration, delay);
    TOOL_ADD_TIMER(dish->tool, duration + delay + 100, _dish_destroy, dish);
}

void dish_show(Dish *dish, uint32_t easetype, int duration, int delay)
{
    animation_show(dish->anim, easetype, duration, delay);
    if (duration > 0) {
        _nemoshow_item_motion(dish->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(dish->group, 1.0);
    }
}

void dish_translate(Dish *dish, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(dish->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_item_translate(dish->group, tx, ty);
    }
}

struct _Menu {
    Category *cat;
    int w, h;
    char *uri;
    struct showone *group;
    struct showone *one;

    char *dish_uri;
};

struct _Category {
    MenuView *menu;
    struct showone *group;
    struct showone *one;

    struct showone *menu_group;
    List *menus;
};

struct _MenuView {
    BistroView *bistro;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;

    struct showone *group;
    struct showone *bg;

    struct showone *category_group;
    List *categories;
    Category *category_sel;

    int mode;
    NemoWidgetGrab *grab;

    int order_w, order_h;
    int request_w, request_h;
    struct showone *order0;
    struct showone *request0;
    List *orders0;
    struct showone *order1;
    struct showone *request1;
    List *orders1;

    struct showone *dish_group;
    List *dishes;
};

struct _BellView {
    BistroView *bistro;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;

    struct showone *group;
    struct showone *bg;
    struct showone *bell_group;
    struct showone *bell_circle;
    struct showone *bell_bell;
    struct showone *bell_quit;

    struct showone *item_group;
    List *items;

    int mode;
    NemoWidgetGrab *grab;
};

struct _IntroView {
    BistroView *bistro;
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;

    struct showone *group;
    int anim_w, anim_h;
    Animation *anim;
    Image *logo0, *logo1;
};

struct _BistroView {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;

    NemoWidget *widget;
    struct showone *group;
    struct showone *bg;

    IntroView *intro;
    MenuView *menu;
    BellView *bell;
};

void intro_view_destroy(IntroView *view)
{
    image_destroy(view->logo0);
    image_destroy(view->logo1);
    animation_destroy(view->anim);
    nemowidget_destroy(view->widget);
}

static void _intro_view_event(NemoWidget *widget, const char *id, void *info, void *userdata);
IntroView *intro_view_create(BistroView *bistro, NemoWidget *parent, int width, int height)
{
    IntroView *view = calloc(sizeof(IntroView), 1);
    view->bistro = bistro;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    NemoWidget *widget;
    struct showone *group;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _intro_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    Animation *animation;
    view->anim_w = 390;
    view->anim_h = 390;
    view->anim = animation = animation_create(view->tool, group,
            view->anim_w, view->anim_h);
    animation_set_fps(animation, 24);
    animation_set_anchor(animation, 0.5, 0.5);
    animation_translate(animation, 0, 0, 0, width/2, height/2);

    int i = 0;
    do {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, BISTRO_IMG_DIR"/intro/seq/%05d.png", i++);
        if (!file_is_exist(buf)) break;
        animation_append_item(animation, buf);
    } while (1);

    Image *img;
    view->logo0 = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_scale(img, 0, 0, 0, 0.0, 0.0);
    image_load(img, view->tool, BISTRO_IMG_DIR"/intro/logo0.png", 112, 81, NULL, NULL);
    image_translate(img, 0, 0, 0, width/2, height/2);

    view->logo1 = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_scale(img, 0, 0, 0, 0.0, 0.0);
    image_load(img, view->tool, BISTRO_IMG_DIR"/intro/logo1.png", 78, 78, NULL, NULL);
    image_translate(img, 0, 0, 0, width/2, height/2);

    return view;
}

void intro_view_show(IntroView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    animation_show(view->anim, easetype, duration, delay);
    image_scale(view->logo0, easetype, duration, delay + 250, 1.0, 1.0);
    image_scale(view->logo1, easetype, duration, delay + 500, 1.0, 1.0);
    image_translate(view->logo0, easetype, duration, delay + 250,
            view->width * 0.2, view->height/2);
    image_translate(view->logo1, easetype, duration, delay + 500,
            view->width * 0.8, view->height/2);
}

void intro_view_hide(IntroView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);
    animation_hide(view->anim, easetype, duration, delay);
    image_scale(view->logo0, easetype, duration, delay, 0.0, 0.0);
    image_scale(view->logo1, easetype, duration, delay, 0.0, 0.0);
    image_translate(view->logo0, easetype, duration, delay,
            view->width/2, view->height/2);
    image_translate(view->logo1, easetype, duration, delay,
            view->width/2, view->height/2);
}

void menu_show(Menu *menu, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(menu->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    }  else {
        nemoshow_item_set_alpha(menu->group, 1.0);
    }
}

void menu_hide(Menu *menu, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(menu->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    }  else {
        nemoshow_item_set_alpha(menu->group, 0.0);
    }
}

void menu_translate(Menu *menu, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(menu->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    }  else {
        nemoshow_item_translate(menu->group, tx, ty);
    }
}

void menu_translate_x(Menu *menu, uint32_t easetype, int duration, int delay, double tx)
{
    if (duration > 0) {
        _nemoshow_item_motion(menu->group, easetype, duration, delay,
                "tx", tx, NULL);
    }  else {
        nemoshow_item_translate(menu->group, tx,
                nemoshow_item_get_translate_y(menu->group));
    }
}

void menu_down(Menu *menu, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion_bounce(menu->group, easetype, duration, delay,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    }  else {
        nemoshow_item_scale(menu->group, 0.75, 0.75);
    }
}

void menu_up(Menu *menu, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion_bounce(menu->group, easetype, duration, delay,
                "sx", 1.15, 1.0, "sy", 1.15, 1.0,
                NULL);
    }  else {
        nemoshow_item_scale(menu->group, 1.0, 1.0);
    }
}

void menu_destroy(Menu *menu)
{
    nemoshow_one_destroy(menu->group);
    free(menu->uri);
    free(menu);
}

Category *menu_view_create_category(MenuView *view,
        const char *uri, const char *id, int w, int h)
{
    if (!file_is_exist(uri)) {
        ERR("file does not exist: %s", uri);
        return NULL;
    }
    if (!file_is_svg(uri)) {
        ERR("file is not a svg: %s", uri);
        return NULL;
    }

    Category *cat = calloc(sizeof(Category), 1);
    cat->menu = view;

    struct showone *group;
    cat->group = group = GROUP_CREATE(view->category_group);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;
    cat->one = one = SVG_PATH_GROUP_CREATE(group, w, h, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_rotate(one, -90);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, id);
    nemoshow_one_set_userdata(one, cat);

    cat->menu_group = group = GROUP_CREATE(view->category_group);

    view->categories = list_append(view->categories, cat);
    return cat;
}

Menu *category_create_menu(Category *cat, const char *uri,
        const char *dish_uri, const char *id, int w, int h)
{
    RET_IF(!file_is_exist(uri), NULL);
    RET_IF(!file_is_image(uri), NULL);

    Menu *menu = calloc(sizeof(Menu), 1);
    menu->cat = cat;
    menu->uri = strdup(uri);
    if (dish_uri) menu->dish_uri = strdup(dish_uri);
    menu->w = w;
    menu->h = h;

    struct showone *group;
    menu->group = group = GROUP_CREATE(cat->menu_group);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;
    menu->one = one = IMAGE_CREATE(group, w, h, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_rotate(one, -90);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, id);
    nemoshow_one_set_userdata(one, menu);

    cat->menus = list_append(cat->menus, menu);
    return menu;
}

Menu *menu_dup(Menu *org, const char *id, int w, int h, int ro)
{
    Menu *menu = calloc(sizeof(Menu), 1);
    menu->cat = org->cat;
    menu->uri = strdup(org->uri);
    menu->w = w;
    menu->h = h;

    struct showone *group;
    menu->group = group = GROUP_CREATE(org->cat->menu->group);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_translate(group,
            nemoshow_item_get_translate_x(org->group),
            nemoshow_item_get_translate_y(org->group));

    struct showone *one;
    menu->one = one = IMAGE_CREATE(group, w, h, org->uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_rotate(one, ro);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, id);
    nemoshow_one_set_userdata(one, menu);

    return menu;
}

void category_destroy(Category *cat)
{
    Menu *menu;
    LIST_FREE(cat->menus, menu) menu_destroy(menu);
    nemoshow_one_destroy(cat->menu_group);
    nemoshow_one_destroy(cat->group);
    free(cat);
}

void category_show(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(cat->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    }  else {
        nemoshow_item_set_alpha(cat->group, 1.0);
    }
}

void category_hide(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(cat->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    }  else {
        nemoshow_item_set_alpha(cat->group, 0.0);
    }
}

void category_translate(Category *cat, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(cat->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    }  else {
        nemoshow_item_translate(cat->group, tx, ty);
    }
}

void category_translate_x(Category *cat, uint32_t easetype, int duration, int delay, double tx)
{
    if (duration > 0) {
        _nemoshow_item_motion(cat->group, easetype, duration, delay,
                "tx", tx, NULL);
    }  else {
        nemoshow_item_translate(cat->group, tx,
                nemoshow_item_get_translate_y(cat->group));
    }
}

void category_down(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion_bounce(cat->group, easetype, duration, delay,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    }  else {
        nemoshow_item_scale(cat->group, 0.75, 0.75);
    }
}

void category_up(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion_bounce(cat->group, easetype, duration, delay,
                "sx", 1.15, 1.0, "sy", 1.15, 1.0,
                NULL);
    }  else {
        nemoshow_item_scale(cat->group, 1.0, 1.0);
    }
}

void category_selected(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion_bounce(cat->group, easetype, duration, delay,
                "sx", 1.6, 1.5, "sy", 1.6, 1.5,
                NULL);
    }  else {
        nemoshow_item_scale(cat->group, 1.5, 1.5);
    }
}

void category_unselected(Category *cat, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(cat->group, easetype, duration, delay,
                "sx", 1.0, "sy", 1.0,
                NULL);
    }  else {
        nemoshow_item_scale(cat->group, 1.0, 1.0);
    }
}

void menu_view_destroy(MenuView *view)
{
    Category *cat;
    LIST_FREE(view->categories, cat) category_destroy(cat);
    nemowidget_destroy(view->widget);
}


static void _menu_view_event(NemoWidget *widget, const char *id, void *info, void *userdata);
MenuView *menu_view_create(BistroView *bistro, NemoWidget *parent, int width, int height)
{
    MenuView *view = calloc(sizeof(MenuView), 1);
    view->bistro = bistro;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    struct showone *group;
    struct showone *one;
    NemoWidget *widget;

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _menu_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    int w, h;
    w = width/10;
    h = height;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->bg = one = RECT_CREATE(group, w, h);
    nemoshow_item_set_fill_color(one, RGBA(0xBDC3C7FF));
    nemoshow_item_scale(one, 0.0, 1.0);
    nemoshow_item_set_alpha(one, 0.0);

    view->dish_group = GROUP_CREATE(group);
    view->category_group = group = GROUP_CREATE(view->group);

    // Category & Menu
    int ww, hh;
    ww = 85;
    hh = 50;
    int cat_cnt = sizeof(CATEGORIES)/sizeof(CATEGORIES[0]);
    int cat_gap = (view->height - ww * cat_cnt)/(cat_cnt + 1);
    int i;
    for (i = 0 ; i < cat_cnt ; i++) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "category_%d", i);
        Category *cat = menu_view_create_category(view, CATEGORIES[i].uri, buf, ww, hh);
        category_translate(cat, 0, 0, 0, -200,
                cat_gap * (i + 1) + ww * i + (ww/2));

        int www, hhh;
        www = 150;
        hhh = 150;
        int menu_cnt = 0;
        int j;
        for (j = 0 ; j < 6 ; j++) {
            if (!MENUS[i][j].uri) break;
            menu_cnt++;
        }

        for (j = 0 ; j < menu_cnt ; j++) {
            snprintf(buf, PATH_MAX, "menu_%d_%d", i, j);
            Menu *menu = category_create_menu(cat, MENUS[i][j].uri, MENUS[i][j].uri2, buf, www, hhh);
            menu_translate(menu, 0, 0, 0, view->width + 200, view->height/2);
        }
    }

    // Order board
    int _width = width - w;
    double www, hhh;
    const char *uri;
    uri = BISTRO_ICON_DIR"/menu/order.svg";
    if (!svg_get_wh(uri, &www, &hhh)) {
        ERR("svg get wh failed: %s", uri);
    } else {
        view->order_w = www;
        view->order_h = hhh;
        view->order0 = one = SVG_PATH_GROUP_CREATE(group, www, hhh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_rotate(one, 180);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        nemoshow_one_set_id(one, "order0");
        nemoshow_one_set_userdata(one, view);
        nemoshow_item_set_alpha(one, 0.0);
    }
    uri = BISTRO_ICON_DIR"/menu/request.svg";
    if (!svg_get_wh(uri, &www, &hhh)) {
        ERR("svg get wh failed: %s", uri);
    } else {
        view->request_w = www;
        view->request_h = hhh;
        view->request0 = one = SVG_PATH_GROUP_CREATE(group, www, hhh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_rotate(one, 180);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        nemoshow_one_set_id(one, "request0");
        nemoshow_one_set_userdata(one, view);
        nemoshow_item_set_alpha(one, 0.0);
    }
    nemoshow_item_translate(view->order0,
            w + _width/2 - view->request_h/2 - 5, -view->order_h);
    nemoshow_item_translate(view->request0,
            w + _width/2 + view->order_w/2 + 5, -view->order_h);

    uri = BISTRO_ICON_DIR"/menu/order.svg";
    if (!svg_get_wh(uri, &www, &hhh)) {
        ERR("svg get wh failed: %s", uri);
    } else {
        view->order1 = one = SVG_PATH_GROUP_CREATE(group, www, hhh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        nemoshow_one_set_id(one, "order1");
        nemoshow_one_set_userdata(one, view);
        nemoshow_item_set_alpha(one, 0.0);
    }
    uri = BISTRO_ICON_DIR"/menu/request.svg";
    if (!svg_get_wh(uri, &www, &hhh)) {
        ERR("svg get wh failed: %s", uri);
    } else {
        view->request_w = www;
        view->request_h = hhh;
        view->request1 = one = SVG_PATH_GROUP_CREATE(group, www, hhh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        nemoshow_one_set_id(one, "request1");
        nemoshow_one_set_userdata(one, view);
        nemoshow_item_set_alpha(one, 0.0);
    }
    nemoshow_item_translate(view->order1,
            w + _width/2 - view->request_h/2 - 5, view->height + view->order_h);
    nemoshow_item_translate(view->request1,
            w + _width/2 + view->order_w/2 + 5, view->height + view->order_h);

    return view;
}

void menu_view_show(MenuView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    _nemoshow_item_motion(view->bg, easetype, duration, delay,
            "sx", 1.0, "alpha", 1.0, NULL);
    int i = 0;
    List *l;
    Category *cat;
    LIST_FOR_EACH(view->categories, l, cat) {
        category_translate_x(cat, easetype, duration, delay + i * 150,
                view->width/10.0/2.0);
        category_show(cat, easetype, duration, delay + i * 150);
        i++;
    }

    _nemoshow_item_motion(view->order0, easetype, duration, delay,
            "alpha", 1.0,
            "ty", (double)view->order_h/2 + 10, NULL);
    _nemoshow_item_motion(view->request0, easetype, duration, delay + 150,
            "alpha", 1.0,
            "ty", (double)view->order_h - view->request_h/2 + 10, NULL);

    _nemoshow_item_motion(view->order1, easetype, duration, delay,
            "alpha", 1.0,
            "ty", (double)view->height - view->order_h/2 - 10, NULL);
    _nemoshow_item_motion(view->request1, easetype, duration, delay + 150,
            "alpha", 1.0,
            "ty", (double)view->height - view->order_h + view->request_h/2 - 10, NULL);
}

void menu_view_hide(MenuView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    _nemoshow_item_motion(view->bg, easetype, duration, delay,
            "sx", 0.0, "alpha", 0.0, NULL);

    List *l;
    Category *cat;
    LIST_FOR_EACH(view->categories, l, cat) {
        category_translate_x(cat, easetype, duration, delay, -200);
        category_hide(cat, easetype, duration, delay);
    }

    _nemoshow_item_motion(view->order0, easetype, duration, delay,
            "alpha", 0.0,
            "ty", -view->order_h, NULL);
    _nemoshow_item_motion(view->order1, easetype, duration, delay,
            "alpha", 0.0,
            "ty", view->height + view->order_h, NULL);
}



void bell_view_destroy(BellView *view)
{
    struct showone *one;
    LIST_FREE(view->items, one);
    nemowidget_destroy(view->widget);
}

static void _bell_view_event(NemoWidget *widget, const char *id, void *info, void *userdata);
BellView *bell_view_create(BistroView *bistro, NemoWidget *parent, int width, int height)
{
    BellView *view = calloc(sizeof(BellView), 1);
    view->bistro = bistro;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    struct showone *group;
    struct showone *one;
    NemoWidget *widget;

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _bell_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    int w, h;
    w = width/8;
    h = height;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    view->bg = one = SVG_PATH_GROUP_CREATE(group, w, h,
            BISTRO_ICON_DIR"/bell/background.svg");
    nemoshow_item_scale(one, 0.0, 1.0);
    nemoshow_item_set_alpha(one, 0.0);

    int ww, hh;
    ww = w * 0.4;
    hh = w * 0.4;

    // main
    view->bell_group = group = GROUP_CREATE(view->group);
    nemoshow_item_translate(group, -ww * 2, height/2);

    view->bell_circle = one = SVG_PATH_GROUP_CREATE(group, ww, hh,
            BISTRO_ICON_DIR"/bell/circle.svg");
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "bell");
    nemoshow_one_set_userdata(one, view);

    view->bell_bell = one = SVG_PATH_GROUP_CREATE(group, ww, hh,
            BISTRO_ICON_DIR"/bell/bell.svg");
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);

    view->bell_quit = one = SVG_PATH_GROUP_CREATE(group, ww, hh,
            BISTRO_ICON_DIR"/bell/quit.svg");
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_alpha(one, 0.0);

    ww = w * 0.6;
    hh = w * 0.6;

    view->item_group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 1.0);
    int cnt = sizeof(BELLS)/sizeof(BELLS[0]);
    int i;
    for (i = 0 ; i < cnt ; i++) {
        char buf[PATH_MAX];
        one = SVG_PATH_GROUP_CREATE(group, ww, hh, BELLS[i].uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, -ww * 2, BELLS[i].y);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        snprintf(buf, PATH_MAX, "bell_%d", i);
        nemoshow_one_set_id(one, buf);
        nemoshow_one_set_userdata(one, view);
        view->items = list_append(view->items, one);
    }
    return view;
}

void bell_view_show(BellView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    _nemoshow_item_motion(view->bell_group, easetype, duration, delay,
            "tx", (double)view->width/8.0/2.0, NULL);
    _nemoshow_item_motion_bounce(view->bell_circle, easetype, duration, delay,
            "sx", 2.5, 2.0, "sy", 2.5, 2.0, NULL);
    _nemoshow_item_motion_bounce(view->bell_bell, easetype, duration, delay + 50,
            "sx", 2.5, 2.0, "sy", 2.5, 2.0, NULL);
    _nemoshow_item_motion_bounce(view->bell_quit, easetype, duration, delay + 50,
            "sx", 2.5, 2.0, "sy", 2.5, 2.0, NULL);
}

void bell_view_hide(BellView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    _nemoshow_item_motion(view->bell_group, easetype, duration, delay,
            "tx", -(view->width/8.0) * 0.4 * 2, NULL);
    _nemoshow_item_motion(view->bell_circle, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion_bounce(view->bell_bell, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion_bounce(view->bell_quit, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);

    _nemoshow_item_motion(view->bg, easetype, duration, delay,
            "sx", 0.0,
            "alpha", 0.0,
            NULL);
    List *l;
    struct showone *one;
    LIST_FOR_EACH(view->items, l, one) {
        _nemoshow_item_motion(one, easetype, duration, delay,
                "tx", -(view->width/8.0) * 0.6 * 2, NULL);
    }
    view->mode = 0;
}

void bistro_view_show(BistroView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_OUT_TYPE;
    int duration = 1000;
    int delay = 150;

    intro_view_show(view->intro, easetype, duration, delay);

    nemoshow_dispatch_frame(view->show);
}

void bistro_view_hide(BistroView *view)
{
    nemowidget_hide(view->widget, 0, 0, 0);

    uint32_t easetype = NEMOEASE_CUBIC_IN_TYPE;
    int duration = 500;
    int delay = 0;

    intro_view_hide(view->intro, easetype, duration, delay);
    nemoshow_dispatch_frame(view->show);
}

BistroView *bistro_view_create(NemoWidget *parent, int width, int height)
{
    BistroView *view = calloc(sizeof(BistroView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->width = width;
    view->height = height;

    NemoWidget *widget;
    struct showone *group;
    struct showone *one;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    view->bg = one = RECT_CREATE(group, width, height);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    // Intro
    view->bell = bell_view_create(view, widget, width, height);
    view->menu = menu_view_create(view, widget, width, height);
    view->intro = intro_view_create(view, widget, width, height);

    return view;
}

static void _intro_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    IntroView *view = userdata;

    if (nemoshow_event_is_up(show, event)) {
        BistroView *bistro = view->bistro;
        intro_view_hide(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        menu_view_show(bistro->menu, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        nemoshow_dispatch_frame(show);
    }
}

static void _bell_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *one = userdata;
    BellView *view = nemoshow_one_get_userdata(one);

    if (nemoshow_event_is_down(show, event)) {
        _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 1.45, 1.5, "sy", 1.45, 1.5,
                NULL);
        _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.45, 1.5, "sy", 1.45, 1.5,
                NULL);
        _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.45, 1.5, "sy", 1.45, 1.5,
                NULL);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        if (nemoshow_event_is_single_click(show, event)) {
            view->mode = 1;
            _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 550, 0,
                    "sx", 1.25, 1.00, "sy", 1.25, 1.00,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 550, 50,
                    "sx", 1.25, 1.00, "sy", 1.25, 1.00,
                    "alpha", 0.0, 0.0,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 550, 50,
                    "sx", 1.25, 1.00, "sy", 1.25, 1.00,
                    "alpha", 1.0, 1.0,
                    NULL);

            _nemoshow_item_motion_bounce(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                    "sx", 1.25, 1.00,
                    "alpha", 1.0, 1.0,
                    NULL);
            int i = 0;
            List *l;
            struct showone *one;
            LIST_FOR_EACH(view->items, l, one) {
                _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 550, i * 150,
                        "tx", BELLS[i].x,
                        "alpha", 1.0,
                        NULL);
                i++;
            }
        } else {
            _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                    "sx", 2.25, 2.00, "sy", 2.25, 2.00,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                    "sx", 2.25, 2.00, "sy", 2.25, 2.00,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                    "sx", 2.25, 2.00, "sy", 2.25, 2.00,
                    NULL);
        }
        nemoshow_dispatch_frame(show);
        view->grab = NULL;
    }
}

static void _bell_view_grab_call_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *one = userdata;
    BellView *view = nemoshow_one_get_userdata(one);
    const char *id = nemoshow_one_get_id(one);
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        if (!strcmp(id, "bell")) {
            _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                    "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                    "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                    NULL);
            _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                    "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                    NULL);
        } else {
            _nemoshow_item_motion_bounce(one, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                    "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                    NULL);
        }
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        if (!strcmp(id, "bell")) {
            if (nemoshow_event_is_single_click(show, event)) {
                view->mode = 0;
                _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 550, 500,
                        "sx", 2.1, 2.0, "sy", 2.1, 2.0,
                        NULL);
                _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 550, 550,
                        "sx", 2.1, 2.0, "sy", 2.1, 2.0,
                        "alpha", 1.0, 1.0,
                        NULL);
                _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 550, 550,
                        "sx", 2.1, 2.0, "sy", 2.1, 2.0,
                        "alpha", 0.0, 0.0,
                        NULL);

                _nemoshow_item_motion(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                        "sx", 0.0,
                        "alpha", 0.0,
                        NULL);
                int i = 0;
                List *l;
                struct showone *one;
                LIST_FOR_EACH_REVERSE(view->items, l, one) {
                    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 550, i * 150,
                            "tx", -(view->width/8.0) * 0.6 * 2,
                            "alpha", 1.0,
                            NULL);
                    i++;
                }
            } else {
                _nemoshow_item_motion_bounce(view->bell_circle, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                        "sx", 1.1, 1.0, "sy", 1.1, 1.0,
                        NULL);
                _nemoshow_item_motion_bounce(view->bell_bell, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                        "sx", 1.1, 1.0, "sy", 1.1, 1.0,
                        NULL);
                _nemoshow_item_motion_bounce(view->bell_quit, NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                        "sx", 1.1, 1.0, "sy", 1.1, 1.0,
                        NULL);
            }
        } else {
            _nemoshow_item_motion_bounce(one, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                    "sx", 1.1, 1.0, "sy", 1.1, 1.0,
                    NULL);
        }

        nemoshow_dispatch_frame(show);
        view->grab = NULL;
    }
}

static void _bell_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    BellView *view = userdata;

    if (nemoshow_event_is_down(show, event)) {
        if (!view->grab) {
            double ex, ey;
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event), &ex, &ey);

            struct showone *one;
            one = nemowidget_pick_one(widget, ex, ey);
            const char *id = nemoshow_one_get_id(one);

            if (id) {
                if (view->mode == 0) {
                    if (!strcmp(id, "bell")) {
                        view->grab = nemowidget_create_grab(widget, event,
                                _bell_view_grab_event, one);
                    }
                } else {
                    view->grab = nemowidget_create_grab(widget, event,
                                _bell_view_grab_call_event, one);
                }
            }
        }
    }
}

// XXX: Temporary
static int order_idx = 0;

static void _menu_view_grab_menu_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *one = userdata;
    Menu *menu = nemoshow_one_get_userdata(one);
    MenuView *view = menu->cat->menu;
    const char *id = nemoshow_one_get_id(one);
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        menu_down(menu, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        menu_up(menu, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0);
        if (nemoshow_event_is_single_click(show, event)) {
            // Show menus in the order board
            char id[PATH_MAX];
            int x, y, w, h;
            w = view->order_w/3;
            h = view->order_h/3;
            int ro = 0;

            if (order_idx%2 == 0) {
                int cnt = list_count(view->orders0);
                snprintf(id, PATH_MAX, "order%d_%d", 0, cnt);

                int gap_x = (view->order_w - w * 2)/3;
                int gap_y = (view->order_h - 30 - h * 2)/3;
                x = nemoshow_item_get_translate_x(view->order0) + view->order_w/2 - w/2;
                y = nemoshow_item_get_translate_y(view->order0) + view->order_h/2 - h/2;
                x += -(gap_x + w) * (cnt%2) - gap_x;
                y += -(gap_y + h) * (cnt/2) - gap_y - 30;
                ro = 180;

                if (cnt > 4) {
                    ERR("More than 4 count is not supported: %d", cnt);
                }
            } else {
                int cnt = list_count(view->orders1);
                snprintf(id, PATH_MAX, "order%d_%d", 1, cnt);

                int gap_x = (view->order_w - w * 2)/3;
                int gap_y = (view->order_h - 30 - h * 2)/3;
                x = nemoshow_item_get_translate_x(view->order1) - view->order_w/2 + w/2;
                y = nemoshow_item_get_translate_y(view->order1) - view->order_h/2 + h/2;
                x += (gap_x + w) * (cnt%2) + gap_x;
                y += (gap_y + h) * (cnt/2) + gap_y + 30;
                ro = 180;

                if (cnt > 4) {
                    ERR("More than 4 count is not supported: %d", cnt);
                }
            }

            Menu *dup = menu_dup(menu, id, w, h, ro);
            menu_show(dup, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
            menu_translate(dup, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, x, y);

            if (order_idx%2 == 0) {
                view->orders0 = list_append(view->orders0, dup);
            } else {
                view->orders1 = list_append(view->orders1, dup);
            }
            order_idx++;

            Dish *dish;
            LIST_FREE(view->dishes, dish)
                dish_hide_destroy(dish, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

            // Show menus in the dish
            if (menu->dish_uri) {
                int ww, hh;
                ww = 220;
                hh = 220;
                dish = dish_create(view->tool, view->dish_group,
                        ww, hh, menu->dish_uri, 180);
                dish_show(dish, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                dish_translate(dish, 0, 0, 0, 330, hh/2 + 25);
                view->dishes = list_append(view->dishes, dish);

                dish = dish_create(view->tool, view->dish_group,
                        ww, hh, menu->dish_uri, 180);
                dish_show(dish, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                dish_translate(dish, 0, 0, 0, 1100, hh/2 + 25);
                view->dishes = list_append(view->dishes, dish);

                dish = dish_create(view->tool, view->dish_group,
                        ww, hh, menu->dish_uri, 0);
                dish_show(dish, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                dish_translate(dish, 0, 0, 0, 330, view->height - (hh/2 + 25));
                view->dishes = list_append(view->dishes, dish);

                dish = dish_create(view->tool, view->dish_group,
                        ww, hh, menu->dish_uri, 0);
                dish_show(dish, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                dish_translate(dish, 0, 0, 0, 1100, view->height - (hh/2 + 25));
                view->dishes = list_append(view->dishes, dish);
            }
        }
        nemoshow_dispatch_frame(show);
        view->grab = NULL;
    }
}

static void _menu_view_grab_category_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showone *one = userdata;
    Category *cat = nemoshow_one_get_userdata(one);
    MenuView *view = cat->menu;
    const char *id = nemoshow_one_get_id(one);
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        category_down(cat, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        if (nemoshow_event_is_single_click(show, event)) {
            if (view->category_sel) {
                category_unselected(view->category_sel, NEMOEASE_CUBIC_INOUT_TYPE, 550, 0);

                List *l;
                Menu *menu;
                LIST_FOR_EACH(view->category_sel->menus, l, menu) {
                    menu_hide(menu, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                    menu_translate_x(menu, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, view->width + 200);
                }
            }

            view->category_sel = cat;
            category_selected(cat, NEMOEASE_CUBIC_INOUT_TYPE, 550, 0);

            int w = view->width/10;
            int hhh = 150;
            int menu_cnt = list_count(cat->menus);
            int menu_gap = (view->width - w - hhh * menu_cnt)/(menu_cnt + 1);

            int j = 0;
            List *l;
            Menu *menu;
            LIST_FOR_EACH(cat->menus, l, menu) {
                menu_show(menu, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                menu_translate_x(menu, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                        view->width - (menu_gap * (j + 1) + hhh * j + (hhh/2)));
                j++;
            }
        } else {
            category_up(cat, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0);
        }

        nemoshow_dispatch_frame(show);
        view->grab = NULL;
    }
}

static void _menu_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    MenuView *view = userdata;

    if (nemoshow_event_is_down(show, event)) {
        if (!view->grab) {
            double ex, ey;
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event), &ex, &ey);

            struct showone *one;
            one = nemowidget_pick_one(widget, ex, ey);
            const char *id = nemoshow_one_get_id(one);

            if (id) {
                if (strstr(id, "category_")) {
                    view->grab = nemowidget_create_grab(widget, event,
                            _menu_view_grab_category_event, one);
                } else if (strstr(id, "menu_")) {
                    view->grab = nemowidget_create_grab(widget, event,
                            _menu_view_grab_menu_event, one);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    Config *config;
    config = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "underlay");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    BistroView *bistro = bistro_view_create(win, config->width, config->height);
    bistro_view_show(bistro);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(config);

    return 0;
}
