#ifndef __NEMOUI_PATH_H__
#define __NEMOUI_PATH_H__

#include <nemoshow.h>
#include <nemoutil-path.h>

#ifdef __cplusplus
extern "C" {
#endif

struct showone *path_draw(Path *path, struct showone *group);
struct showone *path_draw_array(Path *path, struct showone *group);
void path_array_morph(struct showone *one, uint32_t easetype, int duration, int delay, Path *path);
void path_debug(Path *path, struct showone *group, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
