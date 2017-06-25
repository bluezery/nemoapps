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
#include <nemojson.h>

#include "xemoapp.h"
#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define SAVER_TIMEOUT 600000
#define HONEY_CX 2750.0
#define HONEY_CY 1546.875


#define CONTENT "/opt/contents/karim"

typedef struct _KarimData KarimData;
struct _KarimData {
    char *country;
    char *category;
    char *year;
    char *title;
    char *path;
};

typedef struct _KarimGroup KarimGroup;
struct _KarimGroup {
    char *name;
    List *datas;
};

static List *karim_data_load(const char *root, List **countries, List **categories, List **years)
{
    List *files;
    files = fileinfo_readdir(root);

    List *datas = NULL;

    FileInfo *file;
    LIST_FREE(files, file) {
        if (fileinfo_is_dir(file)) {
            KarimData *data = calloc(sizeof(KarimData), 1);
            char *buf;
            char *tok;
            buf = strdup(file->name);
            tok = strtok(buf, "_");
            if (!tok) {
                ERR("ERR: no country : %s", file->path);
                continue;
            }
            data->country = strdup(tok);
            tok = strtok(NULL, "_");
            if (!tok) {
                ERR("ERR: no category : %s", file->path);
                continue;
            }
            data->category = strdup(tok);
            tok = strtok(NULL, "_");
            if (!tok) {
                ERR("ERR: no year : %s", file->path);
                continue;
            }
            data->year = strdup(tok);
            tok = strtok(NULL, "_");
            if (!tok) {
                ERR("ERR: no title : %s", file->path);
                continue;
            }
            data->title = strdup(tok);
            free(buf);

            data->path = strdup(file->path);

            bool done = false;
            List *l;
            KarimGroup *group;

            LIST_FOR_EACH(*countries, l, group) {
                if (!strcmp(data->country, group->name)) {
                    group->datas = list_append(group->datas, data);
                    done = true;
                    break;
                }
            }
            if (!done) {
                group = calloc(sizeof(KarimGroup), 1);
                group->name = strdup(data->country);
                group->datas = list_append(group->datas, data);
                *countries = list_append(*countries, group);
            }

            done = false;
            LIST_FOR_EACH(*categories, l, group) {
                if (!strcmp(data->category, group->name)) {
                    group->datas = list_append(group->datas, data);
                    done = true;
                    break;
                }
            }
            if (!done) {
                group = calloc(sizeof(KarimGroup), 1);
                group->name = strdup(data->category);
                group->datas = list_append(group->datas, data);
                *categories = list_append(*categories, group);
            }

            done = false;
            LIST_FOR_EACH(*years, l, group) {
                if (!strcmp(data->year, group->name)) {
                    group->datas = list_append(group->datas, data);
                    done = true;
                    break;
                }
            }
            if (!done) {
                group = calloc(sizeof(KarimGroup), 1);
                group->name = strdup(data->year);
                group->datas = list_append(group->datas, data);
                *years = list_append(*years, group);
            }

            datas = list_append(datas, data);
        }
    }

    return datas;
}

KarimGroup *karim_data_search_group(List *grps, const char *name)
{
    List *l;
    KarimGroup *grp;
    LIST_FOR_EACH(grps, l, grp) {
        if (!strcmp(grp->name, name)) {
            return grp;
        }
    }
    ERR("No countries: %s", name);
    return NULL;
}

typedef enum
{
    KARIM_TYPE_NONE = 0,
    KARIM_TYPE_MENU,
    KARIM_TYPE_INTRO,
    KARIM_TYPE_REGION,
    KARIM_TYPE_WORK,
    KARIM_TYPE_YEAR,
    KARIM_TYPE_HONEY,
    KARIM_TYPE_VIEWER,
} KarimType;

typedef struct _IntroView IntroView;
typedef struct _RegionView RegionView;
typedef struct _WorkView WorkView;
typedef struct _YearView YearView;
typedef struct _MenuView MenuView;
typedef struct _HoneyView HoneyView;
typedef struct _ViewerView ViewerView;
typedef struct _SaverView SaverView;
typedef struct _Karim Karim;

struct _Karim
{
    struct nemobus *bus;
    List *datas;
    List *countries;
    List *categories;
    List *years;

    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    const char *uuid;
    NemoWidget *parent;

    NemoWidget *widget;
    struct showone *bg;

    KarimType type;
    IntroView *intro;
    RegionView *region;
    WorkView *work;
    YearView *year;
    MenuView *menu;
    HoneyView *honey;
    ViewerView *viewer;
    //SaverView *saver;

    //struct nemotimer *saver_timer;
    NemoWidget *event_widget;
};

typedef enum {
    VIEWER_MODE_INTRO = 0,
    VIEWER_MODE_GALLERY,
} ViewerMode;

typedef struct _ViewerItem ViewerItem;
struct _ViewerItem {
    ViewerView *view;
    NemoWidget *widget0, *widget;
    struct showone *clip;
    Image *img0, *img1;
    struct showone *event;

    char *url;
    struct showone *btn_group;
    struct showone *btn_bg;
    struct showone *btn;
};

struct _ViewerView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;

    struct showone *group;

    NemoWidgetGrab *grab;
    ViewerMode mode;
    double gallery_x;
    List *items;

    NemoWidget *title_widget;
    struct showone *title_group;
    struct showone *title_bg;
    struct showone *title0;
    struct showone *title1;

    struct nemotimer *title_timer;
};

static void intro_view_hide(IntroView *view, uint32_t easetype, int duration, int delay);
static void intro_view_show(IntroView *view, uint32_t easetype, int duration, int delay);
static void menu_view_show(MenuView *view, uint32_t easetype, int duration, int delay);
static void menu_view_hide(MenuView *view, uint32_t easetype, int duration, int delay);
static void region_view_show(RegionView *view, uint32_t easetype, int duration, int delay);
static void region_view_hide(RegionView *view, uint32_t easetype, int duration, int delay);
static void work_view_show(WorkView *view, uint32_t easetype, int duration, int delay);
static void work_view_hide(WorkView *view, uint32_t easetype, int duration, int delay);
static void year_view_show(YearView *view, uint32_t easetype, int duration, int delay);
static void year_view_hide(YearView *view, uint32_t easetype, int duration, int delay);
static void honey_view_show(HoneyView *view, uint32_t easetype, int duration, int delay);
static void honey_view_hide(HoneyView *view, uint32_t easetype, int duration, int delay);
static void viewer_view_show(ViewerView *view, uint32_t easetype, int duration, int delay);
static void viewer_view_hide(ViewerView *view, uint32_t easetype, int duration, int delay);

