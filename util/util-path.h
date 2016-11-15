#include <nemowrapper.h>

#ifndef __UTIL_PATH_H__
#define __UTIL_PATH_H__

typedef struct _PathItem PathItem;
struct _PathItem {
    char type;
    double rxy[2]; // temporary
    double xy[6];
    int cnt;
};

typedef struct _Path Path;
struct _Path {
    char *uri;
    int x, y, width, height;
    bool is_fill;
    uint32_t fill;
    bool is_stroke;
    uint32_t stroke;
    int stroke_width;

    List *items;
    char *d;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline void path_destroy(Path *path)
{
    PathItem *it;
    LIST_FREE(path->items, it) free(it);
    if (path->d) free(path->d);
    free(path);
}

static inline struct showone *path_draw(Path *path, struct showone *group)
{
    struct showone *one;
    one = PATH_CREATE(group);
    nemoshow_item_path_cmd(one, path->d);
    nemoshow_item_set_width(one, path->width);
    nemoshow_item_set_height(one, path->height);
    if (path->is_fill)
        nemoshow_item_set_fill_color(one, RGBA(path->fill));
    if (path->is_stroke) {
        nemoshow_item_set_stroke_color(one, RGBA(path->stroke));
        nemoshow_item_set_stroke_width(one, path->stroke_width);
    }
    return one;
}

static inline struct showone *path_draw_array(Path *path, struct showone *group)
{
    struct showone *one;
    one = PATH_ARRAY_CREATE(group);
    nemoshow_item_set_width(one, path->width);
    nemoshow_item_set_height(one, path->height);

    List *l;
    PathItem *it;
    LIST_FOR_EACH(path->items, l, it) {
        /*
        ERR("%c[%d], %0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf",
                it->type, it->cnt, it->xy[0], it->xy[1], it->xy[2], it->xy[3], it->xy[4], it->xy[5]);
                */
        if (it->type == 'M') {
            nemoshow_item_path_moveto(one, it->xy[0], it->xy[1]);
        } else if (it->type == 'C') {
            nemoshow_item_path_cubicto(one,
                    it->xy[0], it->xy[1], it->xy[2], it->xy[3],
                    it->xy[4], it->xy[5]);
        } else if (it->type == 'L') {
            nemoshow_item_path_lineto(one, it->xy[0], it->xy[1]);
        } else if (it->type == 'Z') {
            nemoshow_item_path_close(one);
        } else {
            ERR("Not supported type: %c[%d] %0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf",
                    it->type, it->cnt, it->xy[0], it->xy[1], it->xy[2], it->xy[3], it->xy[4], it->xy[5]);
        }
        //if (list_get_idx(path->items, it) == 3) break;
    }

    if (path->is_fill)
        nemoshow_item_set_fill_color(one, RGBA(path->fill));
    if (path->is_stroke) {
        nemoshow_item_set_stroke_color(one, RGBA(path->stroke));
        nemoshow_item_set_stroke_width(one, path->stroke_width);
    }
    return one;
}

static inline void path_array_morph(struct showone *one, uint32_t easetype, int duration, int delay, Path *path)
{
    nemoshow_item_set_width(one, path->width);
    nemoshow_item_set_height(one, path->height);

    // XXX: check match??
    if (duration > 0) {
        NemoMotion *m;
        m = nemomotion_create(one->show, easetype, duration, delay);

        int i = 0;
        List *l;
        PathItem *it;
        LIST_FOR_EACH(path->items, l, it)  {
            char buf0[PATH_MAX], buf1[PATH_MAX], buf2[PATH_MAX], buf3[PATH_MAX], buf4[PATH_MAX], buf5[PATH_MAX];
            snprintf(buf0, PATH_MAX, "points:%d", i);
            snprintf(buf1, PATH_MAX, "points:%d", i+1);
            snprintf(buf2, PATH_MAX, "points:%d", i+2);
            snprintf(buf3, PATH_MAX, "points:%d", i+3);
            snprintf(buf4, PATH_MAX, "points:%d", i+4);
            snprintf(buf5, PATH_MAX, "points:%d", i+5);
            nemomotion_attach(m, 1.0,
                    one, buf0, it->xy[0],
                    one, buf1, it->xy[1],
                    one, buf2, it->xy[2],
                    one, buf3, it->xy[3],
                    one, buf4, it->xy[4],
                    one, buf5, it->xy[5],
                    NULL);
            i+=6;
        }
        if (path->is_fill)
            nemomotion_attach(m , 1.0, one, "fill", path->fill, NULL);
        if (path->is_stroke) {
            nemomotion_attach(m , 1.0,
                    one, "stroke", path->stroke,
                    one, "stroke-width", (double)path->stroke_width,
                    NULL);
        }
        nemomotion_run(m);
    } else {
        nemoshow_item_path_clear(one);
        List *l;
        PathItem *it;
        LIST_FOR_EACH(path->items, l, it) {
            if (it->type == 'M') {
                nemoshow_item_path_moveto(one, it->xy[0], it->xy[1]);
            } else if (it->type == 'C') {
                nemoshow_item_path_cubicto(one,
                        it->xy[0], it->xy[1], it->xy[2], it->xy[3],
                        it->xy[4], it->xy[5]);
            } else if (it->type == 'L') {
                nemoshow_item_path_lineto(one, it->xy[0], it->xy[1]);
            } else if (it->type == 'Z') {
                nemoshow_item_path_close(one);
            } else {
                ERR("Not supported type: %c, %0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf,%0.1lf",
                        it->type, it->xy[0], it->xy[1], it->xy[2], it->xy[3], it->xy[4], it->xy[5]);
            }
        }

        if (path->is_fill)
            nemoshow_item_set_fill_color(one, RGBA(path->fill));
        if (path->is_stroke) {
            nemoshow_item_set_stroke_color(one, RGBA(path->stroke));
            nemoshow_item_set_stroke_width(one, path->stroke_width);
        }
    }
}

static inline void path_debug(Path *path, struct showone *group, uint32_t color)
{
    struct showone *font;
    font = FONT_CREATE("NanumGothic", "Regular");

    ERR("0x%x, 0x%x, %d", path->fill, path->stroke, path->stroke_width);
    struct showone *one;
    int j = 0;
    List *ll;
    PathItem *it;
    LIST_FOR_EACH(path->items, ll, it) {
        if (it->type == 'Z') break;
        int i;
        for (i = 0 ; i < it->cnt ; i+=2) {
            char ttt[32];
            snprintf(ttt, 32, "[%d], %.0lf,%.0lf", j++, it->xy[i], it->xy[i+1]);
            one = CIRCLE_CREATE(group, 2);
            nemoshow_item_set_fill_color(one, RGBA(color));
                nemoshow_item_translate(one,
                        it->xy[i], it->xy[i+1]);

            one = TEXT_CREATE(group, font, 15, ttt);
            nemoshow_item_set_fill_color(one, RGBA(color));
                nemoshow_item_translate(one,
                        it->xy[i], it->xy[i+1]);
                ERR("[%d](%c) %.2lf %.2lf (%.2lf, %.2lf, %.2lf %.2lf)",
                        j, it->type,
                        it->xy[i], it->xy[i +1],
                        it->rxy[0], it->rxy[1], it->xy[i], it->xy[i + 1]);
        }
    }
}

typedef struct _PathCoord PathCoord;
struct _PathCoord
{
    double x;
    double y;
    double z;
};

static int
factorial(int n)
{
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

static inline PathCoord bezier(PathCoord *p, int n, double u)
{
    // FIXME: Use pascal's traingle to calculate n combination of k
    // to reduce calculation costs.
    int path_pascal[][11] = {
        {1},
        {1, 1},
        {1, 2, 1},
        {1, 3, 3, 1},
        {1, 4, 6, 4, 1},
        {1, 5, 10, 10, 5, 1},
        {1, 6, 15, 20, 15, 6, 1},
        {1, 7, 21, 35, 35, 21, 7, 1},
        {1, 8, 28, 56, 70, 56, 28, 8, 1},
        {1, 9, 36, 84, 126, 126, 84, 36, 9, 1},
        {1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}
    };

    PathCoord ret = {0, 0, 0};
    int k = 0;

    n--;
    int len = sizeof(path_pascal)/sizeof(path_pascal[0]);
    for (k = 0 ; k <= n ; k++) {
        double factor;
        if (n <= len) {
            factor = path_pascal[n][k];
        } else {
            factor = (factorial(n)/(double)(factorial(k) * factorial(n-k)));
        }

        double blend = factor * pow(u, k) * pow(1-u, n-k);
        ret.x += p[k].x * blend;
        ret.y += p[k].y * blend;
        ret.z += p[k].z * blend;
    }
    return ret;
}

static inline PathCoord quad_bezier(PathCoord p0, PathCoord p1, PathCoord p2, double u)
{
    PathCoord ret;
    double u_2 = u * u;
    double uu = 1 - u;
    double uu_2 = uu * uu;
    ret.x = p0.x * uu_2 + p1.x * 2 * u * uu + p2.x * u_2;
    ret.y = p0.y * uu_2 + p1.y * 2 * u * uu + p2.y * u_2;
    ret.z = p0.z * uu_2 + p1.z * 2 * u * uu + p2.z * u_2;
    return ret;
}

static inline bool _check_match(Path *path0, Path *path1)
{
    //ERR("%s %s", path0->uri, path1->uri);
    if (list_count(path0->items) != list_count(path1->items)) {
        ERR("counte is different %s(%d), %s(%d)",
                path0->uri, list_count(path0->items),
                path1->uri, list_count(path1->items));
        return false;
    }

    List *l;
    PathItem *it0;
    LIST_FOR_EACH(path0->items, l, it0) {
        int idx = list_get_idx(path0->items, it0);
        PathItem *it1 = (PathItem *)LIST_DATA(list_get_nth(path1->items, idx));

        //ERR("%c = %c, %d = %d", it0->type, it1->type,it0->cnt, it1->cnt);
        if (it0->type != it1->type || it0->cnt != it1->cnt) {
            ERR("paths are not matched (%c != %c or %d != %d): %s, %s",
                    it0->type, it1->type, it0->cnt, it1->cnt,
                    path0->uri, path1->uri);
            return false;
        }
    }
    return true;
}

static inline void _calculate_externs(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1, int *x, int *y, int *w, int *h)
{
    int lx, ly, rx, ry;
    if (x0 < x1) lx = x0;
    else lx = x1;
    if (y0 < y1) ly = y0;
    else ly = y1;
    if (x0 + w0 > x1 + w1) rx = x0 + w0;
    else rx = x1 + w1;
    if (y0 + h0 > y1 + h1) ry = y0 + h0;
    else ry = y1 + h1;

    if (x) *x = lx;
    if (y) *y = ly;
    if (w) *w = rx - lx;
    if (h) *h = ry - ly;
}

static inline int _get_median_int(int a, int b, int i, int cnt)
{
    return a + (double)(b - a)/cnt * i;
}

static inline uint32_t _get_median_uint32(uint32_t a, uint32_t b, int i, int cnt)
{
    return a + ((int)(b - a))/cnt * i;
}

static inline double _get_median_double(double a, double b, int i, int cnt)
{
    return a + (double)(b - a)/cnt * i;
}

static inline void _get_median_coord(double x0, double y0, double x1, double y1,
        double *x, double *y,
        int i, int cnt)
{
    if (x) *x = _get_median_double(x0, x1, i, cnt);
    if (y) *y = _get_median_double(y0, y1, i, cnt);

}

static inline uint32_t _get_median_color(uint32_t color0, uint32_t color1, int i, int cnt)
{
    uint32_t R = _get_median_uint32((color0 & 0xFF000000) >> 24, (color1 & 0xFF000000) >> 24, i, cnt);
    uint32_t G = _get_median_uint32((color0 & 0xFF0000) >> 16, (color1 & 0xFF0000) >> 16, i, cnt);
    uint32_t B = _get_median_uint32((color0 & 0xFF00) >> 8, (color1 & 0xFF00) >> 8, i, cnt);
    uint32_t A = _get_median_uint32(color0 & 0xFF, color1 & 0xFF, i, cnt);
    //ERR("%x %x %x", R, (color0 & 0xFF000000) >> 24, (color1 & 0xFF000000) >> 24);
    return RGBA2UINT(R, G, B, A);
}

static inline List *path_get_median(Path *path0, Path *path1, int cnt)
{
    RET_IF(!_check_match(path0, path1), NULL);

    List *paths = NULL;

    int i;
    for (i = 1 ; i <= cnt ; i++) {
        Path *path = (Path *)calloc(sizeof(Path), 1);
        _calculate_externs(path0->x, path0->y, path0->width, path0->height,
                path1->x, path1->y, path1->width, path1->height,
                &path->x, &path->y, &path->width, &path->height);
        if (path0->is_fill && path1->is_fill) {
            path->is_fill = true;
            path->fill = _get_median_color(path0->fill, path1->fill, i, cnt + 1);
        }
        if (path0->is_stroke && path1->is_stroke) {
            path->is_stroke = true;
            path->stroke = _get_median_color(path0->stroke, path1->stroke, i, cnt + 1);
            path->stroke_width = _get_median_int(path0->stroke_width, path1->stroke_width,
                    i, cnt + 1);
        }

        char buf[4096];
        memset(buf, 0, 4096);
        List *l;
        PathItem *it0;
        LIST_FOR_EACH(path0->items, l, it0) {
            char it_buf[128];
            int idx = list_get_idx(path0->items, it0);
            PathItem *it1 = (PathItem *)LIST_DATA(list_get_nth(path1->items, idx));

            PathItem *it = (PathItem *)calloc(sizeof(PathItem), 1);
            it->type = it0->type;
            it->cnt = it0->cnt;

            snprintf(it_buf, 128, "%c", it->type);

            int j;
            for (j = 0 ; j < it->cnt ; j+=2) {
                _get_median_coord(it0->xy[j], it0->xy[j+1], it1->xy[j], it1->xy[j+1],
                        &it->xy[j], &it->xy[j+1],
                        i, cnt + 1);

                char _buf[32];
                snprintf(_buf, 32, "%lf,%lf", it->xy[j], it->xy[j+1]);
                if (j != 0) {
                    strcat(it_buf, ",");
                }
                strcat(it_buf, _buf);
                //ERR("[%d][%d]%lf, %lf, %lf", i, j, it->xy[j], it0->xy[j], it1->xy[j]);
            }
            strcat(buf, it_buf);
            path->items = list_append(path->items, it);
        }
        path->d = strdup(buf);
        paths = list_append(paths, path);
    }

    return paths;
}

// XXX:
static inline Path *path_get_near(Path *path0, Path *path1)
{
    Path *path = (Path *)calloc(sizeof(Path), 1);
    path->x = path0->x;
    path->y = path0->y;
    path->width = path0->width;
    path->height = path0->height;
    path->is_fill = path0->is_fill;
    path->fill = path0->fill;
    path->is_stroke = path0->is_stroke;
    path->stroke = path0->stroke;
    path->stroke_width = path0->stroke_width;

    List *l;
    PathItem *it0;
    LIST_FOR_EACH(path0->items, l, it0) {
        PathItem *it = (PathItem *)calloc(sizeof(PathItem), 1);
        it->type = it0->type;
        it->rxy[0] = it0->rxy[0];
        it->rxy[1] = it0->rxy[1];
        it->cnt = it0->cnt;
        path->items = list_append(path->items, it);

        int i;
        for (i = 0 ; i < it0->cnt ; i+=2) {
            double dist = 999999;
            int tj = 0;
            PathItem *tit = NULL;

            List *ll;
            PathItem *it1;
            LIST_FOR_EACH(path1->items, ll, it1) {
                int j;
                for (j = 0 ; j < it1->cnt ; j+=2) {
                    double _dist = DISTANCE(it0->xy[i], it0->xy[i+1], it1->xy[j], it1->xy[j+1]);
                    ERR("[%d]%lf, %lf ? [%d]%lf, %lf",
                            i, it0->xy[i], it0->xy[i+1], j, it1->xy[j], it1->xy[j+1]);
                    if (dist > _dist) {
                        _dist = dist;
                        tj = j;
                        tit = it1;
                    }
                }
            }
            it->xy[i] = tit->xy[tj];
            it->xy[i + 1] = tit->xy[tj + 1];
            ERR("[%d] %lf %lf", i, it->xy[i], it->xy[i+1]);
        }

    }

    return path;
}

static inline List *svg_parse_items(const char *str)
{
    List *items = NULL;

    char *_temp = strdup(str);
    char *t = _temp;
    PathItem *it = NULL, *pit = NULL;

    char buf[32];
    int buf_cnt = 0;
    buf[buf_cnt] = '\0';
    while (*t && *t != '\0') {
        if ('A' <= *t && *t <= 'Z') {
            if ((strlen(buf) > 0) && (buf_cnt > 0) && it) {
                buf[buf_cnt] = '\0';
                buf_cnt = 0;

                if (it->cnt%2 == 0)
                    it->xy[it->cnt] = it->rxy[0] + atof(buf);
                else
                    it->xy[it->cnt] = it->rxy[1] + atof(buf);
                it->cnt++;
                if (it->type == 'S') {
                    if (it->cnt != 4) {
                        ERR("path item with 'S' tag cannot have %d count", it->cnt);
                    }
                    it->type = 'C';
                    it->cnt = 6;
                    if (pit && pit->type == 'C') {
                        it->xy[4] = it->xy[2];
                        it->xy[5] = it->xy[3];
                        it->xy[2] = it->xy[0];
                        it->xy[3] = it->xy[1];
                        it->xy[0] = 2 * pit->xy[4] - pit->xy[2];
                        it->xy[1] = 2 * pit->xy[5] - pit->xy[3];
                    } else {
                        it->xy[4] = it->xy[2];
                        it->xy[5] = it->xy[3];
                        it->xy[2] = it->xy[0];
                        it->xy[3] = it->xy[1];
                    }
                } else if (it->type == 'H' || it->type == 'V') {
                    if (it->cnt != 1) {
                        ERR("path item with 'H'or 'V' tag cannot have %d count", it->cnt);
                    }
                    it->type = 'L';
                    it->cnt = 2;
                    if (it->type == 'H') {
                        it->xy[1] = it->xy[0];
                        if (pit->type == 'C') {
                            it->xy[0] = pit->xy[4];
                        } else {
                            it->xy[0] = pit->xy[0];
                        }
                    } else {
                        if (pit->type == 'C') {
                            it->xy[1] = pit->xy[5];
                        } else {
                            it->xy[1] = pit->xy[1];
                        }
                    }
                }
                items = list_append(items, it);
                pit = it;
                /*
                   ERR("%c[%d] (%0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf)",
                   it->type, it->cnt,
                   it->xy[0], it->xy[1], it->xy[2], it->xy[3],
                        it->xy[4], it->xy[5]);
                        */
            }

            if (*t == 'Z') break;
            it = (PathItem *)malloc(sizeof(PathItem));
            it->type = *t;
            it->rxy[0] = 0;
            it->rxy[1] = 0;
            it->cnt = 0;
        } else if ('a' <= *t && *t <= 'z') {
            if ((strlen(buf) > 0) && (buf_cnt > 0) && it) {
                buf[buf_cnt] = '\0';
                buf_cnt = 0;

                if (it->cnt%2 == 0)
                    it->xy[it->cnt] = it->rxy[0] + atof(buf);
                else
                    it->xy[it->cnt] = it->rxy[1] + atof(buf);
                it->cnt++;
                if (it->type == 'S') {
                    if (it->cnt != 4) {
                        ERR("path item with 'S' tag cannot have %d count", it->cnt);
                    }
                    it->type = 'C';
                    it->cnt = 6;
                    if (pit && pit->type == 'C') {
                        it->xy[4] = it->xy[2];
                        it->xy[5] = it->xy[3];
                        it->xy[2] = it->xy[0];
                        it->xy[3] = it->xy[1];
                        it->xy[0] = 2 * pit->xy[4] - pit->xy[2];
                        it->xy[1] = 2 * pit->xy[5] - pit->xy[3];
                    } else {
                        it->xy[4] = it->xy[2];
                        it->xy[5] = it->xy[3];
                        it->xy[2] = it->xy[0];
                        it->xy[3] = it->xy[1];
                    }
                } else if (it->type == 'H' || it->type == 'V') {
                    if (it->cnt != 1) {
                        ERR("path item with 'H'or 'V' tag cannot have %d count", it->cnt);
                    }
                    it->type = 'L';
                    it->cnt = 2;
                    if (it->type == 'H') {
                        it->xy[1] = it->xy[0];
                        if (pit->type == 'C') {
                            it->xy[0] = pit->xy[4];
                        } else {
                            it->xy[0] = pit->xy[0];
                        }
                    } else {
                        if (pit->type == 'C') {
                            it->xy[1] = pit->xy[5];
                        } else {
                            it->xy[1] = pit->xy[1];
                        }
                    }
                }
                items = list_append(items, it);
                pit = it;
                /*
                ERR("%c[%d] (%0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf)",
                        it->type, it->cnt,
                        it->xy[0], it->xy[1], it->xy[2], it->xy[3],
                        it->xy[4], it->xy[5]);
                        */
            }

            if (*t == 'z') break;
            it = (PathItem *)malloc(sizeof(PathItem));
            it->type = *t - 32;
            it->rxy[0] = pit->xy[pit->cnt - 2];
            it->rxy[1] = pit->xy[pit->cnt - 1];
            it->cnt = 0;
        } else if (*t == ',') {
            if (strlen(buf) <= 0 || buf_cnt <= 0 || !it) {
                ERR("Wrong token in the path: %c", *t);
                break;
            }
            buf[buf_cnt] = '\0';
            buf_cnt = 0;

            if (it->cnt%2 == 0)
                it->xy[it->cnt] = it->rxy[0] + atof(buf);
            else
                it->xy[it->cnt] = it->rxy[1] + atof(buf);
            it->cnt++;
        } else if (*t == '-') {
            if (!it) {
                ERR("Wrong token in the path: %c", *t);
                break;
            }

            if (strlen(buf) > 0 && buf_cnt > 0) {
                buf[buf_cnt] = '\0';
                buf_cnt = 0;

                if (it->cnt%2 == 0)
                    it->xy[it->cnt] = it->rxy[0] + atof(buf);
                else
                    it->xy[it->cnt] = it->rxy[1] + atof(buf);
                it->cnt++;
            }

            buf[buf_cnt] = *t;
            buf_cnt++;
        } else if ('0' <= *t || *t <= '9' || *t == '.') {
            buf[buf_cnt] = *t;
            buf_cnt++;
        }
        t++;
    }
    // If no close tag,
    if ((strlen(buf) > 0) && (buf_cnt > 0) && it) {
        buf[buf_cnt] = '\0';
        buf_cnt = 0;

        if (it->cnt%2 == 0)
            it->xy[it->cnt] = it->rxy[0] + atof(buf);
        else
            it->xy[it->cnt] = it->rxy[1] + atof(buf);
        it->cnt++;
        if (it->type == 'S') {
            if (it->cnt != 4) {
                ERR("path item with 'S' tag cannot have %d count", it->cnt);
            }
            it->type = 'C';
            it->cnt = 6;
            if (pit && pit->type == 'C') {
                it->xy[4] = it->xy[2];
                it->xy[5] = it->xy[3];
                it->xy[2] = it->xy[0];
                it->xy[3] = it->xy[1];
                it->xy[0] = 2 * pit->xy[4] - pit->xy[2];
                it->xy[1] = 2 * pit->xy[5] - pit->xy[3];
            } else {
                it->xy[4] = it->xy[2];
                it->xy[5] = it->xy[3];
                it->xy[2] = it->xy[0];
                it->xy[3] = it->xy[1];
            }
        } else if (it->type == 'H' || it->type == 'V') {
            if (it->cnt != 1) {
                ERR("path item with 'H'or 'V' tag cannot have %d count", it->cnt);
            }
            it->type = 'L';
            it->cnt = 2;
            if (it->type == 'H') {
                it->xy[1] = it->xy[0];
                if (pit->type == 'C') {
                    it->xy[0] = pit->xy[4];
                } else {
                    it->xy[0] = pit->xy[0];
                }
            } else {
                if (pit->type == 'C') {
                    it->xy[1] = pit->xy[5];
                } else {
                    it->xy[1] = pit->xy[1];
                }
            }
        }
        items = list_append(items, it);
        /*
           ERR("%c[%d] (%0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf, %0.1lf)",
           it->type, it->cnt,
           it->xy[0], it->xy[1], it->xy[2], it->xy[3],
           it->xy[4], it->xy[5]);
           */
    }
    free(_temp);

#if 0
    ERR("(%d)%s", list_count(items), str);
    LIST_FOR_EACH(items, l, it) {
        ERR("[%c,%d]%lf,%lf,%lf,%lf,%lf,%lf", it->type, it->cnt,
                it->xy[0], it->xy[1], it->xy[2], it->xy[3],
                it->xy[4], it->xy[5]);
    }
    ERR("========================================================================");
#endif
    return items;
}

static inline uint32_t svg_parse_color(const char *str)
{
    uint32_t color = 0;
    char *_temp = strdup(str);
    char *t = _temp;
    if (*t == '#') {
        t++;
        while (*t && *t != '\0') {
            if (color != 0) color = color << 4;
            if ('0' <= *t && *t <= '9') {
                color += *t - '0';
            } else if ('A' <= *t && *t <= 'Z') {
                color += *t - 'A' + 10;
            } else if ('a' <= *t && *t <= 'z') {
                color += *t - 'a' + 10;
            }
            t++;
        }
    }
    free(_temp);

    color = color << 8 | 0xFF;

    return color;
}

static inline Path *svg_get_path(const char *uri)
{
    if (!file_is_exist(uri) || file_is_null(uri)) {
        ERR("file does not exist or file is NULL: %s", uri);
        return NULL;
    }
    size_t size;
    char *data;
    data = file_load(uri, &size);
    if (!data) {
        ERR("file has no data: %s", uri);
        return NULL;
    }
    Xml *xml = xml_load(data, size);
    if (!xml) {
        ERR("Something wrong with xml file: %s", uri);
        free(data);
        return NULL;
    }
    free(data);

    Path *path = (Path *)calloc(sizeof(Path), 1);
    path->uri = strdup(uri);

    const char *temp;
    temp = svg_get_value(xml, "/svg", "x");
    if (temp) {
        path->x = atoi(temp);
    }
    temp = svg_get_value(xml, "/svg", "y");
    if (temp) {
        path->y = atoi(temp);
    }
    temp = svg_get_value(xml, "/svg", "width");
    if (temp) {
        path->width = atoi(temp);
    }
    temp = svg_get_value(xml, "/svg", "height");
    if (temp) {
        path->height = atoi(temp);
    }

    temp = svg_get_value(xml, "/svg/path", "d");
    if (temp) {
        path->d = strdup(temp);
        path->items = svg_parse_items(temp);
    }

    temp = svg_get_value(xml, "/svg/path", "fill");
    if (temp && strcmp(temp, "none")) {
        path->is_fill = true;
        path->fill = svg_parse_color(temp);
    }
    temp = svg_get_value(xml, "/svg/path", "stroke");
    if (temp && strcmp(temp, "none")) {
        path->is_stroke = true;
        path->stroke = svg_parse_color(temp);
    }
    temp = svg_get_value(xml, "/svg/path", "stroke-width");
    if (temp) {
        path->stroke_width = atoi(temp);
    }

    xml_unload(xml);
    return path;
}

#ifdef __cplusplus
}
#endif

#endif
