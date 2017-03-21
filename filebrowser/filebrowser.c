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

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define COLOR_BASE 0x00FFC2FF
#define COLOR_ICON 0xFFFFFFFF
#define COLOR_TXT_BACK 0x00BF92FF
#define COLOR_TXT 0xFFFFFFFF

#define BACK_INTERVAL 5000
#define VIDEO_INTERVAL (1000.0/24)

#define GRADIENT0 0x38264CFF
#define GRADIENT1 0x53AED6B2
#define GRADIENT2 0xB2C3E433
#define GRADIENT3 0x53AED6E5
#define GRADIENT4 0xFFFFFFFF
#define GRADIENT5 0x6767677F

size_t strlen_utf8(const char *str)
{
    RET_IF(!str, -1);
    size_t len = 0;
    while (str && (*str != '\0')) {
        //ERR("[0x%x]: (%c)", *str, *str);
        if ((0x80 <= *str) && (*str <= 0xBF)) {
            str++;
            continue;
        }
        str++;
        len++;
    }
    return len;
}

typedef struct _Pos Pos;
struct _Pos
{
    double x, y;
};

static void path_update_diamond(struct showone *one, int w, int h)
{
    double r = 16;
    nemoshow_item_path_moveto(one, w/2 + r, r);
    nemoshow_item_path_cubicto(one, w/2, 0, w/2, 0, w/2 - r, r);
    nemoshow_item_path_lineto(one, r, h/2 - r);
    nemoshow_item_path_cubicto(one, 0, h/2, 0, h/2, r, h/2 + r);
    nemoshow_item_path_lineto(one, w/2 - r, h - r);
    nemoshow_item_path_cubicto(one, w/2, h, w/2, h, w/2 + r, h - r);
    nemoshow_item_path_lineto(one, w - r, h/2 + r);
    nemoshow_item_path_cubicto(one, w, h/2, w, h/2, w - r, h/2 - r);
    nemoshow_item_path_close(one);
}

// anchor: 1.0, 1.0
struct showone *path_diamond_create(struct showone *parent, int w, int h)
{
    struct showone *one;
    one = PATH_CREATE(parent);

    path_update_diamond(one, w, h);

    return one;
}


// anchor 0.5, 0.5
struct showone *path_diamond_create2(struct showone *parent, int w, int h)
{
    double r = 16;
    struct showone *one;
    one = PATH_CREATE(parent);

    nemoshow_item_path_moveto(one, r, r - h/2);
    nemoshow_item_path_cubicto(one, 0, -h/2, 0, -h/2, -r, r - h/2);
    nemoshow_item_path_lineto(one, r - w/2, -r);
    nemoshow_item_path_cubicto(one, -w/2, 0, -w/2, 0, r - w/2, r);
    nemoshow_item_path_lineto(one, -r, h/2 - r);
    nemoshow_item_path_cubicto(one, 0, h/2, 0, h/2, r, h/2 - r);
    nemoshow_item_path_lineto(one, w/2 - r, r);
    nemoshow_item_path_cubicto(one, w/2, 0, w/2, 0, w/2 - r, -r);
    nemoshow_item_path_close(one);

    return one;
}

struct showone *path_diamond_create3(struct showone *parent, int w, int h, int r)
{
    struct showone *one;
    one = PATH_CREATE(parent);

    nemoshow_item_path_moveto(one, w/2.0, 0.0);
    nemoshow_item_path_lineto(one, 0.0, h/2.0);
    nemoshow_item_path_lineto(one, w/2.0, h);
    nemoshow_item_path_lineto(one, w, h/2.0);
    nemoshow_item_path_close(one);

    return one;
}

static char *_explorer_get_thumb_url(const char *path, const char *ext)
{
    if (!path) return NULL;
    char *base = file_get_basename(path);
    char *dir = file_get_dirname(path);
    char *ret;
    if (ext)  {
        ret = strdup_printf("%s/%s/%s.%s", dir, FB_THUMB_DIR, base, ext);
    } else {
        ret = strdup_printf("%s/%s/%s", dir, FB_THUMB_DIR, base);
    }
    free(base);
    free(dir);
    return ret;
}


List *__filetypes = NULL;
typedef struct _FileType FileType;
struct _FileType {
    char *type;
    char *icon;
    char *exec;
    char *ext;
    char *magic;
};

static void filetype_destroy(FileType *filetype)
{
    if (filetype->type) free(filetype->type);
    if (filetype->icon) free(filetype->icon);
    if (filetype->exec) free(filetype->exec);
    if (filetype->ext) free(filetype->ext);
    if (filetype->magic) free(filetype->magic);
    free(filetype);
}

