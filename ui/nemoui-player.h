#ifndef __NEMOMISC_PLAYER_H__
#define __NEMOMISC_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PlayerUI PlayerUI;

void nemoui_player_prepare(PlayerUI *ui);
void nemoui_player_play(PlayerUI *ui);
void nemoui_player_stop(PlayerUI *ui);
void nemoui_player_seek(PlayerUI *ui, double time);
void nemoui_player_append_callback(PlayerUI *ui, const char *id, NemoWidget_Func func, void *userdata);
void nemoui_player_redraw(PlayerUI *ui);
bool nemoui_player_is_prepared(PlayerUI *ui);
const char *nemoui_player_get_uri(PlayerUI *ui);
double nemoui_player_get_cts(PlayerUI *ui);
double nemoui_player_get_duration(PlayerUI *ui);
bool nemoui_player_is_playing(PlayerUI *ui);
void nemoui_player_set_repeat(PlayerUI *ui, int repeat);
void nemoui_player_show(PlayerUI *ui, uint32_t easetype, int duration, int delay);
void nemoui_player_hide(PlayerUI *ui, uint32_t easetype, int duration, int delay);
void nemoui_player_destroy(PlayerUI *ui);
void nemoui_player_scale(PlayerUI *ui, uint32_t easetype, int duration, int delay, float sx, float sy);
void nemoui_player_rotate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float ro);
void nemoui_player_translate(PlayerUI *ui, uint32_t easetype, int duration, int delay, float x, float y);
void nemoui_player_above(PlayerUI *ui, NemoWidget *above);
void nemoui_player_below(PlayerUI *ui, NemoWidget *below);
PlayerUI *nemoui_player_create(NemoWidget *parent, int cw, int ch, const char *path, bool enable_audio, int num_threads, bool no_drop);

void nemoui_player_revoke_translate(PlayerUI *ui);
void nemoui_player_revoke_scale(PlayerUI *ui);
void nemoui_player_revoke_rotate(PlayerUI *ui);
void nemoui_player_revoke_alpha(PlayerUI *ui);

#ifdef __cplusplus
}
#endif

#endif
