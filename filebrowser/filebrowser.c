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

#include "xemoapp.h"
#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "helper_skia.h"

#define COLOR_BASE 0x00FFC2FF
#define COLOR_ICON 0x53AED6FF
#define COLOR_TXT_BACK 0x00BF92FF
#define COLOR_TXT 0xFFFFFFFF

#define BACK_INTERVAL 5000
#define MAX_FILE_CNT 20
#define VIDEO_INTERVAL (1000.0/24)

static char *_parse_url(const char *path)
{
    RET_IF(!path, NULL);

    int line_len;
    char **line_txt = file_read_line(path, &line_len);

    RET_IF(!line_txt || !line_txt[0] || line_len <= 0, NULL);

    int i;
    for (i = 0 ; i < line_len ; i++) {
        if (!line_txt[i]) continue;
        if (strstr(line_txt[i], "URL=")) {
            char *begin = strstr(line_txt[i], "URL=");
            begin+=4;
            return strdup(begin);
        }
    }

    return NULL;
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

List *fileinfo_readdir_img(const char *path)
{
    List *l, *ll;
    List *files = fileinfo_readdir(path);
    FileInfo *file;
    LIST_FOR_EACH_SAFE(files, l, ll, file) {
        if (!fileinfo_is_image(file)) {
            files = list_remove(files, file);
        }
    }
    return files;
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

typedef struct _TypeFile TypeFile;
struct _TypeFile {
    List *files;
    FileType *ft;
};

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *bgpath_local;
    char *bgpath;
    char *sort_style;
    char *titlepath;
    int title_ltx, title_lty;
    int item_ltx, item_lty;
    int item_gap;
    int item_w, item_h;
    int item_txt_h;
    char *rootpath;
    Pos *pos[MAX_FILE_CNT];
    List *filetypes;
};

static void _clip_update(struct showone *clip, int x, int y, int w, int h)
{
    nemoshow_item_path_moveto(clip, x, y);
    nemoshow_item_path_lineto(clip, w, y);
    nemoshow_item_path_lineto(clip, w, h);
    nemoshow_item_path_lineto(clip, x, h);
    nemoshow_item_path_close(clip);
}

static struct showone *_clip_create(struct showone *parent, int x, int y, int w, int h)
{
    struct showone *clip;
    clip = PATH_CREATE(parent);
    _clip_update(clip, x, y, w, h);
    //nemoshow_item_set_fill_color(clip, RGBA(RED));
    return clip;
}

char *remove_ext(const char *path)
{
    char dot='.';
    RET_IF(!path, NULL);
    char *ret = malloc(strlen(path) + 1);

    strcpy(ret, path);
    char *last = strrchr(ret, dot);

    if (last) {
        *last = '\0';
    }

    return ret;
}

static char *_get_url_image_url(const char *path, const char *ext)
{
    if (!path) return NULL;
    char *dir = file_get_dirname(path);
    char *filename = file_get_basename(path);
    char *ret;

    char *base = remove_ext(filename);
    free(filename);

    if (ext) {
        ret = strdup_printf("%s/.%s.%s", dir, base, ext);
    } else {
        ret = strdup_printf("%s/.%s", dir, base);
    }
    free(dir);
    free(base);

    return ret;
}

static char *_get_thumb_url(const char *path, const char *ext)
{
    if (!path) return NULL;
    char *dir = file_get_dirname(path);
    char *base = file_get_basename(path);
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
    Inlist __inlist;
    const FileType *ft;
    char *name;
    char *path;
};

static FBFile *fb_file_create(FileType *ft, const FileInfo *fileinfo)
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
    FBItemType type;
    FBView *view;
    FBFile *file;

    int x, y, w, h;
    int name_h;

    struct showone *group;

    struct showone *blur;
    struct showone *event;
    struct showone *event_bg;

    struct showone *bg_clip;

    // Union bg variables
    Image *bg_img;
    struct showone *bg_svg;
    struct showone *bg_video;

    // Video
    int video_idx;
    struct nemotimer *bg_video_timer;

    struct showone *icon_shadow;
    struct showone *icon;

    Text *name;
    struct nemotimer *name_scrollback_timer, *name_scroll_timer;
    double name_width, name_text_width;
    int name_x, name_y;
    struct showone *name_clip;
    int name_clip_w, name_clip_h;
    Text *name1;
}
;
struct _FBView {
    ConfigApp *app;
    struct nemoshow *show;
    struct nemotool *tool;
    const char *uuid;
    int w, h;

    NemoWidget *widget;

    struct showone *bg_group;
    Image *bg0;
    struct showone *clip;

    char *bgpath_local;
    char *bgpath;
    Image *bg;
    List *bgfileinfos;
    int bgfileinfos_idx;
    struct nemotimer *bg_change_timer;
    Thread *bgdir_thread;
    Text *title, *title1;

    struct showone *it_group;
    List *items;

    Thread *filethread;

    Inlist *files;

    char *rootpath;
    char *curpath;
};