static const FileType *_fileinfo_match_filetype(FileInfo *fileinfo)
{
    List *l;
    FileType *ft;
    LIST_FOR_EACH(__filetypes, l, ft) {
        if (ft->ext && fileinfo->ext &&
                !strcasecmp(ft->ext, fileinfo->ext)) {
            return ft;
        } else if (ft->magic && fileinfo->magic_str &&
                strstr(fileinfo->magic_str, ft->magic)) {
            return ft;
        }
    }
    return NULL;
}

static FileType *filetype_parse_tag(XmlTag *tag)
{
    List *ll;
    XmlAttr *attr;
    FileType *filetype = NULL;
    filetype = calloc(sizeof(FileType), 1);
    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                if (filetype->type) free(filetype->type);
                filetype->type = strdup(attr->val);
            }
        }
        if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                if (filetype->icon) free(filetype->icon);
                filetype->icon = strdup(attr->val);
            }
        }
        if (!strcmp(attr->key, "exec")) {
            if (attr->val) {
                if (filetype->exec) free(filetype->exec);
                filetype->exec = strdup(attr->val);
            }
        }
        if (!strcmp(attr->key, "ext")) {
            if (attr->val) {
                if (filetype->ext) free(filetype->ext);
                filetype->ext = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "magic")) {
            if (attr->val) {
                if (filetype->magic) free(filetype->magic);
                filetype->magic = strdup(attr->val);
            }
        }
    }
    if (!filetype->type) {
        filetype_destroy(filetype);
        ERR("No file type specfied: magic(%s),ext(%s)", filetype->magic, filetype->ext);
        return NULL;
    }
    if (!filetype->ext && !filetype->magic) {
        filetype_destroy(filetype);
        ERR("No file ext or magic specfied: type(%s)", filetype->type);
        return NULL;
    }
    return filetype;
}

typedef enum {
    EXPLORER_ICON_TYPE_QUIT = 0,
    EXPLORER_ICON_TYPE_UP,
    EXPLORER_ICON_TYPE_PREV,
    EXPLORER_ICON_TYPE_NEXT,
} ExplorerIconType;

typedef struct _Explorer Explorer;
typedef struct _ExplorerIcon ExplorerIcon;

struct _ExplorerIcon {
    Explorer *exp;
    ExplorerIconType type;
    int w, h;

    struct showone *group;

    struct showone *bg;
    struct showone *icon;
};

struct _Explorer {
    const char *uuid;
    int w, h;
    int x_pad, y_pad;
    int it_gap;
    int itw, ith;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;

    bool bgidx;
    Image *bg;

    struct showone *group;
    struct showone *title[2];
    int title_idx;

    struct showone *it_group;
    List *items;

    struct showone *icon_group;
    ExplorerIcon *prev, *next;

    char *bgpath_local;
    char *bgpath;

    char *rootpath;
    char *curpath;

    int cnt_inrow;
    int cnt_row;
    int cnt_inpage;

    int page_idx;
    int page_cnt;

    Thread *filethread;
    List *fileinfos;

    Worker *img_worker;
};

typedef enum {
    EXPLORER_ITEM_TYPE_DIR = 0,
    EXPLORER_ITEM_TYPE_FILE,
    EXPLORER_ITEM_TYPE_IMG,
    EXPLORER_ITEM_TYPE_SVG,
    EXPLORER_ITEM_TYPE_VIDEO,
} ExplorerItemType;

typedef struct _ExplorerItem ExplorerItem;
struct _ExplorerItem {
    Explorer *exp;
    ExplorerItemType type;

    int gap;
    int w, h;
    char *exec;
    char *path;

    int video_idx;
    struct nemotimer *video_timer;

    struct showone *blur;
    struct showone *font;

    struct showone *group;

    struct showone *bg;

    struct showone *icon_bg;
    struct showone *icon;

    char *txt_str;
    struct showone *txt_bg;
    struct showone *txt;

    Image *img;
    struct showone *svg;
    struct showone *video;
};

static void explorer_icon_destroy(ExplorerIcon *icon)
{
    nemoshow_one_destroy(icon->group);
    free(icon);
}

