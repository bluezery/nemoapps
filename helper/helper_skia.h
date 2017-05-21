#ifndef __HELPER_SKIA_H__
#define __HELPER_SKIA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SkiaBitmap SkiaBitmap;
SkiaBitmap *skia_bitmap_create(const char *path);
SkiaBitmap *skia_bitmap_dup(SkiaBitmap *bitmap);
void skia_bitmap_destroy(SkiaBitmap *bitmap);
double skia_calculate_text_width(const char *fontfamily, const char *fontstyle, double fontsize, const char *str);

#ifdef __cplusplus
}
#endif

#endif

