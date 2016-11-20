#ifndef __NEMOVIEW_PLAYER_H__
#define __NEMOVIEW_PLAYER_H__

typedef struct _PlayerView PlayerView;

void nemoview_player_play(PlayerView *view);
void nemoview_player_stop(PlayerView *view);
void nemoview_player_show(PlayerView *view);
void nemoview_player_hide(PlayerView *view);
void nemoview_player_destroy(PlayerView *view);
void nemoview_player_translate(PlayerView *view, uint32_t easetype, int duration, int delay, float x, float y);
PlayerView *nemoview_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio);

#endif
