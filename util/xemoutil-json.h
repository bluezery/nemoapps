#ifndef __XEMOUTIL_JSON_H__
#define __XEMOUTIL_JSON_H__

#include <xemoutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XemoJson XemoJson;

void xemojson_destroy(XemoJson *json);
XemoJson *xemojson_create(const char *data);
Values *xemojson_find(XemoJson *json, const char *names);

#ifdef __cplusplus
}
#endif

#endif
