#ifndef __NEMOUI_MISC_H__
#define __NEMOUI_MISC_H__

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include <nemoui-image.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SpinFx SpinFx;
struct _SpinFx {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int w, h;
    int inner_size;
    int num;
    int gap;

    struct showone *blur;
    struct showone *emboss;
    struct showone *group;

    List *items;
};

static inline SpinFx *spinfx_create(struct nemotool *tool, struct showone *parent, int w, int h, int inner_size, int num, int gap)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    gap += 12;
    SpinFx *sf = (SpinFx *)calloc(sizeof(SpinFx), 1);
    sf->tool = tool;
    sf->show = parent->show;
    sf->parent = parent;
    sf->w = w;
    sf->h = h;
    sf->inner_size = inner_size;
    sf->num = num;
    sf->gap = gap;
    sf->group = GROUP_CREATE(parent);

    int r = (360/8)/num;
    int i;
    for (i = 0; i < num ; i++) {
        struct showone *one;
        one = ARC_CREATE(sf->group,
                w - sf->inner_size, h - sf->inner_size,
                r * i  - (360/8) + gap * i,
                r * (i+1) - (360/8) + gap * i);
        //nemoshow_item_set_stroke_cap(one, 0);
        nemoshow_item_set_stroke_color(one, RGBA(0xFFFFFFFF));
        nemoshow_item_set_stroke_width(one, inner_size);
        nemoshow_item_set_alpha(one, 0.0);
        sf->items = list_append(sf->items, one);
    }

    return sf;
}

static inline void spinfx_set_color(SpinFx *sf, uint32_t easetype, int duration, int delay, uint32_t color)
{
    List *l;
    struct showone *one;
    LIST_FOR_EACH(sf->items, l, one) {
        if (duration > 0)
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "fill", color,
                    NULL);
        else
            nemoshow_item_set_fill_color(one, RGBA(color));
    }
}

static inline void spinfx_show(SpinFx *sf, uint32_t easetype, int duration, int delay)
{
    //int r = (360/8)/sf->num;
    int i = 0;

    int ddelay = 150;
    duration -= (sf->num - 1) * ddelay;
    RET_IF(duration < 0)

    List *l;
    struct showone *one;
    LIST_FOR_EACH(sf->items, l, one) {
        double from = NEMOSHOW_ITEM_AT(one, from);
        double to = NEMOSHOW_ITEM_AT(one, to);
        nemoshow_item_set_from(one, from - 360.0);
        nemoshow_item_set_to(one, to - 360.0);
        if (duration > 0) {
            _nemoshow_item_motion(one, easetype, duration - ddelay * i, delay,
                    "from", from,
                    "to",   to,
                    "alpha", 1.0,
                    NULL);
        } else {
            nemoshow_item_set_from(one, from);
            nemoshow_item_set_to(one, to);
            nemoshow_item_set_alpha(one, 1.0);
        }
        i++;
    }
}

static inline void spinfx_hide(SpinFx *sf, uint32_t easetype, int duration, int delay)
{
    //int r = (360/8)/sf->num;
    int i = 0;

    int ddelay = 150;
    duration -= (sf->num - 1) * ddelay;
    RET_IF(duration < 0)

    List *l;
    struct showone *one;
    LIST_FOR_EACH(sf->items, l, one) {
        double from = NEMOSHOW_ITEM_AT(one, from);
        double to = NEMOSHOW_ITEM_AT(one, to);
        nemoshow_item_set_from(one, from - 360.0);
        nemoshow_item_set_to(one, to - 360.0);
        if (duration > 0) {
            _nemoshow_item_motion(one, easetype, duration - ddelay * i, delay,
                    "from", from,
                    "to",   to,
                    "alpha", 0.0,
                    NULL);
        } else {
            nemoshow_item_set_from(one, from);
            nemoshow_item_set_to(one, to);
            nemoshow_item_set_alpha(one, 0.0);
        }
        i++;
    }
}

static inline void spinfx_translate(SpinFx *sf, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0)
    _nemoshow_item_motion(sf->group, easetype, duration, delay,
            "tx", tx,
            "ty", ty,
            NULL);
    else
        nemoshow_item_translate(sf->group, tx, ty);
}

typedef struct _PieHand PieHand;
struct _PieHand {
    struct showone *parent;
    int w, h;
    int inner;
    int num;
    int width, gap;

    struct showone *blur;
    struct showone *group;

    struct showone *ring;
    struct showone *hand;
};

static inline PieHand *pie_hand_create(struct showone *parent, int w, int h, int inner, int hands_num)
{
    PieHand *ph = (PieHand *)calloc(sizeof(PieHand), 1);
    ph->parent = parent;
    ph->w = w;
    ph->h = h;
    ph->inner = inner;
    ph->num = hands_num;
    ph->width = 360/hands_num;

    ph->blur = BLUR_CREATE("solid", 5);

    struct showone *group;
    group = GROUP_CREATE(parent);
    ph->group = group;

    struct showone *one;
    one = ARC_CREATE(group, 0, 0, 0, 360);
    nemoshow_item_set_stroke_cap(one, 0);
    nemoshow_item_set_stroke_color(one, RGBA(0xFFFFFFFF));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    ph->ring = one;

    double start;
    start = -ph->width/2.0 - (ph->width) * (ph->num/4);

    one = ARC_CREATE(group, 0, 0, start, start + ph->width);
    nemoshow_item_set_stroke_cap(one, 0);
    nemoshow_item_set_stroke_color(one, RGBA(0xFFFFFFFF));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_filter(one, ph->blur);
    ph->hand = one;

    return ph;
}

static inline void pie_hand_destroy(PieHand *ph)
{
    nemoshow_one_destroy(ph->hand);
    nemoshow_one_destroy(ph->ring);
    nemoshow_one_destroy(ph->group);
    nemoshow_one_destroy(ph->blur);
    free(ph);
}

static inline void pie_hand_set_color(PieHand *ph, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(ph->ring, easetype, duration, delay,
                "fill", color,
                NULL);
        _nemoshow_item_motion(ph->hand, easetype, duration, delay,
                "fill", color,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(ph->ring, RGBA(color));
        nemoshow_item_set_stroke_color(ph->hand, RGBA(color));
    }
}

static inline void pie_hand_show(PieHand *ph, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(ph->ring, easetype, duration, delay,
                "width", (double)ph->w - ph->inner,
                "height", (double)ph->h - ph->inner,
                "stroke-width", (double)ph->inner,
                "alpha", 0.5,
                NULL);
        _nemoshow_item_motion(ph->hand, easetype, duration, delay,
                "width", (double)ph->w - ph->inner,
                "height", (double)ph->h - ph->inner,
                "stroke-width", (double)ph->inner,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_width(ph->ring, (double)ph->w - ph->inner);
        nemoshow_item_set_height(ph->ring, (double)ph->h - ph->inner);
        nemoshow_item_set_stroke_width(ph->ring, (double)ph->inner);
        nemoshow_item_set_alpha(ph->ring, 0.5);
        nemoshow_item_set_width(ph->hand, (double)ph->w - ph->inner);
        nemoshow_item_set_height(ph->hand, (double)ph->h - ph->inner);
        nemoshow_item_set_stroke_width(ph->hand, (double)ph->inner);
        nemoshow_item_set_alpha(ph->hand, 1.0);
    }
}

static inline void pie_hand_hide(PieHand *ph, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(ph->ring, easetype, duration, delay,
                "width", 0.0,
                "height", 0.0,
                "alpha", 0.0,
                NULL);
        _nemoshow_item_motion(ph->hand, easetype, duration, delay,
                "width", 0.0,
                "height", 0.0,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(ph->ring, 0.0);
        nemoshow_item_set_alpha(ph->hand, 0.0);
    }
}

static inline void pie_hand_translate(PieHand *ph, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(ph->group, easetype, duration, delay,
                "tx", (double)x,
                "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(ph->group, x, y);
    }
}

static inline void pie_hand_update(PieHand *ph, uint32_t easetype, int duration, int delay, int num)
{
    struct nemoshow *show = ph->group->show;

    double start;
    start = -ph->width/2.0 - (ph->width) * (ph->num/4) + num * ph->width;

    double from, to;
    from = start;
    to = start + ph->width;

    if (duration > 0) {
        NemoMotion *m;
        m = nemomotion_create(show, easetype, duration, delay);
        nemomotion_attach(m, 0.5, ph->hand, "to", to, NULL);
        nemomotion_attach(m, 1.0, ph->hand, "from", from, NULL);
        nemomotion_run(m);
    } else {
        nemoshow_item_set_from(ph->hand, from);
        nemoshow_item_set_to(ph->hand, to);
    }
}

typedef struct _CircleHand CircleHand;
struct _CircleHand {
    struct showone *parent;
    int w, h;
    double inner;
    double width;
    double gap;

    struct showone *blur;
    struct showone *group;

    List *hands;
    struct showone *hand;
};


static inline CircleHand *circle_hand_create(struct showone *parent, int w, int h, int inner, int hands_num)
{
    RET_IF(hands_num >= 120, NULL)

    CircleHand *ch = (CircleHand *)calloc(sizeof(CircleHand), 1);
    ch->parent = parent;
    ch->w = w;
    ch->h = h;
    ch->inner = inner;
    ch->width = (360.0/hands_num)/3.0;
    ch->gap = (360 - ch->width * 60)/60.0;

    struct showone *blur;
    blur = BLUR_CREATE("solid", 5);
    ch->blur = blur;

    struct showone *one;
    struct showone *group = GROUP_CREATE(parent);
    ch->group = group;

    double start, _start;
    _start = start = -ch->width/2.0 - (ch->width + ch->gap) * 15;

    int i = 0;
    for (i = 0 ; i < hands_num ; i++) {
        one = ARC_CREATE(group, 0, 0, start, start + ch->width);
        nemoshow_item_set_stroke_cap(one, 0);
        nemoshow_item_set_stroke_color(one, RGBA(0x1EDCDCFF));
        nemoshow_item_set_stroke_width(one, 0);
        nemoshow_item_scale(one, 0.1, 0.1);
        nemoshow_item_set_alpha(one, 0.1);

        start = start + ch->width + ch->gap;
        ch->hands = list_append(ch->hands, one);
    }

    one = ARC_CREATE(group, 0, 0, _start, _start + ch->width);
    nemoshow_item_set_stroke_cap(one, 0);
    nemoshow_item_set_stroke_color(one, RGBA(0x1EDCDCFF));
    nemoshow_item_set_stroke_width(one, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_filter(one, blur);
    ch->hand = one;

    return ch;
}

static inline void circle_hand_destroy(CircleHand *ch)
{
    struct showone *one;
    nemoshow_one_destroy(ch->hand);
    LIST_FREE(ch->hands, one) {
        nemoshow_one_destroy(one);
    }
    nemoshow_one_destroy(ch->group);
    nemoshow_one_destroy(ch->blur);
    free(ch);
}

static inline void circle_hand_update(CircleHand *ch, uint32_t easetype, int duration, int delay,
        int tick)
{
    struct nemoshow *show = ch->group->show;

    double from, to;
    from = -ch->width/2.0 + (ch->width + ch->gap) * (tick- 15);
    to = from + ch->width;
    NemoMotion *m;
    m = nemomotion_create(show, easetype, duration, delay);
    nemomotion_attach(m, 0.5, ch->hand, "to", to, NULL);
    nemomotion_attach(m, 1.0, ch->hand, "from", from, NULL);
    nemomotion_run(m);

    int i = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(ch->hands, l, one) {
        if (i < tick) {
            nemoshow_item_set_filter(one, ch->blur);
            nemoshow_one_dirty(one, NEMOSHOW_ALL_DIRTY);
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "alpha", 1.0,
                    NULL);
        } else {
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "alpha", 0.5,
                    NULL);
            nemoshow_one_dirty(one, NEMOSHOW_ALL_DIRTY);
        }
        i++;
    }

}

