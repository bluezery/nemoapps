#ifndef __NEMOUI_GRAPH__
#define __NEMOUI_GRAPH__

#include <nemotool.h>
#include <nemoshow.h>
#include <nemomisc.h>
#include <nemocanvas.h>

#include "nemoutil.h"
#include "nemowrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/* GraphBar */
/*************************************************/
typedef enum {
    GRAPH_BAR_DIR_T = 0,
    GRAPH_BAR_DIR_B,
    GRAPH_BAR_DIR_R,
    GRAPH_BAR_DIR_L,
    GRAPH_BAR_DIR_MAX
} GraphBarDir;

typedef struct _GraphBar GraphBar;
struct _GraphBar {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int w, h;

    struct {
        struct showone *one;
        int r;
    } blur;
    struct showone *group;

    GraphBarDir dir;

    int gap; // pixel
    List *items;
};

typedef struct _GraphBarItem GraphBarItem;
struct _GraphBarItem {
    GraphBar *bar;
    int w, h;
    struct showone *group;
    struct showone *one;
    bool dirty;

    struct {
        uint32_t base;
        uint32_t active;
    } color;

    double percent;
};

static inline void graph_bar_get_geometry(GraphBar *bar, int *x, int *y, int *w, int *h)
{
    int xx = nemoshow_item_get_translate_x(bar->group);
    int yy = nemoshow_item_get_translate_y(bar->group);
    if (x) *x = xx;
    if (y) *y = yy;
    if (w) *w = bar->w;
    if (h) *h = bar->h;
}

static inline GraphBar *graph_bar_create(struct nemotool *tool, struct showone *parent, int w, int h, GraphBarDir dir)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    if (dir < GRAPH_BAR_DIR_T ||
            dir >= GRAPH_BAR_DIR_MAX) {
        ERR("direction is not correct: %d", dir);
        return NULL;
    }

    GraphBar *bar = (GraphBar *)calloc(sizeof(GraphBar), 1);
    bar->tool = tool;
    bar->show = parent->show;
    bar->parent = parent;
    bar->w = w;
    bar->h = h;
    bar->group = GROUP_CREATE(parent);
    bar->dir = dir;

    return bar;
}

static inline void graph_bar_set_blur(GraphBar *bar, const char *style, int r)
{
    if (bar->blur.one) {
        nemoshow_one_destroy(bar->blur.one);
        bar->blur.one = NULL;
    }

    if (!style || r <= 0) return;

    bar->blur.one = BLUR_CREATE(style, r);
    bar->blur.r = r;

    GraphBarItem *it;
    List *l;
    LIST_FOR_EACH(bar->items, l, it) {
        nemoshow_item_set_filter(it->one, bar->blur.one);
    }
}

static inline void graph_bar_set_gap(GraphBar *bar, int gap)
{
    RET_IF(!bar);
    bar->gap = gap;
}

