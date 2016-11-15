#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdbool.h>
#include <pixman.h>
#include <showone.h>

#ifdef __cplusplus
extern "C" {
#endif

void pixman_composite(pixman_image_t *target, pixman_image_t *src);
void pixman_composite_fit(pixman_image_t *target, pixman_image_t *src);
/**********************************************************/
/* Image */
/**********************************************************/
typedef struct _ImageBitmap ImageBitmap;
bool image_get_wh(const char *path, int *w, int *h);

ImageBitmap *image_bitmap_create(const char *path);
void image_bitmap_attach(ImageBitmap *bitmap, struct showone *one);
void image_bitmap_detach(struct showone *one);
ImageBitmap *image_bitmap_dup(ImageBitmap *src);
void image_bitmap_destroy(ImageBitmap *bitmap);

void nemoshow_item_path_reset_transform(struct showone *one);
#ifdef __cplusplus
}
#endif

#endif
