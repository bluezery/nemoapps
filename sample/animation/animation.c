#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"
#include "nemoui.h"

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Animation *anim = userdata;
    _nemoshow_destroy_transition_all(anim->show);

    animation_hide(anim, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_config(tool, APPNAME, base);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, base->width, base->height);
    nemowidget_show(vector, 0, 0, 0);

    struct showone *canvas = nemowidget_get_canvas(vector);

    // Background animation
    Animation *anim = animation_create(tool, canvas,
            base->width, base->height);
    nemowidget_append_callback(win, "exit", _win_exit, anim);
    int i = 0;
    for (i = 1 ; i <= 60 ; i++) {
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, MAP_BG_DIR"/BG_%05d.png", i);
        animation_append_item(anim, path);
    }
    animation_translate(anim, 0, 0, 0, base->width/2, base->height/2);
    animation_set_fps(anim, 30);
    animation_show(anim, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    nemotool_run(tool);

    animation_destroy(anim);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
