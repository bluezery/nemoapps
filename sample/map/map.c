#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "xemoutil.h"
#include "showhelper.h"
#include "nemowrapper.h"
#include "widget.h"
#include "widgets.h"
#include "nemoui.h"
#include "map.h"

#define ACTIVE      0x1EDCDCFF
#define BASE        0x0F6E6EFF
#define INACTIVE    0x094242FF
#define BLINK       0xFF8C32FF

#if 0
static int exes_idx = 0;
typedef struct _Exe Exe;

typedef struct _Sample Sample;
struct _Sample {
    char *exec;
    char *path;
};

List *samples = NULL;

/*
Sample samples[] = {
    {NULL, "/usr/bin/nemotest_bar"},
    {NULL, "/usr/bin/nemotest_pie"},
    {NULL, "/usr/bin/nemotest_pies"},
    {NULL, "/usr/bin/nemotest_animation"},
    {"/usr/bin/nemoplay:-f", "/home/root/SAMPLES/MILITARY/Avengers.mp4"},
    {"/usr/bin/nemoplay:-f", "/home/root/SAMPLES/MILITARY/Avengers2.mp4"},
    {"/usr/bin/nemoplay:-f", "/home/root/SAMPLES/MILITARY/Terminator genesis.mp4"}
};
*/

static Sample *_sample_create(const char *exec, const char *path)
{
    Sample *sample;
    sample = calloc(sizeof(Sample), 1);
    if (exec) sample->exec = strdup(exec);
    if (path) sample->path = strdup(path);
    return sample;
}

static void _samples_load(const char *path)
{
    Sample *sample;
    sample = _sample_create(NULL, "/usr/bin/nemotest_bar");
    samples = list_append(samples, sample);
    sample = _sample_create(NULL, "/usr/bin/nemotest_pie");
    samples = list_append(samples, sample);
    sample = _sample_create(NULL, "/usr/bin/nemotest_pies");
    samples = list_append(samples, sample);
    sample = _sample_create(NULL, "/usr/bin/nemotest_animation");
    samples = list_append(samples, sample);
    ERR("%d", list_count(samples));

    if (!path) return;
    List *files = fileinfo_readdir(path);
    FileInfo *file;
    List *l;
    LIST_FOR_EACH(files, l, file) {
        if (file->is_dir) continue;
        if (!file->magic_str) continue;
        if (!file->ext) continue;
        if (file_is_video(file->path)) {
            sample =  _sample_create("/usr/bin/nemoplay:-f",
                    file->path);
            samples = list_append(samples, sample);
            ERR("%s, %s", sample->exec, sample->path);
        }
    }
}

static void _sample_execute(struct nemoshow *show, Sample sample, int tx, int ty)
{
    float x, y;
    nemoshow_transform_to_viewport(show, tx, ty, &x, &y);

    nemoshow_view_set_anchor(show, 0.5, 0.5);
    ERR("%s, %s", sample.exec, sample.path);
    if (!sample.exec) {
        nemoshow_view_execute(show, "app", sample.path,
                "path %s x %f y %f r %f",
                sample.path, x, y, 0.0);
    } else {
        char path[PATH_MAX];
        char name[PATH_MAX];
        char args[PATH_MAX];

        char *buf;
        char *tok;
        buf = strdup(sample.exec);
        tok = strtok(buf, ";");
        snprintf(name, PATH_MAX, "%s", tok);
        snprintf(path, PATH_MAX, "%s", tok);
        tok = strtok(NULL, ";");
        snprintf(args, PATH_MAX, "%s;%s", tok, sample.path);
        free(buf);
        nemoshow_view_execute(show, "app", name,
                "path %s x %f y %f r %f args \"%s\"",
                path, x, y, 0.0, args);
    }
}

static void _execute(struct nemoshow *show, int tx, int ty)
{
    if (list_count(samples) == 0) return;
    Sample *sample = LIST_DATA(list_get_nth(samples, exes_idx));
    _sample_execute(show, *sample, tx, ty);
    exes_idx++;
    if (exes_idx >= list_count(samples)) exes_idx = 0;
}
#endif

typedef struct _Sign Sign;
struct _Sign {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    int w, h;
    struct showone *blur;
    struct showone *group;
    struct showone *bg;
    struct showone *text;
    struct showone *deco;
    struct showone *line;
    struct showone *pin;

    struct nemotimer *timer;
};

