#ifndef __NEMOUI_EVENT_H__
#define __NEMOUI_EVENT_H__

#include <widget.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NemouiGesture NemouiGesture;
typedef void (*GestureMove)(NemouiGesture *gesture, NemoWidget *widget, void *event, int tx, int ty, void *userdata);
typedef void (*GestureThrow)(NemouiGesture *gesture, NemoWidget *widget, void *event, int tx, int ty, void *userdata);
typedef void (*GestureScale)(NemouiGesture *gesture, NemoWidget *widget, void *event, double scale, void *userdata);
typedef void (*GestureRotate)(NemouiGesture *gesture, NemoWidget *widget, void *event, double ro, void *userdata);

NemouiGesture *nemoui_gesture_create(NemoWidget *parent, int width, int height);
void nemoui_gesture_translate(NemouiGesture *gesture, int tx, int ty);
void nemoui_gesture_set_move(NemouiGesture *gesture, GestureMove callback, void *userdata);
void nemoui_gesture_set_throw(NemouiGesture *gesture, GestureThrow callback, void *userdata);
void nemoui_gesture_set_scale(NemouiGesture *gesture, GestureScale callback, void *userdata);
void nemoui_gesture_set_rotate(NemouiGesture *gesture, GestureRotate callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
