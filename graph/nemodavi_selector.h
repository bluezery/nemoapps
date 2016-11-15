#ifndef __NEMODAVI_SELECTOR_H__
#define __NEMODAVI_SELECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include "nemodavi.h"
#include "nemodavi_data.h"

struct nemodavi_item;
struct nemodavi_event;

typedef enum {
	NEMODAVI_EVENT_CLICK = 0,
	NEMODAVI_EVENT_LONGPRESS,
	NEMODAVI_EVENT_DOUBLE_CLICK,
	NEMODAVI_EVENT_LAST
} NemoDaviEventType;

typedef void (*nemodavi_selector_data_listener_t) (int index, void *datum, void *userdata);
typedef void (*nemodavi_selector_event_listener_t) (int index, void *datum, void *userdata);
typedef uint32_t (*nemodavi_selector_filter_t) (int index, void *datum, void *userdata);
typedef int (*nemodavi_selector_comparator_t) (void *a, void *b, void *userdata);

struct nemodavi_selector {
	int count;
	char *name;
	struct nemodavi *davi;
	struct nemodavi_item **items;

	struct nemolist link;
};

struct nemodavi_item {
	char *name;
 	struct showone *one;
	struct nemodavi_datum *datum;
	struct nemodavi_event **events;
	nemodavi_selector_data_listener_t ulistener;
	void *userdata;

	struct nemolistener listener;
	struct nemolist set_list;
	struct nemolist link;
};

struct nemodavi_event {
	uint32_t type;
	struct nemodavi_selector *selector;
  nemodavi_selector_event_listener_t listener;
	void *userdata;
};

struct nemodavi_selector_filter {
	uint32_t index;
	struct nemolist link;
};

extern struct nemodavi_selector* nemodavi_selector_get(struct nemodavi *davi, char *name);
extern struct nemodavi_selector* nemodavi_selector_get_by_index(struct nemodavi *davi, char *name, uint32_t index);
extern struct nemodavi_selector* nemodavi_selector_get_by_range(struct nemodavi *davi, char *name, uint32_t from, uint32_t to);
extern struct nemodavi_selector* nemodavi_selector_get_by_filter(struct nemodavi *davi, char *name, nemodavi_selector_filter_t filter, void *userdata);
extern int nemodavi_selector_destory(struct nemodavi_selector *sel);

extern int nemodavi_selector_attach_data_listener(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, void *userdata);
extern int nemodavi_selector_attach_data_listener_by_index(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, uint32_t index, void *userdata);
extern int nemodavi_selector_attach_data_listener_by_range(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, uint32_t from, uint32_t to, void *userdata);
extern int nemodavi_selector_attach_data_listener_by_filter(struct nemodavi_selector *sel, nemodavi_selector_data_listener_t ulistener, void *luserdata, nemodavi_selector_filter_t filter, void *fuserdata);

extern int nemodavi_selector_detach_data_listener(struct nemodavi_selector *sel);
extern int nemodavi_selector_detach_data_listener_by_index(struct nemodavi_selector *sel, uint32_t index);
extern int nemodavi_selector_detach_data_listener_by_range(struct nemodavi_selector *sel, uint32_t from, uint32_t to);
extern int nemodavi_selector_detach_data_listener_by_filter(struct nemodavi_selector *sel, nemodavi_selector_filter_t filter, void *userdata);

extern int nemodavi_selector_attach_event_listener(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, void *userdata);
extern int nemodavi_selector_attach_event_listener_by_index(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, uint32_t index, void *userdata);
extern int nemodavi_selector_attach_event_listener_by_range(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, uint32_t from, uint32_t to, void *userdata);
extern int nemodavi_selector_attach_event_listener_by_filter(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_event_listener_t listener, void *luserdata, nemodavi_selector_filter_t filter, void *fuserdata);

extern int nemodavi_selector_detach_event_listener(struct nemodavi_selector *sel, uint32_t event);
extern int nemodavi_selector_detach_event_listener_by_index(struct nemodavi_selector *sel, uint32_t event, uint32_t index);
extern int nemodavi_selector_detach_event_listener_by_range(struct nemodavi_selector *sel, uint32_t event, uint32_t from, uint32_t to);
extern int nemodavi_selector_detach_event_listener_by_filter(struct nemodavi_selector *sel, uint32_t event, nemodavi_selector_filter_t filter, void *userdata);

extern int nemodavi_selector_sort(struct nemodavi_selector *sel, nemodavi_selector_comparator_t comparator, void *userdata);

static inline struct nemodavi* nemodavi_selector_get_davi(struct nemodavi_selector *sel)
{
	return sel->davi;
}

static inline struct nemodavi_datum* nemodavi_selector_get_datum(struct nemodavi_selector *sel, uint32_t index) 
{
	return sel->items[index]->datum;
}

static inline struct showone* nemodavi_selector_get_showitem(struct nemodavi_selector *sel, uint32_t index) 
{
	return sel->items[index]->one;
}

static inline struct nemolistener* nemodavi_selector_get_data_listener(struct nemodavi_selector *sel, uint32_t index)
{
	return &sel->items[index]->listener;
}

static inline struct nemolist* nemodavi_selector_get_showset_list(struct nemodavi_selector *sel, uint32_t index) 
{
	return &sel->items[index]->set_list;
}

static inline void nemodavi_selector_set_data_listener(struct nemodavi_selector *sel, uint32_t index, nemosignal_notify_t notify)
{
	sel->items[index]->listener.notify = notify;
}

static inline void nemodavi_selector_set_data_ulistener(struct nemodavi_selector *sel, uint32_t index, nemodavi_selector_data_listener_t ulistener, void *userdata)
{
	sel->items[index]->ulistener = ulistener;
	sel->items[index]->userdata = userdata;
}

static inline void nemodavi_selector_set_event_listener(struct nemodavi_selector *sel, uint32_t index, uint32_t event, nemodavi_selector_event_listener_t listener, void *userdata)
{
	sel->items[index]->events[event]->type = event;
	sel->items[index]->events[event]->selector = sel;
	sel->items[index]->events[event]->listener = listener;
	sel->items[index]->events[event]->userdata = userdata;
}

static inline void nemodavi_selector_set_item(struct nemodavi_selector *sel, uint32_t index, struct nemodavi_item *item)
{
	sel->items[index] = item;
}

#ifdef __cplusplus
}
#endif

#endif
