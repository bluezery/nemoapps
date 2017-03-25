#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nemoutil-log.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************/
/* List */
/***************************************************/
typedef struct _ListInfo ListInfo;
typedef struct _List List;
struct _ListInfo
{
    unsigned int cnt;
    List *first;
    List *last;
};

struct _List
{
    void *data;
    ListInfo *info;

    List *prev;
    List *next;
};

#define LIST_FIRST(list) \
    ((list) ? ((list)->info->first) : NULL)

#define LIST_LAST(list) \
    ((list) ? ((list)->info->last) : NULL)

#define LIST_DATA(list) \
    ((list) ? ((list)->data) : NULL)

#define LIST_ITER(l, d) \
    for (d = (l ? l->data : NULL) ; \
            l ; \
            l = l->next, d = (__typeof__(d))(l ? l->data : NULL ))

#define LIST_FOR_EACH(list, l, d) \
    for (l = LIST_FIRST(list), d = (__typeof__(d))(l ? l->data : NULL) ; \
            l ; \
            l = l->next, d = (__typeof__(d))(l ? l->data : NULL ))

#define LIST_FOR_EACH_SAFE(list, l, tmp, d) \
    for (l = LIST_FIRST(list), d = (__typeof__(d))(l ? l->data : NULL), tmp = (l ? l->next : NULL) ; \
            l ; \
            l = tmp, d = (__typeof__(d))(l ? l->data : NULL), tmp = (l ? l->next : NULL))

#define LIST_FOR_EACH_REVERSE(list, l, d) \
    for (l = LIST_LAST(list), d = (__typeof__(d))(l ? l->data : NULL) ; \
            l ; \
            l = l->prev, d = (__typeof__(d))(l ? l->data : NULL ))

#define LIST_FOR_EACH_REVERSE_SAFE(list, l, tmp, d) \
    for (l = LIST_LAST(list), d = (__typeof__(d))(l ? l->data : NULL), tmp = (l ? l->prev : NULL) ; \
            l ; \
            l = tmp, d = (__typeof__(d))(l ? l->data : NULL), tmp = (l ? l->prev : NULL))

#define LIST_FREE(list, d) \
    for (list = LIST_FIRST(list), (list) ? free((list)->info) : (void)(NULL), d = (__typeof__(d))((list) ? (list)->data : NULL); \
            list ; \
            (list) ? free((list)->prev) : (void)(NULL), list = (list) ? (list)->next : NULL, d = (__typeof__(d))((list) ? (list)->data : NULL) )

static inline List *list_insert_after(List *list, void *data)
{
    List *item = (List *)malloc(sizeof(List));
    item->data = data;

    if (list) {
        item->info = list->info;
        item->info->cnt++;

        item->prev = list;
        item->next = list->next;
        if (list->next) {
            list->next->prev = item;
        } else {
            list->info->last =  item;
        }
        list->next = item;
        return list;
    }

    item->prev = NULL;
    item->next = NULL;
    item->info = (ListInfo *)malloc(sizeof(ListInfo));
    item->info->cnt = 1;
    item->info->first = item;
    item->info->last = item;
    return item;
}

static inline List *list_insert_before(List *list, void *data)
{
    List *item = (List *)malloc(sizeof(List));
    item->data = data;

    if (list) {
        item->info = list->info;
        item->info->cnt++;

        item->prev = list->prev;
        item->next = list;
        if (list->prev) {
            list->prev->next = item;
        } else {
            item->info->first = item;
        }
        list->prev = item;
        return list;
    }

    item->prev = NULL;
    item->next = NULL;
    item->info = (ListInfo *)malloc(sizeof(ListInfo));
    item->info->cnt = 1;
    item->info->first = item;
    item->info->last = item;
    return item;
}

static inline List *list_append(List *list, void *data)
{
    return list_insert_after(LIST_LAST(list), data);
}

static inline List *list_insert_sorted(List *list, void *data)
{
    if (!list) {
        list = list_append(list, data);
    } else {
        void *d;
        List *l;
        LIST_FOR_EACH(list, l, d) {
            if (strcmp((char *)d, (char *)data) < 0) {
                list = list_insert_before(l, data);
                break;
            } else if (LIST_LAST(list) == l) {
                list = list_insert_after(l, data);
                break;
            }
        }
    }
    return list;
}

