#ifndef __NEMOUI_EVENT_H__
#define __NEMOUI_EVENT_H__

#include <widget.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NemouiGesture NemouiGesture;
typedef void (*GestureMove)(NemouiGesture *gesture, NemoWidget *widget, void *event, int tx, int ty, void *userdata);
typedef void (*GestureMoveStart)(NemouiGesture *gesture, NemoWidget *widget, void *event,void *userdata);
typedef void (*GestureMoveStop)(NemouiGesture *gesture, NemoWidget *widget, void *event, void *userdata);
typedef void (*GestureThrow)(NemouiGesture *gesture, NemoWidget *widget, void *event, int tx, int ty, void *userdata);
typedef void (*GestureScale)(NemouiGesture *gesture, NemoWidget *widget, void *event, double scale, void *userdata);
typedef void (*GestureScaleStart)(NemouiGesture *gesture, NemoWidget *widget, void *event,void *userdata);
typedef void (*GestureScaleStop)(NemouiGesture *gesture, NemoWidget *widget, void *event, void *userdata);
typedef void (*GestureRotate)(NemouiGesture *gesture, NemoWidget *widget, void *event, double ro, void *userdata);
typedef void (*GestureRotateStart)(NemouiGesture *gesture, NemoWidget *widget, void *event,void *userdata);
typedef void (*GestureRotateStop)(NemouiGesture *gesture, NemoWidget *widget, void *event, void *userdata);

void nemoui_gesture_destroy(NemouiGesture *gesture);
NemouiGesture *nemoui_gesture_create(NemoWidget *parent, int width, int height);
void nemoui_gesture_show(NemouiGesture *gesture);
void nemoui_gesture_hide(NemouiGesture *gesture);
void nemoui_gesture_translate(NemouiGesture *gesture, int tx, int ty);
void nemoui_gesture_set_max_taps(NemouiGesture *gesture, int tab);
void nemoui_gesture_set_move(NemouiGesture *gesture, GestureMove move, GestureMoveStart start, GestureMoveStop stop, void *userdata);
void nemoui_gesture_set_throw(NemouiGesture *gesture, GestureThrow callback, void *userdata);
void nemoui_gesture_set_scale(NemouiGesture *gesture, GestureScale scale, GestureScaleStart start, GestureScaleStop stop, void *userdata);
void nemoui_gesture_set_rotate(NemouiGesture *gesture, GestureRotate rotate, GestureRotateStart start, GestureRotateStop stop, void *userdata);
void nemoui_gesture_get_center(NemouiGesture *gesture, void *event, double *cx, double *cy);

#ifdef __cplusplus
}
#endif

#endif
