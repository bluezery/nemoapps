#ifndef __NEMOUTIL_IMAGE_H__
#define __NEMOUTIL_IMAGE_H__

#include <stdbool.h>
#include <pixman.h>

#ifdef __cplusplus
extern "C" {
#endif

void pixman_composite(pixman_image_t *target, pixman_image_t *src);
void pixman_composite_fit(pixman_image_t *target, pixman_image_t *src);
/**********************************************************/
/* Image */
/**********************************************************/
bool image_get_wh(const char *path, int *w, int *h);

#ifdef __cplusplus
}
#endif

#endif