static void item_show(FBItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->bg_video_timer) {
        nemotimer_set_timeout(it->bg_video_timer, 10 + delay);
        it->video_idx = 1;
    }
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion(it->event_bg, easetype, duration, delay + duration,
                "alpha", 1.0, NULL);
        _nemoshow_item_motion(it->event, easetype, duration, delay + duration,
                "alpha", 1.0, NULL);
        if (it->bg_svg) {
            _nemoshow_item_motion_bounce(it->bg_svg, easetype, duration, delay + 100,
                    "sx", 1.2, 1.0,
                    NULL);
        }
        if (it->bg_video) {
            _nemoshow_item_motion_bounce(it->bg_video, easetype, duration, delay + 100,
                    "sx", 1.2, 1.0, NULL);
        }
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
        nemoshow_item_set_alpha(it->event_bg, 1.0);
        nemoshow_item_set_alpha(it->event, 1.0);
        if (it->bg_svg) nemoshow_item_scale(it->bg_svg, 1.0, 1.0);
        if (it->bg_video) nemoshow_item_scale(it->bg_video, 1.0, 1.0);
    }
    if (it->bg_img) {
        image_scale(it->bg_img, easetype, duration, delay + 100,
                1.0, 1.0);
    }

    if (it->name_scrollback_timer) {
        nemotimer_set_timeout(it->name_scrollback_timer, 2000);
    }
    text_set_alpha(it->name, easetype, duration, delay + duration + 100, 1.0);
    text_set_alpha(it->name1, easetype, duration, delay + duration + 100, 1.0);
    text_set_scale(it->name, easetype, duration, delay + duration + 100, 1.0, 1.0);
    text_set_scale(it->name1, easetype, duration, delay + duration + 100, 1.0, 1.0);
}

static void item_hide(FBItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->bg_video_timer) nemotimer_set_timeout(it->bg_video_timer, 0);
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(it->event_bg, easetype, duration, delay,
                "alpha", 0.0, NULL);
        _nemoshow_item_motion(it->event, easetype, duration, delay,
                "alpha", 0.0, NULL);
        if (it->bg_svg) {
            _nemoshow_item_motion(it->bg_svg, easetype, duration, delay,
                    "sx", 0.0, NULL);
        }
        if (it->bg_video) {
            _nemoshow_item_motion(it->bg_video, easetype, duration, delay,
                    "sx", 0.0, NULL);
        }
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
        nemoshow_item_set_alpha(it->event_bg, 0.0);
        nemoshow_item_set_alpha(it->event, 0.0);
        if (it->bg_svg) nemoshow_item_scale(it->bg_svg, 0.0, 0.0);
        if (it->bg_video) nemoshow_item_scale(it->bg_video, 0.0, 0.0);
    }
    if (it->bg_img) {
        image_scale(it->bg_img, easetype, duration, delay, 0.0, 0.0);
    }
    if (it->name_scroll_timer) {
        nemotimer_set_timeout(it->name_scroll_timer, 0);
    }
    if (it->name_scrollback_timer) {
        nemotimer_set_timeout(it->name_scrollback_timer, 0);
    }
    text_set_alpha(it->name, easetype, duration, delay, 0.0);
    text_set_alpha(it->name1, easetype, duration, delay, 0.0);
    text_set_scale(it->name, easetype, duration, delay, 0.0, 0.0);
    text_set_scale(it->name1, easetype, duration, delay, 0.0, 0.0);
}

static void item_down(FBItem *it)
{
    _nemoshow_item_motion_bounce(it->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 0.6, 0.7, "sy", 0.6, 0.7,
            NULL);
}

static void item_up(FBItem *it)
{
    _nemoshow_item_motion_bounce(it->group, NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
}

static void item_translate(FBItem *it, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(it->group, tx, ty);
    }
}

static void item_exec(FBItem *it)
{
    FBView *view = it->view;
    struct nemoshow *show = nemowidget_get_show(view->widget);
    if (it->type == ITEM_TYPE_DIR) {
        //view_show_dir(view, it->path);
    } else {
        char *type;
        if (it->file->ft->xtype) {
            type = strdup(it->file->ft->xtype);
        } else {
            type = strdup("app");
        }

        char *path;
        if (it->type == ITEM_TYPE_URL) {
            path = _parse_url(it->file->path);
            if (!path) {
                ERR("No url in the %s", it->file->path);
                return;
            }
        } else {
            path = strdup(it->file->path);
        }

        float x, y;
        nemoshow_transform_to_viewport(show,
                NEMOSHOW_ITEM_AT(it->group, tx),
                NEMOSHOW_ITEM_AT(it->group, ty),
                &x, &y);

        if (!it->file->ft->exec) {
            nemo_execute(view->uuid, type, path, NULL, NULL, x, y, 0, 1, 1);
        } else {
            char _path[PATH_MAX];
            char name[PATH_MAX];
            char args[PATH_MAX];

            char *buf;
            char *tok;
            buf = strdup(it->file->ft->exec);
            tok = strtok(buf, ";");
            snprintf(name, PATH_MAX, "%s", tok);
            snprintf(_path, PATH_MAX, "%s", tok);
            tok = strtok(NULL, "");
            if (tok) {
                if (it->type == ITEM_TYPE_URL) {
                    snprintf(args, PATH_MAX, "%s;--url=%s", tok, path);
                } else {
                    snprintf(args, PATH_MAX, "%s;%s", tok, path);
                }
            } else {
                if (it->type == ITEM_TYPE_URL) {
                    snprintf(args, PATH_MAX, "--url=%s", path);
                } else {
                    snprintf(args, PATH_MAX, "%s", path);
                }
            }
            free(buf);
            ERR("%s", args);
            nemo_execute(view->uuid, type, _path, args, NULL, x, y, 0, 1, 1);
        }
    }
}

