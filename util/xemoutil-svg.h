#ifndef __XEMOUTIL_SVG_H__
#define __XEMOUTIL_SVG_H__

#include <xemoutil-xml.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *svg_get_value(Xml *xml, const char *xpath, const char *name);
bool svg_get_wh(const char *uri, double *w, double *h);

#ifdef __cplusplus
}
#endif

#endif
