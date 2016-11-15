#ifndef __NEMODAVI_H__
#define __NEMODAVI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nemolist.h>
#include <nemolistener.h>
#include <showone.h>

#include "nemodavi.h"
#include "nemodavi_attr.h"
#include "nemodavi_data.h"
#include "nemodavi_selector.h"
#include "nemodavi_transition.h"
#include "nemodavi_layout.h"
#include "nemodavi_item.h"

#define NEMODAVI_ONE_TAG(tag)								((tag >> 0) & 0xffff)
#define NEMODAVI_ONE_GROUP(tag)			((tag >> 16) & 0xffff)

#define NEMODAVI_ONE_TAGGROUP(tag, group)		(((tag & 0xffff) << 0) | ((group & 0xffff) << 16))

typedef struct showone* (*nemodavi_append_selector_handler_t) (int index, void *datum, void *userdata);

struct nemodavi {
	struct showone *canvas;

	uint32_t id;
	uint32_t tag_index;
	uint32_t width;
	uint32_t height;

	struct nemodavi_data *data;
	struct nemolist **item_lists;

	struct nemolist selector_list;
	struct nemolist trans_list;

	struct showone *group;
	struct nemodavi_layout *layout;

	struct nemolist link;
};

extern struct nemodavi* nemodavi_create(struct showone *canvas, struct nemodavi_data *data);
extern int nemodavi_destroy(struct nemodavi *davi);
extern int nemodavi_append_selector(struct nemodavi *davi, char *name, int type);
extern int nemodavi_append_selector_by_handler(struct nemodavi *davi, char *name, nemodavi_append_selector_handler_t handler, void *userdata);
extern struct nemodavi_datum* nemodavi_get_datum(struct nemodavi *davi, uint32_t index);
extern void nemodavi_emit_data_change_signal(struct nemodavi *davi);
extern void nemodavi_emit_event(uint32_t event, uint32_t tag);

extern void nemodavi_translate(struct nemodavi *davi, double tx, double ty);
extern void nemodavi_scale(struct nemodavi *davi, double sx, double sy);
extern void nemodavi_rotate(struct nemodavi *davi, double angle);

static inline uint32_t nemodavi_get_width(struct nemodavi *davi)
{
	return davi->width;
}

static inline uint32_t nemodavi_get_height(struct nemodavi *davi)
{
	return davi->height;
}

static inline uint32_t nemodavi_get_datum_count(struct nemodavi *davi)
{
	return davi->data->data_count;
}

static inline struct nemolist* nemodavi_get_item_list(struct nemodavi *davi, uint32_t index)
{
	return davi->item_lists[index];
}

static inline struct nemodavi_layout* nemodavi_get_layout(struct nemodavi *davi)
{
	return davi->layout;
}

static inline void nemodavi_set_layout(struct nemodavi *davi, struct nemodavi_layout *layout)
{
	davi->layout = layout;
}

#ifdef __cplusplus
}
#endif

#endif

