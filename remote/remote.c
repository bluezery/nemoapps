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

#include "util.h"
#include "widgets.h"
#include "nemoui.h"
#include "nemohelper.h"

#define COLOR0 0xEA562DFF
#define COLOR1 0x35FFFFFF
#define COLORBACK 0x10171E99

#define OBJPATH_PLAYER "/nemoplayer"

typedef struct _RemotePlayer RemotePlayer;
struct _RemotePlayer {
    char *objpath;
    char *screenid;
};

RemotePlayer *remoteplayer_create(const char *objpath, const char *screenid)
{
    RemotePlayer *player = calloc(sizeof(RemotePlayer), 1);
    player->objpath = strdup(objpath);
    player->screenid = strdup(screenid);
    return player;
}


typedef struct _RemoteView RemoteView;
struct _RemoteView
{
    struct nemobus *bus;
    char *objpath;

    struct nemotool *tool;
    struct nemoshow *show;
    List *players;

    int width, height;
    Frameback *back;
    NemoWidget *widget;
    struct showone *group;
    struct showone *top, *bottom;

    bool isplaying;
    char *filename;
    struct showone *text_blur;
    struct showone *txt_group;
    Text *name_back, *name;

    struct showone *ss_group;
    struct showone *ss_back;
    struct showone *ss0, *ss1;
    struct showone *progress_gradient;
    struct showone *progress_circle;

    bool is_mute;
    int vol_cnt;
    struct showone *btn_group;
    Button *fr, *ff, *pf, *nf;
    struct showone *vol;
    Button *vol_mute, *vol_up, *vol_down;
    Button *quit;
};

#if 0
static int _envs_callback(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *userdata)
{
    RET_IF(!src || !dst || !cmd || !path, 1);
    RET_IF(strcmp(dst, nemoenvs_get_name(envs)) && strcmp(dst, "/*"), 1);
    ERR("%s %s %s %s", src, dst, cmd, path);

    RemoteView *view = userdata;
    if (!strcmp(cmd, "get") && !strcmp(path, "/nemoplayer/property")) {
        struct itemattr *attr;
        const char *name;
        const char *value;
        nemoitem_attr_for_each(attr, one) {
            name = nemoitem_attr_get_name(attr);
            value = nemoitem_attr_get_value(attr);
            if (!name || !value) continue;
            if (!strcmp(name, "filename")) {
                if (view->filename) free(view->filename);
                view->filename = strdup(value);
                text_update(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, view->filename);
                text_show(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                text_update(view->name,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, view->filename);
                text_show(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
            } else if (!strcmp(name, "isplaying")) {
                if (!strcmp(value, "true")) {
                    view->isplaying = true;
                    _nemoshow_item_motion(view->ss0,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);
                    _nemoshow_item_motion(view->ss1,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);
                } else {
                    view->isplaying = false;
                    _nemoshow_item_motion(view->ss0,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);
                    _nemoshow_item_motion(view->ss1,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);
                }
            } else if (!strcmp(name, "ismute")) {
                if (!strcmp(value, "true")) {
                    if (!view->is_mute) {
                        view->is_mute = true;
                        button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                0, REMOTE_ICON_DIR"/volume_mute.svg", -1, -1);
                        button_set_fill(view->vol_mute, 0, 0, 0, 0, GRAY);
                        _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                "stroke", GRAY,
                                NULL);
                    }
                } else {
                    if (view->is_mute) {
                        view->is_mute = false;
                        button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                0, REMOTE_ICON_DIR"/volume.svg", -1, -1);
                        button_set_fill(view->vol_mute, 0, 0, 0, 0, COLOR1);
                        _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                "stroke", COLOR1,
                                NULL);
                    }
                }
            } else if (!strcmp(name, "volume")) {
                view->vol_cnt = atoi(value);
                _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
                        "to", (360/100.0) * view->vol_cnt - 90.0,
                        NULL);
            }
        }
    } else if (!strcmp(cmd, "set") && !strcmp(path, "/nemolink/control")) {
        char *src = NULL;
        char *id = NULL;
        char *type = NULL;
        struct itemattr *attr;
        const char *name;
        const char *value;
        nemoitem_attr_for_each(attr, one) {
            name = nemoitem_attr_get_name(attr);
            value = nemoitem_attr_get_value(attr);
            if (!name || !value) continue;
            ERR("%s %s", name, value);
            if (!strcmp(name, "src")) {
                src = strdup(value);
            } else if (!strcmp(name, "id")) {
                id = strdup(value);
            } else if (!strcmp(name, "type")) {
                type = strdup(value);
            }
        }
        ERR("get control %s %s %s", src, id, type);
        if (src && id && type) {
            RemotePlayer *player = remoteplayer_create(src, id, type);
            view->players = list_append(view->players, player);

            char buf[512];
            snprintf(buf, sizeof(buf), "%s %s get /nemoplayer/property",
                    nemoenvs_get_name(view->envs), player->src);
            ERR("%s", buf);
            nemoenvs_send(view->envs, buf);
        }
        if (src) free(src);
        if (id) free(id);
        if (type) free(type);

    }
    return 0;
}
#endif

