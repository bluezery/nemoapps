#ifndef __XEMOUTIL_CON_H__
#define __XEMOUTIL_CON_H__

#include <stdbool.h>
#include <nemotool.h>

typedef struct _Con Con;

typedef void (*Con_Data_Callback)(Con *con, char *data, size_t size, void *userdata);
typedef void (*Con_End_Callback)(Con *con, bool success, char *data, size_t size, void *userdata);

#ifdef __cplusplus
extern "C" {
#endif

bool con_init();
void con_shutdown();
void con_destroy(Con *con);
void con_run(Con *con);
Con *con_create(struct nemotool *tool);
const char *con_get_url(Con *con);
void con_set_url(Con *con, const char *url);
void con_set_file(Con *con, const char *path);
void con_set_end_callback(Con *con, Con_End_Callback callback, void *userdata);
void con_set_data_callback(Con *con, Con_Data_Callback callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
