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
#define COLOR_ICON 0x53AED6FF
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

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *bgpath_local;
    char *bgpath;
    char *rootpath;
};

typedef struct _Pos Pos;
struct _Pos
{
    double x, y;
};

#if 0
// 1, 8, 16, ...
const Pos pos[] = {
    // 0
    {0, 0},
    // 1
    {0.5, 0.5}, {0.0, 1.0}, {-0.5, 0.5}, {-1.0, 0.0},
    {-0.5, -0.5}, {0.0, -1.0}, {0.5, -0.5}, {1.0, 0.0},
    // 2
    {1.5, 0.5}, {1.0, 1.0}, {0.5, 1.5}, {0.0, 2.0},
    {-0.5, 1.5}, {-1.0, 1.0}, {-1.5, 0.5}, {-2.0, 0.0},
    {-1.5, -0.5}, {-1.0, -1.0}, {-0.5, -1.5}, {0.0, -2.0},
    {0.5, -1.5}, {1.0, -1.0}, {1.5, -0.5}, {2.0, 0.0}
};
#endif

// 4, 12, 20, ...  (0 * 4 + 4, 2*4 + 4, 4 * 4 + 4, ...)
#define POS_LAYER_CNT 2
const Pos pos[] = {
    // 0
    { 0.5, 0.0},{ 0.0, 0.5},{-0.5, 0.0},{ 0.0,-0.5},
    // 1
    { 1.5, 0.0},{ 1.0, 0.5},{ 0.5, 1.0},
    { 0.0, 1.5},{-0.5, 1.0},{-1.0, 0.5},
    {-1.5, 0.0},{-1.0,-0.5},{-0.5,-1.0},
    { 0.0,-1.5},{ 0.5,-1.0},{ 1.0,-0.5},
    // 2
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

static char *_fb_get_thumb_url(const char *path, const char *ext)
{
    if (!path) return NULL;
    char *base = file_get_basename(path);
    char *dir = file_get_dirname(path);
    char *ret;
    if (ext)  {
        ret = strdup_printf("%s/%s/%s.%s", dir, THUMB_DIR, base, ext);
    } else {
        ret = strdup_printf("%s/%s/%s", dir, THUMB_DIR, base);
    }
    free(base);
    free(dir);
    return ret;
}

typedef struct _Exec Exec;
struct _Exec {
    char *type;
    char *path;
};
static Exec *exec_parse_tag(XmlTag *tag)
{
    List *ll;
    XmlAttr *attr;
    Exec *exec = NULL;

    exec = calloc(sizeof(Exec), 1);
    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                if (exec->type) free(exec->type);
                exec->type = strdup(attr->val);
            }
        }
        if (!strcmp(attr->key, "path")) {
            if (attr->val) {
                if (exec->path) free(exec->path);
                exec->path = strdup(attr->val);
            }
        }
    }
    if (!exec->type) {
        if (exec->path) free(exec->path);
        free(exec);
        ERR("No file type specfied: %s", exec->path);
        return NULL;
    }
    if (!exec->path) {
        if (exec->type) free(exec->type);
        free(exec);
        ERR("No file path specfied: %s", exec->type);
        return NULL;
    }
    return exec;
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

static FileType *filetype_parse_tag(XmlTag *tag, List *execs)
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
    if (!filetype->exec) {
        List *l;
        Exec *exec;
        LIST_FOR_EACH(execs, l, exec) {
            if (!strcmp(filetype->type, exec->type)) {
                filetype->exec = strdup(exec->path);
                break;
            }
        }
    }

    return filetype;
}

typedef enum {
    ICON_TYPE_QUIT = 0,
    ICON_TYPE_UP,
    ICON_TYPE_PREV,
    ICON_TYPE_NEXT,
} FBIconType;

typedef struct _FBView FBView;
typedef struct _FBIcon FBIcon;

struct _FBIcon {
    FBView *view;
    FBIconType type;
    int w, h;

    struct showone *blur;
    struct showone *group;

    struct showone *bg0;
    struct showone *bg;
    struct showone *icon;
};

struct _FBView {
    NemoWidget *win;
    const char *uuid;
    int w, h;
    int itw, ith;
    struct nemoshow *show;
    struct nemotool *tool;
    NemoWidget *widget;

    struct showone *gradient;

    struct nemotimer *bg_timer;
    int bgfiles_idx;
    List *bgfiles;
    Thread *bgdir_thread;
    struct showone *bg_group;
    struct showone *bg_it_clip, *bg_it0_clip;
    double bg_it_clip_x, bg_it_clip_y;

    List *clip_paths;
    bool bgidx;
    Image *bg0, *bg;
    Image *bg_it0, *bg_it;

    struct showone *border;

    struct showone *it_group;
    List *items;

    struct showone *icon_group;
    FBIcon *quit, *up;
    FBIcon *prev, *next;

    char *bgpath_local;
    char *bgpath;

    char *rootpath;
    char *curpath;

    int cnt_inpage;

    int page_idx;
    int page_cnt;

    Thread *filethread;
    List *fileinfos;

    Worker *bg_worker;
    Worker *img_worker;
};