static void _view_grab_button_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    Button *btn = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);
    RemoteView *view = button_get_userdata(btn);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);
    if (nemoshow_event_is_down(show, event)) {
        button_down(btn);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        button_up(btn);
        nemoshow_dispatch_frame(show);
        if (nemoshow_event_is_single_click(show, event)) {
            uint32_t tag = nemoshow_one_get_tag(btn->event);
            if (tag == 999) {
                ERR("exit");
                struct busmsg *msg;
                msg = nemobus_msg_create();
                nemobus_msg_set_name(msg, "command");
                nemobus_msg_set_attr(msg, "from", view->objpath);
                nemobus_msg_set_attr(msg, "type", "exit");
                nemobus_send(view->bus, view->objpath, OBJPATH_PLAYER, msg);
                nemobus_msg_destroy(msg);

                NemoWidget *win = nemowidget_get_top_widget(widget);
                nemowidget_callback_dispatch(win, "exit", NULL);

            } else {
                struct busmsg *msg;
                msg = nemobus_msg_create();
                nemobus_msg_set_name(msg, "command");
                nemobus_msg_set_attr(msg, "from", view->objpath);
                if (tag == 0) {
                    nemobus_msg_set_attr(msg, "type", "pf");
                } else if (tag == 1) {
                    nemobus_msg_set_attr(msg, "type", "fr");
                } else if (tag == 2) {
                    nemobus_msg_set_attr(msg, "type", "ff");
                } else if (tag == 3) {
                    nemobus_msg_set_attr(msg, "type", "nf");
                } else if (tag == 10) {
                    nemobus_msg_set_attr(msg, "type", "volume-down");
                } else if (tag == 11) {
                    if (view->is_mute) {
                        nemobus_msg_set_attr(msg, "type", "volume-enable");
                    } else {
                        nemobus_msg_set_attr(msg, "type", "volume-disable");
                    }
                } else if (tag == 12) {
                    nemobus_msg_set_attr(msg, "type", "volume-up");
                } else {
                    ERR("incorrect tag!!");
                }

                nemobus_send(view->bus, view->objpath, OBJPATH_PLAYER, msg);
                nemobus_msg_destroy(msg);
            }
        }
    }
}

static void _view_grab_ss_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
    struct showone *one = userdata;
    struct nemoshow *show = nemowidget_get_show(widget);
    RemoteView *view = nemoshow_one_get_userdata(one);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);
    if (nemoshow_event_is_down(show, event)) {
        _nemoshow_item_motion_bounce(view->ss_back,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 0.7, 0.75,
                "sy", 0.7, 0.75,
                NULL);
        _nemoshow_item_motion_bounce(view->progress_circle,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 0.7, 0.75,
                "sy", 0.7, 0.75,
                NULL);
        nemoshow_dispatch_frame(show);
    } else if (nemoshow_event_is_up(show, event)) {
        _nemoshow_item_motion_bounce(view->ss_back,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 50,
                "sx", 1.1, 1.0,
                "sy", 1.1, 1.0,
                NULL);
        _nemoshow_item_motion_bounce(view->progress_circle,
                NEMOEASE_CUBIC_INOUT_TYPE, 150, 0,
                "sx", 1.1, 1.0,
                "sy", 1.1, 1.0,
                NULL);
        nemoshow_dispatch_frame(show);
        if (nemoshow_event_is_single_click(show, event)) {
            struct busmsg *msg;
            msg = nemobus_msg_create();
            nemobus_msg_set_name(msg, "command");
            nemobus_msg_set_attr(msg, "from", view->objpath);
            ERR("%d", view->isplaying);
            if (view->isplaying) {
                nemobus_msg_set_attr(msg, "type", "stop");
                view->isplaying = false;
                _nemoshow_item_motion(view->ss0,
                        NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "alpha", 0.0, NULL);
                _nemoshow_item_motion(view->ss1,
                        NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                        "alpha", 1.0, NULL);
            } else {
                nemobus_msg_set_attr(msg, "type", "play");
            }
            nemobus_send(view->bus, view->objpath, OBJPATH_PLAYER, msg);
            nemobus_msg_destroy(msg);
        }
    }
}