static ExplorerIcon *explorer_icon_create(Explorer *exp, ExplorerIconType type, const char *uri)
{
    ExplorerIcon *icon = calloc(sizeof(ExplorerIcon), 1);
    icon->exp = exp;
    icon->type = type;

    struct showone *group;
    icon->group = group = GROUP_CREATE(exp->icon_group);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;
    double w, h;
    if (!svg_get_wh(uri, &w, &h)) {
        ERR("svg get wh failed: %s", uri);
    }
    icon->w = w;
    icon->h = h;

    icon->bg = one = RECT_CREATE(group, w, h);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_tag(one, 0x2);
    nemoshow_one_set_userdata(one, icon);
    nemoshow_item_set_alpha(one, 0.0);

    icon->icon = one = SVG_PATH_CREATE(group, w, h, uri);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    return icon;
}

static void explorer_icon_show(ExplorerIcon *icon, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(icon->group, easetype, duration, delay,
            "alpha", 1.0, NULL);
    _nemoshow_item_motion_bounce(icon->icon, easetype, duration, delay + 150,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
}

static void explorer_icon_hide(ExplorerIcon *icon, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(icon->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->icon, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
}

static void explorer_icon_translate(ExplorerIcon *icon, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(icon->group, tx, ty);
    }
}

static void explorer_icon_down(ExplorerIcon *icon)
{
    _nemoshow_item_motion_bounce(icon->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 0.6, 0.7, "sy", 0.6, 0.7,
            NULL);
}

static void explorer_icon_up(ExplorerIcon *icon)
{
    _nemoshow_item_motion_bounce(icon->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
}

typedef struct _ImgWorkData ImgWorkData;
struct _ImgWorkData {
    Image *img;
    char *path;
    struct nemoshow *show;
    int cw, ch;
    int delay;
    double alpha;

    int w, h;
    ImageBitmap *bitmap;
    ExplorerItem *it;
};

static void _explorer_item_img_work_done(bool cancel, void *userdata)
{
    ImgWorkData *data = userdata;

    if (!cancel) {
        if (!data->bitmap) {
            ERR("bitmap is not loaded correctly");
        } else {
            image_set_bitmap(data->img, data->w, data->h, data->bitmap);
            image_set_alpha(data->img,
                    NEMOEASE_CUBIC_INOUT_TYPE, 500, data->delay,
                    data->alpha);
            nemoshow_dispatch_frame(data->show);

            // translate TXT
            ExplorerItem *it = data->it;
            char *buf;
            size_t len = data->w/8;
            buf = malloc(sizeof(char) * len);
            snprintf(buf, len, "%s", it->txt_str);
            nemoshow_item_set_text(it->txt, buf);

            nemoshow_item_translate(it->txt_bg, 0, data->h/2.0 -it->w/8.0/2.0);
            nemoshow_item_translate(it->txt, 0, data->h/2.0 -it->w/8.0/2.0);
            _nemoshow_item_motion(it->txt_bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                    "alpha", 0.6, NULL);
            _nemoshow_item_motion(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                    "alpha", 1.0, NULL);
        }
    } else {
        if (data->bitmap) image_bitmap_destroy(data->bitmap);
    }

    free(data->path);
    free(data);
}

static void _explorer_item_img_work(void *userdata)
{
    ImgWorkData *data = userdata;

    int w, h;
    if (image_get_wh(data->path, &w, &h)) {
        _rect_ratio_fit(w, h, data->cw, data->ch, &(data->w), &(data->h));
        data->bitmap = image_bitmap_create(data->path);
    } else {
        data->w = data->cw;
        data->h = data->ch;
        data->bitmap = NULL;
        ERR("image get width/height failed: %s", data->path);
    }
}

static void _explorer_item_video_timer(struct nemotimer *timer, void *userdata)
{
    ExplorerItem *it = userdata;

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%03d.jpg", it->video_idx);

    char *path = _explorer_get_thumb_url(it->path, buf);

    if (!file_is_exist(path) || file_is_null(path)) {
        ERR("No video thumbnail: %s", path);
        nemotimer_set_timeout(timer, 0);
        return;
    }

    if (nemoshow_item_get_width(it->video) <= 0 ||
            nemoshow_item_get_height(it->video) <= 0) {
        int w, h;
        if (!image_get_wh(path, &w, &h)) {
            ERR("image get wh failed: %s", path);
        } else {
            _rect_ratio_fit(w, h, it->w, it->h, &w, &h);
            nemoshow_item_set_width(it->video, w);
            nemoshow_item_set_height(it->video, h);
            _nemoshow_item_motion(it->video,
                    NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                    "alpha", 1.0,
                    NULL);
        }
        // translate TXT
        char buf[25];
        snprintf(buf, 25, "%s", it->txt_str);
        nemoshow_item_set_text(it->txt, buf);

        nemoshow_item_translate(it->txt_bg, 0, h/2.0 - it->w/8.0/2.0);
        nemoshow_item_translate(it->txt, 0, h/2.0 - it->w/8.0/2.0);
        _nemoshow_item_motion(it->txt_bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.6, NULL);
        _nemoshow_item_motion(it->txt, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 1.0, NULL);
    }

    nemoshow_item_set_uri(it->video, path);
    if (path) free(path);

    nemoshow_dispatch_frame(it->exp->show);

    it->video_idx++;
    if (it->video_idx > 120) it->video_idx = 1;
    nemotimer_set_timeout(timer, VIDEO_INTERVAL);
}

static void explorer_item_show(ExplorerItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0,
                "sx", 1.0, "sy", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
        nemoshow_item_scale(it->group, 1.0, 1.0);
    }

    Explorer *exp = it->exp;
    if (it->type == EXPLORER_ITEM_TYPE_IMG) {
        char *path = _explorer_get_thumb_url(it->path, NULL);
        if (!file_is_exist(path) || file_is_null(path)) {
            free(path);
            path = strdup(it->path);
        }
        if (!file_is_exist(path) || file_is_null(path)) {
            ERR("file(%s) does not exist or has zero size", path);
        } else {
            ImgWorkData *data = calloc(sizeof(ImgWorkData), 1);
            data->show = it->exp->show;
            data->img = it->img;
            data->path = strdup(path);
            data->cw = it->w;
            data->ch = it->h;
            data->delay = delay;
            data->alpha = 1.0;
            data->it = it;
            worker_append_work(exp->img_worker,
                    _explorer_item_img_work,
                    _explorer_item_img_work_done, data);
        }
    } else if (it->type == EXPLORER_ITEM_TYPE_VIDEO) {
        char *path = _explorer_get_thumb_url(it->path, "001.jpg");
        if (!file_is_exist(path) || file_is_null(path)) {
            ERR("No video thumbnail: %s", path);
#if 0
            // Make video thumbnail
            path = strdup(it->path);
            VideoWorkData *data = calloc(sizeof(VideoWorkData), 1);
            data->it = it;
            data->path = path;
            worker_append_work(exp->img_worker, _explorer_item_video_work,
                    _explorer_item_video_work_done, data);
#endif
        } else {
            it->video_idx = 1;
            if (it->video_timer) nemotimer_set_timeout(it->video_timer, 10 + delay);
        }
        free(path);
    }
}

static void explorer_item_hide(ExplorerItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->video_timer) nemotimer_set_timeout(it->video_timer, 0);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
        nemoshow_item_scale(it->group, 0.0, 0.0);
    }
}