static inline List *list_prepend(List *list, void *data)
{
    return list_insert_before(LIST_FIRST(list), data);
}

static inline List *list_find(List *list, void *data)
{
    if (!list) return NULL;

    List *item = list->info->first;
    while (item) {
        if (item->data == data) break;
        item = item->next;
    }
    return item;
}

static inline List *list_remove(List *list, void *data)
{
    if (!list) return NULL;

    List *item = list_find(list, data);
    if (item) {
        if (item->prev) {
            item->prev->next = item->next;
        } else {
            item->info->first = item->next;
        }
        if (item->next) {
            item->next->prev = item->prev;
        } else {
            item->info->last = item->prev;
        }

        item->info->cnt--;
        if (item->info->cnt == 0) {
            free(item->info);
            free(item);
            return NULL;
        }
        if (list == item) {
            list = list->info->first;
        }
        free(item);
    } else {
        ERR("Could not find data(%p)", data);
    }

    return list;
}

static inline List *list_free(List *list)
{
    if (!list) return NULL;

    list = list->info->first;
    free(list->info);

    List *tmp;
    while (list) {
        tmp = list->next;
        free(list);
        list = tmp;
    }
    return NULL;
}

static inline int list_count(List *list)
{
    if (!list) return 0;
    return list->info->cnt;
}

static inline int list_get_idx(List *list, void *data)
{
    if (!list) return -1;

    List *item = list->info->first;
    unsigned int idx = 0;
    while (item) {
        if (item->data == data) break;
        item = item->next;
        idx++;
    }
    if (!item) return -1;
    return idx;
}

static inline List *list_get_nth(List *list, unsigned int idx)
{
    if (!list) return NULL;
    if (list->info->cnt <= idx) return NULL;

    list = list->info->first;
    while (idx > 0) {
        list = list->next;
        idx--;
    }

    return list;
}

typedef int (*ListCmp)(void *data0, void *data1);
static inline List *list_dup(List *list)
{
    List *ret = NULL;
    List *l;
    void *data;
    LIST_FOR_EACH(list, l, data) {
        ret = list_append(ret, data);
    }
    return ret;
}

static inline void list_sort(List **list, ListCmp cmp)
{
    List *ret = NULL;
    List *left = NULL;
    List *right = NULL;
    void *pivot = LIST_DATA(LIST_FIRST(*list));
    void *data;

    LIST_FREE(*list, data) {
        if (pivot == data) continue;
        if (cmp(data, pivot) > 0) {
            left = list_append(left, data);
        } else {
            right = list_append(right, data);
        }
    }
    if (left) {
        list_sort(&left, cmp);
        LIST_FREE(left, data) {
            ret = list_append(ret, data);
        }
    }
    ret = list_append(ret, pivot);
    if (right) {
        list_sort(&right, cmp);
        LIST_FREE(right, data) {
            ret = list_append(ret, data);
        }
    }
    *list = ret;
}
/**********************************************************/
/* Inline List */
/**********************************************************/
typedef struct _InlistInfo InlistInfo;
typedef struct _Inlist Inlist;

struct _InlistInfo {
    unsigned int cnt;
    Inlist *first;
    Inlist *last;
};

struct _Inlist {
    InlistInfo *info;

    Inlist *prev;
    Inlist *next;
};

#define OFFSET_OF(type, member) \
    ((char *)&((type *)0)->member - (char *)0)

#define CONTAINER_OF(ptr, sample) \
    ((__typeof__(sample) *)((char *)ptr - OFFSET_OF(__typeof__(sample), __inlist)))

#define INLIST(ptr) \
    (&((ptr)->__inlist))

#define INLIST_FIRST(list) \
    ((list) ? ((list)->info->first) : NULL)

#define INLIST_LAST(list) \
    ((list) ? ((list)->info->last) : NULL)

#define INLIST_DATA(list, type) \
    ((type *)((char *)list - OFFSET_OF(type, __inlist)))

#define INLIST_FOR_EACH(list, d) \
    for (d = ((list && INLIST_FIRST(list)) ? CONTAINER_OF(INLIST_FIRST(list), *(d)) : NULL) ; \
            d ; \
            d = (d && INLIST(d)->next) ? CONTAINER_OF(INLIST(d)->next, *(d)) : NULL)