static void _item_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    FBItem *it = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        item_down(it);
    } else if (nemoshow_event_is_up(show, event)) {
        item_up(it);
        if (nemoshow_event_is_single_click(show, event)) {
            item_exec(it);
        }
    }
    nemoshow_dispatch_frame(show);
}

static void _bg_video_timer(struct nemotimer *timer, void *userdata)
{
    FBItem *it = userdata;

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%03d.jpg", it->video_idx);

    char *path = _get_thumb_url(it->file->path, buf);

    if (!file_is_exist(path) || file_is_null(path)) {
        ERR("No animation thumbnail: %s", path);
        nemotimer_set_timeout(timer, 0);
        return;
    }

    if (nemoshow_item_get_width(it->bg_video) <= 0 ||
            nemoshow_item_get_height(it->bg_video) <= 0) {
        int w, h;
        if (!file_get_image_wh(path, &w, &h)) {
            ERR("image get wh failed: %s", path);
        } else {
            _rect_ratio_full(w, h, it->w, it->h, &w, &h);
            nemoshow_item_set_width(it->bg_video, w);
            nemoshow_item_set_height(it->bg_video, h);
            //nemoshow_item_set_anchor(it->bg_video, 0.5, 0.5);
            //nemoshow_item_translate(it->bg_video, it->w/2, it->h/2);
#if 0
            // XXX: move clip as center align
            nemoshow_item_path_translate(it->clip,
                    -(it->w - w)/2, -(it->h - h)/2);
            _nemoshow_item_motion(it->bg_video,
                    NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                    "alpha", 1.0,
                    NULL);
#endif
        }
    }

    nemoshow_item_set_uri(it->bg_video, path);
    if (path) free(path);

    nemoshow_dispatch_frame(it->view->show);

    it->video_idx++;
    if (it->video_idx > 120) it->video_idx = 1;
    nemotimer_set_timeout(timer, VIDEO_INTERVAL);
}

static void view_item_destroy(FBItem *it)
{
    fb_file_destroy(it->file);
    if (it->name1) text_destroy(it->name1);
    if (it->name) text_destroy(it->name);
    if (it->name_scrollback_timer) nemotimer_destroy(it->name_scrollback_timer);
    if (it->name_scroll_timer) nemotimer_destroy(it->name_scroll_timer);
    if (it->icon_shadow) nemoshow_one_destroy(it->icon_shadow);
    if (it->icon) nemoshow_one_destroy(it->icon);
    if (it->bg_video_timer) nemotimer_destroy(it->bg_video_timer);
    if (it->bg_video) nemoshow_one_destroy(it->bg_video);
    if (it->bg_svg) nemoshow_one_destroy(it->bg_svg);
    if (it->bg_img) image_destroy(it->bg_img);

    nemoshow_one_destroy(it->bg_clip);
    nemoshow_one_destroy(it->event);
    nemoshow_one_destroy(it->event_bg);
    nemoshow_one_destroy(it->group);
    nemoshow_one_destroy(it->blur);

    free(it);
}

static void _item_name_scroll_update(NemoMotion *m, uint32_t time, double t, void *userdata)
{
    FBItem *it = userdata;
    nemoshow_item_path_clear(it->name_clip);

    double tx, ty;
    text_get_translate(it->name, &tx, &ty);
    // XXX: clip is XXX!!!
    _clip_update(it->name_clip, it->name_x - tx, 0,
            it->name_clip_w + (it->name_x - tx), it->name_clip_h);
}

static void _item_name_scroll_timer(struct nemotimer *timer, void *userdata)
{
    FBItem *it = userdata;
    int duration = (it->name_text_width - it->name_width) * 50;
    if (duration <= 0) duration = 1000;

    // FIXME: delay is weired if set clip is used.
    text_translate_with_callback(it->name, NEMOEASE_LINEAR_TYPE, duration, 0,
             it->name_x - (it->name_text_width - it->name_width), it->name_y,
             _item_name_scroll_update, it);

    nemotimer_set_timeout(it->name_scrollback_timer, duration + 1000);
    nemoshow_dispatch_frame(it->view->show);
}

static void _item_name_scrollback_timer(struct nemotimer *timer, void *userdata)
{
    FBItem *it = userdata;

    // FIXME: delay is weired if set clip is used.
    text_translate_with_callback(it->name, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            it->name_x, it->name_y,
             _item_name_scroll_update, it);
    nemotimer_set_timeout(it->name_scroll_timer, 1500);

    nemoshow_dispatch_frame(it->view->show);
}