static inline void sign_destroy(Sign *sign)
{
    nemotimer_destroy(sign->timer);
    nemoshow_one_destroy(sign->group);
    nemoshow_one_destroy(sign->blur);
    free(sign);
}

static inline void _sign_timeout(struct nemotimer *timer, void *userdata)
{
    Sign *sign = userdata;
    if (sign->deco) {
        nemoshow_item_rotate(sign->deco, 0.0);
        _nemoshow_item_motion(sign->deco, NEMOEASE_LINEAR_TYPE, 1000, 0,
                "ro", 360.0,
                NULL);
    }

    if (sign->pin) {
        nemoshow_item_rotate(sign->pin, 0.0);
        _nemoshow_item_motion(sign->pin, NEMOEASE_LINEAR_TYPE, 1000, 0,
                "ro", 360.0,
                NULL);
    }

    nemotimer_set_timeout(timer, 1000);
}

static inline Sign *sign_create(struct nemotool *tool, struct nemoshow *show, struct showone *parent, int w, int h)
{
    RET_IF(!tool, NULL);
    RET_IF(!show, NULL);

    Sign *sign = calloc(sizeof(Sign), 1);
    sign->tool = tool;
    sign->show = show;
    sign->parent = parent;
    sign->w = w;
    sign->h = h;
    sign->blur = BLUR_CREATE("solid", 10);
    sign->group = GROUP_CREATE(parent);

    sign->timer = TOOL_ADD_TIMER(tool, 0, _sign_timeout, sign);

    return sign;
}

static inline void sign_set_bg(Sign *sign, const char *svgpath, int w, int h)
{
    struct showone *one;

    one = SVG_PATH_CREATE(sign->group, w, h, svgpath);
    nemoshow_item_set_fill_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 1);

    nemoshow_item_set_filter(one, sign->blur);
    nemoshow_item_set_alpha(one, 0.0);
    sign->bg = one;
}

static inline void sign_set_text(Sign *sign, const char *svgpath, int w, int h, int dx, int dy)
{
    struct showone *one;
    one = SVG_PATH_CREATE(sign->group, w, h, svgpath);
    nemoshow_item_set_fill_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 1);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, dx, dy);
    nemoshow_item_set_filter(one, sign->blur);
    sign->text = one;
}

static inline void sign_set_decoration(Sign *sign, const char *svgpath, int w, int h, int dx, int dy)
{
    struct showone *one;
    one = SVG_PATH_CREATE(sign->group, w, h, svgpath);
    nemoshow_item_set_fill_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, dx + w/2, dy + h/2);
    nemoshow_item_set_filter(one, sign->blur);
    sign->deco = one;
}

static inline void sign_translate(Sign *sign, uint32_t easetype, int duration, int delay, int tx, int ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(sign->group, easetype, duration, delay,
                "tx", (double)tx, "ty", (double)ty,
                NULL);
    } else {
        nemoshow_item_translate(sign->group, tx, ty);
    }
}

static inline void sign_line_to(Sign *sign, int x0, int y0, int x1, int y1, int x2, int y2)
{
    struct showone *one;
    one = PATH_CREATE(sign->group);
    nemoshow_item_path_moveto(one, x0, y0);
    nemoshow_item_path_lineto(one, x1, y1);
    nemoshow_item_path_lineto(one, x2, y2);
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 2);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_filter(one, sign->blur);
    sign->line = one;

    one = SVG_PATH_CREATE(sign->group, 40, 40, SIGN_DIR"/pin.svg");
    nemoshow_item_set_fill_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, x2, y2);
    nemoshow_item_set_filter(one, sign->blur);
    sign->pin = one;

}

static inline void sign_hide(Sign *sign, uint32_t easetype, int duration, int delay)
{
    if (duration > 0) {
        if (sign->bg) {
            _nemoshow_item_motion(sign->bg, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 0.0,
                    NULL);
        }
        if (sign->text) {
            _nemoshow_item_motion(sign->text, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 0.0,
                    NULL);
        }
        if (sign->deco) {
            _nemoshow_item_motion(sign->deco, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 0.0,
                    NULL);
        }
        if (sign->line) {
            _nemoshow_item_motion(sign->line, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 0.0,
                    NULL);
        }
        if (sign->pin) {
            _nemoshow_item_motion(sign->pin, easetype, duration, delay,
                    "alpha", 0.0,
                    "to", 0.0,
                    NULL);
        }
    } else {
        if (sign->bg)
            nemoshow_item_set_alpha(sign->bg, 0.0);
        if (sign->text)
            nemoshow_item_set_alpha(sign->text, 0.0);
        if (sign->deco)
            nemoshow_item_set_alpha(sign->deco, 0.0);
        if (sign->line)
            nemoshow_item_set_alpha(sign->line, 0.0);
        if (sign->pin)
            nemoshow_item_set_alpha(sign->pin, 0.0);
    }
    nemotimer_set_timeout(sign->timer, 0);
}

