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

#include "nemoutil.h"
#include "nemowrapper.h"

#include "widget.h"

uint32_t TXT_COLOR[] =
{
    WHITE,
    BLACK
};

typedef struct _View View;
struct _View {
    struct nemoshow *show;
    struct showone *canvas;
    struct showone *blur;
    struct showone *font;
    struct showone *bg;
    struct showone *clip;
    struct showone *clip_group;
    struct showone *group;
    struct showone **txts;
    int txts_num;
    int color_idx;
    double width, height;
};

static View *_view_create(struct nemoshow *show, struct showone *canvas,
        const char *path)
{
    View *view = calloc(sizeof(View), 1);
    view->show = show;
    view->canvas = canvas;

    view->blur = BLUR_CREATE("solid", 5);
    view->font = FONT_CREATE("NanumGothic", "Light");

    double r = 500;
    int i = 0;
    view->bg = PATH_CIRCLE_CREATE(canvas, r);
    nemoshow_item_scale(view->bg, 0.15, 0.15);
    nemoshow_item_translate(view->bg, 512, 512);
    nemoshow_item_set_stroke_color(view->bg, RGBA(BLUE));
    nemoshow_item_set_stroke_width(view->bg, 4);

    nemoshow_item_set_filter(view->bg, view->blur);
    nemoshow_item_set_alpha(view->bg, 0.5);

    view->clip = PATH_CREATE(canvas);
    nemoshow_item_set_fill_color(view->clip, RGBA(WHITE));
    nemoshow_item_path_cmd(view->clip, "M12 12 L1012 12 L1012 1012 L12 1012 z");
    nemoshow_item_set_alpha(view->clip, 0.0);

    view->clip_group = GROUP_CREATE(canvas);
    nemoshow_item_set_clip(view->clip_group, view->clip);
    view->group = GROUP_CREATE(view->clip_group);
    nemoshow_item_translate(view->group, 12, 12);
    char **line_txt;
    int line_len;

    // Read a file
    line_txt = file_read_line(path, &line_len);
    if (!line_txt || !line_txt[0] || line_len <= 0) {
        ERR("Err: line_txt is NULL or no string or length is 0");
        return NULL;
    }

    int fontsize = 30;
    double width = 0;
    view->txts = calloc(sizeof(struct showone *), line_len);
    for (i = 0 ; i < line_len ; i++) {
        // Remove special characters (e.g. line feed, backspace, etc.)
        const char *utf8 = line_txt[i];
        unsigned int utf8_len = strlen(utf8);
        if (!utf8_len) continue;
        char *str = (char *)malloc(sizeof(char) * (utf8_len + 1));
        unsigned int str_len = 0;
        unsigned int j = 0;
        for (j = 0 ; j < utf8_len ; j++) {
            if ((utf8[j] >> 31) ||
                    (utf8[j] > 0x1F)) { // If it is valid character
                str[str_len] = utf8[j];
                str_len++;
            }
        }
        str = (char *)realloc(str, sizeof(char) * (str_len + 1));
        str[str_len] = '\0';

        struct showone *one;
        one = TEXT_CREATE(view->group, view->font, fontsize, str);
        nemoshow_item_set_fill_color(one, RGBA(TXT_COLOR[0]));
        nemoshow_item_set_alpha(one, 1.0);
        nemoshow_item_translate(one, 0, i * fontsize);

        if (i < 35) {
            nemoshow_item_set_alpha(one, 0.0);
            nemoshow_item_scale(one, 1.0f, 0.1f);
            _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 500, 10 * i,
                    1.0f, "alpha", 1.0f,
                    1.0f, "sy", 1.0f,
                    NULL);
        }
        free(line_txt[i]);
        view->txts[i] = one;
        view->txts_num++;
        nemoshow_one_update(one);
        if (width < NEMOSHOW_ITEM_AT(one, textwidth))
            width = NEMOSHOW_ITEM_AT(one, textwidth);
    }
    free(line_txt);
    view->width = width;
    view->height = view->txts_num * fontsize;

    _nemoshow_item_motion(view->bg, NEMOEASE_CUBIC_IN_TYPE, 1500, 0,
            "alpha", 1.0,
            "sx", 1.0,
            "sy", 1.0,
            NULL);

    PATH_CIRCLE_TO_RECT(view->bg, NEMOEASE_CUBIC_IN_TYPE, 1500, 500);

    return view;
}

#if 0
static void _view_scroll(View *view, int x, int y)
{
    nemoshow_item_translate(view->group, x + 12, y + 12);

    nemoshow_dispatch_frame(view->show);
}
#endif

typedef struct _Context Context;

struct _Context {
    struct nemoshow *show;
    int width, height;
    bool noevent;
    struct nemotool *tool;
    NemoWidget *win;
    View *view;
    double sx, sy;
};

