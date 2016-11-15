#ifndef __NEMODAVI_ITEM_H__
#define __NEMODAVI_ITEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <showitem.h>
#include <showone.h>

typedef enum {
	NEMODAVI_RDONUT_ITEM = NEMOSHOW_LAST_ITEM,
	NEMODAVI_RBAR_ITEM,
	NEMODAVI_LAST_ITEM,
} NemoDaviItemType;

extern struct showone* nemodavi_item_create(int type);

#ifdef __cplusplus
}
#endif

#endif

