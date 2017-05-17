#ifndef __HELPER_SKIA_H__
#define __HELPER_SKIA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SkiaBitmap SkiaBitmap;
SkiaBitmap *skia_bitmap_create(const char *path);
void skia_bitmap_destroy(SkiaBitmap *bitmap);
SkiaBitmap *skia_bitmap_dup(SkiaBitmap *bitmap);

#ifdef __cplusplus
}
#endif

#endif

