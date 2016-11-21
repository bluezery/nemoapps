#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <dbus/dbus.h>

#include "nemoutil.h"
#include "nemoutil-dbus.h"

/********************/
/*** Introspect
 dbus-send --system --dest=net.connman --print-reply / org.freedesktop.DBus.Introspectable.Introspect
 ***/


static void _dump_iter(DBusMessageIter *iter)
{
    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(iter)) {
    } else if (DBUS_TYPE_BYTE == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_INT16 == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(iter)) {
        dbus_bool_t val;
        dbus_message_iter_get_basic(iter, &val);
    } else if (DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(iter)) {
        int64_t val;
        dbus_message_iter_get_basic(iter, &val);
    } else if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(iter)) {
        double val;
        dbus_message_iter_get_basic(iter, &val);
    } else if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(iter)) {
        char *val;
        dbus_message_iter_get_basic(iter, &val);
    } else if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(iter)) {
        DBusMessageIter iter1;
        dbus_message_iter_recurse(iter, &iter1);
        _dump_iter(&iter1);
        char *val;
        dbus_message_iter_get_basic(iter, &val);
    } else if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(iter) ||
            DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(iter)) {
        DBusMessageIter iter1;
        dbus_message_iter_recurse(iter, &iter1);
        do {
            _dump_iter(&iter1);
        } while (dbus_message_iter_next(&iter1));
    } else if (DBUS_TYPE_INVALID == dbus_message_iter_get_arg_type(iter)) {
        ERR("DBus type invalid");
    } else {
        ERR("Not supported DBus type");
    }
}

void nemodbus_util_dump_message(DBusMessage *msg)
{
    RET_IF(!msg);
    const char *method = dbus_message_get_member(msg);
    const char *iface = dbus_message_get_interface(msg);
    const char *path = dbus_message_get_path(msg);
    const char *sender = dbus_message_get_sender(msg);
    const char *signature = dbus_message_get_signature(msg);
    ERR("method: %s, iface: %s, path: %s, sender: %s, signature: %s",
            method, iface, path, sender, signature);

    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        ERR("dbus message has no arguments");
        return;
    }

    _dump_iter(&iter);
}

static void _nemodbus_event(void *data, const char *events)
{
    NemoDbus *nemodbus = data;
    uint32_t flags = 0x0;

    if (strchr(events, 'r')) flags |= DBUS_WATCH_READABLE;
    if (strchr(events, 'u')) flags |= DBUS_WATCH_HANGUP;
    if (strchr(events, 'e')) flags |= DBUS_WATCH_ERROR;

    if (!dbus_watch_handle(nemodbus->watch, flags)) {
        ERR("dbus_watch_handle failed");
    }

    while (dbus_connection_dispatch(nemodbus->dbuscon)
                == DBUS_DISPATCH_DATA_REMAINS);
}

static dbus_bool_t _dbus_add_watch(DBusWatch *watch, void *data)
{
    NemoDbus *nemodbus = data;
    nemodbus->watch = watch;

    if (dbus_watch_get_enabled(watch)) {
        char events[10];
        snprintf(events, 10, "ue");
        uint32_t flags = dbus_watch_get_flags(watch);

        if (flags & DBUS_WATCH_READABLE) {
            strcat(events, "r");
        }

        if (nemotool_watch_source(nemodbus->tool,
                dbus_watch_get_unix_fd(watch),
                events,
                _nemodbus_event,
                nemodbus) < 0) {
            ERR("nemotool_watch_source failed");
            return FALSE;
        }
    }
    return TRUE;
}

static void _dbus_remove_watch(DBusWatch *watch, void *data)
{
    NemoDbus *nemodbus = data;
    nemotool_unwatch_source(nemodbus->tool,
            dbus_watch_get_unix_fd(watch));
}

#if 0
static void _dbus_toggle_watch(DBusWatch *watch, void *data)
{
    NemoDbus *nemodbus = data;
    nemodbus->watch = watch;

    if (dbus_watch_get_enabled(watch)) {
        uint32_t events = EPOLLHUP | EPOLLERR;
        uint32_t flags = dbus_watch_get_flags(watch);

        if (flags & DBUS_WATCH_READABLE)
            events |= EPOLLIN;

        if (nemotool_watch_source(nemodbus->tool,
                dbus_watch_get_unix_fd(watch),
                events,
                _nemodbus_event,
                nemodbus) < 0) {
            ERR("nemotool_watch_source failed");
        }
    }
}
#endif