static FBItem *view_item_create(FBView *view, FBFile *file, int x, int y, int w, int h)
{
    FBItem *it = calloc(sizeof(FBItem), 1);
    it->view = view;
    it->file = file;
    it->x = x;
    it->y = y;
    it->w = w;
    it->h = h;
    it->name_h = view->app->item_txt_h;

    if (!strcmp(file->ft->type, "image")) {
        it->type = ITEM_TYPE_IMG;
    } else if (!strcmp(file->ft->type, "svg")) {
        it->type = ITEM_TYPE_SVG;
    } else if (!strcmp(file->ft->type, "video")) {
        it->type = ITEM_TYPE_VIDEO;
    } else if (!strcmp(file->ft->type, "pdf")) {
        it->type = ITEM_TYPE_PDF;
    } else if (!strcmp(file->ft->type, "url")) {
        it->type = ITEM_TYPE_URL;
    } else {
        ERR("Not supported type: %s", file->ft->type);
        free(it);
        return NULL;
    }

    const char *font_family = view->app->config->font_family;
    const char *font_style = view->app->config->font_style;
    int font_size = view->app->config->font_size;

    struct showone *group;
    struct showone *one;

    struct showone *blur;
    it->blur = blur = BLUR_CREATE("solid", 5);

    it->group = group = GROUP_CREATE(view->it_group);
    nemoshow_item_set_anchor(group, 0.5, 0.5);
    nemoshow_item_set_width(group, it->w);
    nemoshow_item_set_height(group, it->h);
    nemoshow_item_translate(group, it->x, it->y + it->h/2);
    nemoshow_item_set_alpha(group, 0.0);

    it->event_bg = one = RECT_CREATE(group, it->w, it->h);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    nemoshow_item_set_filter(one, blur);
    nemoshow_item_translate(one, 1, 1);
    nemoshow_item_set_alpha(one, 0.0);

    it->event = one = RECT_CREATE(group, it->w, it->h);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    //nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_tag(one, 0x1);
    nemoshow_one_set_userdata(one, it);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));
    nemoshow_item_set_alpha(one, 0.0);

    struct showone *clip;

    if (file->ft->bg) {
        it->bg_clip = clip = _clip_create(group, 0, 0, it->w, it->h);

        List *fileinfos = fileinfo_readdir_img(file->ft->bg);
        if (fileinfos) {
            FileInfo *fileinfo = LIST_DATA(LIST_LAST(fileinfos));

            Image *img;
            it->bg_img = img = image_create(group);
            image_set_clip(img, clip);
            image_load_full(img, view->tool,
                    fileinfo->path, it->w, it->h, NULL, NULL);
            image_scale(img, 0, 0, 0, 0.0, 1.0);
        } else {
            ERR("No background files in the directory, %s", file->ft->bg);
        }
        FileInfo *fileinfo;
        LIST_FREE(fileinfos, fileinfo) fileinfo_destroy(fileinfo);
    } else {
        it->bg_clip = clip = _clip_create(group, 0, 0, it->w, it->h - it->name_h);

        if (it->type == ITEM_TYPE_IMG) {
            Image *img;
            it->bg_img = img = image_create(group);
            image_set_clip(img, clip);
            image_scale(img, 0, 0, 0, 0.0, 1.0);

            char *path = _get_thumb_url(file->path, NULL);
            if (!file_is_exist(path) || file_is_null(path)) {
                image_load_full(img, view->tool, file->path,
                        it->w, it->h - it->name_h, NULL, NULL);
            } else {
                image_load_full(img, view->tool, path,
                        it->w, it->h - it->name_h, NULL, NULL);
            }
            free(path);
        } else if (it->type == ITEM_TYPE_SVG) {
            it->bg_svg = one = SVG_PATH_GROUP_CREATE(group, it->w, it->h, file->path);
            nemoshow_item_set_clip(one, clip);
            nemoshow_item_scale(one, 0.0, 1.0);
        } else if (it->type == ITEM_TYPE_VIDEO) {
            ITEM_CREATE(one, group, NEMOSHOW_IMAGE_ITEM);
            nemoshow_item_set_clip(one, clip);
            it->bg_video = one;
            it->bg_video_timer = TOOL_ADD_TIMER(view->tool, 0, _bg_video_timer, it);
            nemoshow_item_scale(one, 0.0, 1.0);
        } else if (it->type == ITEM_TYPE_URL) {
            Image *img;
            it->bg_img = img = image_create(group);
            image_set_clip(img, clip);
            image_scale(img, 0, 0, 0, 0.0, 1.0);

            char *path = _get_url_image_url(file->path, "png");
            if (file_is_exist(path) && !file_is_null(path)) {
                image_load_full(img, view->tool, path,
                        it->w, it->h - it->name_h, NULL, NULL);
            } else {
                ERR("No thumbnail for url(%s): %s", file->path, path);
            }
            free(path);
        }
    }

    double iw, ih;
    if (file->ft->icon) {
        if (svg_get_wh(file->ft->icon, &iw, &ih)) {
            it->icon_shadow = one = SVG_PATH_GROUP_CREATE(group, iw, ih, file->ft->icon);
            nemoshow_item_set_fill_color(one, RGBA(GRAY));
            nemoshow_item_set_alpha(one, 0.3);
            nemoshow_item_set_filter(one, blur);
            it->icon = one = SVG_PATH_GROUP_CREATE(group, iw, ih, file->ft->icon);
        } else {
            ERR("svg_get_wh() failed");
        }
    }

    char buf[PATH_MAX];
    Text *txt;

    int gap = view->app->item_gap;
    // FIXME: Use harfbuzz or freetype to reduce performance issue!!!
    double tw = skia_calculate_text_width(font_family, font_style, font_size, file->name);
    it->name_text_width = tw;
    it->name_width = it->w - 3 * gap;
    if (it->name_text_width > it->name_width) {
#if 0
        // Recalculate string size to fit the string within it->w - 2 * gap
        int maxlen = strlen(file->name);
        int len = 0;
        while (file->name[len] && len <= maxlen) {
            int diff = 0;
            if ((file->name[len] & 0xF0) == 0xF0)  { // 4 bytes
                diff+=4;
            } else if ((file->name[len] & 0xE0) == 0xE0) { // 3 bytes
                diff+=3;
            } else if ((file->name[len] & 0xC0) == 0xC0) { // 2 bytes
                diff+=2;
            } else { // a byte
                diff+=1;
            }
            len += diff;
            // XXX: don't use snprintf because it's unicode, not simple string!
            memcpy(buf, file->name, len);
            buf[len] = '\0';
            // FIXME: Use harfbuzz or freetype to reduce performance issue!!!
            tw = skia_calculate_text_width(font_family, font_style, font_size, buf);

            if (tw > (it->w - 2 * gap)) {
                len -= diff;
                if (len <= 0) {
                    memset(buf, 0, sizeof(buf));
                } else {
                    memcpy(buf, file->name, len);
                }
                break;
            }
        }
#endif
        it->name_scrollback_timer = TOOL_ADD_TIMER(view->tool, 0, _item_name_scrollback_timer, it);
        it->name_scroll_timer = TOOL_ADD_TIMER(view->tool, 0, _item_name_scroll_timer, it);
    }
    // XXX: don't use snprintf because it's unicode, not simple string!
    memcpy(buf, file->name, strlen(file->name));
    buf[strlen(file->name)] = '\0';

    it->name_clip_w = it->w - 2 - gap * 2;
    it->name_clip_h = font_size;
    it->name_clip = clip = _clip_create(group, 0, 0, it->name_clip_w, it->name_clip_h);

    it->name = txt = text_create(view->tool, group, font_family, font_style, font_size);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_set_alpha(txt, 0, 0, 0, 1.0);
    text_set_scale(txt, 0, 0, 0, 0.0, 1.0);
    text_set_clip(txt, clip);

    if (it->type == ITEM_TYPE_IMG || it->type == ITEM_TYPE_SVG) {
        snprintf(buf, PATH_MAX, "%s", "Image");
    } else if (it->type == ITEM_TYPE_VIDEO) {
        snprintf(buf, PATH_MAX, "%s", "Video");
    } else if (it->type == ITEM_TYPE_PDF) {
        snprintf(buf, PATH_MAX, "%s", "PDF");
    } else if (it->type == ITEM_TYPE_URL) {
        snprintf(buf, PATH_MAX, "%s", "URL");
    }
    it->name1 = txt = text_create(view->tool, group, font_family, font_style, font_size/1.5);
    text_set_anchor(txt, 0.0, 0.0);
    text_update(txt, 0, 0, 0, buf);
    text_set_alpha(txt, 0, 0, 0, 1.0);
    text_set_scale(txt, 0, 0, 0, 0.0, 1.0);

    if (file->ft->bg) {
        text_set_fill_color(it->name, 0, 0, 0, WHITE);
        text_set_fill_color(it->name1, 0, 0, 0, WHITE);

        int gap = view->app->item_gap;
        it->name_x = gap;
        it->name_y = gap;
        text_translate(it->name, 0, 0, 0, it->name_x, it->name_y);
        double th;
        text_get_size(it->name, NULL, &th);
        text_translate(it->name1, 0, 0, 0, gap, gap + th + 2);

        if (it->icon_shadow) {
            nemoshow_item_translate(it->icon_shadow,
                    it->w - iw - gap + 1, it->h - ih - gap + 1);
        }
        if (it->icon) {
            nemoshow_item_translate(it->icon,
                    it->w - iw - gap, it->h - ih - gap);
        }
    } else {
        text_set_fill_color(it->name, 0, 0, 0, BLACK);
        text_set_fill_color(it->name1, 0, 0, 0, BLACK);

        int gap_w = view->app->item_gap;
        int gap_h = view->app->item_gap/2;
        it->name_x = gap_w;
        it->name_y = it->h - it->name_h + gap_h;
        text_translate(it->name, 0, 0, 0, it->name_x, it->name_y);

        double th;
        text_get_size(it->name, NULL, &th);
        text_translate(it->name1, 0, 0, 0, gap_w, it->h - it->name_h + gap_h + th + 2);

        if (it->icon_shadow) {
            nemoshow_item_translate(it->icon_shadow,
                    it->w/2 - iw/2 + 1, (it->h - it->name_h)/2 - ih/2 + 1);
        }
        if (it->icon) {
            nemoshow_item_translate(it->icon,
                    it->w/2 - iw/2, (it->h - it->name_h)/2 - ih/2);
        }
    }

    return it;
}

static void view_destroy(FBView *view)
{
    FBItem *it;
    LIST_FREE(view->items, it) view_item_destroy(it);

    FBFile *file;
    INLIST_FREE(view->files, file) fb_file_destroy(file);

    if (view->curpath) free(view->curpath);

    nemoshow_one_destroy(view->it_group);

    text_destroy(view->title);
    text_destroy(view->title1);

    FileInfo *fileinfo;
    LIST_FREE(view->bgfileinfos, fileinfo) fileinfo_destroy(fileinfo);
    nemotimer_destroy(view->bg_change_timer);
    image_destroy(view->bg);
    image_destroy(view->bg0);
    nemoshow_one_destroy(view->bg_group);

    nemowidget_destroy(view->widget);

    if (view->bgpath) free(view->bgpath);
    if (view->bgpath_local) free(view->bgpath_local);
    free(view->rootpath);
    free(view);
}

static void _fb_bg_change_timer(struct nemotimer *timer, void *userdata)
{
    FBView *view = userdata;

    FileInfo *file = LIST_DATA(list_get_nth(view->bgfileinfos, view->bgfileinfos_idx));


    int w, h;
    file_get_image_wh(file->path, &w, &h);
    image_load(view->bg, view->tool, file->path, w, h, NULL, NULL);
    image_set_alpha(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0, 1.0);
    image_translate(view->bg, 0, 0, 0, w/2.0, h/2.0);

    view->bgfileinfos_idx++;
    if (view->bgfileinfos_idx >= list_count(view->bgfileinfos))
        view->bgfileinfos_idx = 0;

    if (list_count(view->bgfileinfos) > 1)
        nemotimer_set_timeout(timer, BACK_INTERVAL);

    const char *img_path = APP_RES_DIR"/back.png";
    int ww, hh;
    if (!file_get_image_wh(img_path, &ww, &hh)) {
        ERR("image get wh failed: %s", img_path);
    } else {
        struct showone *clip;
        view->clip = clip = _clip_create(view->bg_group, 0, 0, view->w, view->h - h);
        image_load(view->bg0, view->tool, img_path, ww, hh, NULL, NULL);
        image_translate(view->bg0, 0, 0, 0, 0, h);
        image_set_alpha(view->bg0, NEMOEASE_CUBIC_OUT_TYPE, 2000, 500, 1.0);
        image_set_clip(view->bg0, clip);
    }

    nemoshow_dispatch_frame(view->show);
}

static void _view_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    FBView *view = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event), nemoshow_event_get_y(event), &ex, &ey);
    if (nemoshow_event_is_down(show, event)) {
        if (ey > view->h) {
            struct nemotool *tool = nemowidget_get_tool(widget);
            float vx, vy;
            nemoshow_transform_to_viewport(show,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event), &vx, &vy);
            nemotool_touch_bypass(tool,
                    nemoshow_event_get_device(event),
                    vx, vy);
            return;
        }

        if ((nemoshow_event_get_tapcount(event) >= 2)) {
            nemoshow_event_set_done_all(event);
            List *l;
            NemoGrab *grab;
            LIST_FOR_EACH(widget->usergrabs, l, grab) {
                NemoWidgetGrab *wgrab = grab->userdata;
                struct showone *one = nemowidget_grab_get_data(wgrab, "one");
                if (nemoshow_one_get_tag(one) == 0x1) {
                    FBItem *it = nemoshow_one_get_userdata(one);
                    item_up(it);
                }
            }
            nemoshow_dispatch_frame(show);
            return;
        }

        struct showone *one;
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            if (nemoshow_one_get_tag(one) == 0x1) {
                FBItem *it = nemoshow_one_get_userdata(one);
                if (it) {
                    NemoWidgetGrab *grab =
                        nemowidget_create_grab(widget, event,
                                _item_grab_event, it);
                    nemowidget_grab_set_data(grab, "one", one);
                }
            }
        }
    }
    nemoshow_dispatch_frame(view->show);
}

static FBView *view_create(NemoWidget *parent, int width, int height, const char *path, ConfigApp *app)
{
    FBView *view = calloc(sizeof(FBView), 1);
    view->app = app;
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->uuid = nemowidget_get_uuid(parent);
    view->w = width;
    view->h = height;
    view->rootpath = strdup(path);
    if (app->bgpath_local) view->bgpath_local = strdup(app->bgpath_local);
    if (app->bgpath) view->bgpath = strdup(app->bgpath);

    NemoWidget *widget;
    widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _view_event, view);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_show(widget, 0, 0, 0);
    view->widget = widget;

    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

    struct showone *group;
    view->bg_group = group = GROUP_CREATE(canvas);

    Image *img;
    view->bg0 = img = image_create(group);

    const char *font_family = view->app->config->font_family;
    const char *font_style = view->app->config->font_style;

    view->bg = img = image_create(group);
    image_set_anchor(img, 0.5, 0.5);
    image_set_alpha(img, 0, 0, 0, 0.0);
    view->bg_change_timer = TOOL_ADD_TIMER(view->tool, 0, _fb_bg_change_timer, view);

    Text *txt;
    view->title = txt = text_create(view->tool, group, font_family, font_style, 40);
    text_set_anchor(txt, 0.0, 0.0);
    text_translate(txt, 0, 0, 0, 0, view->app->title_lty);

    view->title1 = txt = text_create(view->tool, group, font_family, font_style, 8);
    text_set_anchor(txt, 0.0, 0.0);
    text_set_alpha(txt, 0, 0, 0, 1.0);
    text_translate(txt, 0, 0, 0, 0, view->app->title_lty);

    view->it_group = GROUP_CREATE(canvas);

    return view;
}

static void view_show(FBView *view)
{

    ConfigApp *app = view->app;
    Pos **pos = view->app->pos;

    int cnt = inlist_count(view->files);
    if (cnt >= MAX_FILE_CNT) {
        cnt = MAX_FILE_CNT - 1;
    } else {
        cnt = cnt - 1;
    }

    FBFile *file = INLIST_DATA(INLIST_FIRST(view->files), FBFile);
    List *l;
    int i;
    for (i = 0 ; i < MAX_FILE_CNT ; i++) {
        int x, y, w, h;
        x = pos[cnt][i].x * (app->item_w + app->item_gap) + app->item_ltx;
        y = pos[cnt][i].y * (app->item_h + app->item_gap) + app->item_lty;
        if (pos[cnt][i].wh == 0) {
            w = app->item_w;
            h = app->item_h;
        } else if (pos[cnt][i].wh == 1) {
            w = app->item_w * 2 + app->item_gap;
            h = app->item_h;
        } else if (pos[cnt][i].wh == 2) {
            w = app->item_w;
            h = app->item_h * 2 + app->item_gap;
        } else if (pos[cnt][i].wh == 3) {
            w = app->item_w * 2 + app->item_gap;
            h = app->item_h * 2 + app->item_gap;
        } else {
            ERR("Not supported position wh type: %d", pos[cnt][i].wh);
            break;
        }
        FBItem *it = view_item_create(view, file, x, y, w, h);
        if (!it) {
            ERR("view_item_create() failed: %s", file->path);
        } else {
            view->items = list_append(view->items, it);
        }
        file = INLIST_DATA(INLIST(file)->next, FBFile);
        if (!file) break;
    }

    int delay = 0;
    FBItem *it;
    LIST_FOR_EACH(view->items, l, it) {
        item_show(it, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay);
        item_translate(it, 0, 0, 0,
                it->x + it->w/2, it->y + it->h/2);
        delay += 50;
    }
    nemoshow_dispatch_frame(view->show);
}

typedef struct _BackThreadData BackThreadData;
struct _BackThreadData {
    FBView *view;
    int w, h;
    List *bgfileinfos;
};

static void _load_bg_dir_done(bool cancel, void *userdata)
{
    BackThreadData *data = userdata;
    FBView *view = data->view;
    view->bgdir_thread = NULL;

    if (!cancel) {
        if (data->bgfileinfos) {
            FileInfo *file;
            LIST_FREE(view->bgfileinfos, file)
                fileinfo_destroy(file);
            view->bgfileinfos = data->bgfileinfos;
            view->bgfileinfos_idx = 0;
            nemotimer_set_timeout(view->bg_change_timer, 10);
        }
    } else {
        FileInfo *file;
        LIST_FREE(data->bgfileinfos, file) {
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
    List *bgfileinfos = NULL;
    if (view->bgpath_local) {
        snprintf(localpath, PATH_MAX, "%s/%s",
                view->curpath, view->bgpath_local);
        if (file_is_exist(localpath)) {
            if (file_is_dir(localpath)) {
                bgfileinfos = fileinfo_readdir_img(localpath);
            } else if (file_is_image(localpath)) {
                FileInfo *file;
                file = fileinfo_create
                    (false, realpath(localpath, NULL), basename(localpath));
                bgfileinfos = list_append(bgfileinfos, file);
            }
        }
    }

    if (!bgfileinfos && view->bgpath) {
        if (file_is_dir(view->bgpath)) {
            bgfileinfos = fileinfo_readdir_img(view->bgpath);
        } else if (file_is_image(view->bgpath)) {
            FileInfo *file;
            file = fileinfo_create
                (false, realpath(view->bgpath, NULL), basename(view->bgpath));
            bgfileinfos = list_append(bgfileinfos, file);
        }
    }

    if (!bgfileinfos) {
        ERR("There is no background image path in local(%s) or global(%s)",
                localpath, view->bgpath);
    }
    FileInfo *file;
    LIST_FREE(bgfileinfos, file) {
        if (fileinfo_is_image(file)) {
            data->bgfileinfos = list_append(data->bgfileinfos, file);
        }
    }
    if (!data->bgfileinfos) {
        ERR("No images in the background dir");
    }

    return NULL;
}

typedef struct _FileThreadData FileThreadData;
struct _FileThreadData {
    FBView *view;
    List *files;
};

static void *_fb_file_thread(void *userdata)
{
    FileThreadData *data = userdata;
    FBView *view = data->view;

    List *fileinfos = fileinfo_readdir_sorted(view->curpath);
    List *l;
    FileInfo *fileinfo;
    LIST_FOR_EACH(fileinfos, l, fileinfo) {
        if (fileinfo_is_dir(fileinfo)) {
            // TODO:
            continue;
        } else {
            FileType *ft;
            ft = _fileinfo_match_type(view->app->filetypes, fileinfo);
            if (!ft) {
                ERR("Not supported file type: %s", fileinfo->path);
                continue;
            }
            FBFile *file = fb_file_create(ft, fileinfo);
            data->files = list_append(data->files, file);
        }
    }

    if (view->app->sort_style) {
        if (!strcmp(view->app->sort_style, "filetype")) {
            List *typefiles = NULL;

            List *l;
            FileType *ft;
            LIST_FOR_EACH(view->app->filetypes, l, ft) {
                TypeFile *typefile = calloc(sizeof(TypeFile), 1);
                typefile->ft = ft;
                typefiles = list_append(typefiles, typefile);
            }

            List *files = data->files;
            data->files = NULL;

            FBFile *file;
            LIST_FREE(files, file) {
                List *l;
                TypeFile *typefile = NULL;
                LIST_FOR_EACH(typefiles, l, typefile) {
                    if (typefile->ft == file->ft) {
                       break;
                    }
                }
                if (typefile) {
                    typefile->files = list_append(typefile->files, file);
                } else {
                    ERR("No matched type for file: %s", file->path);
                    fb_file_destroy(file);
                }
            }

            TypeFile *typefile;
                LIST_FREE(typefiles, typefile) {
                FBFile *file;
                LIST_FREE(typefile->files, file) {
                    data->files = list_append(data->files, file);
                }
                free(typefile);
            }
        }
    }

    return NULL;
}

static void _fb_file_thread_done(bool cancel, void *userdata)
{
    FileThreadData *data = userdata;
    FBView *view = data->view;

    view->filethread = NULL;
    if (cancel) {
        FBFile *file;
        LIST_FREE(data->files, file) fb_file_destroy(file);
    } else {
        FBFile *file;
        INLIST_FREE(view->files, file) fb_file_destroy(file);

        LIST_FREE(data->files, file) {
            view->files = inlist_append(view->files, INLIST(file));
        }

        int cnt = inlist_count(view->files);
        if (cnt >= 16) {
            view->w = 745;
            view->h = 530;
        } else if (cnt >= 9) {
            view->w = 745;
            view->h = 435;
        } else {
            view->w = 745;
            view->h = 340;
        }

        // FIXME: nemowidget_resize scaling it's all content
        //nemowidget_resize(nemowidget_get_top_widget(view->widget), view->w, view->h);

        if (view->bgdir_thread) thread_destroy(view->bgdir_thread);
        BackThreadData *bgdata = calloc(sizeof(BackThreadData), 1);
        bgdata->view = view;
        view->bgdir_thread = thread_create(view->tool,
                _load_bg_dir, _load_bg_dir_done, bgdata);
        view_show(view);
    }
    free(data);
}

static void view_show_dir(FBView *view, const char *path)
{
    RET_IF(!path);
    ERR("%s", path);

    if (view->curpath) free(view->curpath);
    view->curpath = strdup(path);

    nemowidget_set_alpha(view->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 1.0);
    nemowidget_show(view->widget, 0, 0, 0);

    if (view->filethread) thread_destroy(view->filethread);
    FileThreadData *data = calloc(sizeof(FileThreadData), 1);
    data->view = view;
    view->filethread = thread_create(view->tool,
            _fb_file_thread, _fb_file_thread_done, data);

    if (view->app->titlepath) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%s/%s", view->curpath, view->app->titlepath);
        int line_len;
        char **line_txt = file_read_line(buf, &line_len);
        if (!line_txt || !line_txt[0] || line_len <= 0) {
            text_update(view->title, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, basename(view->rootpath));
        } else {
            text_update(view->title, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, line_txt[0]);
            if (line_txt[1]) {
                text_update(view->title1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, line_txt[1]);
            }
            int i;
            for (i = 0 ; i < line_len ; i++) free(line_txt[i]);
            free(line_txt);
        }
    } else {
        text_update(view->title, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, basename(view->rootpath));
    }

    text_set_alpha(view->title, 0, 0, 0, 1.0);
    text_translate(view->title, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            view->app->title_ltx, view->app->title_lty);

    double tw, th;
    text_get_size(view->title, &tw, &th);
    text_translate(view->title1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
            view->app->title_ltx, view->app->title_lty + th + 5);
    text_set_alpha(view->title1, 0, 0, 0, 1.0);

#if 0
    struct showone *one = RECT_CREATE(view->bg_group, tw + 1, th + 1);
    nemoshow_item_set_fill_color(one, RGBA(RED));
    nemoshow_item_set_alpha(one, 0.5);
    nemoshow_item_translate(one, view->app->title_ltx, view->app->title_lty);
#endif

    nemoshow_dispatch_frame(view->show);
}

static void view_hide(FBView *view)
{
    nemowidget_set_alpha(view->widget, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    nemowidget_hide(view->widget, 0, 0, 0);

    if (view->filethread) thread_destroy(view->filethread);
    image_set_alpha(view->bg0, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    image_set_alpha(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);

    text_set_alpha(view->title, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    text_set_alpha(view->title1, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);

    if (view->bgdir_thread) thread_destroy(view->bgdir_thread);
    nemotimer_set_timeout(view->bg_change_timer, 0);

    FBItem *it;
    LIST_FREE(view->items, it) item_hide(it, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    nemoshow_dispatch_frame(view->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    FBView *view = userdata;

    view_hide(view);

    nemowidget_win_exit_after(win, 500);
}

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);
    app->title_ltx = 45;
    app->title_lty = 35;
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
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

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
    temp = xml_get_value(xml, buf, "titlepath");
    if (temp && strlen(temp) > 0) {
        app->titlepath = strdup(temp);
    }

    snprintf(buf, PATH_MAX, "%s/sort", prefix);
    temp = xml_get_value(xml, buf, "style");
    if (temp && strlen(temp) > 0) {
        app->sort_style = strdup(temp);
    }

    snprintf(buf, PATH_MAX, "%s/title", prefix);
    temp = xml_get_value(xml, buf, "ltx");
    if (temp && strlen(temp) > 0) {
        app->title_ltx = atoi(temp);
    }
    temp = xml_get_value(xml, buf, "lty");
    if (temp && strlen(temp) > 0) {
        app->title_lty = atoi(temp);
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
    if (app->sort_style) free(app->sort_style);
    if (app->titlepath) free(app->titlepath);
    free(app->rootpath);

    FileType *ft;
    LIST_FREE(app->filetypes, ft) filetype_destroy(ft);

    free(app);
}

int main(int argc, char *argv[])
{
    xemoapp_init();
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->rootpath) {
        app->rootpath = strdup(file_get_homedir());
    }
    pos_init(app->pos);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);

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
    xemoapp_shutdown();

    return 0;
}
