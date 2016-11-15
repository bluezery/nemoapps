#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonavi.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#include "browser.h"

void nemonavi_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct appcontext *context = (struct appcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	nemonavi_set_size(context->navi,
			nemoshow_canvas_get_viewport_width(context->webview),
			nemoshow_canvas_get_viewport_height(context->webview));

	context->ratio = (double) width / (double) context->winw;

	nemonavi_do_message();
}

void nemonavi_dispatch_paint(struct nemonavi *navi, int type, const void *buffer, int width, int height, int dx, int dy, int dw, int dh)
{
	struct appcontext *context = (struct appcontext *)nemonavi_get_userdata(navi);

	if (type == NEMONAVI_PAINT_VIEW_TYPE) {
		GLuint texture = nemoshow_canvas_get_texture(context->webview);

		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

		if (dx == 0 && dy == 0 && dw == width && dh == height) {
			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

			nemoshow_canvas_damage_all(context->webview);
		} else {
			float x0, y0;
			float x1, y1;

			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, dx);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, dy);

			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, dw, dh, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

			nemoshow_canvas_transform_from_viewport(context->webview, dx, dy, &x0, &y0);
			nemoshow_canvas_transform_from_viewport(context->webview, dx + dw, dy + dh, &x1, &y1);

			nemoshow_canvas_damage(context->webview, x0, y0, x1 - x0, y1 - y0);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		nemoshow_dispatch_frame(context->show);
	} else if (type == NEMONAVI_PAINT_POPUP_TYPE) {
		GLuint texture = nemoshow_canvas_get_texture(context->webpopup);

		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

		if (dx == 0 && dy == 0 && dw == width && dh == height) {
			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

			nemoshow_canvas_damage_all(context->webpopup);
		} else {
			float x0, y0;
			float x1, y1;

			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, dx);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, dy);

			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, dw, dh, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

			nemoshow_canvas_transform_from_viewport(context->webpopup, dx, dy, &x0, &y0);
			nemoshow_canvas_transform_from_viewport(context->webpopup, dx + dw, dy + dh, &x1, &y1);

			nemoshow_canvas_damage(context->webpopup, x0, y0, x1 - x0, y1 - y0);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		nemoshow_dispatch_frame(context->show);
	}
}

void nemonavi_dispatch_popup_show(struct nemonavi *navi, int show)
{
	struct appcontext *context = (struct appcontext *)nemonavi_get_userdata(navi);

	if (show != 0)
		nemoshow_one_attach(context->scene, context->webpopup);
	else
		nemoshow_one_detach(context->webpopup);
}

void nemonavi_dispatch_popup_rect(struct nemonavi *navi, int x, int y, int width, int height)
{
	struct appcontext *context = (struct appcontext *)nemonavi_get_userdata(navi);
	float x0, y0;
	float x1, y1;

	nemoshow_transform_from_viewport(context->show, x, y, &x0, &y0);
	nemoshow_transform_from_viewport(context->show, x + width, y + height, &x1, &y1);

	nemoshow_canvas_translate(context->webpopup, x0, y0);
	nemoshow_canvas_set_size(context->webpopup, x1 - x0, y1 - y0);
}

void nemonavi_dispatch_key_event(struct nemonavi *navi, uint32_t code, int focus_on_editable_field)
{
	struct appcontext *context = (struct appcontext *)nemonavi_get_userdata(navi);

	if (focus_on_editable_field == 0) {
		if (code == 8)
			nemonavi_go_back(context->navi);
		else if (code == 171)
			nemonavi_set_zoomlevel(context->navi,
					nemonavi_get_zoomlevel(context->navi) + 0.5f);
		else if (code == 173)
			nemonavi_set_zoomlevel(context->navi,
					nemonavi_get_zoomlevel(context->navi) - 0.5f);
	}
}

void nemonavi_dispatch_loading_state(struct nemonavi *navi, int is_loading, int can_go_back, int can_go_forward)
{
	printf("is_loading: %d, can_go_back: %d, can_go_forward: %d\n", is_loading, can_go_back, can_go_forward);

	struct appcontext *context = nemonavi_get_userdata(navi);
	struct nemodavi_selector *sel;

	if (can_go_back) {
		sel = nemodavi_selector_get(context->davi, "backbtn");
		nemodavi_set_cattr(sel, "fill", 0xFF444444);
	} else {
		sel = nemodavi_selector_get(context->davi, "backbtn");
		nemodavi_set_cattr(sel, "fill", 0xFFDDDDDD);
	}

	if (can_go_forward) {
		sel = nemodavi_selector_get(context->davi, "fwdbtn");
		nemodavi_set_cattr(sel, "fill", 0xFF444444);
	} else {
		sel = nemodavi_selector_get(context->davi, "fwdbtn");
		nemodavi_set_cattr(sel, "fill", 0xFFDDDDDD);
	}
}

