#ifndef __NEMOUI_BTN_H__
#define __NEMOUI_BTN_H__

#include <nemoshow.h>
#include <nemoutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ButtonIcon ButtonIcon;
typedef struct _Button Button;
struct _ButtonIcon
{
    int w, h;
    struct showone *one;
    struct showone *old;
};

struct _Button
{
    int w, h;
    struct showone *blur;
    struct showone *group;
    struct showone *event;
    struct showone *bg0;
    struct showone *bg;
    List *icons;
    void *userdata;
};


ButtonIcon *button_get_nth_icon(Button *btn, int idx);
Button *button_create(struct showone *parent, const char *id, uint32_t tag);
void button_hide(Button *btn, uint32_t easetype, int duration, int delay);
void button_show(Button *btn, uint32_t easetype, int duration, int delay);
void button_enable_bg(Button *btn, int r, uint32_t color);
void button_add_path(Button *btn);
void button_add_circle(Button *btn, int r);
void button_add_svg(Button *btn, const char *svgfile, int w, int h);
void button_add_svg_path(Button *btn, const char *svgfile, int w, int h);
void button_change_svg(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h);
void button_change_svg_path(Button *btn, uint32_t easetype, int duration, int delay, int idx, const char *svgfile, int w, int h);
void button_bg_set_color(Button *btn, uint32_t easetype, int duration, int delay, uint32_t color);
void button_set_stroke(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t stroke, int stroke_width);
void button_set_fill(Button *btn, uint32_t easetype, int duration, int delay, int idx, uint32_t fill);
void button_set_userdata(Button *btn, void *userdata);
void *button_get_userdata(Button *btn);
void button_destroy(Button *btn);
void button_translate(Button *btn, uint32_t easetype, int duration, int delay, double tx, double ty);
void button_down(Button *btn);
void button_up(Button *btn);
#ifdef __cplusplus
}
#endif

#endif
