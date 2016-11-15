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

#include "util.h"
#include "widgets.h"
#include "view.h"
#include "graph.h"
#include "nemohelper.h"

#define SEC_COLOR 0x1EDCDCFF
#define MIN_COLOR 0xFF3300FF
#define HOUR_COLOR 0x0033FFFF
#define MAIN_COLOR 0x0099FFFF

const char *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *WEEKS[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

typedef struct _ClockView ClockView;
struct _ClockView {
    NemoWidget *vector;
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *canvas;
    int w, h;

    struct showone *group;

    struct {
        int text_cx, text_cy;
        int text_r;
        NumberText *text;

        CircleHand *hand;
    } sec;

    struct {
        int text_cx, text_cy;
        int text_r;
        NumberText *text;

        PieHand *hand;
    } min;

    struct {
        int text_cx, text_cy;
        int text_r;
        NumberText *text;

        PieHand *hand;
    } hour;

    struct {
        NumberText *text;
    } year;

    struct {
        Text *text;
    } month;

    struct {
        NumberText *text;
    } day;

    struct {
        Text *text;
    } week;

    struct nemotimer *timer;
};

static void clock_destroy(ClockView *clock)
{
    if (clock->timer) nemotimer_destroy(clock->timer);

    text_destroy(clock->week.text);
    number_text_destroy(clock->day.text);
    text_destroy(clock->month.text);
    number_text_destroy(clock->year.text);

    number_text_destroy(clock->hour.text);
    pie_hand_destroy(clock->hour.hand);

    number_text_destroy(clock->min.text);
    pie_hand_destroy(clock->min.hand);

    number_text_destroy(clock->sec.text);
    circle_hand_destroy(clock->sec.hand);

    nemoshow_one_destroy(clock->group);
    nemowidget_destroy(clock->vector);
    free(clock);
}

static ClockView *clock_create(NemoWidget *parent, int w, int h)
{
    ClockView *clock = calloc(sizeof(ClockView), 1);

    NemoWidget *vector;
    vector = nemowidget_create_vector(parent, w, h);
    nemowidget_enable_event(vector, false);
    nemowidget_show(vector, 0, 0, 0);
    clock->vector = vector;

    struct showone *canvas;
    struct nemotool *tool;
    clock->tool = tool = nemowidget_get_tool(vector);
    clock->show = nemowidget_get_show(vector);
    clock->canvas = canvas = nemowidget_get_canvas(vector);
    clock->w = w;
    clock->h = h;
    nemowidget_attach_scope_circle(vector, w/2, h/2, w/2);

    struct showone *group = GROUP_CREATE(canvas);
    clock->group = group;

    double ww, hh;

    // Seconds
    ww = hh = w - 10;
    clock->sec.hand = circle_hand_create(group, ww, hh, 20, 60);
    circle_hand_translate(clock->sec.hand, clock->w/2.0, clock->h/2.0);

    clock->sec.text_r = ww/2.0 - 35;
    clock->sec.text_cx = w/2.0;
    clock->sec.text_cy = h/2.0;
    clock->sec.text = number_text_create(tool, group, 15, 2, 2);
    number_text_set_color(clock->sec.text, 0, 0, 0, SEC_COLOR);
    number_text_translate(clock->sec.text, 0, 0, 0, clock->w/2.0, clock->h/2.0);

    // Minutes
    ww = hh = w - 120;
    clock->min.hand = pie_hand_create(group, ww, hh, 5, 60);
    pie_hand_set_color(clock->min.hand, 0, 0, 0, MIN_COLOR);
    pie_hand_translate(clock->min.hand, 0, 0, 0, clock->w/2.0, clock->h/2.0);

    clock->min.text_r = ww/2.0 - 25;
    clock->min.text_cx = w/2.0;
    clock->min.text_cy = h/2.0;
    clock->min.text = number_text_create(tool, group, 15, 2, 2);
    number_text_set_color(clock->min.text, 0, 0, 0, MIN_COLOR);
    number_text_translate(clock->min.text, 0, 0, 0, clock->w/2.0, clock->h/2.0);

    // Hours
    ww = hh = w - 210;
    clock->hour.hand = pie_hand_create(group, ww, hh, 5, 12);
    pie_hand_set_color(clock->hour.hand, 0, 0, 0, HOUR_COLOR);
    pie_hand_translate(clock->hour.hand, 0, 0, 0, clock->w/2.0, clock->h/2.0);

    clock->hour.text_r = ww/2.0 - 30;
    clock->hour.text_cx = w/2.0;
    clock->hour.text_cy = h/2.0;
    clock->hour.text = number_text_create(tool, group, 20, 2, 2);
    number_text_set_color(clock->hour.text, 0, 0, 0, HOUR_COLOR);
    number_text_translate(clock->hour.text, 0, 0, 0, clock->w/2.0, clock->h/2.0);
    double tx, ty;
    // Year
    tx = w/2.0;
    ty = h/2.0 - 50;
    clock->year.text = number_text_create(tool, group, 35, 4, 4);
    number_text_set_color(clock->year.text, 0, 0, 0, MAIN_COLOR);
    number_text_translate(clock->year.text, 0, 0, 0, tx, ty);
    number_text_motion_line(clock->year.text, NEMOEASE_CUBIC_INOUT_TYPE, 3000, 3000);

    // Month
    tx = w/2.0 - 25;
    ty = h/2.0 + 10;
    clock->month.text = text_create(tool, group, "Taurus Mono Outline", "Regular", 35);
    text_set_blur(clock->month.text, "solid", 20);
    text_motion_blur(clock->month.text, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0);
    text_set_fill_color(clock->month.text, 0, 0, 0, MAIN_COLOR);
    text_update(clock->month.text, 0, 0, 0, MONTHS[0]);
    text_translate(clock->month.text, 0, 0, 0, tx, ty);

    // Day
    tx = w/2.0 + 25;
    ty = h/2.0 + 10;
    clock->day.text = number_text_create(tool, group, 20, 2, 2);
    number_text_set_color(clock->day.text, 0, 0, 0, MAIN_COLOR);
    number_text_translate(clock->day.text, 0, 0, 0, tx, ty);

    // Week
    tx = w/2.0;
    ty = h/2.0 + 40;
    clock->week.text = text_create(tool, group, "Taurus Mono Outline", "Regular", 46);
    text_set_blur(clock->week.text, "solid", 20);
    text_motion_blur(clock->week.text, NEMOEASE_CUBIC_INOUT_TYPE, 2000, 0);
    text_set_fill_color(clock->week.text, 0, 0, 0, MAIN_COLOR);
    text_update(clock->week.text, 0, 0, 0, WEEKS[0]);
    text_translate(clock->week.text, 0, 0, 0, tx, ty);

    return clock;
}

static void _clock_timeout(struct nemotimer *timer, void *userdata)
{
    ClockView *clock = userdata;

    Clock now;
    now = clock_get();

    double tx, ty;
    double pie;
    // Seconds
    circle_hand_update(clock->sec.hand, NEMOEASE_CUBIC_OUT_TYPE, 980, 0, now.secs);
    number_text_update(clock->sec.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, now.secs);
    pie = -M_PI/2.0 + now.secs * (M_PI * 2)/60.0;
    tx = clock->sec.text_cx + clock->sec.text_r * cos(pie);
    ty = clock->sec.text_cy + clock->sec.text_r * sin(pie);
    number_text_translate(clock->sec.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, tx, ty);

    // Minutes
    pie_hand_update(clock->min.hand, NEMOEASE_CUBIC_OUT_TYPE, 980, 0, now.mins);
    number_text_update(clock->min.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, now.mins);
    pie = -M_PI/2.0 + now.mins * (M_PI * 2)/60.0;
    tx = clock->min.text_cx + clock->min.text_r * cos(pie);
    ty = clock->min.text_cy + clock->min.text_r * sin(pie);
    number_text_translate(clock->min.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, tx, ty);

    // Hours
    pie_hand_update(clock->hour.hand, NEMOEASE_CUBIC_OUT_TYPE, 980, 0, now.hours);
    number_text_update(clock->hour.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, now.hours);
    pie = -M_PI/2.0 + now.hours * (M_PI * 2)/12.0;
    tx = clock->hour.text_cx + clock->hour.text_r * cos(pie);
    ty = clock->hour.text_cy + clock->hour.text_r * sin(pie);
    number_text_translate(clock->hour.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, tx, ty);

    // Year
    number_text_update(clock->year.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, now.year);

    // Month
    text_update(clock->month.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, MONTHS[now.month]);

    // Day
    number_text_update(clock->day.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, now.day);

    // Week
    text_update(clock->week.text, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0, WEEKS[now.week]);

    nemotimer_set_timeout(timer, 1000);
    nemoshow_dispatch_frame(clock->show);
}

static void clock_show(ClockView *clock, uint32_t easetype, int duration, int delay)
{
    circle_hand_show(clock->sec.hand, easetype, duration, delay);
    number_text_show(clock->sec.text, easetype, duration, delay);

    pie_hand_show(clock->min.hand, easetype, duration, delay + 250);
    number_text_show(clock->min.text, easetype, duration, delay + 250);

    pie_hand_show(clock->hour.hand, easetype, duration, delay + 450);
    number_text_show(clock->hour.text, easetype, duration, delay + 450);

    number_text_show(clock->year.text, easetype, duration, delay);

    text_show(clock->month.text, easetype, duration, delay);

    number_text_show(clock->day.text, easetype, duration, delay);

    text_show(clock->week.text, easetype, duration, delay);

    if (clock->timer) nemotimer_destroy(clock->timer);
    struct nemotimer *timer = nemotimer_create(clock->tool);
    nemotimer_set_timeout(timer, duration + delay);
    nemotimer_set_callback(timer, _clock_timeout);
    nemotimer_set_userdata(timer, clock);
    clock->timer = timer;

    //nemoshow_dispatch_frame(clock->show);
}

static void clock_hide(ClockView *clock, uint32_t easetype, int duration, int delay)
{
    circle_hand_hide(clock->sec.hand, easetype, duration, delay);
    number_text_hide(clock->sec.text, easetype, duration, delay);

    pie_hand_hide(clock->min.hand, easetype, duration, delay + 150);
    number_text_hide(clock->min.text, easetype, duration, delay + 150);

    pie_hand_hide(clock->hour.hand, easetype, duration, delay + 250);
    number_text_hide(clock->hour.text, easetype, duration, delay + 250);

    number_text_hide(clock->year.text, easetype, duration, delay);

    text_hide(clock->month.text, easetype, duration, delay);

    number_text_hide(clock->day.text, easetype, duration, delay);

    text_hide(clock->week.text, easetype, duration, delay);

    if (clock->timer) {
        nemotimer_destroy(clock->timer);
        clock->timer = NULL;
    }
    nemoshow_dispatch_frame(clock->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    ClockView *clock = userdata;
    _nemoshow_destroy_transition_all(clock->show);

    clock_hide(clock, NEMOEASE_CUBIC_IN_TYPE, 500, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    Config *config;
    config = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, config);

    ClockView *clock;
    clock = clock_create(win, config->width, config->height);
    clock_show(clock, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    nemowidget_append_callback(win, "exit", _win_exit, clock);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    clock_destroy(clock);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(config);

    return 0;
}
