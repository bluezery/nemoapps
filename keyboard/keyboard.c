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

#include "xemoutil.h"
#include "widgets.h"
#include "nemoui.h"

const char *LAYOUT[] =  {
    "eng",
    "kor"
};

#define BG_TIMEOUT 2000
#define FRAME_COLOR WHITE
#define KEY_COLOR WHITE

#define GRADIENT0 0x5c8affff
#define GRADIENT1 BLACK
#define GRADIENT2 0xdf41a1ff

typedef struct _KeyFrameType KeyFrameType;
struct _KeyFrameType {
    int w, h;
};

KeyFrameType keyframetypes[] =
{
    {65,22},{60,34},{124,34},{60,60},{124,60},
    {90,60},{94,60},{107,60},{141,60},{188,60},
    {380,60},{60,28},{93,28}, {93,28}
};

typedef struct _KeyCoord KeyCoord;
struct _KeyCoord {
    int x, y;
    int frametype;
    int layer;
    const char name[16];
    uint32_t code;
    const char *path;
    const char *path_s;
    const char *path_k;
    const char *path_k_s;
    const char *path_special;
};

KeyCoord keycoords[] =
{
    // 0 (esc)
    {12,8,12,0, "FOCUS", 0, APP_ICON_DIR"/key-78.svg", NULL, NULL, NULL, NULL},
    {875,8,13,0, "QUIT", 1, APP_ICON_DIR"/key-00.svg", NULL, NULL, NULL, NULL},
    // 1 (2)
    {12,44,1,1, "ESC", 1, APP_ICON_DIR"/key-01.svg", NULL, NULL, NULL, NULL},
    {76,44,1,1, "F1", 59, APP_ICON_DIR"/key-02.svg", NULL, NULL, NULL, NULL},
    {140,44,1,1, "F2", 60, APP_ICON_DIR"/key-03.svg", NULL, NULL, NULL, NULL},
    {204,44,1,1, "F3", 61, APP_ICON_DIR"/key-04.svg", NULL, NULL, NULL, NULL},
    {268,44,1,1, "F4", 62, APP_ICON_DIR"/key-05.svg", NULL, NULL, NULL, NULL},
    {332,44,1,1, "F5", 63, APP_ICON_DIR"/key-06.svg", NULL, NULL, NULL, NULL},
    {396,44,1,1, "F6", 64, APP_ICON_DIR"/key-07.svg", NULL, NULL, NULL, NULL},
    {460,44,1,1, "F7", 65, APP_ICON_DIR"/key-08.svg", NULL, NULL, NULL, NULL},
    {524,44,1,1, "F8", 66, APP_ICON_DIR"/key-09.svg", NULL, NULL, NULL, NULL},
    {588,44,1,1, "F9", 67, APP_ICON_DIR"/key-10.svg", NULL, NULL, NULL, NULL},
    {652,44,1,1, "F10", 68, APP_ICON_DIR"/key-11.svg", NULL, NULL, NULL, NULL},
    {716,44,1,1, "F11", 87, APP_ICON_DIR"/key-12.svg", NULL, NULL, NULL, NULL},
    {780,44,1,1, "F12", 88, APP_ICON_DIR"/key-13.svg", NULL, NULL, NULL, NULL},
    {844,44,2,1, "DEL", 111, APP_ICON_DIR"/key-14.svg", NULL, NULL, NULL, NULL},
    // 2 (16)
    {12,88,3,2, "`", 41, APP_ICON_DIR"/key-15.svg", APP_ICON_DIR"/key-15_S.svg", NULL, NULL, NULL},
    {76,88,3,2, "1", 2, APP_ICON_DIR"/key-16.svg", APP_ICON_DIR"/key-16_S.svg", NULL, NULL, NULL},
    {140,88,3,2, "2", 3, APP_ICON_DIR"/key-17.svg", APP_ICON_DIR"/key-17_S.svg", NULL, NULL, NULL},
    {204,88,3,2, "3", 4, APP_ICON_DIR"/key-18.svg", APP_ICON_DIR"/key-18_S.svg", NULL, NULL, NULL},
    {268,88,3,2, "4", 5, APP_ICON_DIR"/key-19.svg", APP_ICON_DIR"/key-19_S.svg", NULL, NULL, NULL},
    {332,88,3,2, "5", 6, APP_ICON_DIR"/key-20.svg", APP_ICON_DIR"/key-20_S.svg", NULL, NULL, NULL},
    {396,88,3,2, "6", 7, APP_ICON_DIR"/key-21.svg", APP_ICON_DIR"/key-21_S.svg", NULL, NULL, NULL},
    {460,88,3,2, "7", 8, APP_ICON_DIR"/key-22.svg", APP_ICON_DIR"/key-22_S.svg", NULL, NULL, NULL},
    {524,88,3,2, "8", 9, APP_ICON_DIR"/key-23.svg", APP_ICON_DIR"/key-23_S.svg", NULL, NULL, NULL},
    {588,88,3,2, "9", 10, APP_ICON_DIR"/key-24.svg", APP_ICON_DIR"/key-24_S.svg", NULL, NULL, NULL},
    {652,88,3,2, "0", 11, APP_ICON_DIR"/key-25.svg", APP_ICON_DIR"/key-25_S.svg", NULL, NULL, NULL},
    {716,88,3,2, "-", 12, APP_ICON_DIR"/key-26.svg", APP_ICON_DIR"/key-26_S.svg", NULL, NULL, NULL},
    {780,88,3,2, "=", 13, APP_ICON_DIR"/key-27.svg", APP_ICON_DIR"/key-27_S.svg", NULL, NULL, NULL},
    {844,88,4,2, "BACK", 14, APP_ICON_DIR"/key-28.svg", NULL, NULL, NULL, NULL},
    // 3 (30)
    {12,152,5,3, "TAP", 15, APP_ICON_DIR"/key-29.svg", NULL, NULL, NULL, NULL},
    {106,152,3,3, "q", 16, APP_ICON_DIR"/key-30.svg", APP_ICON_DIR"/key-30_S.svg", APP_ICON_DIR"/key-30_k.svg", APP_ICON_DIR"/key-30_k_S.svg", NULL},
    {170,152,3,3, "w", 17, APP_ICON_DIR"/key-31.svg", APP_ICON_DIR"/key-31_S.svg", APP_ICON_DIR"/key-31_k.svg", APP_ICON_DIR"/key-31_k_S.svg", NULL},
    {234,152,3,3, "e", 18, APP_ICON_DIR"/key-32.svg", APP_ICON_DIR"/key-32_S.svg", APP_ICON_DIR"/key-32_k.svg", APP_ICON_DIR"/key-32_k_S.svg", NULL},
    {298,152,3,3, "r", 19, APP_ICON_DIR"/key-33.svg", APP_ICON_DIR"/key-33_S.svg", APP_ICON_DIR"/key-33_k.svg", APP_ICON_DIR"/key-33_k_S.svg", NULL},
    {362,152,3,3, "t", 20, APP_ICON_DIR"/key-34.svg", APP_ICON_DIR"/key-34_S.svg", APP_ICON_DIR"/key-34_k.svg", APP_ICON_DIR"/key-34_k_S.svg", NULL},
    {426,152,3,3, "y", 21, APP_ICON_DIR"/key-35.svg", APP_ICON_DIR"/key-35_S.svg", APP_ICON_DIR"/key-35_k.svg", NULL, NULL},
    {490,152,3,3, "u", 22, APP_ICON_DIR"/key-36.svg", APP_ICON_DIR"/key-36_S.svg", APP_ICON_DIR"/key-36_k.svg", NULL, NULL},
    {554,152,3,3, "i", 23, APP_ICON_DIR"/key-37.svg", APP_ICON_DIR"/key-37_S.svg", APP_ICON_DIR"/key-37_k.svg", NULL, NULL},
    {618,152,3,3, "o", 24, APP_ICON_DIR"/key-38.svg", APP_ICON_DIR"/key-38_S.svg", APP_ICON_DIR"/key-38_k.svg", APP_ICON_DIR"/key-38_k_S.svg", NULL},
    {682,152,3,3, "p", 25, APP_ICON_DIR"/key-39.svg", APP_ICON_DIR"/key-39_S.svg", APP_ICON_DIR"/key-39_k.svg", APP_ICON_DIR"/key-39_k_S.svg", NULL},
    {746,152,3,3, "[", 26, APP_ICON_DIR"/key-40.svg", APP_ICON_DIR"/key-40_S.svg", NULL, NULL, NULL},
    {810,152,3,3, "]", 27, APP_ICON_DIR"/key-41.svg", APP_ICON_DIR"/key-41_S.svg", NULL, NULL, NULL},
    {874,152,6,3, "\\", 43, APP_ICON_DIR"/key-42.svg", APP_ICON_DIR"/key-42_S.svg", NULL, NULL, NULL},
    // 4 (44)
    {12,216,7,4, "CAPS", 58, APP_ICON_DIR"/key-43.svg", NULL, NULL, NULL, NULL},
    {123,216,3,4, "a", 30, APP_ICON_DIR"/key-44.svg", APP_ICON_DIR"/key-44_S.svg", APP_ICON_DIR"/key-44_k.svg", NULL, NULL},
    {187,216,3,4, "s", 31, APP_ICON_DIR"/key-45.svg", APP_ICON_DIR"/key-45_S.svg", APP_ICON_DIR"/key-45_k.svg", NULL, NULL},
    {251,216,3,4, "d", 32, APP_ICON_DIR"/key-46.svg", APP_ICON_DIR"/key-46_S.svg", APP_ICON_DIR"/key-46_k.svg", NULL, NULL},
    {315,216,3,4, "f", 33, APP_ICON_DIR"/key-47.svg", APP_ICON_DIR"/key-47_S.svg", APP_ICON_DIR"/key-47_k.svg", NULL, NULL},
    {379,216,3,4, "g", 34, APP_ICON_DIR"/key-48.svg", APP_ICON_DIR"/key-48_S.svg", APP_ICON_DIR"/key-48_k.svg", NULL, NULL},
    {443,216,3,4, "h", 35, APP_ICON_DIR"/key-49.svg", APP_ICON_DIR"/key-49_S.svg", APP_ICON_DIR"/key-49_k.svg", NULL, NULL},
    {507,216,3,4, "j", 36, APP_ICON_DIR"/key-50.svg", APP_ICON_DIR"/key-50_S.svg", APP_ICON_DIR"/key-50_k.svg", NULL, NULL},
    {571,216,3,4, "k", 37 , APP_ICON_DIR"/key-51.svg", APP_ICON_DIR"/key-51_S.svg", APP_ICON_DIR"/key-51_k.svg", NULL, NULL},
    {635,216,3,4, "l", 38, APP_ICON_DIR"/key-52.svg", APP_ICON_DIR"/key-52_S.svg", APP_ICON_DIR"/key-52_k.svg", NULL, NULL},
    {699,216,3,4, ";", 39, APP_ICON_DIR"/key-53.svg", APP_ICON_DIR"/key-53_S.svg", NULL, NULL, NULL},
    {763,216,3,4, "'", 40, APP_ICON_DIR"/key-54.svg", APP_ICON_DIR"/key-54_S.svg", NULL, NULL, NULL},
    {827,216,8,4, "ENTER", 28, APP_ICON_DIR"/key-55.svg", NULL, NULL, NULL, NULL},
    // 5 (5r7)
    {12,280,4,5, "SHIFT", 42, APP_ICON_DIR"/key-56.svg", NULL, NULL, NULL, NULL},
    {140,280,3,5, "z", 44, APP_ICON_DIR"/key-57.svg", APP_ICON_DIR"/key-57_S.svg", APP_ICON_DIR"/key-57_k.svg", NULL, NULL},
    {204,280,3,5, "x", 45, APP_ICON_DIR"/key-58.svg", APP_ICON_DIR"/key-58_S.svg", APP_ICON_DIR"/key-58_k.svg", NULL, NULL},
    {268,280,3,5, "c", 46, APP_ICON_DIR"/key-59.svg", APP_ICON_DIR"/key-59_S.svg", APP_ICON_DIR"/key-59_k.svg", NULL, NULL},
    {332,280,3,5, "y", 47, APP_ICON_DIR"/key-60.svg", APP_ICON_DIR"/key-60_S.svg", APP_ICON_DIR"/key-60_k.svg", NULL, NULL},
    {396,280,3,5, "b", 48, APP_ICON_DIR"/key-61.svg", APP_ICON_DIR"/key-61_S.svg", APP_ICON_DIR"/key-61_k.svg", NULL, NULL},
    {460,280,3,5, "n", 49, APP_ICON_DIR"/key-62.svg", APP_ICON_DIR"/key-62_S.svg", APP_ICON_DIR"/key-62_k.svg", NULL, NULL},
    {524,280,3,5, "m", 50, APP_ICON_DIR"/key-63.svg", APP_ICON_DIR"/key-63_S.svg", APP_ICON_DIR"/key-63_k.svg", NULL, NULL},
    {588,280,3,5, ",", 51, APP_ICON_DIR"/key-64.svg", APP_ICON_DIR"/key-64_S.svg", NULL, NULL, NULL},
    {652,280,3,5, ".", 52, APP_ICON_DIR"/key-65.svg", APP_ICON_DIR"/key-65_S.svg", NULL, NULL, NULL},
    {716,280,3,5, "/", 53, APP_ICON_DIR"/key-66.svg", APP_ICON_DIR"/key-66_S.svg", NULL, NULL, NULL},
    {780,280,9,5, "SHIFT", 54, APP_ICON_DIR"/key-67.svg", NULL, NULL, NULL, NULL},
    // 6 (69)
    {12,344,4,6, "CTRL", 29, APP_ICON_DIR"/key-68.svg", NULL, NULL, NULL, NULL},
    {140,344,3,6, "ALT", 56, APP_ICON_DIR"/key-69.svg", NULL, NULL, NULL, NULL},
    {204,344,10,6, "SPACE", 57, APP_ICON_DIR"/key-70.svg", NULL, NULL, NULL, NULL},
    {588,344,3,6, "KOR", 100, APP_ICON_DIR"/key-71.svg", NULL, NULL, NULL, APP_ICON_DIR"/key-71-2.svg"},
    {652,344,3,6, "ALT", 56, APP_ICON_DIR"/key-72.svg", NULL, NULL, NULL, NULL},
    {716,344,3,6, "CTRL", 29, APP_ICON_DIR"/key-73.svg", NULL, NULL, NULL, NULL},
    {780,344,3,6, "LEFT", 105, APP_ICON_DIR"/key-74.svg", NULL, NULL, NULL, NULL},
    {844,344,11,6, "UP", 103, APP_ICON_DIR"/key-75.svg", NULL, NULL, NULL, NULL},
    {844,376,11,6, "DOWN", 108, APP_ICON_DIR"/key-76.svg", NULL, NULL, NULL, NULL},
    {908,344,3, 6, "RIGHT", 106, APP_ICON_DIR"/key-77.svg", NULL, NULL, NULL, NULL},
};