static inline void circle_hand_show(CircleHand *ch, uint32_t easetype, int duration, int delay)
{
    int i = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(ch->hands, l, one) {
        if (duration > 0) {
        _nemoshow_item_motion(one, easetype, duration - 20 * i, delay + 20 * i,
                "width",  (double)ch->w - ch->inner,
                "height", (double)ch->h - ch->inner,
                "stroke-width", (double)ch->inner,
                "sx", 1.0,
                "sy", 1.0,
                "alpha", 0.5,
                NULL);
        } else {
            nemoshow_item_set_width(one, ch->w - ch->inner);
            nemoshow_item_set_height(one, ch->h - ch->inner);
            nemoshow_item_set_stroke_width(one, ch->inner);
            nemoshow_item_scale(one, 1.0, 1.0);
            nemoshow_item_set_alpha(one, 0.5);
        }
        i++;
    }

    if (duration > 0) {
        _nemoshow_item_motion(ch->hand, easetype, duration, delay,
                "width",  (double)ch->w - ch->inner,
                "height", (double)ch->h - ch->inner,
                "stroke-width", (double)ch->inner,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_width(ch->hand, ch->w - ch->inner);
        nemoshow_item_set_height(ch->hand, ch->h - ch->inner);
        nemoshow_item_set_stroke_width(ch->hand, ch->inner);
        nemoshow_item_set_alpha(ch->hand, 1.0);
    }

}

static inline void circle_hand_hide(CircleHand *ch, uint32_t easetype, int duration, int delay)
{
    int i = 0;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(ch->hands, l, one) {
        if (duration > 0) {
            _nemoshow_item_motion(one, easetype, duration - 10 * i, delay + 10 * i,
                    "width",  0.0,
                    "height", 0.0,
                    "stroke-width", 0.0,
                    "sx", 0.0,
                    "sy", 0.0,
                    "alpha", 0.0,
                    NULL);
        } else {
            nemoshow_item_set_width(one, 0.0);
            nemoshow_item_set_height(one, 0.0);
            nemoshow_item_set_stroke_width(one, 0.0);
            nemoshow_item_scale(one, 0.0, 0.0);
            nemoshow_item_set_alpha(one, 0.0);
        }
        i++;
    }

    if (duration > 0) {
        _nemoshow_item_motion(ch->hand, easetype, duration, delay,
                "width",  0.0,
                "height", 0.0,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(ch->hand, 0.0);
    }
}

static inline void circle_hand_translate(CircleHand *hand, int x, int y)
{
    nemoshow_item_translate(hand->group, x, y);
}

typedef struct _NumberText NumberText;
struct _NumberText {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int fontsize;
    int stroke_w;
    int x_offset;
    int y_offset;

    bool is_show;
    int number;

    struct showone *group;
    List *texts;

    struct {
        uint32_t easetype;
        int duration;
        int delay;
    } line;
    struct nemotimer *motion_timer;
};

static inline NumberText *number_text_create(struct nemotool *tool, struct showone *parent, int fontsize, int stroke_w, int cnt)
{
    NumberText *nt = (NumberText *)calloc(sizeof(NumberText), 1);
    nt->tool = tool;
    nt->show = parent->show;
    nt->parent = parent;
    nt->fontsize = fontsize;
    nt->x_offset = fontsize;
    nt->y_offset = fontsize;

    struct showone *group;
    group = GROUP_CREATE(parent);
    nt->group = group;

    nemoshow_item_set_width(group, nt->x_offset * cnt);
    nemoshow_item_set_height(group, nt->y_offset);
    nemoshow_item_set_anchor(group, 0.5, 0.5);

    struct showone *one;
    int i = 0;
    for (i = 0 ; i < cnt ; i++) {
        one = NUM_CREATE(group, WHITE, stroke_w, fontsize, 0);
        nemoshow_item_set_alpha(one, 0.0);
        nt->texts = list_append(nt->texts, one);
    }

    double adv = ((cnt - 1) * fontsize)/2;
    one = (struct showone *)LIST_DATA(LIST_LAST(nt->texts));
    nemoshow_item_translate(one, adv, 0);

    return nt;
}

static inline void number_text_destroy(NumberText *nt)
{
    struct showone *one;
    if (nt->motion_timer) {
        nemotimer_destroy(nt->motion_timer);
    }

    LIST_FREE(nt->texts, one) {
        nemoshow_one_destroy(one);
    }
    nemoshow_one_destroy(nt->group);
    free(nt);
}

static inline void number_text_set_color(NumberText *nt, uint32_t easetype, int duration, int delay, uint32_t color)
{
    List *l;
    struct showone *one;
    LIST_FOR_EACH(nt->texts, l, one) {
        NUM_SET_COLOR(one, easetype, duration, delay, color);
    }
}

static inline void number_text_set_filter(NumberText *nt, struct showone *filter)
{
    List *l;
    struct showone *one;
    LIST_FOR_EACH(nt->texts, l, one) {
        nemoshow_item_set_filter(one, filter);
    }
}

static inline void number_text_set_shader(NumberText *nt, struct showone *shader)
{
    List *l;
    struct showone *one;
    LIST_FOR_EACH(nt->texts, l, one) {
        nemoshow_item_set_shader(one, shader);
    }
}

static inline void _number_text_motion_line_timeout(struct nemotimer *timer, void *userdata)
{
    NumberText *nt = (NumberText *)userdata;

    int cnt = list_count(nt->texts);
    uint32_t easetype = nt->line.easetype;
    int adv = 150;
    int duration = nt->line.duration - adv * (cnt - 1) ;
    int delay = nt->line.delay;

    List *l;
    struct showone *one;
    LIST_FOR_EACH(nt->texts, l, one) {
        NemoMotion *m = nemomotion_create(nt->show, easetype, duration, delay);
        nemomotion_attach(m, 0.5, one, "to", 0.0, NULL);
        nemomotion_attach(m, 1.0, one, "to", 1.0, NULL);
        nemomotion_run(m);
        delay += adv;
    }
    nemotimer_set_timeout(timer, nt->line.duration + nt->line.delay);
}

static inline void number_text_motion_line(NumberText *nt, uint32_t easetype, int duration, int delay)
{
    if (nt->motion_timer) {
        nemotimer_destroy(nt->motion_timer);
        nt->motion_timer = NULL;
    }

    if (duration <= 0) return;

    struct nemotimer *timer;
    timer = nemotimer_create(nt->tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _number_text_motion_line_timeout);
    nemotimer_set_userdata(timer, nt);

    nt->motion_timer = timer;
    nt->line.easetype = easetype;
    nt->line.duration = duration;
    nt->line.delay = delay;
}

// FIXME: Should be duration > 0
static inline void number_text_update(NumberText *nt, uint32_t easetype, int duration, int delay, int num)
{
    if (nt->number == num) return;
    nt->number = num;

    int div = pow(10, (list_count(nt->texts) - 1));
    RET_IF(div <= 0);

    int tmp = num;
    List *l;
    struct showone *one;
    LIST_FOR_EACH_REVERSE(nt->texts, l, one) {
        NUM_UPDATE(one, easetype, duration, delay, tmp%10);
        if (nt->is_show) {
            if (tmp > 0) {
                if (duration > 0) {
                    _nemoshow_item_motion(one, easetype, duration, delay,
                            "alpha", 1.0,
                            NULL);
                } else {
                    nemoshow_item_set_alpha(one, 1.0);
                }
            } else {
                if (duration > 0) {
                    _nemoshow_item_motion(one, easetype, duration, delay,
                            "alpha", 0.0,
                            NULL);
                } else {
                    nemoshow_item_set_alpha(one, 1.0);
                }
            }
        }
        tmp = tmp/10;
    }

    int i = 0;
    LIST_FOR_EACH(nt->texts, l, one) {
        if (duration > 0) {
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "tx", (double)nt->x_offset * i,
                    NULL);
        } else {
            nemoshow_item_translate(one, nt->x_offset * i, 0);
        }
        i++;
    }
}

static inline void number_text_translate(NumberText *nt, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(nt->group, easetype, duration, delay,
                "tx", (double)x,
                "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(nt->group, x, y);
    }
}

static inline void number_text_show(NumberText *nt, uint32_t easetype, int duration, int delay)
{
    nt->is_show = true;
    int tmp = nt->number;
    List *l;
    struct showone *one;
    LIST_FOR_EACH_REVERSE(nt->texts, l, one) {
        if (tmp > 0) {
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "alpha", 1.0,
                    NULL);
        } else {
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "alpha", 0.0,
                    NULL);
        }
        tmp = tmp/10;
    }


    if (nt->motion_timer) {
        nemotimer_set_timeout(nt->motion_timer, 100);
    }
}

static inline void number_text_hide(NumberText *nt, uint32_t easetype, int duration, int delay)
{
    nt->is_show = false;
    List *l;
    struct showone *one;
    LIST_FOR_EACH(nt->texts, l, one) {
        if (duration > 0) {
            _nemoshow_item_motion(one, easetype, duration, delay,
                    "alpha", 0.0,
                    NULL);
        } else {
            nemoshow_item_set_alpha(one, 0.0);
        }
    }
    if (nt->motion_timer) {
        nemotimer_set_timeout(nt->motion_timer, 0);
    }
}

#define TEXT_PATH_VERSION 1

typedef struct _Text Text;
struct _Text {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    struct showone *emboss;
    struct showone *font;
    int fontsize;
    struct showone *group;

    double ax, ay;
    char *fontfamily;
    char *str;
    int idx;
    struct showone *one[2];

    struct {
        struct showone *one;
        uint32_t easetype;
        int duration;
        int delay;
        double r;
    } blur;
    struct nemotimer *motion_timer;
};

static inline Text *text_create(struct nemotool *tool, struct showone *parent, const char *family, const char *style,  int fontsize)
{
    struct nemoshow *show = parent->show;
    Text *text = (Text *)calloc(sizeof(Text), 1);
    text->tool = tool;
    text->show = show;
    text->parent = parent;
    text->ax = 0.5;
    text->ay = 0.5;

    struct showone *font;
    font = FONT_CREATE(family, style);
    text->font = font;
    text->fontsize = fontsize;

    struct showone *blur;
    blur = BLUR_CREATE("", 0);
    text->blur.one = blur;

    struct showone *group;
    group = GROUP_CREATE(parent);
    text->group = group;
    text->idx = 0;

    text->fontfamily = strdup(family);

#if TEXT_PATH_VERSION
    text->one[0] = PATH_TEXT_CREATE(group, family, fontsize, "");
#else
    text->one[0] = TEXT_CREATE(group, font, fontsize, "");
#endif
    nemoshow_item_set_anchor(text->one[0], 0.5, 0.5);
    nemoshow_item_set_fill_color(text->one[0], RGBA(0xFFFFFFFF));
    nemoshow_item_set_alpha(text->one[0], 1.0);

#if TEXT_PATH_VERSION
    text->one[1] = PATH_TEXT_CREATE(group, family, fontsize, "");
#else
    text->one[1] = TEXT_CREATE(group, font, fontsize, "");
#endif
    nemoshow_item_set_anchor(text->one[1], 0.5, 0.5);
    nemoshow_item_set_fill_color(text->one[1], RGBA(0xFFFFFFFF));
    nemoshow_item_set_alpha(text->one[1], 1.0);

    return text;
}

static inline void text_set_filter(Text *text, struct showone *filter)
{
    nemoshow_item_set_filter(text->one[0], filter);
    nemoshow_item_set_filter(text->one[1], filter);
}

static inline void text_set_anchor(Text *text, double ax, double ay)
{
    nemoshow_item_set_anchor(text->one[0], ax, ay);
    nemoshow_item_set_anchor(text->one[1], ax, ay);
}

static inline void text_get_size(Text *text, double *tw, double *th)
{
    nemoshow_one_update(text->one[text->idx]);
#if TEXT_PATH_VERSION
    if (tw) *tw = nemoshow_item_get_width(text->one[text->idx]);
    if (th) *th = nemoshow_item_get_height(text->one[text->idx]);
#else
    if (tw) *tw = NEMOSHOW_ITEM_AT(text->one[text->idx], textwidth);
    if (th) *th = NEMOSHOW_ITEM_AT(text->one[text->idx], textheight);
#endif
}

static inline void text_set_align(Text *text, double ax, double ay)
{
#if TEXT_PATH_VERSION
    // default is 0.5, 0.5
    text->ax = ax;
    text->ay = ay;
    nemoshow_one_update(text->one[0]);
    nemoshow_one_update(text->one[1]);
    nemoshow_item_translate(text->one[0],
            -nemoshow_item_get_x(text->one[0]) * (1.0 - 2.0 * ax),
            -nemoshow_item_get_y(text->one[0]) * (1.0 - 2.0 * ay));
    nemoshow_item_translate(text->one[1],
            -nemoshow_item_get_x(text->one[1]) * (1.0 - 2.0 * ax),
            -nemoshow_item_get_y(text->one[1]) * (1.0 - 2.0 * ay));
#else
    ERR("normal text item does not support set align");
#endif
}

static inline void text_destroy(Text *text)
{
    free(text->fontfamily);
    nemoshow_one_destroy(text->one[0]);
    nemoshow_one_destroy(text->one[1]);
    nemoshow_one_destroy(text->group);
    nemoshow_one_destroy(text->font);
    nemoshow_one_destroy(text->blur.one);
    if (text->motion_timer) {
        nemotimer_destroy(text->motion_timer);
    }
    free(text);
}

static inline double text_get_width(Text *text)
{
    return nemoshow_item_get_width(text->one[text->idx]);
}

static inline void text_set_blur(Text *text, const char *style, int r)
{
    /*
     * FIXME: blur should be created before text->one[0] or [1]
     * because of dirty order.
    if (text->blur.one) {
        nemoshow_one_destroy(text->blur.one);
        text->blur.one = NULL;
    }
    */

    RET_IF(!text->blur.one);
    if (!style || r <= 0) return;

    struct showone *blur;
    blur = text->blur.one;
    nemoshow_filter_set_blur(blur, style, r);
    text->blur.r = r;

    nemoshow_item_set_filter(text->one[0], blur);
    nemoshow_item_set_filter(text->one[1], blur);
}

static inline void _text_motion_blur_timeout(struct nemotimer *timer, void *userdata)
{
    Text *text = (Text *)userdata;

    NemoMotion *m;
    m = nemomotion_create(text->show, text->blur.easetype,
            text->blur.duration, text->blur.delay);
    nemomotion_attach(m, 0.5, text->blur.one, "r", 1.0, NULL);
    nemomotion_attach(m, 1.0, text->blur.one, "r", text->blur.r, NULL);
    nemomotion_run(m);
    nemotimer_set_timeout(timer, text->blur.duration + text->blur.delay);
}

static inline void text_motion_blur(Text *text, uint32_t easetype, int duration, int delay)
{
    RET_IF(!text->blur.one);

    if (text->motion_timer) {
        nemotimer_destroy(text->motion_timer);
        text->motion_timer = NULL;
    }
    if (duration <= 0) return;

    struct nemotimer *timer;
    timer = nemotimer_create(text->tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _text_motion_blur_timeout);
    nemotimer_set_userdata(timer, text);

    text->motion_timer = timer;
    text->blur.easetype = easetype;
    text->blur.duration = duration;
    text->blur.delay = delay;
}

static inline void text_set_fill_color(Text *text, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(text->one[0], easetype, duration, delay,
                "fill", color,
                NULL);
        _nemoshow_item_motion(text->one[1], easetype, duration, delay,
                "fill", color,
                NULL);
    } else {
        nemoshow_item_set_fill_color(text->one[0], RGBA(color));
        nemoshow_item_set_fill_color(text->one[1], RGBA(color));
    }
}