static inline void _sign_hide_destroy_done(struct nemotimer *timer, void *userdata)
{
    Sign *sign = userdata;
    sign_destroy(sign);
    nemotimer_destroy(timer);
}

static inline void sign_hide_destroy(Sign *sign, uint32_t easetype, int duration, int delay)
{
    sign_hide(sign, easetype, duration, delay);

    struct nemotimer *timer;
    timer = nemotimer_create(sign->tool);
    nemotimer_set_timeout(timer, duration + delay);
    nemotimer_set_callback(timer, _sign_hide_destroy_done);
    nemotimer_set_userdata(timer, sign);
}


static inline void sign_show(Sign *sign, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(sign->timer, 100);
    if (duration > 0) {
        if (sign->bg) {
            nemoshow_item_set_to(sign->bg, 0.0);
            _nemoshow_item_motion(sign->bg, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 1.0,
                    NULL);
        }
        if (sign->text) {
            nemoshow_item_set_to(sign->text, 0.0);
            _nemoshow_item_motion(sign->text, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 1.0,
                    NULL);
        }
        if (sign->deco) {
            nemoshow_item_set_to(sign->deco, 0.0);
            _nemoshow_item_motion(sign->deco, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 1.0,
                    NULL);
        }
        if (sign->line) {
            nemoshow_item_set_to(sign->line, 0.0);
            _nemoshow_item_motion(sign->line, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 1.0,
                    NULL);
        }
        if (sign->pin) {
            nemoshow_item_set_to(sign->pin, 0.0);
            _nemoshow_item_motion(sign->pin, easetype, duration, delay,
                    "alpha", 1.0,
                    "to", 1.0,
                    NULL);
        }
    } else {
        if (sign->bg)
            nemoshow_item_set_alpha(sign->bg, 1.0);
        if (sign->text)
            nemoshow_item_set_alpha(sign->text, 1.0);
        if (sign->deco)
            nemoshow_item_set_alpha(sign->deco, 1.0);
        if (sign->line)
            nemoshow_item_set_alpha(sign->line, 1.0);
        if (sign->pin)
            nemoshow_item_set_alpha(sign->pin, 1.0);
    }

    if (sign->timer) {
        nemotimer_destroy(sign->timer);
    }
    struct nemotimer *timer;
    timer = nemotimer_create(sign->tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _sign_timeout);
    nemotimer_set_userdata(timer, sign);
    sign->timer = timer;
}

typedef struct _TestMap TestMap;
struct _TestMap {
    int w, h;
    struct nemoshow *show;
    struct nemotool *tool;
    Map *map;

    NemoWidget *win;

    struct showone *blur;
    NemoWidget *vector;
    struct showone *bg0;
    struct showone *bg1;
    struct showone *bg2;
    struct nemotimer *back_timer;

    NemoWidget *sign_vector;
    Sign *sign0, *sign1, *sign2;

    struct nemotimer *timer;
};

void test_map_destroy(TestMap *test)
{
    nemoshow_one_destroy(test->bg0);
    nemoshow_one_destroy(test->bg1);
    nemoshow_one_destroy(test->bg2);

    nemowidget_destroy(test->vector);
    map_destroy(test->map);
    free(test);
}

