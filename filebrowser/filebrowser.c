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

#define MAX_FILE_CNT 20

static char *_parse_url(const char *str)
{
    RET_IF(!str, NULL);

    char *begin = strstr(str, "(URL=<");
    RET_IF(!begin, NULL);
    begin+=5;

    char *end = strstr(str, ">)");
    RET_IF(!end, NULL);
    end-=1;

    RET_IF(begin == end, NULL);
    int cnt = end - begin + 1;

    return strndup(begin, cnt);
}

typedef struct _Pos Pos;
struct _Pos
{
    int x, y, wh;
};

static Pos pos_new(int x, int y, int wh)
{
    // x, y, wh is index
    Pos pos;
    pos.x = x;
    pos.y = y;
    pos.wh = wh;
    return pos;
}

static void pos_init(Pos *pos[])
{
    int i;
    for (i = 0 ; i < MAX_FILE_CNT ; i++) {
        pos[i] = calloc(sizeof(Pos), i+1);
    }

    // 1
    pos[0][0] = pos_new(0, 0, 3);

    pos[1][0] = pos_new(0, 0, 3);
    pos[1][1] = pos_new(2, 0, 3);

    pos[2][0] = pos_new(0, 0, 3);
    pos[2][1] = pos_new(2, 0, 3);
    pos[2][2] = pos_new(4, 0, 3);

    pos[3][0] = pos_new(0, 0, 3);
    pos[3][1] = pos_new(2, 0, 2);
    pos[3][2] = pos_new(3, 0, 3);
    pos[3][3] = pos_new(5, 0, 3);

    pos[4][0] = pos_new(0, 0, 3);
    pos[4][1] = pos_new(2, 0, 2);
    pos[4][2] = pos_new(3, 0, 3);
    pos[4][3] = pos_new(5, 0, 1);
    pos[4][4] = pos_new(5, 1, 1);

    pos[5][0] = pos_new(0, 0, 3);
    pos[5][1] = pos_new(2, 0, 2);
    pos[5][2] = pos_new(3, 0, 3);
    pos[5][3] = pos_new(5, 0, 1);
    pos[5][4] = pos_new(5, 1, 0);
    pos[5][5] = pos_new(6, 1, 0);

    pos[6][0] = pos_new(0, 0, 3);
    pos[6][1] = pos_new(2, 0, 0);
    pos[6][2] = pos_new(2, 1, 0);
    pos[6][3] = pos_new(3, 0, 3);
    pos[6][4] = pos_new(5, 0, 1);
    pos[6][5] = pos_new(5, 1, 0);
    pos[6][6] = pos_new(6, 1, 0);

    pos[7][0] = pos_new(0, 0, 3);
    pos[7][1] = pos_new(2, 0, 0);
    pos[7][2] = pos_new(2, 1, 0);
    pos[7][3] = pos_new(3, 0, 3);
    pos[7][4] = pos_new(5, 0, 0);
    pos[7][5] = pos_new(6, 0, 0);
    pos[7][6] = pos_new(5, 1, 0);
    pos[7][7] = pos_new(6, 1, 0);

    // 9
    pos[8][0] = pos_new(0, 0, 3);
    pos[8][1] = pos_new(0, 2, 0);
    pos[8][2] = pos_new(1, 2, 0);
    pos[8][3] = pos_new(2, 0, 0);
    pos[8][4] = pos_new(2, 1, 2);
    pos[8][5] = pos_new(3, 0, 3);
    pos[8][6] = pos_new(3, 2, 1);
    pos[8][7] = pos_new(5, 0, 1);
    pos[8][8] = pos_new(5, 1, 3);

    pos[9][0] = pos_new(0, 0, 3);
    pos[9][1] = pos_new(0, 2, 0);
    pos[9][2] = pos_new(1, 2, 0);
    pos[9][3] = pos_new(2, 0, 0);
    pos[9][4] = pos_new(2, 1, 2);
    pos[9][5] = pos_new(3, 0, 3);
    pos[9][6] = pos_new(3, 2, 1);
    pos[9][7] = pos_new(5, 0, 0);
    pos[9][8] = pos_new(6, 0, 0);
    pos[9][9] = pos_new(5, 1, 3);

    pos[10][0]  = pos_new(0, 0, 3);
    pos[10][1]  = pos_new(0, 2, 0);
    pos[10][2]  = pos_new(1, 2, 0);
    pos[10][3]  = pos_new(2, 0, 0);
    pos[10][4]  = pos_new(2, 1, 0);
    pos[10][5]  = pos_new(2, 2, 0);
    pos[10][6]  = pos_new(3, 0, 3);
    pos[10][7]  = pos_new(3, 2, 1);
    pos[10][8]  = pos_new(5, 0, 0);
    pos[10][9]  = pos_new(6, 0, 0);
    pos[10][10] = pos_new(5, 1, 3);

    pos[11][0]  = pos_new(0, 0, 3);
    pos[11][1]  = pos_new(0, 2, 0);
    pos[11][2]  = pos_new(1, 2, 0);
    pos[11][3]  = pos_new(2, 0, 0);
    pos[11][4]  = pos_new(2, 1, 0);
    pos[11][5]  = pos_new(2, 2, 0);
    pos[11][6]  = pos_new(3, 0, 2);
    pos[11][7]  = pos_new(3, 2, 1);
    pos[11][8]  = pos_new(4, 0, 2);
    pos[11][9]  = pos_new(5, 0, 0);
    pos[11][10] = pos_new(6, 0, 0);
    pos[11][11] = pos_new(5, 1, 3);

    pos[12][0]  = pos_new(0, 0, 3);
    pos[12][1]  = pos_new(0, 2, 0);
    pos[12][2]  = pos_new(1, 2, 0);
    pos[12][3]  = pos_new(2, 0, 0);
    pos[12][4]  = pos_new(2, 1, 0);
    pos[12][5]  = pos_new(2, 2, 0);
    pos[12][6]  = pos_new(3, 0, 0);
    pos[12][7]  = pos_new(3, 1, 0);
    pos[12][8]  = pos_new(3, 2, 1);
    pos[12][9]  = pos_new(4, 0, 2);
    pos[12][10] = pos_new(5, 0, 0);
    pos[12][11] = pos_new(6, 0, 0);
    pos[12][12] = pos_new(5, 1, 3);

    pos[13][0]  = pos_new(0, 0, 3);
    pos[13][1]  = pos_new(0, 2, 0);
    pos[13][2]  = pos_new(1, 2, 0);
    pos[13][3]  = pos_new(2, 0, 0);
    pos[13][4]  = pos_new(2, 1, 0);
    pos[13][5]  = pos_new(2, 2, 0);
    pos[13][6]  = pos_new(3, 0, 0);
    pos[13][7]  = pos_new(3, 1, 0);
    pos[13][8]  = pos_new(3, 2, 1);
    pos[13][9]  = pos_new(4, 0, 0);
    pos[13][10] = pos_new(4, 1, 0);
    pos[13][11] = pos_new(5, 0, 0);
    pos[13][12] = pos_new(6, 0, 0);
    pos[13][13] = pos_new(5, 1, 3);

    pos[14][0]  = pos_new(0, 0, 3);
    pos[14][1]  = pos_new(0, 2, 0);
    pos[14][2]  = pos_new(1, 2, 0);
    pos[14][3]  = pos_new(2, 0, 0);
    pos[14][4]  = pos_new(2, 1, 0);
    pos[14][5]  = pos_new(2, 2, 0);
    pos[14][6]  = pos_new(3, 0, 0);
    pos[14][7]  = pos_new(3, 1, 0);
    pos[14][8]  = pos_new(3, 2, 0);
    pos[14][9]  = pos_new(4, 0, 0);
    pos[14][10] = pos_new(4, 1, 0);
    pos[14][11] = pos_new(4, 2, 0);
    pos[14][12] = pos_new(5, 0, 0);
    pos[14][13] = pos_new(6, 0, 0);
    pos[14][14] = pos_new(5, 1, 3);

    // 16
    pos[15][0]  = pos_new(0, 0, 3);
    pos[15][1]  = pos_new(0, 2, 0);
    pos[15][2]  = pos_new(1, 2, 0);
    pos[15][3]  = pos_new(0, 3, 1);
    pos[15][4]  = pos_new(2, 0, 0);
    pos[15][5]  = pos_new(2, 1, 0);
    pos[15][6]  = pos_new(2, 2, 0);
    pos[15][7]  = pos_new(2, 3, 0);
    pos[15][8]  = pos_new(3, 0, 2);
    pos[15][9]  = pos_new(3, 2, 3);
    pos[15][10] = pos_new(4, 0, 0);
    pos[15][11] = pos_new(4, 1, 0);
    pos[15][12] = pos_new(5, 0, 1);
    pos[15][13] = pos_new(5, 1, 3);
    pos[15][14] = pos_new(5, 3, 0);
    pos[15][15] = pos_new(6, 3, 0);

    pos[16][0]  = pos_new(0, 0, 3);
    pos[16][1]  = pos_new(0, 2, 0);
    pos[16][2]  = pos_new(1, 2, 0);
    pos[16][3]  = pos_new(0, 3, 1);
    pos[16][4]  = pos_new(2, 0, 0);
    pos[16][5]  = pos_new(2, 1, 0);
    pos[16][6]  = pos_new(2, 2, 0);
    pos[16][7]  = pos_new(2, 3, 0);
    pos[16][8]  = pos_new(3, 0, 2);
    pos[16][9]  = pos_new(3, 2, 3);
    pos[16][10] = pos_new(4, 0, 0);
    pos[16][11] = pos_new(4, 1, 0);
    pos[16][12] = pos_new(5, 0, 0);
    pos[16][13] = pos_new(6, 0, 0);
    pos[16][14] = pos_new(5, 1, 3);
    pos[16][15] = pos_new(5, 3, 0);
    pos[16][16] = pos_new(6, 3, 0);

    pos[17][0]  = pos_new(0, 0, 3);
    pos[17][1]  = pos_new(0, 2, 0);
    pos[17][2]  = pos_new(1, 2, 0);
    pos[17][3]  = pos_new(0, 3, 1);
    pos[17][4]  = pos_new(2, 0, 0);
    pos[17][5]  = pos_new(2, 1, 0);
    pos[17][6]  = pos_new(2, 2, 0);
    pos[17][7]  = pos_new(2, 3, 0);
    pos[17][8]  = pos_new(3, 0, 2);
    pos[17][9]  = pos_new(3, 2, 1);
    pos[17][10] = pos_new(3, 3, 1);
    pos[17][11] = pos_new(4, 0, 0);
    pos[17][12] = pos_new(4, 1, 0);
    pos[17][13] = pos_new(5, 0, 0);
    pos[17][14] = pos_new(6, 0, 0);
    pos[17][15] = pos_new(5, 1, 3);
    pos[17][16] = pos_new(5, 3, 0);
    pos[17][17] = pos_new(6, 3, 0);

    pos[18][0]  = pos_new(0, 0, 3);
    pos[18][1]  = pos_new(0, 2, 0);
    pos[18][2]  = pos_new(1, 2, 0);
    pos[18][3]  = pos_new(0, 3, 1);
    pos[18][4]  = pos_new(2, 0, 0);
    pos[18][5]  = pos_new(2, 1, 0);
    pos[18][6]  = pos_new(2, 2, 0);
    pos[18][7]  = pos_new(2, 3, 0);
    pos[18][8]  = pos_new(3, 0, 2);
    pos[18][9]  = pos_new(3, 2, 1);
    pos[18][10] = pos_new(3, 3, 0);
    pos[18][11] = pos_new(4, 3, 0);
    pos[18][12] = pos_new(4, 0, 0);
    pos[18][13] = pos_new(4, 1, 0);
    pos[18][14] = pos_new(5, 0, 0);
    pos[18][15] = pos_new(6, 0, 0);
    pos[18][16] = pos_new(5, 1, 3);
    pos[18][17] = pos_new(5, 3, 0);
    pos[18][18] = pos_new(6, 3, 0);

    pos[19][0]  = pos_new(0, 0, 3);
    pos[19][1]  = pos_new(0, 2, 0);
    pos[19][2]  = pos_new(1, 2, 0);
    pos[19][3]  = pos_new(0, 3, 1);
    pos[19][4]  = pos_new(2, 0, 0);
    pos[19][5]  = pos_new(2, 1, 0);
    pos[19][6]  = pos_new(2, 2, 0);
    pos[19][7]  = pos_new(2, 3, 0);
    pos[19][8]  = pos_new(3, 0, 2);
    pos[19][9]  = pos_new(3, 2, 0);
    pos[19][10] = pos_new(3, 3, 0);
    pos[19][11] = pos_new(4, 0, 0);
    pos[19][12] = pos_new(4, 1, 0);
    pos[19][13] = pos_new(4, 2, 0);
    pos[19][14] = pos_new(4, 3, 0);
    pos[19][15] = pos_new(5, 0, 0);
    pos[19][16] = pos_new(6, 0, 0);
    pos[19][17] = pos_new(5, 1, 3);
    pos[19][18] = pos_new(5, 3, 0);
    pos[19][19] = pos_new(6, 3, 0);
}


