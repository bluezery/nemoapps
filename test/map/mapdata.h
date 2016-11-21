#ifndef __MAPDATA_H_
#define __MAPDATA_H_

#include "nemoutil.h"

extern List *__data_parties;
extern List *__data_runners;
extern List *__data_areas;

typedef struct _Party Party;
typedef struct _Area Area;
typedef struct _Runner Runner;

struct _Party {
    char *name;
    uint32_t color;
    List *runners;
    int cnt;
    int predominate_cnt;
};

struct _Area {
    uint32_t key;
    int depth;

    char *name;
    char *uri;
    int x, y, w, h;

    struct {
        char *uri;
        int x, y, w, h;
    } text;

    Area *parent;
    List *children;

    List *runners;

    Runner *runners2;
    int runners2_cnt;

    int vote_total;
};

struct _Runner {
    uint32_t key;
    char *name;
    char *uri;
    int votes;
    Party *party;
    Area *area;
};

//***********************//
//***** Party *****//
//***********************//
static inline Party *party_create(const char *name, uint32_t color)
{
    Party *party = calloc(sizeof(Party), 1);
    party->name = strdup(name);
    party->color = color;
    __data_parties = list_append(__data_parties, party);
    return party;
}

static inline void party_destroy(Party *party)
{
    __data_parties = list_remove(__data_parties, party);
    Runner *runner;
    LIST_FREE(party->runners, runner) {
        runner->party = NULL;
    }
    free(party->name);
    free(party);
}

static inline void party_append_runner(Party *party, Runner *runner)
{
    party->runners = list_append(party->runners, runner);
    runner->party = party;
}
//***********************//
//***** Runner *****//
//***********************//
static inline Runner *runner_create(uint32_t key, const char *name, const char *uri)
{
    Runner *runner = calloc(sizeof(Runner), 1);
    runner->key = key;
    runner->name = strdup(name);
    runner->uri = strdup(uri);
    __data_runners = list_append(__data_runners, runner);
    return runner;
}

static inline void runner_destroy(Runner *runner)
{
    __data_runners = list_remove(__data_runners, runner);
    free(runner->name);
    free(runner->uri);
    free(runner);
}

static inline void runner_set_vote(Runner *runner, int votes)
{
    runner->votes = votes;
}

static inline Runner *runner_search(uint32_t key)
{
    List *l;
    Runner *runner = NULL;
    LIST_FOR_EACH(__data_runners, l, runner) {
        if (runner->key == key)
            break;
    }
    return runner;
}

//***********************//
//***** Area *****//
//***********************//
static inline Area *area_create(uint32_t key, int depth, const char *name, const char *uri, int x, int y, int w, int h)
{
    Area *area = calloc(sizeof(Area), 1);
    area->key = key;
    area->name = strdup(name);
    area->depth = depth;
    area->uri = strdup(uri);
    area->x = x;
    area->y = y;
    area->w = w;
    area->h = h;
    __data_areas = list_append(__data_areas, area);
    return area;
}

static inline void area_set_text(Area *area, const char *uri, int x, int y, int w, int h)
{
    area->text.uri = strdup(uri);
    area->text.x = x;
    area->text.y = y;
    area->text.w = w;
    area->text.h = h;
}

static inline void area_set_vote_total(Area *area, int total)
{
    area->vote_total = total;
}

static inline void area_destroy(Area *area)
{
    __data_areas = list_remove(__data_areas, area);
    free(area->uri);
    if (area->text.uri) free(area->text.uri);
    free(area->name);

    Runner *runner;
    LIST_FREE(area->runners, runner) {
        runner->area = NULL;
    }
    free(area);
}

static inline List *area_get_runners(Area *area)
{
    if (!area) return NULL;
    return area->runners;
}

static inline List *area_get_children(Area *area)
{
    if (!area) return NULL;
    return area->children;
}

static inline void area_append_child(Area *area, Area *child)
{
    child->parent = area;
    area->children = list_append(area->children, child);
}

static inline Area *area_get_parent(Area *area)
{
    if (!area) return NULL;
    return area->parent;
}

static inline void area_append_runner(Area *area, Runner *runner)
{
    area->runners2_cnt++;
    area->runners2 = realloc(area->runners2, sizeof(Runner) * area->runners2_cnt);
    area->runners2[area->runners2_cnt - 1] = *runner;

    area->runners = list_append(area->runners, runner);
    runner->area = area;
}

static inline Area *area_search(uint32_t key)
{
    List *l;
    Area *area = NULL;
    LIST_FOR_EACH(__data_areas, l, area) {
        if (area->key == key)
            break;
    }
    return area;
}

static inline Party *util_match_party(List *parties, const char *name)
{
    RET_IF(!name, NULL)
    if (!parties) return NULL;
    List *l;
    Party *party = NULL;
    LIST_FOR_EACH(parties, l, party) {
        if (!strcmp(name, party->name)) {
            break;
        }
    }
    return party;
}

// Need to call list_free(retunred)
static inline List *area_calculate_parties(List *parties, Area *area)
{
    RET_IF(!area, NULL);
    if (!parties) {
        List *l;
        Party *party;
        LIST_FOR_EACH(__data_parties, l, party) {
            Party *tmp = calloc(sizeof(Party), 1);
            tmp->name = strdup(party->name);
            tmp->color = party->color;
            parties = list_append(parties, tmp);
        }
    }

    List *l;
    Area *child;
    LIST_FOR_EACH(area->children, l, child) {
        area_calculate_parties(parties, child);
    }

    Runner *top = LIST_DATA(LIST_FIRST(area->runners));
    Runner *runner;
    LIST_FOR_EACH(area->runners, l, runner) {
        Party *tmp = util_match_party(parties, runner->party->name);
        if (!tmp) {
            ERR("Something wroing :( (runner: %s[%s])", runner->name, runner->party->name);
            continue;
        }

        tmp->cnt++;
        if (top->votes < runner->votes) {
            top = runner;
        }
    }

    if (top) {
        Party *party;
        LIST_FOR_EACH(parties, l, party) {
            if (!strcmp(top->party->name, party->name)) {
                party->predominate_cnt++;
                break;
            }
        }
    }


    return parties;
}


#endif