typedef struct _Key Key;
struct _Key {
    struct nemotool *tool;
    int type;
    int x, y, w, h;
    struct showone *group;
    struct showone *bg;
    struct showone *frame;
    struct showone *one;
    struct showone *one_s;
    struct showone *one_k;
    struct showone *one_k_s;
    struct showone *one_special;
    int frametype;
    KeyCoord *keycoord;

    struct nemotimer *repeat_timer;
};

static void _key_repeat_timeout(struct nemotimer *timer, void *userdata)
{
    Key *key = userdata;
    ERR("PRESSED: %s(%d)", key->keycoord->name, key->keycoord->code);
    nemotool_keyboard_key(key->tool, get_time(), key->keycoord->code, 1);
    ERR("RELEASED: %s(%d)", key->keycoord->name, key->keycoord->code);
    nemotool_keyboard_key(key->tool, get_time(), key->keycoord->code, 0);
    nemotimer_set_timeout(timer, 100);
}

Key *key_create(struct nemotool *tool, struct showone *parent, int x, int y, KeyCoord *keycoord)
{
    int w, h;
    Key *key = calloc(sizeof(Key), 1);
    key->tool = tool;
    key->x = x;
    key->y = y;
    key->frametype = keycoord->frametype;
    key->w = w = keyframetypes[key->frametype].w;
    key->h = h = keyframetypes[key->frametype].h;
    key->keycoord = keycoord;

    if (!strcmp(keycoord->name, "CTRL") || !strcmp(keycoord->name, "SHIFT") ||
            !strcmp(keycoord->name, "ALT")) {
        key->type = 1;
    } else if (!strcmp(keycoord->name, "FOCUS") || !strcmp(keycoord->name, "QUIT")) {
        key->type = 2;
    }

    struct showone *group;
    struct showone *one;
    key->group = group = GROUP_CREATE(parent);
    nemoshow_item_set_width(group, w);
    nemoshow_item_set_height(group, h);
    nemoshow_item_translate(group, x, y);

    if (strcmp(keycoord->name, "FOCUS") && strcmp(keycoord->name, "QUIT")) {
        key->bg = one = RRECT_CREATE(group, w, h, 10, 10);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(WHITE));
        nemoshow_item_set_alpha(one, 0.0);
    }

    char path[PATH_MAX];
    if (key->frametype == 0) {
        snprintf(path, PATH_MAX, APP_ICON_DIR"/quit.svg");
    } else {
        snprintf(path, PATH_MAX, APP_ICON_DIR"/Frame-%02d.svg", key->frametype);
    }
    key->frame = one = SVG_PATH_CREATE(group, w, h, path);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    //nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_fill_color(one, RGBA(FRAME_COLOR));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_one_set_userdata(one, key);

    key->one = one = SVG_PATH_CREATE(group, w, h, keycoord->path);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, w/2, h/2);
    nemoshow_item_set_fill_color(one, RGBA(KEY_COLOR));
    nemoshow_item_set_alpha(one, 0.0);

    if (keycoord->path_s) {
        key->one_s = one = SVG_PATH_CREATE(group, w, h, keycoord->path_s);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(GREEN));
        nemoshow_item_set_alpha(one, 0.0);
    }
    if (keycoord->path_k_s) {
        key->one_k_s = one = SVG_PATH_CREATE(group, w, h, keycoord->path_k_s);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(RED));
        nemoshow_item_set_alpha(one, 0.0);
    }
    if (keycoord->path_k) {
        key->one_k = one = SVG_PATH_CREATE(group, w, h, keycoord->path_k);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(RED));
        nemoshow_item_set_alpha(one, 0.0);
    }
    if (keycoord->path_special) {
        key->one_special = one = SVG_PATH_CREATE(group, w, h, keycoord->path_special);
        nemoshow_item_set_anchor(one, 0.5, 0.5);
        nemoshow_item_translate(one, w/2, h/2);
        nemoshow_item_set_fill_color(one, RGBA(RED));
        nemoshow_item_set_alpha(one, 0.0);
    }

    key->repeat_timer = TOOL_ADD_TIMER(tool, 0, _key_repeat_timeout, key);

    return key;
}

