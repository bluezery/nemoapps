#ifndef __UTIL_DBUS_H__
#define __UTIL_DBUS_H__

#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <magic.h>

#include <dbus/dbus.h>

#include <nemotool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NemoDbus NemoDbus;
typedef void (*NemoDbusCallback)(NemoDbus *nemodubs, DBusError *err, DBusMessage *msg, void *data);

struct _NemoDbus
{
    struct nemotool *tool;
    DBusConnection *dbuscon;
    DBusWatch *watch;
};

void nemodbus_util_dump_message(DBusMessage *msg);
NemoDbus *nemodbus_create(struct nemotool *tool);
void nemodbus_destroy(NemoDbus *nemodbus);
bool nemodbus_send_message(NemoDbus *nemodbus, DBusMessage *msg, NemoDbusCallback callback, void *userdata);
bool nemodbus_call_method(NemoDbus *nemodbus, const char *target, const char *path, const char *iface, const char *method, NemoDbusCallback callback, void *userdata);
void nemodbus_set_dict_entry(DBusMessageIter *iter, const char *property, int type, const void *value);

#ifdef __cplusplus
}
#endif

#endif
