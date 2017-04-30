#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include <nemoutil.h>
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"

#define COL_ACTIVE      0x1EDCDCFF
#define COL_BASE        0x0F6E6EFF
#define COL_INACTIVE    0x094242FF
#define COL_BLINK       0xFF8C32FF

static void _frame_add_clicked(NemoWidget *frame, const char *id, void *info, void *userdata)
{
    ERR("%s", id);
}

static void _frame_quit_clicked(NemoWidget *frame, const char *id, void *info, void *userdata)
{
    NemoWidget *win = nemowidget_get_top_widget(frame);
    nemowidget_win_exit(win);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_config(tool, APPNAME, base);

    NemoWidget *frame = nemowidget_create(&NEMOWIDGET_FRAME, win, base->width, base->height);
    nemowidget_frame_set_color(frame, COL_INACTIVE, COL_BASE, COL_ACTIVE, COL_BLINK);
    nemowidget_append_callback(frame, "add,clicked", _frame_add_clicked, NULL);
    nemowidget_append_callback(frame, "quit,clicked", _frame_quit_clicked, NULL);
    nemowidget_show(frame, NEMOEASE_CUBIC_OUT_TYPE, -1, 0);

    nemotool_run(tool);

    nemowidget_destroy(frame);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
