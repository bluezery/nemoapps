#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"

#define MAX_NUM_CPU 32
#define UPDATE_TIMEOUT 2000

#define GAP 5

#define COLOR_BASE 0x1EDCDCFF
#define COLOR_TEXT 0xBBF4F4FF

/******************/
/**** Cpu Stat ****/
/******************/
typedef struct _Cpu Cpu;
struct _Cpu
{
    char *name;
    int user;
    int nice;
    int system;
    int idle;
    int total;

    double user_share;
    double nice_share;
    double system_share;
    double idle_share;
};

typedef struct _CpuStat CpuStat;
struct _CpuStat
{
    int cnt;
    Cpu *cpus;
};

static void
_cpustat_free(CpuStat *stat)
{
    int i = 0 ;
    for (i = 0 ; i < stat->cnt ; i++) {
        free(stat->cpus[i].name);
    }
    free(stat->cpus);
    free(stat);
}

static CpuStat *
_cpustat_load()
{
    int line_len;
    char **lines = file_read_line("/proc/stat", &line_len);

    int i = 0;
    int cnt = 0;
    Cpu *cpus = NULL;
    for (i = 0 ; i < line_len ; i++) {
        if (!strncmp(lines[i], "cpu", 3)) {
            char *token;

            cpus = realloc(cpus, sizeof(Cpu) * (cnt + 1));
            token = strtok(lines[i], " ");
            cpus[cnt].name = strdup(token);
            token = strtok(NULL, " ");
            cpus[cnt].user = atoi(token);
            token = strtok(NULL, " ");
            cpus[cnt].nice = atoi(token);
            token = strtok(NULL, " ");
            cpus[cnt].system = atoi(token);
            token = strtok(NULL, " ");
            cpus[cnt].idle = atoi(token);

            cpus[cnt].total =
                cpus[cnt].user + cpus[cnt].nice +
                cpus[cnt].system + cpus[cnt].idle;
            cnt++;
        }
        free(lines[i]);
    }
    free(lines);

    CpuStat *stat = calloc(sizeof(CpuStat), 1);
    stat->cpus = cpus;
    stat->cnt = cnt;
    return stat;
}

static CpuStat *
_cpustat_update(CpuStat *old)
{
    CpuStat *new = _cpustat_load();

    if (!old) {
        int i = 0;
        Cpu *cpus = new->cpus;
        for (i = 0 ; i < new->cnt ; i++) {
            if (cpus[i].total == 0) continue;
            cpus[i].user_share = (double)cpus[i].user/cpus[i].total;
            cpus[i].nice_share = (double)cpus[i].nice/cpus[i].total;
            cpus[i].system_share = (double)cpus[i].system/cpus[i].total;
            cpus[i].idle_share = (double)cpus[i].idle/cpus[i].total;
        }
        return new;
    }

    if (old->cnt != new->cnt) {
        ERR("cpu count is changed!!!, Is it possible??");
        return new;
    }

    int i = 0;
    for (i = 0 ; i < old->cnt ; i++) {
        int total;
        total = new->cpus[i].total - old->cpus[i].total;
        if (total != 0) {
            new->cpus[i].user_share =
              (new->cpus[i].user - old->cpus[i].user)/(double)total;
            new->cpus[i].nice_share =
              (new->cpus[i].nice - old->cpus[i].nice)/(double)total;
            new->cpus[i].system_share =
              (new->cpus[i].system - old->cpus[i].system)/(double)total;
            new->cpus[i].idle_share =
              (new->cpus[i].idle - old->cpus[i].idle)/(double)total;
        }
    }
    _cpustat_free(old);
    return new;
}

/**********************/
/**** Memory Stat *****/
/**********************/
typedef struct _MemStat MemStat;
struct _MemStat {
    int phy_total;
    int phy_used;
    int phy_free;
    int swap_total;
    int swap_used;
    int swap_free;
    double phy_share;
    double swap_share;
};

static void
_memstat_free(MemStat *stat)
{
    free(stat);
}