void key_translate(Key *key, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    _nemoshow_item_motion(key->group, easetype, duration, delay,
            "tx", tx, "ty", ty, NULL);
}

void key_show(Key *key, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(key->frame, easetype, duration, delay,
            "alpha", 1.0,
            NULL);
    _nemoshow_item_motion(key->one, easetype, duration, delay,
            "alpha", 1.0,
            NULL);
}

void key_hide(Key *key, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(key->frame, easetype, duration, delay,
            "alpha", 0.0,
            NULL);
    _nemoshow_item_motion(key->one, easetype, duration, delay,
            "alpha", 0.0,
            NULL);
}

void key_up(Key *key)
{
    if (key->bg)
        _nemoshow_item_motion_bounce(key->bg, NEMOEASE_CUBIC_OUT_TYPE, 150, 0,
                "sx", 1.1, 1.0, "sy", 1.1, 1.0,
                "alpha", 0.0,
                NULL);
    _nemoshow_item_motion_bounce(key->frame, NEMOEASE_CUBIC_OUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);
    _nemoshow_item_motion_bounce(key->one, NEMOEASE_CUBIC_OUT_TYPE, 150, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            "fill", WHITE, WHITE,
            NULL);
}

void key_down(Key *key)
{
    if (key->bg)
        _nemoshow_item_motion_bounce(key->bg, NEMOEASE_CUBIC_OUT_TYPE, 150, 0,
                "sx", 0.7, 0.9, "sy", 0.7, 0.9,
                "alpha", 1.0, 1.0,
                NULL);
    _nemoshow_item_motion_bounce(key->frame, NEMOEASE_CUBIC_OUT_TYPE, 150, 0,
            "sx", 0.7, 0.9, "sy", 0.7, 0.9,
            NULL);
    _nemoshow_item_motion_bounce(key->one, NEMOEASE_CUBIC_OUT_TYPE, 350, 0,
            "sx", 1.25, 1.0, "sy", 1.25, 1.0,
            "fill", 0x5c8affff, 0x5c8affff,
            NULL);
}