static inline void graph_bar_item_enable_event(GraphBarItem *it, uint32_t tag)
{
    nemoshow_one_set_state(it->one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_tag(it->one, tag);
}

static inline GraphBarItem *graph_bar_create_item(GraphBar *bar)
{
    GraphBarItem *it = (GraphBarItem *)calloc(sizeof(GraphBarItem), 1);
    it->bar = bar;

    struct showone *group;
    it->group = group = GROUP_CREATE(bar->group);

    struct showone *one;
    one = RRECT_CREATE(it->group, 0, 0, 4, 4);
    nemoshow_item_set_fill_color(one, RGBA(0xFFFFFF));
    nemoshow_item_set_alpha(one, 0.0);
    if (bar->dir == GRAPH_BAR_DIR_T) {
        nemoshow_item_translate(one, 0, bar->h);
        nemoshow_item_set_width(one, bar->w);
        nemoshow_item_set_height(one, 0);
    } else if (bar->dir == GRAPH_BAR_DIR_B) {
        nemoshow_item_translate(one, 0, 0);
        nemoshow_item_set_width(one, bar->w);
        nemoshow_item_set_height(one, 0);
    } else if (bar->dir == GRAPH_BAR_DIR_R) {
        nemoshow_item_translate(one, 0, 0);
        nemoshow_item_set_width(one, 0);
        nemoshow_item_set_height(one, bar->h);
    } else if (bar->dir == GRAPH_BAR_DIR_L) {
        nemoshow_item_translate(one, bar->w, 0);
        nemoshow_item_set_width(one, 0);
        nemoshow_item_set_height(one, bar->h);
    }
    if (bar->blur.one) nemoshow_item_set_filter(one, bar->blur.one);
    it->one = one;

    bar->items = list_append(bar->items, it);

    return it;
}

static inline void graph_bar_item_destroy(GraphBarItem *it)
{
    RET_IF(!it);
    RET_IF(!it->bar);

    it->bar->items = list_remove(it->bar->items, it);
    nemoshow_one_destroy(it->group);
    free(it);
}

static inline void graph_bar_item_set_color(GraphBarItem *it, uint32_t color_base, uint32_t color_active)
{
    it->color.base = color_base;
    it->color.active = color_active;
    nemoshow_item_set_fill_color(it->one, RGBA(it->color.base));
}

static inline void graph_bar_item_set_percent(GraphBarItem *it, double percent)
{
    RET_IF(!it);

    it->percent = percent;
    it->dirty = true;
}

static inline void graph_bar_update(GraphBar *bar, uint32_t easetype, int duration, int delay)
{
    double tot = 0;
    if (bar->dir == GRAPH_BAR_DIR_T || bar->dir == GRAPH_BAR_DIR_B) {
        tot = bar->h - (list_count(bar->items) - 1) * bar->gap;
    } else if (bar->dir == GRAPH_BAR_DIR_R || bar->dir == GRAPH_BAR_DIR_L) {
        tot = bar->w - (list_count(bar->items) - 1) * bar->gap;
    } else {
        ERR("Invalid direction: %d", bar->dir);
    }

    List *l;
    GraphBarItem *it;
    double adv = 0;
    double val;
    if (duration > 0) {
        LIST_FOR_EACH(bar->items, l, it) {
            val = tot * it->percent;
            if (bar->dir == GRAPH_BAR_DIR_T) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     0.0,
                        "ty",     (double)(bar->h - val) - adv,
                        "width",  (double)bar->w,
                        "height", (double)val,
                        NULL);
            } else if (bar->dir == GRAPH_BAR_DIR_B) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     0.0,
                        "ty",     (double)adv,
                        "width",  (double)bar->w,
                        "height", (double)val,
                        NULL);
            } else if (bar->dir == GRAPH_BAR_DIR_R) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     (double)adv,
                        "ty",     0.0,
                        "width",  (double)val,
                        "height", (double)bar->h,
                        NULL
                        );
            } else if (bar->dir == GRAPH_BAR_DIR_L) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     (double)(bar->w - val) - adv,
                        "ty",     0.0,
                        "width",  (double)val,
                        "height", (double)bar->h,
                        NULL
                        );
            }
            adv += val + bar->gap;

            if (it->dirty) {
                NemoMotion *m = nemomotion_create(bar->show, easetype, duration, delay);
                nemomotion_attach(m, 0.9, it->one, "fill", it->color.active, NULL);
                nemomotion_attach(m, 1.0, it->one, "fill", it->color.base, NULL);
                nemomotion_run(m);

                if (bar->blur.one) {
                    NemoMotion *m = nemomotion_create(bar->show, easetype, duration, delay);
                    nemomotion_attach(m, 0.5, bar->blur.one, "r", 0.0, NULL);
                    nemomotion_attach(m, 1.0, bar->blur.one, "r", (double)bar->blur.r, NULL);
                    nemomotion_run(m);
                }
                it->dirty = false;
            }
        }
    } else {
        LIST_FOR_EACH(bar->items, l, it) {
            val = tot * it->percent;
            if (bar->dir == GRAPH_BAR_DIR_T) {
                nemoshow_item_translate(it->one, 0.0, (double)(bar->h - val) - adv);
                nemoshow_item_set_width(it->one, (double)bar->w);
                nemoshow_item_set_height(it->one, (double)val);
            } else if (bar->dir == GRAPH_BAR_DIR_B) {
                nemoshow_item_translate(it->one, 0.0, (double)adv);
                nemoshow_item_set_width(it->one, (double)bar->w);
                nemoshow_item_set_height(it->one, (double)val);
            } else if (bar->dir == GRAPH_BAR_DIR_R) {
                nemoshow_item_translate(it->one, (double)adv, 0.0);
                nemoshow_item_set_width(it->one, (double)val);
                nemoshow_item_set_height(it->one, (double)bar->h);
            } else if (bar->dir == GRAPH_BAR_DIR_L) {
                nemoshow_item_translate(it->one, (double)(bar->w - val) - adv, 0.0);
                nemoshow_item_set_width(it->one, (double)val);
                nemoshow_item_set_height(it->one, (double)bar->h);
            }
            adv += val + bar->gap;
        }
    }
}

