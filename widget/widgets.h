#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include <widget.h>

#ifdef __cplusplus
extern "C" {
#endif

// ************************************//
// Button Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_BUTTON;
static inline NemoWidget *nemowidget_create_button(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_BUTTON, parent, width, height);
}
void nemowidget_button_set_bg_circle(NemoWidget *button);
void nemowidget_button_set_bg_rect(NemoWidget *button);
void nemowidget_button_bg_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color);
void nemowidget_button_bg_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w);
void nemowidget_button_set_svg(NemoWidget *button, const char *uri);
void nemowidget_button_svg_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color);
void nemowidget_button_svg_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w);
void nemowidget_button_set_text(NemoWidget *button, const char *family, const char *style, int fontsize, const char *str);
void nemowidget_button_text_set_fill(NemoWidget *button, uint32_t easetype, int duration, int delay,  uint32_t color);
void nemowidget_button_text_set_stroke(NemoWidget *button, uint32_t easetype, int duration, int delay, uint32_t color, double stroke_w);

// ************************************//
// Frame Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_FRAME;
static inline NemoWidget *nemowidget_create_frame(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_FRAME, parent, width, height);
}
void nemowidget_frame_set_color(NemoWidget *frame, uint32_t inactive, uint32_t base, uint32_t active, uint32_t blink);

// ************************************//
// Bar Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_BAR;
typedef enum {
    NEMOWIDGET_BAR_DIR_T = 0,
    NEMOWIDGET_BAR_DIR_B,
    NEMOWIDGET_BAR_DIR_R,
    NEMOWIDGET_BAR_DIR_L,
    NEMOWIDGET_BAR_DIR_MAX
} NemoWidget_BarDirection;

static inline NemoWidget *nemowidget_create_bar(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_BAR, parent, width, height);
}
void nemowidget_bar_set_color(NemoWidget *bar, uint32_t color);
void nemowidget_bar_set_color_motion(NemoWidget *bar, int duration, int delay, uint32_t color);
void nemowidget_bar_set_percent(NemoWidget *bar, double percent);
void nemowidget_bar_set_percent_motion(NemoWidget *bar, int duration, int delay, double percent);
void nemowidget_bar_set_direction(NemoWidget *bar, NemoWidget_BarDirection dir);
void nemowidget_bar_set_direction_motion(NemoWidget *bar, int duration, int delay, NemoWidget_BarDirection dir);

// ************************************//
// Dim Widget
// ************************************//
extern NemoWidgetClass NEMOWIDGET_DIM;
static inline NemoWidget *nemowidget_create_dim(NemoWidget *parent, int width, int height)
{
    return nemowidget_create(&NEMOWIDGET_DIM, parent, width, height);
}
void nemowidget_dim_set_bg(NemoWidget *dim, struct showone *bg);

#ifdef __cplusplus
}
#endif

#endif