static void explorer_item_down(ExplorerItem *it)
{
    _nemoshow_item_motion_bounce(it->group,
            NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 0.6, 0.7, "sy", 0.6, 0.7,
            NULL);
}

static void explorer_item_up(ExplorerItem *it)
{
    _nemoshow_item_motion_bounce(it->group,
            NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
}

static void explorer_item_translate(ExplorerItem *it, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group,
                easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(it->group, tx, ty);
    }
}

static void explorer_show_dir(Explorer *exp, const char *path);
static void explorer_show_page(Explorer *exp, int page_idx);

static void explorer_item_exec(ExplorerItem *it)
{
    Explorer *exp = it->exp;
    struct nemoshow *show = nemowidget_get_show(exp->widget);

    if (it->type == EXPLORER_ITEM_TYPE_DIR) {
        explorer_show_dir(exp, it->path);
    } else {
        float x, y;
        nemoshow_transform_to_viewport(show,
                NEMOSHOW_ITEM_AT(it->group, tx),
                NEMOSHOW_ITEM_AT(it->group, ty),
                &x, &y);

        nemoshow_view_set_anchor(show, 0.5, 0.5);
        //ERR("%s, %s", it->exec, it->path);
        if (!it->exec) {
            nemo_execute(exp->uuid, "app", it->path, NULL, NULL, x, y, 0, 1, 1);
        } else {
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
            snprintf(args, PATH_MAX, "%s;%s", tok, it->path);
            free(buf);
            nemo_execute(exp->uuid, "app", path, args, NULL, x, y, 0, 1, 1);
        }
    }
}