static void karim_change_view(Karim *karim, KarimType type)
{
    if (type == karim->type) return;

    if (karim->type == KARIM_TYPE_NONE) {
        if (type == KARIM_TYPE_INTRO) {
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_INTRO) {
        if (type == KARIM_TYPE_MENU) {
            intro_view_hide(karim->intro, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);
            menu_view_show(karim->menu, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_MENU) {
        if (type == KARIM_TYPE_INTRO) {
            menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type ==  KARIM_TYPE_REGION) {
            region_view_show(karim->region, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else if (type == KARIM_TYPE_WORK) {
            work_view_show(karim->work, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else if (type == KARIM_TYPE_YEAR) {
            year_view_show(karim->year, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_REGION) {
        if (type == KARIM_TYPE_INTRO) {
            region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type == KARIM_TYPE_WORK) {
            region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            work_view_show(karim->work, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_YEAR) {
            region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            year_view_show(karim->year, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_HONEY) {
            menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            honey_view_show(karim->honey, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_WORK) {
        if (type == KARIM_TYPE_INTRO) {
            work_view_hide(karim->work, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type == KARIM_TYPE_REGION) {
            work_view_hide(karim->work, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            region_view_show(karim->region, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_YEAR) {
            work_view_hide(karim->work, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            year_view_show(karim->year, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_HONEY) {
            honey_view_show(karim->honey, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_YEAR) {
        if (type == KARIM_TYPE_INTRO) {
            year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type == KARIM_TYPE_REGION) {
            year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            region_view_show(karim->region, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_WORK) {
            year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            work_view_show(karim->work, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            karim->type = type;
        } else if (type == KARIM_TYPE_HONEY) {
            honey_view_show(karim->honey, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_HONEY) {
        if (type == KARIM_TYPE_INTRO) {
            honey_view_hide(karim->honey, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            work_view_hide(karim->work, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type == KARIM_TYPE_REGION ||
                type == KARIM_TYPE_WORK ||
                type == KARIM_TYPE_YEAR) {
            honey_view_hide(karim->honey, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
            karim->type = type;
        } else if (type == KARIM_TYPE_VIEWER) {
            viewer_view_show(karim->viewer, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    } else if (karim->type == KARIM_TYPE_VIEWER) {
        if (type == KARIM_TYPE_INTRO) {
            viewer_view_hide(karim->viewer, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            honey_view_hide(karim->honey, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            work_view_hide(karim->work, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 2000, 1000);
            karim->type = type;
        } else if (type == KARIM_TYPE_HONEY) {
            viewer_view_hide(karim->viewer, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
            karim->type = type;
        } else {
            ERR("not supported: %d -> %d", karim->type, type);
        }
    }
}

void viewer_item_translate(ViewerItem *item, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    nemowidget_translate(item->widget0, easetype, duration, delay, tx, ty);
    nemowidget_translate(item->widget, easetype, duration, delay, tx, ty);
}

void viewer_item_down(ViewerItem *item, uint32_t easetype, int duration, int delay)
{
    nemowidget_set_alpha(item->widget0, easetype, duration, delay, 0.5);
}

void viewer_item_up(ViewerItem *item, uint32_t easetype, int duration, int delay)
{
    nemowidget_set_alpha(item->widget0, easetype, duration, delay, 1.0);
}

void viewer_item_mode(ViewerItem *item, uint32_t easetype, int duration, int delay, ViewerMode mode)
{
    if (mode == VIEWER_MODE_INTRO) {
        nemowidget_show(item->widget0, 0, 0, 0);
        nemowidget_hide(item->widget, 0, 0, 0);
        nemowidget_set_alpha(item->widget0, easetype, duration, delay, 1.0);
        nemowidget_set_alpha(item->widget, easetype, duration, delay, 0.0);
    } else if (mode == VIEWER_MODE_GALLERY) {
        nemowidget_hide(item->widget0, 0, 0, 0);
        nemowidget_show(item->widget, 0, 0, 0);
        nemowidget_set_alpha(item->widget0, easetype, duration, delay, 0.0);
        nemowidget_set_alpha(item->widget, easetype, duration, delay, 1.0);
    }
}

void viewer_view_mode(ViewerView *view, ViewerMode mode, ViewerItem *modeitem)
{
    if (view->mode == !!mode) return;

    view->mode = mode;
    if (mode == VIEWER_MODE_INTRO) {
        int cnt = list_count(view->items);
        double w = view->w/cnt;
        view->gallery_x = -(cnt - 1) * (w/2);
        int i = 0;

        List *l;
        ViewerItem *item;
        LIST_FOR_EACH(view->items, l, item) {
            viewer_item_mode(item, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, mode);
            viewer_item_translate(item, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    view->gallery_x + w * i, 0);
            i++;
        }
    } else if (mode == VIEWER_MODE_GALLERY) {
        int id = list_get_idx(view->items, modeitem);
        view->gallery_x = -view->w * ((int)id);

        int i = 0;
        List *l;
        ViewerItem *item;
        LIST_FOR_EACH(view->items, l, item) {
            viewer_item_mode(item, NEMOEASE_CUBIC_OUT_TYPE, 1500, 0, mode);
            viewer_item_translate(item, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    view->gallery_x + view->w * i, 0);
            i++;
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void _viewer_item_btn_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);

    ViewerItem *item = userdata;
    ViewerView *view = item->view;

    if (nemoshow_event_is_down(show, event)) {
        _nemoshow_item_motion_bounce(item->btn_group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 1.5, 1.25, "sy", 1.5, 1.25, NULL);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        view->grab = NULL;
        _nemoshow_item_motion_bounce(item->btn_group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 0.8, 1.0, "sy", 0.8, 1.0, NULL);

        if (nemoshow_event_is_single_click(show, event)) {
            char path[PATH_MAX];
            char name[PATH_MAX];
            char args[PATH_MAX];

            snprintf(name, PATH_MAX, "/usr/bin/electron");
            snprintf(path, PATH_MAX, "%s", name);
            snprintf(args, PATH_MAX, "%s", item->url);

            ERR("(%s) (%s) (%s)", path, name, item->url);
            nemo_execute(view->karim->uuid, "xapp", path, args, "off",
                    view->w/2.0, view->h/2.0, 0, 1, 1);
        }
        nemoshow_dispatch_frame(show);
    }
}

static void _viewer_item_gallery_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    ViewerItem *item = userdata;
    ViewerView *view = item->view;

    double ex;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, NULL);
    double gx;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_grab_x(event),
            nemoshow_event_get_grab_y(event), &gx, NULL);

    if (nemoshow_event_is_motion(show, event)) {
        int i = 0;
        List *l;
        ViewerItem *item;
        LIST_FOR_EACH(view->items, l, item) {
            viewer_item_translate(item, 0, 0, 0,
                    view->gallery_x + view->w * i + ex - gx, 0);
            i++;
        }
        nemoshow_dispatch_frame(view->show);
    } else if (nemoshow_event_is_up(show, event)) {
        view->grab = NULL;
        if (nemoshow_event_is_single_click(show, event)) {
            int rx = view->w/6;
            if (ex < rx || ex > (view->w - rx)) {
                viewer_view_mode(view, VIEWER_MODE_INTRO, NULL);
            }
        } else {
            int tx = ex - gx;
            if (tx > view->w/4) {
                view->gallery_x += view->w;
            } else if (tx < -view->w/4) {
                view->gallery_x -= view->w;
            }

            int cnt = list_count(view->items);
            if (view->gallery_x > 0) view->gallery_x = 0;
            else if (view->gallery_x < (double)-(cnt - 1) * view->w)
                view->gallery_x = -(cnt - 1) * view->w;

            int i = 0;
            List *l;
            ViewerItem *item;
            LIST_FOR_EACH(view->items, l, item) {
                viewer_item_translate(item, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        view->gallery_x + view->w * i, 0);
                i++;
            }
            nemoshow_dispatch_frame(view->show);
        }
    }
}

static void _viewer_item_gallery_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    ViewerItem *item = userdata;
    ViewerView *view = item->view;

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);

        if (!view->grab) {
            if (one) {
                view->grab = nemowidget_create_grab(widget, event,
                        _viewer_item_btn_grab_event, item);
            } else {
                view->grab = nemowidget_create_grab(widget, event,
                        _viewer_item_gallery_grab_event, item);
            }
        }
    }
}

static void _viewer_item_clip_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);

    ViewerItem *item = userdata;
    ViewerView *view = item->view;

    if (nemoshow_event_is_down(show, event)) {
        viewer_item_down(item, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        view->grab = NULL;
        viewer_item_up(item, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
        if (nemoshow_event_is_single_click(show, event)) {
            viewer_view_mode(view, VIEWER_MODE_GALLERY, item);
        }
        nemoshow_dispatch_frame(show);
    }
}

static void _viewer_item_clip_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    ViewerItem *item = userdata;
    ViewerView *view = item->view;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        struct showone *one;
        one = nemowidget_pick_one(widget, ex, ey);
        if (!view->grab && one) {
            view->grab = nemowidget_create_grab(widget, event,
                    _viewer_item_clip_grab_event, item);
        }
    }
}

ViewerItem *viewer_view_create_item(ViewerView *view, NemoWidget *parent,
        const char *uri, const char *url, double width)
{
    ViewerItem *item = calloc(sizeof(ViewerItem), 1);
    item->view = view;
    if (url) item->url = strdup(url);

    int w, h;
    file_get_image_wh(uri, &w, &h);
    _rect_ratio_fit(w, h, view->w, view->h, &w, &h);

    NemoWidget *widget;
    struct showone *canvas;
    item->widget0 = widget = nemowidget_create_vector(parent, w, h);
    nemowidget_append_callback(widget, "event", _viewer_item_clip_event, item);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    canvas = nemowidget_get_canvas(widget);

    // XXX: design clip as center aligned
    struct showone *clip;
    item->clip = clip = PATH_CREATE(NULL);
    nemoshow_item_path_moveto(clip, (w - width)/2, 0);
    nemoshow_item_path_lineto(clip, (w - width)/2 + width, 0);
    nemoshow_item_path_lineto(clip, (w - width)/2 + width, view->h);
    nemoshow_item_path_lineto(clip, (w - width)/2, view->h);
    nemoshow_item_path_lineto(clip, (w - width)/2, 0);
    nemoshow_item_path_close(clip);

    struct showone *one;
    item->event = one = RECT_CREATE(canvas, width, view->h);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, view->w/2 - width/2, 0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, item);

    Image *img;
    item->img0 = img = image_create(canvas);
    image_load_fit(img, view->tool, uri, view->w, view->h, NULL, NULL);
    image_set_anchor(img, 0.5, 0.5);
    image_translate(img, 0, 0, 0, view->w/2, view->h/2);
    image_set_clip(img, clip);

    item->widget = widget = nemowidget_create_vector(parent, w, h);
    nemowidget_append_callback(widget, "event", _viewer_item_gallery_event, item);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    canvas = nemowidget_get_canvas(widget);

    item->img1 = img = image_create(canvas);
    image_load_fit(img, view->tool, uri, view->w, view->h, NULL, NULL);

    if (url) {
        double sx, sy;
        double x, y;
        sx = view->w/1920.0;
        sy = view->h/1080.0;

        struct showone *group;
        item->btn_group = group = GROUP_CREATE(canvas);

        double ww, hh;
        const char *uri;
        uri = APP_ICON_DIR"/viewer/3d button-background.svg";
        svg_get_wh(uri, &ww, &hh);
        ww = ww * sx;
        hh = hh * sy;
        item->btn_bg = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
        nemoshow_one_set_userdata(one, item);

        x = view->w - ww;
        y = view->h - hh;
        nemoshow_item_translate(group, x, y);

        x = 0;
        y = 0;
        uri = APP_ICON_DIR"/viewer/3d button-3d.svg";
        svg_get_wh(uri, &ww, &hh);
        ww = ww * sx;
        hh = hh * sy;
        item->btn = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
    }

    return item;
}

static void _viewer_view_title_timeout(struct nemotimer *timer, void *userdata)
{
    ViewerView *view = userdata;
    uint32_t easetype = NEMOEASE_CUBIC_IN_TYPE;
    int duration = 1000;
    int delay = 0;
    _nemoshow_item_motion(view->title_group, easetype, duration, delay + 250 + 250,
            "tx", 0.0, "ty", 0.0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(view->title0, easetype, duration, delay + 250,
            "sx", 0.0, "sy", 0.8, NULL);
    _nemoshow_item_motion(view->title1, easetype, duration, delay,
            "sx", 0.0, "sy", 0.8, NULL);
    nemoshow_dispatch_frame(view->show);
}

static ViewerView *viewer_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    ViewerView *view = calloc(sizeof(ViewerView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *group;
    struct showone *one;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    int cnt = 4;
    int i;
    for (i = 1 ; i <= cnt ; i++) {
        char uri[PATH_MAX];
        snprintf(uri, PATH_MAX, "%s/viewer/%d.png", APP_IMG_DIR, i);

        ViewerItem *item;
        const char *url = NULL;
        if (i == 2) url = APP_LINK_DIR"/2";
        item = viewer_view_create_item(view, parent, uri, url, (double)view->w/cnt);
        view->items = list_append(view->items, item);
    }

    view->title_widget = widget = nemowidget_create_vector(parent, width, height);
    view->title_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    nemoshow_item_set_alpha(group, 0.0);

    double sx, sy;
    sx = view->w/1920.0;
    sy = view->h/1080.0;

    double x, y;
    double ww, hh;
    const char *uri;
    x = 0;
    y = 0;
    uri = APP_ICON_DIR"/viewer/pink background.svg";
    svg_get_wh(uri, &ww, &hh);
    // Designed for 1920x1080
    ww = ww * sx;
    hh = hh * sy;
    x = x * sx;
    y = y * sy;
    view->title_bg = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_set_alpha(one, 0.5);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);

    x = 70;
    y = 70;
    uri = APP_ICON_DIR"/viewer/title.svg";
    svg_get_wh(uri, &ww, &hh);
    // Designed for 1920x1080
    ww = ww * sx;
    hh = hh * sy;
    x = x * sx;
    y = y * sy;
    view->title0 = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.8);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);

    x = 100;
    y = 370;
    uri = APP_ICON_DIR"/viewer/subtitle.svg";
    svg_get_wh(uri, &ww, &hh);
    // Designed for 1920x1080
    ww = ww * sx;
    hh = hh * sy;
    x = x * sx;
    y = y * sy;
    view->title1 = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.8);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);

    view->title_timer = TOOL_ADD_TIMER(view->tool, 0, _viewer_view_title_timeout, view);

    return view;
}

static void viewer_view_show(ViewerView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 1.0);

    int cnt = list_count(view->items);
    double w = view->w/cnt ;
    double start = -(cnt - 1) * (w/2);
    int i = 0;

    List *l;
    ViewerItem *item;
    LIST_FOR_EACH(view->items, l, item) {
        viewer_item_mode(item, easetype, duration, delay, VIEWER_MODE_INTRO);
        viewer_item_translate(item, easetype, duration, delay, start + w * i, 0);
        i++;
    }

    nemotimer_set_timeout(view->title_timer, 5000);
    _nemoshow_item_motion(view->title_group, easetype, duration, delay + 500,
            "tx", 50.0, "ty", 50.0,
            "alpha", 1.0, NULL);
    _nemoshow_item_motion(view->title0, easetype, duration, delay + 500,
            "sx", 1.0, "sy", 1.0, NULL);
    _nemoshow_item_motion(view->title1, easetype, duration, delay + 500 + 500,
            "sx", 1.0, "sy", 1.0, NULL);
    nemoshow_dispatch_frame(view->show);
}

static void viewer_view_hide(ViewerView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 0.0);

    List *l;
    ViewerItem *item;
    LIST_FOR_EACH(view->items, l, item) {
        viewer_item_mode(item, easetype, duration, delay, VIEWER_MODE_INTRO);
        viewer_item_translate(item, easetype, duration, delay, 0, 0);
    }

    nemotimer_set_timeout(view->title_timer, 0);
    _nemoshow_item_motion(view->title_group, easetype, duration, delay + 500,
            "tx", 0.0, "ty", 0.0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(view->title0, easetype, duration, delay + 500,
            "sx", 0.0, "sy", 0.8, NULL);
    _nemoshow_item_motion(view->title1, easetype, duration, delay + 500 + 500,
            "sx", 0.0, "sy", 0.8, NULL);
    nemoshow_dispatch_frame(view->show);
}

typedef struct _HoneyItem HoneyItem;
struct _HoneyItem {
    HoneyView *view;
    int x, y, w, h;
    char *path;
    struct showone *group;
    Image *img;
    struct showone *one_sel;
};

struct _HoneyView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *bg_widget;
    Image *bg0;

    NemoWidget *widget;

    int widget_w, widget_h;
    double widget_x, widget_y;
    NemoWidgetGrab *item_grab;
    NemoWidgetGrab *scroll_grab;
    int bg_ix, bg_iy;
    Image *bg;
    struct showone *title;

    List *items;
};

static void honey_item_down(HoneyItem *it)
{
    image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.0);
    _nemoshow_item_motion(it->one_sel, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0,
            NULL);
}

static void honey_item_up(HoneyItem *it)
{
    image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 1.0);
    _nemoshow_item_motion(it->one_sel, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0,
            NULL);
}

static List *fileinfo_readdir_img(const char *path)
{
    List *l, *ll;
    List *files = fileinfo_readdir(path);
    FileInfo *file;
    LIST_FOR_EACH_SAFE(files, l, ll, file) {
        if (!fileinfo_is_image(file)) {
            // FIXME: fail to get magic string for some images (e.g. TIFF, name user count (xx) exceeded)
            if (!fileinfo_is_image_ext(file)) {
                files = list_remove(files, file);
            }
        }
    }
    return files;
}

static HoneyItem *honey_view_create_item(HoneyView *view, const char *path, double x, double y, double w, double h, double sx, double sy)
{
    List *files = fileinfo_readdir_img(path);
    RET_IF(!files, NULL);
    FileInfo *file = LIST_DATA(LIST_FIRST(files));

    struct showone *group;
    struct showone *one;

    struct showone *canvas;
    canvas = nemowidget_get_canvas(view->widget);

    HoneyItem *it = calloc(sizeof(HoneyItem), 1);
    it->view = view;
    it->group = group = GROUP_CREATE(canvas);
    it->x = x * sx;
    it->y = y * sy;
    nemoshow_item_translate(it->group, it->x, it->y);


    char uri[PATH_MAX];
    snprintf(uri, PATH_MAX, "%s", file->path);
    it->w = w * sx;
    it->h = h * sy;
    Image *img;
    it->img = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_load_fit(img, view->tool, uri, it->w, it->h, NULL, NULL);

    double ww, hh;
    snprintf(uri, PATH_MAX, "%s", APP_ICON_DIR"/honey/icon-selected.svg");
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    it->one_sel = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_alpha(one, 0.0);

    return it;
}

static void honey_view_show(HoneyView *view, uint32_t easetype, int duration, int delay)
{
    RET_IF(!view);
    nemowidget_show(view->bg_widget, 0, 0, 0);
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration + 1000, delay + 1000, 1.0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    nemoshow_dispatch_frame(view->show);
}

static void honey_view_hide(HoneyView *view, uint32_t easetype, int duration, int delay)
{
    RET_IF(!view);
    nemowidget_hide(view->bg_widget, 0, 0, 0);
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration, delay, 0.0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    nemoshow_dispatch_frame(view->show);
}

static void _honey_view_item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    HoneyItem *it = nemoshow_one_get_userdata(one);
    RET_IF(!it);
    HoneyView *view = it->view;

    if (nemoshow_event_is_down(show, event)) {
        honey_item_down(it);
        nemoshow_dispatch_frame(view->show);
    } else if (nemoshow_event_is_up(show, event)) {
        honey_item_up(it);
        view->item_grab = NULL;

        if (nemoshow_event_is_single_click(show, event)) {
            //karim_change_view(view->karim, KARIM_TYPE_VIEWER);

            // Zooming effect
            double scale = 1.0;
            double sx, sy;
            double ix, iy, iw, ih;
            sx = view->w/3840.0;
            sy = view->h/2160.0;
            ix = it->x * scale;
            iy = it->y * scale;
            iw = it->w * scale;
            ih = it->h * scale;

            /*
            nemowidget_translate(view->widget, NEMOEASE_CUBIC_IN_TYPE, 1000, 0,
                    -(ix - (view->w - iw)/2.0), -(iy - (view->h - ih)/2.0));
                    */
            /*
            nemowidget_scale(view->widget, NEMOEASE_CUBIC_IN_TYPE, 1000, 0,
                    scale, scale);
                    */
        }
        nemoshow_dispatch_frame(view->show);
    }
}

static void _honey_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    HoneyView *view = userdata;

    if (nemoshow_event_is_down(show, event)) {
        double widget_x, widget_y;
        nemowidget_get_geometry(view->widget, &view->widget_x, &view->widget_y, NULL, NULL);
    } else if (nemoshow_event_is_motion(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        double gx, gy;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_grab_x(event),
                nemoshow_event_get_grab_y(event), &gx, &gy);
        double tx, ty;
        double tw, th;
        tx = view->widget_x + ex - gx;
        ty = view->widget_y + ey - gy;
        tw = view->widget_w - view->w;
        th = view->widget_h - view->h;
        if (tx > 0) {
            tx = 0;
        } else if (tx < -tw) {
            tx = -tw;
        }
        if (ty > 0) {
            ty = 0;
        } else if (ty < -th) {
            ty = -th;
        }
        nemowidget_translate(view->widget, 0, 0, 0, tx, ty);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        double widget_x, widget_y;
        nemowidget_get_geometry(view->widget, &widget_x, &widget_y, NULL, NULL);

        view->scroll_grab = NULL;

        double gap_x, gap_y;
        gap_x = 100;
        gap_y = 100;

        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        double gx, gy;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_grab_x(event),
                nemoshow_event_get_grab_y(event), &gx, &gy);
        double tx, ty;
        double tw, th;
        tx = widget_x + ex - gx;
        ty = widget_y + ey - gy;
        tw = view->widget_w - view->w;
        th = view->widget_h - view->h;
        if (tx > -gap_x) {
            tx = -gap_x;
        } else if (tx < -tw + gap_x) {
            tx = -tw + gap_x;
        }
        if (ty > -gap_y) {
            ty = -gap_y;
        } else if (ty < -th + gap_y) {
            ty = -th + gap_y;
        }

        nemowidget_translate(view->widget, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0, tx, ty);
        nemoshow_dispatch_frame(show);
    }
}

static void _honey_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    HoneyView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);

        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->item_grab) {
            view->item_grab = nemowidget_create_grab(widget, event,
                    _honey_view_item_grab_event, one);
        }
        if (!view->scroll_grab) {
            view->scroll_grab = nemowidget_create_grab(widget, event,
                    _honey_view_grab_event, view);
        }
    }
}

static HoneyView *honey_view_create(Karim *karim, NemoWidget *parent, int width, int height, KarimGroup *grp)
{
    HoneyView *view = calloc(sizeof(HoneyView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    double sx, sy;
    sx = view->w/3840.0;
    sy = view->h/2160.0;

    NemoWidget *widget;
    view->bg_widget = widget = nemowidget_create_vector(parent, view->w, view->h);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    const char *uri;
    uri = APP_IMG_DIR"/honey/background0.png";

    Image *img;
    view->bg0 = img = image_create(nemowidget_get_canvas(widget));
    image_load_full(img, view->tool, uri, view->w, view->h, NULL, NULL);

    int w, h;
    uri = APP_IMG_DIR"/honey/background.png";
    file_get_image_wh(uri, &w, &h);

    view->widget_w = w = w * sx;
    view->widget_h = h = h * sy;
    view->bg_ix = (w - view->w)/2;
    view->bg_iy = (h - view->h)/2;

    view->widget = widget = nemowidget_create_vector(parent, w, h);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_translate(widget, 0, 0, 0, -view->bg_ix, -view->bg_iy);
    nemowidget_append_callback(widget, "event", _honey_view_event, view);
    view->widget_x = -view->bg_ix;
    view->widget_y = -view->bg_iy;

    struct showone *canvas;
    struct showone *one;
    canvas = nemowidget_get_canvas(widget);

    view->bg = img = image_create(canvas);
    image_load_full(img, view->tool, uri, w, h, NULL, NULL);
    //image_translate(img, 0, 0, 0, -view->bg_ix, -view->bg_iy);

    double x, y;
    double ww, hh;
    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%s/.karim/%s.svg", CONTENT, grp->name);
    svg_get_wh(buf, &ww, &hh);
    x = HONEY_CX * sx;
    y = HONEY_CY * sy;
    ww = ww * sx;
    hh = hh * sy;
    /*
    ww = 259.219 * sx;
    hh = 224.176 * sy;
    */
    view->title = one = SVG_PATH_GROUP_CREATE(canvas, ww, hh, buf);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, x, y);

    double xx, yy;
    xx = 0;
    yy = 0;
    double r;
    r = 223.375;
    int layer = 1;
    int layer_cnt = 0;
    int cnt = 0;

    KarimData *data;
    List *l;
    LIST_FOR_EACH(grp->datas, l, data) {
        if (layer_cnt == 0) {
            if (cnt == 0) {
                xx = HONEY_CX;
                yy = layer * r + HONEY_CY;
            }
            xx = r * cos(-150.0 * (M_PI/180.0)) + xx;
            yy = r * sin(-150.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else if (layer_cnt == 1) {
            xx = r * cos(-90.0 * (M_PI/180.0)) + xx;
            yy = r * sin(-90.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else if (layer_cnt == 2) {
            xx = r * cos(-30.0 * (M_PI/180.0)) + xx;
            yy = r * sin(-30.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else if (layer_cnt == 3) {
            xx = r * cos(30.0 * (M_PI/180.0)) + xx;
            yy = r * sin(30.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else if (layer_cnt == 4) {
            xx = r * cos(90.0 * (M_PI/180.0)) + xx;
            yy = r * sin(90.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else if (layer_cnt == 5) {
            xx = r * cos(150.0 * (M_PI/180.0)) + xx;
            yy = r * sin(150.0 * (M_PI/180.0)) + yy;
            cnt++;
        } else {
            ERR("something wrong!!: %d", layer_cnt);
        }

        HoneyItem *it = honey_view_create_item(view, data->path, xx, yy, 140, 140, sx, sy);
        view->items = list_append(view->items, it);
        if (cnt >= layer) {
            cnt = 0;
            layer_cnt++;
        }
        if (layer_cnt >= 6) {
            layer_cnt= 0;
            layer++;
        }
    }

    return view;
}

struct _YearView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;

    NemoWidget *bg_widget;
    struct showone *bg_group;
    int bg_w;
    int bg_cnt_x;
    int bg_idx;
    struct nemotimer *bg_timer;
    List *bgs;

    NemoWidget *widget;
    NemoWidgetGrab *grab;
    struct nemotimer *btn_timer;

    struct showone *btn_group;
    struct showone *btn_bg;

    List *mains;
};

typedef struct _YearSub YearSub;
struct _YearSub {
    double bg_x, bg_y;
    const char *bg;
    double txt_x, txt_y;
    const char *txt;
};

YearSub YEAR_SUB0[] = {
    {399.08, 345.81, APP_ICON_DIR"/year/year-subcircle.svg",
     335.57, 309.96, APP_ICON_DIR"/year/1994.svg"},
    {420.47, 280.59, APP_ICON_DIR"/year/year-subcircle.svg",
     366.02, 230.02, APP_ICON_DIR"/year/1995.svg"},
    {471.74, 234.57, APP_ICON_DIR"/year/year-subcircle.svg",
     443.66, 166.76, APP_ICON_DIR"/year/1996.svg"},
    {539.16, 220.36, APP_ICON_DIR"/year/year-subcircle.svg",
     546.76, 152.51, APP_ICON_DIR"/year/1997.svg"},
    {604.64, 241.76, APP_ICON_DIR"/year/year-subcircle.svg",
     633.45, 166.19, APP_ICON_DIR"/year/1998.svg"},
};

YearSub YEAR_SUB1[] = {
    {586.6, 591.81, APP_ICON_DIR"/year/year-subcircle.svg",
     504.06, 607.5, APP_ICON_DIR"/year/1999.svg"},
    {595.33, 660.6, APP_ICON_DIR"/year/year-subcircle.svg",
     523.82, 691.26, APP_ICON_DIR"/year/2000.svg"},
    {637.07, 715.42, APP_ICON_DIR"/year/year-subcircle.svg",
     591.65, 762.98, APP_ICON_DIR"/year/2001.svg"},
    {700.61, 741.81, APP_ICON_DIR"/year/year-subcircle.svg",
     686.69, 797.26, APP_ICON_DIR"/year/2002.svg"},
    {768.95, 733.29, APP_ICON_DIR"/year/year-subcircle.svg",
     782.62, 793.77, APP_ICON_DIR"/year/2003.svg"},
};

YearSub YEAR_SUB2[] = {
    {755.02, 280.8, APP_ICON_DIR"/year/year-subcircle.svg",
     704.76, 221.16, APP_ICON_DIR"/year/2004.svg"},
    {793.34, 223.55, APP_ICON_DIR"/year/year-subcircle.svg",
     753.27, 151.45, APP_ICON_DIR"/year/2005.svg"},
    {855.14, 193.11, APP_ICON_DIR"/year/year-subcircle.svg",
     849.73, 111.17, APP_ICON_DIR"/year/2006.svg"},
    {923.88, 197.66, APP_ICON_DIR"/year/year-subcircle.svg",
     943.74, 121.4, APP_ICON_DIR"/year/2007.svg"},
    {985, 229.9, APP_ICON_DIR"/year/year-subcircle.svg",
     1023.4, 159.1, APP_ICON_DIR"/year/2008.svg"},
};

YearSub YEAR_SUB3[] = {
    {1273.33,  655.73, APP_ICON_DIR"/year/year-subcircle.svg",
     1328.85, 675.29, APP_ICON_DIR"/year/2009.svg"},
    {1284.7, 723.68, APP_ICON_DIR"/year/year-subcircle.svg",
     1340.84, 750.79, APP_ICON_DIR"/year/2010.svg"},
    {1260.58,  788.21, APP_ICON_DIR"/year/year-subcircle.svg",
     1305.95, 824.71, APP_ICON_DIR"/year/2011.svg"},
    {1207.42,  832.02, APP_ICON_DIR"/year/year-subcircle.svg",
     1238.44, 877.22, APP_ICON_DIR"/year/2012.svg"},
    {1139.47,  843.4, APP_ICON_DIR"/year/year-subcircle.svg",
     1158.25, 905.47, APP_ICON_DIR"/year/2013.svg"},
};

YearSub YEAR_SUB4[] = {
    {1338.42, 214.77, APP_ICON_DIR"/year/year-subcircle.svg",
     1374.39, 157.14, APP_ICON_DIR"/year/2014.svg"},
    {1418.46, 249.49, APP_ICON_DIR"/year/year-subcircle.svg",
     1463.69, 211.7, APP_ICON_DIR"/year/2015.svg"},
    {1462.42, 330.68, APP_ICON_DIR"/year/year-subcircle.svg",
     1515.54, 344.46, APP_ICON_DIR"/year/2016.svg"},
    {1438.54, 422.41, APP_ICON_DIR"/year/year-subcircle.svg",
     1495.22, 450.55, APP_ICON_DIR"/year/2017.svg"},
};

typedef struct _YearSubItem YearSubItem;

struct _YearSubItem {
    YearView *view;
    struct showone *group;
    struct showone *bg;
    struct showone *txt;
};


YearSubItem *year_view_create_sub_item(YearView *view, const char *bg_path, double bg_x, double bg_y, const char *txt_path, double txt_x, double txt_y, double sx, double sy)
{
    double x, y, ww, hh;
    struct showone *group;
    struct showone *one;

    YearSubItem *it = calloc(sizeof(YearSubItem), 1);
    it->view = view;
    it->group = group = GROUP_CREATE(view->btn_group);

    x = bg_x * sx;
    y = bg_y * sy;

    const char *uri;
    uri = bg_path;
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    it->bg = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "year_sub");
    nemoshow_one_set_userdata(one, it);

    x = txt_x * sx;
    y = txt_y * sy;

    uri = txt_path;
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    it->txt = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "year_sub");
    nemoshow_one_set_userdata(one, it);

    return it;
}

static void year_sub_item_show(YearSubItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion_bounce(it->bg, easetype, duration, delay,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
    _nemoshow_item_motion_bounce(it->txt, easetype, duration, delay + 250 * 2,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0,
            "ro", 30.0, 0.0,
            NULL);
}

static void year_sub_item_hide(YearSubItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(it->bg, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion(it->txt, easetype, duration, delay + 250 * 2,
            "sx", 0.0, "sy", 0.0,
            "ro", -30.0, 0.0,
            NULL);
}

static void year_sub_item_down(YearSubItem *it)
{
    _nemoshow_item_motion_bounce(it->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.3, 1.2, "sy", 1.3, 1.2,
            NULL);
    _nemoshow_item_motion_bounce(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.3, 1.2, "sy", 1.3, 1.2,
            NULL);
}

static void year_sub_item_up(YearSubItem *it)
{
    _nemoshow_item_motion_bounce(it->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.9, 1.0, "sy", 0.9, 1.0,
            NULL);
    _nemoshow_item_motion_bounce(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.9, 1.0, "sy", 0.9, 1.0,
            NULL);
}

typedef struct _YearMain YearMain;
struct _YearMain {
    double x, y;
    const char *btn;
    const char *btn_sel;

    double arch_x, arch_y;
    const char *arch;
    YearSub *sub;
    int sub_cnt;
};

YearMain YEAR_MAIN[] = {
    {477.8, 296.4,
        APP_ICON_DIR"/year/year-button-1994_1998.svg", APP_ICON_DIR"/year/year-button-1994_1998-selected.svg",
        416., 241.14, APP_ICON_DIR"/year/year-subline-1994_1998.svg", YEAR_SUB0, 5},
    {663.01, 507.33,
        APP_ICON_DIR"/year/year-button-1999_2003.svg", APP_ICON_DIR"/year/year-button-1999_2003-selected.svg",
        605.17, 617.26, APP_ICON_DIR"/year/year-subline-1999_2003.svg", YEAR_SUB1, 5},
    {819.38, 273.21,
        APP_ICON_DIR"/year/year-button-2004_2008.svg", APP_ICON_DIR"/year/year-button-2004_2008-selected.svg",
        778.5, 207.4, APP_ICON_DIR"/year/year-subline-2004_2008.svg", YEAR_SUB2, 5},
    {1016.4, 592.0,
        APP_ICON_DIR"/year/year-button-2009_2013.svg", APP_ICON_DIR"/year/year-button-2009_2013-selected.svg",
        1165.19, 682.88, APP_ICON_DIR"/year/year-subline-2009_2013.svg", YEAR_SUB3, 5},
    {1220.51, 281.8,
        APP_ICON_DIR"/year/year-button-2014_2017.svg", APP_ICON_DIR"/year/year-button-2014_2017-selected.svg",
        1367.23, 236.25, APP_ICON_DIR"/year/year-subline-2014_2017.svg", YEAR_SUB4, 4}
};

typedef struct _YearMainItem YearMainItem;
struct _YearMainItem {
    YearView *view;
    struct showone *group;
    struct showone *bg;
    struct showone *btn;
    struct showone *btn_sel;
    struct showone *arch;

    List *items;
};

YearMainItem *year_view_create_main_item(YearView *view, const char *btn_path, const char *btn_sel_path, double btn_x, double btn_y, const char *arch, double arch_x, double arch_y, YearSub *sub, int cnt, double sx, double sy)
{
    double x, y, ww, hh;
    struct showone *group;
    struct showone *one;

    YearMainItem *it = calloc(sizeof(YearMainItem), 1);
    it->view = view;
    it->group = group = GROUP_CREATE(view->btn_group);

    x = btn_x * sx;
    y = btn_y * sy;

    const char *uri;
    uri = btn_path;
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;

    it->bg = one = CIRCLE_CREATE(group, ww/2);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_fill_color(one, RGBA(0xe11A8EFF));
    nemoshow_item_translate(one, x + ww/2, y + hh/2);

    it->btn = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "year_main");
    nemoshow_one_set_userdata(one, it);

    uri = btn_sel_path;
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    it->btn_sel = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    uri = arch;
    x = arch_x * sx;
    y = arch_y * sy;
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    it->arch = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x + ww/2, y + hh/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 1.0);

    int i;
    for (i = 0 ; i < cnt ; i++) {
        YearSubItem *sub_it;
        sub_it = year_view_create_sub_item(it->view, sub[i].bg, sub[i].bg_x, sub[i].bg_y,
                sub[i].txt, sub[i].txt_x, sub[i].txt_y, sx, sy);
        it->items = list_append(it->items, sub_it);
    }

    return it;
}

static void year_main_item_show(YearMainItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(it->bg, easetype, duration, delay, "alpha", 1.0, NULL);
    _nemoshow_item_motion_bounce(it->btn, easetype, duration, delay + 500 + delay,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
}

static void year_main_item_hide(YearMainItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(it->bg, easetype, duration, delay, "alpha", 0.0, NULL);
    _nemoshow_item_motion(it->btn, easetype, duration, delay,
            "alpha", 1.0,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion(it->btn_sel, easetype, duration, delay,
            "alpha", 0.0,
            "sx", 0.0, "sy", 0.0, NULL);

    _nemoshow_item_motion(it->arch, easetype, duration, delay,
            "sx", 0.0, "sy", 1.0,
            NULL);
    List *l;
    YearSubItem *subit;
    LIST_FOR_EACH(it->items, l, subit) {
        year_sub_item_hide(subit, easetype, duration, delay);
    }
}

static void year_main_item_down(YearMainItem *it)
{
    _nemoshow_item_motion_bounce(it->btn, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, 0.0,
            "sx", 1.3, 1.2, "sy", 1.3, 1.2,
            NULL);
    _nemoshow_item_motion_bounce(it->btn_sel, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.3, 1.2, "sy", 1.3, 1.2,
            "alpha", 1.0, 1.0,
            NULL);
}

static void year_main_item_up(YearMainItem *it)
{
    _nemoshow_item_motion_bounce(it->btn, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, 1.0,
            "sx", 0.8, 1.0, "sy", 0.8, 1.0,
            NULL);
    _nemoshow_item_motion_bounce(it->btn_sel, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.8, 1.0, "sy", 0.8, 1.0,
            "alpha", 0.0, 0.0,
            NULL);
}

static void year_main_item_click(YearMainItem *it)
{
    _nemoshow_item_motion_bounce(it->btn, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, 0.0,
            "sx", 0.8, 1.0, "sy", 0.8, 1.0,
            NULL);
    _nemoshow_item_motion_bounce(it->btn_sel, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, 1.0,
            "sx", 0.8, 1.0, "sy", 0.8, 1.0,
            NULL);

    _nemoshow_item_motion_bounce(it->arch, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0,
            NULL);
    uint32_t easetype = NEMOEASE_CUBIC_INOUT_TYPE;
    int duration = 1000;
    int delay = 0;
    List *l;
    YearSubItem *subit;
    LIST_FOR_EACH(it->items, l, subit) {
        year_sub_item_show(subit, easetype, duration, delay);
        delay += 250;
    };
}

static void year_main_item_unclick(YearMainItem *it)
{
    _nemoshow_item_motion(it->btn, NEMOEASE_CUBIC_IN_TYPE, 500, 0,
            "alpha", 1.0,
            NULL);
    _nemoshow_item_motion(it->btn_sel, NEMOEASE_CUBIC_IN_TYPE, 500, 0,
            "alpha", 0.0,
            NULL);
    _nemoshow_item_motion(it->arch, NEMOEASE_CUBIC_INOUT_TYPE, 500, 300,
            "sx", 0.0, "sy", 0.0,
            NULL);

    List *l;
    YearSubItem *subit;
    LIST_FOR_EACH(it->items, l, subit) {
        year_sub_item_hide(subit, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    };
}


typedef struct _YearBG YearBG;
struct _YearBG {
    YearView *view;
    NemoWidget *widget;
    struct showone *group;
    List *imgs;
};

YearBG *year_bg_create(YearView *view, NemoWidget *parent, const char *uri, int w, int h, int cnt_y)
{
    YearBG *bg = calloc(sizeof(YearBG), 1);
    bg->view = view;

    NemoWidget *widget;
    struct showone *group;

    bg->widget = widget = nemowidget_create_vector(parent, w, h * cnt_y);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    bg->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    int j;
    for (j = 0 ; j < cnt_y ; j++) {
        Image *img = image_create(group);
        image_translate(img, 0, 0, 0, 0, h * j);
        image_load_full(img, view->tool, uri, w, h, NULL, NULL);
        bg->imgs = list_append(bg->imgs, img);
    }
    return bg;
}

void year_bg_translate(YearBG *bg, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    nemowidget_translate(bg->widget, easetype, duration, delay, tx, ty);
}

static void _year_view_btn_timeout(struct nemotimer *timer, void *userdata)
{
    int duration = 2000;
    int delay = 0;
    YearView *view = userdata;

    List *l;
    YearMainItem *it;
    LIST_FOR_EACH(view->mains, l, it) {
        NemoMotion *m;
        m = nemomotion_create(view->show, NEMOEASE_LINEAR_TYPE, duration, delay);
        nemomotion_attach(m, 0.5,
                it->bg, "sx", 1.25,
                it->bg, "sy", 1.25,
                NULL);
        nemomotion_attach(m, 1.0,
                it->bg, "sx", 1.0,
                it->bg, "sy", 1.0,
                NULL);
        nemomotion_run(m);
        delay += 500;
    }

    nemotimer_set_timeout(timer, duration + delay);
}

static void _year_view_bg_timeout(struct nemotimer *timer, void *userdata)
{
    int duration = 15000;
    YearView *view = userdata;
    int i = 0;
    List *l;
    l = list_get_nth(view->bgs, view->bg_idx);
    while (l) {
        YearBG *bg = LIST_DATA(l);
        year_bg_translate(bg, 0, 0, 0, i * view->bg_w, 0);
        year_bg_translate(bg, NEMOEASE_LINEAR_TYPE, duration, 0, (i - 1) * view->bg_w, 0);

        l = l->next;
        if (!l) l = LIST_FIRST(view->bgs);
        i++;
        if (i >= view->bg_cnt_x) break;
    }

    nemotimer_set_timeout(timer, duration);
}

static void _year_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    const char *id = nemoshow_one_get_id(one);
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        if (!strcmp(id, "year_main")) {
            YearMainItem *it = nemoshow_one_get_userdata(one);
            YearView *view = it->view;
            year_main_item_down(it);
            nemoshow_dispatch_frame(view->show);
        } else if (!strcmp(id, "year_sub")) {
            YearSubItem *it = nemoshow_one_get_userdata(one);
            YearView *view = it->view;
            year_sub_item_down(it);
            nemoshow_dispatch_frame(view->show);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        if (!strcmp(id, "year_main")) {
            YearMainItem *it = nemoshow_one_get_userdata(one);
            YearView *view = it->view;
            view->grab = NULL;
            if (nemoshow_event_is_single_click(show, event)) {
                year_main_item_click(it);

                List *l;
                YearMainItem *_it;
                LIST_FOR_EACH(view->mains, l, _it) {
                    if (it != _it)
                        year_main_item_unclick(_it);
                }
            } else {
                year_main_item_up(it);
            }
            nemoshow_dispatch_frame(view->show);
        } else if (!strcmp(id, "year_sub")) {
            YearSubItem *it = nemoshow_one_get_userdata(one);
            YearView *view = it->view;
            view->grab = NULL;
            year_sub_item_up(it);

            if (nemoshow_event_is_single_click(show, event)) {
            }
        }
    }
}

static void _year_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    YearView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->grab) {
            view->grab = nemowidget_create_grab(widget, event,
                    _year_view_grab_event, one);
        }
    }
}

static YearView *year_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    YearView *view = calloc(sizeof(YearView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    struct showone *group;
    struct showone *one;

    // Background
    NemoWidget *widget;
    view->bg_widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    view->bg_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    int w, h;
    const char *uri = APP_IMG_DIR"/year/year-background.png";
    file_get_image_wh(uri, &w, &h);
    int cnt_x, cnt_y;
    view->bg_w = w;
    view->bg_cnt_x = cnt_x = width/w + 2;
    cnt_y = height/h + 1;

    int i;
    for (i = 0 ; i < cnt_x ; i++) {
        YearBG *bg = calloc(sizeof(YearBG), 1);
        bg = year_bg_create(view, parent, uri, w, h, cnt_y);
        year_bg_translate(bg, 0, 0, 0, i * w, 0);
        view->bgs = list_append(view->bgs, bg);
    }
    view->bg_timer = TOOL_ADD_TIMER(view->tool, 0, _year_view_bg_timeout, view);

    view->btn_timer = TOOL_ADD_TIMER(view->tool, 0, _year_view_btn_timeout, view);

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_append_callback(widget, "event", _year_view_event, view);

    view->btn_group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    nemoshow_item_translate(group, 0, -view->h);

    double sx ,sy;
    double x, y;
    double ww, hh;
    sx = view->w/1920.0;
    sy = view->h/1080.0;

    uri = APP_ICON_DIR"/year/year-pink.svg";
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sy;
    x = 467.49 * sx;
    y = 259.98 * sy;
    view->btn_bg = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_item_translate(one, x, y);

    for (i = 0 ; i < sizeof(YEAR_MAIN)/sizeof(YEAR_MAIN[0]) ; i++) {
        YearMainItem *it;
        it = year_view_create_main_item(view, YEAR_MAIN[i].btn, YEAR_MAIN[i].btn_sel,
                YEAR_MAIN[i].x, YEAR_MAIN[i].y,
                YEAR_MAIN[i].arch, YEAR_MAIN[i].arch_x, YEAR_MAIN[i].arch_y,
                YEAR_MAIN[i].sub, YEAR_MAIN[i].sub_cnt,
                sx, sy);
        view->mains = list_append(view->mains, it);
    }

    return view;
}

static void year_view_show(YearView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_show(view->bg_widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration, delay, 1.0);
    List *l;
    YearBG *bg;
    LIST_FOR_EACH(view->bgs, l, bg) {
        nemowidget_set_alpha(bg->widget, easetype, duration, delay, 1.0);
    }

    _nemoshow_item_motion(view->btn_group, easetype, duration, delay,
            "ty", 0.0, NULL);

    YearMainItem *it;
    LIST_FOR_EACH(view->mains, l, it) {
        year_main_item_show(it, easetype, duration, delay);
        delay += 250;
    }

    view->bg_idx = 0;
    nemotimer_set_timeout(view->bg_timer, duration + delay);
    nemotimer_set_timeout(view->btn_timer, duration + delay);
    nemoshow_dispatch_frame(view->show);
}

static void year_view_hide(YearView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_hide(view->bg_widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration, delay, 0.0);
    int i = 0;
    List *l;
    YearBG *bg;
    LIST_FOR_EACH(view->bgs, l, bg) {
        year_bg_translate(bg, easetype, duration, delay, i * view->bg_w, 0);
        nemowidget_set_alpha(bg->widget, easetype, duration, delay, 0.0);
        i++;
    }

    _nemoshow_item_motion(view->btn_group, easetype, duration, delay + 500,
            "ty", (double)-view->h, NULL);

    YearMainItem *it;
    LIST_FOR_EACH(view->mains, l, it) {
        year_main_item_hide(it, easetype, duration, delay);
    }

    nemotimer_set_timeout(view->bg_timer, 0);
    nemotimer_set_timeout(view->btn_timer, 0);

    nemoshow_dispatch_frame(view->show);
}

typedef struct _RegionData RegionData;
struct _RegionData {
    char *country;
    double x, y, txt_x, txt_y;
};

RegionData REGION_DATAS[] = {
    {"canada", 324, 207, 301, 248},
    {"usa", 334, 323, 330, 364},
    {"mexico", 285, 423, 264, 464},
    {"colombia", 431, 561, 398, 602},
    {"peru", 425, 626, 416, 667},
    {"brazil", 611, 625, 590, 666},
    {"sweden", 932, 181, 830, 146},
    {"norway", 916, 200, 817, 165},
    {"denmark", 925, 231, 797, 185},
    {"uk", 861, 234, 767, 216},
    {"netherlands", 902, 250, 657, 245},
    {"belgium", 890, 269, 707, 265},
    {"switzerland", 918, 284, 657, 285},
    {"france", 879, 300, 719, 305},
    {"portugal", 829, 324, 692, 325},
    {"spain", 848, 333, 733, 345},
    {"finland", 966, 160, 1006, 147},
    {"latvia", 966, 179, 1006, 166},
    {"germany", 922, 254, 1006, 185},
    {"poland", 956, 243, 1006, 204},
    {"czech republic", 943, 261, 1006, 223},
    {"slovenia", 950, 279, 1030, 256},
    {"croatia", 968, 286, 1030, 275},
    {"romania", 968, 286, 1030, 275}, // removed !!!
    {"serbia", 993, 307, 1030, 294},
    {"bosnia", 974, 307, 1030, 313},
    {"turkey", 1040, 343, 1023, 363},
    {"greece", 986, 332, 944, 352},
    {"italy", 945, 309, 877, 333},
    {"egypt", 989, 416, 973, 457},
    {"israel", 1055, 388, 1036, 429},
    {"qatar", 1132, 408, 1169, 417},
    {"saudi arabia", 1111, 449, 1060, 489},
    {"uae", 1161, 445, 1198, 454},
    {"russia", 1303, 180, 1284, 221},
    {"china", 1365, 306, 1353, 347},
    {"india", 1296, 423, 1282, 464},
    {"korea", 1546, 351, 1531, 392},
    {"japan", 1610, 371, 1648, 381},
    {"taiwan", 1544, 428, 1582, 438},
    {"phillipines", 1558, 495, 1596, 504},
    {"malaysia", 1424, 518, 1390, 559},
    {"singapore", 1429, 590, 1392, 631},
    {"australia", 1603, 734, 1565, 775}
};

RegionData REGION_TXTS[] = {
    {"canada", 301, 248},
    {"usa", 330, 364},
    {"mexico", 264, 464},
    {"colombia", 398, 602},
    {"peru", 416, 667},
    {"brazil", 590, 666},
    {"sweden", 830, 146},
    {"norway", 817, 165},
    {"denmark", 797, 185},
    {"uk", 767, 216},
    {"netherlands", 657, 245},
    {"belgium", 707, 265},
    {"switzerland", 657, 285},
    {"france", 719, 305},
    {"portugal", 692, 325},
    {"spain", 733, 345},
    {"finland", 1006, 147},
    {"latvia", 1006, 166},
    {"germany", 1006, 185},
    {"poland", 1006, 204},
    {"czech republic", 1006, 223},
    {"slovenia", 1030, 247},
    {"croatia", 1030, 266},
    {"romania", 1030, 285},
    {"serbia", 1030, 304},
    {"bosnia", 1030, 323},
    {"turkey", 1023, 363},
    {"greece", 944, 352},
    {"italy", 877, 333},
    {"egypt", 973, 457},
    {"israel", 1036, 429},
    {"qatar", 1169, 417},
    {"saudi arabia", 1060, 489},
    {"uae", 1198, 454},
    {"russia", 1284, 221},
    {"china", 1353, 347},
    {"india", 1282, 464},
    {"korea", 1531, 392},
    {"japan", 1648, 381},
    {"taiwan", 1582, 438},
    {"phillipines", 1596, 504},
    {"malaysia", 1390, 559},
    {"singapore", 1392, 631},
    {"australia", 1565, 775}
};

struct _RegionView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;

    NemoWidgetGrab *grab;
    struct showone *group;
    List *maps;
    List *items;
};

typedef struct _RegionMap RegionMap;
struct _RegionMap {
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *group;

    struct showone *one;
    List *ones;

    struct nemotimer *timer;
};

static void _region_map_timeout(struct nemotimer *timer, void *userdata)
{
    RegionMap *map = userdata;
    int delay = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(map->ones, l, one) {
        _nemoshow_item_motion_bounce(one, NEMOEASE_LINEAR_TYPE, 2000, delay,
                "alpha", 1.0, 0.0,
                NULL);
        delay += 250;
    }
    nemotimer_set_timeout(timer, 2000 + delay + 250);
    nemoshow_dispatch_frame(map->show);
}

static RegionMap *region_map_create(RegionView *view, struct showone *parent, const char *uri, int width, int height)
{
    RegionMap *map = calloc(sizeof(RegionMap), 1);
    map->tool = view->tool;
    map->show = view->show;
    map->w = width;
    map->h = height;

    struct showone *one;
    struct showone *group;
    map->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    char buf[PATH_MAX];

    int i;
    for (i = 2 ; i <= 5 ; i++) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%s%d.svg", uri, i);
        one = SVG_PATH_GROUP_CREATE(group, width, height, buf);
        nemoshow_item_set_alpha(one, 0.0);
        map->ones = list_append(map->ones, one);
    }

    snprintf(buf, PATH_MAX, "%s%d.svg", uri, 1);
    map->one = one = SVG_PATH_GROUP_CREATE(group, width, height, buf);

    map->timer = TOOL_ADD_TIMER(map->tool, 0, _region_map_timeout, map);

    return map;
}

static void region_map_translate(RegionMap *map, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(map->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(map->group, tx, ty);
    }
}

static void region_map_show(RegionMap *map, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(map->group, easetype, duration, delay,
            "alpha", 1.0,
            "tx", 0.0, "ty", 0.0,
            NULL);
    nemotimer_set_timeout(map->timer, duration + delay);
}

static void region_map_hide(RegionMap *map, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(map->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    nemotimer_set_timeout(map->timer, 0);
}

typedef struct _RegionItem RegionItem;
struct _RegionItem {
    RegionView *view;
    char *country;
    struct showone *group;
    struct showone *icon;
    struct showone *txt;
};

static RegionItem *region_view_create_item(RegionView *view, int idx, double sx, double sy)
{
    struct showone *one;
    struct showone *group;

    RegionItem *it = calloc(sizeof(RegionItem), 1);
    it->view = view;
    it->group = group = GROUP_CREATE(view->group);
    it->country = strdup(REGION_DATAS[idx-1].country);

    double w, h;
    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, APP_ICON_DIR"/region/icon/region-icon%02d.svg", idx);
    if (!file_is_exist(buf)) {
        ERR("%s does not exist!", buf);
        nemoshow_one_destroy(group);
        free(it);
        return NULL;
    }
    svg_get_wh(buf, &w, &h);
    w *= sx;
    h *= sy;
    it->icon = one = SVG_PATH_GROUP_CREATE(group, w, h, buf);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one,
            REGION_DATAS[idx-1].x * sx + w/2,
            REGION_DATAS[idx-1].y * sy + h/2);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_scale(one, 0.0, 0.0);

    snprintf(buf, PATH_MAX, APP_ICON_DIR"/region/text/region-text%02d.svg", idx);
    svg_get_wh(buf, &w, &h);
    w *= sx;
    h *= sy;
    it->txt = one = SVG_PATH_GROUP_CREATE(group, w, h, buf);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one,
            REGION_DATAS[idx-1].txt_x * sx + w/2,
            REGION_DATAS[idx-1].txt_y * sy + h/2);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_scale(one, 0.0, 0.0);

    return it;
}

static void region_item_down(RegionItem *it)
{
    _nemoshow_item_motion_bounce(it->icon, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.5, 1.25, "sy", 1.5, 1.25,
            NULL);
    _nemoshow_item_motion_bounce(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 1.5, 1.25, "sy", 1.5, 1.25,
            NULL);
}

static void region_item_up(RegionItem *it)
{
    _nemoshow_item_motion_bounce(it->icon, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.5, 1.0, "sy", 0.5, 1.0,
            NULL);
    _nemoshow_item_motion_bounce(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "sx", 0.5, 1.0, "sy", 0.5, 1.0,
            NULL);
}

static void _region_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    RegionItem *it = nemoshow_one_get_userdata(one);
    RegionView *view = it->view;

    if (nemoshow_event_is_down(show, event)) {
        region_item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        region_item_up(it);
        view->grab = NULL;
    }
    if (nemoshow_event_is_single_click(show, event)) {
        Karim *karim = view->karim;

        KarimGroup *grp = karim_data_search_group(karim->countries, it->country);
        karim->honey = honey_view_create(karim, karim->parent, karim->w, karim->h, grp);
        karim_change_view(karim, KARIM_TYPE_HONEY);
    }
}

static void _region_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    RegionView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->grab) {
            view->grab = nemowidget_create_grab(widget, event,
                    _region_view_grab_event, one);
        }
    }
}

static RegionView *region_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    RegionView *view = calloc(sizeof(RegionView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _region_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *group;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

#if 0
    // Designed for 1920x1080
    sx = view->w/1920.0;
    sy = view->h/1908.0;
    int w, h;
    const char *uri;
    Image *img;
    uri = APP_IMG_DIR"/region/BG.png";
    file_get_image_wh(uri, &w, &h);
    w = w * sx;
    h = h * sy;
    view->bg = img = image_create(group);
    image_load_full(img, view->tool, uri, w, h, NULL, NULL);
#endif

    int i;
    for (i = 1 ; i <= 25 ; i++) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, APP_ICON_DIR"/region/map/karim-map%02d-", i);
        RegionMap *map;
        map = region_map_create(view, group, buf, width, height);

        if (1 == i) {
            region_map_translate(map, 0, 0, 0, -width/4, 0);
        } else if (2 <= i && i <= 7) {
            region_map_translate(map, 0, 0, 0, 0, -height/4);
        } else if (6 <= i && i <= 7) {
            region_map_translate(map, 0, 0, 0, width/4, 0);
        } else if (8 <= i && i <= 9) {
            region_map_translate(map, 0, 0, 0, width/2, 0);
        } else if (10 <= i && i <= 12) {
            region_map_translate(map, 0, 0, 0, 0, -height/4);
        } else if (13 == i) {
            region_map_translate(map, 0, 0, 0, 0, height);
        } else if (14 <= i && i <= 20) {
            region_map_translate(map, 0, 0, 0, width, 0);
        } else  {
            region_map_translate(map, 0, 0, 0, 0, height);
        }

        view->maps = list_append(view->maps, map);
    }

    double sx, sy;
    // Designed for 1920x1080
    sx = view->w/1920.0;
    sy = view->h/1080.0;
    for (i = 1 ; i <= sizeof(REGION_DATAS)/sizeof(REGION_DATAS[0]) ; i++) {
        // XXX: romania is removed from map
        if (i == 24) continue;
        RegionItem *it = region_view_create_item(view, i, sx, sy);
        view->items = list_append(view->items, it);
    }

    return view;
}

static void region_view_show(RegionView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    int i = 1;
    int _delay = 0;
    List *l;
    RegionMap *map;
    LIST_FOR_EACH(view->maps, l, map) {
        region_map_show(map, easetype, duration, delay + _delay);
        _delay += 150;
        i++;
    }

    i = 1;
    RegionItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        int ddelay;
        if (1 <= i && i <= 6) {
            ddelay = delay + 150 + 50 * i;
        } else if (i == 9) {
            ddelay = delay + 150 * 11;
        } else if (i == 40) {
            ddelay = delay + 150 * 14;
        } else if (i == 41) {
            ddelay = delay + 150 * 15;
        } else if (i == 43) {
            ddelay = delay + 150 * 21;
        } else if (i == 44) {
            ddelay = delay + 150 * 23;
        } else {
            ddelay = delay + 150 * 10 + 50 * (i - 10);
        }
        _nemoshow_item_motion_bounce(it->icon, easetype, 1000, ddelay + 500,
                "alpha", 1.0, 1.0,
                "sx", 1.5, 1.0, "sy", 1.5, 1.0,
                NULL);
        _nemoshow_item_motion_bounce(it->txt, easetype, 1000, ddelay + 600,
                "alpha", 1.0, 1.0,
                "sx", 1.5, 1.0, "sy", 1.5, 1.0,
                NULL);
        i++;
    }

    nemoshow_dispatch_frame(view->show);
}

static void region_view_hide(RegionView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    int width = view->h;
    int height = view->w;
    int i = 1;
    List *l;
    RegionMap *map;
    LIST_FOR_EACH(view->maps, l, map) {
        if (1 == i) {
            region_map_translate(map, easetype, duration, delay, -width/4, 0);
        } else if (2 <= i && i <= 7) {
            region_map_translate(map, easetype, duration, delay, 0, -height/4);
        } else if (6 <= i && i <= 7) {
            region_map_translate(map, easetype, duration, delay, width/4, 0);
        } else if (8 <= i && i <= 9) {
            region_map_translate(map, easetype, duration, delay, width/2, 0);
        } else if (10 <= i && i <= 12) {
            region_map_translate(map, easetype, duration, delay, 0, -height/4);
        } else if (13 == i) {
            region_map_translate(map, easetype, duration, delay, 0, height);
        } else if (14 <= i && i <= 20) {
            region_map_translate(map, easetype, duration, delay, width, 0);
        } else  {
            region_map_translate(map, easetype, duration, delay, 0, height);
        }
        region_map_hide(map, easetype, duration, delay);
        i++;
    }

    RegionItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        _nemoshow_item_motion(it->icon, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
        _nemoshow_item_motion(it->txt, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
    }
    nemoshow_dispatch_frame(view->show);
}

typedef struct _Coord Coord;
struct _Coord {
    double x, y;
};

Coord WORK_WAVE_COORDS[5] = {
    {-50, -86},
    {-50, 108},
    {-50, 214},
    {-50, 508},
    {-50, 651}
};

typedef struct _WorkData WorkData;
struct _WorkData {
    char *category;
    double x, y;
    double txt_x, txt_y;
};

WorkData WORK_DATAS[] = {
	{"concept+exhibition", 814, 384, 727, 434},
	{"furniture", 798, 68, 731, 112},
	{"product", 1635, 85, 1565, 135},
	{"residential", 360, 222, 276, 273},
	{"media", 1670, 276, 1623, 323},
	{"publicspaces", 1298, 354, 1244, 400},
	{"art", 1801, 443, 1765, 496},
	{"packaging", 334, 643, 263, 690},
	{"surfaces", 1034, 514, 970, 565},
	{"tabletop", 1631, 626, 1560, 678},
	{"fashion", 438, 853, 364, 896},
	{"lighting", 1136, 769, 1065, 815},
	{"graphic", 1730, 826, 1659, 874},
};

typedef struct _WorkWave WorkWave;
typedef struct _WorkItem WorkItem;

struct _WorkWave {
    WorkView *view;
    Path *path0, *path1;

    struct showone *group;
    List *ones;
    struct nemotimer *timer;
    int idx;
};

struct _WorkItem {
    char *category;
    WorkView *view;
    struct showone *group;
    Image *img, *img1;
    struct showone *txt;
    struct nemotimer *timer;
    int idx;
};

struct _WorkView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;

    NemoWidgetGrab *grab;
    struct showone *group;
    List *waves;
    List *icons;
};

static void _work_icon_timeout(struct nemotimer *timer, void *userdata)
{
    WorkItem *it = userdata;
    int duration = 5000;
    image_rotate(it->img, 0, 0, 0, 0.0);
    image_rotate(it->img, NEMOEASE_LINEAR_TYPE, duration, 0, 360.0);
    if (it->img1) {
        image_rotate(it->img1, 0, 0, 0, 0.0);
        image_rotate(it->img1, NEMOEASE_LINEAR_TYPE, duration, 0, 360.0);
    }
    nemotimer_set_timeout(timer, duration);
}

static WorkItem *work_view_create_item(WorkView *view, int idx, double sx, double sy)
{
    WorkItem *it = calloc(sizeof(WorkItem), 1);
    it->view = view;
    it->category = strdup(WORK_DATAS[idx-1].category);

    struct showone *group;
    struct showone *one;
    it->group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);

    int w, h;
    Image *img;

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, APP_IMG_DIR"/work/icon-%02d.png", idx);
    file_get_image_wh(buf, &w, &h);
    w = w * (view->w/3840.0);
    h = h * (view->h/2160.0);
    it->img = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    nemoshow_one_set_state(image_get_one(img), NEMOSHOW_PICK_STATE);

    double img_tx, img_ty;
    img_tx = WORK_DATAS[idx-1].x * sx;
    img_ty = WORK_DATAS[idx-1].y * sy;
    //if (id) nemoshow_one_set_id(image_get_one(img), it);
    nemoshow_one_set_userdata(image_get_one(img), it);
    image_translate(img, 0, 0, 0, img_tx, img_ty);
    image_scale(img, 0, 0, 0, 0, 0);
    image_set_alpha(img, 0, 0, 0, 0.0);
    image_load_full(img, view->tool, buf, w, h, NULL, NULL);

    snprintf(buf, PATH_MAX, APP_IMG_DIR"/work/icon-%02d-1.png", idx);
    file_get_image_wh(buf, &w, &h);
    w = w * (view->w/3840.0);
    h = h * (view->h/2160.0);
    it->img1 = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_translate(img, 0, 0, 0, img_tx, img_ty);
    image_scale(img, 0, 0, 0, 0, 0);
    image_set_alpha(img, 0, 0, 0, 0.0);
    image_load_full(img, view->tool, buf, w, h, NULL, NULL);

    snprintf(buf, PATH_MAX, APP_ICON_DIR"/work/text/work-text%02d.svg", idx);
    double txt_tx, txt_ty;
    txt_tx = WORK_DATAS[idx-1].txt_x * sx;
    txt_ty = WORK_DATAS[idx-1].txt_y * sy;
    double ww, hh;
    svg_get_wh(buf, &ww, &hh);
    ww = ww * (view->w/1920.0);
    hh = hh * (view->h/1080.0);
    it->txt = one = SVG_PATH_GROUP_CREATE(group, ww, hh, buf);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, txt_tx + ww/2, txt_ty + hh/2);
    nemoshow_item_scale(one, 0.0, 1.0);

    it->timer = TOOL_ADD_TIMER(view->tool, 0, _work_icon_timeout, it);

    return it;
}

static void work_item_show(WorkItem *it, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(it->timer, 10 + delay);
    _nemoshow_item_motion(it->group, easetype, duration, delay,
            "alpha", 1.0, NULL);

    image_set_alpha(it->img, easetype, duration, delay, 1.0);
    _nemoshow_item_motion_bounce(image_get_group(it->img), easetype, duration, delay,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0, NULL);
    _nemoshow_item_motion_bounce(it->txt, easetype, duration, delay,
            "sx", 1.25, 1.0, NULL);
}

static void work_item_hide(WorkItem *it, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(it->timer, 0);
    _nemoshow_item_motion(it->group, easetype, duration, delay,
            "alpha", 0.0, NULL);

    image_set_alpha(it->img, easetype, duration, delay, 0.0);
    if (it->img1) image_set_alpha(it->img1, easetype, duration, delay, 0.0);
    image_scale(it->img, easetype, duration, delay, 0.0, 0.0);
    _nemoshow_item_motion_bounce(it->txt, easetype, duration, delay,
            "sx", 1.5, 1.0, NULL);
}

static void _work_wave_timeout(struct nemotimer *timer, void *userdata)
{
    WorkWave *wave = userdata;
    int cnt = list_count(wave->ones);

    int duration = 5000;
    uint32_t easetype = NEMOEASE_SINUSOIDAL_INOUT_TYPE;

#if 0
    struct showone *one;
    one = LIST_DATA(list_get_nth(wave->ones, wave->idx%cnt));
    //nemoshow_item_set_alpha(one, 0.0);
    _nemoshow_item_motion(one, easetype, 1000, 0,
            "alpha", 1.0, NULL);

    if (wave->idx < cnt) {
        path_array_morph(one, 0, 0, 0, wave->path0);
        path_array_morph(one, easetype, duration, 0, wave->path1);
        _nemoshow_item_motion(one, easetype, duration, 0,
                "tx", 50.0, NULL);
    } else {
        path_array_morph(one, 0, 0, 0, wave->path1);
        path_array_morph(one, easetype, duration, 0, wave->path0);
        _nemoshow_item_motion(one, easetype, duration, 0,
                "tx", 0.0, NULL);
    }

    wave->idx++;
    if (wave->idx >= cnt * 2) wave->idx = 0;

    nemotimer_set_timeout(wave->timer, duration/cnt);
    nemoshow_dispatch_frame(wave->view->show);
#else
    struct showone *one;
    one = LIST_DATA(list_get_nth(wave->ones, wave->idx));
    nemoshow_item_set_alpha(one, 0.0);
    _nemoshow_item_motion(one, easetype, 1000, 0,
            "alpha", 1.0, NULL);

    path_array_morph(one, 0, 0, 0, wave->path0);
    path_array_morph(one, easetype, duration, 0, wave->path1);

    wave->idx++;
    if (wave->idx >= cnt) wave->idx = 0;

    nemotimer_set_timeout(wave->timer, duration/cnt);
    nemoshow_dispatch_frame(wave->view->show);
#endif
}

WorkWave *work_view_create_wave(WorkView *view, const char *uri0, const char *uri1)
{
    WorkWave *wave = calloc(sizeof(WorkWave), 1);
    wave->view = view;

    struct showone *one;
    struct showone *group;
    wave->group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);

    wave->path0 = svg_get_path(uri0);
    wave->path1 = svg_get_path(uri1);

    wave->path1->fill &= 0xFFFFFF00;
    wave->path1->stroke &= 0xFFFFFF00;

    double sx, sy;
    sx = view->w/1920.0;
    sy = view->h/1080.0;

    int cnt = 10;
    int i;
    for (i = 0 ; i < cnt ; i++) {
        one = path_draw_array(wave->path0, group);
        nemoshow_item_scale(one, sx, sy);
        nemoshow_item_set_alpha(one, 0.0);
        wave->ones = list_append(wave->ones, one);
    }

    wave->timer = TOOL_ADD_TIMER(view->tool, 0, _work_wave_timeout, wave);

    return wave;
}

static void work_wave_translate(WorkWave *wave, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(wave->group, tx, ty);
    }
}

static void work_wave_scale(WorkWave *wave, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "sx", sx, "sy", sy,
                NULL);
    } else {
        nemoshow_item_scale(wave->group, sx, sy);
    }
}

static void work_wave_show(WorkWave *wave, uint32_t easetype, int duration, int delay)
{
    wave->idx = 0;
    nemotimer_set_timeout(wave->timer, 10 + delay);
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(wave->group, 1.0);
    }
}

