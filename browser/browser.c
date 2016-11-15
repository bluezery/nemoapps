#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>
#include <nemonavi.h>

#include "nemodavi.h"
#include "util-config.h"

#include "browser.h"

const static char *g_http_scheme = "http://";
const static char *g_go_text = "Go";
const static char *g_cancel_text = "Cancel";

static int dispatch_tap_grab(struct nemoshow *show, struct showgrab *grab, struct showevent *event)
{
	struct appcontext *ctx;

	ctx	= (struct appcontext *)nemoshow_grab_get_userdata(grab);

	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_set_keyboard_focus(show, ctx->webframe);
		nemoshow_event_update_taps(show, ctx->webframe, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
	}

	if (nemoshow_event_is_single_click(show, event)) {
		uint32_t tag = nemoshow_canvas_pick_tag(ctx->webframe,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event));

		printf("clicked tag: %d\n", tag);

		ctx->x = nemoshow_event_get_x(event);
		ctx->y = nemoshow_event_get_y(event);

		nemoshow_transform_to_viewport(show, ctx->x, ctx->y, &ctx->frame_vx, &ctx->frame_vy);
		nemodavi_emit_event(NEMODAVI_EVENT_CLICK, tag);
	}

	nemoshow_dispatch_frame(show);

	if (nemoshow_event_is_up(show, event)) {
		printf("grab is destroyed\n");
		nemoshow_grab_destroy(grab);
		return 0;
	}

	return 1;
}

static int is_special_key(int keycode) {
	switch(keycode) {
		case NEMO_BROWSER_KEY_TAB:
		case NEMO_BROWSER_KEY_SHIFT1:
		case NEMO_BROWSER_KEY_SHIFT2:
		case NEMO_BROWSER_KEY_CTRL1:
		case NEMO_BROWSER_KEY_CTRL2:
		case NEMO_BROWSER_KEY_ALT1:
		case NEMO_BROWSER_KEY_ALT2:
		case NEMO_BROWSER_KEY_CAPSLOCK:
		case NEMO_BROWSER_KEY_WIN:
			return 1;
		default:
			return 0;
	}
}

static void stop_urlbar_ring(struct appcontext *ctx)
{
	struct nemodavi_selector *sel;

	if (ctx->rtrans) {
		sel = nemodavi_selector_get(ctx->davi, "urlbarring");
		nemodavi_set_dattr(sel, "alpha", 0.0);
		nemodavi_transition_destroy(ctx->rtrans);
		ctx->rtrans = NULL;
	}
}

static void start_urlbar_ring(struct appcontext *ctx)
{
	struct nemodavi_selector *sel;
	struct nemodavi_transition *trans;

	if (!ctx) {
		return;
	}

	stop_urlbar_ring(ctx);

	trans = ctx->rtrans = nemodavi_transition_create(ctx->davi);
	sel = nemodavi_selector_get(ctx->davi, "urlbarring");
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.3, 0.5);
	nemodavi_transition_set_dattr(trans, sel, "alpha", 0.0, 1.0);
	nemodavi_transition_set_delay(trans, 0);
	nemodavi_transition_set_repeat(trans, 0);
	nemodavi_transition_set_duration(trans, 3000);
	nemodavi_transition_set_ease(trans, NEMOEASE_LINEAR_TYPE);

	nemodavi_transition_run(ctx->davi);
}

static void reset_url_stuff(struct appcontext *ctx)
{
	struct nemodavi_selector *sel;

	if (!ctx) {
		return;
	}

	ctx->on_editurl = 0;

	memset(ctx->tmp_page, 0x0, sizeof(ctx->tmp_page));
	strncpy(ctx->tmp_page, g_http_scheme, strlen(g_http_scheme));

	sel = nemodavi_selector_get(ctx->davi, "url");
	nemodavi_set_sattr(sel, "text", ctx->start_page);
	nemodavi_set_cattr(sel, "fill", 0xFF444444);

	sel = nemodavi_selector_get(ctx->davi, "gobtn");
	nemodavi_set_cattr(sel, "fill", 0xFFEEEEEE);

	stop_urlbar_ring(ctx);

	nemoshow_dispatch_frame(ctx->show);
}