typedef enum {
    ITEM_TYPE_DIR = 0,
    ITEM_TYPE_FILE,
    ITEM_TYPE_IMG,
    ITEM_TYPE_SVG,
    ITEM_TYPE_VIDEO,
} FBItemType;

typedef struct _FBItem FBItem;
struct _FBItem {
    FBView *view;
    FBItemType type;

    int gap;
    int w, h;
    char *exec;
    char *path;

    int video_idx;
    struct nemotimer *anim_timer;

    struct showone *blur;
    struct showone *font;

    struct showone *group;

    struct showone *bg;

    struct showone *icon_bg;
    struct showone *icon;

    struct showone *txt_bg;
    struct showone *txt;

    struct showone *clip;
    Image *img;
    struct showone *svg;
    struct showone *anim;
};

static void fb_icon_destroy(FBIcon *icon)
{
    nemoshow_one_destroy(icon->group);
    free(icon);
}

static FBIcon *fb_icon_create(FBView *view, FBIconType type, const char *uri, int w, int h)
{
    FBIcon *icon = calloc(sizeof(FBIcon), 1);
    icon->view = view;
    icon->type = type;
    icon->w = w;
    icon->h = h;

    struct showone *blur;
    icon->blur = blur = BLUR_CREATE("solid", 15);

    struct showone *group;
    icon->group = group = GROUP_CREATE(view->icon_group);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;

    icon->bg0 = one = path_diamond_create(group, w, h);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);
    nemoshow_item_set_filter(one, blur);

    icon->bg = one = path_diamond_create(group, w, h);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_tag(one, 0x2);
    nemoshow_one_set_userdata(one, icon);
    nemoshow_item_set_fill_color(one, RGBA(0x53AED6FF));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);

    icon->icon = one = SVG_PATH_CREATE(group, w/2, h/2, uri);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_scale(one, 0.0, 0.0);

    return icon;
}