static void work_wave_hide(WorkWave *wave, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(wave->timer, 0);
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(wave->group, 0.0);
    }
    List *l;
    struct showone *one;
    LIST_FOR_EACH(wave->ones, l, one) {
         path_array_morph(one, easetype, duration, delay, wave->path0);
    }
}

void work_view_show(WorkView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);

    // Designed for 1920x1080
    double sx= view->w/1920.0;
    double sy = view->h/1080.0;
    List *l;
    WorkWave *wave;
    int _delay = 0;
    int i = 1;
    LIST_FOR_EACH(view->waves, l, wave) {
        if (i%2 == 0) {
            work_wave_translate(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay + _delay,
                    WORK_WAVE_COORDS[i-1].x * sx,
                    WORK_WAVE_COORDS[i-1].y * sy);
        }
        work_wave_scale(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay + _delay, 1.0, 1.0);
        work_wave_show(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay + _delay);
        _delay += 250;
        i++;
    }
    _delay = 0;
    WorkItem *it;
    LIST_FOR_EACH(view->icons, l, it) {
        work_item_show(it, NEMOEASE_CUBIC_OUT_TYPE, 1000, delay + _delay);
        _delay += 200;
    }
}

void work_view_hide(WorkView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    // Designed for 1920x1080
    double sx= view->w/1920.0;
    double sy = view->h/1080.0;
    int i = 1;
    List *l;
    WorkWave *wave;
    LIST_FOR_EACH(view->waves, l, wave) {
        if (i%2 == 0) {
            work_wave_translate(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay,
                    WORK_WAVE_COORDS[i-1].x * sx + view->w,
                    WORK_WAVE_COORDS[i-1].y * sy);
        } else {
            work_wave_translate(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay,
                    WORK_WAVE_COORDS[i-1].x * sx,
                    WORK_WAVE_COORDS[i-1].y * sy);
        }
        work_wave_scale(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay, 0.0, 1.0);
        work_wave_hide(wave, NEMOEASE_CUBIC_INOUT_TYPE, 1500, delay);
        i++;
    }
    WorkItem *it;
    LIST_FOR_EACH(view->icons, l, it) {
        work_item_hide(it, NEMOEASE_CUBIC_IN_TYPE, 500, 0);
    }
}