static inline void text_set_stroke_color(Text *text, uint32_t easetype, int duration, int delay, uint32_t color, int stroke_w)
{
    if (duration > 0) {
        _nemoshow_item_motion(text->one[0], easetype, duration, delay,
                "stroke", color,
                "stroke-width", (double)stroke_w,
                NULL);
        _nemoshow_item_motion(text->one[1], easetype, duration, delay,
                "stroke", color,
                "stroke-width", (double)stroke_w,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(text->one[0], RGBA(color));
        nemoshow_item_set_stroke_width(text->one[0], stroke_w);
        nemoshow_item_set_stroke_color(text->one[1], RGBA(color));
        nemoshow_item_set_stroke_width(text->one[1], stroke_w);
    }
}


static inline void text_translate(Text *text, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(text->group, easetype, duration, delay,
                "tx", (double)x,
                "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(text->group, x, y);
    }
}

static inline void text_show(Text *text, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(text->one[text->idx], easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(text->one[text->idx], 1.0);
    }
    if (text->motion_timer) {
        nemotimer_set_timeout(text->motion_timer, 100);
    }
}

static inline void text_hide(Text *text, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(text->one[text->idx], easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(text->one[text->idx], 0.0);
    }
    if (text->motion_timer) {
        nemotimer_set_timeout(text->motion_timer, 0);
    }
}

static inline void text_update(Text *text, uint32_t easetype, int duration, int delay, const char *str)
{
    RET_IF(!str);

    if (text->str && !strcmp(text->str, str)) return;

    if (text->str) free(text->str);
    if (str) text->str = strdup(str);

    if (duration > 0) {
        _nemoshow_item_motion(text->one[text->idx], easetype, duration, delay,
                "alpha", 0.0,
                NULL);
        text->idx = !text->idx;
        _nemoshow_item_motion(text->one[text->idx], easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(text->one[text->idx], 0.0);
        text->idx = !text->idx;
        nemoshow_item_set_alpha(text->one[text->idx], 1.0);
    }

#if TEXT_PATH_VERSION
    nemoshow_item_path_clear(text->one[text->idx]);
    if (str)
        nemoshow_item_path_text(text->one[text->idx],
                text->fontfamily, text->fontsize, str, strlen(str), 0, 0);
#else
    nemoshow_item_set_text(text->one[text->idx], str);
#endif
    text_set_align(text, text->ax, text->ay);
}

typedef struct _Animation Animation;
typedef struct _AnimationItem AnimationItem;
typedef void (*AnimationDone)(Animation *anim, void *userdata);

struct _AnimationItem {
    char *path;
};

struct _Animation
{
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *show_group;
    struct showone *group;
    int width, height;

    struct showone *img;
    unsigned int fps;
    int repeat;
    int idx;
    List *items;
    struct nemotimer *timer;

    AnimationDone done;
    void *done_userdata;
};

static inline void _animation(struct nemotimer *timer, void *userdata)
{
    Animation *anim = (Animation *)userdata;

    int cnt = list_count(anim->items);
    RET_IF(cnt <= 0);

    AnimationItem *it = (AnimationItem *)LIST_DATA(list_get_nth(anim->items, anim->idx));
    RET_IF(!it);

    //ERR("%s", it->path);
    nemoshow_item_set_uri(anim->img, it->path);
    nemoshow_dispatch_frame(anim->show);

    anim->idx++;
    if (anim->idx >= cnt) {
        anim->idx = 0;
        if (anim->repeat > 0) anim->repeat--;
        if (anim->repeat != 0) {
            nemotimer_set_timeout(timer, 1000/anim->fps);
        } else {
            nemotimer_set_timeout(timer, 0);
            if (anim->done) anim->done(anim, anim->done_userdata);
        }
    } else {
        nemotimer_set_timeout(timer, 1000/anim->fps);
    }
}

static inline Animation *animation_create(struct nemotool *tool, struct showone *parent, int width, int height)
{
    Animation *anim;
    anim = (Animation *)calloc(sizeof(Animation), 1);
    anim->tool = tool;
    anim->show = parent->show;
    anim->width = width;
    anim->height = height;
    anim->fps = 60;
    anim->repeat = -1; // infinite

    struct showone *group;
    anim->show_group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);

    anim->group = group = GROUP_CREATE(parent);

    struct showone *one;
    ITEM_CREATE(one, group, NEMOSHOW_IMAGE_ITEM);
    anim->img = one;
    nemoshow_item_set_width(one, width);
    nemoshow_item_set_height(one, height);

    anim->timer = nemotimer_create(tool);
    nemotimer_set_timeout(anim->timer, 0);
    nemotimer_set_callback(anim->timer, _animation);
    nemotimer_set_userdata(anim->timer, anim);

    return anim;
}

static inline void animation_set_done(Animation *anim, AnimationDone callback, void *userdata)
{
    anim->done = callback;
    anim->done_userdata = userdata;
}

static inline void animation_set_repeat(Animation *anim, int repeat)
{
    anim->repeat = repeat;
}

static inline void animation_set_fps(Animation *anim, unsigned int fps)
{
    anim->fps = fps;
}

static inline void animation_show(Animation *anim, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(anim->show_group, easetype, duration, delay,
                "alpha", 1.0,
                "sx", 1.0, "sy", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(anim->show_group, 1.0);
        nemoshow_item_scale(anim->show_group, 1.0, 1.0);
    }

    if (anim->fps) {
        nemotimer_set_timeout(anim->timer, 100 + delay);
    }
}

static inline void animation_hide(Animation *anim, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(anim->show_group, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(anim->show_group, 0.0);
        nemoshow_item_set_alpha(anim->show_group, 0.0);
        nemoshow_item_scale(anim->show_group, 0.0, 0.0);
    }
    nemotimer_set_timeout(anim->timer, 0);
}

static inline void animation_translate(Animation *anim, uint32_t easetype, int duration, int delay, int x, int y)
{
    if (duration > 0) {
        _nemoshow_item_motion(anim->group, easetype, duration, delay,
                "tx", (double)x, "ty", (double)y,
                NULL);
    } else {
        nemoshow_item_translate(anim->group, x, y);
    }
}

static inline void animation_rotate(Animation *anim, uint32_t easetype, int duration, int delay, double ro)
{
    if (duration > 0) {
        _nemoshow_item_motion(anim->group, easetype, duration, delay,
                "ro", ro,
                NULL);
    } else {
        nemoshow_item_rotate(anim->group, ro);
    }
}

static inline void animation_scale(Animation *anim, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(anim->group, easetype, duration, delay,
                "sx", sx, "sy", sy,
                NULL);
    } else {
        nemoshow_item_scale(anim->group, sx, sy);
    }
}

static inline void animation_set_anchor(Animation *anim, double ax, double ay)
{
    nemoshow_item_set_anchor(anim->img, ax, ay);
}

static inline void animation_append_item(Animation *anim, const char *path)
{
    RET_IF(!path);

    AnimationItem *it = (AnimationItem *)calloc(sizeof(AnimationItem), 1);
    it->path = strdup(path);
    anim->items = list_append(anim->items, it);
}

static inline void animation_set_clip(Animation *anim, struct showone *clip)
{
    RET_IF(!anim->img);
    nemoshow_item_set_clip(anim->img, clip);
}

static inline void animation_destroy(Animation *anim)
{
    nemotimer_destroy(anim->timer);

    AnimationItem *it;
    LIST_FREE(anim->items, it) {
        free(it->path);
        free(it);
    }

    nemoshow_one_destroy(anim->img);
    nemoshow_one_destroy(anim->group);
    free(anim);
}

typedef struct _PlexusPoint PlexusPoint;
struct _PlexusPoint {
    int x, y;
};

typedef struct _Plexus2 Plexus2;
struct _Plexus2 {
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    struct showone *group;

    struct showone *one;
    List *points;
};

static inline Plexus2 *plexus2_create(struct nemotool *tool, struct showone *parent, int w, int h, int num)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    Plexus2 *plexus = (Plexus2 *)calloc(sizeof(Plexus2), 1);
    plexus->tool = tool;
    plexus->show = parent->show;
    plexus->parent = parent;
    plexus->w = w;
    plexus->h = h;
    plexus->group = GROUP_CREATE(parent);

    int i;
    for (i = 0 ; i < num ; i++) {
        PlexusPoint *pt = (PlexusPoint *)calloc(sizeof(PlexusPoint), 1);
        pt->x = ((double)rand()/RAND_MAX) * w;
        pt->y = ((double)rand()/RAND_MAX) * h;
        plexus->points = list_append(plexus->points, pt);
    }

    ITEM_CREATE(plexus->one, plexus->group, NEMOSHOW_PATHARRAY_ITEM);
    nemoshow_item_set_stroke_color(plexus->one, RGBA(WHITE));
    nemoshow_item_set_stroke_width(plexus->one, 1);

    i = 0;
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        int j;
        for (j = i ; j < list_count(plexus->points) ; j++) {
            PlexusPoint *ptt;
            ptt = (PlexusPoint *)LIST_DATA(list_get_nth(plexus->points, j));
            if (pt == ptt) continue;
            nemoshow_item_path_moveto(plexus->one, pt->x, pt->y);
            nemoshow_item_path_lineto(plexus->one, ptt->x, ptt->y);
        }
        i++;
    }

    return plexus;
}

static inline void _plexus2_transition(Plexus2 *plexus, uint32_t easetype, int duration, int delay)
{
    NemoMotion *m = NULL;
    if (duration <= 0) {
        nemoshow_item_path_clear(plexus->one);
    } else {
        m = nemomotion_create(plexus->show, easetype, duration, delay);
    }

    int i = 0, k = 0;
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        int j;
        for (j = i ; j < list_count(plexus->points) ; j++) {
            PlexusPoint *ptt;
            ptt = (PlexusPoint *)LIST_DATA(list_get_nth(plexus->points, j));
            if (pt == ptt) continue;

            if (duration > 0) {
                char buf0[12], buf1[12], buf2[12], buf3[12], buf4[12], buf5[12];
                snprintf(buf0, 12, "points:%d", k++);
                snprintf(buf1, 12, "points:%d", k++);
                snprintf(buf2, 12, "points:%d", k++);
                snprintf(buf3, 12, "points:%d", k++);
                snprintf(buf4, 12, "points:%d", k++);
                snprintf(buf5, 12, "points:%d", k++);

                nemomotion_attach(m, 1.0,
                        plexus->one, buf0, (double)pt->x,
                        plexus->one, buf1, (double)pt->y,
                        plexus->one, buf2, 0,
                        plexus->one, buf3, 0,
                        plexus->one, buf4, 0,
                        plexus->one, buf5, 0,
                        NULL);

                snprintf(buf0, 12, "points:%d", k++);
                snprintf(buf1, 12, "points:%d", k++);
                snprintf(buf2, 12, "points:%d", k++);
                snprintf(buf3, 12, "points:%d", k++);
                snprintf(buf4, 12, "points:%d", k++);
                snprintf(buf5, 12, "points:%d", k++);
                nemomotion_attach(m, 1.0,
                        plexus->one, buf0, (double)ptt->x,
                        plexus->one, buf1, (double)ptt->y,
                        plexus->one, buf2, 0,
                        plexus->one, buf3, 0,
                        plexus->one, buf4, 0,
                        plexus->one, buf5, 0,
                        NULL);
            } else {
                nemoshow_item_path_moveto(plexus->one, pt->x, pt->y);
                nemoshow_item_path_lineto(plexus->one, ptt->x, ptt->y);
            }
        }
        i++;
    }
    if (duration > 0)
        nemomotion_run(m);
}

static inline void plexus2_show(Plexus2 *plexus, uint32_t easetype, int duration, int delay)
{
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        pt->x = ((double)rand()/RAND_MAX) * plexus->w;
        pt->y = ((double)rand()/RAND_MAX) * plexus->h;
    }
    _plexus2_transition(plexus, easetype, duration, delay);
}

static inline void plexus2_hide(Plexus2 *plexus, uint32_t easetype, int duration, int delay)
{
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        pt->x = plexus->w/2;
        pt->y = plexus->h/2;
    }

    _plexus2_transition(plexus, easetype, duration, delay);
}

static inline void plexus2_translate(Plexus2 *plexus, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(plexus->group, easetype, duration, delay,
                "tx", (double)tx,
                "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(plexus->group, tx, ty);
    }
}

static inline void plexus2_set_wh(Plexus2 *plexus, uint32_t easetype, int duration, int delay, int w, int h)
{
    plexus->w = w;
    plexus->h = h;
    plexus2_show(plexus, easetype, duration, delay);
}

typedef struct _Plexus Plexus;
struct _Plexus {
    int inr, outr;
    int w, h;
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;

    uint32_t color;
    double alpha;
    struct showone *group;

    struct showone *one;
    List *paths;
    List *points;
};

static inline void plexus_set_filter(Plexus *plexus, struct showone *filter)
{
    List *l;
    struct showone *path;
    LIST_FOR_EACH(plexus->paths, l, path) {
        nemoshow_item_set_filter(path, filter);
    }
}

static inline void plexus_set_shader(Plexus *plexus, struct showone *shader)
{
    List *l;
    struct showone *path;
    LIST_FOR_EACH(plexus->paths, l, path) {
        nemoshow_item_set_shader(path, shader);
    }
}

static inline Plexus *plexus_create(struct nemotool *tool, struct showone *parent, int w, int h, int sw, int num)
{
    RET_IF(!tool, NULL);
    RET_IF(!parent, NULL);

    Plexus *plexus = (Plexus *)calloc(sizeof(Plexus), 1);
    plexus->tool = tool;
    plexus->show = parent->show;
    plexus->parent = parent;
    plexus->w = w;
    plexus->h = h;
    plexus->group = GROUP_CREATE(parent);
    plexus->alpha = 1.0;

    int i;
    for (i = 0 ; i < num ; i++) {
        PlexusPoint *pt = (PlexusPoint *)calloc(sizeof(PlexusPoint), 1);
        pt->x = w/2;
        pt->y = h/2;
        plexus->points = list_append(plexus->points, pt);
    }

    plexus->one = PATH_GROUP_CREATE(plexus->group);
    nemoshow_item_set_alpha(plexus->one, 0.0);

    i = 0;
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        int j;
        for (j = i ; j < list_count(plexus->points) ; j++) {
            PlexusPoint *ptt;
            ptt = (PlexusPoint *)LIST_DATA(list_get_nth(plexus->points, j));
            if (pt == ptt) continue;
            struct showone *path;
            path = nemoshow_path_create(NEMOSHOW_ARRAY_PATH);
            nemoshow_one_attach(plexus->one, path);
            nemoshow_path_moveto(path, pt->x, pt->y);
            nemoshow_path_lineto(path, ptt->x, ptt->y);
            nemoshow_path_set_stroke_color(path, RGBA(GRAY));
            nemoshow_path_set_stroke_width(path, sw);
            plexus->paths = list_append(plexus->paths, path);
        }
        i++;
    }

    return plexus;
}

static inline void plexus_set_color(Plexus *plexus, uint32_t color)
{
    plexus->color = color;
}

static inline void _plexus_transition(Plexus *plexus, uint32_t easetype, int duration, int delay)
{
    NemoMotion *m = NULL;

    int i = 0, k = 0;
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        int j;
        for (j = i ; j < list_count(plexus->points) ; j++) {
            PlexusPoint *ptt;
            ptt = (PlexusPoint *)LIST_DATA(list_get_nth(plexus->points, j));
            if (pt == ptt) continue;

            struct showone *path;
            path = (struct showone *)LIST_DATA(list_get_nth(plexus->paths, k));
            if (duration > 0) {
                uint32_t color = 0x0;
                if (plexus->color > 0) {
                    color = plexus->color;
                } else {
                    int idx = rand() % 4;
                    if (idx == 0) {
                        color = RED;
                    } else if (idx == 1) {
                        color = BLUE;
                    } else if (idx == 2) {
                        color = GREEN;
                    } else {
                        color = YELLOW;
                    }
                }

                m = nemomotion_create(plexus->show, easetype, duration,
                        delay);
                nemomotion_attach(m, 1.0,
                        path, "stroke", color, NULL);
                nemomotion_attach(m, 1.0,
                        path, "points:0", (double)pt->x,
                        path, "points:1", (double)pt->y,
                        path, "points:2", 0,
                        path, "points:3", 0,
                        path, "points:4", 0,
                        path, "points:5", 0,
                        NULL);
                nemomotion_attach(m, 1.0,
                        path, "points:6",  (double)ptt->x,
                        path, "points:7",  (double)ptt->y,
                        path, "points:8",  0,
                        path, "points:9",  0,
                        path, "points:10", 0,
                        path, "points:11", 0,
                        NULL);
                nemomotion_run(m);
            } else {
                nemoshow_path_clear(path);
                nemoshow_path_moveto(path, pt->x, pt->y);
                nemoshow_path_lineto(path, ptt->x, ptt->y);
            }
            k++;
        }
        i++;
    }
}

