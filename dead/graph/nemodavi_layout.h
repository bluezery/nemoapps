#ifndef __NEMODAVI_LAYOUT_H__
#define __NEMODAVI_LAYOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nemolist.h>

#include "nemodavi.h"
#include "nemodavi_layout.h"

typedef enum {
	NEMODAVI_LAYOUT_HISTOGRAM = 0,
	NEMODAVI_LAYOUT_DONUT,
	NEMODAVI_LAYOUT_DONUTBAR,
	NEMODAVI_LAYOUT_DONUTARRAY,
	NEMODAVI_LAYOUT_ARC,
	NEMODAVI_LAYOUT_AREA,
	NEMODAVI_LAYOUT_TEST,
	NEMODAVI_LAYOUT_LAST
} NemoDaviLayoutType;

typedef enum {
	NEMODAVI_LAYOUT_DELEGATOR_CREATE = 0,
	NEMODAVI_LAYOUT_DELEGATOR_SHOW,
	NEMODAVI_LAYOUT_DELEGATOR_UPDATE,
	NEMODAVI_LAYOUT_DELEGATOR_HIDE,
	NEMODAVI_LAYOUT_DELEGATOR_LAST
} NemoDaviLayoutDelegatorType;

typedef enum {
	NEMODAVI_LAYOUT_GETTER_VALUE = 0,
	NEMODAVI_LAYOUT_GETTER_NAME,
	NEMODAVI_LAYOUT_GETTER_COLOR,
	NEMODAVI_LAYOUT_GETTER_USR1,
	NEMODAVI_LAYOUT_GETTER_USR2,
	NEMODAVI_LAYOUT_GETTER_USR3,
	NEMODAVI_LAYOUT_GETTER_USR4,
	NEMODAVI_LAYOUT_GETTER_USR5,
	NEMODAVI_LAYOUT_GETTER_USR6,
	NEMODAVI_LAYOUT_GETTER_USR7,
	NEMODAVI_LAYOUT_GETTER_USR8,
	NEMODAVI_LAYOUT_GETTER_USR9,
	NEMODAVI_LAYOUT_GETTER_USR10,
	NEMODAVI_LAYOUT_GETTER_LAST
} NemoDaviLayoutGetterType;

typedef int (*nemodavi_layout_delegator_t) (struct nemodavi_layout *layout);
typedef nemodavi_layout_delegator_t* (*nemodavi_layout_register_t) (void);
typedef char* (*nemodavi_layout_getter_t) (uint32_t index, void *data);

struct nemodavi_layout {
	uint32_t type;
	uint32_t created;
	struct nemodavi *davi;
	nemodavi_layout_delegator_t *delegators;
	nemodavi_layout_getter_t getters[NEMODAVI_LAYOUT_GETTER_LAST];
	struct nemolist link;
};

extern struct nemodavi_layout* nemodavi_layout_append(struct nemodavi *davi, NemoDaviLayoutType type);
extern int nemodavi_layout_call_delegator(struct nemodavi_layout *layout, NemoDaviLayoutDelegatorType type);
extern int nemodavi_layout_set_getter(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, nemodavi_layout_getter_t getter);
extern nemodavi_layout_getter_t nemodavi_layout_get_getter(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type);

extern int nemodavi_layout_call_getter_int(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, int *out);
extern int nemodavi_layout_call_getter_double(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, double *out);
extern int nemodavi_layout_call_getter_string(struct nemodavi_layout *layout, NemoDaviLayoutGetterType type, uint32_t index, void *datum, char **out);

extern nemodavi_layout_delegator_t* nemodavi_layout_allocate_delegators();

static inline struct nemodavi* nemodavi_layout_get_davi(struct nemodavi_layout *layout)
{
	return layout->davi;
}

#ifdef __cplusplus
}
#endif

#endif