static MemStat *
_memstat_load(MemStat *stat)
{
    int line_len;
    char **lines = file_read_line("/proc/meminfo", &line_len);
    int i = 0;

    if (!stat) stat = calloc(sizeof(MemStat), 1);
    for (i = 0 ; i < line_len ; i++) {
        if (!strncmp(lines[i], "MemTotal:", 9)) {
            char *token;
            token = strtok(lines[i], " ");
            token = strtok(NULL, " ");
            stat->phy_total = atoi(token)/1024;
        }
        if (!strncmp(lines[i], "MemFree:", 8)) {
            char *token;
            token = strtok(lines[i], " ");
            token = strtok(NULL, " ");
            stat->phy_free = atoi(token)/1024;
        }
        if (!strncmp(lines[i], "SwapTotal:", 10)) {
            char *token;
            token = strtok(lines[i], " ");
            token = strtok(NULL, " ");
            stat->swap_total = atoi(token)/1024;
        }
        if (!strncmp(lines[i], "SwapFree:", 9)) {
            char *token;
            token = strtok(lines[i], " ");
            token = strtok(NULL, " ");
            stat->swap_free = atoi(token)/1024;
        }
        free(lines[i]);
    }
    free(lines);

    stat->phy_used = stat->phy_total - stat->phy_free;
    stat->swap_used = stat->swap_total - stat->swap_free;
    stat->phy_share = (stat->phy_used/(double)stat->phy_total) * 100;
    stat->swap_share = (stat->swap_used/(double)stat->swap_total) * 100;

    return stat;
}

static MemStat *
_memstat_update(MemStat *stat)
{
    return _memstat_load(stat);
}

typedef struct _Stat Stat;

typedef struct _Guage Guage;
struct _Guage {
    struct nemoshow *show;
    struct showone *parent;
    int x, y, w, h;
    int color;

    struct showone *grp;
    const char *ttt;
    struct showone *txt;
    int border_sw;
    struct showone *border;
    struct showone *idle_txt0;
    struct showone *idle_txt1;
    struct showone *idle_txt2;

    int cnt;
    int used_txt_sz;
    struct showone **used_txt0;
    struct showone **used_txt1;
    struct showone **used_txt2;
    struct showone **used;
    int *used_prev_w;
    struct showone **bar3d;
};

struct _Stat {
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;
    int w, h;

    struct showone *blur;
    struct showone *font;

    double height;
    struct nemotimer *timer;
    CpuStat *cpustat;
    MemStat *memstat;

    struct {
        struct {
            Guage all;
            Guage phy;
            Guage swap;
        } mem;
        struct {
            Guage all;
            int cnt;
            Guage *cpus;
        } cpu;
    } main;
};

static void _guage_destroy_done(NemoMotion *m, void *userdata)
{
    Guage *g = userdata;
    nemoshow_one_destroy(g->grp);
}

static void _guage_destroy(Guage *g)
{
    struct nemoshow *show = g->show;
    NemoMotion *m;

    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 200);
    nemomotion_attach(m, 1.0,
            g->txt, "tx", (double)g->w,
            g->txt, "alpha", 0.0,
            NULL);
    nemomotion_set_done_callback(m, _guage_destroy_done, g);
    nemomotion_run(m);

    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 100);
    nemomotion_attach(m, 1.0,
            g->border, "tx", (double)g->w,
            g->border, "width", (double)g->h,
            g->border, "alpha", 0.0,
            NULL);
    nemomotion_run(m);

    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 1.0,
            g->idle_txt0, "tx", (double)g->w - GAP,
            g->idle_txt0, "alpha", 0.0,
            g->idle_txt1, "tx", (double)g->w - GAP,
            g->idle_txt1, "alpha", 0.0,
            g->idle_txt2, "tx", (double)g->w - GAP,
            g->idle_txt2, "alpha", 0.0,
            NULL);
    nemomotion_run(m);

    int i = 0;
    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 100);
    for (i = 0 ; i < g->cnt ; i++) {
        nemomotion_attach(m, 1.0,
                g->used[i], "tx", (double)g->w - GAP,
                g->used[i], "width", 0.0,
                g->used[i], "alpha", 0.0,
                g->bar3d[i], "tx", (double)g->w - GAP,
                g->bar3d[i], "width", 0.0,
                g->bar3d[i], "alpha", 0.0,
                g->used_txt0[i], "tx", (double)g->w - GAP,
                g->used_txt0[i], "alpha", 0.0,
                g->used_txt1[i], "tx", (double)g->w - GAP,
                g->used_txt1[i], "alpha", 0.0,
                g->used_txt2[i], "tx", (double)g->w - GAP,
                g->used_txt2[i], "alpha", 0.0,
                NULL);
    }
    nemomotion_run(m);
}