static void work_icon_down(WorkItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion_bounce(image_get_group(it->img), easetype, duration, delay,
            "sx", 1.5, 1.4, "sy", 1.5, 1.4,
            NULL);
    if (it->img1) {
        _nemoshow_item_motion_bounce(image_get_group(it->img1), easetype, duration, delay,
                "sx", 1.5, 1.4, "sy", 1.5, 1.4,
                NULL);
        image_set_alpha(it->img, easetype, duration, delay, 0.0);
        image_set_alpha(it->img1, easetype, duration, delay, 1.0);
    }
}

static void work_icon_up(WorkItem *it, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion_bounce(image_get_group(it->img), easetype, duration, delay,
            "sx", 0.8, 1.0, "sy", 0.8, 1.0,
            NULL);
    if (it->img1) {
        _nemoshow_item_motion_bounce(image_get_group(it->img1), easetype, duration, delay,
                "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                NULL);
        image_set_alpha(it->img, easetype, duration, delay, 1.0);
        image_set_alpha(it->img1, easetype, duration, delay, 0.0);
    }
}

static void _work_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    const char *id = nemoshow_one_get_id(one);
    WorkItem *icon = nemoshow_one_get_userdata(one);
    WorkView *view = icon->view;
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        work_icon_down(icon, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0);
        nemoshow_dispatch_frame(view->show);
    } else if (nemoshow_event_is_up(show, event)) {
        work_icon_up(icon, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0);
        view->grab = NULL;
        if (nemoshow_event_is_single_click(show, event)) {
            if (!strcmp(id, "tabletop")) {
                karim_change_view(view->karim, KARIM_TYPE_HONEY);
            }
        }
        nemoshow_dispatch_frame(view->show);
    }
}