static inline void _plexus_update(Plexus *plexus)
{
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        if (plexus->inr >= 0 && plexus->outr > 0) {
            double r = ((double)rand()/RAND_MAX) * (plexus->outr - plexus->inr)
                + plexus->inr;
            double angle = ((double)rand()/RAND_MAX) * 2 * M_PI;

            pt->x = r * cos(angle) + r;
            pt->y = r * sin(angle) + r;
        } else {
            pt->x = ((double)rand()/RAND_MAX) * plexus->w;
            pt->y = ((double)rand()/RAND_MAX) * plexus->h;
        }
    }
}

static inline void plexus_show(Plexus *plexus, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(plexus->one, easetype, duration, delay,
                "alpha", plexus->alpha, NULL);
    } else {
        nemoshow_path_set_alpha(plexus->one, plexus->alpha);
    }
    _plexus_update(plexus);
    _plexus_transition(plexus, easetype, duration, delay);
}

static inline void plexus_set_alpha(Plexus *plexus, double alpha)
{
    plexus->alpha = alpha;
}

static inline void plexus_hide(Plexus *plexus, uint32_t easetype, int duration, int delay)
{
    List *l;
    PlexusPoint *pt;
    LIST_FOR_EACH(plexus->points, l, pt) {
        pt->x = plexus->w/2;
        pt->y = plexus->h/2;
    }
    if (duration > 0)
        _nemoshow_item_motion(plexus->one, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    else
        nemoshow_item_set_alpha(plexus->one, 0.0);
    _plexus_transition(plexus, easetype, duration, delay);
}

static inline void plexus_translate(Plexus *plexus, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(plexus->group, easetype, duration, delay,
                "tx", (double)tx,
                "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(plexus->group, tx, ty);
    }
}

static inline void plexus_set_wh(Plexus *plexus, uint32_t easetype, int duration, int delay, int w, int h)
{
    plexus->inr = -1;
    plexus->outr = -1;
    plexus->w = w;
    plexus->h = h;
    nemoshow_item_set_width(plexus->one, w);
    nemoshow_item_set_height(plexus->one, h);
    _plexus_update(plexus);
    _plexus_transition(plexus, easetype, duration, delay);
}

static inline void plexus_set_r(Plexus *plexus, uint32_t easetype, int duration, int delay, int inr, int outr)
{
    nemoshow_item_set_width(plexus->one, outr * 2);
    nemoshow_item_set_height(plexus->one, outr * 2);

    plexus->inr = inr;
    plexus->outr = outr;
}

#define FRAME_ROUND 4.0

#define FRAME_COLOR0 0xEA562DFF
#define FRAME_COLOR1 BLACK
#define FRAME_COLOR2 0x35FFFFFF
#define FRAME_COLOR3 0x5c8affff
#define FRAME_COLOR4 WHITE
#define FRAME_COLOR5 0xdf41a1ff

typedef struct _Frameface Frameface;
struct _Frameface {
    bool is_full;
    int width, height;
    struct showone *gradient;

    NemoWidget *widget;
    struct showone *one;
};

void frameface_destroy(Frameface *face)
{
    nemoshow_one_destroy(face->gradient);
    nemowidget_destroy(face->widget);
    free(face);
}

static inline Frameface *frameface_create(NemoWidget *parent, int width, int height, int gap)
{
    Frameface *face = (Frameface *)calloc(sizeof(Frameface), 1);
    face->width = width;
    face->height = height;

    struct showone *gradient;
    face->gradient = gradient =
        LINEAR_GRADIENT_CREATE(0, 0, width, height);
    STOP_CREATE(gradient, FRAME_COLOR0, 0.0);
    STOP_CREATE(gradient, BLACK, 0.5);
    STOP_CREATE(gradient, FRAME_COLOR1, 1.0);

    NemoWidget *widget;
    face->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event(widget, false);
    nemowidget_enable_event_repeat(widget, true);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

    struct showone *one;
    face->one = one = RRECT_CREATE(canvas,
            width - gap * 2, height - gap * 2, FRAME_ROUND, FRAME_ROUND);
    nemoshow_item_set_stroke_color(one, RGBA(MIDNIGHTBLUE));
    nemoshow_item_set_stroke_width(one, gap * 2);
    nemoshow_item_set_shader(one, gradient);
    nemoshow_item_translate(one, gap, gap);

    return face;
}

static inline void frameface_gradient_motion(Frameface *face, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        NemoMotion *m = nemomotion_create(face->one->show, easetype, duration, delay);
        nemomotion_attach(m, 0.5,
                ONE_CHILD0(face->gradient), "fill", WHITE,
                ONE_CHILD1(face->gradient), "fill", WHITE,
                ONE_CHILD2(face->gradient), "fill", WHITE,
                NULL);
        if (face->is_full) {
            nemomotion_attach(m, 1.0,
                    ONE_CHILD0(face->gradient), "fill", BLACK,
                    ONE_CHILD1(face->gradient), "fill", BLACK,
                    ONE_CHILD2(face->gradient), "fill", BLACK,
                    NULL);
        } else {
            nemomotion_attach(m, 1.0,
                    ONE_CHILD0(face->gradient), "fill", FRAME_COLOR0,
                    ONE_CHILD1(face->gradient), "fill", FRAME_COLOR1,
                    ONE_CHILD2(face->gradient), "fill", FRAME_COLOR2,
                    NULL);
        }
        nemomotion_run(m);
    } else {
        nemoshow_stop_set_color(ONE_CHILD0(face->gradient), RGBA(FRAME_COLOR0));
        nemoshow_stop_set_color(ONE_CHILD2(face->gradient), RGBA(FRAME_COLOR1));
    }
}

static inline void frameface_go_full(Frameface *face)
{
    face->is_full = true;
    NemoMotion *m = nemomotion_create(face->one->show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 0.5,
            ONE_CHILD0(face->gradient), "fill", FRAME_COLOR3,
            ONE_CHILD1(face->gradient), "fill", FRAME_COLOR4,
            ONE_CHILD2(face->gradient), "fill", FRAME_COLOR5,
            NULL);
    nemomotion_attach(m, 1.0,
            ONE_CHILD0(face->gradient), "fill", BLACK,
            ONE_CHILD1(face->gradient), "fill", BLACK,
            ONE_CHILD2(face->gradient), "fill", BLACK,
            NULL);
    nemomotion_run(m);

    _nemoshow_item_motion(face->one, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "ox", 0.0, "oy", 0.0, NULL);
}

static inline void frameface_go_normal(Frameface *face)
{
    face->is_full = false;
    NemoMotion *m = nemomotion_create(face->one->show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 0.5,
            ONE_CHILD0(face->gradient), "fill", FRAME_COLOR3,
            ONE_CHILD1(face->gradient), "fill", FRAME_COLOR4,
            ONE_CHILD2(face->gradient), "fill", FRAME_COLOR5,
            NULL);
    nemomotion_attach(m, 1.0,
            ONE_CHILD0(face->gradient), "fill", FRAME_COLOR0,
            ONE_CHILD1(face->gradient), "fill", FRAME_COLOR1,
            ONE_CHILD2(face->gradient), "fill", FRAME_COLOR2,
            NULL);
    nemomotion_run(m);

    // XXX: needs 1000 delay because contents can be sticked out
    // when frame_go_normal is called.
    _nemoshow_item_motion(face->one, NEMOEASE_CUBIC_OUT_TYPE, 500, 1000,
            "ox", FRAME_ROUND, "oy", FRAME_ROUND, NULL);
}