static void fb_icon_show(FBIcon *icon, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(icon->group, easetype, duration, delay,
            "alpha", 1.0, NULL);
    _nemoshow_item_motion_bounce(icon->bg0, easetype, duration, delay,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
    _nemoshow_item_motion_bounce(icon->bg, easetype, duration, delay,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
    _nemoshow_item_motion_bounce(icon->icon, easetype, duration, delay + 150,
            "sx", 1.2, 1.0, "sy", 1.2, 1.0, NULL);
}

static void fb_icon_hide(FBIcon *icon, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(icon->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->bg0, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion(icon->bg, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
    _nemoshow_item_motion(icon->icon, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, NULL);
}

static void fb_icon_translate(FBIcon *icon, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(icon->group, easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(icon->group, tx, ty);
    }
}

static void fb_icon_down(FBIcon *icon)
{
    _nemoshow_item_motion_bounce(icon->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 0.6, 0.7, "sy", 0.6, 0.7,
            NULL);
}

static void fb_icon_up(FBIcon *icon)
{
    _nemoshow_item_motion_bounce(icon->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
}

typedef struct _ClipImgWorkData ClipImgWorkData;
struct _ClipImgWorkData {
    Image *img;
    char *path;
    struct nemoshow *show;
    struct showone *clip;
    int cw, ch;
    int delay;
    double alpha;

    int w, h;
    ImageBitmap *bitmap;
};

static void _clip_img_work_done(bool cancel, void *userdata)
{
    ClipImgWorkData *data = userdata;

    if (!cancel) {
        if (!data->bitmap) {
            ERR("bitmap is not loaded correctly");
        } else {
            image_set_bitmap(data->img, data->w, data->h, data->bitmap);
            // XXX: move clip as center align
            nemoshow_item_path_translate(data->clip,
                    (data->w - data->cw)/2, (data->h - data->ch)/2);
            image_set_alpha(data->img,
                    NEMOEASE_CUBIC_INOUT_TYPE, 500, data->delay,
                    data->alpha);
            nemoshow_dispatch_frame(data->show);
        }
    } else {
        if (data->bitmap) image_bitmap_destroy(data->bitmap);
    }

    free(data->path);
    free(data);
}

static void _clip_img_work(void *userdata)
{
    ClipImgWorkData *data = userdata;

    int w, h;
    if (image_get_wh(data->path, &w, &h)) {
        _rect_ratio_full(w, h, data->cw, data->ch, &(data->w), &(data->h));
        data->bitmap = image_bitmap_create(data->path);
    } else {
        data->w = data->cw;
        data->h = data->ch;
        data->bitmap = NULL;
        ERR("image get width/height failed: %s", data->path);
    }
}

static void _fb_item_anim_timer(struct nemotimer *timer, void *userdata)
{
    FBItem *it = userdata;

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%03d.jpg", it->video_idx);

    char *path = _fb_get_thumb_url(it->path, buf);

    if (!file_is_exist(path) || file_is_null(path)) {
        ERR("No animation thumbnail: %s", path);
        nemotimer_set_timeout(timer, 0);
        return;
    }

    if (nemoshow_item_get_width(it->anim) <= 0 ||
            nemoshow_item_get_height(it->anim) <= 0) {
        int w, h;
        if (!image_get_wh(path, &w, &h)) {
            ERR("image get wh failed: %s", path);
        } else {
            _rect_ratio_full(w, h, it->w, it->h, &w, &h);
            nemoshow_item_set_width(it->anim, w);
            nemoshow_item_set_height(it->anim, h);
            nemoshow_item_set_anchor(it->anim, 0.5, 0.5);
            nemoshow_item_translate(it->anim, it->w/2, it->h/2);
            // XXX: move clip as center align
            nemoshow_item_path_translate(it->clip,
                    -(it->w - w)/2, -(it->h - h)/2);
            _nemoshow_item_motion(it->anim,
                    NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                    "alpha", 1.0,
                    NULL);
        }
    }

    nemoshow_item_set_uri(it->anim, path);
    if (path) free(path);

    nemoshow_dispatch_frame(it->view->show);

    it->video_idx++;
    if (it->video_idx > 120) it->video_idx = 1;
    nemotimer_set_timeout(timer, VIDEO_INTERVAL);
}

static void fb_update_clip(FBView *view);
static void _fb_item_update(NemoMotion *m, uint32_t time, double t, void *userdata)
{
    FBItem *it = userdata;
    fb_update_clip(it->view);
}

static void fb_item_show(FBItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion_bounce_with_callback(it->group,
                _fb_item_update, it,
                easetype, duration, delay,
                "sx", 1.2, 1.0, "sy", 1.2, 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
        nemoshow_item_scale(it->group, 1.0, 1.0);
        fb_update_clip(it->view);
    }

    FBView *view = it->view;
    if (it->type == ITEM_TYPE_IMG) {
        char *path = _fb_get_thumb_url(it->path, NULL);
        if (!file_is_exist(path) || file_is_null(path)) {
            free(path);
            path = strdup(it->path);
        }
        if (!file_is_exist(path) || file_is_null(path)) {
            ERR("file(%s) does not exist or has zero size", path);
        } else {
            ClipImgWorkData *data = calloc(sizeof(ClipImgWorkData), 1);
            data->show = it->view->show;
            data->img = it->img;
            data->path = strdup(path);
            data->clip = it->clip;
            data->cw = it->w;
            data->ch = it->h;
            data->delay = delay;
            data->alpha = 1.0;
            worker_append_work(view->img_worker, _clip_img_work,
                    _clip_img_work_done, data);

        }
    } else if (it->type == ITEM_TYPE_VIDEO) {
        char *path = _fb_get_thumb_url(it->path, "001.jpg");
        if (!file_is_exist(path) || file_is_null(path)) {
            ERR("No animation thumbnail: %s", path);
#if 0
            // Make animtion thumbnail
            path = strdup(it->path);
            VideoWorkData *data = calloc(sizeof(VideoWorkData), 1);
            data->it = it;
            data->path = path;
            worker_append_work(view->img_worker, _fb_item_video_work,
                    _fb_item_video_work_done, data);
#endif
        } else {
            it->video_idx = 1;
            if (it->anim_timer) nemotimer_set_timeout(it->anim_timer, 10 + delay);
        }
        free(path);
    }
}

static void fb_item_hide(FBItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->anim_timer) nemotimer_set_timeout(it->anim_timer, 0);
    if (duration > 0) {
        _nemoshow_item_motion_with_callback(it->group,
                _fb_item_update, it,
                easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
        nemoshow_item_scale(it->group, 0.0, 0.0);
        fb_update_clip(it->view);
    }
}

static void fb_item_down(FBItem *it)
{
    _nemoshow_item_motion_bounce_with_callback(it->group,
            _fb_item_update, it,
            NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 0.6, 0.7, "sy", 0.6, 0.7,
            NULL);
}

static void fb_item_up(FBItem *it)
{
    _nemoshow_item_motion_bounce_with_callback(it->group,
            _fb_item_update, it,
            NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
}

static void fb_item_translate(FBItem *it, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion_with_callback(it->group,
                _fb_item_update, it,
                easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(it->group, tx, ty);
    }
}

static void fb_show_dir(FBView *view, const char *path);
static void fb_show_page(FBView *view, int page_idx);

static void fb_item_exec(FBItem *it)
{
    FBView *view = it->view;
    struct nemoshow *show = nemowidget_get_show(view->widget);

    if (it->type == ITEM_TYPE_DIR) {
        fb_show_dir(view, it->path);
    } else {
        float x, y;
        nemoshow_transform_to_viewport(show,
                NEMOSHOW_ITEM_AT(it->group, tx),
                NEMOSHOW_ITEM_AT(it->group, ty),
                &x, &y);

        nemoshow_view_set_anchor(show, 0.5, 0.5);
        ERR("%s, %s", it->exec, it->path);
        if (!it->exec) {
            nemo_execute(view->uuid, "app", it->path, NULL, NULL, x, y, 0, 1, 1);
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
            nemo_execute(view->uuid, "app", path, args, NULL, x, y, 0, 1, 1);
        }
    }
}

static struct showone *fb_item_get_path_clip(FBItem *it)
{
    double w, h;
    w = it->w * NEMOSHOW_ITEM_AT(it->group, sx);
    h = it->h * NEMOSHOW_ITEM_AT(it->group, sy);
    //w -= 6;
    //h -= 6;
    if (w <= 21 || h <= 21) return NULL;

    struct showone *one;
    one = path_diamond_create(NULL, w, h);
    nemoshow_item_path_translate(one,
            NEMOSHOW_ITEM_AT(it->group, tx) - w/2.0,
            NEMOSHOW_ITEM_AT(it->group, ty) - h/2.0);
    return one;
}

static void fb_update_clip(FBView *view)
{
    if (!view->bg_it0_clip && !view->bg_it_clip) return;

    if (view->bg_it0_clip) nemoshow_item_path_clear(view->bg_it0_clip);
    if (view->bg_it_clip) nemoshow_item_path_clear(view->bg_it_clip);

    struct showone *path;
    LIST_FREE(view->clip_paths, path) {
        nemoshow_one_destroy(path);
    }

    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        if (it->type != ITEM_TYPE_DIR &&
                it->type != ITEM_TYPE_FILE)
            continue;
        path = fb_item_get_path_clip(it);
        if (path) {
            // XXX: cannot translate parent clip, so translsate each one.
            // maybe skia bug...
            nemoshow_item_path_translate(path, view->bg_it_clip_x, view->bg_it_clip_y);
            if (view->bg_it0_clip) nemoshow_item_path_append(view->bg_it0_clip, path);
            if (view->bg_it_clip) nemoshow_item_path_append(view->bg_it_clip, path);
            view->clip_paths = list_append(view->clip_paths, path);
        }
    }
}

static void _fb_grab_item_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    FBItem *it = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        fb_item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        fb_item_up(it);
        if (nemoshow_event_is_single_click(show, event)) {
            fb_item_exec(it);
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _fb_grab_icon_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    FBIcon *icon = userdata;
    FBView *view = icon->view;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        fb_icon_down(icon);
    } else if (nemoshow_event_is_up(show, event)) {
        fb_icon_up(icon);
        if (nemoshow_event_is_single_click(show, event)) {
            if (icon->type == ICON_TYPE_QUIT) {
                NemoWidget *win = nemowidget_get_top_widget(widget);
                nemowidget_callback_dispatch(win, "exit", NULL);
            } else if (icon->type == ICON_TYPE_UP) {
                char *uppath = file_get_updir(view->curpath);
                ERR("%s %s", view->curpath, uppath);
                fb_show_dir(view, uppath);
                free(uppath);
            } else if (icon->type == ICON_TYPE_PREV) {
                fb_show_page(view, view->page_idx-1);
            } else if (icon->type == ICON_TYPE_NEXT) {
                fb_show_page(view, view->page_idx+1);
            }
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void _fb_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    FBView *view = userdata;
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
                    FBItem *it = nemoshow_one_get_userdata(one);
                    fb_item_up(it);
                } else if (nemoshow_one_get_tag(one) == 0x2) {
                    FBIcon *icon = nemoshow_one_get_userdata(one);
                    fb_icon_up(icon);
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
        if (one) {
            if (nemoshow_one_get_tag(one) == 0x1) {
                FBItem *it = nemoshow_one_get_userdata(one);
                if (it) {
                    NemoWidgetGrab *grab =
                        nemowidget_create_grab(widget, event,
                                _fb_grab_item_event, it);
                    nemowidget_grab_set_data(grab, "one", one);
                }
            } else if (nemoshow_one_get_tag(one) == 0x2) {
                FBIcon *icon = nemoshow_one_get_userdata(one);
                if (icon) {
                    NemoWidgetGrab *grab =
                        nemowidget_create_grab(widget, event,
                                _fb_grab_icon_event, icon);
                    nemowidget_grab_set_data(grab, "one", one);
                }
            }
        } else {
            struct nemotool *tool = nemowidget_get_tool(widget);
            float vx, vy;
            nemoshow_transform_to_viewport(show,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event), &vx, &vy);
            nemotool_touch_bypass(tool,
                    nemoshow_event_get_device(event),
                    vx, vy);
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static void fb_remove_item(FBView *view, FBItem *it)
{
    view->items = list_remove(view->items, it);

    if (it->anim_timer) nemotimer_destroy(it->anim_timer);

    if (it->clip) nemoshow_one_destroy(it->clip);
    if (it->img) image_destroy(it->img);
    nemoshow_one_destroy(it->group);
    nemoshow_one_destroy(it->font);
    nemoshow_one_destroy(it->blur);

    if (it->exec) free(it->exec);
    if (it->path) free(it->path);

    free(it);
}

static FBItem *fb_append_item(FBView *view, FBItemType type, const char *icon_uri, const char *txt, const char *exec, const char *path, int x, int y, int w, int h)
{
    FBItem *it = calloc(sizeof(FBItem), 1);
    it->view = view;
    it->type = type;
    it->gap = 12;
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
    group = GROUP_CREATE(view->it_group);
    nemoshow_item_set_width(group, w);
    nemoshow_item_set_height(group, h);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_translate(group, x, y);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);
    it->group = group;

    struct showone *one;
    one = path_diamond_create(group, it->w, it->h);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_one_set_tag(one, 0x1);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_fill_color(one, RGBA(LIGHTBLUE));
    nemoshow_item_set_shader(one, view->gradient);
    it->bg = one;

    if (icon_uri) {
        one = SVG_PATH_CREATE(group, it->w, it->h, icon_uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2 + 1, h/2 + 1);
        nemoshow_item_set_fill_color(one, RGBA(WHITE));
        nemoshow_item_set_filter(one, blur);
        it->icon_bg = one;

        one = SVG_PATH_CREATE(group, it->w, it->h, icon_uri);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(COLOR_ICON));
        it->icon = one;
    }

    struct showone *clip;
    clip = path_diamond_create(NULL, it->w, it->h);
    nemoshow_item_path_translate(clip, it->gap/2, it->gap/2);

    it->clip = clip;
    if (type == ITEM_TYPE_IMG) {
        it->img = image_create(group);
        image_set_anchor(it->img, 0.5, 0.5);
        image_translate(it->img, 0, 0, 0, it->w/2, it->h/2);
        image_set_alpha(it->img, 0, 0, 0, 0.0);
        // XXX: clip should not be controlled by image
        nemoshow_item_set_clip(image_get_one(it->img), clip);
    } else if (type == ITEM_TYPE_SVG) {
        // FIXME: Need thread???
        ITEM_CREATE(one, group, NEMOSHOW_PATHGROUP_ITEM);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, it->w/2, it->h/2);
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
            nemoshow_item_set_clip(one, clip);
            // XXX: move clip as center align
            nemoshow_item_path_translate(clip,
                    -(it->w - w)/2, -(it->h - h)/2);
        }
    } else if (type == ITEM_TYPE_VIDEO) {
        ITEM_CREATE(one, group, NEMOSHOW_IMAGE_ITEM);
        nemoshow_item_set_alpha(one, 0.0);
        nemoshow_item_set_clip(one, clip);
        it->anim = one;
        it->anim_timer = TOOL_ADD_TIMER(view->tool, 0,
                _fb_item_anim_timer, it);
    }

    char buf[16];
    snprintf(buf, 16, "%s", txt);

    one = TEXT_CREATE(group, font, it->w/11, buf);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2 + 1, h/2 + h/8 + 1);
    nemoshow_item_set_filter(one, blur);
    it->txt_bg = one;

    one = TEXT_CREATE(group, font, it->w/11, buf);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_TXT));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2 + h/8);
    it->txt = one;

    one = path_diamond_create(group, it->w, it->h);
    nemoshow_item_set_stroke_color(one, RGBA(WHITE));
    nemoshow_item_set_stroke_width(one, 1);
    nemoshow_item_translate(one, (w - it->w)/2, (h - it->h)/2);
    nemoshow_item_set_shader(one, view->gradient);

    view->items = list_append(view->items, it);
    return it;
}