static void _work_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    WorkView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->grab) {
            view->grab = nemowidget_create_grab(widget, event,
                    _work_view_grab_event, one);
        }
    }
}

WorkView *work_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    WorkView *view = calloc(sizeof(WorkView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _work_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *group;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    // Designed for 1920x1080
    double sx= view->w/1920.0;
    double sy = view->h/1080.0;
    int i;
    for (i = 1 ; i <= 5; i++) {
        char buf0[PATH_MAX], buf1[PATH_MAX];
        snprintf(buf0, PATH_MAX, APP_ICON_DIR"/work/karim-workwave-%d-1.svg", i);
        snprintf(buf1, PATH_MAX, APP_ICON_DIR"/work/karim-workwave-%d-2.svg", i);
        WorkWave *wave;
        if (i%2 == 0) {
            wave = work_view_create_wave(view, buf1, buf0);
        } else {
            wave = work_view_create_wave(view, buf0, buf1);
        }
        if (i%2 == 0) {
            work_wave_translate(wave, 0, 0, 0,
                    WORK_WAVE_COORDS[i-1].x * sx + view->w,
                    WORK_WAVE_COORDS[i-1].y * sy);
        } else {
            work_wave_translate(wave, 0, 0, 0,
                    WORK_WAVE_COORDS[i-1].x * sx,
                    WORK_WAVE_COORDS[i-1].y * sy);
        }
        work_wave_scale(wave, 0, 0, 0, 0.0, 1.0);
        view->waves = list_append(view->waves, wave);
    }

    for (i = 1 ; i <= 13 ; i++) {
        WorkItem *icon;
        icon = work_view_create_item(view, i, sx, sy);
        view->icons = list_append(view->icons, icon);
    }

    return view;
}

struct _MenuView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;

    NemoWidgetGrab *grab;
    struct showone *group;

    struct nemotimer *timer;
    struct showone *out;
    List *outs;
    struct showone *btn_region;
    struct showone *btn_wave;
    struct showone *btn_year;
};

