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

typedef struct _TestBin TestBin;
struct _TestBin {
    const char *name;
    const char *exe;
    uint32_t text_color;
    uint32_t bg_color;
    int x, y, w, h;
};

#define BG_COLOR 0x34495EFF

TestBin testbins[] = {
    {"Exit", NULL, WHITE, BG_COLOR, 0, 0, 100, 100},
    {"Bar", "/usr/bin/nemotest_bar", RED, BG_COLOR, 110, 0, 100, 100},
    {"Pie", "/usr/bin/nemotest_pie", BLUE, BG_COLOR, 0, 110, 100, 100},
    {"Pies", "/usr/bin/nemotest_pies", GREEN, BG_COLOR, 110, 110, 100, 100},
    {"Anim", "/usr/bin/nemotest_animation", ORANGE, BG_COLOR, 210, 210, 100, 100}
};

typedef struct _Test Test;
struct _Test {
    struct nemoshow *show;
    NemoWidget *win;
    const char *uuid;
    List *btns;
};

static void test_hide(Test *test, uint32_t easetype, int duration, int delay)
{
    int adv = 150;
    int _duration = duration - (list_count(test->btns) - 1) * adv;

    int i = 0;
    List *l;
    NemoWidget *btn;
    LIST_FOR_EACH(test->btns, l, btn) {
        nemowidget_button_bg_set_fill(btn, easetype, _duration, delay,
                0x0);
        nemowidget_button_text_set_fill(btn, easetype, _duration, delay,
                0x0);
        nemowidget_translate(btn, easetype, _duration, delay, 0, 0);
        nemowidget_hide(btn, easetype, _duration, delay);
        delay += adv;
        i++;
    }
}

static void test_show(Test *test, uint32_t easetype, int duration, int delay)
{
    int adv = 150;
    int _duration = duration - (list_count(test->btns) - 1) * adv;

    int i = 0;
    List *l;
    NemoWidget *btn;
    LIST_FOR_EACH(test->btns, l, btn) {
        nemowidget_button_bg_set_fill(btn, easetype, _duration, delay,
                testbins[i].bg_color);
        nemowidget_button_text_set_fill(btn, easetype, _duration, delay,
                testbins[i].text_color);
        nemowidget_translate(btn, easetype, _duration, delay,
                testbins[i].x, testbins[i].y);
        nemowidget_show(btn, easetype, _duration, delay);
        i++;
        delay += adv;
    }
}

static void _btn_clicked_callback(NemoWidget *btn, const char *id, void *info, void *userdata)
{
    Test *test = userdata;
    struct nemoshow *show = nemowidget_get_show(btn);
    const char *tag = nemowidget_get_tag(btn);
    RET_IF(!tag);
    ERR("%s", tag);

    double wx, wy, ww, wh;
    nemowidget_get_geometry(btn, &wx, &wy, &ww, &wh);

    int i = 0;
    for (i = 0 ; i < sizeof(testbins)/sizeof(testbins[0]) ; i++) {
        ERR("%s", tag);
        if (!strcmp(tag, testbins[i].name)) {
            if (!testbins[i].exe) {
                _nemoshow_destroy_transition_all(test->show);
                test_hide(test, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);
                nemowidget_win_exit_if_no_trans(test->win);
            } else {
                float x, y;
                nemoshow_transform_to_viewport(show,
                        wx + ww/2, wy + wh/2, &x, &y);
                nemoshow_view_set_anchor(show, 0.5, 0.5);
                ERR("%s", testbins[i].exe);

                nemo_execute(test->uuid, "app", testbins[i].exe, NULL, NULL,
                        wx + ww/2, wy + wh/2, 0, 1, 1);
            }
            break;
        }
    }
}

static Test *test_create(NemoWidget *win)
{
    Test *test = calloc(sizeof(Test), 1);
    test->show = nemowidget_get_show(win);
    test->win = win;
    test->uuid = nemowidget_get_uuid(win);

    NemoWidget *btn;
    int i = 0;
    for (i = 0 ; i < sizeof(testbins)/sizeof(testbins[0]) ; i++) {
        btn = nemowidget_create_button(win, testbins[i].w, testbins[i].h);
        nemowidget_set_tag(btn, testbins[i].name);
        nemowidget_button_set_bg_circle(btn);
        nemowidget_button_set_text(btn, "NanumGothic", "Regular", 35, testbins[i].name);
        nemowidget_append_callback(btn, "clicked", _btn_clicked_callback, test);

        test->btns = list_append(test->btns, btn);
    }
    return test;
}

static void test_destroy(Test *test)
{
    NemoWidget *btn;
    LIST_FREE(test->btns, btn) nemowidget_destroy(btn);
    free(test);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Test *test = userdata;
    _nemoshow_destroy_transition_all(test->show);

    test_hide(test, NEMOEASE_CUBIC_IN_TYPE, 1000, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_config(tool, APPNAME, base);

    Test *test = test_create(win);
    nemowidget_append_callback(win, "exit", _win_exit, test);
    test_show(test, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    nemotool_run(tool);

    test_destroy(test);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
