#include <nemoshow.h>
#include <showone.h>
#include <showitem.h>
#include <showcanvas.h>

#include "mapinfo_util.h"

double mapinfo_util_get_runner_start_angle(uint32_t num, uint32_t nth)
{
	double angle = 0.0;

	switch (num) {
		case 2:
			angle = nth * 180.0;
			break;
		case 3:
			angle = 30 + (nth * 120.0);
			break;
		default:
			break;
	}

	return angle;
}

void mapinfo_util_move_item_above(struct showone *canvas, struct showone *item)
{
	nemoshow_one_detach(canvas, item);
	nemoshow_one_attach(canvas, item);
}

struct showone *mapinfo_util_create_image(struct showone* canvas, double w, double h, char *uri)
{
    struct showone *one;
    one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
    nemoshow_attach_one(NEMOSHOW_CANVAS_AT(canvas, show), one);
    nemoshow_one_attach(canvas, one);
    nemoshow_item_set_tsr(one);

    nemoshow_item_set_width(one, w);
    nemoshow_item_set_height(one, h);
		if (uri) {
	nemoshow_item_set_uri(one, uri);
		}

    return one;
}

struct showone *mapinfo_util_create_clip(struct showone* canvas, double w, double h, double r)
{
	struct showone *one;
	float x, y;

	x = y = 0;

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(NEMOSHOW_CANVAS_AT(canvas, show), one);
	nemoshow_one_attach(canvas, one);

	nemoshow_item_path_moveto(one, x + r, y, 1, 0);
	nemoshow_item_path_lineto(one, x + w - r, y, 1, 0);
	nemoshow_item_path_cubicto(one, x + w, y, x + w, y, x + w, y + r, 1, 0);
	nemoshow_item_path_lineto(one, x + w, y + h - r, 1, 0);
	nemoshow_item_path_cubicto(one, x + w, y + h, x + w, y + h, x + w - r, y + h, 1, 0);
	nemoshow_item_path_lineto(one, x + r, y + h, 1, 0);
	nemoshow_item_path_cubicto(one, x, y + h, x, y + h, x, y + h - r, 1, 0);
	nemoshow_item_path_lineto(one, x, y + r, 1, 0);
	nemoshow_item_path_cubicto(one, x, y, x, y, x + r, y, 1, 0);
	nemoshow_item_path_close(one, 1, 0);

	return one;
}

struct showone* mapinfo_util_create_transition()
{
	struct showone *frame;
	frame	= nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);
	return frame;
}

struct showtransition* mapinfo_util_execute_transition(struct nemoshow* show, struct showone* frame, int easytype, uint32_t duration, uint32_t delay, int repeat)
{
	struct showone *sequence;
	struct showone *ease;
	struct showtransition *trans;
	sequence = nemoshow_sequence_create_easy(show, frame, NULL);

	// transition -> seqences -> frames -> sets
	ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, easytype);
	trans = nemoshow_transition_create(ease, duration, delay);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);
	nemoshow_transition_set_repeat(trans, repeat);

    nemoshow_dispatch_frame(show);
	return trans;
}

struct showone* mapinfo_util_create_circle_path(double r, int isstroke, int isfill)
{
	double x, y, c;
	struct showone *path = nemoshow_item_create(NEMOSHOW_PATH_TYPE);
  nemoshow_item_set_tsr(path);

	x = y = 0.0;
	c = 0.551915024494;

	nemoshow_item_path_clear(path);
	nemoshow_item_path_moveto(path, x, y + r, isstroke, isfill);
	nemoshow_item_path_cubicto(path, (x + r) * c, y + r, x + r, (y + r) * c, x + r, y, isstroke, isfill);
	nemoshow_item_path_cubicto(path, x + r, -1 * (x + r) * c, (x + r) * c, - (y + r), x, -1 * (y + r), isstroke, isfill);
	nemoshow_item_path_cubicto(path, -1 * (x + r) * c, -1 * (y + r), -1 * (x + r), -1 * (y + r) * c, -1 * (x + r), y, isstroke, isfill);
	nemoshow_item_path_cubicto(path, -1 * (x + r), (y + r) * c, -1 * (x + r) * c, y + r, x, y + r, isstroke, isfill);
	nemoshow_item_path_close(path, isstroke, isfill);

	return path;
}