static inline void graph_bar_destroy(GraphBar *bar)
{
    RET_IF(!bar);

    List *l, *ll;
    GraphBarItem *it;
    LIST_FOR_EACH_SAFE(bar->items, l, ll, it) {
        graph_bar_item_destroy(it);
    }

    if (bar->blur.one) nemoshow_one_destroy(bar->blur.one);
    nemoshow_one_destroy(bar->group);
    free(bar);
}

static inline void graph_bar_show(GraphBar *bar, uint32_t easetype, int duration, int delay)
{
    GraphBarItem *it;
    List *l;
    LIST_FOR_EACH(bar->items, l, it) {
        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "alpha", 1.0,
                    NULL);
        } else {
            nemoshow_item_set_alpha(it->one, 1.0);
        }
    }
    graph_bar_update(bar, easetype, duration, delay);
}

static inline void graph_bar_hide(GraphBar *bar, uint32_t easetype, int duration, int delay)
{
    GraphBarItem *it;
    List *l;
    LIST_FOR_EACH(bar->items, l, it) {
        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "alpha", 0.0,
                    NULL);

            if (bar->dir == GRAPH_BAR_DIR_T) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     0.0,
                        "ty",     (double)bar->h,
                        "width",  (double)bar->w,
                        "height", 0.0,
                        NULL);
            } else if (bar->dir == GRAPH_BAR_DIR_B) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     0.0,
                        "ty",     (double)0.0,
                        "width",  (double)bar->w,
                        "height", (double)0.0,
                        NULL);
            } else if (bar->dir == GRAPH_BAR_DIR_R) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     (double)0.0,
                        "ty",     0.0,
                        "width",  0.0,
                        "height", (double)bar->h,
                        NULL
                        );
            } else if (bar->dir == GRAPH_BAR_DIR_L) {
                _nemoshow_item_motion(it->one, easetype, duration, delay,
                        "tx",     (double)bar->w,
                        "ty",     0.0,
                        "width",  0.0,
                        "height", (double)bar->h,
                        NULL
                        );
            }
            NemoMotion *m = nemomotion_create(bar->show, easetype, duration, delay);
            nemomotion_attach(m, 0.9, it->one, "fill", it->color.active, NULL);
            nemomotion_attach(m, 1.0, it->one, "fill", it->color.base, NULL);
            nemomotion_run(m);

            if (bar->blur.one) {
                NemoMotion *m = nemomotion_create(bar->show, easetype, duration, delay);
                nemomotion_attach(m, 0.5, bar->blur.one, "r", 0.0, NULL);
                nemomotion_attach(m, 1.0, bar->blur.one, "r", (double)bar->blur.r, NULL);
                nemomotion_run(m);
            }
        } else {
            nemoshow_item_set_alpha(it->one, 0.0);
        }
    }
}

static inline void graph_bar_translate(GraphBar *bar, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(bar->group, easetype, duration, delay,
                "tx", tx, "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(bar->group, tx, ty);
    }
}

/*************************************************/
/* GraphPie */
/*************************************************/
typedef struct _GraphPie GraphPie;
struct _GraphPie {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int w, h, size;

    struct {
        struct showone *one;
        int r;
    } blur;
    struct showone *group;

    int gap; // radius 0 ~ 360
    List *items;
};

typedef struct _GraphPieItem GraphPieItem;
struct _GraphPieItem {
    GraphPie *pie;
    struct showone *group;
    struct showone *one;
    bool dirty;

    struct {
        uint32_t base;
        uint32_t active;
    } color;

    double percent;
};

static inline bool graph_pie_is(GraphPie *pie, int x, int y)
{
    int cx = 0, cy = 0;
    _nemoshow_item_get_geometry(pie->group, &cx, &cy, NULL, NULL);

    if (CIRCLE_IN(cx, cy, pie->w/2.0, x, y)) {
        if (!CIRCLE_IN(cx, cy, pie->w/2.0 - pie->size * 2, x, y)) {
            return true;
        }
    }

    return false;
}