NemoDbus *nemodbus_create(struct nemotool *tool)
{
    NemoDbus *nemodbus = calloc(sizeof(NemoDbus), 1);
    nemodbus->tool = tool;

    DBusError err;
    dbus_error_init(&err);
    DBusConnection *dbuscon;
    dbuscon = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        ERR("dbus bus get failed");
        dbus_error_free(&err);
        return NULL;
    }
    nemodbus->dbuscon = dbuscon;

    if (!dbus_connection_set_watch_functions(dbuscon,
            _dbus_add_watch,
            _dbus_remove_watch,
            NULL,    //_dbus_toggle_watch,
            nemodbus,
            NULL)) {
        ERR("dbus connection set watch fuctions failed");
        dbus_connection_close(dbuscon);
        free(nemodbus);
        return NULL;
    }

    return nemodbus;
}

void nemodbus_destroy(NemoDbus *nemodbus)
{
    dbus_connection_set_watch_functions(nemodbus->dbuscon,
            NULL, NULL, NULL, NULL, NULL);
    dbus_connection_unref(nemodbus->dbuscon);
    free(nemodbus);
}

typedef struct _NemoDbusCallData NemoDbusCallData;
struct _NemoDbusCallData {
    NemoDbus *nemodbus;
    DBusPendingCall *call;
    NemoDbusCallback callback;
    void *userdata;
};

static void _nemodbus_send_message_notify(DBusPendingCall *call, void *userdata)
{
    DBusMessage *msg;
    DBusError err;
    NemoDbusCallData *data = userdata;

    if (!dbus_pending_call_get_completed(call)) {
        ERR("dbus call not completed");
        free(data);
        dbus_pending_call_unref(call);
        return;
    }

    dbus_error_init(&err);
    msg = dbus_pending_call_steal_reply(call);
    if (!msg || dbus_set_error_from_message(&err, msg)) {
        ERR("dbus pending call steal reply failed :%s :%s ",
                err.name, err.message);
        if (data->callback) {
            data->callback(data->nemodbus, &err, NULL, data->userdata);
            dbus_error_free(&err);
        }
        free(data);
        return;
    }

    if (data->callback) {
        data->callback(data->nemodbus, NULL, msg, data->userdata);
    }
    dbus_message_unref(msg);
    dbus_pending_call_unref(call);
}

bool nemodbus_send_message(NemoDbus *nemodbus, DBusMessage *msg, NemoDbusCallback callback, void *userdata)
{
    DBusPendingCall *call;
    if (!dbus_connection_send_with_reply(nemodbus->dbuscon, msg, &call, -1)) {
        ERR("dbus connection send with replay failed");
        return false;
    }
    if (!call) {
        ERR("dbus pending call is NULL");
        return false;
    }

    if (callback) {
        NemoDbusCallData *data = calloc(sizeof(NemoDbusCallData), 1);
        data->nemodbus = nemodbus;
        data->call = call;
        data->callback = callback;
        data->userdata = userdata;

        if (!dbus_pending_call_set_notify(call, _nemodbus_send_message_notify, data, free)) {
            free(data);
            dbus_message_unref(msg);
            dbus_pending_call_cancel(call);
            return false;
        }
    }

    return true;
}

bool nemodbus_call_method(NemoDbus *nemodbus, const char *target, const char *path, const char *iface, const char *method, NemoDbusCallback callback, void *userdata)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(target, path, iface, method);
    RET_IF(!msg, false);

    return nemodbus_send_message(nemodbus, msg, callback, userdata);
}

void nemodbus_set_dict_entry(DBusMessageIter *iter, const char *property, int type, const void *value)
{
    DBusMessageIter entry;
    DBusMessageIter variant;
    const char *stype = NULL;

    dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry);

    if (type == DBUS_TYPE_BOOLEAN) {
        stype = DBUS_TYPE_BOOLEAN_AS_STRING;
    } else if (type == DBUS_TYPE_BYTE) {
        stype = DBUS_TYPE_BYTE_AS_STRING;
    } else if (type == DBUS_TYPE_STRING) {
        stype = DBUS_TYPE_STRING_AS_STRING;
    } else if (type == DBUS_TYPE_INT32) {
        stype = DBUS_TYPE_INT32_AS_STRING;
    }

    if (stype) {
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, stype, &variant);
        dbus_message_iter_append_basic(&variant, type, value);
        dbus_message_iter_close_container(&entry, &variant);
    }

    dbus_message_iter_close_container(iter, &entry);
}