static void _menu_view_timeout(struct nemotimer *timer, void *userdata)
{
    MenuView *view = userdata;
    int duration = 1500;
    int delay = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(view->outs, l, one) {
        _nemoshow_item_motion_bounce(one, NEMOEASE_LINEAR_TYPE, duration, delay,
                "alpha", 1.0, 0.0,
                NULL);
        nemoshow_item_scale(one, 0.8, 0.8);
        _nemoshow_item_motion(one, NEMOEASE_LINEAR_TYPE, duration, delay,
                "sx", 1.25,
                "sy", 1.25,
                NULL);
        delay += 250;
    }
    nemotimer_set_timeout(timer, duration + delay + 250);
    nemoshow_dispatch_frame(view->show);
}

static void _menu_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    const char *id = nemoshow_one_get_id(one);
    MenuView *view = nemoshow_one_get_userdata(one);
    RET_IF(!id);

    if (nemoshow_event_is_down(show, event)) {
        if (!strcmp(id, "region")) {
            _nemoshow_item_motion_bounce(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
                    "sx", 1.5, 1.4, "sy", 1.5, 1.4,
                    NULL);
        } else if (!strcmp(id, "work")) {
            _nemoshow_item_motion_bounce(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
                    "sx", 1.5, 1.4, "sy", 1.5, 1.4,
                    NULL);
        } else if (!strcmp(id, "year")) {
            _nemoshow_item_motion_bounce(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
                    "sx", 1.5, 1.4, "sy", 1.5, 1.4,
                    NULL);
        }
        nemoshow_dispatch_frame(view->show);
    } else if (nemoshow_event_is_up(show, event)) {
        view->grab = NULL;
        if (nemoshow_event_is_single_click(show, event)) {
            double sx, sy;
            sx = view->w/1920.0;
            sy = view->h/1080.0;
            double gw = 440 * sx;
            double gh = 140 * sy;
            _nemoshow_item_motion(view->group, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    "alpha", 1.0,
                    "sx", 1.0, "sy", 1.0,
                    "tx", view->w/2.0 + gw/2, "ty", view->h * 0.9 + gh/2,
                    NULL);

            uint32_t color0 = 0xF04E98FF;
            uint32_t color1 = 0xF7ACB87F;
            if (!strcmp(id, "region")) {
                _nemoshow_item_motion_bounce(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
                _nemoshow_item_motion(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color0,
                        NULL);
                _nemoshow_item_motion(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                _nemoshow_item_motion(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                karim_change_view(view->karim, KARIM_TYPE_REGION);
            } else if (!strcmp(id, "work")) {
                _nemoshow_item_motion_bounce(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
                _nemoshow_item_motion(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                _nemoshow_item_motion(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color0,
                        NULL);
                _nemoshow_item_motion(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                karim_change_view(view->karim, KARIM_TYPE_WORK);
            } else if (!strcmp(id, "year")) {
                _nemoshow_item_motion_bounce(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
                _nemoshow_item_motion(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                _nemoshow_item_motion(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color1,
                        NULL);
                _nemoshow_item_motion(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "fill", color0,
                        NULL);
                karim_change_view(view->karim, KARIM_TYPE_YEAR);
            }
        } else {
            if (!strcmp(id, "region")) {
                _nemoshow_item_motion_bounce(view->btn_region, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
            } else if (!strcmp(id, "work")) {
                _nemoshow_item_motion_bounce(view->btn_wave, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
            } else if (!strcmp(id, "year")) {
                _nemoshow_item_motion_bounce(view->btn_year, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "sx", 0.8, 1.0, "sy", 0.8, 1.0,
                        NULL);
            }
        }

        nemoshow_dispatch_frame(view->show);
    }
}

static void _menu_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    MenuView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->grab) {
            view->grab = nemowidget_create_grab(widget, event,
                    _menu_view_grab_event, one);
        }
    }
}

static MenuView *menu_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    MenuView *view = calloc(sizeof(MenuView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_append_callback(widget, "event", _menu_view_event, view);

    double sx, sy;
    sx = view->w/1920.0;
    sy = view->h/1080.0;
    double gw = 440 * sx;
    double gh = 140 * sy;

    struct showone *group;
    struct showone *one;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    nemoshow_item_set_width(group, gw);
    nemoshow_item_set_height(group, gh);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_translate(group, width/2 + gw/2, height * 0.9 + gh/2);

    double stroke_alpha = 0.88;
    double fill_alpha = 0.5;
    int i;
    for (i = 2; i <= 7 ; i++) {
        char uri[PATH_MAX];
        snprintf(uri, PATH_MAX, "%s%d.svg", APP_ICON_DIR"/menu/karim-buttonoutline-", i);
        double w, h;
        svg_get_wh(uri, &w, &h);
        w = w * sx;
        h = h * sx;
        one = SVG_PATH_GROUP_CREATE(group, w, h, uri);
        nemoshow_item_set_fill_color(one, 255, 255, 255, 255.0 * stroke_alpha);
        stroke_alpha -= 0.11;
        nemoshow_item_set_fill_color(one, 255, 255, 255, 255.0 * fill_alpha);
        fill_alpha -= 0.07;
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_scale(one, 0.8, 0.8);
        nemoshow_item_set_alpha(one, 0.0);
        view->outs = list_append(view->outs, one);
    }

    double w, h;
    const char *uri;
    uri =  APP_ICON_DIR"/menu/karim-buttonoutline-1.svg";
    svg_get_wh(uri, &w, &h);
    w = w * sx;
    h = h * sx;
    view->out = one = SVG_PATH_GROUP_CREATE(group, w, h, uri);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    double ww, hh;
    uri =  APP_ICON_DIR"/menu/karim-button-1.svg";
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sx;
    view->btn_region = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "region");
    nemoshow_one_set_userdata(one, view);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, 53 * sx - (w - ww)/2, 25 * sy - (h - hh)/2);
    nemoshow_item_set_fill_color(one, RGBA(0xF7ACB87F));

    uri =  APP_ICON_DIR"/menu/karim-button-2.svg";
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sx;
    view->btn_wave = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "work");
    nemoshow_one_set_userdata(one, view);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, 175 * sx - (w - ww)/2, 25 * sy - (h - hh)/2);
    nemoshow_item_set_fill_color(one, RGBA(0xF7ACB87F));

    uri =  APP_ICON_DIR"/menu/karim-button-3.svg";
    svg_get_wh(uri, &ww, &hh);
    ww = ww * sx;
    hh = hh * sx;
    view->btn_year = one = SVG_PATH_GROUP_CREATE(group, ww, hh, uri);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "year");
    nemoshow_one_set_userdata(one, view);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, 297 * sx - (w - ww)/2, 25 * sy - (h - hh)/2);
    nemoshow_item_set_fill_color(one, RGBA(0xF7ACB87F));

    view->timer = TOOL_ADD_TIMER(view->tool, 0, _menu_view_timeout, view);

    return view;
}

static void menu_view_show(MenuView *view, uint32_t easetype, int duration, int delay)
{
    double sx, sy;
    sx = view->w/1920.0;
    sy = view->h/1080.0;
    double gw = 440 * sx;
    double gh = 140 * sy;

    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    _nemoshow_item_motion(view->group, easetype, duration, delay,
            "alpha", 1.0,
            "sx", 1.5, "sy", 1.5,
            "tx", view->w/2.0 + gw/2*1.5, "ty", view->h/2.0 + gh/2*1.5,
            NULL);
    nemoshow_dispatch_frame(view->show);
    nemotimer_set_timeout(view->timer, duration + delay);
}

#if 0
static void menu_view_right(MenuView *view, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(view->group, easetype, duration, delay,
            "tx", (double)view->w,
            NULL);
    nemoshow_dispatch_frame(view->show);
}
#endif

static void menu_view_hide(MenuView *view, uint32_t easetype, int duration, int delay)
{
    /*
    double sx, sy;
    sx = view->w/1920.0;
    sy = view->h/1080.0;
    double gw = 440 * sx;
    double gh = 140 * sy;
    */

    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);
    _nemoshow_item_motion(view->group, easetype, duration, delay,
            "alpha", 0.0,
            /*
            "sx", 1.5, "sy", 1.5,
            "tx", view->w/2.0 + gw/2*1.5, "ty", view->h/2.0 + gh/2*1.5,
            */
            NULL);
    nemoshow_dispatch_frame(view->show);
    nemotimer_set_timeout(view->timer, 0);
}