static void _map_item_clicked(MapItem *it, void *userdata)
{
    TestMap *test = userdata;
    DataMapItem *data = it->userdata;
    ERR("zoom: %s(%d)", data->name, data->id);
    map_zoom_item(test->map, it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    if (data->id == 100) {
        sign_show(test->sign0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        sign_show(test->sign1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 300);
        sign_show(test->sign2, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 600);
    } else {
        sign_hide(test->sign2, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        sign_hide(test->sign1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 300);
        sign_hide(test->sign0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 600);
    }
}

static void _circle_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);
    TestMap *test = userdata;
    if (nemoshow_event_is_single_click(show, event)) {
        MapItem *it = LIST_DATA(LIST_FIRST(test->map->items));
        DataMapItem *data = it->userdata;
        ERR("zoom: %s(%d)", data->name, data->id);
        map_zoom_item(test->map, it, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

        sign_hide(test->sign2, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
        sign_hide(test->sign1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 300);
        sign_hide(test->sign0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 600);
    }
}

static void _test_map_back_timeout(struct nemotimer *timer, void *userdata)
{
    TestMap *test = userdata;

    nemoshow_item_rotate(test->bg0, 0.0);
    nemoshow_item_rotate(test->bg1, 360.0);
    nemoshow_item_rotate(test->bg2, 0.0);
    _nemoshow_item_motion(test->bg0, NEMOEASE_LINEAR_TYPE, 9500, 0,
            "ro", 360.0,
            NULL);
    _nemoshow_item_motion(test->bg1, NEMOEASE_LINEAR_TYPE, 10000, 0,
            "ro", 0.0,
            NULL);
    _nemoshow_item_motion(test->bg2, NEMOEASE_LINEAR_TYPE, 10000, 0,
            "ro", 360.0,
            NULL);

    nemotimer_set_timeout(timer, 10000);
    nemoshow_dispatch_frame(test->show);
}

TestMap *test_map_create(NemoWidget *win, int width, int height)
{
    TestMap *test = calloc(sizeof(TestMap), 1);
    test->w = width;
    test->h = height;
    test->show = nemowidget_get_show(win);
    test->tool = nemowidget_get_tool(win);

    // Circle
    NemoWidget *vector;
    test->vector = vector = nemowidget_create_vector(win, width, height);
    nemowidget_append_callback(vector, "event", _circle_event, test);
    nemowidget_show(vector, 0, 0, 0);

    struct showone *canvas = nemowidget_get_canvas(vector);

    struct showone *blur;
    blur = test->blur = BLUR_CREATE("solid", 5);

    struct showone *one;
    one = SVG_PATH_CREATE(canvas, test->w, test->h, BACK_DIR"/bg0.svg");
    nemoshow_item_set_fill_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 2.0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, test->w/2, test->h/2);
    nemoshow_item_set_filter(one, blur);
    nemoshow_item_set_pick(one, NEMOSHOW_ITEM_PATH_PICK);
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    test->bg0 = one;

    one = SVG_PATH_CREATE(canvas, test->w, test->h, BACK_DIR"/bg1.svg");
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 2.0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, test->w/2, test->h/2);
    nemoshow_item_set_filter(one, blur);
    test->bg1 = one;

    one = SVG_PATH_CREATE(canvas, test->w, test->h, BACK_DIR"/bg2.svg");
    nemoshow_item_set_stroke_color(one, RGBA(ACTIVE));
    nemoshow_item_set_stroke_width(one, 2.0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_translate(one, test->w/2, test->h/2);
    nemoshow_item_set_filter(one, blur);
    test->bg2 = one;

    test->back_timer = TOOL_ADD_TIMER(test->tool, 0, _test_map_back_timeout, test);

    Sign *sign;
    sign = sign_create(test->tool, test->show, canvas, 165, 55);
    sign_set_bg(sign, SIGN_DIR"/area_bg1.svg", 165, 55);
    sign_set_decoration(sign, SIGN_DIR"/circle.svg", 25, 25, 12, (55-25)/2);
    sign_set_text(sign, SIGN_DIR"/area_text1.svg", 100, 21, 45, (55-21)/2);
    sign_translate(sign, 0, 0, 0, 50, 300);
    sign_line_to(sign, 164, 48, 190, 48, 220, 120);
    test->sign0 = sign;

    sign = sign_create(test->tool, test->show, canvas, 165, 55);
    sign_set_bg(sign, SIGN_DIR"/area_bg0.svg",165, 55);
    sign_set_decoration(sign, SIGN_DIR"/circle.svg", 25, 25, 12, (55-25)/2);
    sign_set_text(sign, SIGN_DIR"/area_text2.svg", 100, 21, 45, (55-21)/2);
    sign_translate(sign, 0, 0, 0, 290, 200);
    sign_line_to(sign, 3, 50, 3, 62, 130, 280);
    test->sign1 = sign;

    sign = sign_create(test->tool, test->show, canvas, 165, 55);
    sign_set_bg(sign, SIGN_DIR"/area_bg0.svg", 165, 55);
    sign_set_decoration(sign, SIGN_DIR"/circle.svg", 25, 25, 12, (55-25)/2);
    sign_set_text(sign, SIGN_DIR"/area_text3.svg", 100, 21, 45, (55-21)/2);
    sign_translate(sign, 0, 0, 0, 520, 300);
    sign_line_to(sign, 0, 48, -15, 48, -50, 150);
    test->sign2 = sign;

    List *it_datas = csv_load_datamap_items(MAP_DATA_DIR"/"MAP_DATA_FILE);
    csv_calculate_datamap_items(it_datas);

    int gap = 40;
    int w = width - gap * 2;
    int h = width - gap * 2;
    Map *map;
    test->map = map = map_create(win, w, h);
    map_translate(map, 0, 0, 0, gap, gap);
    map_set_clip_circle(map, w/2);

    DataMapItem *it_data;
    List *l;
    LIST_FOR_EACH(it_datas, l, it_data) {
        // Map BG
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, MAP_BG_DIR"/%s", it_data->bg.path);
        MapItem *it = map_append_item(map, path,
                it_data->bg.x, it_data->bg.y,
                it_data->bg.w, it_data->bg.h);
        map_item_set_stroke(it, 0, 0, 0, WHITE, 1);
        //if (it_data->id == 100) {
            map_item_set_callback(it, "clicked", _map_item_clicked, test);
        //}
        map_item_set_userdata(it, it_data);
    }
    // Center alignment at first
    map_content_translate(map, 0, 0, 0, -(map->cx1 - map->cx0)/2, -(map->cy1 - map->cy0)/2);

    return test;
}

void test_map_show(TestMap *test, uint32_t easetype, int duration, int delay)
{
    map_show(test->map, easetype, duration, delay);
    MapItem *it = LIST_DATA(LIST_FIRST(test->map->items));
    map_zoom_item(test->map, it, NEMOEASE_CUBIC_OUT_TYPE,
            duration - 500, 500 + delay);

    nemowidget_show(test->vector, easetype, duration, delay);
    _nemoshow_item_motion(test->bg0, easetype, duration, delay,
            "alpha", 1.0,
            NULL);
    _nemoshow_item_motion(test->bg1, easetype, duration, delay,
            "alpha", 0.8,
            NULL);
    _nemoshow_item_motion(test->bg2, easetype, duration, delay,
            "alpha", 1.0,
            NULL);

    nemotimer_set_timeout(test->back_timer, 100);

    nemoshow_dispatch_frame(test->show);
}

void test_map_hide(TestMap *test, uint32_t easetype, int duration, int delay)
{
    nemotimer_set_timeout(test->back_timer, 0);
    _nemoshow_item_motion(test->bg0, easetype, duration, delay,
            "alpha", 0.0,
            NULL);
    _nemoshow_item_motion(test->bg1, easetype, duration, delay,
            "alpha", 0.0,
            NULL);
    _nemoshow_item_motion(test->bg2, easetype, duration, delay,
            "alpha", 0.0,
            NULL);

    sign_hide_destroy(test->sign0, easetype, duration, delay);
    sign_hide_destroy(test->sign1, easetype, duration, delay);
    sign_hide_destroy(test->sign2, easetype, duration, delay);

    map_hide(test->map, easetype, duration, delay);
    nemowidget_hide(test->vector, easetype, duration, delay);
}

static bool _win_is_map_area(NemoWidget *win, double x, double y, void *userdata)
{
    TestMap *test = userdata;

    if (CIRCLE_IN(test->w/2, test->h/2,
                test->w < test->h ? test->w/2 : test->h/2, x, y))
        return true;
    return false;
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    TestMap *test = userdata;

    test_map_hide(test, NEMOEASE_CUBIC_IN_TYPE, 1500, 0);

    nemowidget_win_exit_if_no_trans(win);
}

int main(int argc, char *argv[])
{
    char *path = NULL;
    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:", options, NULL))) {
        if (opt == -1) break;

        switch (opt) {
            case 'f':
                path = strdup(optarg);
                break;
            default:
                break;
        }
    }

    if (path) free(path);

    Config *base;
    base = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();

    NemoWidget *win;
    win = nemowidget_create_win_config(tool, APPNAME, base);

    TestMap *test = test_map_create(win, base->width, base->height);
    nemowidget_append_callback(win, "exit", _win_exit, test);
    nemowidget_win_set_event_area(win, _win_is_map_area, test);
    test_map_show(test, NEMOEASE_CUBIC_OUT_TYPE, 2000, 0);

    nemotool_run(tool);

    test_map_destroy(test);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(base);

    return 0;
}