static void _guage_translate(Guage *g, int x, int y)
{
    g->x = x;
    g->y = y;
    nemoshow_item_translate(g->grp, g->x, g->y);
}

static void _guage_create(Guage *g, struct showone *parent,
        struct showone *font, int fontsize, const char *txt,
        int idle_txt_sz, int used_txt_sz,
        uint32_t color, int cnt,
        int width, int height)
{
    g->parent = parent;
    g->show = parent->show;
    g->w = width;
    g->h = height;
    g->cnt = cnt;
    g->used_txt_sz = used_txt_sz;
    g->ttt = txt;
    g->color = color;

    int sw = GAP - 4; // stroke width
    g->border_sw = sw;
    int y = 0;
    struct showone *grp;
    g->grp = grp = GROUP_CREATE(parent);

    g->txt = TEXT_CREATE(grp, font, fontsize, txt);
    nemoshow_item_set_fill_color(g->txt, RGBA(COLOR_TEXT));
    nemoshow_item_set_alpha(g->txt, 0.0);
    nemoshow_item_translate(g->txt, width, 0);

    y = fontsize + GAP;
    g->border = RRECT_CREATE(grp, 0, height, 2, 2);
    nemoshow_item_set_stroke_color(g->border, RGBA(COLOR_BASE));
    nemoshow_item_set_stroke_width(g->border, sw);
    nemoshow_item_set_alpha(g->border, 0.0);
    nemoshow_item_translate(g->border, width, y);

    int i = 0;
    g->used_txt0 = malloc(sizeof(struct showone *) * cnt);
    g->used_txt1 = malloc(sizeof(struct showone *) * cnt);
    g->used_txt2 = malloc(sizeof(struct showone *) * cnt);
    g->used = malloc(sizeof(struct showone *) * cnt);
    g->used_prev_w = malloc(sizeof(int) * cnt);
    g->bar3d = malloc(sizeof(struct showone *) * cnt);

    int x = width - GAP;
    y = y + GAP;    // rect y position
    int yy = fontsize + height - GAP - used_txt_sz - 2;   // text y position
    for (i = 0 ; i < cnt ; i++) {
        g->used[i] = RRECT_CREATE(grp, 0, g->h - GAP *2, 2, 2);
        nemoshow_item_set_fill_color(g->used[i], RGBA(color));
        nemoshow_item_set_alpha(g->used[i], 0.0);
        nemoshow_item_translate(g->used[i], x, y);

        g->used_txt0[i] = NUM_CREATE(grp, COLOR_TEXT, 3, idle_txt_sz, 0);
        nemoshow_item_set_alpha(g->used_txt0[i], 0.0);
        nemoshow_item_translate(g->used_txt0[i], x, yy);

        g->used_txt1[i] = NUM_CREATE(grp, COLOR_TEXT, 3, idle_txt_sz, 0);
        nemoshow_item_set_alpha(g->used_txt1[i], 0.0);
        nemoshow_item_translate(g->used_txt1[i], x, yy);

        g->used_txt2[i] = NUM_CREATE(grp, COLOR_TEXT, 3, idle_txt_sz, 0);
        nemoshow_item_set_alpha(g->used_txt2[i], 0.0);
        nemoshow_item_translate(g->used_txt2[i], x, yy);

        g->bar3d[i] = RRECT_CREATE(grp, 2, 25, 2, 2);
        nemoshow_item_set_fill_color(g->bar3d[i], RGBA(COLOR_TEXT));
        nemoshow_item_set_alpha(g->bar3d[i], 0.0);
        nemoshow_item_translate(g->bar3d[i], x, y + 3);
    }

    g->idle_txt0 = NUM_CREATE(grp, COLOR_TEXT, 3, used_txt_sz, 0);
    nemoshow_item_set_alpha(g->idle_txt0, 0.0);
    nemoshow_item_translate(g->idle_txt0, x, y);

    g->idle_txt1 = NUM_CREATE(grp, COLOR_TEXT, 3, used_txt_sz, 0);
    nemoshow_item_set_alpha(g->idle_txt1, 0.0);
    nemoshow_item_translate(g->idle_txt1, x, y);

    g->idle_txt2 = NUM_CREATE(grp, COLOR_TEXT, 3, used_txt_sz, 0);
    nemoshow_item_set_alpha(g->idle_txt2, 0.0);
    nemoshow_item_translate(g->idle_txt2, x, y);

    NemoMotion *m;
    m = nemomotion_create(g->show, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemomotion_attach(m, 1.0,
            g->txt, "tx", 0.0,
            g->txt, "alpha", 1.0,
            NULL);
    nemomotion_run(m);

    m = nemomotion_create(g->show, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
    nemomotion_attach(m, 1.0,
            g->border, "tx", 0.0,
            g->border, "width", (double)width,
            g->border, "alpha", 1.0,
            NULL);
    nemomotion_run(m);

    m = nemomotion_create(g->show, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 100);
    nemomotion_attach(m, 1.0,
            g->idle_txt0, "tx", (double)GAP,
            g->idle_txt1, "tx", (double)GAP + fontsize - 10,
            g->idle_txt2, "tx", (double)GAP + (fontsize - 10) * 2,
            g->idle_txt0, "alpha", 1.0f,
            g->idle_txt1, "alpha", 1.0f,
            g->idle_txt2, "alpha", 1.0f,
            NULL);
    nemomotion_run(m);
}

static void _guage_motion(Guage *g, int cnt, double ratio[], uint32_t dur, uint32_t delay)
{
    RET_IF(cnt != g->cnt);
    RET_IF(!ratio);
    struct nemoshow *show = g->show;

    double idle = 1.0;
    int i = 0;
    for (i = 0; i < cnt ; i++) {
        idle -= ratio[i];
    }

    NemoMotion *m0, *m;
    m = nemomotion_create(show, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0);

    if (idle < 1.0) {
        NUM_UPDATE(g->idle_txt0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0, idle * 10);
        NUM_UPDATE(g->idle_txt1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0, (int)(idle * 100) % 10);
        nemomotion_attach(m, 1.0,
            g->idle_txt0, "alpha", 1.0,
            g->idle_txt1, "alpha", 1.0,
            g->idle_txt2, "alpha", 0.0,
            NULL);

    } else {
        NUM_UPDATE(g->idle_txt0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0, 1);
        NUM_UPDATE(g->idle_txt1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0, 0);
        NUM_UPDATE(g->idle_txt2, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0, 0);
        nemomotion_attach(m, 1.0,
                g->idle_txt0, "alpha", 1.0,
                g->idle_txt1, "alpha", 1.0,
                g->idle_txt2, "alpha", 1.0,
                NULL);
    }
    nemomotion_run(m);

    // gap for each side = GAP * 2
    // gap between gauage bars = GAP * (cnt - 1)
    double w = (g->w - (GAP * 2)) - (GAP * (cnt - 1));
    double x = (g->w - (GAP * 2)) + GAP;

    for (i = cnt - 1 ; i >= 0 ; i--) {
        int ww = (double)w * ratio[i];
        if ((i != cnt - 1) && (x != (g->w - GAP * 2 + GAP)))
            x -= (ww + 2);
        else  x -= ww;

        if (g->used_prev_w[i] != ww) {
            m = nemomotion_create(show, NEMOEASE_LINEAR_TYPE, dur + 500, delay);
            nemomotion_attach(m, 0.5, g->used[i], "fill", ORANGE, NULL);
            nemomotion_attach(m, 1.0, g->used[i], "fill", COLOR_BASE, NULL);
            nemomotion_run(m);
            g->used_prev_w[i] = ww;
        }
        m0 = nemomotion_create(show, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay);
        nemomotion_attach(m0, 1.0,
                g->used[i], "tx", x,
                g->used[i], "width", (double)ww,
                g->used[i], "alpha", 1.0,
                g->bar3d[i], "tx", x + 3,
                g->bar3d[i], "width", (double)ww - 3 * 2,
                g->bar3d[i], "alpha", 0.2,
                NULL);
        nemomotion_run(m0);

        double alpha;
        if (ww < (g->used_txt_sz * 2)) {
            alpha = 0.0f;
        } else {
            alpha = 1.0f;
        }

        m = nemomotion_create(show, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 150);
        nemomotion_attach(m, 1.0,
                g->used_txt0[i], "alpha", alpha,
                g->used_txt1[i], "alpha", alpha,
                g->used_txt0[i], "tx", x,
                g->used_txt1[i], "tx", x + g->used_txt_sz,
                NULL);
        nemomotion_run(m);

        NUM_UPDATE(g->used_txt0[i], NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 150,
                ratio[i] * 10);
        NUM_UPDATE(g->used_txt1[i], NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 150,
                (int)(ratio[i] * 100) % 10);
    }
}

#if 0
static bool _guage_at(Guage *g, double _x, double _y)
{
    double x = NEMOSHOW_ITEM_AT(g->grp, tx) + NEMOSHOW_ITEM_AT(g->border, x);
    double y = NEMOSHOW_ITEM_AT(g->grp, ty) + NEMOSHOW_ITEM_AT(g->border, y);
    double w = g->w;
    double h = g->h;

    if (RECTS_CROSS(_x, _y, 0, 0, x, y, w, h)) return true;
    return false;
}

static Guage *_stat_search_guage(Stat *stat, double x, double y)
{
    Guage *g;
    g = &(stat->main.mem.all);
    if (_guage_at(g, x, y)) return g;
    g = &(stat->main.mem.phy);
    if (_guage_at(g, x, y)) return g;
    g = &(stat->main.mem.swap);
    if (_guage_at(g, x, y)) return g;
    g = &(stat->main.cpu.all);
    if (_guage_at(g, x, y)) return g;

    int i = 0;
    for (i = 0 ; i < stat->main.cpu.cnt ; i++) {
        g = &(stat->main.cpu.cpus[i]);
        if (_guage_at(g, x, y)) return g;
    }
    return NULL;
}
#endif

static void _stat_main_create(Stat *stat)
{
    struct showone *canvas = nemowidget_get_canvas(stat->widget);
    struct showone *font = stat->font;

    Guage *g;
    // Memory
    int x = 12;
    int y = 12;

    g = &(stat->main.mem.all);
    _guage_create(g, canvas, font, 40, "Memory", 30, 30, COLOR_BASE, 2, 1000, 100);
    _guage_translate(g, x, y);

    y += (40 + 100) + 25;
    g = &(stat->main.mem.phy);
    _guage_create(g, canvas, font, 35, "Physical", 20, 20, COLOR_BASE, 1, 475, 70);
    _guage_translate(g, x, y);

    g = &(stat->main.mem.swap);
    _guage_create(g, canvas, font, 35, "Swap", 20, 20, COLOR_BASE, 1, 475, 70);
    _guage_translate(g, x + 520, y);

    // CPU
    y += (20 + 70) + 80;
    int cnt = stat->cpustat->cnt - 1;
    stat->main.cpu.cnt = cnt;
    g = &(stat->main.cpu.all);
    _guage_create(g, canvas, font, 40, "CPU", 30, 30, COLOR_BASE, 3, 1000, 100);
    _guage_translate(g, x, y);

    if (cnt > 16) ERR("Warnnig some cpus are not shown");

    y += (40 + 100) + 25;
    g = stat->main.cpu.cpus = malloc(sizeof(Guage) * cnt);

    int w = 0;
    int i = 0;
    int yy = 0;
    if (cnt > 12) {
        w = 220;
        for (i = 0 ; i < cnt ; i++) {
            char buf[8];
            snprintf(buf, 8, "CPU%d", i);
            if (i % 4 == 0) x = 12;
            else if (i % 4 == 1)  x = 12 + 260;
            else if (i % 4 == 2)  x = 12 + 260 * 2;
            else x = 12 + 260 * 3;
            yy = y + (int)((i/4) * (35 + 70 + 15));

            g = &(stat->main.cpu.cpus[i]);
            _guage_create(g, canvas, font, 35, buf, 20, 20, COLOR_BASE, 3, w, 70);
            _guage_translate(g, x, yy);
        }
    } else if (cnt > 8) {
        int i = 0;
        w = 300;
        for (i = 0 ; i < cnt ; i++) {
            char buf[8];
            snprintf(buf, 8, "CPU%d", i);
            if (i % 3 == 0) x = 12;
            else if (i % 3 == 1)  x = 12 + 350;
            else x = 12 + 350 * 2;
            yy = y + (int)((i/3) * (35 + 70 + 15));

            g = &(stat->main.cpu.cpus[i]);
            _guage_create(g, canvas, font, 35, buf, 20, 20, COLOR_BASE, 3, w, 70);
            _guage_translate(g, x, yy);
        }
    } else {
        int i = 0;
        w = 475;
        for (i = 0 ; i < cnt ; i++) {
            char buf[8];
            snprintf(buf, 8, "CPU%d", i);
            if (i % 2) x = 12 + 525;
            else  x = 12;
            yy = y + (int)((i/2) * (35 + 70 + 15));

            g = &(stat->main.cpu.cpus[i]);
            _guage_create(g, canvas, font, 35, buf, 20, 20, COLOR_BASE, 3, w, 70);
            _guage_translate(g, x, yy);
        }
    }
    stat->height = yy + 35 + 70 + 15;
}

static void _stat_main_update(Stat *stat)
{
    // Memory
    double r[3];
    int memtot = stat->memstat->phy_total + stat->memstat->swap_total;
    if (memtot != 0) {
        r[0] = (double)stat->memstat->phy_used/memtot;
        r[1] = (double)stat->memstat->swap_used/memtot;
    } else {
        r[0] = 0;
        r[1] = 0;
    }

    uint32_t dur = 1000;
    uint32_t delay = (UPDATE_TIMEOUT - 1000)/stat->main.cpu.cnt;
    Guage *g;
    g = &(stat->main.mem.all);
    _guage_motion(g, 2, r, dur, 0);

    if (stat->memstat->phy_total != 0)
        r[0] = (double)stat->memstat->phy_used/stat->memstat->phy_total;
    else
        r[0] = 0;
    g = &(stat->main.mem.phy);
    _guage_motion(g, 1, &r[0], dur, delay);

    if (stat->memstat->swap_total != 0)
        r[0] = (double)stat->memstat->swap_used/stat->memstat->swap_total;
    else
        r[0] = 0;
    g = &(stat->main.mem.swap);
    _guage_motion(g, 1, &r[1], dur, delay * 2);

    // CPU
    Cpu cpu;

    cpu = stat->cpustat->cpus[0];
    g = &(stat->main.cpu.all);
    r[0] = cpu.user_share;
    r[1] = cpu.nice_share;
    r[2] = cpu.system_share;
    _guage_motion(g, 3, r, dur, 0);

    int i = 1;
    for (i = 0 ; i < stat->main.cpu.cnt ; i++) {
        cpu = stat->cpustat->cpus[i + 1];
        g = &(stat->main.cpu.cpus[i]);
        r[0] = cpu.user_share;
        r[1] = cpu.nice_share;
        r[2] = cpu.system_share;
        _guage_motion(g, 3, r, dur, i * delay);
    }
}

static void _stat_timeout(struct nemotimer *timer, void *userdata)
{
    Stat *stat = userdata;

    stat->cpustat = _cpustat_update(stat->cpustat);
    stat->memstat = _memstat_update(stat->memstat);

    _stat_main_update(stat);

    nemotimer_set_timeout(timer, UPDATE_TIMEOUT);
    nemoshow_dispatch_frame(stat->show);
}

// FIXME: Desgined for 1024x1024
static Stat *stat_create(NemoWidget *parent, int width, int height)
{
    Stat *stat = calloc(sizeof(Stat), 1);
    stat->tool = nemowidget_get_tool(parent);
    stat->show = nemowidget_get_show(parent); ;
    stat->w = width;
    stat->h = height;

    stat->blur = BLUR_CREATE("solid", 15);
    stat->font = FONT_CREATE("Quark Storm 3D", "Regular");

    NemoWidget *vector;
    stat->widget = vector = nemowidget_create_vector(parent, 1024, 1024);
    nemowidget_enable_event(vector, false);
    nemowidget_show(vector, 0, 0, 0);

    stat->cpustat = _cpustat_update(stat->cpustat);
    stat->memstat = _memstat_update(stat->memstat);
    _stat_main_create(stat);

    return stat;
}

static void stat_destroy(Stat *stat)
{
    nemotimer_destroy(stat->timer);
    nemoshow_one_destroy(stat->blur);
    nemoshow_one_destroy(stat->font);
    _cpustat_free(stat->cpustat);
    _memstat_free(stat->memstat);
    nemowidget_destroy(stat->widget);
}
static void stat_show(Stat *stat, uint32_t easetype, int duration, int delay)
{
    struct nemotimer *timer;
    timer = nemotimer_create(stat->tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _stat_timeout);
    nemotimer_set_userdata(timer, stat);
    stat->timer = timer;
}

static void stat_hide(Stat *stat, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(stat->timer, 0);

    Guage *g;
    g = &(stat->main.mem.all);
    _guage_destroy(g);
    g = &(stat->main.mem.phy);
    _guage_destroy(g);
    g = &(stat->main.mem.swap);
    _guage_destroy(g);
    g = &(stat->main.cpu.all);
    _guage_destroy(g);

    int i = 0;
    int cnt = stat->cpustat->cnt - 1;
    for(i = 0; i < cnt ; i++) {
        g = &(stat->main.cpu.cpus[i]);
        _guage_destroy(g);
    }
    nemoshow_dispatch_frame(stat->show);
}

static bool _win_is_stat_area(NemoWidget *win, double x, double y, void *userdata)
{
    Stat *stat = userdata;

    double xx, yy;
    nemowidget_transform_from_global(stat->widget, x, y, &xx, &yy);

    if (RECTS_CROSS(xx, yy, 1, 1, 0, 0, stat->w, stat->height))
        return true;
    return false;
}

static void _win_exit(NemoWidget *win, const char *id, void *data, void *userdata)
{
    Stat *stat = userdata;

    stat_hide(stat, NEMOEASE_CUBIC_IN_TYPE, 1500, 0);

    nemowidget_win_exit_after(win, 1500);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win(tool, APPNAME, base->width, base->height);
    // FIXME: Scene is desgined for 1024x1024
    nemowidget_win_load_scene(win, 1024, 1024);

    Stat *stat;
    stat = stat_create(win, 1024, 1024);
    stat_show(stat, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
    nemowidget_append_callback(win, "exit", _win_exit, stat);
    nemowidget_win_set_event_area(win, _win_is_stat_area, stat);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    stat_destroy(stat);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
