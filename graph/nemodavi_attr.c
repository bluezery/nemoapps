#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>
#include <nemoshow.h>
#include <showone.h>
#include <showhelper.h>
#include <showcolor.h>
#include <showitem.h>

#include "nemodavi.h"
#include "nemodavi_attr.h"

static void set_cattr_one(struct showone *one, char *attr, uint32_t value)
{
	double r, g, b, a;

	if (!one || !attr) {
		return;
	}

	a = (double) NEMOSHOW_COLOR_UINT32_A(value);
	r = (double) NEMOSHOW_COLOR_UINT32_R(value);
	g = (double) NEMOSHOW_COLOR_UINT32_G(value);
	b = (double) NEMOSHOW_COLOR_UINT32_B(value);

	if (!strcmp(attr, "fill")) {
		nemoshow_item_set_fill_color(one, r, g, b, a);
	} else if (!strcmp(attr, "stroke")) {
		nemoshow_item_set_stroke_color(one, r, g, b, a);
	} else {
		printf("not matched attr: %s\n", attr);
	}
}

static void set_dattr_one(struct showone *one, char *attr, double value)
{
	double cp;

	if (!one || !attr) {
		return;
	}

	if (!strcmp(attr, "alpha")) {
		nemoshow_item_set_alpha(one, value);
	} else if (!strcmp(attr, "ax")) {
		cp = NEMOSHOW_ITEM_AT(one, ay);
		nemoshow_item_set_anchor(one, value, cp);
	} else if (!strcmp(attr, "ay")) {
		cp = NEMOSHOW_ITEM_AT(one, ax);
		nemoshow_item_set_anchor(one, cp, value);
	} else if (!strcmp(attr, "from")) {
		nemoshow_item_set_from(one, value);
	} else if (!strcmp(attr, "font-size")) {
		nemoshow_item_set_fontsize(one, value);
	} else if (!strcmp(attr, "height")) {
		nemoshow_item_set_height(one, value);
	} else if (!strcmp(attr, "px")) {
		cp = NEMOSHOW_ITEM_AT(one, py);
		nemoshow_item_pivot(one, value, cp);
	} else if (!strcmp(attr, "py")) {
		cp = NEMOSHOW_ITEM_AT(one, px);
		nemoshow_item_pivot(one, cp, value);
	} else if (!strcmp(attr, "r")) {
		nemoshow_item_set_r(one, value);
	} else if (!strcmp(attr, "ro")) {
		nemoshow_item_rotate(one, value);
	} else if (!strcmp(attr, "ox")) {
		nemoshow_item_set_roundx(one, value);
	} else if (!strcmp(attr, "oy")) {
		nemoshow_item_set_roundy(one, value);
	} else if (!strcmp(attr, "stroke-width")) {
		nemoshow_item_set_stroke_width(one, value);
	} else if (!strcmp(attr, "stroke-cap")) {
		nemoshow_item_set_stroke_cap(one, (int) value);
	} else if (!strcmp(attr, "sx")) {
		cp = NEMOSHOW_ITEM_AT(one, sy);
		nemoshow_item_scale(one, value, cp);
	} else if (!strcmp(attr, "sy")) {
		cp = NEMOSHOW_ITEM_AT(one, sx);
		nemoshow_item_scale(one, cp, value);
	} else if (!strcmp(attr, "spacingmul")) {
		cp = NEMOSHOW_ITEM_AT(one, spacingadd);
		nemoshow_item_set_spacing(one, value, cp);
	} else if (!strcmp(attr, "spacingadd")) {
		cp = NEMOSHOW_ITEM_AT(one, spacingmul);
		nemoshow_item_set_spacing(one, cp, value);
	} else if (!strcmp(attr, "to")) {
		nemoshow_item_set_to(one, value);
	} else if (!strcmp(attr, "tx")) {
		cp = NEMOSHOW_ITEM_AT(one, ty);
		nemoshow_item_translate(one, value, cp);
	} else if (!strcmp(attr, "ty")) {
		cp = NEMOSHOW_ITEM_AT(one, tx);
		nemoshow_item_translate(one, cp, value);
	} else if (!strcmp(attr, "width")) {
		nemoshow_item_set_width(one, value);
	}
}