static void load_bar_url(struct appcontext *ctx)
{
	struct nemodavi_selector *sel;

	if (!ctx) {
		return;
	}

	// in case of enter, we need to load new page
	memset(ctx->start_page, 0x0, sizeof(ctx->start_page));
	strncpy(ctx->start_page, ctx->tmp_page, sizeof(ctx->tmp_page));

	//memset(ctx->tmp_page, 0x0, strlen(ctx->tmp_page));
	reset_url_stuff(ctx);

	// TODO new url should be checked if it is remote url or file
	printf("load url: %s\n", ctx->start_page);
	nemonavi_load_url(ctx->navi, ctx->start_page);
}

static void dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	int need_del = 0;
	double attr;
	uint32_t len, keycode, serial0, serial1;
	struct nemodavi_selector *sel;
	struct appcontext *ctx = (struct appcontext *) nemoshow_get_userdata(show);

	/*
	if (nemoshow_event_is_keyboard_enter(show, event) && !nemoshow_event_is_keyboard_down(show, event)) {
		printf("%s\n", __FUNCTION__);
		sel = nemodavi_selector_get(ctx->davi, "url");
		nemodavi_set_cattr(sel, "fill", 0xFFFF0000);
		return;
	}
	*/

	if (nemoshow_event_is_keyboard_down(show, event)) {

		if (!ctx->on_editurl) {
			// run url bar ring
			start_urlbar_ring(ctx);
		}
		ctx->on_editurl = 1;

		sel = nemodavi_selector_get(ctx->davi, "url");
		nemodavi_set_cattr(sel, "fill", 0xFFF55C4E);

		keycode = nemoshow_event_get_value(event);
		printf("keycode: %d\n", keycode);
		if (is_special_key(keycode)) {
			// special key should be used in url bar
			return;
		}

		if (keycode == NEMO_BROWSER_KEY_ENTER) {
			load_bar_url(ctx);
			return;
		}

		len = strlen(ctx->tmp_page);
		printf("current len: %d\n", len);

		if (keycode == NEMO_BROWSER_KEY_BACKSPACE || keycode == NEMO_BROWSER_KEY_DEL) {
			need_del = 1;
		}

		if (need_del && !len) {
			return;
		}

		if (need_del) {
			ctx->tmp_page[len - 1] = '\0';
		} else {
			ctx->tmp_page[len] = nemotool_get_keysym(ctx->tool, keycode);
			ctx->tmp_page[len + 1] = '\0';
		}
		printf("typyed url: %s (%d)\n", ctx->tmp_page, keycode);

		sel = nemodavi_selector_get(ctx->davi, "url");
		nemodavi_set_sattr(sel, "text", ctx->tmp_page);

		sel = nemodavi_selector_get(ctx->davi, "gobtn");
		nemodavi_set_cattr(sel, "fill", 0xFF444444);

		nemoshow_dispatch_frame(ctx->show);

		return;
	}

	if (nemoshow_event_is_down(show, event)) {
		struct showgrab *grab;
		grab = nemoshow_grab_create(show, event, dispatch_tap_grab);
		nemoshow_grab_set_userdata(grab, ctx);
		nemoshow_dispatch_grab(show, event);
	}
}

static struct showone* create_font(char *font_family, char *font_style)
{
	struct showone *font;

	if (!font_family) {
		return NULL;
	}

	font = nemoshow_font_create();
	nemoshow_font_load_fontconfig(font, font_family, font_style);
	nemoshow_font_use_harfbuzz(font);

	return font;
}

struct showone* backbtn_selector_append(int index, void *datum, void *userdata)
{
	struct showone *one;
	struct nemodavi_selector *sel;
	struct nemodavi *davi = (struct nemodavi *) userdata;

	one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);
	nemoshow_item_set_width(one, BTN_W);
	nemoshow_item_set_height(one, BTN_W);
	nemoshow_item_path_load_svg(one, IMG_DIR"/back.svg", 0, 0, BTN_W, BTN_W);

	return one;
}

struct showone* fwdbtn_selector_append(int index, void *datum, void *userdata)
{
	struct showone *one;
	struct nemodavi_selector *sel;
	struct nemodavi *davi = (struct nemodavi *) userdata;

