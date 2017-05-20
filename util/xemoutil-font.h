#ifndef __XEMOUTIL_FONT__
#define __XEMOUTIL_FONT__

#include <fontconfig/fontconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XemoFont XemoFont;

bool xemofont_init();
void xemofont_shutdown();
void xemofont_destroy(XemoFont *font);
List *xemofont_list_get(int *num);
XemoFont *xemofont_create(const char *font_family, const char *font_style, int font_slant, int font_weight, int font_width, int font_spacing);
const char *xemofont_get_family(XemoFont *font);
const char *xemofont_get_style(XemoFont *font);
const char *xemofont_get_filepath(XemoFont *font);

#ifdef __cplusplus
}
#endif

#endif
