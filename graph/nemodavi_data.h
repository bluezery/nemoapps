#ifndef __NEMODAVI_DATA_H__
#define __NEMODAVI_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <nemolist.h>
#include <nemolistener.h>

typedef enum {
	NEMODAVI_DATUM_TYPE_INT = 0,
	NEMODAVI_DATUM_TYPE_DOUBLE,
	NEMODAVI_DATUM_TYPE_PTR,
	NEMODAVI_DATUM_TYPE_LAST 
} NemoDaviDatumType;

struct nemodavi_datum {
	int type;

	int __datum_i__;
	double __datum_d__;
	void *__datum_p__;

	void *paramptr;

	struct nemosignal signal;
};

struct nemodavi_data {
	uint32_t ref;
	uint32_t data_count;
	struct nemodavi_datum **datums;
};

extern struct nemodavi_data* nemodavi_data_create();
extern int nemodavi_data_destory(struct nemodavi_data *data);
extern int nemodavi_data_bind_ptr_array(struct nemodavi_data *data, void **datums, uint32_t count, size_t onesize);
extern int nemodavi_data_bind_int_array(struct nemodavi_data *data, int *datums, uint32_t count);
extern int nemodavi_data_bind_double_array(struct nemodavi_data *data, double *datums, uint32_t count);
extern int nemodavi_data_bind_ptr_one(struct nemodavi_data *data, void *datum, uint32_t index);
extern int nemodavi_data_bind_int_one(struct nemodavi_data *data, int datum, uint32_t index);
extern int nemodavi_data_bind_double_one(struct nemodavi_data *data, double datum, uint32_t index);
extern int nemodavi_data_attach_datum_listener(struct nemodavi_datum *datum, struct nemolistener *listner);
extern int nemodavi_data_detach_datum_listener(struct nemodavi_datum *datum, struct nemolistener *listner);
extern int nemodavi_data_reference(struct nemodavi_data *data);
extern int nemodavi_data_unreference(struct nemodavi_data *data);
extern int nemodavi_data_emit_all_signals(struct nemodavi_data *data);

extern void* nemodavi_data_get_bind_datum(struct nemodavi_datum *datum);
extern void nemodavi_data_free_datum_paramptr(struct nemodavi_datum *datum);

static inline void* nemodavi_data_get_ptr_one(struct nemodavi_datum *datum)
{
	return datum->__datum_p__;
}

static inline int nemodavi_data_get_int_one(struct nemodavi_datum *datum)
{
	return datum->__datum_i__;
}

static inline double nemodavi_data_get_double_one(struct nemodavi_datum *datum)
{
	return datum->__datum_d__;
}

static inline struct nemolist* nemodavi_data_get_datum_listener_list(struct nemodavi_datum *datum)
{
	return &datum->signal.listener_list;
}

static inline struct nemosignal* nemodavi_data_get_signal(struct nemodavi_data *data, uint32_t index)
{
	return &data->datums[index]->signal;
}

static inline void nemodavi_data_set_ptr_datum(struct nemodavi_data *data, void *datum, uint32_t index)
{
	data->datums[index]->__datum_p__ = datum;
}

static inline void nemodavi_data_set_int_datum(struct nemodavi_data *data, int datum, uint32_t index)
{
	data->datums[index]->__datum_i__ = datum;
}

static inline void nemodavi_data_set_double_datum(struct nemodavi_data *data, double datum, uint32_t index)
{
	data->datums[index]->__datum_d__ = datum;
}

#ifdef __cplusplus
}
#endif

#endif