	one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);
	nemoshow_item_set_width(one, BTN_W);
	nemoshow_item_set_height(one, BTN_W);
	nemoshow_item_path_load_svg(one, IMG_DIR"/forward.svg", 0, 0, BTN_W, BTN_W);

	return one;
}

struct showone* refreshbtn_selector_append(int index, void *datum, void *userdata)
{
	struct showone *one;
	struct nemodavi_selector *sel;
	struct nemodavi *davi = (struct nemodavi *) userdata;

	one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);
	nemoshow_item_set_width(one, BTN_W);
	nemoshow_item_set_height(one, BTN_W);
	nemoshow_item_path_load_svg(one, IMG_DIR"/refresh.svg", 0, 0, BTN_W, BTN_W);

	return one;
}

struct showone* exitbtn_selector_append(int index, void *datum, void *userdata)
{
	struct showone *one;
	struct nemodavi_selector *sel;
	struct nemodavi *davi = (struct nemodavi *) userdata;

	one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);
	nemoshow_item_set_width(one, BTN_W);
	nemoshow_item_set_height(one, BTN_W);
	nemoshow_item_path_load_svg(one, IMG_DIR"/exit.svg", 0, 0, BTN_W, BTN_W);

	return one;
}

struct showone* outline_selector_append(int index, void *datum, void *userdata)
{
	double width, height;
	struct showone *one;
	struct nemodavi_selector *sel;
	struct appcontext *ctx = (struct appcontext *) userdata;

	width = nemoshow_canvas_get_width(ctx->weboutline);
	height = nemoshow_canvas_get_height(ctx->weboutline);

	one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_item_path_use(one, NEMOSHOW_ITEM_BOTH_PATH);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_path_load_svg(one, IMG_DIR"/outline.svg", 0, 0, width, height);

	return one;
}

void backbtn_click(int index, void *datum, void *userdata)
{
	printf("%s\n", __FUNCTION__);

	struct appcontext *ctx = (struct appcontext *) userdata;
	nemonavi_go_back(ctx->navi);
}

void fwdbtn_click(int index, void *datum, void *userdata)
{
	printf("%s\n", __FUNCTION__);
	struct appcontext *ctx = (struct appcontext *) userdata;
	nemonavi_go_forward(ctx->navi);
}

void refreshbtn_click(int index, void *datum, void *userdata)
{
	struct nemodavi_selector *sel;

	printf("%s\n", __FUNCTION__);
	struct appcontext *ctx = (struct appcontext *) userdata;

	nemonavi_reload(ctx->navi);
	reset_url_stuff(ctx);
}

void urlbar_click(int index, void *datum, void *userdata)
{
	struct nemodavi_selector *sel;
	struct appcontext *ctx = (struct appcontext *) userdata;
	printf("%s: frame_vx: %f, frame_vy: %f\n", __FUNCTION__, ctx->frame_vx, ctx->frame_vy);
}

void gobtn_click(int index, void *datum, void *userdata)
{
	struct nemodavi_selector *sel;
	struct appcontext *ctx = (struct appcontext *) userdata;

	printf("%s\n", __FUNCTION__);

	if (ctx->on_editurl) {
		load_bar_url(ctx);
	}
}

void exitbtn_click(int index, void *datum, void *userdata)
{
	struct nemodavi_selector *sel;

	printf("%s\n", __FUNCTION__);
	struct appcontext *ctx = (struct appcontext *) userdata;

	nemotool_exit(ctx->tool);
}

static struct showone* urlbarring_set_filter(int index, void *datum, void *userdata)
{
	struct showone *one;

	one = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(one, "solid", 2);

	return one;
}