static void _explorer_grab_item_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    ExplorerItem *it = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        explorer_item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        explorer_item_up(it);
        if (nemoshow_event_is_single_click(show, event)) {
            explorer_item_exec(it);
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _explorer_grab_icon_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    ExplorerIcon *icon = userdata;
    Explorer *exp = icon->exp;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        explorer_icon_down(icon);
    } else if (nemoshow_event_is_up(show, event)) {
        explorer_icon_up(icon);
        if (nemoshow_event_is_single_click(show, event)) {
            if (icon->type == EXPLORER_ICON_TYPE_QUIT) {
                NemoWidget *win = nemowidget_get_top_widget(widget);
                nemowidget_callback_dispatch(win, "exit", NULL);
            } else if (icon->type == EXPLORER_ICON_TYPE_UP) {
                char *uppath = file_get_updir(exp->curpath);
                explorer_show_dir(exp, uppath);
                free(uppath);
            } else if (icon->type == EXPLORER_ICON_TYPE_PREV) {
                explorer_show_page(exp, exp->page_idx-1);
            } else if (icon->type == EXPLORER_ICON_TYPE_NEXT) {
                explorer_show_page(exp, exp->page_idx+1);
            }
        }
    }
    nemoshow_dispatch_frame(exp->show);
}

static void _explorer_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    Explorer *exp = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        if ((nemoshow_event_get_tapcount(event) >= 2)) {
            nemoshow_event_set_done_all(event);
            List *l;
            NemoGrab *grab;
            LIST_FOR_EACH(widget->usergrabs, l, grab) {
                NemoWidgetGrab *wgrab = grab->userdata;
                struct showone *one = nemowidget_grab_get_data(wgrab, "one");
                if (nemoshow_one_get_tag(one) == 0x1) {
                    ExplorerItem *it = nemoshow_one_get_userdata(one);
                    explorer_item_up(it);
                } else if (nemoshow_one_get_tag(one) == 0x2) {
                    ExplorerIcon *icon = nemoshow_one_get_userdata(one);
                    explorer_icon_up(icon);
                }
            }
            nemoshow_dispatch_frame(show);

            return;
        }

        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event), nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);
        ERR("%p", one);
        if (one) {
            if (nemoshow_one_get_tag(one) == 0x1) {
                ExplorerItem *it = nemoshow_one_get_userdata(one);
                if (it) {
                    NemoWidgetGrab *grab =
                        nemowidget_create_grab(widget, event,
                                _explorer_grab_item_event, it);
                    nemowidget_grab_set_data(grab, "one", one);
                }
            } else if (nemoshow_one_get_tag(one) == 0x2) {
                ExplorerIcon *icon = nemoshow_one_get_userdata(one);
                if (icon) {
                    NemoWidgetGrab *grab =
                        nemowidget_create_grab(widget, event,
                                _explorer_grab_icon_event, icon);
                    nemowidget_grab_set_data(grab, "one", one);
                }
            }
        }
    }
    nemoshow_dispatch_frame(exp->show);
}

static void explorer_remove_item(Explorer *exp, ExplorerItem *it)
{
    exp->items = list_remove(exp->items, it);

    if (it->video_timer) nemotimer_destroy(it->video_timer);

    free(it->txt_str);
    if (it->img) image_destroy(it->img);
    nemoshow_one_destroy(it->group);
    nemoshow_one_destroy(it->font);
    nemoshow_one_destroy(it->blur);

    if (it->exec) free(it->exec);
    if (it->path) free(it->path);

    free(it);
}