typedef struct _Exec Exec;
struct _Exec {
    char *type;
    char *icon;
    char *bg;
    char *xtype;
    char *path;
};

void exec_destroy(Exec *exec)
{
    RET_IF(!exec);
    free(exec->type);
    if (exec->icon) free(exec->icon);
    if (exec->bg) free(exec->bg);
    if (exec->xtype) free(exec->xtype);
    free(exec->path);
}

static Exec *tag_parse_exec(XmlTag *tag)
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
        } else if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                if (exec->icon) free(exec->icon);
                exec->icon = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "bg")) {
            if (attr->val) {
                if (exec->bg) free(exec->bg);
                exec->bg = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "xtype")) {
            if (attr->val) {
                if (exec->xtype) free(exec->xtype);
                exec->xtype = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "path")) {
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

typedef struct _FileType FileType;
struct _FileType {
    char *type;
    char *ext;
    char *icon;
    char *bg;
    char *magic;

    char *xtype;
    char *exec;
};

static void filetype_destroy(FileType *filetype)
{
    if (filetype->type) free(filetype->type);
    if (filetype->ext) free(filetype->ext);
    if (filetype->icon) free(filetype->icon);
    if (filetype->bg) free(filetype->bg);
    if (filetype->magic) free(filetype->magic);

    if (filetype->xtype) free(filetype->xtype);
    if (filetype->exec) free(filetype->exec);
    free(filetype);
}

static FileType *_fileinfo_match_type(List *filetypes, FileInfo *fileinfo)
{
    List *l;
    FileType *ft;
    LIST_FOR_EACH(filetypes, l, ft) {
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

static FileType *tag_parse_filetype(XmlTag *tag, List *execs)
{
    List *ll;
    XmlAttr *attr;
    FileType *ft = NULL;
    ft = calloc(sizeof(FileType), 1);
    LIST_FOR_EACH(tag->attrs, ll, attr) {
        if (!strcmp(attr->key, "type")) {
            if (attr->val) {
                if (ft->type) free(ft->type);
                ft->type = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "icon")) {
            if (attr->val) {
                if (ft->icon) free(ft->icon);
                ft->icon = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "bg")) {
            if (attr->val) {
                if (ft->bg) free(ft->bg);
                ft->bg = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "exec")) {
            if (attr->val) {
                if (ft->exec) free(ft->exec);
                ft->exec = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "ext")) {
            if (attr->val) {
                if (ft->ext) free(ft->ext);
                ft->ext = strdup(attr->val);
            }
        } else if (!strcmp(attr->key, "magic")) {
            if (attr->val) {
                if (ft->magic) free(ft->magic);
                ft->magic = strdup(attr->val);
            }
        }
    }
    if (!ft->type) {
        filetype_destroy(ft);
        ERR("No ft type specfied: magic(%s),ext(%s)", ft->magic, ft->ext);
        return NULL;
    }
    if (!ft->ext && !ft->magic) {
        filetype_destroy(ft);
        ERR("No ft ext or magic specfied: type(%s)", ft->type);
        return NULL;
    }

    if (!ft->exec) {
        List *l;
        Exec *exec;
        LIST_FOR_EACH(execs, l, exec) {
            if (!strcmp(ft->type, exec->type)) {
                if (exec->icon && !ft->icon) {
                    ft->icon = strdup(exec->icon);
                }
                if (exec->bg && !ft->bg) {
                    ft->bg = strdup(exec->bg);
                }
                if (exec->xtype) ft->xtype = strdup(exec->xtype);
                ft->exec = strdup(exec->path);
                break;
            }
        }
    }

    return ft;
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *bgpath_local;
    char *bgpath;
    int item_ltx, item_lty;
    int item_gap;
    int item_w, item_h;
    int item_txt_h;
    char *rootpath;
    Pos *pos[MAX_FILE_CNT];
    List *filetypes;
};










static char *_get_thumb_url(const char *path, const char *ext)
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

typedef enum {
    ICON_TYPE_QUIT = 0,
    ICON_TYPE_UP,
    ICON_TYPE_PREV,
    ICON_TYPE_NEXT,
} FBIconType;

typedef struct _FBFile FBFile;
struct _FBFile {
    const FileType *ft;
    char *name;
    char *path;
};

static FBFile *fb_file_create(FileType *ft, FileInfo *fileinfo)
{
    FBFile *file = calloc(sizeof(FBFile), 1);
    file->ft = ft;
    file->path = strdup(fileinfo->path);
    file->name = strdup(fileinfo->name);
    return file;
}

static void fb_file_destroy(FBFile *file)
{
    free(file->path);
    free(file->name);
    free(file);
}

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

typedef enum {
    ITEM_TYPE_DIR = 0,
    ITEM_TYPE_FILE,
    ITEM_TYPE_URL,
    ITEM_TYPE_IMG,
    ITEM_TYPE_SVG,
    ITEM_TYPE_VIDEO,
    ITEM_TYPE_PDF
} FBItemType;

typedef struct _FBItem FBItem;
struct _FBItem {
    FBView *view;
    FBFile *file;

    int x, y, w, h;
    int txt_h;

    struct showone *group;

    struct showone *event;

    struct showone *bg_clip;
    Image *bg;
    // Video
    int video_idx;
    struct nemotimer *video_timer;

    struct showone *icon_shadow;
    struct showone *icon;

    Text *txt_shadow, *txt;
    Text *txt_shadow2, *txt2;
};

/*
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

    char *path = _get_thumb_url(it->path, buf);

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
        char *path = _get_thumb_url(it->path, NULL);
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
        char *path = _get_thumb_url(it->path, "001.jpg");
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

static void view_show_dir(FBView *view, const char *path);
static void view_show_page(FBView *view, int page_idx);

static void fb_item_exec(FBItem *it)
{
    FBView *view = it->view;
    struct nemoshow *show = nemowidget_get_show(view->widget);
    i (it->type == ITEM_TYPE_DIR) {
        view_show_dir(view, it->path);
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
                view_show_dir(view, uppath);
                free(uppath);
            } else if (icon->type == ICON_TYPE_PREV) {
                view_show_page(view, view->page_idx-1);
            } else if (icon->type == ICON_TYPE_NEXT) {
                view_show_page(view, view->page_idx+1);
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

*/
struct _FBView {
    ConfigApp *app;
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

static FBItem *view_append_item(FBView *view, FBFile *file, int x, int y, int w, int h)
{
    FBItem *it = calloc(sizeof(FBItem), 1);
    it->view = view;
    it->file = file;
    it->x = x;
    it->y = y;
    it->w = w;
    it->h = h;
    it->txt_h = view->app->item_txt_h;

    const char *font_family = view->app->config->font_family;
    const char *font_style = view->app->config->font_style;
    int font_size = view->app->config->font_size;

    struct showone *group;
    it->group = group = GROUP_CREATE(view->it_group);
    nemoshow_item_set_width(group, it->w);
    nemoshow_item_set_height(group, it->h);
    /*
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);
    */
    nemoshow_item_translate(group, it->x + it->w/2, it->y + it->h/2);

    struct showone *one;
    it->event = one = RECT_CREATE(group, it->w, it->h);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_tag(one, 0x1);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_alpha(one, 0.0);
    //nemoshow_item_set_fill_color(one, RGBA(RED));

    it->bg = image_create(group);
    image_set_anchor(it->bg, 0.5, 0.5);

    // FIXME: multibyte & length calculation for fixed size
    char buf[PATH_MAX];
    Text *txt;

    snprintf(buf, it->w/7, "%s", file->name);
    it->txt_shadow = txt = text_create(view->tool, group, font_family, font_style, font_size);
    text_set_fill_color(txt, 0, 0, 0, BLACK);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_show(txt, 0, 0, 0);

    it->txt = txt = text_create(view->tool, group, font_family, font_style, font_size);
    text_set_fill_color(txt, 0, 0, 0, WHITE);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_show(txt, 0, 0, 0);

    if (!strcmp(file->ft->type, "image") || strcmp(file->ft->type, "svg")) {
        snprintf(buf, it->w/7, "%s", "Image");
    } else if (!strcmp(file->ft->type, "video")) {
        snprintf(buf, it->w/7, "%s", "Video");
    } else if (!strcmp(file->ft->type, "pdf")) {
        snprintf(buf, it->w/7, "%s", "PDF");
    } else if (!strcmp(file->ft->type, "url")) {
        snprintf(buf, it->w/7, "%s", "URL");
    }
    it->txt_shadow2 = txt = text_create(view->tool, group, font_family, font_style, font_size);
    text_set_fill_color(txt, 0, 0, 0, BLACK);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_show(txt, 0, 0, 0);

    it->txt2 = txt = text_create(view->tool, group, font_family, font_style, font_size);
    text_set_fill_color(txt, 0, 0, 0, WHITE);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_show(txt, 0, 0, 0);

    struct showone *clip;
    clip = PATH_CREATE(group);
    nemoshow_item_path_moveto(one, 0, 0);
    nemoshow_item_path_lineto(one, it->w, 0);
    nemoshow_item_path_lineto(one, it->w, it->h);
    nemoshow_item_path_lineto(one, 0, 0);
    nemoshow_item_path_close(one);

    double iw, ih;
    if (file->ft->icon) {
        svg_get_wh(file->ft->icon, &iw, &ih);
        ERR("[%s] [%s] %lf, %lf", file->path, file->ft->icon, iw, ih);
        it->icon_shadow = one = SVG_PATH_CREATE(group, iw, ih, file->ft->icon);
        nemoshow_item_set_anchor(one, 0.5, 0.5);

        it->icon = one = SVG_PATH_CREATE(group, iw, ih, file->ft->icon);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
    }

    if (file->ft->bg) {
        List *fileinfos = fileinfo_readdir(file->ft->bg);
        if (fileinfos) {
            int r = rand() % list_count(fileinfos);
            FileInfo *fileinfo = LIST_DATA(list_get_nth(fileinfos, r));

            image_load_full(it->bg, view->tool,
                    fileinfo->path, it->w, it->h, NULL, NULL);
            text_translate(it->txt_shadow, 0, 0, 0,
                    -it->w/2 + view->app->item_gap + 1,
                    -it->h/2 + view->app->item_gap + 1);
            text_translate(it->txt, 0, 0, 0,
                    -it->w/2 + view->app->item_gap,
                    -it->h/2 + view->app->item_gap);
            text_translate(it->txt_shadow2, 0, 0, 0,
                    -it->w/2 + view->app->item_gap + 1,
                    -it->h/2 + view->app->item_gap + 1 + font_size);
            text_translate(it->txt2, 0, 0, 0,
                    -it->w/2 + view->app->item_gap,
                    -it->h/2 + view->app->item_gap + font_size);

            if (it->icon_shadow) {
                nemoshow_item_translate(it->icon_shadow,
                        -it->w/2 + it->w - iw - view->app->item_gap + 1,
                        -it->h/2 + it->h - ih - view->app->item_gap + 1);
            }
            if (it->icon) {
                nemoshow_item_translate(it->icon,
                        -it->w/2 + it->w - iw - view->app->item_gap,
                        -it->h/2 + it->h - ih - view->app->item_gap);
            }
        } else {
            ERR("No background files in the directory, %s", file->ft->bg);
        }
    } else {
        if (!strcmp(file->ft->type, "image")) {
            image_load_full(it->bg, view->tool, file->path,
                    it->w, it->h - it->txt_h, NULL, NULL);
            image_set_clip(it->bg, clip);
        } else if (!strcmp(file->ft->type, "svg")) {
        } else if (!strcmp(file->ft->type, "video")) {
        } else if (!strcmp(file->ft->type, "url")) {
        }
        //it->bg = one = IMAGE_CREATE(group, it->w, it->h - it->txt_h);
    }


    /*
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
    */

    view->items = list_append(view->items, it);
    return it;
}

static void view_destroy(FBView *view)
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


    /*
    FBItem *it;
    while((it = LIST_DATA(LIST_FIRST(view->items)))) {
        fb_remove_item(view, it);
    }
    */

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

static FBView *view_create(NemoWidget *parent, int width, int height, const char *path, ConfigApp *app)
{
    FBView *view = calloc(sizeof(FBView), 1);
    view->win = parent;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->w = width;
    view->h = height;
    view->rootpath = strdup(path);
    view->app = app;
    view->cnt_inpage = 20;
    if (app->bgpath_local) view->bgpath_local = strdup(app->bgpath_local);
    if (app->bgpath) view->bgpath = strdup(app->bgpath);

    /*
    view->itw = width/(POS_LAYER_CNT * 2) - 12;
    view->ith = height/(POS_LAYER_CNT * 2) - 12;
    */
    /*
    view->bg_worker = worker_create(view->tool);
    view->img_worker = worker_create(view->tool);
    */

    NemoWidget *widget;
    widget = nemowidget_create_vector(parent, width, height);
    //nemowidget_append_callback(widget, "event", _fb_event, view);
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

/*
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
*/

static void fb_arrange(FBView *view, uint32_t easetype, int duration, int delay)
{
    int x, y, w, h;
    x = view->w/2.0;
    y = view->h/2.0;
    w = view->itw;
    h = view->ith;

    /*
    int idx = 0;
    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        fb_item_translate(it, 0, 0, 0,
                x + w * pos[idx].x,
                y + h * pos[idx].y);
        idx++;
    }
    */
}

static void view_show_page(FBView *view, int page_idx)
{
    RET_IF(page_idx >= view->page_cnt);
    RET_IF(page_idx < 0);

    ERR("%d", page_idx);

    view->page_idx = page_idx;

    int idx = page_idx * view->cnt_inpage;

    /*
     * Only show page 1
    if (page_idx > 0) {
        fb_icon_show(view->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250);
    } else {
        fb_icon_hide(view->prev, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }

    if (idx + view->cnt_inpage < cnt) {
        fb_icon_show(view->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 400);
    } else {
        fb_icon_hide(view->next, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0);
    }
    */

    /*
    FBItem *it;
    while((it = LIST_DATA(LIST_FIRST(view->items)))) {
        fb_remove_item(view, it);
    }
    */

    List *l;
    List *files = NULL;
    FBFile *file;
    FileInfo *fileinfo;
    LIST_FOR_EACH(view->fileinfos, l, fileinfo) {
        if (fileinfo_is_dir(fileinfo)) {
            // XXXX
        } else {
            FileType *ft;
            ft = _fileinfo_match_type(view->app->filetypes, fileinfo);
            if (!ft) {
                ERR("Not supported ft: %s", fileinfo->path);
                continue;
            }
            file = fb_file_create(ft, fileinfo);
            files = list_append(files, file);
        }
        idx++;
        if (idx >= MAX_FILE_CNT) break;
    }

    ConfigApp *app = view->app;
    Pos **pos = view->app->pos;
    idx = 0;
    int cnt = list_count(files) - 1;
    LIST_FOR_EACH(files, l, file) {
        int x, y, w, h;
        x = pos[cnt][idx].x * (app->item_w + app->item_gap) + app->item_ltx;
        y = pos[cnt][idx].y * (app->item_h + app->item_gap) + app->item_lty;
        if (pos[cnt][idx].wh == 0) {
            w = app->item_w;
            h = app->item_h;
        } else if (pos[cnt][idx].wh == 1) {
            w = app->item_w * 2 + app->item_gap;
            h = app->item_h;
        } else if (pos[cnt][idx].wh == 2) {
            w = app->item_w;
            h = app->item_h * 2 + app->item_gap;
        } else if (pos[cnt][idx].wh == 3) {
            w = app->item_w * 2 + app->item_gap;
            h = app->item_h * 2 + app->item_gap;
        } else {
            ERR("Not supported position wh type: %d", pos[cnt][idx].wh);
            abort();
        }
        FBItem *it;
        it = view_append_item(view, file, x, y, w, h);
        idx++;
    }

    /*
    fb_arrange(view, 0, 0, 0);
    fb_show_items(view, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    */
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
    // Only show 1 page
    view->page_cnt = 1;

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
        _nemoshow_item_motion(view->bg_group, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "alpha", 1.0, NULL);

        if (view->bgdir_thread) thread_destroy(view->bgdir_thread);
        BackThreadData *bgdata = calloc(sizeof(BackThreadData), 1);
        bgdata->view = view;
        view->bgdir_thread = thread_create(view->tool,
                _load_bg_dir, _load_bg_dir_done, bgdata);
        view_show_page(view, 0);
    }
    free(data);
}

static void view_show_dir(FBView *view, const char *path)
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

    /*
    int delay = 0;
    List *l;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        fb_item_hide(it, NEMOEASE_CUBIC_OUT_TYPE, 150, delay);
        delay += 30;
    }
    */
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
    app->item_ltx = 45;
    app->item_lty = 115;
    app->item_gap = 10;
    app->item_w = 85;
    app->item_h = 85;
    app->item_txt_h = 40;

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        const char *prefix= "config";
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/background", prefix);
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

        snprintf(buf, PATH_MAX, "%s/item", prefix);
        temp = xml_get_value(xml, buf, "ltx");
        if (temp && strlen(temp) > 0) {
            app->item_ltx = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "lty");
        if (temp && strlen(temp) > 0) {
            app->item_lty = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "gap");
        if (temp && strlen(temp) > 0) {
            app->item_gap = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "width");
        if (temp && strlen(temp) > 0) {
            app->item_w = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "height");
        if (temp && strlen(temp) > 0) {
            app->item_h = atoi(temp);
        }
        temp = xml_get_value(xml, buf, "txt_height");
        if (temp && strlen(temp) > 0) {
            app->item_txt_h = atoi(temp);
        }

        List *l;
        XmlTag *tag;

        List *execs = NULL;
        snprintf(buf, PATH_MAX, "%s/exec", prefix);
        List *_execs = xml_search_tags(xml, buf);
        LIST_FOR_EACH(_execs, l, tag) {
            Exec *exec = tag_parse_exec(tag);
            if (!exec) continue;
            execs = list_append(execs, exec);
        }

        snprintf(buf, PATH_MAX, "%s/filetype", prefix);
        List *types = xml_search_tags(xml, buf);
        LIST_FOR_EACH(types, l, tag) {
            FileType *ft = tag_parse_filetype(tag, execs);
            if (!ft) continue;
            app->filetypes = list_append(app->filetypes, ft);
        }

        Exec *exec;
        LIST_FREE(execs, exec) exec_destroy(exec);

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

    FileType *ft;
    LIST_FREE(app->filetypes, ft) filetype_destroy(ft);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->rootpath) {
        app->rootpath = strdup(file_get_homedir());
    }
    pos_init(app->pos);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, app->config);

    FBView *view;
    view = view_create(win, app->config->width, app->config->height, app->rootpath, app);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    view_show_dir(view, app->rootpath);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    view_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);

    _config_unload(app);

    return 0;
}