static inline void frameface_hide(Frameface *face)
{
    nemowidget_hide(face->widget, 0, 0, 0);
    nemowidget_set_alpha(face->widget, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    frameface_gradient_motion(face, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
}

static inline void frameface_show(Frameface *face)
{
    nemowidget_show(face->widget, 0, 0, 0);
    nemowidget_set_alpha(face->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
    frameface_gradient_motion(face, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
}

typedef struct _Frameback Frameback;
struct _Frameback {
    bool is_full;
    int width, height;
    NemoWidget *parent;
    struct showone *gradient;

    NemoWidget *widget;
    struct showone *one;
};

void frameback_destroy(Frameback *back)
{
    nemoshow_one_destroy(back->gradient);
    nemowidget_destroy(back->widget);
    free(back);
}

Frameback *frameback_create(NemoWidget *parent, int width, int height)
{
    Frameback *back = (Frameback *)calloc(sizeof(Frameback), 1);

    struct showone *gradient;
    back->gradient = gradient =
        LINEAR_GRADIENT_CREATE(0, 0, width, height);
    STOP_CREATE(gradient, FRAME_COLOR0, 0.0);
    STOP_CREATE(gradient, BLACK, 0.5);
    STOP_CREATE(gradient, FRAME_COLOR1, 1.0);

    NemoWidget *widget;
    back->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_enable_event(widget, false);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    struct showone *canvas;
    canvas = nemowidget_get_canvas(widget);

    struct showone *one;
    back->one = one = RRECT_CREATE(canvas, width, height, FRAME_ROUND, FRAME_ROUND);
    nemoshow_item_set_fill_color(one, RGBA(MIDNIGHTBLUE));
    nemoshow_item_set_shader(one, gradient);

    return back;
}

static inline void frameback_gradient_motion(Frameback *back, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        NemoMotion *m = nemomotion_create(back->one->show, easetype, duration, delay);
        nemomotion_attach(m, 0.5,
                ONE_CHILD0(back->gradient), "fill", WHITE,
                ONE_CHILD1(back->gradient), "fill", WHITE,
                ONE_CHILD2(back->gradient), "fill", WHITE,
                NULL);
        if (back->is_full) {
            nemomotion_attach(m, 1.0,
                    ONE_CHILD0(back->gradient), "fill", BLACK,
                    ONE_CHILD1(back->gradient), "fill", BLACK,
                    ONE_CHILD2(back->gradient), "fill", BLACK,
                    NULL);
        } else {
            nemomotion_attach(m, 1.0,
                    ONE_CHILD0(back->gradient), "fill", FRAME_COLOR0,
                    ONE_CHILD1(back->gradient), "fill", FRAME_COLOR1,
                    ONE_CHILD2(back->gradient), "fill", FRAME_COLOR2,
                    NULL);
        }
        nemomotion_run(m);
    } else {
        nemoshow_stop_set_color(ONE_CHILD0(back->gradient), RGBA(FRAME_COLOR0));
        nemoshow_stop_set_color(ONE_CHILD2(back->gradient), RGBA(FRAME_COLOR1));
    }
}

static inline void frameback_go_full(Frameback *back)
{
    back->is_full = true;
    NemoMotion *m = nemomotion_create(back->one->show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 0.5,
            ONE_CHILD0(back->gradient), "fill", FRAME_COLOR3,
            ONE_CHILD1(back->gradient), "fill", FRAME_COLOR4,
            ONE_CHILD2(back->gradient), "fill", FRAME_COLOR5,
            NULL);
    nemomotion_attach(m, 1.0,
            ONE_CHILD0(back->gradient), "fill", BLACK,
            ONE_CHILD1(back->gradient), "fill", BLACK,
            ONE_CHILD2(back->gradient), "fill", BLACK,
            NULL);
    nemomotion_run(m);

    _nemoshow_item_motion(back->one, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "ox", 0.0, "oy", 0.0, NULL);
}

static inline void frameback_go_normal(Frameback *back)
{
    back->is_full = false;
    NemoMotion *m = nemomotion_create(back->one->show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 0.5,
            ONE_CHILD0(back->gradient), "fill", FRAME_COLOR3,
            ONE_CHILD1(back->gradient), "fill", FRAME_COLOR4,
            ONE_CHILD2(back->gradient), "fill", FRAME_COLOR5,
            NULL);
    nemomotion_attach(m, 1.0,
            ONE_CHILD0(back->gradient), "fill", FRAME_COLOR0,
            ONE_CHILD1(back->gradient), "fill", FRAME_COLOR1,
            ONE_CHILD2(back->gradient), "fill", FRAME_COLOR2,
            NULL);
    nemomotion_run(m);

    _nemoshow_item_motion(back->one, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "ox", FRAME_ROUND, "oy", FRAME_ROUND, NULL);
}

static inline void frameback_hide(Frameback *back)
{
    nemowidget_hide(back->widget, 0, 0, 0);
    nemowidget_set_alpha(back->widget, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0.0);
    frameback_gradient_motion(back, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
}

static inline void frameback_show(Frameback *back)
{
    nemowidget_show(back->widget, 0, 0, 0);
    nemowidget_set_alpha(back->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
    frameback_gradient_motion(back, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
}

typedef struct _Content Content;
struct _Content {
    NemoWidget *widget;
    List *groups;
    //struct showone *group;
};

typedef struct _Frame Frame;
struct _Frame {
    int width, height;
    Frameface *face;
    Frameback *back;

    int content_width, content_height, content_gap;
    struct {
        double x, y, w, h;
        double group_sx, group_sy;
    } content_real;
    List *contents;
};

static inline double frame_get_aspect_ratio(Frame *frame)
{
    return (double)frame->width/frame->height;
}

static inline void frame_get_content_wh(Frame *frame, int *w, int *h)
{
    if (w) *w = frame->content_width;
    if (h) *h = frame->content_height;
}

static inline void frame_destroy(Frame *frame)
{
    frameback_destroy(frame->back);
    frameface_destroy(frame->face);
    Content *content;
    LIST_FREE(frame->contents, content) {
        nemowidget_destroy(content->widget);
        free(content);
    }

    free(frame);
}

static inline Frame *frame_create(NemoWidget *parent, int width, int height, int content_gap)
{
    Frame *frame = (Frame *)calloc(sizeof(Frame), 1);
    frame->width = width;
    frame->height = height;
    frame->content_gap = content_gap;
    frame->content_width = frame->width - frame->content_gap * 2;
    frame->content_height = frame->height - frame->content_gap * 2;
    frame->content_real.x = frame->content_gap;
    frame->content_real.y = frame->content_gap;
    frame->content_real.w = frame->content_width;
    frame->content_real.h = frame->content_height;
    frame->content_real.group_sx = 1.0;
    frame->content_real.group_sy = 1.0;
    frame->back = frameback_create(parent, width, height);
    frame->face = frameface_create(parent, width, height, content_gap);

    return frame;
}

static inline void frame_gradient_motion(Frame *frame, uint32_t easetype, int duration, int delay)
{
    frameface_gradient_motion(frame->face, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
    //frameback_gradient_motion(frame->back, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);
}

// XXX: Because group & widget was scaled down to maintain it's original ratio
// and event coorinates was not scaled down.
static inline void frame_util_transform_to_group(struct showone *group, double ex, double ey, double *x, double *y)
{
    double tx, ty, sx, sy, w, h;
    tx = nemoshow_item_get_translate_x(group);
    ty = nemoshow_item_get_translate_y(group);
    sx = nemoshow_item_get_scale_x(group);
    sy = nemoshow_item_get_scale_y(group);
    w = nemoshow_item_get_width(group);
    h = nemoshow_item_get_height(group);
    w = w * sx;
    h = h * sy;

    /*
     *  widget & window have same size
    i(x + w < ex || ey < ty || ty + h < ey) {
        return;
    }
     */

    if (x) *x = (ex - tx) * (1.0/sx);
    if (y) *y = (ey - ty) * (1.0/sy);
}

static inline void frame_go_full(Frame *frame, int fs_width, int fs_height)
{
    frameback_go_full(frame->back);
    frameface_go_full(frame->face);

    double ratio0, ratio1, ratio;
    ratio0 = (double)frame->content_width/frame->content_height;
    ratio1 = (double)fs_width/fs_height;
    ratio = ratio0/ratio1;

    if (ratio > 1.0) {
        ratio = 1.0/ratio;

        frame->content_real.x = 0.0;
        frame->content_real.y = frame->content_height * (1.0 - ratio)/2.0;
        frame->content_real.w = frame->content_width;
        frame->content_real.h = frame->content_height * ratio;
        frame->content_real.group_sx = 1.0;
        frame->content_real.group_sy = ratio;
    } else {
        frame->content_real.x = frame->content_width * (1.0 - ratio)/2.0;
        frame->content_real.y = 0.0;
        frame->content_real.w = frame->content_width * ratio;
        frame->content_real.h = frame->content_height;
        frame->content_real.group_sx = ratio;
        frame->content_real.group_sy = 1.0;
    }

    List *l;
    Content *content;
    LIST_FOR_EACH(frame->contents, l, content) {
        nemowidget_resize(content->widget,
                frame->content_real.w, frame->content_real.h);
        nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                frame->content_real.x, frame->content_real.y);

        // XXX: If vector canvas, internal group's scale never follow its widget.
        List *ll;
        struct showone *group;
        LIST_FOR_EACH(content->groups, ll, group) {
            _nemoshow_item_motion(group,
                    NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    "sx", frame->content_real.group_sx,
                    "sy", frame->content_real.group_sy,
                    NULL);
        }
    }
}

static inline void frame_go_normal(Frame *frame)
{
    frameback_go_normal(frame->back);
    frameface_go_normal(frame->face);

    frame->content_real.x = frame->content_gap;
    frame->content_real.y = frame->content_gap;
    frame->content_real.w = frame->content_width;
    frame->content_real.h = frame->content_height;
    frame->content_real.group_sx = 1.0;
    frame->content_real.group_sy = 1.0;

    List *l;
    Content *content;
    LIST_FOR_EACH(frame->contents, l, content) {
        nemowidget_resize(content->widget,
                frame->content_real.w, frame->content_real.h);
        nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                frame->content_real.x, frame->content_real.y);

        // XXX: If vector canvas, internal group's scale never follow its widget.
        List *ll;
        struct showone *group;
        LIST_FOR_EACH(content->groups, ll, group) {
            _nemoshow_item_motion(group,
                    NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    "sx", frame->content_real.group_sx,
                    "sy", frame->content_real.group_sy,
                    NULL);
        }
    }
}

static inline void frame_go_widget_next(Frame *frame, int outidx, int inidx)
{
    int cnt = list_count(frame->contents);
    if (cnt <= 1) {
        ERR("It needs at least 2 contents to go animation");
        return;
    }
    if (outidx < 0 || outidx >= cnt || inidx < 0 || inidx >= cnt)  {
        ERR("outidx(%d) or inidx(%d) is not correct\n", outidx, inidx);
    }

    Content *in = (Content *)LIST_DATA(list_get_nth(frame->contents, inidx));
    Content *out = (Content *)LIST_DATA(list_get_nth(frame->contents, outidx));

    nemowidget_translate(in->widget, 0, 0, 0, frame->width, frame->content_real.y);
    nemowidget_scale(in->widget, 0, 0, 0, 1.0, 1.0);
    nemowidget_set_alpha(in->widget, 0, 0, 0, 0.25);
    nemowidget_translate(in->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            frame->content_real.x, frame->content_real.y);
    nemowidget_set_alpha(in->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 1.0);

    nemowidget_scale(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.001, 1.0);
    nemowidget_translate(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0,
            frame->content_gap);
    nemowidget_set_alpha(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 0.0);
}

static inline void frame_go_widget_prev(Frame *frame, int outidx, int inidx)
{
    int cnt = list_count(frame->contents);
    if (cnt <= 1) {
        ERR("It needs at least 2 contents to go animation");
        return;
    }
    if (outidx < 0 || outidx >= cnt || inidx < 0 || inidx >= cnt)  {
        ERR("outidx(%d) or inidx(%d) is not correct\n", outidx, inidx);
    }

    Content *in = (Content *)LIST_DATA(list_get_nth(frame->contents, inidx));
    Content *out = (Content *)LIST_DATA(list_get_nth(frame->contents, outidx));

    nemowidget_translate(in->widget, 0, 0, 0, -frame->width * 2, frame->content_real.y);
    nemowidget_scale(in->widget, 0, 0, 0, 2.0, 1.0);
    nemowidget_set_alpha(in->widget, 0, 0, 0, 0.5);
    nemowidget_translate(in->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            frame->content_real.x, frame->content_real.y);
    nemowidget_scale(in->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 1.0, 1.0);
    nemowidget_set_alpha(in->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 1.0);

    nemowidget_translate(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            frame->width, frame->content_gap);
    nemowidget_scale(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0, 0.001, 1.0);
    nemowidget_set_alpha(out->widget, NEMOEASE_CUBIC_INOUT_TYPE, 500, 250, 0.0);
}

static inline void frame_hide(Frame *frame)
{
    frameback_hide(frame->back);
    frameface_hide(frame->face);
    List *l;
    Content *content;
    LIST_FOR_EACH(frame->contents, l, content) {
        nemowidget_hide(content->widget, 0, 0, 0);
    }
}

static inline void frame_show(Frame *frame)
{
    frameback_show(frame->back);
    frameface_show(frame->face);
}

static inline void frame_content_hide(Frame *frame, int idx)
{
    int cnt = list_count(frame->contents);
    if (idx >= cnt) {
        ERR("index(%d) is bigger than number(%d) of contents", idx, cnt);
    }

    if (0 <= idx) {
        Content *content = (Content *)LIST_DATA(list_get_nth(frame->contents, idx));

        nemowidget_set_alpha(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0);
        nemowidget_scale(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0, 0.0);
        nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                frame->content_real.x, frame->height/2);
    } else {
        List *l;
        Content *content;
        LIST_FOR_EACH(frame->contents, l, content) {
            nemowidget_set_alpha(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 0.0);
            nemowidget_scale(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0, 0.0);
            nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    frame->content_real.x, frame->height/2);
        }
    }
}

static inline void frame_content_show(Frame *frame, int idx)
{
    int cnt = list_count(frame->contents);
    if (idx >= cnt) {
        ERR("index(%d) is bigger than number(%d) of contents", idx, cnt);
    }

    if (idx >= 0) {
        Content *content = (Content *)LIST_DATA(list_get_nth(frame->contents, idx));

        nemowidget_show(content->widget, 0, 0, 0);
        nemowidget_set_alpha(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
        nemowidget_scale(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0, 1.0);
        nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                frame->content_real.x, frame->content_real.y);
    } else {
        List *l;
        Content *content;
        LIST_FOR_EACH(frame->contents, l, content) {
            nemowidget_show(content->widget, 0, 0, 0);
            nemowidget_set_alpha(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0);
            nemowidget_scale(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0, 1.0, 1.0);
            nemowidget_translate(content->widget, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
                    frame->content_real.x, frame->content_real.y);
        }
    }
}

static inline void frame_attach_event(Frame *frame, NemoWidget_Func func, void *userdata)
{
    nemowidget_append_callback(frame->face->widget, "event", func, userdata);
}

// XXX: Widget should be created with size of content width, height
// XXX: If vector canvas, internal vector objects' size (scale) will not be changed.
//  So please use frame_append_widget_group
static inline Content *frame_append_widget(Frame *frame, NemoWidget *widget)
{
    List *l;
    Content *content;
    LIST_FOR_EACH(frame->contents, l, content) {
        if (content->widget == widget) {
            ERR("Already appended widget: %p(%s)", widget, widget->klass->name);
            return content;
        }
    }

    content = (Content *)calloc(sizeof(Content), 1);
    content->widget = widget;
    frame->contents = list_append(frame->contents, content);

    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    nemowidget_scale(widget, 0, 0, 0, 0.0, 1.0);
    nemowidget_translate(content->widget, 0, 0, 0,
            frame->content_width/2, frame->content_real.y);
    nemowidget_resize(widget, frame->content_width, frame->content_height);
    nemowidget_stack_above(frame->face->widget, widget);

    return content;
}


static inline void frame_remove_widget(Frame *frame, NemoWidget *widget)
{
    List *l;
    Content *content;
    LIST_FOR_EACH(frame->contents, l, content) {
        if (content->widget == widget) {
            break;
        }
    }

    if (!content) return;

    frame->contents = list_remove(frame->contents, content);
    free(content);
}

// XXX: Widget should be created with size of content width, height
static inline Content *frame_append_widget_group(Frame *frame, NemoWidget *widget, struct showone *group)
{
    Content *content = frame_append_widget(frame, widget);
    if (widget->klass->type != NEMOWIDGET_TYPE_VECTOR) {
        ERR("Widget is not vector type and cannot append group");
        return content;
    }
    content->groups = list_append(content->groups, group);

    return content;
}

typedef struct _PolyCoord PolyCoord;
struct _PolyCoord {
    uint32_t time;
    int x;
    int y;
};

typedef struct _PolyLine PolyLine;
struct _PolyLine {
    struct nemotool *tool;
    uint32_t tag;
    struct showone *blur;
    struct showone *group;
    uint32_t color;
    int min_dist;
    int width;
    List *dots;
    List *ones;

    struct showone *one;
    struct nemotimer *timer;
};

static inline void polyline_set_tag(PolyLine *pl, uint32_t tag)
{
    pl->tag = tag;
}

static inline PolyLine *polyline_create(struct nemotool *tool, struct showone *parent, uint32_t color, int width)
{
    PolyLine *pl = (PolyLine *)calloc(sizeof(PolyLine), 1);
    pl->tool = tool;
    pl->group = GROUP_CREATE(parent);
    pl->color = color;
    pl->width = width;
    pl->min_dist = 5.0;
    return pl;
}

static inline void polyline_set_min_distance(PolyLine *pl, int min_dist)
{
    pl->min_dist = min_dist;
}

static inline void polyline_set_blur(PolyLine *pl, const char *style, double r)
{
    if (pl->blur) nemoshow_one_destroy(pl->blur);
    pl->blur = BLUR_CREATE(style, r);
}

static inline void polyline_destroy(PolyLine *pl)
{
    PolyCoord *dot;
    if (pl->blur) nemoshow_one_destroy(pl->blur);
    struct showone *one;
    LIST_FREE(pl->dots, dot) free(dot);
    LIST_FREE(pl->ones, one) nemoshow_one_destroy(one);
    if (pl->one) nemoshow_one_destroy(pl->one);
    nemoshow_one_destroy(pl->group);
    free(pl);
}

static inline void _polyline_destroy(struct nemotimer *timer, void *userdata)
{
    PolyLine *pl = (PolyLine *)userdata;
    polyline_destroy(pl);
    nemotimer_destroy(timer);
}

static inline void polyline_hide_destroy(PolyLine *pl, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(pl->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    if (duration > 0) {
        TOOL_ADD_TIMER(pl->tool, duration + delay, _polyline_destroy, pl);
    } else {
        polyline_destroy(pl);
    }
}

static inline void polyline_add_dot(PolyLine *pl, int x, int y)
{
    PolyCoord *prev = (PolyCoord *)LIST_DATA(LIST_LAST(pl->dots));

    if (!prev) {
        PolyCoord *dot = (PolyCoord *)malloc(sizeof(PolyCoord));
        dot->x = x;
        dot->y = y;
        pl->dots = list_append(pl->dots, dot);
        return;
    }
    if (abs(x - prev->x) <= pl->min_dist && abs(y - prev->y) <= pl->min_dist) return;

    PolyCoord *dot = (PolyCoord *)malloc(sizeof(PolyCoord));
    dot->x = x;
    dot->y = y;
    pl->dots = list_append(pl->dots, dot);

    struct showone *one;
    one = PATH_LINE_CREATE(pl->group, prev->x, prev->y, dot->x, dot->y);
    nemoshow_item_set_stroke_width(one, pl->width);
    nemoshow_item_set_stroke_color(one, RGBA(pl->color));
    if (pl->blur) nemoshow_item_set_filter(one, pl->blur);
    pl->ones = list_append(pl->ones, one);
}

static inline void polyline_add_dot_custom(PolyLine *pl, int x, int y, double alpha, uint32_t color)
{
    polyline_add_dot(pl, x, y);

    struct showone *one;
    one = (struct showone *)LIST_DATA(LIST_LAST(pl->ones));
    if (!one) return;
    nemoshow_item_set_alpha(one, alpha);
    nemoshow_item_set_stroke_color(one, RGBA(color));
}

static inline void polyline_done(PolyLine *pl)
{
    struct showone *one;
    LIST_FREE(pl->ones, one) nemoshow_one_destroy(one);

    if (pl->one) nemoshow_one_destroy(pl->one);

    if (list_count(pl->dots) < 2)  return;

    pl->one = one = PATH_CREATE(pl->group);
    nemoshow_item_set_stroke_width(one, pl->width);
    nemoshow_item_set_stroke_color(one, RGBA(pl->color));
    if (pl->blur) nemoshow_item_set_filter(one, pl->blur);

    List *l;
    PolyCoord *dot;
    LIST_FOR_EACH(pl->dots, l, dot) {
        if (LIST_DATA(LIST_FIRST(pl->dots)) == dot) {
            nemoshow_item_path_moveto(one, dot->x, dot->y);
        } else
            nemoshow_item_path_lineto(one, dot->x, dot->y);
    }
}

typedef struct _PolyBezier PolyBezier;
struct _PolyBezier {
    struct nemotool *tool;
    uint32_t tag;
    struct showone *blur;
    struct showone *group;
    uint32_t color;
    int width;
    int min_dist;
    List *dots;
    List *ones;

    struct showone *one;
    struct nemotimer *timer;
};

static inline void polybezier_set_tag(PolyBezier *pl, long tag)
{
    pl->tag = tag;
}

static inline PolyBezier *polybezier_create(struct nemotool *tool, struct showone *parent, uint32_t color, int width)
{
    PolyBezier *pl = (PolyBezier *)calloc(sizeof(PolyBezier), 1);
    pl->tool = tool;
    pl->group = GROUP_CREATE(parent);
    pl->color = color;
    pl->width = width;
    pl->min_dist = 5.0;
    return pl;
}

static inline void polybezier_set_min_distance(PolyBezier *pl, int min_dist)
{
    pl->min_dist = min_dist;
}

static inline void polybezier_set_blur(PolyBezier *pl, const char *style, double r)
{
    if (pl->blur) nemoshow_one_destroy(pl->blur);
    pl->blur = BLUR_CREATE(style, r);
}

static inline void polybezier_destroy(PolyBezier *pl)
{
    PolyCoord *dot;
    if (pl->blur) nemoshow_one_destroy(pl->blur);
    struct showone *one;
    LIST_FREE(pl->dots, dot) free(dot);
    LIST_FREE(pl->ones, one) nemoshow_one_destroy(one);
    if (pl->one) nemoshow_one_destroy(pl->one);
    nemoshow_one_destroy(pl->group);
    free(pl);
}

static inline void _polybezier_destroy(struct nemotimer *timer, void *userdata)
{
    PolyBezier *pl = (PolyBezier *)userdata;
    polybezier_destroy(pl);
    nemotimer_destroy(timer);
}

static inline void polybezier_hide_destroy(PolyBezier *pl, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(pl->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
    if (duration > 0) {
        TOOL_ADD_TIMER(pl->tool, duration + delay, _polybezier_destroy, pl);
    } else {
        polybezier_destroy(pl);
    }
}

static inline void polybezier_add_dot(PolyBezier *pl, int x, int y)
{
    PolyCoord *prev = (PolyCoord *)LIST_DATA(LIST_LAST(pl->dots));

    if (!prev) {
        PolyCoord *dot = (PolyCoord *)malloc(sizeof(PolyCoord));
        dot->x = x;
        dot->y = y;
        pl->dots = list_append(pl->dots, dot);
        return;
    }
    if (abs(x - prev->x) <= pl->min_dist && abs(y - prev->y) <= pl->min_dist) return;

    PolyCoord *dot = (PolyCoord *)malloc(sizeof(PolyCoord));
    dot->x = x;
    dot->y = y;
    pl->dots = list_append(pl->dots, dot);

    struct showone *one;
    int cnt = list_count(pl->dots);
    if (cnt >= 3) {
        PolyCoord *p0, *p1, *p2;
        p0 = (PolyCoord *)LIST_DATA(list_get_nth(pl->dots, cnt - 3));
        p1 = (PolyCoord *)LIST_DATA(list_get_nth(pl->dots, cnt - 2));
        p2 = (PolyCoord *)LIST_DATA(list_get_nth(pl->dots, cnt - 1));

        int x0, y0, x1, y1;
        x0 = (p1->x + p0->x)/2;
        y0 = (p1->y + p0->y)/2;
        x1 = (p2->x + p1->x)/2;
        y1 = (p2->y + p1->y)/2;

        one = PATH_BEZIER_CREATE(pl->group,
                x0, y0, p1->x, p1->y, x1, y1);
        nemoshow_item_set_stroke_width(one, pl->width);
        nemoshow_item_set_stroke_color(one, RGBA(pl->color));
        if (pl->blur) nemoshow_item_set_filter(one, pl->blur);
        pl->ones = list_append(pl->ones, one);
    }
}

static inline void polybezier_done(PolyBezier *pl)
{
    struct showone *one;
    LIST_FREE(pl->ones, one) nemoshow_one_destroy(one);

    if (pl->one) nemoshow_one_destroy(pl->one);

    if (list_count(pl->dots) < 3)  return;

    pl->one = one = PATH_CREATE(pl->group);
    nemoshow_item_set_stroke_width(one, pl->width);
    nemoshow_item_set_stroke_color(one, RGBA(pl->color));
    if (pl->blur) nemoshow_item_set_filter(one, pl->blur);

    char *prev = NULL;
    char *buf = NULL;

    List *l;
    PolyCoord *p0 = NULL, *p1 = NULL, *p2;
    LIST_FOR_EACH(pl->dots, l, p2) {
        if (p0 && p1 && p2) {
            int x0, y0, x1, y1;
            x0 = (p1->x + p0->x)/2;
            y0 = (p1->y + p0->y)/2;
            x1 = (p2->x + p1->x)/2;
            y1 = (p2->y + p1->y)/2;

            if (prev) {
                buf = strdup_printf("%s M%d %d Q%d %d %d %d",
                        prev, x0, y0, p1->x, p1->y, x1, y1);
                free(prev);
            } else {
                buf = strdup_printf("M%d %d Q%d %d %d %d",
                        x0, y0, p1->x, p1->y, x1, y1);
            }
            prev = strdup_printf(buf);
            free(buf);
        }
        p0 = p1;
        p1 = p2;
    }
    nemoshow_item_path_cmd(one, prev);
}

typedef struct _SketchDot SketchDot;
struct _SketchDot {
    struct nemotool *tool;
    struct showone *parent;
    uint32_t color;
    struct showone *blur;
    struct showone *group;
    List *ones;
    int idx;
};

SketchDot *sketch_create_dot(struct nemotool *tool, struct showone *parent, int r, uint32_t color, int cnt)
{
    SketchDot *dot = (SketchDot *)malloc(sizeof(SketchDot));
    dot->tool = tool;
    dot->parent = parent;
    dot->color = color;
    dot->blur = BLUR_CREATE("solid", 15);
    dot->ones = NULL;
    dot->idx = 0;

    struct showone *group;
    dot->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    int i;
    for (i = 0 ; i < cnt ; i++) {
        struct showone *one;
        one = CIRCLE_CREATE(group, r);
        nemoshow_item_set_fill_color(one, RGBA(color));
        nemoshow_item_set_filter(one, dot->blur);
        nemoshow_item_set_alpha(one, 0.0);
        nemoshow_item_scale(one, 0.0, 0.0);
        dot->ones = list_append(dot->ones, one);
    }

    return dot;
}

void sketch_dot_show(SketchDot *dot, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(dot->group, easetype, duration, delay,
            "alpha", 1.0, NULL);
}

void sketch_dot_hide(SketchDot *dot, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(dot->group, easetype, duration, delay,
            "alpha", 0.0, NULL);
}

void sketch_dot_destroy(SketchDot *dot)
{
    nemoshow_one_destroy(dot->blur);
    nemoshow_one_destroy(dot->group);
    free(dot);
}

void _sketch_dot_destroy(struct nemotimer *timer, void *userdata)
{
    SketchDot *dot = (SketchDot *)userdata;
    sketch_dot_destroy(dot);
    nemotimer_destroy(timer);
}

void sketch_dot_hide_destroy(SketchDot *dot, uint32_t easetype, int duration, int delay)
{
    sketch_dot_hide(dot, easetype, duration, delay);
    if (duration > 0) {
        TOOL_ADD_TIMER(dot->tool, duration + delay, _sketch_dot_destroy, dot);
    } else {
        sketch_dot_destroy(dot);
    }
}

void sketch_dot_translate(SketchDot *dot, int tx, int ty)
{
    int dur = 500;

    struct showone *one;
    one = (struct showone *)LIST_DATA(list_get_nth(dot->ones, dot->idx));
    nemoshow_item_translate(one, tx, ty);
    NemoMotion *m;
    m = nemomotion_create(one->show, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0);
    nemomotion_attach(m, 0.5,
            one, "alpha", 1.0,
            one, "sx", 1.0,
            one, "sy", 1.0,
            NULL);
    nemomotion_attach(m, 1.0,
            one, "alpha", 0.5,
            one, "sx", 0.0,
            one, "sy", 0.0,
            NULL);
    nemomotion_run(m);
    dot->idx++;
    if (dot->idx >= list_count(dot->ones)) dot->idx = 0;
}

typedef struct _Sketch Sketch;
struct _Sketch {
    bool enable;
    int dot_cnt;
    struct nemoshow *show;
    struct nemotool *tool;
    int w, h;
    NemoWidget *widget;
    struct showone *group;
    List *lines;

    List *dots;

    uint32_t color;
    int size;
    bool blur_on;
    uint32_t min_dist;
};

static inline void _sketch_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    Sketch *sketch = (Sketch *)userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    frame_util_transform_to_group(sketch->group, ex, ey, &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        if (sketch->dot_cnt > 0) {
            SketchDot *dot = sketch_create_dot(sketch->tool, sketch->group, 10, sketch->color, sketch->dot_cnt);
            sketch_dot_show(dot, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
            sketch->dots = list_append(sketch->dots, dot);
            nemowidget_grab_set_data(grab, "dot", dot);
        }

        PolyBezier *pl = polybezier_create(nemowidget_get_tool(widget),
                sketch->group, sketch->color, sketch->size);
        polybezier_set_min_distance(pl, sketch->min_dist);
        if (sketch->blur_on) polybezier_set_blur(pl, "solid", 4);
        sketch->lines = list_append(sketch->lines, pl);
        polybezier_add_dot(pl, ex, ey);
        nemowidget_grab_set_data(grab, "pl", pl);
    } else if (nemoshow_event_is_motion(show, event)) {
        PolyBezier *pl = (PolyBezier *)nemowidget_grab_get_data(grab, "pl");
        if (pl) {
            polybezier_add_dot(pl, ex, ey);
            nemoshow_dispatch_frame(sketch->show);
        }

        SketchDot *dot = (SketchDot *)nemowidget_grab_get_data(grab, "dot");
        if (dot) {
            sketch_dot_translate(dot, ex, ey);
        }
    } else if (nemoshow_event_is_up(show, event)) {
        PolyBezier *pl = (PolyBezier *)nemowidget_grab_get_data(grab, "pl");
        if (pl) {
            polybezier_add_dot(pl, ex, ey);
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            polybezier_set_tag(pl, ts.tv_sec);
            polybezier_done(pl);
        }

        SketchDot *dot = (SketchDot *)nemowidget_grab_get_data(grab, "dot");
        if (dot) {
            sketch->dots = list_remove(sketch->dots, dot);
            sketch_dot_hide_destroy(dot, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
        }
    }
}

static inline void __sketch_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showevent *event = (struct showevent *)info;

    Sketch *sketch = (Sketch *)userdata;
    if (!sketch->enable) return;
    if (nemoshow_event_is_done(event))  return;

    if (nemoshow_event_is_down(show, event)) {
        nemowidget_create_grab(widget, event, _sketch_grab_event, sketch);
    }
}

static inline Sketch *sketch_create(NemoWidget *parent, int w, int h)
{
    Sketch *sketch = (Sketch *)calloc(sizeof(Sketch), 1);
    sketch->show = nemowidget_get_show(parent);
    sketch->tool = nemowidget_get_tool(parent);
    sketch->w = w;
    sketch->h = h;
    sketch->color = WHITE;
    sketch->size = 2;
    sketch->blur_on = true;
    sketch->enable = false;
    sketch->min_dist = 5.0;
    sketch->dot_cnt = 0;

    struct showone *group;
    NemoWidget *widget;
    sketch->widget = widget = nemowidget_create_vector(parent, w, h);
    nemowidget_append_callback(widget, "event", __sketch_event, sketch);
    sketch->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    nemoshow_item_set_width(sketch->group, w);
    nemoshow_item_set_height(sketch->group, h);
    nemoshow_item_set_alpha(group, 0.0);

    return sketch;
}

static inline void sketch_set_min_distance(Sketch *sketch, uint32_t min_dist)
{
    sketch->min_dist = min_dist;
}

static inline void sketch_enable(Sketch *sketch, bool enable)
{
    sketch->enable = !!enable;
}

static inline void sketch_set_color(Sketch *sketch, uint32_t color)
{
    sketch->color = color;
}

static inline void sketch_set_size(Sketch *sketch, int size)
{
    sketch->size = size;
}

static inline void sketch_enable_blur(Sketch *sketch, bool blur_on)
{
    sketch->blur_on = blur_on;
}

static inline void sketch_set_dot_count(Sketch *sketch, int cnt)
{
    sketch->dot_cnt= cnt;
}

static inline void sketch_attach_event(Sketch *sketch, NemoWidget_Func func, void *userdata)
{
    nemowidget_append_callback(sketch->widget, "event", func, userdata);
}

static inline void sketch_destroy(Sketch *sketch)
{
    PolyBezier *pl;
    LIST_FREE(sketch->lines, pl) {
        polybezier_destroy(pl);
    }
    SketchDot *dot;
    LIST_FREE(sketch->dots, dot) {
        sketch_dot_destroy(dot);
    }
    nemoshow_one_destroy(sketch->group);
    nemowidget_destroy(sketch->widget);
    free(sketch);
}

static inline void sketch_show(Sketch *sketch, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(sketch->widget, 0, 0, 0);
    if (duration > 0) {
        _nemoshow_item_motion(sketch->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(sketch->group, 1.0);
    }
}

static inline void sketch_hide(Sketch *sketch, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(sketch->widget, 0, 0, 0);
    if (duration > 0) {
        _nemoshow_item_motion(sketch->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(sketch->group, 0.0);
    }
}

static inline void sketch_translate(Sketch *sketch, double tx, double ty)
{
    nemowidget_translate(sketch->widget, 0, 0, 0, tx, ty);
}

static inline void sketch_undo(Sketch *sketch, int cnt)
{
    PolyBezier *pl;
    if (cnt <= 0) {
        LIST_FREE(sketch->lines, pl) {
            polybezier_hide_destroy(pl, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
        }
    } else {
        pl = (PolyBezier *)LIST_DATA(LIST_LAST(sketch->lines));
        if (pl) {
            sketch->lines = list_remove(sketch->lines, pl);
            polybezier_hide_destroy(pl, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
        }
    }
}

static inline void sketch_remove_old(Sketch *sketch, long time)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    PolyBezier *pl;
    List *l, *ll;
    LIST_FOR_EACH_SAFE(sketch->lines, l, ll, pl) {
        if (!pl->one) continue;
        if (pl->tag + time/1000 <= ts.tv_sec) {
            sketch->lines = list_remove(sketch->lines, pl);
            polybezier_hide_destroy(pl, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
        }
    }
}

typedef struct _Gallery Gallery;
typedef struct _GalleryItem GalleryItem;

struct _GalleryItem {
    Gallery *gallery;
    int w, h;
    char *path;
    Image *img;
    struct nemotimer *hide_timer;
};

struct _Gallery {
    int mode; // 0, 1, (fit, full)
    struct nemotool *tool;
    int w, h;
    struct showone *group;
    struct showone *bg;
    List *items;
};

static inline GalleryItem *gallery_append_item(Gallery *gallery, const char *path)
{
    GalleryItem *it = (GalleryItem *)calloc(sizeof(GalleryItem), 1);
    it->gallery = gallery;
    it->path = strdup(path);
    it->img = image_create(gallery->group);
    image_set_alpha(it->img, 0, 0, 0, 0.0);
    gallery->items = list_append(gallery->items, it);

    return it;
}

static inline void gallery_remove_item(GalleryItem *it)
{
    Gallery *gallery = it->gallery;
    gallery->items = list_remove(gallery->items, it);
    image_destroy(it->img);
    free(it->path);
    free(it);
}

typedef struct _GalleryItemShowData GalleryItemShowData;
struct _GalleryItemShowData {
    uint32_t easetype;
    int duration;
    int delay;
};

static inline void _gallery_item_show_done(bool success, void *userdata)
{
    RET_IF(!success);
    GalleryItem *it = (GalleryItem *)userdata;
    image_set_alpha(it->img, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, 1.0);
}

static inline void gallery_item_show(GalleryItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->hide_timer) {
        nemotimer_destroy(it->hide_timer);
        it->hide_timer = NULL;
    }
    Gallery *gallery = it->gallery;
    if (gallery->mode == 0) {
        image_load_full(it->img, gallery->tool, it->path, gallery->w, gallery->h,
                _gallery_item_show_done, it);
    } else if (gallery->mode == 1) {
        image_load_fit(it->img, gallery->tool, it->path, gallery->w, gallery->h,
                _gallery_item_show_done, it);
    } else {
        ERR("Not supportd gallery mode: %d", gallery->mode);
    }
}

static inline void _gallery_item_hide_done(struct nemotimer *timer, void *userdata)
{
    GalleryItem *it = (GalleryItem *)userdata;
    image_unload(it->img);
    nemotimer_destroy(it->hide_timer);
    it->hide_timer = NULL;
}

static inline void gallery_item_hide(GalleryItem *it, uint32_t easetype, int duration, int delay)
{
    if (it->hide_timer) return;
    Gallery *gallery = it->gallery;
    image_load_cancel(it->img);
    image_set_alpha(it->img, easetype, duration, delay, 0.0);
    it->hide_timer = TOOL_ADD_TIMER(gallery->tool, duration + delay + 10,
            _gallery_item_hide_done, it);
}

static inline void gallery_show(Gallery *gallery, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(gallery->group, easetype, duration, delay,
                "alpha", 1.0, NULL);
    } else {
        nemoshow_item_set_alpha(gallery->group, 1.0);
    }
}

static inline void gallery_hide(Gallery *gallery, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(gallery->group, easetype, duration, delay,
                "alpha", 0.0, NULL);
    } else {
        nemoshow_item_set_alpha(gallery->group, 0.0);
    }
}

static inline Gallery *gallery_create(struct nemotool *tool, struct showone *parent, int w, int h)
{
    Gallery *gallery = (Gallery *)calloc(sizeof(Gallery), 1);
    gallery->tool = tool;
    gallery->w = w;
    gallery->h = h;
    struct showone *group;
    group = gallery->group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    struct showone *one;
    one = gallery->bg = RECT_CREATE(group, w, h);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));

    return gallery;
}

static inline void gallery_set_bg_color(Gallery *gallery, uint32_t color)
{
    nemoshow_item_set_fill_color(gallery->bg, RGBA(color));
    nemoshow_dispatch_frame(gallery->bg->show);
}

static inline void gallery_destroy(Gallery *gallery)
{
    List *l, *ll;
    GalleryItem *it;
    LIST_FOR_EACH_SAFE(gallery->items, l, ll, it) {
        gallery_remove_item(it);
    }

    nemoshow_one_destroy(gallery->bg);
    nemoshow_one_destroy(gallery->group);
    free(gallery);
}

typedef struct _ButtonIcon ButtonIcon;
struct _ButtonIcon
{
    int w, h;
    struct showone *one;
    struct showone *old;
};

typedef struct _Button Button;
struct _Button
{
    int w, h;
    struct showone *blur;
    struct showone *group;
    struct showone *event;
    struct showone *bg0;
    struct showone *bg;
    List *icons;
    void *userdata;
};

ButtonIcon *button_get_nth_icon(Button *btn, int idx)
{
    return (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
}

static inline Button *button_create(struct showone *parent, const char *id, uint32_t tag)
{
    Button *btn = (Button *)calloc(sizeof(Button), 1);
    btn->blur = BLUR_CREATE("solid", 10);
    btn->group = GROUP_CREATE(parent);
    nemoshow_item_scale(btn->group, 0.0, 0.0);
    nemoshow_item_set_alpha(btn->group, 0.0);

    struct showone *one;
    one = btn->event = RECT_CREATE(btn->group, 0, 0);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, id);
    nemoshow_one_set_tag(one, tag);
    nemoshow_one_set_userdata(one, btn);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    return btn;
}

static inline void button_hide(Button *btn, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "alpha", 0.0,
                "sx", 0.0, "sy", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(btn->group, 0.0);
        nemoshow_item_scale(btn->group, 0.0, 0.0);
    }
}

static inline void button_show(Button *btn, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
        _nemoshow_item_motion_bounce(btn->group, easetype, duration, delay,
                "sx", 1.15, 1.0, "sy", 1.15, 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(btn->group, 1.0);
        nemoshow_item_scale(btn->group, 1.0, 1.0);
    }
}

static inline void button_enable_bg(Button *btn, int r, uint32_t color)
{
    if (btn->w < r * 2) btn->w = r * 2;
    if (btn->h < r * 2) btn->h = r * 2;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    struct showone *one;
    one = CIRCLE_CREATE(btn->group, r);
    nemoshow_item_set_fill_color(one, RGBA(GRAY));
    nemoshow_item_set_alpha(one, 0.5);
    nemoshow_item_set_filter(one, btn->blur);
    btn->bg0 = one;

    one = CIRCLE_CREATE(btn->group, r);
    nemoshow_item_set_fill_color(one, RGBA(color));
    btn->bg = one;
}

static inline void button_add_path(Button *btn)
{
    // FIXME: can not calculate btn->w, btn->h?

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);

    struct showone *one;
    icon->one = one = PATH_CREATE(btn->group);
    btn->icons = list_append(btn->icons, icon);
}


static inline void button_add_circle(Button *btn, int r)
{
    if (btn->w < r * 2) btn->w = r * 2;
    if (btn->h < r * 2) btn->h = r * 2;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = r * 2;
    icon->h = r * 2;

    struct showone *one;
    icon->one = one = CIRCLE_CREATE(btn->group, r);
    btn->icons = list_append(btn->icons, icon);
}

static inline void button_add_svg(Button *btn, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = w;
    icon->h = h;

    struct showone *one;
    icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, w, h, svgfile);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    btn->icons = list_append(btn->icons, icon);
}

static inline void button_add_svg_path(Button *btn, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)calloc(sizeof(ButtonIcon), 1);
    icon->w = w;
    icon->h = h;

    struct showone *one;
    icon->one = one = SVG_PATH_CREATE(btn->group, w, h, svgfile);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    btn->icons = list_append(btn->icons, icon);
}