static ExplorerItem *explorer_append_item(Explorer *exp, ExplorerItemType type, const char *icon_uri, const char *txt, const char *exec, const char *path, int w, int h)
{
    ExplorerItem *it = calloc(sizeof(ExplorerItem), 1);
    it->exp = exp;
    it->type = type;
    it->gap = exp->it_gap;
    it->w = w - it->gap;
    it->h = h - it->gap;

    if (exec) it->exec = strdup(exec);
    if (path) it->path = strdup(path);

    struct showone *blur;
    it->blur = blur = BLUR_CREATE("solid", 5);

    struct showone *font;
    font = FONT_CREATE("NanumGothic", "Regular");
    it->font = font;

    struct showone *group;
    group = GROUP_CREATE(exp->it_group);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);
    it->group = group;

    struct showone *one;
    one = RECT_CREATE(group, it->w, it->h);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_one_set_tag(one, 0x1);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_alpha(one, 0.0);
    it->bg = one;

    if (type == EXPLORER_ITEM_TYPE_IMG) {
        it->img = image_create(group);
        image_set_anchor(it->img, 0.5, 0.5);
        image_set_alpha(it->img, 0, 0, 0, 0.0);
    } else if (type == EXPLORER_ITEM_TYPE_SVG) {
        // FIXME: Need thread???
        ITEM_CREATE(one, group, NEMOSHOW_PATHGROUP_ITEM);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_set_alpha(one, 1.0);
        it->svg = one;

        double ww, hh;
        if (!svg_get_wh(it->path, &ww, &hh)) {
            ERR("svg get wh failed: %s", it->path);
        } else {
            int w, h;
            _rect_ratio_full(ww, hh, it->w, it->h, &w, &h);
            nemoshow_item_set_width(one, w);
            nemoshow_item_set_height(one, h);
            nemoshow_item_path_load_svg(one, it->path, 0, 0, w, h);
        }
    } else if (type == EXPLORER_ITEM_TYPE_VIDEO) {
        ITEM_CREATE(one, group, NEMOSHOW_IMAGE_ITEM);
        nemoshow_item_set_alpha(one, 0.0);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        it->video = one;
        it->video_timer = TOOL_ADD_TIMER(exp->tool, 0,
                _explorer_item_video_timer, it);
    }

    if (icon_uri) {
        double ww, hh;
        if (!svg_get_wh(icon_uri, &ww, &hh)) {
            ERR("svg get wh failed: %s", icon_uri);
        }
        one = SVG_PATH_GROUP_CREATE(group, ww, hh, icon_uri);
        nemoshow_item_translate(one, 1,1);
        nemoshow_item_set_fill_color(one, RGBA(BLACK));
        nemoshow_item_set_filter(one, blur);
        it->icon_bg = one;

        one = SVG_PATH_GROUP_CREATE(group, ww, hh, icon_uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_set_fill_color(one, RGBA(COLOR_ICON));
        it->icon = one;
    }


    one = RECT_CREATE(group, it->w, it->w/8);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));
    nemoshow_item_translate(one, 0, it->h/2.0 - it->w/8/2.0);
    nemoshow_item_set_alpha(one, 0.0);
    it->txt_bg = one;

    it->txt_str = strdup(txt);

    char buf[25];
    snprintf(buf, 25, "%s", txt);
    one = TEXT_CREATE(group, font, it->w/11, NULL);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_TXT));
    nemoshow_item_translate(one, 0, it->h/2.0 - it->w/8/2.0);
    nemoshow_item_set_alpha(one, 0.0);
    it->txt = one;

    exp->items = list_append(exp->items, it);
    return it;
}

static void explorer_destroy(Explorer *exp)
{
    worker_destroy(exp->img_worker);

    explorer_icon_destroy(exp->prev);
    explorer_icon_destroy(exp->next);

    ExplorerItem *it;
    while((it = LIST_DATA(LIST_FIRST(exp->items)))) {
        explorer_remove_item(exp, it);
    }

    if (exp->curpath) free(exp->curpath);
    if (exp->bgpath) free(exp->bgpath);
    free(exp->rootpath);

    image_destroy(exp->bg);
    nemoshow_one_destroy(exp->icon_group);
    nemoshow_one_destroy(exp->it_group);
    nemowidget_destroy(exp->widget);
    free(exp);
}

static Explorer *explorer_create(NemoWidget *parent, int width, int height, const char *path)
{
    Explorer *exp = calloc(sizeof(Explorer), 1);
    exp->show = nemowidget_get_show(parent);
    exp->tool = nemowidget_get_tool(parent);
    exp->uuid = nemowidget_get_uuid(parent);
    exp->w = width;
    exp->h = height;
    exp->rootpath = strdup(path);
    exp->cnt_inrow = 4;
    exp->cnt_row = 3;
    exp->cnt_inpage = exp->cnt_row * exp->cnt_inrow;
    exp->x_pad = 50;
    exp->y_pad = 100;
    exp->it_gap = 20;
    exp->itw = (width - exp->x_pad * 2)/exp->cnt_inrow;
    exp->ith = (height - exp->y_pad * 2)/exp->cnt_row;

    exp->img_worker = worker_create(exp->tool);

    NemoWidget *widget;
    exp->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _explorer_event, exp);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

    struct showone *group;
    exp->group = group = GROUP_CREATE(canvas);

    Image *img;
    exp->bg = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_translate(img, 0, 0, 0, exp->w/2, exp->h/2);
    image_set_alpha(img, 0, 0, 0, 0.0);
    image_load(img, exp->tool, FB_IMG_DIR"/background.png",
            exp->w, exp->h, NULL, NULL);

