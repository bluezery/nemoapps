#ifndef __NEMOMISC_PLAYER_H__
#define __NEMOMISC_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PlayerUI PlayerUI;

void nemoui_player_play(PlayerUI *ui, uint32_t easetype, int duration, int delay);
void nemoui_player_stop(PlayerUI *ui);
void nemoui_player_show(PlayerUI *ui, uint32_t easetype, int duration, int delay);
void nemoui_player_hide(PlayerUI *ui);
void nemoui_player_destroy(PlayerUI *ui);
void nemoui_player_translate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float x, float y);
void nemoui_player_above(PlayerUI *ui, NemoWidget *above);
void nemoui_player_below(PlayerUI *ui, NemoWidget *below);
void nemoui_player_scale(PlayerUI *ui, uint32_t easetype, int duration, int delay, float sx, float sy);
PlayerUI *nemoui_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio);

#ifdef __cplusplus
}
#endif

#endif
