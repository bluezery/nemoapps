#ifndef __NEMOUI_TEXT__
#define __NEMOUI_TEXT__

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemowrapper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Text Text;
struct _Text {
    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;
    struct showone *emboss;
    struct showone *font;
    int fontsize;
    struct showone *group;

    int tw, th;
    double ax, ay;
    char *fontfamily;
    char *fontstyle;
    char *str;
    int idx;
    struct showone *one[2];
    struct showone *clip;

    struct {
        struct showone *one;
        uint32_t easetype;
        int duration;
        int delay;
        double r;
    } blur;
    struct nemotimer *motion_timer;
};

Text *text_create(struct nemotool *tool, struct showone *parent, const char *family, const char *style,  int fontsize);
void text_set_filter(Text *text, struct showone *filter);
void text_set_anchor(Text *text, double ax, double ay);
void text_get_size(Text *text, double *tw, double *th);
void text_set_align(Text *text, double ax, double ay);
void text_destroy(Text *text);
double text_get_width(Text *text);
void text_set_blur(Text *text, const char *style, int r);
void _text_motion_blur_timeout(struct nemotimer *timer, void *userdata);
void text_motion_blur(Text *text, uint32_t easetype, int duration, int delay);
void text_set_fill_color(Text *text, uint32_t easetype, int duration, int delay, uint32_t color);
void text_set_stroke_color(Text *text, uint32_t easetype, int duration, int delay, uint32_t color, int stroke_w);
void text_translate(Text *text, uint32_t easetype, int duration, int delay, int x, int y);
void text_translate_with_callback(Text *txt, uint32_t easetype, int duration, int delay, int x, int y, NemoMotion_FrameCallback callback, void *userdata);
void text_get_translate(Text *txt, double *tx, double *ty);
void text_update(Text *text, uint32_t easetype, int duration, int delay, const char *str);
void text_set_alpha(Text *txt, uint32_t easetype, int duration, int delay, double alpha);
void text_set_scale(Text *txt, uint32_t easetype, int duration, int delay, double sx, double sy);
void text_set_clip(Text *txt, struct showone *clip);

#ifdef __cplusplus
}
#endif

#endif