#if 0
    double w, h;
    char uri[PATH_MAX];
    snprintf(uri, PATH_MAX, "%s/.title.svg", exp->rootpath);
    if (!svg_get_wh(uri, &w, &h)) {
        ERR("svg get wh failed: %s", uri);
    }
    double hh = exp->y_pad/3;
    double scale = hh/h;
    w *= scale;

    struct showone *one;
    one = SVG_PATH_GROUP_CREATE(group, w, hh, uri);
    nemoshow_item_translate(one, exp->x_pad, 30);
    //nemoshow_item_set_alpha(one, 0.0);
    exp->title = one;
#endif
    /*
    struct showone *one;
    one = PATH_TEXT_CREATE(group, "NanumGothic", 40, NULL);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_translate(one, exp->x_pad, 30);
    nemoshow_item_set_alpha(one, 0.0);
    exp->title = one;
    */

    exp->it_group = GROUP_CREATE(canvas);
    exp->icon_group = GROUP_CREATE(canvas);

    ExplorerIcon *icon;
    exp->prev = icon = explorer_icon_create(exp, EXPLORER_ICON_TYPE_PREV,
            FB_ICON_DIR"/prev.svg");
    explorer_icon_translate(icon, 0, 0, 0,
            exp->x_pad,
            exp->h - exp->y_pad/2);

    exp->next = icon = explorer_icon_create(exp, EXPLORER_ICON_TYPE_NEXT,
            FB_ICON_DIR"/next.svg");
    explorer_icon_translate(icon, 0, 0, 0,
            exp->x_pad + exp->itw * exp->cnt_inrow,
            exp->h - exp->y_pad/2);

    nemowidget_show(widget, 0, 0, 0);

    return exp;
}

static void explorer_show_items(Explorer *exp, uint32_t easetype, int duration, int delay)
{
    worker_stop(exp->img_worker);

    List *l;
    ExplorerItem *it;
    LIST_FOR_EACH(exp->items, l, it) {
        explorer_item_show(it, easetype, duration, delay);
        delay += 50;
    }

    worker_start(exp->img_worker);
}

static void explorer_arrange(Explorer *exp, uint32_t easetype, int duration, int delay)
{
    int row = 0;
    int idx = 0;
    List *l;
    ExplorerItem *it;
    LIST_FOR_EACH(exp->items, l, it) {
        int x, y;
        row = idx/exp->cnt_inrow;
        x = exp->x_pad + exp->itw/2 + exp->itw * (idx % exp->cnt_inrow) + ((row%2) * exp->it_gap);
        y = exp->y_pad + exp->ith/2 + exp->ith * row;
        explorer_item_translate(it, 0, 0, 0, x, y);
        idx++;
    }
}

