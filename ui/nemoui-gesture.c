#include <nemoutil.h>
#include "nemoui-gesture.h"

#define SCALE_COEFF 0.01
#define HISTORY_MAX 50
#define HISTORY_MIN_DIST 10

#define HISTORY_MIN_CNT 5
#define THROW_COEFF 200
#define THROW_MIN_DIST 100

typedef struct _Grab Grab;
struct _Grab {
    uint64_t device;
    int idx;
};

typedef struct _Coord Coord;
struct _Coord
{
    uint32_t time;
    double x, y;
};

struct _NemouiGesture
{
    NemoWidget *widget;

    List *grabs;

    Grab *grab0, *grab1;

    double ro;

    double scale_sum;
    double scale_x;
    double scale_y;

    int history_idx;
    Coord histories[HISTORY_MAX];

    GestureMove move;
    GestureMoveStart move_start;
    GestureMoveStop move_stop;
    void *move_userdata;
    GestureScale scale;
    GestureScaleStart scale_start;
    GestureScaleStop scale_stop;
    void *scale_userdata;
    GestureRotate rotate;
    GestureRotateStart rotate_start;
    GestureRotateStop rotate_stop;
    void *rotate_userdata;
    GestureThrow throw;
    void *throw_userdata;
};

static void gesture_update_index(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    // Update index
    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                break;
            }
        }
        if (i >= nemoshow_event_get_tapcount(event)) {
            ERR("grab is lost!!!!!");
            continue;
        }
        g->idx = i;
    }
}

void nemoui_gesture_get_center(NemouiGesture *gesture, void *event, double *cx, double *cy)
{
    RET_IF(!gesture);
    int cnt = list_count(gesture->grabs);
    RET_IF(cnt <= 0);

    double sumx = 0, sumy = 0;
    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        int i = 0;
        for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
            if (g->device == nemoshow_event_get_device_on(event, i)) {
                sumx += nemoshow_event_get_x_on(event, i);
                sumy += nemoshow_event_get_y_on(event, i);
                break;
            }
        }
    }
    if (cx) *cx = sumx/cnt;
    if (cy) *cy = sumy/cnt;
}

static void gesture_rotate_init(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    if (list_count(gesture->grabs) < 2) return;

    // Find far distance
    double x0, x1, y0, y1;
    double dist = 0;
    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        List *ll;
        Grab *gg;
        LIST_FOR_EACH(gesture->grabs, ll, gg) {
            double _x0, _y0, _x1, _y1;
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, g->idx),
                    nemoshow_event_get_y_on(event, g->idx), &_x0, &_y0);
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, gg->idx),
                    nemoshow_event_get_y_on(event, gg->idx), &_x1, &_y1);
            double _dist = DISTANCE(_x0, _y0, _x1, _y1);
            if (dist < _dist) {
                dist = _dist;
                x0 = _x0;
                y0 = _y0;
                x1 = _x1;
                y1 = _y1;
                gesture->grab0 = g;
                gesture->grab1 = gg;
            }
        }
    }

    //gesture->ro = nemoshow_item_get_rotate(gesture->group) + atan2f(x0 - x1, y0 - y1) * 180/M_PI;
    gesture->ro = atan2f(x0 - x1, y0 - y1) * (180/M_PI);
}

static bool gesture_rotate_calculate(NemouiGesture *gesture, NemoWidget *widget, void *event, double *ro)
{
    if (list_count(gesture->grabs) < 2) return false;

    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int i;
    for (i = 0 ; i < nemoshow_event_get_tapcount(event) ; i++) {
        if (gesture->grab0->device == nemoshow_event_get_device_on(event, i)) {
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, i),
                    nemoshow_event_get_y_on(event, i), &x0, &y0);
            continue;
        } else if (gesture->grab1->device == nemoshow_event_get_device_on(event, i)) {
            nemowidget_transform_from_global(widget,
                    nemoshow_event_get_x_on(event, i),
                    nemoshow_event_get_y_on(event, i), &x1, &y1);
            continue;
        }
    }
    double r1 = atan2f(x0 - x1, y0 - y1) * (180/M_PI);
    if (ro) *ro = gesture->ro - r1;
    return true;
}

