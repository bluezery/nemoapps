#ifndef __NEMOUI_DRAWER_H__
#define __NEMOUI_DRAWER_H__

#include <nemoshow.h>
#include <nemoui-button.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DrawingBox DrawingBox;
struct _DrawingBox {
    int mode; // 0 (pencil) , 1 (painter)
    int r;
    struct showone *group;

    double size;
    unsigned int size_idx;
    uint32_t color;
    unsigned int color_idx;

    Button *pencil_btn;

    Button *quit_btn;
    //Button *share_btn;
    Button *undo_btn;
    Button *undo_all_btn;

    Button *stroke_btn;
    Button *color_btn;
};

DrawingBox *drawingbox_create(struct showone *parent, int r);
void drawingbox_destroy(DrawingBox *dbox);
void drawingbox_change_color(DrawingBox *dbox, uint32_t easetype, int duration, int delay);
void drawingbox_change_stroke(DrawingBox *dbox, uint32_t easetype, int duration, int delay);
void drawingbox_show_pencil(DrawingBox *dbox);
void drawingbox_show_menu(DrawingBox *dbox);
void drawingbox_show(DrawingBox *dbox);
void drawingbox_hide(DrawingBox *dbox);
void drawingbox_get_translate(DrawingBox *dbox, double *tx, double *ty);
void drawingbox_translate(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double tx, double ty);
void drawingbox_scale(DrawingBox *dbox, uint32_t easetype, int duration, int delay, double sx, double sy);


#ifdef __cplusplus
}
#endif

#endif