static inline GraphPie *graph_pie_create(struct nemotool *tool, struct showone *parent, int w, int h, int size)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    GraphPie *pie = (GraphPie *)calloc(sizeof(GraphPie), 1);
    pie->tool = tool;
    pie->show = parent->show;
    pie->parent = parent;
    pie->w = w;
    pie->h = h;
    pie->size = size;

    pie->group = GROUP_CREATE(pie->parent);

    return pie;
}

static inline void graph_pie_set_blur(GraphPie *pie, const char *style, int r)
{
    if (pie->blur.one) {
        nemoshow_one_destroy(pie->blur.one);
        pie->blur.one = NULL;
    }

    if (!style || r <= 0) return;

    pie->blur.one = BLUR_CREATE(style, r);
    pie->blur.r = r;

    GraphPieItem *it;
    List *l;
    LIST_FOR_EACH(pie->items, l, it) {
        nemoshow_item_set_filter(it->one, pie->blur.one);
    }
}

static inline void graph_pie_translate(GraphPie *pie, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(pie->group, easetype, duration, delay,
                "tx", (double)x,
                "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(pie->group, x, y);
    }
}

static inline void graph_pie_set_gap(GraphPie *pie, int gap)
{
    RET_IF(!pie);
    pie->gap = gap;
}

static inline void graph_pie_set_size(GraphPie *pie, uint32_t easetype, int duration, int delay,
        int w, int h, int size)
{
    pie->w = w;
    pie->h = h;
    pie->size = size;

    GraphPieItem *it;
    List *l;
    LIST_FOR_EACH(pie->items, l, it) {
        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "width", (double)pie->w - pie->size,
                    "height", (double)pie->h - pie->size,
                    "stroke-width", (double)pie->size,
                    NULL);
        } else {
            nemoshow_item_set_width(it->one, pie->w - pie->w);
            nemoshow_item_set_height(it->one, pie->h - pie->h);
            nemoshow_item_set_stroke_width(it->one, pie->size);
        }
    }
}

static inline GraphPieItem *graph_pie_create_item(GraphPie *pie)
{
    GraphPieItem *it = (GraphPieItem *)calloc(sizeof(GraphPieItem), 1);
    it->pie = pie;

    struct showone *group;
    it->group = group = GROUP_CREATE(pie->group);

    struct showone *one;

    one = ARC_CREATE(group, 0, 0, 0, 0);
    nemoshow_item_set_stroke_cap(one, 0);
    nemoshow_item_set_stroke_color(one, RGBA(0xFFFFFFFF));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    if (pie->blur.one) nemoshow_item_set_filter(one, pie->blur.one);

    it->one = one;

    pie->items = list_append(pie->items, it);

    return it;
}
static inline void graph_pie_item_destroy(GraphPieItem *it)
{
    RET_IF(!it);
    RET_IF(!it->pie);

    it->pie->items = list_remove(it->pie->items, it);
    nemoshow_one_destroy(it->group);
    free(it);
}

static inline void graph_pie_item_set_color(GraphPieItem *it, uint32_t color_base, uint32_t color_active)
{
    it->color.base = color_base;
    it->color.active = color_active;
    nemoshow_item_set_stroke_color(it->one, RGBA(it->color.base));
}

static inline void graph_pie_item_set_percent(GraphPieItem *it, double percent)
{
    RET_IF(!it);

    it->percent = percent;
    it->dirty = true;
}

static inline void graph_pie_update(GraphPie *pie, uint32_t easetype, int duration, int delay)
{
    List *l;
    GraphPieItem *it;
    double tot = 0;
    tot = 360 - (list_count(pie->items)) * pie->gap;

    double from, to;
    from = 0;
    LIST_FOR_EACH(pie->items, l, it) {
        to = it->percent * tot + from;

        double _delay = 0;
        double _delay2 = 0;
        if (NEMOSHOW_ITEM_AT(it->one, from) > from) {
            _delay = 250;
        }
        if (NEMOSHOW_ITEM_AT(it->one, to) < to) {
            _delay2 = 250;
        }

        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration - _delay, delay + _delay,
                    "from", from,
                    NULL);
            _nemoshow_item_motion(it->one, easetype, duration - _delay2, delay + _delay2,
                    "to", to,
                    NULL);
            if (it->dirty) {
                NemoMotion *m = nemomotion_create(pie->show, easetype, duration, delay);
                nemomotion_attach(m, 0.9, it->one, "stroke", it->color.active, NULL);
                nemomotion_attach(m, 1.0, it->one, "stroke", it->color.base, NULL);
                nemomotion_run(m);

                if (pie->blur.one) {
                    NemoMotion *m = nemomotion_create(pie->show, easetype, duration, delay);
                    nemomotion_attach(m, 0.5, pie->blur.one, "r", 0.0, NULL);
                    nemomotion_attach(m, 1.0, pie->blur.one, "r", (double)pie->blur.r, NULL);
                    nemomotion_run(m);
                }
                it->dirty = false;
            }
        } else {
            nemoshow_item_set_from(it->one, from);
            nemoshow_item_set_to(it->one, to);
        }
        from = to + pie->gap;

    }
}