typedef struct _IntroGrab IntroGrab;
struct _IntroGrab {
    int cnt;
    uint32_t device;
};

struct _IntroView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;
    double scale;

    Path *path0, *path1;
    List *paths;

    struct showone *group;
    struct showone *logo;
    NemoWidgetGrab *logo_grab;
    List *grabs;

    List *effects;
    int idx;

    List *touch_effects;

    struct nemotimer *timer;
};

typedef struct _IntroEffect IntroEffect;
struct _IntroEffect {
    IntroView *view;
    unsigned long time;
    List *ones;
};

static void intro_effect_destroy(IntroEffect *fx)
{
    struct showone *one;
    LIST_FREE(fx->ones, one) {
        nemoshow_one_destroy(one);
    }
}

static void intro_effect_hide(IntroEffect *fx, uint32_t easetype, int duration, int delay)
{
    List *l;
    struct showone *one;
    int _delay = 0;
    LIST_FOR_EACH_REVERSE(fx->ones, l, one) {
        _nemoshow_item_motion(one, easetype, duration, delay + _delay,
                "alpha", 0.0, "sx", fx->view->scale, "sy", fx->view->scale,
                NULL);
        _delay += 50;
    }
}

static void intro_view_destroy(IntroView *view)
{
    nemotimer_destroy(view->timer);

    Path *path;
    LIST_FREE(view->paths, path) path_destroy(path);
    path_destroy(view->path0);
    path_destroy(view->path1);

    IntroEffect *fx;
    LIST_FREE(view->effects, fx) {
        intro_effect_destroy(fx);
    }

    nemoshow_one_destroy(view->group);
    nemowidget_destroy(view->widget);
    free(view);
}

static void intro_view_show(IntroView *view, uint32_t easetype, int duration, int delay)
{
    view->idx = 0;
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
    nemotimer_set_timeout(view->timer, 10);
    nemoshow_dispatch_frame(view->show);
}

static void intro_view_hide(IntroView *view, uint32_t easetype, int duration, int delay)
{
    IntroGrab *grab;
    LIST_FREE(view->grabs, grab) {
        free(grab);
    }
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);

    _nemoshow_item_motion(view->logo, easetype, duration/2, delay,
            "sx", 0.0, "sy", 0.0,
            NULL);

    int cnt = list_count(view->effects);
    int idx = view->idx - 1;
    if (idx < 0) {
        idx = cnt - 1;
    }
    int _delay = delay;
    do {
        IntroEffect *fx = LIST_DATA(list_get_nth(view->effects, idx));
        intro_effect_hide(fx, NEMOEASE_CUBIC_IN_TYPE, duration/cnt, _delay);
        _delay += 150;

        if (idx == view->idx) break;
        idx--;
        if (idx < 0) {
            idx = list_count(view->effects) - 1;
        }
    } while (1);

    nemotimer_set_timeout(view->timer, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _intro_view_timeout(struct nemotimer *timer, void *userdata)
{
    IntroView *view = userdata;

    if (!view->logo_grab) {
        _nemoshow_item_motion_bounce(view->logo, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "sx", 1.2, 1.0, "sy", 1.2, 1.0,
                "alpha", 0.5, 1.0,
                NULL);
    }

    IntroEffect *fx = LIST_DATA(list_get_nth(view->effects, view->idx));
    view->idx++;
    if (view->idx >= list_count(view->effects)) {
        view->idx = 0;
    }

    int delay = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH_REVERSE(fx->ones, l, one) {
        nemoshow_item_scale(one, 0.0, 0.0);
        nemoshow_item_set_alpha(one, 0.5);
        nemoshow_item_rotate(one, 0.0);
        _nemoshow_item_motion(one, NEMOEASE_LINEAR_TYPE, 20000, delay,
                "sx", view->scale, "sy", view->scale,
                "alpha", 1.0,
                "ro", 270.0,
                NULL);
        delay += 20;
    }

    List *ll;
    LIST_FOR_EACH_SAFE(view->touch_effects, l, ll, fx) {
        if (fx->time + 6 <= time(NULL)) {
            view->touch_effects = list_remove(view->touch_effects, fx);
            intro_effect_destroy(fx);
        }
    }

    nemoshow_dispatch_frame(view->show);
    nemotimer_set_timeout(timer, 5200 + delay);
}

static void intro_view_add_effect(IntroView *view, double ex, double ey)
{
    IntroEffect *fx = malloc(sizeof(IntroEffect));
    fx->time = time(NULL);
    view->touch_effects = list_append(view->touch_effects, fx);
    fx->ones = NULL;

    struct showone *one;
    one = path_draw(view->path0, view->group);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, ex, ey);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_alpha(one, 1.0);
    fx->ones = list_append(fx->ones, one);

    List *l;
    Path *path;
    LIST_FOR_EACH(view->paths, l, path) {
        one = path_draw(path, view->group);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, ex, ey);
        nemoshow_item_scale(one, 0.0, 0.0);
        nemoshow_item_set_alpha(one, 1.0);
        fx->ones = list_append(fx->ones, one);
    }

    one = path_draw(view->path1, view->group);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, ex, ey);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_alpha(one, 1.0);
    fx->ones = list_append(fx->ones, one);

    int delay = 0;
    LIST_FOR_EACH_REVERSE(fx->ones, l, one) {
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_OUT_TYPE, 3000, delay,
                "sx", 0.5, "sy", 0.5,
                "alpha", 0.0,
                "ro", 120.0,
                NULL);
        delay += 20;
    }
}

static void _intro_view_logo_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    struct showone *one = userdata;
    IntroView *view = nemoshow_one_get_userdata(one);
    if (nemoshow_event_is_down(show, event)) {
        _nemoshow_item_motion_bounce(view->logo, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 0.5, 0.75, "sy", 0.5, 0.75,
                NULL);
    } else if (nemoshow_event_is_up(show, event)) {
        _nemoshow_item_motion_bounce(view->logo, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 1.25, 1.0, "sy", 1.25, 1.0,
                NULL);
        view->logo_grab = NULL;

        karim_change_view(view->karim, KARIM_TYPE_MENU);
    }
}

static void _intro_view_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    IntroView *view = userdata;
    if (nemoshow_event_is_down(show, event)) {
        intro_view_add_effect(view, ex, ey);
        IntroGrab *ig = malloc(sizeof(IntroGrab));
        ig->cnt = 10;
        ig->device = nemoshow_event_get_device(event);
        view->grabs = list_append(view->grabs, ig);
    } else if (nemoshow_event_is_motion(show, event)) {
        List *l;
        IntroGrab *ig;
        LIST_FOR_EACH(view->grabs, l,  ig) {
            if (ig->device == nemoshow_event_get_device(event)) {
                ig->cnt--;
                if (ig->cnt == 0) {
                    intro_view_add_effect(view, ex, ey);
                    ig->cnt = 10;
                }
                break;
            }
        }
    } else if (nemoshow_event_is_up(show, event)) {
        List *l, *ll;
        IntroGrab *ig;
        LIST_FOR_EACH_SAFE(view->grabs, l,  ll, ig) {
            if (ig->device == nemoshow_event_get_device(event)) {
                view->grabs = list_remove(view->grabs, ig);
                free(ig);
            }
        }
    }
}

static void _intro_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    IntroView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        one = nemowidget_pick_one(view->widget, ex, ey);
        if (one && !view->logo_grab) {
            view->logo_grab = nemowidget_create_grab(widget, event,
                    _intro_view_logo_grab_event, one);
        } else {
            nemowidget_create_grab(widget, event,
                    _intro_view_grab_event, view);
        }
    }
}

static IntroView *intro_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    IntroView *view = calloc(sizeof(IntroView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    const char *uri;
    uri = APP_ICON_DIR"/intro/intro-motion1.svg";
    view->path0 = svg_get_path(uri);
    uri = APP_ICON_DIR"/intro/intro-motion2.svg";
    view->path1 = svg_get_path(uri);
    view->paths = path_get_median(view->path0, view->path1, 10);

    double sx, sy;
    sx = (double)width/view->path0->width;
    sy = (double)height/view->path0->height;
    if (sx > sy) view->scale = sx * 2.25;
    else view->scale = sy * 2.25;

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _intro_view_event, view);
    nemowidget_set_alpha(view->widget, 0, 0, 0, 0.0);

    struct showone *group;
    struct showone *one;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    double w, h;

    uri = APP_ICON_DIR"/intro/intro-karimlogo.svg";
    svg_get_wh(uri, &w, &h);
    view->logo = one = SVG_PATH_GROUP_CREATE(group, w, h, uri);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_userdata(one, view);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, width/2, height/2);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_alpha(one, 0.0);

    int i;
    int cnt = 4;
    for (i = 0 ; i < cnt ; i++) {
        IntroEffect *fx = malloc(sizeof(IntroEffect));
        fx->view = view;
        view->effects = list_append(view->effects, fx);
        fx->ones = NULL;

        one = path_draw(view->path0, group);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, width/2, height/2);
        nemoshow_item_scale(one, 0.0, 0.0);
        nemoshow_item_set_alpha(one, 0.0);
        fx->ones = list_append(fx->ones, one);

        List *l;
        Path *path;
        LIST_FOR_EACH(view->paths, l, path) {
            one = path_draw(path, group);
            nemoshow_item_set_anchor(one, 0.5, 0.5);
            nemoshow_item_translate(one, width/2, height/2);
            nemoshow_item_scale(one, 0.0, 0.0);
            nemoshow_item_set_alpha(one, 0.0);
            fx->ones = list_append(fx->ones, one);
        }

        one = path_draw(view->path1, group);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, width/2, height/2);
        nemoshow_item_scale(one, 0.0, 0.0);
        nemoshow_item_set_alpha(one, 0.0);
        fx->ones = list_append(fx->ones, one);
    }

    view->timer = TOOL_ADD_TIMER(view->tool, 0, _intro_view_timeout, view);

    return view;
}

#if 0
typedef struct _SaverWave SaverWave;
struct _SaverWave {
    SaverView *view;
    struct showone *group;
    Path *path0, *path1;
    List *ones;
    struct nemotimer *timer;
    int idx;
};

struct _SaverView {
    Karim *karim;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *parent;