static void gesture_scale_init(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    if (list_count(gesture->grabs) < 2) return;
    //nemoshow_item_set_anchor(gesture->group, cx, cy);

    double cx, cy;
    nemoui_gesture_get_center(gesture, event, &cx, &cy);

    int k = 0;
    double sum = 0;

    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        double xx, yy;
        xx = nemoshow_event_get_x_on(event, g->idx);
        yy= nemoshow_event_get_y_on(event, g->idx);
        sum += DISTANCE(cx, cy, xx, yy);
        k++;
    }

    gesture->scale_sum = sum/k * SCALE_COEFF;
    //gesture->scale_x = nemoshow_item_get_scale_x(gesture->group);
    //gesture->scale_y = nemoshow_item_get_scale_y(gesture->group);
}

bool gesture_scale_calculate(NemouiGesture *gesture, NemoWidget *widget, void *event, double *scale)
{
    if (list_count(gesture->grabs) < 2) return false;

    double cx, cy;
    nemoui_gesture_get_center(gesture, event, &cx, &cy);

    int k = 0;
    double sum = 0;

    // Sum scale
    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        double xx, yy;
        xx = nemoshow_event_get_x_on(event, g->idx);
        yy= nemoshow_event_get_y_on(event, g->idx);
        sum += DISTANCE(cx, cy, xx, yy);
        k++;
    }

    double _scale = sum/k * SCALE_COEFF;
    if (scale) *scale = -(_scale - gesture->scale_sum);
    //double scale_x = gesture->scale_x + (double)(scale - gesture->scale_sum) * coeff;
    //double scale_y = gesture->scale_y + (double)(scale - gesture->scale_sum) * coeff;
    /*
    if (scale_x <= 5.0 && scale_x <= 5.0 && scale_y >= 0.5 && scale_y >= 0.5) {
        gesture_scale(gesture, 0, 0, 0, scale_x, scaley);
    }
    */
    return true;
}

static void gesture_move_init(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    // diff update
    double cx, cy;
    nemoui_gesture_get_center(gesture, event, &cx, &cy);

    /*
    double tx, ty;
    tx = nemoshow_item_get_translate_x(gesture->group);
    ty = nemoshow_item_get_translate_y(gesture->group);
    */


    // history init
    int i;
    for (i = 0 ; i < HISTORY_MAX ; i++) {
        gesture->histories[i].time = 0;
        gesture->histories[i].x = 0;
        gesture->histories[i].y = 0;
    }
    gesture->history_idx = 0;
    //nemotimer_set_timeout(gesture->color_timer, 20);
}

static void gesture_move_calculate(NemouiGesture *gesture, NemoWidget *widget, void *event, int *tx, int *ty)
{
    double cx, cy;
    nemoui_gesture_get_center(gesture, event, &cx, &cy);

    // history update
    gesture->histories[gesture->history_idx].time = nemoshow_event_get_time(event);
    gesture->histories[gesture->history_idx].x = cx;
    gesture->histories[gesture->history_idx].y = cy;
    gesture->history_idx++;
    if (gesture->history_idx >= HISTORY_MAX) gesture->history_idx = 0;

    if (tx) *tx = cx;
    if (ty) *ty = cy;
    /*
    _translate(gesture, 0, 0, 0,
            cx + gesture->move_diff_x,
            cy + gesture->move_diff_y);
            */
}