static int init_webframe(struct appcontext *ctx)
{
	struct nemodavi *davi;
	struct nemodavi_selector *sel;

	if (!ctx) {
		return -1;
	}


	davi = ctx->ol_davi = nemodavi_create(ctx->weboutline, NULL);

	nemodavi_append_selector_by_handler(davi, "outline", outline_selector_append, ctx);
	sel = nemodavi_selector_get(davi, "outline");
	nemodavi_set_dattr(sel, "tx", 0);
	nemodavi_set_dattr(sel, "ty", 0);
	nemodavi_set_cattr(sel, "fill", 0xFFFFFFFF);

	davi = ctx->davi = nemodavi_create(ctx->webframe, NULL);

	nemodavi_append_selector(davi, "background", NEMOSHOW_RRECT_ITEM);
	sel = nemodavi_selector_get(davi, "background");
	nemodavi_set_dattr(sel, "tx", 0);
	nemodavi_set_dattr(sel, "ty", 0);
	nemodavi_set_dattr(sel, "ox", BEZEL_W * 4.2);
	nemodavi_set_dattr(sel, "oy", BEZEL_W * 4.2);
	nemodavi_set_dattr(sel, "width", ctx->winw);
	nemodavi_set_dattr(sel, "height", ctx->winh);
	nemodavi_set_dattr(sel, "alpha", 1.0f);
	nemodavi_set_cattr(sel, "fill", 0xFFFFFFFF);

	nemodavi_append_selector(davi, "header", NEMOSHOW_RECT_ITEM);
	sel = nemodavi_selector_get(davi, "header");
	nemodavi_set_dattr(sel, "tx", 0);
	nemodavi_set_dattr(sel, "ty", 0);
	nemodavi_set_dattr(sel, "width", ctx->winw - BEZEL_W * 2);
	nemodavi_set_dattr(sel, "height", CHROME_H);
	nemodavi_set_dattr(sel, "tx", BEZEL_W);
	nemodavi_set_dattr(sel, "ty", BEZEL_W);
	nemodavi_set_dattr(sel, "alpha", 0.0f);
	nemodavi_set_cattr(sel, "fill", 0xFFFF0000);

	nemodavi_append_selector_by_handler(davi, "backbtn", backbtn_selector_append, davi);
	sel = nemodavi_selector_get(davi, "backbtn");
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.0195);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xFFDDDDDD);
	nemodavi_set_cattr(sel, "stroke", 0xFFFFFFFF);
	nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, backbtn_click, ctx);

	nemodavi_append_selector_by_handler(davi, "fwdbtn", fwdbtn_selector_append, ctx);
	sel = nemodavi_selector_get(davi, "fwdbtn");
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.0781);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xFFDDDDDD);
	nemodavi_set_cattr(sel, "stroke", 0xFFFFFFFF);
	nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, fwdbtn_click, ctx);

	nemodavi_append_selector_by_handler(davi, "refreshbtn", refreshbtn_selector_append, ctx);
	sel = nemodavi_selector_get(davi, "refreshbtn");
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.15625);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xFF444444);
	nemodavi_set_cattr(sel, "stroke", 0xFFFFFFFF);
	nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, refreshbtn_click, ctx);

	nemodavi_append_selector(davi, "urlbar", NEMOSHOW_RRECT_ITEM);
	sel = nemodavi_selector_get(davi, "urlbar");
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.525);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_dattr(sel, "ox", BEZEL_W);
	nemodavi_set_dattr(sel, "oy", BEZEL_W);
	nemodavi_set_dattr(sel, "width", ctx->winw * 0.7);
	nemodavi_set_dattr(sel, "height", CHROME_H * 0.8);
	nemodavi_set_dattr(sel, "alpha", 1.0f);
	nemodavi_set_cattr(sel, "fill", 0xFFCCCCCC);
	//nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, urlbar_click, ctx);

	nemodavi_append_selector(davi, "urlbarring", NEMOSHOW_RRECT_ITEM);
	sel = nemodavi_selector_get(davi, "urlbarring");
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.525);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_dattr(sel, "ox", BEZEL_W);
	nemodavi_set_dattr(sel, "oy", BEZEL_W);
	nemodavi_set_dattr(sel, "width", ctx->winw * 0.7);
	nemodavi_set_dattr(sel, "height", CHROME_H * 0.8);
	nemodavi_set_dattr(sel, "alpha", 0.0f);
	nemodavi_set_dattr(sel, "stroke-width", 2);
	nemodavi_set_cattr(sel, "stroke", 0xFFFF0000);
	nemodavi_set_cattr(sel, "fill", 0x00000000);
	nemodavi_set_oattr_handler(sel, "filter", urlbarring_set_filter, NULL);
	//nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, urlbar_click, ctx);

	nemodavi_append_selector(davi, "url", NEMOSHOW_TEXTBOX_ITEM);
	sel = nemodavi_selector_get(davi, "url");
	nemodavi_set_oattr(sel, "font", ctx->font);
	nemodavi_set_dattr(sel, "font-size", 15);
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.1796);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.0);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_dattr(sel, "spacingmul", 10);
	nemodavi_set_dattr(sel, "spacingadd", 10);
	nemodavi_set_dattr(sel, "width", ctx->winw * 0.7);
	nemodavi_set_dattr(sel, "height", CHROME_H * 0.5);
	nemodavi_set_dattr(sel, "alpha", 1.0f);
	nemodavi_set_cattr(sel, "fill", 0xFF444444);
	nemodavi_set_sattr(sel, "text", ctx->start_page);
	//nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, urlbar_click, ctx);

	nemodavi_append_selector(davi, "gobtn", NEMOSHOW_TEXT_ITEM);
	sel = nemodavi_selector_get(davi, "gobtn");
	nemodavi_set_oattr(sel, "font", ctx->font);
	nemodavi_set_dattr(sel, "font-size", 15);
	nemodavi_set_dattr(sel, "tx", ctx->winw * 0.89);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xFFEEEEEE);
	nemodavi_set_sattr(sel, "text", g_go_text);
	nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, gobtn_click, ctx);

	nemodavi_append_selector_by_handler(davi, "exitbtn", exitbtn_selector_append, davi);
	sel = nemodavi_selector_get(davi, "exitbtn");
	nemodavi_set_dattr(sel, "tx", ctx->winw - ctx->winw * 0.0195);
	nemodavi_set_dattr(sel, "ty", BTN_Y);
	nemodavi_set_dattr(sel, "ax", 0.5);
	nemodavi_set_dattr(sel, "ay", 0.5);
	nemodavi_set_cattr(sel, "fill", 0xFF444444);
	nemodavi_set_cattr(sel, "stroke", 0xFFFFFFFF);
	nemodavi_selector_attach_event_listener(sel, NEMODAVI_EVENT_CLICK, exitbtn_click, ctx);

	return 0;
}

