#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"
#include "sound.h"

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *bgpath;
};

static ConfigApp *_config_load(const char *domain, const char *appname, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, appname, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        char buf[PATH_MAX];
        const char *temp;

        snprintf(buf, PATH_MAX, "%s/background", appname);
        temp = xml_get_value(xml, buf, "path");
        if (!temp) {
            ERR("No background path in %s", appname);
        } else {
            app->bgpath = strdup(temp);
        }

        xml_unload(xml);
    }

    struct option options[] = {
        {"file", required_argument, NULL, 'f'},
        { NULL }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:", options, NULL)) != -1) {
        switch(opt) {
            case 'f':
                if (app->bgpath) free(app->bgpath);
                app->bgpath = strdup(optarg);
            default:
                break;
        }
    }

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->bgpath) free(app->bgpath);
    free(app);
}


typedef struct _Ground Ground;
struct _Ground {
    int width, height;
    struct nemoshow *show;
    struct nemotool *tool;

    NemoWidget *widget;
    struct showone *group;
    struct showone *bg;
};

Ground *ground_create(NemoWidget *parent, int width, int height, ConfigApp *app)
{
    Ground *ground = calloc(sizeof(Ground), 1);
    ground->show = nemowidget_get_show(parent);
    ground->tool = nemowidget_get_tool(parent);
    ground->width = width;
    ground->height = height;

    NemoWidget *widget;
    struct showone *one;
    struct showone *group;

    ground->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);

    ground->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));
    ground->bg = one = IMAGE_CREATE(group, width, height, app->bgpath);

    return ground;
}

void ground_show(Ground *ground)
{
    nemowidget_show(ground->widget, 0, 0, 0);
    nemowidget_set_alpha(ground->widget, 0, 0, 0, 1.0);
    nemoshow_dispatch_frame(ground->show);
}

void ground_hide(Ground *ground)
{
    nemowidget_hide(ground->widget, 0, 0, 0);
    nemowidget_set_alpha(ground->widget, 0, 0, 0, 0.0);
    nemoshow_dispatch_frame(ground->show);
}

int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    if (!app->bgpath) {
        ERR("No background path, exit!");
        return -1;
    }

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_config(tool, APPNAME, app->config);
    nemowidget_win_set_anchor(win, 0, 0);
    nemowidget_win_set_layer(win, "background");
    nemowidget_win_enable_move(win, 0);
    nemowidget_win_enable_rotate(win, 0);
    nemowidget_win_enable_scale(win, 0);

    Ground *ground = ground_create(win, app->config->width, app->config->height, app);
    ground_show(ground);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(app);

    return 0;
}