static bool gesture_throw_calculate(NemouiGesture *gesture, NemoWidget *widget, void *event, double *tx, double *ty)
{
    if (list_count(gesture->grabs) < 2) return false;

    double sum = 0;
    int i, j, k = 0;
    int start = -9999, end;
    i = gesture->history_idx;

    do {
        if (gesture->histories[i].time == 0) {
            i++;
            if (i >= HISTORY_MAX) i = 0;
            continue;
        }

        j = i + 1;
        if (j >= HISTORY_MAX) j = 0;

        if (gesture->histories[j].time == 0) break;

        if (abs(gesture->histories[i].x - gesture->histories[j].y) <= HISTORY_MIN_DIST &&
                abs(gesture->histories[i].y - gesture->histories[i].y <= HISTORY_MIN_DIST)) {
            i++;
            if (i >= HISTORY_MAX) i = 0;
            continue;
        }

        int tdiff = gesture->histories[j].time - gesture->histories[i].time;
        if (tdiff <= 0) {
            tdiff = 8;
        }

        if (start < -100) start = i;
        end = i;

        sum += DISTANCE(gesture->histories[j].x, gesture->histories[j].y,
                gesture->histories[i].x, gesture->histories[i].y)/tdiff;
        k++;
        i++;
        if (i >= HISTORY_MAX) i = 0;
    } while (i != gesture->history_idx);

    if (k < HISTORY_MIN_CNT) return false;

    double dir = atan2f(
            (gesture->histories[start].y - gesture->histories[end].y),
            (gesture->histories[start].x - gesture->histories[end].x));
    double avg = (sum/k) * THROW_COEFF;

    if (tx) *tx = -avg * cos(dir) + gesture->histories[end].x;
    if (ty) *ty = -avg * sin(dir) + gesture->histories[end].y;

    gesture->history_idx = 0;

    return true;
    /*
       if (tx <= 0) tx = 0;
       if (tx >= bg->width) tx = bg->width;
       if (ty <= 0) ty = 0;
       if (ty >= bg->height) ty = bg->height;

       if (abs(nemoshow_item_get_translate_x(gesture->group) - tx) > THROW_MIN_DIST ||
       abs(nemoshow_item_get_translate_y(gesture->group) - ty) > THROW_MIN_DIST) {
       _nemoshow_item_motion(gesture->group,
       NEMOEASE_CUBIC_OUT_TYPE, THROW_DURATION, 0,
       "tx", tx, "ty", ty, NULL);
       }
    */

}

static void gesture_append_grab(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    Grab *grab = malloc(sizeof(Grab));
    grab->device = nemoshow_event_get_device(event);
    grab->idx = -1;

    gesture->grabs = list_append(gesture->grabs, grab);

    gesture_update_index(gesture, widget, event);
    gesture_move_init(gesture, widget, event);
    gesture_rotate_init(gesture, widget, event);
    gesture_scale_init(gesture, widget, event);
}

static void gesture_remove_grab(NemouiGesture *gesture, NemoWidget *widget, void *event)
{
    List *l;
    Grab *g;
    LIST_FOR_EACH(gesture->grabs, l, g) {
        if (g->device == nemoshow_event_get_device(event)) {
            break;
        }
    }
    RET_IF(!g);
    gesture->grabs = list_remove(gesture->grabs, g);

    gesture_update_index(gesture, widget, event);
    gesture_rotate_init(gesture, widget, event);
    gesture_scale_init(gesture, widget, event);
}

static void _gesture_grab_event(NemoWidgetGrab *grab, NemoWidget *widget, struct showevent *event, void *userdata)
{
}

