#ifndef __MAPINFO__
#define __MAPINFO__

#ifdef __cplusplus
extern "C" {
#endif

//#include <test/map.h>
//
#define MAPINFO_ENTER	1
#define MAPINFO_EXIT	0

#include "mapdata.h"

typedef enum {
	MAPINFO_MODE_RUNNER = 0,
	MAPINFO_MODE_PARTY,
	MAPINFO_MODE_LAST
} MapInfoMode;

typedef enum {
	MAPINFO_REQ_ENTER = 0,
	MAPINFO_REQ_EXIT,
	MAPINFO_REQ_UPDATE,
	MAPINFO_REQ_LAST
} MapInfoReq; 

typedef uint32_t (*mapinfo_mode_prepare_t) (Area *);
typedef uint32_t (*mapinfo_mode_enter_t) (uint32_t);
typedef uint32_t (*mapinfo_mode_exit_t) (uint32_t);
typedef uint32_t (*mapinfo_mode_update_t) (Area *);

struct mapinfo_mode {
	mapinfo_mode_prepare_t prepare;
	mapinfo_mode_enter_t enter;
	mapinfo_mode_update_t update;
	mapinfo_mode_exit_t exit;

	int width;
	int height;
	struct showone* canvas;
};

extern int mapinfo_init(struct nemotool *tool, struct nemoshow* show, struct showone* canvas, int scene_w, int scene_h);
extern int mapinfo_deinit();
extern int mapinfo_update_ui(Area* areainfo, MapInfoMode mode, MapInfoReq req);

extern int mapinit_mode_runner_init(struct mapinfo_mode *mode);
extern int mapinit_mode_party_init(struct mapinfo_mode *mode);

#ifdef __cplusplus
}
#endif

#endif