static void _view_event(NemoWidget *widget, const char *id, void *event, void *userdata)
{
    struct nemoshow *show = nemowidget_get_show(widget);

    if (nemoshow_event_is_down(show, event)) {
        struct showone *one;
        double ex, ey;
        nemowidget_transform_from_global(widget,
                nemoshow_event_get_x(event),
                nemoshow_event_get_y(event), &ex, &ey);
        one = nemowidget_pick_one(widget, ex, ey);
        if (one) {
            const char *id = nemoshow_one_get_id(one);
            if (id && !strcmp(id, "button")) {
                Button *btn = nemoshow_one_get_userdata(one);
                nemowidget_create_grab(widget, event,
                        _view_grab_button_event, btn);
            } else if (id && !strcmp(id, "start_stop")) {
                nemowidget_create_grab(widget, event,
                        _view_grab_ss_event, one);
            }
        }
    }
}

static void _bus_callback(void *data, const char *events)
{
    RET_IF(!events);
    if (strchr(events, 'u')) {
        ERR("SIGHUP received");
        return;
    } else if (strchr(events, 'e')) {
        ERR("Error received");
        return;
    }

    RemoteView *view = data;

	struct nemoitem *it;
	struct itemone *one;
    it = nemobus_recv_item(view->bus);
    RET_IF(!it);

	nemoitem_for_each(one, it) {
        const char *from = nemoitem_one_get_attr(one, "from");
        if (!from) {
            ERR("No objpath in the attribute");
            continue;
        }

        const char *path = nemoitem_one_get_path(one);
        const char *name = strrchr(path, '/');
        if (!path) continue;
        if (!name) continue;

        if (!strcmp(name, "/property")) {
            const char *filename = nemoitem_one_get_attr(one, "filename");
            const char *isplaying = nemoitem_one_get_attr(one, "isplaying");
            const char *ismute = nemoitem_one_get_attr(one, "ismute");
            const char *volume = nemoitem_one_get_attr(one, "volume");

            ERR("%s %s %s %s", filename, isplaying, ismute, volume);
            if (filename) {
                if (view->filename) free(view->filename);
                view->filename = strdup(filename);
                text_update(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, view->filename);
                text_show(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
                text_update(view->name,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0, view->filename);
                text_show(view->name_back,
                        NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
            }
            if (isplaying) {
                if (!strcmp(isplaying, "true")) {
                    view->isplaying = true;
                    _nemoshow_item_motion(view->ss0,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);
                    _nemoshow_item_motion(view->ss1,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);
                } else {
                    view->isplaying = false;
                    _nemoshow_item_motion(view->ss0,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 0.0, NULL);
                    _nemoshow_item_motion(view->ss1,
                            NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                            "alpha", 1.0, NULL);
                }
            }
            if (ismute) {
                if (!strcmp(ismute, "true")) {
                    if (!view->is_mute) {
                        view->is_mute = true;
                        button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                0, REMOTE_ICON_DIR"/volume_mute.svg", -1, -1);
                        button_set_fill(view->vol_mute, 0, 0, 0, 0, GRAY);
                        _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                "stroke", GRAY,
                                NULL);
                    }
                } else {
                    if (view->is_mute) {
                        view->is_mute = false;
                        button_change_svg(view->vol_mute, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                0, REMOTE_ICON_DIR"/volume.svg", -1, -1);
                        button_set_fill(view->vol_mute, 0, 0, 0, 0, COLOR1);
                        _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 500, 0,
                                "stroke", COLOR1,
                                NULL);
                    }
                }
            }
            if (volume) {
                view->vol_cnt = atoi(volume);
                _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_INOUT_TYPE, 250, 0,
                        "to", (360/100.0) * view->vol_cnt - 90.0,
                        NULL);
            }
        }
	}
	nemoitem_destroy(it);
}