void key_special(Key *key, bool on)
{
    if (!key->one_special);
    if (on) {
        _nemoshow_item_motion(key->one,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 0.0, "alpha", 0.0,
                NULL);
        _nemoshow_item_motion(key->one_special,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, "alpha", 1.0,
                NULL);
    } else {
        _nemoshow_item_motion(key->one,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, "alpha", 1.0,
                NULL);
        _nemoshow_item_motion(key->one_special,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
}

void key_on(Key *key, bool on)
{
    if (on) {
        _nemoshow_item_motion(key->one,
                NEMOEASE_CUBIC_OUT_TYPE, 300, 0,
                "fill", RED,
                NULL);
        _nemoshow_item_motion(key->frame,
                NEMOEASE_CUBIC_OUT_TYPE, 300, 0,
                "fill", RED,
                NULL);
        if (key->bg)
            _nemoshow_item_motion(key->bg,
                    NEMOEASE_CUBIC_IN_TYPE, 300, 0,
                    "alpha", 1.0,
                    NULL);
    } else {
        _nemoshow_item_motion(key->one,
                NEMOEASE_CUBIC_IN_TYPE, 300, 0,
                "fill", WHITE,
                NULL);
        _nemoshow_item_motion(key->frame,
                NEMOEASE_CUBIC_OUT_TYPE, 300, 0,
                "fill", WHITE,
                NULL);
        if (key->bg)
            _nemoshow_item_motion(key->bg,
                    NEMOEASE_CUBIC_IN_TYPE, 300, 0,
                    "alpha", 0.0,
                    NULL);
    }
}

void key_normal(Key *key)
{
    _nemoshow_item_motion(key->one,
            NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "sx", 1.0, "sy", 1.0, "alpha", 1.0,
            NULL);
    if (key->one_s) {
        _nemoshow_item_motion(key->one_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k) {
        _nemoshow_item_motion(key->one_k,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k_s) {
        _nemoshow_item_motion(key->one_k_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
}

void key_shift(Key *key)
{
    _nemoshow_item_motion(key->one,
            NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0,
            NULL);
    if (key->one_s) {
        _nemoshow_item_motion(key->one_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, "alpha", 1.0,
                NULL);
    }
    if (key->one_k) {
        _nemoshow_item_motion(key->one_k,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k_s) {
        _nemoshow_item_motion(key->one_k_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
}

void key_kor(Key *key)
{
    _nemoshow_item_motion(key->one,
            NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0,
            NULL);
    if (key->one_s) {
        _nemoshow_item_motion(key->one_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k) {
        _nemoshow_item_motion(key->one_k,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, "alpha", 1.0,
                NULL);
    }
    if (key->one_k_s) {
        _nemoshow_item_motion(key->one_k_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
}

void key_kor_shift(Key *key)
{
    _nemoshow_item_motion(key->one,
            NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
            "sx", 0.0, "sy", 0.0, "alpha", 0.0,
            NULL);
    if (key->one_s) {
        _nemoshow_item_motion(key->one_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k) {
        _nemoshow_item_motion(key->one_k,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 0.0, "sy", 0.0, "alpha", 0.0,
                NULL);
    }
    if (key->one_k_s) {
        _nemoshow_item_motion(key->one_k_s,
                NEMOEASE_CUBIC_OUT_TYPE, 500, 0,
                "sx", 1.0, "sy", 1.0, "alpha", 1.0,
                NULL);
    }
}
typedef struct _Keyboard Keyboard;
struct _Keyboard {
    char *id;
    bool initialized;
    int width, height;
    struct nemotool *tool;
    struct nemoshow *show;
    NemoWidget *widget;
    struct showone *group;

    struct showone *gradient0, *gradient1;
    struct showone *bg0, *bg1;
    int bg_timer_idx;
    struct nemotimer *bg_timer;

    Key *esc;
    Key *keys[sizeof(keycoords)/sizeof(keycoords[0])];

    int shift_cnt;
    int ctrl_cnt;
    int alt_cnt;
    bool is_capslock;
    int layout;
};

void keyboard_update_layout(Keyboard *keyboard)
{
    int i = 0;
    for (i = 0 ; i < sizeof(keycoords)/sizeof(keycoords[0]) ; i++) {
        Key *key = keyboard->keys[i];
        if (keyboard->layout == 1) {
            if (keyboard->shift_cnt > 0) {
                if (key->one_k_s)
                    key_kor_shift(key);
                else if (!key->one_k && key->one_s)
                    key_shift(key);
                else if (key->one_k)
                    key_kor(key);
            } else if (keyboard->is_capslock) {
                ERR("capslock");
                if (key->one_k_s &&
                        ((16 <= key->keycoord->code && key->keycoord->code <= 25) ||
                        (30 <= key->keycoord->code && key->keycoord->code <= 38) ||
                        (44 <= key->keycoord->code && key->keycoord->code <= 50)))
                    key_kor_shift(key);
                else if (key->one_k)
                    key_kor(key);
            } else {
                if (key->one_k)
                    key_kor(key);
                else if (!key->one_k && key->one_s)
                    key_normal(key);
            }
        } else {
            if (keyboard->shift_cnt > 0 ) {
                if (key->one_s)
                    key_shift(key);
            } else if (keyboard->is_capslock) {
                if (key->one_s &&
                        ((16 <= key->keycoord->code && key->keycoord->code <= 25) ||
                        (30 <= key->keycoord->code && key->keycoord->code <= 38) ||
                        (44 <= key->keycoord->code && key->keycoord->code <= 50)))
                    key_shift(key);
            } else {
                key_normal(key);
            }
        }
    }
    nemoshow_dispatch_frame(keyboard->show);
}

void keyboard_focus(Keyboard *keyboard)
{
    _nemoshow_item_motion_bounce(keyboard->bg0, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "sx", 0.9, 1.0, "sy", 0.9, 1.0, NULL);
    _nemoshow_item_motion_bounce(keyboard->bg1, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "sx", 0.9, 1.0, "sy", 0.9, 1.0, NULL);
    int gamma = 50;
    int i = 0;
    for (i = 0 ; i < sizeof(keycoords)/sizeof(keycoords[0]) ; i++) {
        Key *key = keyboard->keys[i];
        int delay0, delay1;
        if (key->keycoord->layer == 0) {
            delay0 = gamma * i;
            delay1 = gamma * i + gamma;
        } else if (key->keycoord->layer == 1) {
            delay0 = gamma * (i);
            delay1 = gamma * (i) + gamma;
        } else if (key->keycoord->layer == 2) {
            delay0 = gamma * (i - 15);
            delay1 = gamma * (i - 15) + gamma;
        } else if (key->keycoord->layer == 3) {
            delay0 = gamma * (i - 29);
            delay1 = gamma * (i - 29) + gamma;
        } else if (key->keycoord->layer == 4) {
            delay0 = gamma * (i - 43);
            delay1 = gamma * (i - 43) + gamma;
        } else if (key->keycoord->layer == 5) {
            delay0 = gamma * (i - 56);
            delay1 = gamma * (i - 56) + gamma;
        } else if (key->keycoord->layer == 6) {
            delay0 = gamma * (i - 68);
            delay1 = gamma * (i - 68) + gamma;
        }
        delay0 += gamma * (key->keycoord->layer + 1);
        delay1 += gamma * (key->keycoord->layer + 1);
        _nemoshow_item_motion_bounce(key->group, NEMOEASE_CUBIC_OUT_TYPE, 300, delay0,
                "sx", 0.9, 1.0, "sy", 0.9, 1.0, NULL);
    }
    nemoshow_dispatch_frame(keyboard->show);
}

void keyboard_show(Keyboard *keyboard)
{
    nemotimer_set_timeout(keyboard->bg_timer, 10);
    nemowidget_show(keyboard->widget, 0, 0, 0);
    _nemoshow_item_motion(keyboard->bg0, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "sx", 1.0, "sy", 1.0,
            "alpha", 0.9, NULL);
    _nemoshow_item_motion(keyboard->bg1, NEMOEASE_CUBIC_OUT_TYPE, 1000, 250,
            "sx", 1.0, "sy", 1.0,
            "alpha", 0.9, NULL);

    int gamma = 50;
    int i = 0;
    for (i = 0 ; i < sizeof(keycoords)/sizeof(keycoords[0]) ; i++) {
        Key *key = keyboard->keys[i];
        int delay0, delay1;
        if (key->keycoord->layer == 0) {
            delay0 = gamma * i;
            delay1 = gamma * i + gamma;
        } else if (key->keycoord->layer == 1) {
            delay0 = gamma * (i);
            delay1 = gamma * (i) + gamma;
        } else if (key->keycoord->layer == 2) {
            delay0 = gamma * (i - 15);
            delay1 = gamma * (i - 15) + gamma;
        } else if (key->keycoord->layer == 3) {
            delay0 = gamma * (i - 29);
            delay1 = gamma * (i - 29) + gamma;
        } else if (key->keycoord->layer == 4) {
            delay0 = gamma * (i - 43);
            delay1 = gamma * (i - 43) + gamma;
        } else if (key->keycoord->layer == 5) {
            delay0 = gamma * (i - 56);
            delay1 = gamma * (i - 56) + gamma;
        } else if (key->keycoord->layer == 6) {
            delay0 = gamma * (i - 68);
            delay1 = gamma * (i - 68) + gamma;
        }
        delay0 += gamma * (key->keycoord->layer + 1);
        delay1 += gamma * (key->keycoord->layer + 1);
        key_translate(key, NEMOEASE_CUBIC_OUT_TYPE, 300, delay0,
                keycoords[i].x, keycoords[i].y);
        key_show(key, NEMOEASE_CUBIC_OUT_TYPE, 300, delay1);
    }

    nemoshow_dispatch_frame(keyboard->show);
}

static void keyboard_hide(Keyboard *keyboard)
{
    int gamma = 50;
    int i = 0;
    for (i = sizeof(keycoords)/sizeof(keycoords[0]) -1 ; i >= 0 ; i--) {
        Key *key = keyboard->keys[i];
        int delay0, delay1;
        if (key->keycoord->layer == 0) {
            delay0 = gamma * i;
            delay1 = gamma * i + gamma;
        } else if (key->keycoord->layer == 1) {
            delay0 = gamma * (i);
            delay1 = gamma * (i) + gamma;
        } else if (key->keycoord->layer == 2) {
            delay0 = gamma * (i - 15);
            delay1 = gamma * (i - 15) + gamma;
        } else if (key->keycoord->layer == 3) {
            delay0 = gamma * (i - 29);
            delay1 = gamma * (i - 29) + gamma;
        } else if (key->keycoord->layer == 4) {
            delay0 = gamma * (i - 43);
            delay1 = gamma * (i - 43) + gamma;
        } else if (key->keycoord->layer == 5) {
            delay0 = gamma * (i - 56);
            delay1 = gamma * (i - 56) + gamma;
        } else if (key->keycoord->layer == 6) {
            delay0 = gamma * (i - 68);
            delay1 = gamma * (i - 68) + gamma;
        }
        delay0 += gamma * (key->keycoord->layer + 1);
        delay1 += gamma * (key->keycoord->layer + 1);
        key_translate(key, NEMOEASE_CUBIC_OUT_TYPE, 300, delay0,
                keyboard->width/2, keyboard->height/2);
        key_hide(key, NEMOEASE_CUBIC_OUT_TYPE, 300, delay1);
    }

    nemotimer_set_timeout(keyboard->bg_timer, 0);
    nemowidget_hide(keyboard->widget, 0, 0, 0);
    _nemoshow_item_motion(keyboard->bg0, NEMOEASE_CUBIC_OUT_TYPE, 1000, 250,
            "sx", 0.0, "sy", 0.0,
            "alpha", 0.0, NULL);
    _nemoshow_item_motion(keyboard->bg1, NEMOEASE_CUBIC_OUT_TYPE, 1000, 250,
            "sx", 0.0, "sy", 0.0,
            "alpha", 0.0, NULL);

    nemoshow_dispatch_frame(keyboard->show);
}
static void _keyboard_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    if (nemoshow_event_is_done(event) != 0) return;

    struct nemoshow *show = nemowidget_get_show(widget);
    struct nemotool *tool = nemowidget_get_tool(widget);
    Key *key = userdata;
    Keyboard *keyboard = nemowidget_grab_get_data(grab, "keyboard");
    if (nemoshow_event_is_down(show, event)) {
        key_down(key);
        if (key->type == 1) {
            ERR("PRESSED: %s(%d)", key->keycoord->name, key->keycoord->code);
            nemotool_keyboard_key(tool, get_time(), key->keycoord->code, 1);
            if (!strcmp(key->keycoord->name, "SHIFT")) {
                keyboard->shift_cnt++;
            } else if (!strcmp(key->keycoord->name, "CTRL")) {
                keyboard->ctrl_cnt++;
            } else if (!strcmp(key->keycoord->name, "ALT")) {
                keyboard->alt_cnt++;
            }
            keyboard_update_layout(keyboard);
        } else if (nemoshow_event_get_tapcount(event) == 1 && key->type == 2) {
            nemoshow_view_move(show,
                    nemoshow_event_get_serial_on(event, 0));
        } else {
            if (strcmp(key->keycoord->name, "CAPS") &&
                    strcmp(key->keycoord->name, "KOR") &&
                    strcmp(key->keycoord->name, "FOCUS") &&
                    strcmp(key->keycoord->name, "QUIT")) {
                nemotimer_set_timeout(key->repeat_timer, 500);
            }
        }
    } else if (nemoshow_event_is_up(show, event)) {
        key_up(key);
        nemotimer_set_timeout(key->repeat_timer, 0);
        if (nemoshow_event_is_single_click(show, event)) {
            if (!strcmp(key->keycoord->name, "QUIT")) {
                keyboard_hide(keyboard);
                nemowidget_win_exit_after(
                        nemowidget_get_top_widget(keyboard->widget), 1100);
                return;
            }
        }

        if (!strcmp(key->keycoord->name, "FOCUS")) {
            double x, y;
            nemowidget_transform_to_window(widget,
                    nemoshow_event_get_x(event),
                    nemoshow_event_get_y(event),
                    &x, &y);
            nemoshow_view_focus_on(show, x, y);
            nemotool_keyboard_layout(tool,
                    LAYOUT[keyboard->layout]);
            nemotool_keyboard_enter(keyboard->tool);
            keyboard_focus(keyboard);
            ERR("FOCUSED: %lf %lf", x, y);
            return;
        }

        if (key->type == 1) {
            ERR("RELEASED: %s(%d)", key->keycoord->name, key->keycoord->code);
            nemotool_keyboard_key(tool, get_time(), key->keycoord->code, 0);
            if (!strcmp(key->keycoord->name, "SHIFT")) {
                keyboard->shift_cnt--;
            } else if (!strcmp(key->keycoord->name, "CTRL")) {
                keyboard->ctrl_cnt--;
            } else if (!strcmp(key->keycoord->name, "ALT")) {
                keyboard->alt_cnt--;
            }
        } else {
            ERR("PRESSED: %s(%d)", key->keycoord->name, key->keycoord->code);
            nemotool_keyboard_key(tool, get_time(), key->keycoord->code, 1);
            ERR("RELEASED: %s(%d)", key->keycoord->name, key->keycoord->code);
            nemotool_keyboard_key(tool, get_time(), key->keycoord->code, 0);

            if (!strcmp(key->keycoord->name, "CAPS")) {
                keyboard->is_capslock = !keyboard->is_capslock;
                key_on(keyboard->keys[44], keyboard->is_capslock);
            }
            if (!strcmp(key->keycoord->name, "KOR")) {
                if (keyboard->layout == 1) {
                    keyboard->layout = 0;
                    key_special(keyboard->keys[72], false);
                } else if (keyboard->layout == 0) {
                    keyboard->layout = 1;
                    key_special(keyboard->keys[72], true);
                }

                ERR("%s", LAYOUT[keyboard->layout]);
                nemotool_keyboard_layout(tool, LAYOUT[keyboard->layout]);
            }
        }
        keyboard_update_layout(keyboard);
    }
    nemoshow_dispatch_frame(keyboard->show);
}

static void _keyboard_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);
    Keyboard *keyboard = userdata;

    if (!keyboard->initialized) {
        if (keyboard->id) {
            // XXX: At launching time, interface can not be initialized yet.
            nemotool_keyboard_enter(keyboard->tool);
        }
        nemotool_keyboard_layout(keyboard->tool, LAYOUT[keyboard->layout]);
        keyboard->initialized = true;
    }

    if (nemoshow_event_is_down(show, event)) {
        if ((nemoshow_event_get_tapcount(event) >= 2) &&
                (keyboard->shift_cnt <= 0) &&
                (keyboard->alt_cnt <= 0) &&
                (keyboard->ctrl_cnt <= 0)) {
            ERR("USED ALL");
            nemoshow_event_set_done_all(event);
            List *l;
            NemoGrab *grab;
            LIST_FOR_EACH(widget->usergrabs, l, grab) {
                NemoWidgetGrab *wgrab = grab->userdata;
                Key *key = nemowidget_grab_get_userdata(wgrab);
                key_up(key);
                nemotimer_set_timeout(key->repeat_timer, 0);
            }
            nemoshow_dispatch_frame(keyboard->show);
            return;
        }

        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event), nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            Key *key = nemoshow_one_get_userdata(one);
            NemoWidgetGrab *grab = nemowidget_create_grab(widget,
                    event, _keyboard_grab_event, key);
            nemowidget_grab_set_data(grab, "keyboard", keyboard);
        }
    }
}

static void _keyboard_background_timeout(struct nemotimer *timer, void *userdata)
{
    Keyboard *keyboard = userdata;

    int timeout = BG_TIMEOUT - 10;
    //ERR("%d", keyboard->bg_timer_idx);
    if (keyboard->bg_timer_idx == 0) {
        _nemoshow_item_motion(ONE_CHILD0(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT1, NULL);
        _nemoshow_item_motion(ONE_CHILD1(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT2, NULL);
        _nemoshow_item_motion(ONE_CHILD2(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT0, NULL);
        keyboard->bg_timer_idx++;
    } else if (keyboard->bg_timer_idx == 1) {
        _nemoshow_item_motion(ONE_CHILD0(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT2, NULL);
        _nemoshow_item_motion(ONE_CHILD1(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT0, NULL);
        _nemoshow_item_motion(ONE_CHILD2(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT1, NULL);
        keyboard->bg_timer_idx++;
    } else {
        _nemoshow_item_motion(ONE_CHILD0(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT0, NULL);
        _nemoshow_item_motion(ONE_CHILD1(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT1, NULL);
        _nemoshow_item_motion(ONE_CHILD2(keyboard->gradient0), NEMOEASE_LINEAR_TYPE, timeout, 0,
                    "fill", GRADIENT2, NULL);
        keyboard->bg_timer_idx = 0;
    }

    nemotimer_set_timeout(timer, BG_TIMEOUT);
    nemoshow_dispatch_frame(keyboard->show);
}

Keyboard *keyboard_create(NemoWidget *parent, int width, int height, const char *id, const char *layout)
{
    Keyboard *keyboard = calloc(sizeof(Keyboard), 1);
    keyboard->tool = nemowidget_get_tool(parent);
    keyboard->show = nemowidget_get_show(parent);
    keyboard->width = width;
    keyboard->height = height;
    if (id) keyboard->id = strdup(id);

    keyboard->layout = -1;
    int i = 0;
    while (LAYOUT[i]) {
        if (!strcmp(LAYOUT[i], layout)) {
            keyboard->layout = i;
            break;
        }
        i++;
    }
    if (keyboard->layout < 0) {
        ERR("no layout specified: fallback to %s", LAYOUT[0]);
        keyboard->layout = 0;
    }

    NemoWidget *widget;
    struct showone *canvas;
    struct showone *group;
    keyboard->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _keyboard_event, keyboard);
    canvas = nemowidget_get_canvas(widget);
    group = GROUP_CREATE(canvas);

    struct showone *gradient;
    struct showone *one;

    keyboard->gradient0 = gradient = LINEAR_GRADIENT_CREATE(0, 0, width, height);
    STOP_CREATE(gradient, GRADIENT0, 0.0);
    STOP_CREATE(gradient, GRADIENT1, 0.5);
    STOP_CREATE(gradient, GRADIENT2, 1.0);
    keyboard->bg0 = one = SVG_PATH_CREATE(group, width, height, APP_ICON_DIR"/BG-01.svg");
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, width/2, height/2);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));
    nemoshow_item_set_shader(one, gradient);
    nemoshow_item_scale(one, 0.0, 1.0);
    nemoshow_item_set_alpha(one, 0.0);

    keyboard->gradient1 = gradient = LINEAR_GRADIENT_CREATE(0, 0, width, height);
    STOP_CREATE(gradient, GRADIENT2, 0.0);
    STOP_CREATE(gradient, GRADIENT1, 0.5);
    STOP_CREATE(gradient, GRADIENT0, 1.0);
    keyboard->bg1 = one = SVG_PATH_CREATE(group, width, height, APP_ICON_DIR"/BG-02.svg");
    nemoshow_item_translate(one, width/2, height/2);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_fill_color(one, RGBA(BLACK));
    nemoshow_item_set_shader(one, gradient);
    nemoshow_item_scale(one, 1.0, 0.0);
    nemoshow_item_set_alpha(one, 0.0);

    keyboard->bg_timer = TOOL_ADD_TIMER(keyboard->tool, 0,
            _keyboard_background_timeout, keyboard);

    for (i = 0 ; i < sizeof(keycoords)/sizeof(keycoords[0]) ; i++) {
        keyboard->keys[i] = key_create(keyboard->tool, group, width/2, height/2, &(keycoords[i]));
    }

    return keyboard;
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Keyboard *keyboard = userdata;
    keyboard_hide(keyboard);
    nemowidget_win_exit_after(win, 1100);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *layout;
};

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        char *prefix = "config";
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/layout", prefix);
        temp = xml_get_value(xml, buf, "name");
        if (temp && strlen(temp) > 0) {
            app->layout = strdup(temp);
        }

        xml_unload(xml);
    }


    struct option options[] = {
        {"layout", required_argument, NULL, 'l'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "l:", options, NULL)) != -1) {
        switch(opt) {
            case 'l':
                if (app->layout) free(app->layout);
                app->layout = strdup(optarg);
                break;
            default:
                break;
        }
    }


    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->layout) free(app->layout);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);
    if (app->config->id)
        nemoshow_view_focus_to(nemowidget_get_show(win), app->config->id);
    nemowidget_win_set_type(win, "keypad");
    nemowidget_win_enable_state(win, "keypad", false);
    nemowidget_win_enable_move(win, 2);
    nemowidget_win_enable_scale(win, 2);
    nemowidget_win_enable_rotate(win, 2);

    Keyboard *keyboard = keyboard_create(win, app->config->width, app->config->height, app->config->id, app->layout);
    nemowidget_append_callback(win, "exit", _win_exit, keyboard);
    keyboard_show(keyboard);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
