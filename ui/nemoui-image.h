#ifndef __NEMOUI_IMAGE_H__
#define __NEMOUI_IMAGE_H__

#include <stdbool.h>
#include <nemoshow.h>
#include <nemotool.h>
#include <xemoutil-image.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ImageBitmap ImageBitmap;
ImageBitmap *image_bitmap_create(const char *path);
ImageBitmap *image_bitmap_dup(ImageBitmap *src);
void image_bitmap_destroy(ImageBitmap *bitmap);

typedef void (*ImageLoadDone)(bool success, void *userdata);
typedef struct _ImageLoader ImageLoader;
typedef struct _Image Image;

void image_set_bitmap(Image *img, int width, int height, ImageBitmap *bitmap);
ImageBitmap *image_get_bitmap(Image *img);
ImageLoader *image_load(Image *img, struct nemotool *tool, const char *uri, int width, int height, ImageLoadDone done, void *userdata);
ImageLoader *image_load_full(Image *img, struct nemotool *tool, const char *uri, int width, int height, ImageLoadDone done, void *userdata);
ImageLoader *image_load_fit(Image *img, struct nemotool *tool, const char *uri, int width, int height, ImageLoadDone done, void *userdata);
void imageloader_cancel(ImageLoader *loader);
Image *image_create(struct showone *parent);
void image_load_cancel(Image *img);
void image_unload(Image *img);
void image_destroy(Image *img);
int image_get_width(Image *img);
int image_get_height(Image *img);
void image_load_sync(Image *img, const char *uri, int width, int height);
void image_translate(Image *img, uint32_t easetype, int duration, int delay, double tx, double ty);
void image_scale(Image *img, uint32_t easetype, int duration, int delay, double sx, double sy);
void image_rotate(Image *img, uint32_t easetype, int duration, int delay, double ro);
void image_set_alpha(Image *img, uint32_t easetype, int duration, int delay, double alpha);
void image_set_anchor(Image *img, double ax, double ay);
void image_set_clip(Image *img, struct showone *clip);
struct showone *image_get_one(Image *img);
struct showone *image_get_group(Image *img);
void image_below(Image *img, struct showone *one);
void image_above(Image *img, struct showone *one);

#ifdef __cplusplus
}
#endif

#endif