RemoteView *remoteview_create(NemoWidget *parent, int width, int height)
{
    struct nemotool *tool;
    struct nemoshow *show;
    RemoteView *view = calloc(sizeof(RemoteView), 1);
    view->tool = tool = nemowidget_get_tool(parent);
    view->show = show = nemowidget_get_show(parent);
    view->width = width;
    view->height = height;

    struct nemobus *bus;
    view->bus = bus = nemobus_create();
    if (!bus) {
        ERR("nemo bus create failed");
        free(view);
        return NULL;
    }

    view->objpath = strdup_printf("/%s/%s", APPNAME,
            nemowidget_get_uuid(parent));
    nemobus_connect(bus, NULL);
    nemobus_advertise(bus, "set", view->objpath);
	nemotool_watch_source(tool,
			nemobus_get_socket(bus),
			"reh",
            _bus_callback,
			view);

    struct busmsg *msg;
    msg = nemobus_msg_create();
    nemobus_msg_set_name(msg, "property");
    nemobus_msg_set_attr(msg, "from", view->objpath);
    nemobus_send(view->bus, view->objpath, OBJPATH_PLAYER, msg);
    nemobus_msg_destroy(msg);

    view->back = frameback_create(parent, width, height);

    NemoWidget *widget;
    struct showone *group;
    struct showone *one;
    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _view_event, view);
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    // Start/Stop
    int r = (height - 10)/2;
    int rx = r + 10, ry = height/2;

    view->ss_group = group = GROUP_CREATE(view->group);
    nemoshow_item_set_alpha(group, 0.0);
    nemoshow_item_scale(group, 0.0, 0.0);
    nemoshow_item_translate(group, rx, ry);

    one = CIRCLE_CREATE(group, r);
    nemoshow_item_set_fill_color(one, RGBA(COLORBACK));
    nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
    nemoshow_one_set_id(one, "start_stop");
    nemoshow_one_set_tag(one, 1);
    nemoshow_one_set_userdata(one, view);
    view->ss_back = one;

    one = SVG_PATH_CREATE(group, r/2, r/2, REMOTE_ICON_DIR"/stop.svg");
    nemoshow_item_set_fill_color(one, RGBA(COLOR0));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    view->ss0 = one;

    one = SVG_PATH_CREATE(group, r/2, r/2, REMOTE_ICON_DIR"/play.svg");
    nemoshow_item_set_fill_color(one, RGBA(COLOR0));
    nemoshow_item_set_anchor(one, 0.5, 0.5);
    nemoshow_item_set_alpha(one, 0.0);
    view->ss1 = one;

    double rr = r * 0.7;
    struct showone *progress_gradient;
    view->progress_gradient = progress_gradient =
        LINEAR_GRADIENT_CREATE(0, 0, rr/3, rr/3);
    STOP_CREATE(progress_gradient, COLOR0, 0.0);
    STOP_CREATE(progress_gradient, COLOR1, 1.0);

    one = CIRCLE_CREATE(group, rr);
    nemoshow_item_set_stroke_color(one, RGBA(COLOR1));
    nemoshow_item_set_stroke_width(one, 5);
    nemoshow_item_set_shader(one, progress_gradient);
    view->progress_circle = one;

    // Filename
    int fontsize = 15;
    struct showone *blur;
    view->text_blur = blur = BLUR_CREATE("solid", 1);
    view->txt_group = group = GROUP_CREATE(view->group);
    nemoshow_item_translate(group, (width - rx)/2 + rx, fontsize);

    Text *text;
    view->name_back = text =
        text_create(tool, group, "NanumGothic", "Regular", fontsize);
    text_set_stroke_color(text, 0, 0, 0, COLORBACK, 1);
    text_set_fill_color(text, 0, 0, 0, COLORBACK);
    text_set_filter(text, blur);

    view->name = text = text_create(tool, group, "NanumGothic", "Regular", fontsize);
    text_set_fill_color(text, 0, 0, 0, WHITE);
    text_show(text, 1, 0, 0);

    // FR, FF, PF, NF
    int ww, hh;
    ww = r;
    hh = r;
    int pos_x, pos_y;
    pos_x = r * 2.75 + 10;
    pos_y = r * 1.25;
    view->btn_group = group = GROUP_CREATE(view->group);

    Button *btn;
    btn = button_create(group, "button", 0);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/pf.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, GRAY);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->pf = btn;

    pos_x += r + 10;
    btn = button_create(group, "button", 1);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/fr.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->fr = btn;

    pos_x += r + 10;
    btn = button_create(group, "button", 2);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/ff.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->ff = btn;

    pos_x += r + 10;
    btn = button_create(group, "button", 3);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/nf.svg", ww, ww);
    button_set_fill(btn, 0, 0, 0, 0, GRAY);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->nf = btn;

    // Voume
    pos_x += r + 10;
    btn = button_create(group, "button", 10);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn,
            REMOTE_ICON_DIR"/volume_down.svg", ww * 0.75, hh * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->vol_down = btn;

    pos_x += r + 10;
    int www, hhh;
    www = ww * 1.25;
    hhh = hh * 1.25;
    btn = button_create(group, "button", 11);
    button_enable_bg(btn, www/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/volume.svg", www * 0.75, www * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->vol_mute = btn;

    view->vol_cnt = 0;
    one = ARC_CREATE(group, www - 4, hhh - 4, -90, -90);
    nemoshow_item_set_stroke_color(one, RGBA(COLOR1));
    nemoshow_item_set_stroke_width(one, 4);
    nemoshow_item_translate(one, pos_x, pos_y);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_scale(one, 0.0, 0.0);
    view->vol = one;

    pos_x += r + 10;
    btn = button_create(group, "button", 12);
    button_enable_bg(btn, ww/2, COLORBACK);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/volume_up.svg", ww * 0.75, hh * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->vol_up = btn;

    pos_x += r + 30;
    btn = button_create(group, "button", 999);
    button_enable_bg(btn, ww/2, RED);
    button_add_svg_path(btn, REMOTE_ICON_DIR"/quit.svg", ww * 0.75, hh * 0.75);
    button_set_fill(btn, 0, 0, 0, 0, COLOR1);
    button_translate(btn, 0, 0, 0, pos_x, pos_y);
    button_set_userdata(btn, view);
    view->quit = btn;

    return view;
}