static void _quit_done(struct nemotimer *timer, void *userdata)
{
    Context *ctx = userdata;
    nemotool_exit(ctx->tool);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Context *ctx = userdata;
    if (ctx->noevent) return;

    ctx->noevent = true;
    View *view = ctx->view;
    struct showone *one;
    int i = 0;
    for (i = 0 ; i < view->txts_num ; i++) {
        if (i >= 35) break;
        one = view->txts[i];
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_OUT_TYPE, 500, 10 * (34 - i),
                "alpha", 0.0f,
                "sy", 0.1f,
                NULL);
    }

    _nemoshow_item_motion(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 1000, 400,
            "alpha", 0.0,
            "sx", 0.1,
            "sy", 0.1,
            NULL);

    PATH_CIRCLE_TO_CIRCLE(view->bg, NEMOEASE_CUBIC_OUT_TYPE, 1000, 400);

    struct nemotimer *timer = nemotimer_create(nemowidget_get_tool(win));
    nemotimer_set_timeout(timer, 1000);
    nemotimer_set_userdata(timer, ctx);
    nemotimer_set_callback(timer, _quit_done);
    nemoshow_dispatch_frame(ctx->show);
}

#if 0
static void _tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
    nemotale_event_update_node_taps(tale, node, event, type);
    int id = nemotale_node_get_id(node);
    if (id != 1) return;

    struct nemoshow *show = nemotale_get_userdata(tale);
    struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);
    Context *ctx = nemoshow_get_userdata(show);
    View *view = ctx->view;
    if (ctx->noevent) return;

    if (nemoshow_event_is_down(tale, event, type) ||
            nemoshow_event_is_up(tale, event, type)) {
        if (nemoshow_event_is_single_tap(tale, event, type)) {
            nemocanvas_move(canvas, event->taps[0]->serial);
        } else if (nemotale_is_double_taps(tale, event, type)) {
            nemocanvas_pick(canvas,
                    event->taps[0]->serial,
                    event->taps[1]->serial,
                    (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) |
                    (1 << NEMO_SURFACE_PICK_TYPE_SCALE) |
                    (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
        }
    }

    if (nemotale_is_triple_taps(tale, event, type)) {
        if (nemoshow_event_is_down(tale, event, type)) {
            ctx->sx = NEMOSHOW_ITEM_AT(view->group, tx);
            ctx->sy = NEMOSHOW_ITEM_AT(view->group, ty);
        } else if (nemotale_is_motion_event(tale, event, type)) {
            double x, y;
            x = (event->taps[0]->x - event->taps[0]->grab_x +
                    event->taps[1]->x - event->taps[1]->grab_x +
                    event->taps[2]->x - event->taps[2]->grab_x)/3 +
                ctx->sx;
            y = (event->taps[0]->y - event->taps[0]->grab_y +
                    event->taps[1]->y - event->taps[1]->grab_y +
                    event->taps[2]->y - event->taps[2]->grab_y)/3 +
                ctx->sy;
            if (-(view->width - 1000.0) > x) x = -(view->width - 1000.0);
            if (x > 0) x = 0;
            if (-(view->height - 1000.0) > y) y = -(view->height - 1000.0);
            if (y > 0) y = 0;
            _view_scroll(view, x, y);
        }
        nemocanvas_miss(canvas);
    }
}
#endif


typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *path;
};

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, appname, filename, argc, argv);

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        { NULL }
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "f:", options, NULL))) {
        if (opt == -1) break;
        switch(opt) {
            case 'f':
                app->path = strdup(optarg);
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
    if (app->path) free(app->path);
    free(app);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->path) {
        ERR("Usage: %s -f FILENAME", APPNAME);
        return -1;
    }

    Context *ctx;
    struct nemoshow *show;

    ctx = calloc(sizeof(Context), 1);
    ctx->width = 1024;
    ctx->height = 1024;

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win(tool, APPNAME, app->config->width, app->config->height);
    // FIXME: Scene is desgined for 1024x1024
    nemowidget_win_load_scene(win, 1024, 1024);
    nemowidget_append_callback(win, "exit", _win_exit, ctx);
    ctx->win = win;
    ctx->tool = nemowidget_get_tool(win);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, ctx->width, ctx->height);
    nemowidget_enable_event(vector, false);
    nemowidget_show(vector, 0, 0, 0);

    struct showone *canvas;
    show = nemowidget_get_show(vector);
    canvas = nemowidget_get_canvas(vector);

    ctx->show = show;
    ctx->view = _view_create(show, canvas, app->path);
    nemoshow_dispatch_frame(show);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    free(ctx);

    return 0;
}