static void init_webview(struct appcontext* ctx)
{
	int width, height;
	struct nemonavi *navi;
	struct nemotimer *timer;

	if (!ctx) {
		return;
	}

	width = nemoshow_canvas_get_width(ctx->webview);
	height = nemoshow_canvas_get_height(ctx->webview);

	ctx->navi = navi = nemonavi_create("http://www.google.com");
	nemonavi_set_size(navi, width, height);
	nemonavi_set_dispatch_paint(navi, nemonavi_dispatch_paint);
	nemonavi_set_dispatch_popup_show(navi, nemonavi_dispatch_popup_show);
	nemonavi_set_dispatch_popup_rect(navi, nemonavi_dispatch_popup_rect);
	nemonavi_set_dispatch_key_event(navi, nemonavi_dispatch_key_event);
	//nemonavi_set_dispatch_focus_change(navi, nemonavi_dispatch_focus_change);
	nemonavi_set_dispatch_loading_state(navi, nemonavi_dispatch_loading_state);
	nemonavi_set_userdata(navi, ctx);

	if (!ctx->start_page) {
		return;
	}

	if (ctx->start_remote) {
		nemonavi_load_url(navi, ctx->start_page);
	} else {
		nemonavi_load_page(navi, ctx->start_page);
	}

	ctx->webtimer = timer = nemotimer_create(ctx->tool);
	nemotimer_set_callback(timer, nemonavi_dispatch_timer);
	nemotimer_set_userdata(timer, ctx);
	nemotimer_set_timeout(timer, NEMONAVI_MESSAGE_TIMEOUT);
}

static bool config_load_get_wh(const char *project_name, const char *appname, const char *config_file, int *w, int *h)
{
    Xml *xml;
    xml = xml_load_from_domain(project_name, config_file);
    if (!xml) return false;

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%s/size", appname);

    const char *temp;
    temp = xml_get_value(xml, buf, "width");
    if (!temp) {
        ERR("No width in configuration file");
    } else {
        if (w) *w = atoi(temp);
    }

    temp = xml_get_value(xml, buf, "height");
    if (!temp) {
        ERR("No height in configuration file");
    } else {
        if (h) *h = atoi(temp);
    }

    xml_unload(xml);
    return true;
}