static void fb_view_destroy(FBView *view)
{
    worker_destroy(view->bg_worker);
    worker_destroy(view->img_worker);
    nemotimer_destroy(view->bg_timer);

    FileInfo *file;
    LIST_FREE(view->bgfiles, file)
        fileinfo_destroy(file);

    struct showone *path;
    LIST_FREE(view->clip_paths, path)
        nemoshow_one_destroy(path);
    if (view->bg_it0_clip) nemoshow_one_destroy(view->bg_it0_clip);
    if (view->bg_it_clip) nemoshow_one_destroy(view->bg_it_clip);

    fb_icon_destroy(view->quit);
    fb_icon_destroy(view->up);
    fb_icon_destroy(view->prev);
    fb_icon_destroy(view->next);

    FBItem *it;
    while((it = LIST_DATA(LIST_FIRST(view->items)))) {
        fb_remove_item(view, it);
    }

    if (view->curpath) free(view->curpath);
    if (view->bgpath) free(view->bgpath);
    if (view->bgpath_local) free(view->bgpath_local);
    free(view->rootpath);

    image_destroy(view->bg0);
    image_destroy(view->bg);
    image_destroy(view->bg_it0);
    image_destroy(view->bg_it);
    nemoshow_one_destroy(view->icon_group);
    nemoshow_one_destroy(view->it_group);
    nemoshow_one_destroy(view->bg_group);
    nemowidget_destroy(view->widget);
    free(view);
}

