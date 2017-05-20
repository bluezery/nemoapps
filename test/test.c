#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"

void _timeout(struct nemotimer *timer, void *userdata)
{
    List *ones = userdata;
    struct showone *one;
    List *l;
    LIST_FOR_EACH(ones, l, one) {
        _nemoshow_item_motion_bounce(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                "alpha", 0.5, 1.0,
                "sx", 0.5, 1.0,
                "sy", 0.5, 1.0,
                NULL);
    }

    one = LIST_DATA(LIST_FIRST(ones));

    nemoshow_dispatch_frame(nemoshow_one_get_show(one));
    nemotimer_set_timeout(timer, 500);
}

int main(int argc, char *argv[])
{
    Config *config;
    config = config_load(PROJECT_NAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, config);

    NemoWidget *widget;
    widget = nemowidget_create_vector(win, config->width, config->height);
    struct showone *group;
    struct showone *one;
    group = GROUP_CREATE(nemowidget_get_canvas(widget));

    List *ones = NULL;
    int i;
    for (i = 0 ; i < 1 ; i++) {
        one = RECT_CREATE(group, config->width, config->height);
        nemoshow_item_set_fill_color(one, RGBA(RED));
        ones = list_append(ones, one);
    }

    struct nemotimer *timer = TOOL_ADD_TIMER(tool, 20, _timeout, ones);

    nemoshow_dispatch_frame(nemowidget_get_show(win));

    nemotool_run(tool);

    LIST_FREE(ones, one) nemoshow_one_destroy(one);

    nemotimer_destroy(timer);
    nemoshow_one_destroy(group);
    nemowidget_destroy(widget);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(config);

    return 0;
}

