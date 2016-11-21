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
#include "nemoui.h"
#include "nemohelper.h"

const char *WEEKS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

typedef struct _ClockView ClockView;
struct _ClockView {
    struct nemoshow *show;
    struct nemotool *tool;
    struct nemotimer *timer;

    NemoWidget *widget;
    struct showone *linear, *radial;
    struct showone *group;
    struct showone *out0, *out1;
    struct showone *bg;
    struct showone *bar;
    Text *hour0, *hour1;
    Text *min0, *min1;
    Text *sec;
    Text *period;
    Text *week;

};

static void _clockview_timeout(struct nemotimer *timer, void *userdata)
{
    ClockView *view = userdata;

    Clock now = clock_get();

    char buf[PATH_MAX];

    snprintf(buf, PATH_MAX, "%01d", now.hours/10);
    text_update(view->hour0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, buf);
    snprintf(buf, PATH_MAX, "%01d", now.hours%10);
    text_update(view->hour1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, buf);

    snprintf(buf, PATH_MAX, "%01d", now.mins/10);
    text_update(view->min0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, buf);
    snprintf(buf, PATH_MAX, "%01d", now.mins%10);
    text_update(view->min1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, buf);

    if (now.hours >= 12) {
        text_update(view->period, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "PM");
    } else {
        text_update(view->period, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0,
                "AM");
    }

    text_update(view->week, 0, 0, 0, WEEKS[now.week]);

    _nemoshow_item_motion(view->bar, NEMOEASE_CUBIC_INOUT_TYPE, 980, 0,
            "to", (now.secs/60.0) * 360.0 - 90.0,
            NULL);

    nemotimer_set_timeout(timer, 1000);
    nemoshow_dispatch_frame(view->show);
}

ClockView *clockview_create(NemoWidget *parent, int w, int h)
{
    ClockView *view = calloc(sizeof(ClockView), 1);
    view->show = nemowidget_get_show(parent);
    view->tool = nemowidget_get_tool(parent);
    view->timer = TOOL_ADD_TIMER(view->tool, 0, _clockview_timeout, view);

    NemoWidget *widget;
    view->widget = widget = nemowidget_create_vector(parent, w, h);

    struct showone *linear, *radial;
    view->linear = linear = LINEAR_GRADIENT_CREATE(-w/2, h/2, w/2, 0);
    STOP_CREATE(linear, 0x00FF00FF, 0.0);
    STOP_CREATE(linear, 0xF15A24FF, 1.0);

    view->radial = radial = RADIAL_GRADIENT_CREATE(0, 0, w/2);
    STOP_CREATE(radial, 0x1B1F49FF, 0.0);
    STOP_CREATE(radial, 0x272557FF, 0.7);
    STOP_CREATE(radial, 0x563D8DFF, 1.0);

    struct showone *group;
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    struct showone *one;
    view->out0 = one = CIRCLE_CREATE(group, 500/2 - 2);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_stroke_color(one, RGBA(0x662D91FF));
    nemoshow_item_set_stroke_width(one, 4);

    view->out1 = one = CIRCLE_CREATE(group, 480/2 - 5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_stroke_color(one, RGBA(0x662D91FF));
    nemoshow_item_set_stroke_width(one, 10);

    view->bg = one = CIRCLE_CREATE(group, 448/2);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_fill_color(one, RGBA(0x662D91FF));
    nemoshow_item_set_shader(one, radial);

    view->bar = one = ARC_CREATE(group, 460 - 6, 460 - 6, -90, 360.0);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_stroke_cap(one, 0);
    nemoshow_item_set_stroke_color(one, RGBA(0x662D91FF));
    nemoshow_item_set_stroke_width(one, 6);
    nemoshow_item_set_shader(one, linear);

    Text *text;
    double tw, th, th2;
    view->hour0 = text = text_create(view->tool, group,
            "NanumGothic", "Regular", 114);
    text_set_anchor(text, 0.5, 1.05);
    text_set_align(text, 0.0, 0.5);
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_update(text, 0, 0, 0, "0");
    text_get_size(text, &tw, &th);
    text_translate(text, 0, 0, 0, w/2 - tw/2, h/2);

    view->hour1 = text = text_create(view->tool, group,
            "NanumGothic", "Regular", 114);
    text_set_align(text, 0.0, 0.5);
    text_set_anchor(text, 0.5, 1.05);
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_update(text, 0, 0, 0, "0");
    text_translate(text, 0, 0, 0, w/2 + tw/2, h/2);

    view->min0 = text = text_create(view->tool, group, "NanumGothic", "Regular",
            114);
    text_set_anchor(text, 0.5, -0.05);
    text_set_align(text, 0.0, 0.5);
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_update(text, 0, 0, 0, "0");
    text_get_size(text, &tw, &th);
    text_translate(text, 0, 0, 0, w/2 - tw/2, h/2);

    view->min1 = text = text_create(view->tool, group, "NanumGothic", "Regular",
            114);
    text_set_anchor(text, 0.5, -0.05);
    text_set_align(text, 0.0, 0.5);
    text_update(text, 0, 0, 0, "0");
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_translate(text, 0, 0, 0, w/2 + tw/2, h/2);

    view->period = text = text_create(view->tool, group, "NanumGothic", "Regular",
            30);
    text_set_anchor(text, 1.0, 0.0);
    text_update(text, 0, 0, 0, "AM");
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_translate(text, 0, 0, 0, w/2 + 105, h/2 - 70);
    text_get_size(text, NULL, &th2);

    view->week = text = text_create(view->tool, group, "NanumGothic", "Regular",
            20);
    text_set_anchor(text, 1.0, 0.0);
    text_update(text, 0, 0, 0, "Sun");
    text_set_fill_color(text, 0, 0, 0, 0xF2F2F2FF);
    text_translate(text, 0, 0, 0, w/2 + 105, h/2 - 70 + 30);

    return view;
}

static void clockview_show(ClockView *view)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemotimer_set_timeout(view->timer, 10);
    nemoshow_dispatch_frame(view->show);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, base);

    ClockView *view;
    view = clockview_create(win, base->width, base->height);
    clockview_show(view);
    //nemowidget_append_callback(win, "exit", _win_exit, view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    //clockview_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