static inline void button_change_svg(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    struct showone *one;
    if (icon->old) nemoshow_one_destroy(icon->old);
    icon->old = icon->one;
    if (w >= 0 && h >= 0) {
        icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, w, h, svgfile);
        icon->w = w;
        icon->h = h;
    } else {
        icon->one = one = SVG_PATH_GROUP_CREATE(btn->group, icon->w, icon->h, svgfile);
    }
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    _nemoshow_item_motion(icon->old, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
}

static inline void button_change_svg_path(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h)
{
    if (btn->w < w) btn->w = w;
    if (btn->h < h) btn->h = h;
    nemoshow_item_set_width(btn->event, btn->w);
    nemoshow_item_set_height(btn->event, btn->h);

    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    struct showone *one;
    if (icon->old) nemoshow_one_destroy(icon->old);
    icon->old = icon->one;
    if (w >= 0 && h >= 0) {
        icon->one = one = SVG_PATH_CREATE(btn->group, w, h, svgfile);
        icon->w = w;
        icon->h = h;
    } else {
        icon->one = one = SVG_PATH_CREATE(btn->group, icon->w, icon->h, svgfile);
    }
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);

    _nemoshow_item_motion(icon->old, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(icon->one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);
}

static inline void button_bg_set_color(Button *btn, uint32_t easetype, int duration, int delay, uint32_t color)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->bg, easetype, duration, delay,
                "fill", color, NULL);
    } else {
        nemoshow_item_set_fill_color(btn->bg, RGBA(color));
    }
}

