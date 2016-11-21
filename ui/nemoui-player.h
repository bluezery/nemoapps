#ifndef __NEMOVIEW_PLAYER_H__
#define __NEMOVIEW_PLAYER_H__

typedef struct _PlayerView PlayerView;

void nemoui_player_play(PlayerView *view);
void nemoui_player_stop(PlayerView *view);
void nemoui_player_show(PlayerView *view);
void nemoui_player_hide(PlayerView *view);
void nemoui_player_destroy(PlayerView *view);
void nemoui_player_translate(PlayerView *view, uint32_t easetype, int duration, int delay, float x, float y);
PlayerView *nemoui_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio);

#endif
