#ifndef __MAPINFO_UTIL__
#define __MAPINFO_UTIL__

#ifdef __cplusplus
extern "C" {
#endif

#include <showone.h>

extern double mapinfo_util_get_runner_start_angle(uint32_t num, uint32_t nth);
extern void mapinfo_util_move_item_above(struct showone *canvas, struct showone *item);
extern struct showone *mapinfo_util_create_image(struct showone* canvas, double w, double h, char *uri);
extern struct showone *mapinfo_util_create_clip(struct showone* canvas, double w, double h, double r);

extern struct showone* mapinfo_util_create_transition();
extern struct showtransition* mapinfo_util_execute_transition(struct nemoshow* show, struct showone* frame, int easytype, uint32_t duration, uint32_t delay, int repeat);
extern struct showone* mapinfo_util_create_circle_path(double r, int isstroke, int isfill);

#ifdef __cplusplus
}
#endif

#endif