static void set_sattr_one(struct showone *one, char *attr, char *value)
{
	if (!one || !attr || !value) {
		return;
	}
	
	if (!strcmp(attr, "image-uri")) {
		nemoshow_item_set_uri(one, value);
	} else if (!strcmp(attr, "text")) {
		nemoshow_item_set_text(one, value);
	}
}

static void set_oattr_one(struct showone *one, char *attr, struct showone *value)
{
	if (!one || !attr || !value) {
		return;
	}

	if (!strcmp(attr, "clip")) {
		nemoshow_item_set_clip(one, value);
	} else if (!strcmp(attr, "filter")) {
		nemoshow_item_set_filter(one, value);
	} else if (!strcmp(attr, "font")) {
		nemoshow_item_set_font(one, value);
	} else if (!strcmp(attr, "path")) {
		nemoshow_item_set_path(one, value);
	} else if (!strcmp(attr, "shader")) {
		nemoshow_item_set_shader(one, value);
	}
}


int nemodavi_set_cattr(struct nemodavi_selector *sel, char *attr, uint32_t value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		one = nemodavi_selector_get_showitem(sel, i);
		set_cattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_dattr(struct nemodavi_selector *sel, char *attr, double value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		one = nemodavi_selector_get_showitem(sel, i);
		set_dattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_sattr(struct nemodavi_selector *sel, char *attr, char *value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr || !value) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		one = nemodavi_selector_get_showitem(sel, i);
		set_sattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_oattr(struct nemodavi_selector *sel, char *attr, struct showone *value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr || !value) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		one = nemodavi_selector_get_showitem(sel, i);
		set_oattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_cattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, uint32_t value)
{
	struct showone *one;

	if (!sel || !attr) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	one = nemodavi_selector_get_showitem(sel, index);
	set_cattr_one(one, attr, value);

	return 0;
}

int nemodavi_set_dattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, double value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	one = nemodavi_selector_get_showitem(sel, index);
	set_dattr_one(one, attr, value);

	return 0;
}

int nemodavi_set_sattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, char *value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr || !value) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	one = nemodavi_selector_get_showitem(sel, index);
	set_sattr_one(one, attr, value);

	return 0;
}

int nemodavi_set_oattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index, struct showone *value)
{
	uint32_t i;
	struct showone *one;

	if (!sel || !attr || !value) {
		return -1;
	}

	if (index < 0 || index >= sel->count) {
		return -1;
	}

	one = nemodavi_selector_get_showitem(sel, index);
	set_oattr_one(one, attr, value);

	return 0;
}


