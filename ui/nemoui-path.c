#include "nemowrapper.h"
#include "nemoui-path.h"

struct showone *path_draw(Path *path, struct showone *group)
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

struct showone *path_draw_array(Path *path, struct showone *group)
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

void path_array_morph(struct showone *one, uint32_t easetype, int duration, int delay, Path *path)
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

void path_debug(Path *path, struct showone *group, uint32_t color)
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