int nemonavi_dispatch_touch_grab(struct nemoshow *show, struct showgrab *grab, struct showevent *event)
{
	struct appcontext *context = (struct appcontext *)nemoshow_grab_get_userdata(grab);
	float x, y;

	if (nemoshow_event_is_down(show, event)) {
		/*
		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
		*/

		int id = nemonavi_get_touchid_empty(context->navi);

		nemonavi_set_touchid(context->navi, id);

		nemoshow_set_keyboard_focus(show, context->webview);

		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemoshow_grab_set_tag(grab, id + 1);

		nemonavi_send_touch_down_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio, id, nemoshow_event_get_time(event) / 1000.0f);
	} else if (nemoshow_event_is_motion(show, event)) {
		int id = nemoshow_grab_get_tag(grab);

		if (id > 0) {
			nemoshow_transform_to_viewport(show,
					nemoshow_event_get_x(event),
					nemoshow_event_get_y(event),
					&x, &y);

			nemonavi_send_touch_motion_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio, id - 1, nemoshow_event_get_time(event) / 1000.0f);

			nemonavi_do_message();
		}
	} else if (nemoshow_event_is_up(show, event)) {
		int id = nemoshow_grab_get_tag(grab);

		if (id > 0) {
			nemoshow_transform_to_viewport(show,
					nemoshow_event_get_x(event),
					nemoshow_event_get_y(event),
					&x, &y);

			nemonavi_send_touch_up_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio, id - 1, nemoshow_event_get_time(event) / 1000.0f);

			nemonavi_put_touchid(context->navi, id - 1);

			nemonavi_do_message();
		}

		nemoshow_grab_destroy(grab);

		return 0;
	} else if (nemoshow_event_is_cancel(show, event)) {
		int id = nemoshow_grab_get_tag(grab);

		if (id > 0) {
			nemoshow_transform_to_viewport(show,
					nemoshow_event_get_x(event),
					nemoshow_event_get_y(event),
					&x, &y);

			nemonavi_send_touch_cancel_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio, id - 1, nemoshow_event_get_time(event) / 1000.0f);

			nemonavi_put_touchid(context->navi, id - 1);

			nemonavi_do_message();
		}

		nemoshow_grab_destroy(grab);

		return 0;
	}

	return 1;
}