static void remoteview_show(RemoteView *view)
{
    frameback_show(view->back);
    nemowidget_show(view->widget, 0, 0, 0);
    _nemoshow_item_motion(view->ss_group, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "alpha", 1.0,
            NULL);
    _nemoshow_item_motion_bounce(view->ss_group, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0,
            "sx", 1.1, 1.0, "sy", 1.1, 1.0,
            NULL);

    button_show(view->fr, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);
    button_show(view->ff, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150);
    button_show(view->pf, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 2);
    button_show(view->nf, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 3);
    button_show(view->vol_mute, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 4);
    button_show(view->vol_down, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 5);
    button_show(view->vol_up, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 6);
    button_show(view->quit, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 7);
    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_OUT_TYPE, 1000, 150 * 4,
            "alpha", 1.0,
            "sx", 1.0, "sy", 1.0,
            NULL);
    nemoshow_dispatch_frame(view->show);
}

static void remoteview_hide(RemoteView *view)
{
    frameback_hide(view->back);
    nemowidget_hide(view->widget, 0, 0, 0);
    _nemoshow_item_motion(view->ss_group, NEMOEASE_CUBIC_IN_TYPE, 1000, 0,
            "alpha", 0.0,
            "sx", 0.0, "sy", 0.0,
            NULL);
    button_hide(view->fr, NEMOEASE_CUBIC_IN_TYPE, 500, 0);
    button_hide(view->ff, NEMOEASE_CUBIC_IN_TYPE, 500, 50);
    button_hide(view->pf, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 2);
    button_hide(view->nf, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 3);
    button_hide(view->vol_mute, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 4);
    button_hide(view->vol_down, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 5);
    button_hide(view->vol_up, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 6);
    button_hide(view->quit, NEMOEASE_CUBIC_IN_TYPE, 500, 50 * 7);
    _nemoshow_item_motion(view->vol, NEMOEASE_CUBIC_IN_TYPE, 1000, 0,
            "alpha", 0.0,
            "sx", 0.0, "sy", 0.0,
            NULL);
    nemoshow_dispatch_frame(view->show);
}

static void remoteview_destroy(RemoteView *view)
{
    frameback_destroy(view->back);
    free(view);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    RemoteView *view = userdata;

    remoteview_hide(view);

    nemowidget_win_exit_after(win, 1000);
}

int main(int argc, char *argv[])
{
    Config *config;
    config = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, config);
    nemowidget_win_set_layer(win, "overlay");

    RemoteView *view = remoteview_create(win, config->width, config->height);
    nemowidget_append_callback(win, "exit", _win_exit, view);
    remoteview_show(view);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    remoteview_destroy(view);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    config_unload(config);

    return 0;
}