static void _fb_bg_timer(struct nemotimer *timer, void *userdata)
{
    FBView *view = userdata;

    FileInfo *file = LIST_DATA(list_get_nth(view->bgfiles, view->bgfiles_idx));

    view->bgidx = !view->bgidx;

    int w, h;
    file_get_image_wh(file->path, &w, &h);

    if (view->bgidx) {
        image_load_fit(view->bg, view->tool, file->path, view->w, view->h, NULL, NULL);
        image_set_alpha(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0, 1.0);
        image_translate(view->bg, 0, 0, 0, w/2, h/2);
        image_set_alpha(view->bg0, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0, 0.0);
    } else {
        image_load_fit(view->bg0, view->tool, file->path, view->w, view->h, NULL, NULL);
        image_set_alpha(view->bg0, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0, 1.0);
        image_translate(view->bg0, 0, 0, 0, w/2, h/2);
        image_set_alpha(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0, 0.0);
    }

    view->bgfiles_idx++;
    if (view->bgfiles_idx >= list_count(view->bgfiles))
        view->bgfiles_idx = 0;

    if (list_count(view->bgfiles) > 1)
        nemotimer_set_timeout(timer, BACK_INTERVAL);

    nemoshow_dispatch_frame(view->show);
}

static FBView *fb_view_create(NemoWidget *parent, int width, int height, const char *path, ConfigApp *app)
{
    FBView *view = calloc(sizeof(FBView), 1);
    view->win = parent;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->w = width;
    view->h = height;
    view->itw = width/(POS_LAYER_CNT * 2) - 12;
    view->ith = height/(POS_LAYER_CNT * 2) - 12;
    view->rootpath = strdup(path);
    view->cnt_inpage = sizeof(pos)/sizeof(pos[0]);
    if (app->bgpath_local) view->bgpath_local = strdup(app->bgpath_local);
    if (app->bgpath) view->bgpath = strdup(app->bgpath);

    /*
    view->bg_worker = worker_create(view->tool);
    view->img_worker = worker_create(view->tool);
    */

    NemoWidget *widget;
    widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _fb_event, view);
    view->widget = widget;

    struct showone *one;
    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

    struct showone *group;
    view->bg_group = group = GROUP_CREATE(canvas);
    nemoshow_item_set_alpha(group, 0.0);

    one = RECT_CREATE(group, view->w, view->h);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

    Image *img;
    view->bg = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_set_alpha(img, 0, 0, 0, 0.0);
    view->bg0 = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_set_alpha(img, 0, 0, 0, 0.0);

    view->bg_timer = TOOL_ADD_TIMER(view->tool, 0, _fb_bg_timer, view);

    view->it_group = GROUP_CREATE(canvas);

    return view;
}

