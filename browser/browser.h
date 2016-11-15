#include <nemonavi.h>
#include <nemodavi.h>
#include <nemoshow.h>
#include <nemotool.h>
#include <showone.h>

#define NEMONAVI_MESSAGE_TIMEOUT		(8)
#define WINW			640
#define WINH			480

#define CHROME_H	40
#define BEZEL_W	  5
#define BTN_W			CHROME_H * 0.5
#define BTN_Y			CHROME_H * 0.5 + BEZEL_W

struct appcontext {
	struct nemoshow *show;
	struct nemotool *tool;

	struct showone *scene;
	struct showone *webframe;
	struct showone *weboutline;
	struct showone *webview;
	struct showone *webpopup;
	struct showone *font;

	struct nemonavi *navi;
	struct nemonavi *ol_davi;
	char start_page[512];
	char tmp_page[512];
	int start_remote;
	int on_editurl;
	int on_webview_focus;


	struct nemodavi *davi;
	struct nemodavi_transition *rtrans;

	struct nemotimer *webtimer;

	int winw, winh;
	double ratio;

	// touch clicked coord
	float x, y; // scene coord
	float frame_vx, frame_vy; // real local coord
	float webview_vx, webview_vy; // real local coord

	struct nemohangul *hangul;
};

typedef enum {
	NEMO_BROWSER_KEY_TAB = 15,
	NEMO_BROWSER_KEY_SHIFT1 = 42,
	NEMO_BROWSER_KEY_SHIFT2 = 54,
	NEMO_BROWSER_KEY_CTRL1 = 29,
	NEMO_BROWSER_KEY_CTRL2 = 97,
	NEMO_BROWSER_KEY_ALT1 = 56,
	NEMO_BROWSER_KEY_ALT2 = 100,
	NEMO_BROWSER_KEY_CAPSLOCK = 58,
	NEMO_BROWSER_KEY_BACKSPACE = 14,
	NEMO_BROWSER_KEY_WIN = 125,
	NEMO_BROWSER_KEY_ENTER = 28,
	NEMO_BROWSER_KEY_DEL = 111
} NemoBrowserSpecialKey;

extern void nemonavi_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height);
extern void nemonavi_dispatch_paint(struct nemonavi *navi, int type, const void *buffer, int width, int height, int dx, int dy, int dw, int dh);
extern void nemonavi_dispatch_popup_show(struct nemonavi *navi, int show);
extern void nemonavi_dispatch_popup_rect(struct nemonavi *navi, int x, int y, int width, int height);
extern void nemonavi_dispatch_key_event(struct nemonavi *navi, uint32_t code, int focus_on_editable_field);
extern void nemonavi_dispatch_loading_state(struct nemonavi *navi, int is_loading, int can_go_back, int can_go_forward);
extern int nemonavi_dispatch_touch_grab(struct nemoshow *show, struct showgrab *grab, struct showevent *event);
extern void nemonavi_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event);
extern void nemonavi_dispatch_focus_change(struct nemonavi *navi, int is_editable);
extern void nemonavi_dispatch_timer(struct nemotimer *timer, void *data);