static inline void graph_pie_destroy(GraphPie *pie)
{
    RET_IF(!pie);

    List *l, *ll;
    GraphPieItem *it;
    LIST_FOR_EACH_SAFE(pie->items, l, ll, it) {
        graph_pie_item_destroy(it);
    }

    if (pie->blur.one) nemoshow_one_destroy(pie->blur.one);
    nemoshow_one_destroy(pie->group);
    free(pie);
}

static inline void graph_pie_show(GraphPie *pie, uint32_t easetype, int duration, int delay)
{
    GraphPieItem *it;
    List *l;
    LIST_FOR_EACH(pie->items, l, it) {
        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "width", (double)pie->w - pie->size,
                    "height", (double)pie->h - pie->size,
                    "stroke-width", (double)pie->size,
                    "alpha", 1.0,
                    NULL);
        } else {
            nemoshow_item_set_width(it->one, pie->w - pie->size);
            nemoshow_item_set_height(it->one, pie->h - pie->size);
            nemoshow_item_set_stroke_width(it->one, pie->size);
            nemoshow_item_set_alpha(it->one, 1.0);
        }
    }
    graph_pie_update(pie, easetype, duration, delay);
}

static inline void graph_pie_hide(GraphPie *pie, uint32_t easetype, int duration, int delay)
{
    GraphPieItem *it;
    List *l;
    LIST_FOR_EACH(pie->items, l, it) {
        if (duration > 0) {
            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "width", 0.0,
                    "height", 0.0,
                    "stroke-width", 0.0,
                    "alpha", 0.0,
                    NULL);

            _nemoshow_item_motion(it->one, easetype, duration, delay,
                    "from", 0.0,
                    "to", 0.0,
                    NULL);
            NemoMotion *m = nemomotion_create(pie->show, easetype, duration, delay);
            nemomotion_attach(m, 0.9, it->one, "stroke", it->color.active, NULL);
            nemomotion_attach(m, 1.0, it->one, "stroke", it->color.base, NULL);
            nemomotion_run(m);

            if (pie->blur.one) {
                NemoMotion *m = nemomotion_create(pie->show, easetype, duration, delay);
                nemomotion_attach(m, 0.5, pie->blur.one, "r", 1.0, NULL);
                nemomotion_attach(m, 1.0, pie->blur.one, "r", (double)pie->blur.r, NULL);
                nemomotion_run(m);
            }
        } else {
            nemoshow_item_set_width(it->one, 0.0);
            nemoshow_item_set_height(it->one, 0.0);
            nemoshow_item_set_stroke_width(it->one, 0);
            nemoshow_item_set_from(it->one, 0.0);
            nemoshow_item_set_to(it->one, 0.0);
            nemoshow_item_set_alpha(it->one, 0.0);
        }
    }
}

/*************************************************/
/* GraphPies */
/*************************************************/
typedef struct _GraphPies GraphPies;
struct _GraphPies {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int w, h, inner;
    struct showone *group;

    struct {
        char *style;
        int r;
    } blur;

    int gap;
    List *items;
};

typedef struct _GraphPiesItem GraphPiesItem;
struct _GraphPiesItem {
    GraphPies *pies;

    int w, h, size;
    GraphPie *pie;
};

static inline bool graph_pies_is(GraphPies *pies, int x, int y)
{
    List *l;
    GraphPiesItem *it;
    LIST_FOR_EACH(pies->items, l, it) {
        if (graph_pie_is(it->pie, x, y))
            return true;
    }
    return false;
}