static void _nemoui_gesture_event(NemoWidget *widget, const char *id, void *info, void *userdata)
{
    NemouiGesture *gesture = userdata;
    struct showevent *event = info;
    struct nemoshow *show = nemowidget_get_show(widget);

    double ex, ey;
    nemowidget_transform_from_global(widget,
            nemoshow_event_get_x(event),
            nemoshow_event_get_y(event), &ex, &ey);

    if (nemoshow_event_is_down(show, event)) {
        gesture_append_grab(gesture, widget, event);
        if (list_count(gesture->grabs) == 2) {
            if (gesture->scale_start) {
                gesture->scale_start(gesture, widget, event, gesture->scale_userdata);
            }
            if (gesture->rotate_start) {
                gesture->rotate_start(gesture, widget, event, gesture->rotate_userdata);
            }
        }
        /*
        if (list_count(gesture->grabs) == 1) {
            // Revoke all transitions
            nemoshow_revoke_transition(show, gesture->group, "tx");
            nemoshow_revoke_transition(show, gesture->group, "ty");
            nemoshow_revoke_transition(show, gesture->group, "sx");
            nemoshow_revoke_transition(show, gesture->group, "sy");
            nemoshow_revoke_transition(show, gesture->group, "ro");
            nemotimer_set_timeout(gesture->move_timer, 0);
        }
        */
    } else if (nemoshow_event_is_motion(show, event)) {
        Grab *g = LIST_DATA(LIST_FIRST(gesture->grabs));
        // XXX: To reduce duplicated caclulation
        if (g->device == nemoshow_event_get_device(event)) {
            if (gesture->move) {
                int tx, ty;
                gesture_move_calculate(gesture, widget, event, &tx, &ty);
                gesture->move(gesture, widget, event, tx, ty, gesture->move_userdata);
            }
            if (gesture->scale) {
                double scale;
                if (gesture_scale_calculate(gesture, widget, event, &scale)) {
                    gesture->scale(gesture, widget, event, scale, gesture->scale_userdata);
                }
            }
            if (gesture->rotate) {
                double ro;
                if (gesture_rotate_calculate(gesture, widget, event, &ro)) {
                    gesture->rotate(gesture, widget, event, ro, gesture->rotate_userdata);
                }
            }
        }
    } else if (nemoshow_event_is_up(show, event)) {
        gesture_remove_grab(gesture, widget, event);
        if (gesture->throw) {
            double tx, ty;
            if (gesture_throw_calculate(gesture, widget, event, &tx, &ty)) {
                gesture->throw(gesture, widget, event, tx, ty, gesture->throw_userdata);
            }
        }
        if (list_count(gesture->grabs) == 1) {
            if (gesture->scale_stop) {
                gesture->scale_stop(gesture, widget, event, gesture->scale_userdata);
            }
            if (gesture->rotate_stop) {
                gesture->rotate_stop(gesture, widget, event, gesture->rotate_userdata);
            }
        }
    }
}

NemouiGesture *nemoui_gesture_create(NemoWidget *parent, int width, int height)
{
    NemouiGesture *gesture = calloc(sizeof(NemouiGesture), 1);

    NemoWidget *widget;
    gesture->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_append_callback(widget, "event", _nemoui_gesture_event, gesture);
    nemowidget_enable_event_repeat(widget, true);

    return gesture;
}

void nemoui_gesture_show(NemouiGesture *gesture)
{
    nemowidget_show(gesture->widget, 0, 0, 0);
}

void nemoui_gesture_hide(NemouiGesture *gesture)
{
    nemowidget_hide(gesture->widget, 0, 0, 0);
}

void nemoui_gesture_translate(NemouiGesture *gesture, int tx, int ty)
{
    nemowidget_translate(gesture->widget, 0, 0, 0, tx, ty);
}

void nemoui_gesture_set_move(NemouiGesture *gesture, GestureMove move, GestureMoveStart start, GestureMoveStop stop, void *userdata)
{
    gesture->move = move;
    gesture->move_start = start;
    gesture->move_stop = stop;
    gesture->move_userdata = userdata;
}

void nemoui_gesture_set_throw(NemouiGesture *gesture, GestureThrow callback, void *userdata)
{
    gesture->throw = callback;
    gesture->throw_userdata = userdata;
}

void nemoui_gesture_set_scale(NemouiGesture *gesture, GestureScale scale, GestureScaleStart start, GestureScaleStop stop, void *userdata)
{
    gesture->scale = scale;
    gesture->scale_start = start;
    gesture->scale_stop = stop;
    gesture->scale_userdata = userdata;
}

void nemoui_gesture_set_rotate(NemouiGesture *gesture, GestureRotate rotate, GestureRotateStart start, GestureRotateStop stop, void *userdata)
{
    gesture->rotate = rotate;
    gesture->rotate_start = start;
    gesture->rotate_stop = stop;
    gesture->rotate_userdata = userdata;
}