    NemoWidget *bg_widget;
    Image *bg;

    NemoWidget *widget;
    struct showone *group;
    struct showone *logo;
    List *waves;
};

static void _saver_wave_timeout(struct nemotimer *timer, void *userdata)
{
    SaverWave *wave = userdata;

    int cnt = list_count(wave->ones);

    int duration = 10000;
    uint32_t easetype = NEMOEASE_SINUSOIDAL_IN_TYPE;

    struct showone *one;
    one = LIST_DATA(list_get_nth(wave->ones, wave->idx));
    nemoshow_item_set_alpha(one, 0.0);
    _nemoshow_item_motion(one, easetype, 500, 0,
            "alpha", 1.0, NULL);
    path_array_morph(one, 0, 0, 0, wave->path0);
    path_array_morph(one, easetype, duration, 0, wave->path1);

    wave->idx++;
    if (wave->idx >= cnt) wave->idx = 0;

    nemotimer_set_timeout(wave->timer, duration/cnt);
    nemoshow_dispatch_frame(wave->view->show);
}

SaverWave *saver_view_create_wave(SaverView *view, const char *uri0, const char *uri1,
        double alpha0, double alpha1, int cnt)
{
    SaverWave *wave = calloc(sizeof(SaverWave), 1);
    wave->view = view;

    struct showone *one;
    struct showone *group;
    wave->group = group = GROUP_CREATE(view->group);

    wave->path0 = svg_get_path(uri0);
    wave->path1 = svg_get_path(uri1);

    uint32_t mask0 = 0xFFFFFF00 + 255 * alpha0;
    uint32_t mask1 = 0xFFFFFF00 + 255 * 0.0;
    wave->path0->fill &= mask0;
    wave->path0->stroke &= mask0;
    wave->path1->fill &= mask1;
    wave->path1->stroke &= mask1;

    double sx, sy;
    sx = view->w/3840.0;
    sy = view->h/2160.0;

    double w, h;
    const char *uri;
    uri = APP_ICON_DIR"/saver/BG-logo.svg";
    svg_get_wh(uri, &w, &h);
    w = w * sx;
    h = h * sy;
    view->logo = one = SVG_PATH_GROUP_CREATE(group, w, h, uri);
    nemoshow_item_translate(one, view->w/2, view->h/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);

    int i;
    for (i = 0 ; i < cnt ; i++) {
        one = path_draw_array(wave->path0, group);
        nemoshow_item_scale(one, sx, sy);
        nemoshow_item_set_alpha(one, 0.0);
        wave->ones = list_append(wave->ones, one);
    }

    wave->timer = TOOL_ADD_TIMER(view->tool, 0, _saver_wave_timeout, wave);

    return wave;
}

static void saver_wave_show(SaverWave *wave, uint32_t easetype, int duration, int delay)
{
    wave->idx = 0;
    nemotimer_set_timeout(wave->timer, 10 + delay);
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(wave->group, 1.0);
    }
}

static void saver_wave_hide(SaverWave *wave, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(wave->timer, 0);
    List *l;
    struct showone *one;
    LIST_FOR_EACH(wave->ones, l, one) {
        path_array_morph(one, easetype, duration, delay, wave->path0);
    }
    if (duration > 0) {
        _nemoshow_item_motion(wave->group, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(wave->group, 0.0);
    }
}

void saver_view_show(SaverView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->bg_widget, 0, 0, 0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration, delay, 1.0);
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    nemowidget_scale(view->widget, easetype, duration, delay, 1.0, 1.0);
    nemowidget_translate(view->widget, easetype, duration, delay, 0, 0);

    List *l;
    SaverWave *wave;
    int _delay = 0;
    int i = 1;
    LIST_FOR_EACH(view->waves, l, wave) {
        saver_wave_show(wave, easetype, duration, delay + _delay);
        _delay += 250;
        i++;
    }
    nemoshow_dispatch_frame(view->show);
}

void saver_view_hide(SaverView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->bg_widget, 0, 0, 0);
    nemowidget_set_alpha(view->bg_widget, easetype, duration, delay, 0.0);
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);
    nemowidget_scale(view->widget, easetype, duration, delay, 0.0, 0.0);
    nemowidget_translate(view->widget, easetype, duration, delay, view->w/2, view->h/2);

    List *l;
    SaverWave *wave;
    LIST_FOR_EACH(view->waves, l, wave) {
        saver_wave_hide(wave, easetype, duration, delay);
    }
    nemoshow_dispatch_frame(view->show);
}

static void _saver_view_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    SaverView *view = userdata;
    Karim *karim = view->karim;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        nemotimer_set_timeout(karim->saver_timer, SAVER_TIMEOUT);
        saver_view_hide(view, NEMOEASE_CUBIC_IN_TYPE, 3000, 0);
        if (karim->type == KARIM_TYPE_NONE) {
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
        } else if (karim->type == KARIM_TYPE_INTRO) {
            intro_view_show(karim->intro, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
        } else if (karim->type == KARIM_TYPE_HONEY) {
            honey_view_show(karim->honey, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
        } else if (karim->type == KARIM_TYPE_VIEWER) {
            viewer_view_show(karim->viewer, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
        } else {
            menu_view_show(karim->menu, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            if (karim->type == KARIM_TYPE_REGION) {
                //menu_view_right(karim->menu, NEMOEASE_LINEAR_TYPE, 1000, 0);
                region_view_show(karim->region, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            } else if (karim->type == KARIM_TYPE_WORK) {
                work_view_show(karim->work, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            } else if (karim->type == KARIM_TYPE_YEAR) {
                year_view_show(karim->year, NEMOEASE_CUBIC_OUT_TYPE, 1000, 500);
            }
        }
    }
}

SaverView *saver_view_create(Karim *karim, NemoWidget *parent, int width, int height)
{
    SaverView *view = calloc(sizeof(SaverView), 1);
    view->karim = karim;
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->w = width;
    view->h = height;

    // Designed for 3840x2160
    double sx= view->w/3840.0;
    double sy = view->h/2160.0;

    NemoWidget *widget;
    view->bg_widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    int w, h;
    const char *uri = APP_IMG_DIR"/saver/BG-base.png";
    file_get_image_wh(uri, &w, &h);
    w *= sx;
    h *= sy;

    Image *img;
    view->bg = img = image_create(nemowidget_get_canvas(widget));
    image_load_full(img, view->tool, uri, w, h, NULL, NULL);

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _saver_view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_scale(view->widget, 0, 0, 0, 0.0, 0.0);
    nemowidget_translate(view->widget, 0, 0, 0, view->w/2, view->h/2);

    struct showone *group;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    int i;
    for (i = 1 ; i <= 5; i++) {
        char buf0[PATH_MAX], buf1[PATH_MAX];
        snprintf(buf0, PATH_MAX, APP_ICON_DIR"/saver/BG-%d-1.svg", i);
        snprintf(buf1, PATH_MAX, APP_ICON_DIR"/saver/BG-%d-2.svg", i);
        double a0, a1;
        if (i == 1) {
            a0 = 1.0;
            a1 = 0.5;
        } else if (i == 2 || i == 3) {
            a0 = 0.5;
            a1 = 0.3;
        } else if (i == 4 || i == 5) {
            a0 = 0.8;
            a1 = 0.1;
        }

        int cnt = 10;
        if (i == 4 || i == 5) cnt = 13;
        if (i == 2 || i == 3) cnt = 20;
        SaverWave *wave;
        wave = saver_view_create_wave(view, buf0, buf1, a0, a1, cnt);
        view->waves = list_append(view->waves, wave);
    }

    return view;
}

static void _karim_saver_timeout(struct nemotimer *timer, void *userdata)
{
    Karim *karim = userdata;
    saver_view_show(karim->saver, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    menu_view_hide(karim->menu, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    if (karim->type == KARIM_TYPE_INTRO) {
        intro_view_hide(karim->intro, NEMOEASE_CUBIC_IN_TYPE, 2000, 0);
    } else if (karim->type == KARIM_TYPE_REGION) {
        region_view_hide(karim->region, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    } else if (karim->type == KARIM_TYPE_WORK) {
        work_view_hide(karim->work, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    } else if (karim->type == KARIM_TYPE_YEAR) {
        year_view_hide(karim->year, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    } else if (karim->type == KARIM_TYPE_HONEY) {
        honey_view_hide(karim->honey, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    } else if (karim->type == KARIM_TYPE_VIEWER) {
        viewer_view_hide(karim->viewer, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    }
}
#endif

static void _dispatch_bus(void *userdata, const char *events)
{
    Karim *karim = userdata;

	struct nemojson *json;
	struct nemoitem *msg;
	struct itemone *one;
	char buffer[4096];
	int length;
	int i;

	length = nemobus_recv(karim->bus, buffer, sizeof(buffer));
	if (length <= 0) {
        ERR("length is 0");
		return;
    }

	json = nemojson_create_string(buffer, length);
	nemojson_update(json);

	for (i = 0; i < nemojson_get_count(json); i++) {
		msg = nemoitem_create();
		nemojson_object_load_item(nemojson_get_object(json, i), msg, "/nemokarim");

		nemoitem_for_each(one, msg) {
			if (nemoitem_one_has_path_suffix(one, "/in") != 0) {
                ERR("karim in!!");
                if (karim->type == KARIM_TYPE_INTRO) {
                    ERR("Hide INTRO!!");
                    _nemoshow_item_motion_bounce(karim->intro->logo, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "sx", 1.25, 1.0, "sy", 1.25, 1.0,
                            NULL);
                    karim->intro->logo_grab = NULL;

                    karim_change_view(karim, KARIM_TYPE_MENU);
                }
            } else if (nemoitem_one_has_path_suffix(one, "/out") != 0) {
                ERR("karim out!!");
                if (karim->type != KARIM_TYPE_INTRO) {
                    ERR("Show INTRO!!");
                    karim_change_view(karim, KARIM_TYPE_INTRO);
                }
                break;
            }
		}

		nemoitem_destroy(msg);
	}

	nemojson_destroy(json);
}

static Karim *karim_create(NemoWidget *parent, int width, int height)
{
    Karim *karim = calloc(sizeof(Karim), 1);
    karim->parent = parent;
    karim->w = width;
    karim->h = height;
    karim->tool = nemowidget_get_tool(parent);
    karim->show = nemowidget_get_show(parent);
    karim->uuid = nemowidget_get_uuid(parent);

    struct nemobus *bus;
    const char *busid = "/nemokarim";
    karim->bus = bus = nemobus_create();

    nemobus_connect(bus, NULL);
    nemobus_advertise(bus, "set", busid);

    nemotool_watch_source(karim->tool,
            nemobus_get_socket(bus),
            "reh",
            _dispatch_bus,
           karim);

    karim->datas = karim_data_load(CONTENT,
            &(karim->countries), &(karim->categories), &(karim->years));
#if 0
    List *l;
    KarimData *data;
    LIST_FOR_EACH(karim->datas, l, data) {
        ERR("[%s][%s][%s][%s][%s]", data->country, data->category, data->year, data->title, data->path);
    }

    ERR("*****************************************");

    KarimGroup *group;
    LIST_FOR_EACH(karim->countries, l, group) {
        ERR("============= [%s] ============", group->name);
        List *ll;
        KarimData *data;
        LIST_FOR_EACH(group->datas, ll, data) {
            ERR("[%s][%s][%s][%s][%s]", data->country, data->category, data->year, data->title, data->path);
        }
    }
    ERR("*****************************************");

    LIST_FOR_EACH(karim->categories, l, group) {
        ERR("============= [%s] ============", group->name);
        List *ll;
        KarimData *data;
        LIST_FOR_EACH(group->datas, ll, data) {
            ERR("[%s][%s][%s][%s][%s]", data->category, data->country, data->year, data->title, data->path);
        }
    }
    ERR("*****************************************");

    LIST_FOR_EACH(karim->years, l, group) {
        ERR("============= [%s] ============", group->name);
        List *ll;
        KarimData *data;
        LIST_FOR_EACH(group->datas, ll, data) {
            ERR("[%s][%s][%s][%s][%s]", data->year, data->country, data->category, data->title, data->path);
        }
    }
#endif

    NemoWidget *widget;
    karim->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *one;
    const char *uri = APP_ICON_DIR"/bg.svg";
    karim->bg = one = SVG_PATH_GROUP_CREATE(nemowidget_get_canvas(widget), width, height, uri);

    karim->intro = intro_view_create(karim, parent, width, height);
    karim->region = region_view_create(karim, parent, width, height);
    karim->work = work_view_create(karim, parent, width, height);
    karim->year = year_view_create(karim, parent, width, height);
    karim->menu = menu_view_create(karim, parent, width, height);
    karim->viewer = viewer_view_create(karim, karim->parent, karim->w, karim->h);

    //karim->saver_timer = TOOL_ADD_TIMER(karim->tool, 0, _karim_saver_timeout, karim);

    karim->event_widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event_repeat(widget, true);

    //karim->saver = saver_view_create(karim, parent, width, height);

    return karim;
}

static void karim_destroy(Karim *karim)
{
    intro_view_destroy(karim->intro);
    nemobus_destroy(karim->bus);
    free(karim);
}

static void karim_show(Karim *karim, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(karim->widget, 0, 0, 0);
    nemowidget_set_alpha(karim->widget, easetype, duration, delay, 1.0);
    nemowidget_show(karim->event_widget, 0, 0, 0);

    karim_change_view(karim, KARIM_TYPE_INTRO);

    //nemotimer_set_timeout(karim->saver_timer, SAVER_TIMEOUT);
    //saver_view_show(karim->saver, easetype, duration, delay);
}

static void karim_hide(Karim *karim, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(karim->widget, 0, 0, 0);
    nemowidget_set_alpha(karim->widget, easetype, duration, delay, 0.0);
    nemowidget_hide(karim->event_widget, 0, 0, 0);

    karim_change_view(karim, KARIM_TYPE_NONE);

    //nemotimer_set_timeout(karim->saver_timer, 0);
    //saver_view_hide(karim->saver, easetype, duration, delay);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Karim *karim = userdata;

    karim_hide(karim, NEMOEASE_CUBIC_IN_TYPE, 500, 0);

    nemowidget_win_exit_after(win, 500);
}

int main(int argc, char *argv[])
{
    xemoapp_init();
    Config *config;
    config = config_load(PROJECT_NAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, config);
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    Karim *karim;
    karim = karim_create(win, config->width, config->height);
    nemowidget_append_callback(win, "exit", _win_exit, karim);
    karim_show(karim, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    karim_destroy(karim);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(config);
    xemoapp_shutdown();

    return 0;
}