static inline GraphPies *graph_pies_create(struct nemotool *tool, struct showone *parent, int w, int h, int inner)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    GraphPies *pies = (GraphPies *)calloc(sizeof(GraphPies), 1);
    pies->tool = tool;
    pies->show = parent->show;
    pies->parent = parent;
    pies->w = w;
    pies->h = h;
    pies->inner = inner;
    pies->group = GROUP_CREATE(pies->parent);

    return pies;
}

static inline void graph_pies_set_blur(GraphPies *pies, const char *style, int r)
{
    if (pies->blur.style) {
        free(pies->blur.style);
        pies->blur.style = NULL;
    }

    if (style) pies->blur.style = strdup(style);
    pies->blur.r = r;

    GraphPiesItem *it;
    List *l;
    LIST_FOR_EACH(pies->items, l, it) {
        graph_pie_set_blur(it->pie, style, r);
    }
}


static inline void graph_pies_translate(GraphPies *pies, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(pies->group, easetype, duration, delay,
                "tx", (double)x,
                "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(pies->group, x, y);
    }
}

static inline void graph_pies_set_gap(GraphPies *pies, int gap)
{
    RET_IF(!pies);
    pies->gap = gap;
}

static inline GraphPiesItem *graph_pies_create_item(GraphPies *pies)
{
    GraphPiesItem *it;
    it = (GraphPiesItem *)calloc(sizeof(GraphPiesItem), 1);
    it->pies = pies;
    it->pie = graph_pie_create(pies->tool, pies->group, 0, 0, 0);
    graph_pie_set_blur(it->pie, pies->blur.style, pies->blur.r);

    pies->items = list_append(pies->items, it);
    return it;
}

static inline void graph_pies_item_destroy(GraphPiesItem *it)
{
    RET_IF(!it);
    RET_IF(!it->pies);

    it->pies->items = list_remove(it->pies->items, it);
    graph_pie_destroy(it->pie);
    free(it);
}

static inline GraphPie *graph_pies_item_get_pie(GraphPiesItem *it)
{
    RET_IF(!it, NULL);

    return it->pie;
}

static inline void graph_pies_update(GraphPies *pies, uint32_t easetype, int duration, int delay)
{
    int cnt = list_count(pies->items);

    double adv = 150;
    double _delay = delay;
    double _duration = duration - (cnt - 1) * adv;

    GraphPiesItem *it;
    List *l;
    LIST_FOR_EACH_REVERSE(pies->items, l, it) {
        graph_pie_update(it->pie, easetype, _duration, _delay);
        _delay += adv;
    }
}

static inline void graph_pies_destroy(GraphPies *pies)
{
    RET_IF(!pies);

    List *l, *ll;
    GraphPiesItem *it;
    LIST_FOR_EACH_SAFE(pies->items, l, ll, it) {
        graph_pies_item_destroy(it);
    }

    if (pies->blur.style) free(pies->blur.style);
    nemoshow_one_destroy(pies->group);
    free(pies);
}

static inline void graph_pies_show(GraphPies *pies, uint32_t easetype, int duration, int delay)
{
    int cnt = list_count(pies->items);

    double w, h;
    w = pies->w/2 - pies->inner - (cnt - 1) * pies->gap;
    h = pies->h/2 - pies->inner - (cnt - 1) * pies->gap;

    double w_size = w/cnt;
    double h_size = h/cnt;

    double ww_size = (w_size + pies->inner) * 2;
    double hh_size = (h_size + pies->inner) * 2;

    double _delay = delay;
    double _duration = duration/cnt;
    double adv = _duration;

    GraphPiesItem *it;
    List *l;
    LIST_FOR_EACH(pies->items, l, it) {
        graph_pie_show(it->pie, easetype, _duration, _delay);
        graph_pie_set_size(it->pie, easetype, _duration, _delay, ww_size, hh_size,
                w_size > h_size ? w_size : h_size);
        ww_size += (w_size + pies->gap) * 2;
        hh_size += (h_size + pies->gap) * 2;
        _delay += adv;
    }
    graph_pies_update(pies, easetype, duration, delay);
}

static inline void graph_pies_hide(GraphPies *pies, uint32_t easetype, int duration, int delay)
{
    GraphPiesItem *it;
    List *l;
    LIST_FOR_EACH(pies->items, l, it) {
        graph_pie_set_size(it->pie, easetype, duration, delay, 0, 0, 0);
        graph_pie_hide(it->pie, easetype, duration, delay);
    }
}

#ifdef __cplusplus
}
#endif

#endif