static void fb_show_items(FBView *view, uint32_t easetype, int duration, int delay)
{
    worker_stop(view->img_worker);

    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        fb_item_show(it, easetype, duration, delay);
        delay += 50;
    }

    worker_start(view->img_worker);
}

static void fb_arrange(FBView *view, uint32_t easetype, int duration, int delay)
{
    int x, y, w, h;
    x = view->w/2.0;
    y = view->h/2.0;
    w = view->itw;
    h = view->ith;

    int idx = 0;
    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        fb_item_translate(it, 0, 0, 0,
                x + w * pos[idx].x,
                y + h * pos[idx].y);
        idx++;
    }
}

static void fb_show_page(FBView *view, int page_idx)
{
    if (page_idx >= view->page_cnt) return;
    if (page_idx < 0) return;

    ERR("%d", page_idx);

    view->page_idx = page_idx;

    int filecnt = list_count(view->fileinfos);
    int fileinfoidx = page_idx * view->cnt_inpage;

    if (page_idx > 0) {
        fb_icon_show(view->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250);
    } else {
        fb_icon_hide(view->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }

    if (fileinfoidx + view->cnt_inpage < filecnt) {
        fb_icon_show(view->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 400);
    } else {
        fb_icon_hide(view->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }

    FBItem *it;
    while((it = LIST_DATA(LIST_FIRST(view->items)))) {
        fb_remove_item(view, it);
    }

    int idx = 0;
    while (idx < view->cnt_inpage && fileinfoidx < filecnt) {
        FBItemType type;
        const char *icon_uri = NULL;
        const char *name = NULL;
        const char *exec = NULL;
        const char *path = NULL;

        FileInfo *fileinfo =
            LIST_DATA(list_get_nth(view->fileinfos, fileinfoidx));
        if (!fileinfo) {
            ERR("WTF: fileinfo is NULL");
            fileinfoidx++;
            continue;
        }

        if (fileinfo_is_dir(fileinfo)) {
            type = ITEM_TYPE_DIR;
            icon_uri = APP_RES_DIR"/dir.svg";
            exec = NULL;
        } else {
            const FileType *filetype =
                _fileinfo_match_filetype(fileinfo);
            if (!filetype) {
                ERR("Not supported file: %s", fileinfo->path);
                fileinfoidx++;
                continue;
            }
            if (!strcmp(filetype->type, "image")) {
                type = ITEM_TYPE_IMG;
            } else if (!strcmp(filetype->type, "svg")) {
                type = ITEM_TYPE_SVG;
            } else if (!strcmp(filetype->type, "video")) {
                type = ITEM_TYPE_VIDEO;
            } else {
                type = ITEM_TYPE_FILE;
            }
            icon_uri = filetype->icon;
            exec = filetype->exec;
        }
        name = fileinfo->name;
        path = fileinfo->path;

        it = fb_append_item(view, type, icon_uri, name, exec, path,
                0, 0, view->itw, view->ith);
        idx++;
        fileinfoidx++;
    }
    if (list_count(view->items) < 5) {
        int vals[10];
        vals[0] = view->w/2;
        vals[1] = view->h * 0.25;
        vals[2] = view->w * 0.75;
        vals[3] = view->h/2;
        vals[4] = view->w/2;
        vals[5] = view->h * 0.75;
        vals[6] = view->w * 0.25;
        vals[7] = view->h/2;
        vals[8] = view->w/2;
        vals[9] = view->h * 0.25;
        nemowidget_detach_scope(view->widget);
        nemowidget_attach_scope_polygon(view->widget, 10, vals);

        fb_icon_translate(view->prev,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                (view->itw/3) + (view->w/2) * 0.5, view->h/2);
        fb_icon_translate(view->next,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                view->w - view->itw/3 - (view->w/2) * 0.5, view->h/2);
        _nemoshow_item_motion(view->border, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 0.5, "sy", 0.5, NULL);
        image_scale(view->bg0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 0.5, 0.5);
        /*
        image_translate(view->bg0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150,
                view->w/4.0, view->h/4.0);
                */
        image_scale(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 0.5, 0.5);
        /*
        image_translate(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150,
                view->w/4.0, view->h/4.0);
                */
    } else {
        int vals[10];
        vals[0] = view->w/2;
        vals[1] = 0;
        vals[2] = view->w;
        vals[3] = view->h/2;
        vals[4] = view->w/2;
        vals[5] = view->h;
        vals[6] = 0;
        vals[7] = view->h/2;
        vals[8] = view->w/2;
        vals[9] = 0;
        nemowidget_detach_scope(view->widget);
        nemowidget_attach_scope_polygon(view->widget, 8, vals);

        fb_icon_translate(view->prev,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                view->itw/3, view->h/2);
        fb_icon_translate(view->next,
                NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                view->w - view->itw/3, view->h/2);
        _nemoshow_item_motion(view->border, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, NULL);
        image_scale(view->bg0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 1.0, 1.0);
        //image_translate(view->bg0, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 0.0, 0.0);
        image_scale(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 1.0, 1.0);
        //image_translate(view->bg, NEMOEASE_CUBIC_INOUT_TYPE, 500, 150, 0.0, 0.0);
    }

    fb_arrange(view, 0, 0, 0);
    fb_show_items(view, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    nemoshow_dispatch_frame(view->show);
}

typedef struct _BackThreadData BackThreadData;
struct _BackThreadData {
    FBView *view;
    int w, h;
    List *bgfiles;
};

static void _load_bg_dir_done(bool cancel, void *userdata)
{
    BackThreadData *data = userdata;
    FBView *view = data->view;
    view->bgdir_thread = NULL;

    if (!cancel) {
        if (data->bgfiles) {
            FileInfo *file;
            LIST_FREE(view->bgfiles, file)
                fileinfo_destroy(file);
            view->bgfiles = data->bgfiles;
            view->bgfiles_idx = 0;
            nemotimer_set_timeout(view->bg_timer, 10);
        }
    } else {
        FileInfo *file;
        LIST_FREE(data->bgfiles, file) {
            fileinfo_destroy(file);
        }
    }
    free(data);
}

static void *_load_bg_dir(void *userdata)
{
    BackThreadData *data = userdata;
    FBView *view = data->view;

    char localpath[PATH_MAX];
    List *bgfiles = NULL;
    if (view->bgpath_local) {
        snprintf(localpath, PATH_MAX, "%s/%s",
                view->curpath, view->bgpath_local);
        if (file_is_exist(localpath)) {
            if (file_is_dir(localpath)) {
                bgfiles = fileinfo_readdir(localpath);
            } else {
                if (file_is_image(localpath)) {
                    FileInfo *file;
                    file = fileinfo_create
                        (false, realpath(localpath, NULL), basename(localpath));
                    bgfiles = list_append(bgfiles, file);
                }
            }
        }
    }

    if (!bgfiles) {
        if (view->bgpath) {
            bgfiles = fileinfo_readdir(view->bgpath);
            if (file_is_image(view->bgpath)) {
                FileInfo *file;
                file = fileinfo_create
                    (false, realpath(view->bgpath, NULL), basename(view->bgpath));
                bgfiles = list_append(bgfiles, file);
            }
        }
    }

    if (!bgfiles) {
        ERR("There is no background image path in local(%s) or global(%s)",
                localpath, view->bgpath);
    }
    FileInfo *file;
    LIST_FREE(bgfiles, file) {
        if (fileinfo_is_image(file)) {
            data->bgfiles = list_append(data->bgfiles, file);
        }
    }
    if (!data->bgfiles) {
        ERR("No images in the background dir");
    }

    return NULL;
}

typedef struct _FileThreadData FileThreadData;
struct _FileThreadData {
    FBView *view;
    List *fileinfos;
};

static void *_fb_load_file(void *userdata)
{
    FileThreadData *data = userdata;
    FBView *view = data->view;

    data->fileinfos = fileinfo_readdir(view->curpath);

    return NULL;
}

static void _fb_load_file_done(bool cancel, void *userdata)
{
    FileThreadData *data = userdata;
    FBView *view = data->view;

    view->filethread = NULL;
    if (cancel) {
        FileInfo *fileinfo;
        LIST_FREE(data->fileinfos, fileinfo) fileinfo_destroy(fileinfo);
    } else {
        FileInfo *fileinfo;
        LIST_FREE(view->fileinfos, fileinfo) fileinfo_destroy(fileinfo);
        view->fileinfos = data->fileinfos;
        int cnt = list_count(view->fileinfos);
        if (cnt >= 16) {
            nemowidget_resize(view->win, 745, 530);
        } else if (cnt >= 9) {
            nemowidget_resize(view->win, 745, 435);
        } else {
            nemowidget_resize(view->win, 745, 340);
        }

        double w, h;
        nemowidget_get_geometry(view->win, NULL, NULL, &w, &h);
        ERR("%d(%lf, %lf)", cnt, w, h);
        _nemoshow_item_motion(view->bg_group, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 1.0, NULL);

        if (view->bgdir_thread) thread_destroy(view->bgdir_thread);
        BackThreadData *bgdata = calloc(sizeof(BackThreadData), 1);
        bgdata->view = view;
        view->bgdir_thread = thread_create(view->tool,
                _load_bg_dir, _load_bg_dir_done, bgdata);
        //fb_show_page(view, 0);
    }
    free(data);
}

static void fb_show_dir(FBView *view, const char *path)
{
    RET_IF(!path);
    ERR("%s", path);

    nemowidget_show(view->widget, 0, 0, 0);

    if (view->curpath) free(view->curpath);
    view->curpath = strdup(path);

    if (view->filethread) thread_destroy(view->filethread);
    FileThreadData *filedata = calloc(sizeof(FileThreadData), 1);
    filedata->view = view;
    view->filethread = thread_create(view->tool,
            _fb_load_file, _fb_load_file_done, filedata);

    nemoshow_dispatch_frame(view->show);
}

static void fb_hide(FBView *view)
{
    worker_stop(view->bg_worker);
    worker_stop(view->img_worker);

    if (view->bgdir_thread) thread_destroy(view->bgdir_thread);
    nemotimer_set_timeout(view->bg_timer, 0);

    _nemoshow_item_motion(view->border, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    image_set_alpha(view->bg0, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    image_set_alpha(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    image_set_alpha(view->bg_it0, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    image_set_alpha(view->bg_it, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);

    fb_icon_hide(view->prev, NEMOEASE_CUBIC_OUT_TYPE, 250, 0);
    fb_icon_hide(view->quit, NEMOEASE_CUBIC_OUT_TYPE, 250, 100);
    fb_icon_hide(view->up, NEMOEASE_CUBIC_OUT_TYPE, 250, 100);
    fb_icon_hide(view->next, NEMOEASE_CUBIC_OUT_TYPE, 250, 200);

    int delay = 0;
    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        fb_item_hide(it, NEMOEASE_CUBIC_OUT_TYPE, 150, delay);
        delay += 30;
    }
    nemoshow_dispatch_frame(view->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    FBView *view = userdata;

    fb_hide(view);

    nemowidget_win_exit_after(win, 500);
}

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        const char *root= "config";
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/background", root);
        temp = xml_get_value(xml, buf, "localpath");
        if (temp && strlen(temp) > 0) {
            app->bgpath_local = strdup(temp);
        }
        temp = xml_get_value(xml, buf, "path");
        if (temp && strlen(temp) > 0) {
            app->bgpath = strdup(temp);
        } else {
            ERR("No background path in configuration file");
        }

        List *execs = NULL;
        snprintf(buf, PATH_MAX, "%s/exec", root);
        List *_execs = xml_search_tags(xml, buf);
        List *l;
        XmlTag *tag;
        LIST_FOR_EACH(_execs, l, tag) {
            Exec *exec = exec_parse_tag(tag);
            if (!exec) continue;
            execs = list_append(execs, exec);
        }

        snprintf(buf, PATH_MAX, "%s/filetype", root);
        List *types = xml_search_tags(xml, buf);
        LIST_FOR_EACH(types, l, tag) {
            FileType *filetype = filetype_parse_tag(tag, execs);
            if (!filetype) continue;
            __filetypes = list_append(__filetypes, filetype);
        }

        Exec *exec;
        LIST_FREE(execs, exec) {
            free(exec->type);
            free(exec->path);
            free(exec);
        }

        xml_unload(xml);
    }

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        {"background", required_argument, NULL, 'b'},
        { NULL }
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "f:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                app->rootpath = strdup(optarg);
                break;
            case 'b':
                if (app->bgpath) free(app->bgpath);
                app->bgpath = strdup(optarg);
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
    if (app->bgpath_local) free(app->bgpath_local);
    if (app->bgpath) free(app->bgpath);
    free(app->rootpath);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->rootpath) {
        app->rootpath = strdup(file_get_homedir());
    }

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);

    FBView *view;
    view = fb_view_create(win, app->config->width, app->config->height, app->rootpath, app);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    fb_show_dir(view, app->rootpath);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    fb_view_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);

    FileType *filetype;
    LIST_FREE(__filetypes, filetype) filetype_destroy(filetype);
    _config_unload(app);

    return 0;
}