int nemodavi_set_cattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_cattr_handler_t handler, void *userdata)
{
	uint32_t i, value;
	void *bound;
	struct nemodavi_datum *datum;
	struct showone *one;

	if (!sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		one = nemodavi_selector_get_showitem(sel, i);
		set_cattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_dattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_dattr_handler_t handler, void *userdata)
{
	uint32_t i;
	double value;
	void *bound;
	struct nemodavi_datum *datum;
	struct showone *one;

	if (!sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		one = nemodavi_selector_get_showitem(sel, i);
		set_dattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_sattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_sattr_handler_t handler, void *userdata)
{
	uint32_t i;
	char *value;
	void *bound;
	struct nemodavi_datum *datum;
	struct showone *one;

	if (!sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		one = nemodavi_selector_get_showitem(sel, i);
		set_sattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_oattr_handler(struct nemodavi_selector *sel, char *attr, nemodavi_set_oattr_handler_t handler, void *userdata)
{
	int i;
	struct showone *value;
	void *bound;
	struct nemodavi_datum *datum;
	struct showone *one;

	if (!sel || !attr || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		datum = nemodavi_selector_get_datum(sel, i);

		bound = nemodavi_data_get_bind_datum(datum);
		value = handler(i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

		one = nemodavi_selector_get_showitem(sel, i);
		set_oattr_one(one, attr, value);
	}

	return 0;
}

int nemodavi_set_attr_handler(struct nemodavi_selector *sel, nemodavi_set_attr_handler_t handler, void *userdata)
{
	int i;
	struct showone *value;
	void *bound;
	struct nemodavi_datum *datum;
	struct showone *one;

	if (!sel || !handler) {
		return -1;
	}

	for (i = 0; i < sel->count; i++) {
		one = nemodavi_selector_get_showitem(sel, i);
		datum = nemodavi_selector_get_datum(sel, i);
		bound = nemodavi_data_get_bind_datum(datum);

		handler(one, i, bound, userdata);
		nemodavi_data_free_datum_paramptr(datum);

	}

	return 0;
}

uint32_t nemodavi_get_cattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index)
{
	uint32_t value;
	struct showone *one;

	if (index < 0 || index >= sel->count) {
		return 0;
	}

	one = nemodavi_selector_get_showitem(sel, index);

	if (!strcmp(attr, "fill")) {
		value = NEMOSHOW_COLOR_TO_UINT32(NEMOSHOW_ITEM_AT(one, fills)); 
		printf("%s) %x\n", __FUNCTION__, value);
	} else if (!strcmp(attr, "stroke")) {
		value = NEMOSHOW_COLOR_TO_UINT32(NEMOSHOW_ITEM_AT(one, strokes)); 
	} else {
		value = 0;
	}

	return value;
}

double nemodavi_get_dattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index)
{
	double value;
	struct showone *one;

	if (index < 0 || index >= sel->count) {
		return 0;
	}

	one = nemodavi_selector_get_showitem(sel, index);

	if (!strcmp(attr, "alpha")) {
		value = NEMOSHOW_ITEM_AT(one, alpha);
	} else if (!strcmp(attr, "ax")) {
		value = NEMOSHOW_ITEM_AT(one, ax);
	} else if (!strcmp(attr, "ay")) {
		value = NEMOSHOW_ITEM_AT(one, ay);
	} else if (!strcmp(attr, "from")) {
		value = NEMOSHOW_ITEM_AT(one, from);
	} else if (!strcmp(attr, "font-size")) {
		value = NEMOSHOW_ITEM_AT(one, fontsize);
	} else if (!strcmp(attr, "height")) {
		value = NEMOSHOW_ITEM_AT(one, height);
	} else if (!strcmp(attr, "px")) {
		value = NEMOSHOW_ITEM_AT(one, px);
	} else if (!strcmp(attr, "py")) {
		value = NEMOSHOW_ITEM_AT(one, py);
	} else if (!strcmp(attr, "r")) {
		value = NEMOSHOW_ITEM_AT(one, r);
	} else if (!strcmp(attr, "ro")) {
		value = NEMOSHOW_ITEM_AT(one, ro);
	} else if (!strcmp(attr, "ox")) {
		value = NEMOSHOW_ITEM_AT(one, ox);
	} else if (!strcmp(attr, "oy")) {
		value = NEMOSHOW_ITEM_AT(one, oy);
	} else if (!strcmp(attr, "stroke-width")) {
		value = NEMOSHOW_ITEM_AT(one, stroke_width);
	} else if (!strcmp(attr, "sx")) {
		value = NEMOSHOW_ITEM_AT(one, sx);
	} else if (!strcmp(attr, "sy")) {
		value = NEMOSHOW_ITEM_AT(one, sy);
	} else if (!strcmp(attr, "spacingmul")) {
		value = NEMOSHOW_ITEM_AT(one, spacingmul);
	} else if (!strcmp(attr, "spacingadd")) {
		value = NEMOSHOW_ITEM_AT(one, spacingadd);
	} else if (!strcmp(attr, "to")) {
		value = NEMOSHOW_ITEM_AT(one, to);
	} else if (!strcmp(attr, "tx")) {
		value = NEMOSHOW_ITEM_AT(one, tx);
	} else if (!strcmp(attr, "ty")) {
		value = NEMOSHOW_ITEM_AT(one, ty);
	} else if (!strcmp(attr, "width")) {
		value = NEMOSHOW_ITEM_AT(one, width);
	} else if (!strcmp(attr, "x")) {
		value = NEMOSHOW_ITEM_AT(one, x);
	} else if (!strcmp(attr, "y")) {
		value = NEMOSHOW_ITEM_AT(one, y);
	} else {
		value = 0.0;
	}

	return value;
}

char* nemodavi_get_sattr_by_index(struct nemodavi_selector *sel, char *attr, uint32_t index)
{
	char *value;
	struct showone *one;

	if (index < 0 || index >= sel->count) {
		return NULL;
	}

	one = nemodavi_selector_get_showitem(sel, index);
	if (!strcmp(attr, "image-uri")) {
		value = NEMOSHOW_ITEM_AT(one, uri);
	} else if (!strcmp(attr, "text")) {
		value = NEMOSHOW_ITEM_AT(one, text);
	} else {
		value = NULL;
	}	

	return value;
}