static inline void button_set_stroke(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t stroke, int stroke_width)
{
    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    if (duration > 0) {
        _nemoshow_item_motion(icon->one, easetype, duration, delay,
                "stroke", stroke, "stroke-width", (double)stroke_width,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(icon->one, RGBA(stroke));
        nemoshow_item_set_stroke_width(icon->one, stroke_width);
    }
}

static inline void button_set_fill(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t fill)
{
    ButtonIcon *icon = (ButtonIcon *)LIST_DATA(list_get_nth(btn->icons, idx));
    RET_IF(!icon);

    if (duration > 0) {
        _nemoshow_item_motion(icon->one, easetype, duration, delay,
                "fill", fill, NULL);
    } else {
        nemoshow_item_set_fill_color(icon->one, RGBA(fill));
    }
}

static inline void button_set_userdata(Button *btn, void *userdata)
{
    btn->userdata = userdata;
}

static inline void *button_get_userdata(Button *btn)
{
    return btn->userdata;
}

static inline void button_destroy(Button *btn)
{
    nemoshow_one_destroy(btn->blur);
    ButtonIcon *icon;
    LIST_FREE(btn->icons, icon) {
        nemoshow_one_destroy(icon->one);
        if (icon->old) nemoshow_one_destroy(icon->old);
        free(icon);
    }
    nemoshow_one_destroy(btn->group);
    free(btn);
}

static inline void button_translate(Button *btn, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(btn->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_item_translate(btn->group, tx, ty);
    }
}

static inline void button_down(Button *btn)
{
    if (btn->bg0)
        _nemoshow_item_motion_bounce(btn->bg0,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    if (btn->bg)
        _nemoshow_item_motion_bounce(btn->bg,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    List *l;
    ButtonIcon *icon;
    LIST_FOR_EACH(btn->icons, l, icon) {
        _nemoshow_item_motion_bounce(icon->one,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 0.7, 0.75, "sy", 0.7, 0.75,
                NULL);
    }
}

static inline void button_up(Button *btn)
{
    if (btn->bg0)
        _nemoshow_item_motion_bounce(btn->bg0,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                NULL);
    if (btn->bg)
        _nemoshow_item_motion_bounce(btn->bg,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.1, 1.00, "sy", 1.1, 1.00,
                NULL);
    List *l;
    ButtonIcon *icon;
    LIST_FOR_EACH(btn->icons, l, icon) {
        _nemoshow_item_motion_bounce(icon->one,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 1.1, 1.00, "sy", 1.0, 1.00,
                NULL);
    }
}

#define DBOX_CYAN 0x00D8DEFF
#define DBOX_WHITE WHITE
#define DBOX_BLACK 0x5D5D5DFF
#define DBOX_MAGENTA  MAGENTA
#define DBOX_YELLOW YELLOW


uint32_t DRAWING_BOX_COLOR[] = {
    DBOX_BLACK, DBOX_CYAN, DBOX_MAGENTA, DBOX_YELLOW, DBOX_WHITE
};

double DRAWING_BOX_SIZE[] = {
    1, 2, 3, 5, 8,
};

typedef struct _DrawingBox DrawingBox;
struct _DrawingBox {
    int mode; // 0 (pencil) , 1 (painter)
    int r;
    struct showone *group;

    double size;
    unsigned int size_idx;
    uint32_t color;
    unsigned int color_idx;

    Button *pencil_btn;

    Button *quit_btn;
    Button *share_btn;
    Button *undo_btn;
    Button *undo_all_btn;

    Button *stroke_btn;
    Button *color_btn;
};

DrawingBox *drawingbox_create(struct showone *parent, int r)
{
    DrawingBox *dbox = (DrawingBox *)calloc(sizeof(DrawingBox), 1);

    dbox->r = r;
    dbox->size = 1;
    dbox->color = DRAWING_BOX_COLOR[0];

    struct showone *group;
    group = dbox->group = GROUP_CREATE(parent);
    nemoshow_item_set_alpha(group, 0.0);

    int wh = r * 2;
    Button *btn;

    btn = dbox->pencil_btn = button_create(group, "dbox", 10);
    button_enable_bg(btn, r, DBOX_CYAN);
    button_add_svg_path(btn, ICON_DIR"/common/pencil.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_WHITE);

    btn = dbox->quit_btn = button_create(group, "dbox", 11);
    button_enable_bg(btn, r, DBOX_CYAN);
    button_add_svg_path(btn, ICON_DIR"/common/quit.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_WHITE);

    btn = dbox->share_btn = button_create(group, "dbox", 12);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, ICON_DIR"/common/share.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->undo_btn = button_create(group, "dbox", 13);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, ICON_DIR"/common/undo.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->undo_all_btn = button_create(group, "dbox", 14);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, ICON_DIR"/common/undo_all.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->stroke_btn = button_create(group, "dbox", 15);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg_path(btn, ICON_DIR"/common/stroke_1.svg", wh, wh);
    button_set_fill(btn, 0, 0, 0, 0, DBOX_BLACK);

    btn = dbox->color_btn = button_create(group, "dbox", 16);
    button_enable_bg(btn, r, DBOX_WHITE);
    button_add_svg(btn, ICON_DIR"/common/rainbow.svg", wh, wh);
    button_add_circle(btn, r * 0.75);
    button_set_fill(btn, 0, 0, 0, 1, dbox->color);

    ButtonIcon *icon = button_get_nth_icon(btn, 1);
    nemoshow_item_set_alpha(icon->one, 0.0);
    nemoshow_item_scale(icon->one, 0.0, 0.0);

    return dbox;
}

void drawingbox_destroy(DrawingBox *dbox)
{
    button_destroy(dbox->pencil_btn);
    button_destroy(dbox->quit_btn);
    button_destroy(dbox->share_btn);
    button_destroy(dbox->undo_btn);
    button_destroy(dbox->undo_all_btn);
    button_destroy(dbox->stroke_btn);
    button_destroy(dbox->color_btn);

    nemoshow_one_destroy(dbox->group);
    free(dbox);
}

void drawingbox_change_color(DrawingBox *dbox, uint32_t easetype, int duration, int delay)
{
    dbox->color_idx++;
    if (dbox->color_idx >= (sizeof(DRAWING_BOX_COLOR)/sizeof(DRAWING_BOX_COLOR[0]))) {
        dbox->color_idx = 0;
    }
    dbox->color = DRAWING_BOX_COLOR[dbox->color_idx];

    button_set_fill(dbox->color_btn, easetype, duration, delay,
            1, dbox->color);
    button_set_fill(dbox->stroke_btn, easetype, duration, delay,
            0, dbox->color);

    ButtonIcon *rainbow = button_get_nth_icon(dbox->color_btn, 0);
    ButtonIcon *color = button_get_nth_icon(dbox->color_btn, 1);
    _nemoshow_item_motion(rainbow->one, easetype, duration, delay,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0, NULL);
    _nemoshow_item_motion(color->one, easetype, duration, delay,
            "sx", 1.0, "sy", 1.0, "alpha", 1.0, NULL);
}

void drawingbox_change_stroke(DrawingBox *dbox, uint32_t easetype, int duration, int delay)
{
    dbox->size_idx++;
    if (dbox->size_idx >= (sizeof(DRAWING_BOX_SIZE)/sizeof(DRAWING_BOX_SIZE[0]))) {
        dbox->size_idx = 0;
    }
    dbox->size = DRAWING_BOX_SIZE[dbox->size_idx];

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, ICON_DIR"/common/stroke_%d.svg", dbox->size_idx);
    button_change_svg_path(dbox->stroke_btn, easetype, duration, delay,
            0, buf, -1, -1);
    button_set_fill(dbox->stroke_btn, easetype, duration, delay,
            0, dbox->color);
}

void drawingbox_show_pencil(DrawingBox *dbox)
{
    dbox->mode = 0;

    button_bg_set_color(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            DBOX_CYAN);
    button_set_fill(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            0, DBOX_WHITE);
    button_show(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_hide(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);
}

void drawingbox_show_menu(DrawingBox *dbox)
{
    dbox->mode = 1;
    int r = dbox->r * 2.5;

    int delay = 100;

    button_hide(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);
    button_translate(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0, 0, 0);

    button_show(dbox->quit_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 0);

    button_show(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay);
    button_translate(dbox->stroke_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay,
           r * sin(2 * M_PI * 0.4), r *cos(2 * M_PI * 0.4));

    button_show(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2);
    button_translate(dbox->color_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
           r * sin(2 * M_PI * 0.6), r *cos(2 * M_PI * 0.6));
    ButtonIcon *rainbow = button_get_nth_icon(dbox->color_btn, 0);
    ButtonIcon *color = button_get_nth_icon(dbox->color_btn, 1);
    _nemoshow_item_motion(rainbow->one, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
            "sx", 1.0, "sy", 1.0, "alpha", 1.0, NULL);
    _nemoshow_item_motion(color->one, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 2,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0, NULL);

    button_show(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 3);
    button_translate(dbox->share_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 3,
           r * sin(2 * M_PI * 0.2), r *cos(2 * M_PI * 0.2));

    button_show(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 4);
    button_translate(dbox->undo_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 4,
           r * sin(2 * M_PI * 0.8), r *cos(2 * M_PI * 0.8));

    button_show(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 5);
    button_translate(dbox->undo_all_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, delay * 5,
           r * sin(2 * M_PI * 0.0), r *cos(2 * M_PI * 0.0));
}

void drawingbox_show(DrawingBox *dbox)
{
    _nemoshow_item_motion(dbox->group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 1.0, NULL);

    button_show(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 350);
}

void drawingbox_hide(DrawingBox *dbox)
{
    _nemoshow_item_motion(dbox->group, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
            "alpha", 0.0, NULL);

    button_hide(dbox->pencil_btn, NEMOEASE_CUBIC_OUT_TYPE, 500, 150);
}

void drawingbox_get_translate(DrawingBox *dbox, double *tx, double *ty)
{
    if (tx) *tx = nemoshow_item_get_translate_x(dbox->group);
    if (ty) *ty = nemoshow_item_get_translate_y(dbox->group);
}

void drawingbox_translate(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(dbox->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "tx");
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "ty");
        nemoshow_item_translate(dbox->group, tx, ty);
    }
}

void drawingbox_scale(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(dbox->group, easetype, duration, delay,
                "sx", sx, "sy", sy, NULL);
    } else {
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "sx");
        nemoshow_revoke_transition_one(dbox->group->show, dbox->group, "sy");
        nemoshow_item_scale(dbox->group, sx, sy);
    }
}

typedef struct _MapItem MapItem;
typedef struct _Map Map;

typedef void (*MapItemFunc)(MapItem *it, void *userdata);


struct _MapItem {
    Map *map;
    int x, y, w, h;
    char *uri;

    struct showone *group;
    struct showone *one;
    uint32_t fill;
    uint32_t stroke;
    double stroke_width;

    struct {
        MapItemFunc func;
        void *userdata;
    } clicked;
    void *userdata;
};

struct _Map {
    int w, h;
    NemoWidget *widget;

    double x, y;
    double cx0, cy0, cx1, cy1;
    struct showone *group;
    struct showone *scalegroup;
    double scale;

    List *items;
    MapItem *zoom_it;

    int dx, dy;
    MapItem *grab_it;

    int clip_r;
    struct showone *clip;
};

static inline void map_item_set_callback(MapItem *it, const char *id, MapItemFunc func, void *userdata)
{
    RET_IF(!id);
    if (!strcmp(id, "clicked")) {
        it->clicked.func = func;
        it->clicked.userdata = userdata;
    }
}

static inline void map_item_set_userdata(MapItem *it, void *userdata)
{
    it->userdata = userdata;
}

static inline void map_item_show(MapItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 1.0);
    }
}