#define INLIST_FOR_EACH_SAFE(list, tmp, d) \
    for (d = ((list && INLIST_FIRST(list)) ? CONTAINER_OF(INLIST_FIRST(list), *(d) ): NULL), \
            tmp = (d && INLIST(d)->next) ? CONTAINER_OF(INLIST(d)->next, *(d)) : NULL ; \
            d ; \
            d= tmp, tmp = (d && INLIST(d)->next) ? CONTAINER_OF(INLIST(d)->next, *(d)) : NULL)

#define INLIST_FOR_EACH_REVERSE(list, d) \
    for (d = ((list) ? CONTAINER_OF(INLIST_LAST(list), *(d)) : NULL) ; \
            d ; \
            d = CONTAINER_OF(INLIST(d)->prev, *(d)))

#define INLIST_FOR_EACH_REVERSE_SAFE(list, tmp, d) \
    for (d = ((list && INLIST_LAST(list)) ? CONTAINER_OF(INLIST_LAST(list), *(d) ): NULL), \
            tmp = (d && INLIST(d)->prev) ? CONTAINER_OF(INLIST(d)->prev, *(d)) : NULL ; \
            d ; \
            d = tmp, tmp = (d && INLIST(d)->prev) ? CONTAINER_OF(INLIST(d)->prev, *(d)) : NULL)

#define INLIST_FREE(list, d) \
    for (list = INLIST_FIRST(list), (list) ? free((list)->info) : NULL, \
            d = ((list) ? CONTAINER_OF(list, *(d)) : NULL) ;\
            d ;\
            list = (list) ? (list)->next : NULL, d = d ? CONTAINER_OF(list, *(d)) : NULL)

static inline Inlist *inlist_insert_after(Inlist *list, Inlist *item)
{
    if (list) {
        item->info = list->info;
        list->info->cnt++;

        item->prev = list;
        item->next = list->next;
        if (list->next) {
            list->next->prev = item;
        } else {
            list->info->last = item;
        }
        list->next = item;
        return list;
    }

    item->prev = NULL;
    item->next = NULL;
    item->info = (InlistInfo *)malloc(sizeof(InlistInfo));
    item->info->cnt = 1;
    item->info->first = item;
    item->info->last = item;
    return item;
}

static inline Inlist *inlist_insert_before(Inlist *list, Inlist *item)
{
    if (list) {
        item->info = list->info;
        list->info->cnt++;

        item->prev = list->prev;
        item->next = list;
        if (list->prev) {
            list->prev->next = item;
        }
        else {
            list->info->first = item;
        }
        list->prev = item;
        return list;
    }

    item->prev = NULL;
    item->next = NULL;
    item->info = (InlistInfo *)malloc(sizeof(InlistInfo));
    item->info->cnt = 1;
    item->info->first = item;
    item->info->last = item;
    return item;
}

static inline Inlist *inlist_append(Inlist *list, Inlist *item)
{
    return inlist_insert_after(INLIST_LAST(list), item);
}

static inline Inlist *inlist_prepend(Inlist *list, Inlist *item)
{
    return inlist_insert_before(INLIST_FIRST(list), item);
}

static inline Inlist *inlist_remove(Inlist *list, Inlist *item)
{
    if (item->prev) {
        item->prev->next = item->next;
    } else {
        item->info->first = item->next;
    }

    if (item->next) {
        item->next->prev = item->prev;
    } else {
        item->info->last = item->prev;
    }

    item->info->cnt--;
    if (item->info->cnt == 0) {
        free(item->info);
        return NULL;
    }
    if (list == item) return item->info->first;
    return list;
}

static inline Inlist *inlist_free(Inlist *list)
{
    if (!list) return NULL;

    list = list->info->first;
    free(list->info);

    Inlist *tmp;
    while (list) {
        tmp = list->next;
        list->info = NULL;
        list->next = NULL;
        list->prev = NULL;
        list = tmp;
    }
    return NULL;
}

static inline int inlist_count(Inlist *list)
{
    if (!list) return 0;
    return list->info->cnt;
}

static inline int inlist_get_idx(Inlist *list)
{
    if (!list) return 0;

    Inlist *item = list->info->first;
    int idx = 0;
    while (item) {
        if (item == list) break;
        item = item->next;
        idx++;
    }
    if (!item) return -1;
    return idx;
}

#ifdef __cplusplus
}
#endif

#endif