static void explorer_show_page(Explorer *exp, int page_idx)
{
    if (page_idx >= exp->page_cnt) return;
    if (page_idx < 0) return;

    ERR("%d", page_idx);

    exp->page_idx = page_idx;

    int filecnt = list_count(exp->fileinfos);
    int fileinfoidx = page_idx * exp->cnt_inpage;

    if (page_idx > 0) {
        explorer_icon_show(exp->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250);
    } else {
        explorer_icon_hide(exp->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }

    if (fileinfoidx + exp->cnt_inpage < filecnt) {
        explorer_icon_show(exp->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 400);
    } else {
        explorer_icon_hide(exp->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }

    ExplorerItem *it;
    while((it = LIST_DATA(LIST_FIRST(exp->items)))) {
        explorer_remove_item(exp, it);
    }

    int idx = 0;
    while (idx < exp->cnt_inpage && fileinfoidx < filecnt) {
        ExplorerItemType type;
        const char *icon_uri = NULL;
        const char *name = NULL;
        const char *exec = NULL;
        const char *path = NULL;

        FileInfo *fileinfo =
            LIST_DATA(list_get_nth(exp->fileinfos, fileinfoidx));
        if (!fileinfo) {
            ERR("WTF: fileinfo is NULL");
            fileinfoidx++;
            continue;
        }

        if (fileinfo_is_dir(fileinfo)) {
            type = EXPLORER_ITEM_TYPE_DIR;
            icon_uri = FB_ICON_DIR"/dir.svg";
            exec = NULL;
        } else {
            const FileType *filetype =
                _fileinfo_match_filetype(fileinfo);
            if (!filetype) {
                ERR("Not supported file type: %s", fileinfo->path);
                fileinfoidx++;
                continue;
            }
            if (!strcmp(filetype->type, "image")) {
                type = EXPLORER_ITEM_TYPE_IMG;
            } else if (!strcmp(filetype->type, "svg")) {
                type = EXPLORER_ITEM_TYPE_SVG;
            } else if (!strcmp(filetype->type, "video")) {
                type = EXPLORER_ITEM_TYPE_VIDEO;
            } else {
                type = EXPLORER_ITEM_TYPE_FILE;
            }
            icon_uri = filetype->icon;
            exec = filetype->exec;
        }
        name = fileinfo->name;
        path = fileinfo->path;

        it = explorer_append_item(exp, type, icon_uri, name, exec, path,
                exp->itw, exp->ith);
        idx++;
        fileinfoidx++;
    }

    explorer_arrange(exp, 0, 0, 0);
    explorer_show_items(exp, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    nemoshow_dispatch_frame(exp->show);
}

typedef struct _FileThreadData FileThreadData;
struct _FileThreadData {
    Explorer *exp;
    List *fileinfos;
};

static void *_explorer_load_file(void *userdata)
{
    FileThreadData *data = userdata;
    Explorer *exp = data->exp;

    data->fileinfos = fileinfo_readdir(exp->curpath);

    return NULL;
}

static void _explorer_load_file_done(bool cancel, void *userdata)
{
    FileThreadData *data = userdata;
    Explorer *exp = data->exp;

    exp->filethread = NULL;
    if (cancel) {
        FileInfo *fileinfo;
        LIST_FREE(data->fileinfos, fileinfo) fileinfo_destroy(fileinfo);
    } else {
        FileInfo *fileinfo;
        LIST_FREE(exp->fileinfos, fileinfo) fileinfo_destroy(fileinfo);
        exp->fileinfos = data->fileinfos;
        exp->page_cnt = list_count(exp->fileinfos)/exp->cnt_inpage + 1;
        explorer_show_page(exp, 0);
    }
    free(data);
}

static void explorer_show_dir(Explorer *exp, const char *path)
{
    RET_IF(!path);
    ERR("%s", path);

    if (exp->curpath) free(exp->curpath);
    exp->curpath = strdup(path);

    char *temp = strdup(path);
    char *title = basename(temp);
    struct showone *one;
    ERR("%s", title);
    one = PATH_TEXT_CREATE(exp->group, "NanumGothic", 40, title);
    free(temp);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_translate(one, exp->x_pad, 30);
    nemoshow_item_set_alpha(one, 0.0);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
    if (exp->title[exp->title_idx])
        _nemoshow_item_motion(exp->title[exp->title_idx],
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.0, NULL);

    if (exp->title_idx == 0) exp->title_idx = 1;
    else exp->title_idx = 0;
    exp->title[exp->title_idx] = one;



    image_set_alpha(exp->bg, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);

    if (exp->filethread) thread_destroy(exp->filethread);
    FileThreadData *filedata = calloc(sizeof(FileThreadData), 1);
    filedata->exp = exp;
    exp->filethread = thread_create(exp->tool,
            _explorer_load_file, _explorer_load_file_done, filedata);
    nemoshow_dispatch_frame(exp->show);
}

static void explorer_hide(Explorer *exp)
{
    worker_stop(exp->img_worker);

    image_set_alpha(exp->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.0);

    explorer_icon_hide(exp->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    explorer_icon_hide(exp->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);

    int delay = 0;
    List *l;
    ExplorerItem *it;
    LIST_FOR_EACH(exp->items, l, it) {
        explorer_item_hide(it, NEMOEASE_CUBIC_OUT_TYPE, 250, delay);
        delay += 30;
    }
    nemoshow_dispatch_frame(exp->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Explorer *exp = userdata;

    explorer_hide(exp);

    nemowidget_win_exit_after(win, 500);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *bgpath;
    char *rootpath;
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
        snprintf(buf, PATH_MAX, "%s/filetype", appname);

        List *types = xml_search_tags(xml, buf);
        List *l;
        XmlTag *tag;
        LIST_FOR_EACH(types, l, tag) {
            FileType *filetype = filetype_parse_tag(tag);
            if (!filetype) continue;
            __filetypes = list_append(__filetypes, filetype);
        }

        xml_unload(xml);
    }

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        { NULL }
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "f:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                app->rootpath = strdup(optarg);
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
    if (app->bgpath) free(app->bgpath);
    free(app->rootpath);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->rootpath) {
        app->rootpath = strdup(file_get_homedir());
    }

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);

    Explorer *exp;
    exp = explorer_create(win, app->config->width, app->config->height, app->rootpath);
    nemowidget_append_callback(win, "exit", _win_exit, exp);
    explorer_show_dir(exp, app->rootpath);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    explorer_destroy(exp);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);

    FileType *filetype;
    LIST_FREE(__filetypes, filetype) filetype_destroy(filetype);
    _config_unload(app);

    return 0;
}