void nemonavi_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{

	struct appcontext *context = (struct appcontext *)nemoshow_get_userdata(show);
	float x, y;

		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);


	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}

		if (nemoshow_event_is_more_taps(show, event, 2)) {
			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}

	if (nemoshow_event_is_pointer_enter(show, event)) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_enter_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio);
	} else if (nemoshow_event_is_pointer_leave(show, event)) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_leave_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio);
	} else if (nemoshow_event_is_pointer_button_down(show, event, 0)) {
		nemoshow_set_keyboard_focus(show, canvas);

		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_down_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio,
				nemoshow_event_get_value(event));
	} else if (nemoshow_event_is_pointer_button_up(show, event, 0)) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_up_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio,
				nemoshow_event_get_value(event));
	} else if (nemoshow_event_is_pointer_motion(show, event)) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_motion_event(context->navi, x - BEZEL_W * context->ratio, y - (CHROME_H + BEZEL_W) * context->ratio);
	} else if (nemoshow_event_is_pointer_axis(show, event)) {
		if (nemoshow_event_get_axis(event) == NEMO_POINTER_AXIS_ROTATE_X) {
			nemonavi_send_pointer_wheel_event(context->navi, 0.0f, nemoshow_event_get_rotate(event) < 0.0f ? 40.0f : -40.0f);
		} else {
			nemonavi_send_pointer_wheel_event(context->navi, nemoshow_event_get_rotate(event) < 0.0f ? 40.0f : -40.0f, 0.0f);
		}
	}


	if (nemoshow_event_is_keyboard_down(show, event)) {
	} else if (nemoshow_event_is_keyboard_up(show, event)) {
		if (nemoshow_event_get_value(event) == KEY_HANGEUL) {
			if (context->hangul == NULL) {
				context->hangul = nemohangul_create();
			} else {
				nemohangul_destroy(context->hangul);
				context->hangul = NULL;
			}
		}
	}

	if (nemoshow_event_is_keyboard_layout(show, event)) {
		const char *layout = nemoshow_event_get_name(event);

		if (strcmp(layout, "kor") == 0) {
			if (context->hangul == NULL)
				context->hangul = nemohangul_create();
		} else {
			if (context->hangul != NULL) {
				nemohangul_destroy(context->hangul);
				context->hangul = NULL;
			}
		}
	}

	if (context->hangul == NULL) {
		if (nemoshow_event_is_keyboard_down(show, event)) {
			nemonavi_send_keyboard_down_event(context->navi,
					nemoshow_event_get_value(event),
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)),
					nemotool_get_modifiers(context->tool));
		} else if (nemoshow_event_is_keyboard_up(show, event)) {
			nemonavi_send_keyboard_up_event(context->navi,
					nemoshow_event_get_value(event),
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)),
					nemotool_get_modifiers(context->tool));
		}
	} else {
		if (nemoshow_event_is_keyboard_down(show, event)) {
		} else if (nemoshow_event_is_keyboard_up(show, event)) {
			uint32_t code = nemoshow_event_get_value(event);

			if (keycode_is_normal(code) != 0) {
				uint32_t sym = nemotool_get_keysym(context->tool, code);
				const uint32_t *ucs;

				if (keycode_is_alphabet(code) != 0) {
					if (nemohangul_is_empty(context->hangul) == 0) {
						nemonavi_send_keyboard_down_event(context->navi, KEY_BACKSPACE, 0, nemotool_get_modifiers(context->tool));
						nemonavi_send_keyboard_up_event(context->navi, KEY_BACKSPACE, 0, nemotool_get_modifiers(context->tool));
					}

					nemohangul_process(context->hangul, sym);
				} else if (keycode_is_space(code) != 0) {
					nemohangul_reset(context->hangul);
				} else if (keycode_is_backspace(code) != 0) {
					nemonavi_send_keyboard_down_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));
					nemonavi_send_keyboard_up_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));

					nemohangul_backspace(context->hangul);
				} else {
					nemonavi_send_keyboard_down_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));
					nemonavi_send_keyboard_up_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));

					nemohangul_reset(context->hangul);
				}

				ucs = nemohangul_get_commit_string(context->hangul);
				if (ucs[0] != '\0')
					nemonavi_send_keyboard_up_event(context->navi, KEY_A, ucs[0], nemotool_get_modifiers(context->tool));

				ucs = nemohangul_get_preedit_string(context->hangul);
				if (ucs[0] != '\0')
					nemonavi_send_keyboard_up_event(context->navi, KEY_A, ucs[0], nemotool_get_modifiers(context->tool));

				if (keycode_is_space(code) != 0) {
					nemonavi_send_keyboard_down_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));
					nemonavi_send_keyboard_up_event(context->navi, code, sym, nemotool_get_modifiers(context->tool));
				}
			}
		}
	}

	if (nemoshow_event_is_touch_down(show, event)) {
		struct showgrab *grab;

		grab = nemoshow_grab_create(show, event, nemonavi_dispatch_touch_grab);
		nemoshow_grab_set_userdata(grab, context);
		nemoshow_dispatch_grab(show, event);
	}

	if (nemoshow_event_is_single_click(show, event)) {
		float x, y;
		x = nemoshow_event_get_x(event) - BEZEL_W * context->ratio;
		y = nemoshow_event_get_y(event) - (CHROME_H + BEZEL_W) * context->ratio;

		nemoshow_transform_to_viewport(show, x, y, &context->webview_vx, &context->webview_vy);
	}

	nemonavi_do_message();
}

void nemonavi_dispatch_focus_change(struct nemonavi *navi, int is_editable)
{
	struct appcontext *context = (struct appcontext *)nemonavi_get_userdata(navi);
}

void nemonavi_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct appcontext *ctx = (struct appcontext *)data;

	nemonavi_do_message();
	nemotimer_set_timeout(timer, NEMONAVI_MESSAGE_TIMEOUT);
}