static inline void map_item_hide(MapItem *it, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        _nemoshow_item_motion(it->group, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(it->group, 0.0);
    }
}

static inline void map_item_set_fill(MapItem *it, uint32_t easetype, int duration, int delay, uint32_t color)
{
    it->fill = color;
    if (duration > 0) {
        _nemoshow_item_motion(it->one, easetype, duration, delay,
                "fill", color, NULL);
    } else {
        nemoshow_item_set_fill_color(it->one, RGBA(color));
    }
}

static inline void map_item_set_stroke(MapItem *it, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_width)
{
    Map *map = it->map;
    it->stroke = color;
    it->stroke_width = stroke_width;

    if (duration > 0) {
        _nemoshow_item_motion(it->one, easetype, duration, delay,
                "stroke-width", it->stroke_width/map->scale,
                "stroke", color,
                NULL);
    } else {
        nemoshow_item_set_stroke_color(it->one, RGBA(color));
        nemoshow_item_set_stroke_width(it->one, it->stroke_width/map->scale);
    }
}

static inline void _map_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    MapItem *it = (MapItem *)userdata;

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        _nemoshow_item_motion(it->one, NEMOEASE_CUBIC_OUT_TYPE, 250, 0,
                "fill", WHITE, NULL);
    } else if (nemoshow_event_is_up(show, event)) {
        map_item_set_fill(it, NEMOEASE_CUBIC_IN_TYPE, 250, 0, it->fill);
        if (nemoshow_event_is_single_click(show, event)) {
            if (it->clicked.func) {
                it->clicked.func(it, it->clicked.userdata);
            }
        }
    }
}

static inline void _map_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    struct showevent *event = (struct showevent *)info;

    if (nemoshow_event_is_done(event))  return;

    if (nemoshow_event_is_down(show, event)) {
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);

        struct showone *one;
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            MapItem *it = (MapItem *)nemoshow_one_get_userdata(one);
            nemowidget_create_grab(widget, event, _map_grab_event, it);
            nemowidget_enable_event_repeat(widget, false);
        } else {
            nemowidget_enable_event_repeat(widget, true);
        }
    }
}

static inline Map *map_create(NemoWidget *parent, int width, int height)
{
    Map *map = (Map *)calloc(sizeof(Map), 1);
    map->w = width;
    map->h = height;

    NemoWidget *widget;
    map->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _map_event, map);

    struct showone *group;
    map->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    nemoshow_item_set_alpha(group, 0.0);

    map->scale = 1.0;
    map->scalegroup = group = GROUP_CREATE(map->group);

    return map;
}

static inline void map_destroy(Map *map)
{
    if (map->clip) nemoshow_one_destroy(map->clip);
    nemoshow_one_destroy(map->scalegroup);
    nemoshow_one_destroy(map->group);
    nemowidget_destroy(map->widget);
    free(map);
}

static inline void map_translate(Map *map, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    nemowidget_translate(map->widget, easetype, duration, delay, tx, ty);
}

static inline void map_show(Map *map, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(map->widget, 0, 0, 0);
    if (duration > 0) {
        _nemoshow_item_motion(map->group, easetype, duration, delay,
                "alpha", 1.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(map->group, 1.0);
    }
}

static inline void map_hide(Map *map, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(map->widget, 0, 0, 0);
    if (duration > 0) {
        _nemoshow_item_motion(map->group, easetype, duration, delay,
                "alpha", 0.0,
                NULL);
    } else {
        nemoshow_item_set_alpha(map->group, 0.0);
    }
}

static inline void map_content_scale(Map *map, uint32_t easetype, int duration, int delay, double scale)
{
    /*
    double tx = (map->w - map->w * scale)/2.0 + dx;
    double ty = (map->h - map->h * scale)/2.0 + dy;
    */
    map->scale = scale;
    if (duration > 0) {
        _nemoshow_item_motion(map->scalegroup, easetype, duration, delay,
                "sx", scale,
                "sy", scale,
                NULL);
    } else {
        nemoshow_item_scale(map->scalegroup, scale, scale);
    }

    List *l;
    MapItem *it;
    LIST_FOR_EACH(map->items, l, it) {
        nemoshow_item_set_stroke_width(it->one,
                it->stroke_width/map->scale);
    }
}

static inline void map_content_translate(Map *map, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    map->x = tx;
    map->y = ty;
    if (duration > 0) {
        _nemoshow_item_motion(map->scalegroup, easetype, duration, delay,
                "tx", tx,
                "ty", ty,
                NULL);
    } else {
        nemoshow_item_translate(map->scalegroup, tx, ty);
    }

}

static inline MapItem *map_append_item(Map *map, const char *uri, int x, int y, int width, int height)
{
    MapItem *it = (MapItem *)calloc(sizeof(MapItem), 1);
    it->map = map;
    it->uri = strdup(uri);
    it->x = x;
    it->y = y;
    it->w = width;
    it->h = height;

    // Calculate content size
    if (list_count(map->items) == 0) {
        map->cx0 = it->x;
        map->cy0 = it->y;
        map->cx1 = it->x + it->w;
        map->cy1 = it->y + it->h;
    } else {
        if (map->cx0 > it->x) {
            map->cx0 = it->x;
        }
        if (map->cy0 > it->y) {
            map->cy0 = it->y;
        }
        if (map->cx1 < it->x + it->w) {
            map->cx1 = it->x + it->w;
        }
        if (map->cy1 < it->y + it->h) {
            map->cy1 = it->y + it->h;
        }
    }

    struct showone *group;
    struct showone *one;
    it->group = group = GROUP_CREATE(map->scalegroup);
    it->one = one = SVG_PATH_CREATE(group, it->w, it->h, it->uri);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_item_translate(it->group, x, y);
    nemoshow_one_set_userdata(one, it);

    map->items = list_append(map->items, it);

    return it;
}

static inline void map_remove_item(Map *map, MapItem *it)
{
    map->items = list_remove(map->items, it);
    nemoshow_one_destroy(it->group);
    free(it->uri);
    free(it);
}

static inline void map_zoom_item(Map *map, MapItem *it, uint32_t easetype, int duration, int delay)
{
    map->zoom_it = it;
    double w = map->w;
    double h = map->h;

    int iw, ih, ix, iy;
    ix = it->x;
    iy = it->y;
    iw = it->w;
    ih = it->h;

    int ww, hh;
    _rect_ratio_fit(iw, ih, w, h, &ww, &hh);

    // FIXME:??
    //double scale = (double)ww/(iw > ih ? iw : ih);
    double scale = (double)ww/iw;
    double x = -ix * scale + (w - ww)/2;
    double y = -iy * scale + (h - hh)/2;

    map_content_scale(map, easetype, duration, delay, scale);
    map_content_translate(map, easetype, duration, delay, x, y);
}

static inline void map_set_clip(Map *map, struct showone *clip)
{
    if (map->clip) {
        nemoshow_one_destroy(map->clip);
        map->clip = NULL;
    }
    if (!clip) return;

    map->clip = clip;

    nemoshow_item_set_clip(map->scalegroup, clip);
}

static inline void map_set_clip_circle(Map *map, int r)
{
    if (map->clip) {
        nemoshow_one_destroy(map->clip);
        map->clip = NULL;
    }
    if (r <= 0) return;

    map->clip_r = r;
    struct showone *clip;
    clip = PATH_CIRCLE_CREATE(map->group, map->clip_r);
    nemoshow_item_translate(clip, map->clip_r, map->clip_r);

    map_set_clip(map, clip);
}

#ifdef __cplusplus
}
#endif

#endif