static struct appcontext* init_app(int argc, char *argv[]) {
	int winw, winh;
	struct appcontext *ctx;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;

	ctx = (struct appcontext *)malloc(sizeof(struct appcontext));
	memset(ctx, 0, sizeof(struct appcontext));

	config_load_get_wh(PROJECT_NAME, APPNAME, CONFXML, &winw, &winh);
	printf("load winw: %d, winh: %d\n", winw, winh);

	ctx->winw = winw;
	ctx->winh = winh;
	ctx->ratio = (double) winw / (double) winw;

	printf("init ratio: %f\n", ctx->ratio);

	tool = ctx->tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);

	show = ctx->show = nemoshow_create_view(tool, winw, winh);
	nemoshow_set_dispatch_resize(show, nemonavi_dispatch_show_resize);

	nemoshow_view_set_layer(show, "service");
	nemoshow_view_set_state(show, "keypad");
	nemoshow_view_set_state(show, "sound");
	nemoshow_view_set_state(show, "layer");

	ctx->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, winw);
	nemoshow_scene_set_height(scene, winh);
	nemoshow_set_scene(show, scene);

	canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, winw);
	nemoshow_canvas_set_height(canvas, winh);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0xFF, 0xFF, 0xFF, 0xFF);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_one_attach(scene, canvas);

	ctx->webframe = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, winw);
	nemoshow_canvas_set_height(canvas, winh);
	//nemoshow_canvas_set_fill_color(canvas, 0x00, 0x00, 0x00, 0xff);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	ctx->webview = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, winw - (BEZEL_W) * 2 - 4);
	nemoshow_canvas_set_height(canvas, winh - (CHROME_H + BEZEL_W * 2) - 4);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);
	nemoshow_canvas_translate(canvas, BEZEL_W + 2, (CHROME_H + BEZEL_W) + 2);

	// TODO add popup?
	ctx->webpopup = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, winw - (BEZEL_W) * 2 - 4);
	nemoshow_canvas_set_height(canvas, winh - (CHROME_H + BEZEL_W * 2) - 4);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);

	ctx->weboutline = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, winw - (BEZEL_W * 2) - 2);
	nemoshow_canvas_set_height(canvas, winh - (CHROME_H + BEZEL_W * 2) - 2);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);
	nemoshow_canvas_translate(canvas, (BEZEL_W + 1), (CHROME_H + BEZEL_W) + 1);

	ctx->font = create_font("NanumGothic", "Bold");
	strncpy(ctx->tmp_page, g_http_scheme, strlen(g_http_scheme));

	nemoshow_set_userdata(show, ctx);

	return ctx;
}

static void deinit_app(struct appcontext* ctx) {
	nemoshow_destroy_view(ctx->show);
	nemotool_disconnect_wayland(ctx->tool);
	nemotool_destroy(ctx->tool);
	nemonavi_destroy(ctx->navi);

	free(ctx);

	nemonavi_exit_once();
}

int main (int argc, char *argv[])
{
	int opt, is_remote = 0;
	char *path = NULL;

	struct appcontext *ctx;
	struct option options[] = {
		{ "url", required_argument, NULL, 'u' },
		{ "file", required_argument, NULL, 'f' },
		{ 0 }
	};

	// TODO we need to check reg expression for url and file path
	while (opt = getopt_long(argc, argv, "u:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'u':
				path = strdup(optarg);
				is_remote = 1;
				break;

			case 'f':
				path = strdup(optarg);
				is_remote = 0;
				break;

			default:
				break;
		}
	}
	nemonavi_init_once(argc, argv);

	ctx = init_app(argc, argv);
	strncpy(ctx->start_page, path, strlen(path));
	ctx->start_remote = is_remote;

	init_webframe(ctx);
	init_webview(ctx);

	nemoshow_dispatch_frame(ctx->show);
	nemotool_run(ctx->tool);

	deinit_app(ctx);
}
