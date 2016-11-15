#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "util.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"

static void _event(NemoWidget *button, const char *id, void *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(button);
    if (nemoshow_event_is_down(show, event))
        ERR("Down: (%lf, %lf)", nemoshow_event_get_x(event), nemoshow_event_get_y(event));
    else if (nemoshow_event_is_up(show, event))
        ERR("Up: (%lf, %lf)", nemoshow_event_get_x(event), nemoshow_event_get_y(event));
}

static void _clicked(NemoWidget *button, const char *id, void *info, void *userdata)
{
    ERR("%s", id);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_base(tool, APPNAME, base);

    NemoWidget *btn;
    btn = nemowidget_create_button(win, 100, 100);
    nemowidget_button_set_bg_circle(btn);
    nemowidget_button_set_text(btn, "BM DoHyeon", "Regular", 30, "Button");
    nemowidget_button_bg_set_fill(btn, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, GREEN);
    nemowidget_button_text_set_fill(btn, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, BLUE);
    nemowidget_append_callback(btn, "event", _event, NULL);
    nemowidget_append_callback(btn, "clicked", _clicked, NULL);
    nemowidget_show(btn, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    nemowidget_translate(btn, 0, 0, 0, 100, 100);
    nemowidget_translate(btn, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, 200, 150);

    nemotool_run(tool);

    nemowidget_destroy(btn);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);
    config_unload(base);

    return 0;
}
