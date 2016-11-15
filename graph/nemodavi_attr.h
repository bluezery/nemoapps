#ifndef __NEMODAVI_ATTR_H__
#define __NEMODAVI_ATTR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <showone.h>

#include "nemodavi.h"
#include "nemodavi_selector.h"

typedef void (nemodavi_set_attr_handler_t)(struct showone*, int index, void *datum, void *userdata);
typedef uint32_t (nemodavi_set_cattr_handler_t)(int index, void *datum, void *userdata);
typedef double (nemodavi_set_dattr_handler_t)(int index, void *datum, void *userdata);
typedef char* (nemodavi_set_sattr_handler_t)(int index, void *datum, void *userdata);
typedef struct showone* (nemodavi_set_oattr_handler_t)(int index, void *datum, void *userdata);
typedef double (nemodavi_delay_handler_t)(int index, void *datum, void *userdata);


extern int nemodavi_set_cattr(struct nemodavi_selector *sel, char *attr, uint32_t value);
extern int nemodavi_set_dattr(struct nemodavi_selector *sel, char *attr, double value);
extern int nemodavi_set_sattr(struct nemodavi_selector *sel, char *attr, char *value);
extern int nemodavi_set_oattr(struct nemodavi_selector *sel, char *attr, struct showone *value);
extern int nemodavi_set_cattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, uint32_t value);
extern int nemodavi_set_dattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, double value);
extern int nemodavi_set_sattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, char *value);
extern int nemodavi_set_oattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, struct showone *value);

extern int nemodavi_set_cattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_cattr_handler_t handler, void *userdata);
extern int nemodavi_set_dattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_dattr_handler_t handler, void *userdata);
extern int nemodavi_set_sattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_sattr_handler_t handler, void *userdata);
extern int nemodavi_set_oattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_oattr_handler_t handler, void *userdata);
extern int nemodavi_set_attr_handler(struct nemodavi_selector *sel, nemodavi_set_attr_handler_t handler, void *userdata);

extern uint32_t nemodavi_get_cattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index);
extern double nemodavi_get_dattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index);
extern char* nemodavi_get_sattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif

