#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <sys/epoll.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemobus.h>

#include "util.h"
#include "widgets.h"
#include "nemohelper.h"

const char *TARGET = "net.connman";
const char *IFACE_MANAGER = "net.connman.Manager";
const char *IFACE_TECHNOLOGY = "net.connman.Technology";
const char *IFACE_SERVICE = "net.connman.Service";

const char *PATH_WIFI = "/net/connman/technology/wifi";
const char *PATH_P2P = "/net/connman/technology/p2p";
const char *PATH_ETHERNET = "/net/connman/technology/ethernet";
const char *PATH_SERVICE = "/net/connman/service";

typedef struct _ConnmanConnectData  ConnmanConnectData;
typedef struct _ConnmanEventData  ConnmanEventData;
typedef struct _Connman Connman;

typedef void (*ConnmanEventCallback)(Connman *con, const char *errmsg, void *event, void *data);
struct _Connman {
    NemoDbus *nemodbus;
    ConnmanEventCallback register_cb;
    ConnmanEventCallback unregister_cb;
    ConnmanEventCallback message_cb;
    void *userdata;

    List *connects;
    int state; // -1 (, 0 (connecting), 1 (online)
};

struct _ConnmanConnectData {
    Connman *con;
    ConnmanEventCallback callback;
    void *userdata;
    char *path;
    char *passwd;
};

struct _ConnmanEventData {
    Connman *con;
    ConnmanEventCallback callback;
    void *userdata;
};

Connman *connman_create(struct nemotool *tool)
{
    Connman *con = calloc(sizeof(Connman), 1);

    NemoDbus *nemodbus;
    con->nemodbus = nemodbus = nemodbus_create(tool);

    return con;
}

void connman_destroy(Connman *con)
{
    nemodbus_destroy(con->nemodbus);
    free(con);
}

static DBusHandlerResult _connman_message(DBusConnection *dbuscon, DBusMessage *msg, void *userdata)
{
    Connman *con = userdata;

    const char *method = dbus_message_get_member(msg);
	msg = dbus_message_ref(msg);
    ERR("%s", method);

	if (strcmp("Release", method) == 0) {
	} else if (strcmp("ReportError", method) == 0) {
	} else if (strcmp("RequestBrowser", method) == 0) {
	} else if (strcmp("RequestInput", method) == 0) {
		DBusMessageIter iter;
		DBusMessageIter dict;
		DBusMessage *reply;
        const char *path;

		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &path);

        List *l;
        ConnmanConnectData *connect = NULL;
        LIST_FOR_EACH(con->connects, l, connect) {
            if (!strcmp(connect->path, path)) {
                break;
            }
        }
        if (connect) {
            reply = dbus_message_new_method_return(msg);
            dbus_message_iter_init_append(reply, &iter);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
                    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                    DBUS_TYPE_STRING_AS_STRING
                    DBUS_TYPE_VARIANT_AS_STRING
                    DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                    &dict);
            nemodbus_set_dict_entry(&dict, "Passphrase",
                    DBUS_TYPE_STRING, &connect->passwd);
            ERR("%s, %s", connect->path, connect->passwd);

            dbus_message_iter_close_container(&iter, &dict);

            nemodbus_send_message(con->nemodbus, reply, NULL, NULL);
        }
	} else if (strcmp("Cancel", method) == 0) {
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

    return DBUS_HANDLER_RESULT_HANDLED;
}

static void _connman_unregister(DBusConnection *dbuscon, void *userdata)
{
    Connman *con = userdata;

    con->unregister_cb(con, NULL, NULL, con->userdata);
}

static DBusObjectPathVTable _connman_object_table = {
    .unregister_function = _connman_unregister,
    .message_function = _connman_message
};

static void _connman_register_callback(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    Connman *con = userdata;

    if (err) {
        con->register_cb(con, err->message, NULL, con->userdata);
    } else {
        con->register_cb(con, NULL, NULL, con->userdata);
    }
}

void connman_agent_on(Connman *con, const char *agent_path, ConnmanEventCallback register_cb, ConnmanEventCallback unregister_cb, ConnmanEventCallback message_cb, void *userdata)
{
    con->register_cb = register_cb;
    con->unregister_cb = unregister_cb;
    con->message_cb = message_cb;
    con->userdata = userdata;

    dbus_connection_register_object_path(con->nemodbus->dbuscon,
            agent_path, &_connman_object_table, con);

    DBusMessageIter iter;
    DBusMessage *msg = dbus_message_new_method_call(TARGET, "/",
            IFACE_MANAGER, "RegisterAgent");
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &agent_path);

    nemodbus_send_message(con->nemodbus, msg, _connman_register_callback, con);
}

static void _connman_get_properties(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;

    char errmsg[PATH_MAX];
    if (err) {
        snprintf(errmsg, PATH_MAX, "%s", err->message);
        goto _get_properties_FAIL;
    }

    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
        return;
    }

    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
    }

    DBusMessageIter iter0;
    dbus_message_iter_recurse(&iter, &iter0);
    if (DBUS_TYPE_DICT_ENTRY != dbus_message_iter_get_arg_type(&iter0)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
    }

    DBusMessageIter iter1;
    dbus_message_iter_recurse(&iter0, &iter1);

    if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter1)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
    }

    const char *str = NULL;
    dbus_message_iter_get_basic(&iter1, &str);
    if (!str || strcmp(str, "State")) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
    }

    dbus_message_iter_next(&iter1);
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter1)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_properties_FAIL;
    }

    DBusMessageIter iter2;
    dbus_message_iter_recurse(&iter1, &iter2);
    dbus_message_iter_get_basic(&iter2, &str);

    char *state = NULL;
    if (str) {
        state = strdup(str);
    }

    data->callback(data->con, NULL, (void *)state, data->userdata);
    free(state);
    free(data);
    return;

_get_properties_FAIL:
    data->callback(data->con, errmsg, NULL, data->userdata);
    free(data);
    return;
}

typedef struct _ConnmanTechnology ConnmanTechnology;
struct _ConnmanTechnology {
    char *path;
    char *name;
    char *type;
    bool is_powered;
    bool is_connected;
    bool is_tethering;
};

static void _connman_get_technologies(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;
    List *nts = NULL;

    char errmsg[PATH_MAX];
    if (err) {
        snprintf(errmsg, PATH_MAX, "%s", err->message);
        goto _get_technologies_FAIL;
    }

    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_technologies_FAIL;
    }

    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_technologies_FAIL;
    }

    DBusMessageIter iter0;
    dbus_message_iter_recurse(&iter, &iter0);

    do {
        if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&iter0)) {
            DBusMessageIter iter1;
            dbus_message_iter_recurse(&iter0, &iter1);

            if (DBUS_TYPE_OBJECT_PATH != dbus_message_iter_get_arg_type(&iter1))
                continue;

            ConnmanTechnology *nt = calloc(sizeof(ConnmanTechnology), 1);
            nts = list_append(nts, nt);

            const char *str = NULL;
            dbus_message_iter_get_basic(&iter1, &str);
            if (str) {
                nt->path = strdup(str);
            }

            dbus_message_iter_next(&iter1);
            if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter1))
                continue;

            DBusMessageIter iter2;
            dbus_message_iter_recurse(&iter1, &iter2);

            do {
                if (DBUS_TYPE_DICT_ENTRY != dbus_message_iter_get_arg_type(&iter2))
                    continue;
                DBusMessageIter iter3;
                dbus_message_iter_recurse(&iter2, &iter3);

                const char *str = NULL;
                dbus_message_iter_get_basic(&iter3, &str);
                if (str && !strcmp(str, "Name")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter4)) {
                            const char *str = NULL;
                            dbus_message_iter_get_basic(&iter4, &str);
                            if (str) {
                                nt->name = strdup(str);
                            }
                        }
                    }
                } else if (str && !strcmp(str, "Type")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter4)) {
                            const char *str = NULL;
                            dbus_message_iter_get_basic(&iter4, &str);
                            if (str) {
                                nt->type = strdup(str);
                            }
                        }
                    }
                } else if (str && !strcmp(str, "Powered")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            nt->is_powered = val;
                        }
                    }
                } else if (str && !strcmp(str, "Connected")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            nt->is_connected = val;
                        }
                    }
                } else if (str && !strcmp(str, "Tethering")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            nt->is_tethering = val;
                        }
                    }
                }

            } while (dbus_message_iter_next(&iter2));

        }
    } while (dbus_message_iter_next(&iter0));

    data->callback(data->con, NULL, nts, data->userdata);
    free(data);

    ConnmanTechnology *nt;
    LIST_FREE(nts, nt) {
        if (nt->path) free(nt->path);
        if (nt->name) free(nt->name);
        if (nt->type) free(nt->type);
        free(nt);
    }

    return;

_get_technologies_FAIL:
    data->callback(data->con, errmsg, NULL, data->userdata);
    free(data);
    return;
}

typedef struct _ConnmanEthernet ConnmanEthernet;
struct _ConnmanEthernet {
    char *method;
    char *interface;
    char *address;
    int mtu;
};

typedef struct _ConnmanIPv4 ConnmanIPv4;
struct _ConnmanIPv4 {
    char *method;
    char *address;
    char *netmask;
    char *gateway;
};

typedef struct _ConnmanService ConnmanService;
struct _ConnmanService {
    char *path;
    char *type;
    char *securities[8];

    char *state;
    int strength;
    bool favorite;
    bool immutable;
    bool autoconnect;

    char *name;
    ConnmanEthernet *ethernet;
    ConnmanIPv4 *ipv4;

    char *nameservers[4];
    char *timeservers[4];

    // TODO: ipv6, proxy and etc.
};

static void _connman_get_services(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;
    List *svcs = NULL;

    char errmsg[PATH_MAX];
    if (err) {
        snprintf(errmsg, PATH_MAX, "%s", err->message);
        goto _get_services_FAIL;
    }

    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_services_FAIL;
    }

    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter)) {
        snprintf(errmsg, PATH_MAX, "%s", "Reply parsing failed");
        goto _get_services_FAIL;
    }

    DBusMessageIter iter0;
    dbus_message_iter_recurse(&iter, &iter0);

    do {
        if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&iter0)) {
            DBusMessageIter iter1;
            dbus_message_iter_recurse(&iter0, &iter1);

            ConnmanService *svc = calloc(sizeof(ConnmanService), 1);
            svcs = list_append(svcs, svc);

            const char *str = NULL;

            if (DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&iter1)) {

                dbus_message_iter_get_basic(&iter1, &str);
                if (str) {
                    svc->path = strdup(str);
                }
            }

            dbus_message_iter_next(&iter1);
            if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter1))
                continue;

            DBusMessageIter iter2;
            dbus_message_iter_recurse(&iter1, &iter2);

            do {
                if (DBUS_TYPE_DICT_ENTRY != dbus_message_iter_get_arg_type(&iter2))
                    continue;
                DBusMessageIter iter3;
                dbus_message_iter_recurse(&iter2, &iter3);

                const char *str = NULL;
                dbus_message_iter_get_basic(&iter3, &str);
                if (str && !strcmp(str, "Type")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter4)) {
                            const char *str = NULL;
                            dbus_message_iter_get_basic(&iter4, &str);
                            if (str) {
                                svc->type = strdup(str);
                            }
                        }
                    }
                } else if (str && !strcmp(str, "Security")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);

                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter4))
                            continue;

                        DBusMessageIter iter5;
                        dbus_message_iter_recurse(&iter4, &iter5);

                        int i = 0;
                        do {
                            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter5)) {
                                const char *str = NULL;
                                dbus_message_iter_get_basic(&iter5, &str);
                                if (str) {
                                    svc->securities[i] = strdup(str);
                                    i++;
                                }
                            }
                        } while (dbus_message_iter_next(&iter5));
                    }
                } else if (str && !strcmp(str, "State")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter4)) {
                            const char *str = NULL;
                            dbus_message_iter_get_basic(&iter4, &str);
                            svc->state = strdup(str);
                        }
                    }
                } else if (str && !strcmp(str, "Strength")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BYTE == dbus_message_iter_get_arg_type(&iter4)) {
                            int val = 0;
                            dbus_message_iter_get_basic(&iter4, &val);
                            svc->strength = val;
                        }
                    }
                } else if (str && !strcmp(str, "Favorite")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            svc->favorite = val;
                        }
                    }
                } else if (str && !strcmp(str, "Immutable")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            svc->immutable = val;
                        }
                    }
                } else if (str && !strcmp(str, "AutoConnect")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter4)) {
                            bool val = false;
                            dbus_message_iter_get_basic(&iter4, &val);
                            svc->autoconnect = val;
                        }
                    }
                } else if (str && !strcmp(str, "Name")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);
                        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter4)) {
                            const char *str;
                            dbus_message_iter_get_basic(&iter4, &str);
                            svc->name = strdup(str);
                        }
                    }
                } else if (str && !strcmp(str, "Ethernet")) {
                    dbus_message_iter_next(&iter3);

                    svc->ethernet = calloc(sizeof(ConnmanEthernet), 1);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);

                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter4)) continue;
                        DBusMessageIter iter5;
                        dbus_message_iter_recurse(&iter4, &iter5);

                        do {
                            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&iter5)) {
                                DBusMessageIter iter6;
                                dbus_message_iter_recurse(&iter5, &iter6);

                                if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter6))
                                    continue;

                                const char *str;
                                dbus_message_iter_get_basic(&iter6, &str);
                                if (str && !strcmp(str, "Method")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ethernet->method = strdup(str);
                                    }
                                } else if (str && !strcmp(str, "Interface")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ethernet->interface = strdup(str);
                                    }
                                } else if (str && !strcmp(str, "Address")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ethernet->address = strdup(str);
                                    }
                                } else if (str && !strcmp(str, "MTU")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_UINT16 == dbus_message_iter_get_arg_type(&iter7)) {
                                        int val;
                                        dbus_message_iter_get_basic(&iter7, &val);
                                        svc->ethernet->mtu = val;
                                    }
                                }
                            }
                        } while (dbus_message_iter_next(&iter5));
                    }
                } else if (str && !strcmp(str, "IPv4")) {
                    dbus_message_iter_next(&iter3);

                    svc->ipv4 = calloc(sizeof(ConnmanIPv4), 1);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);

                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter4)) continue;
                        DBusMessageIter iter5;
                        dbus_message_iter_recurse(&iter4, &iter5);

                        do {
                            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&iter5)) {
                                DBusMessageIter iter6;
                                dbus_message_iter_recurse(&iter5, &iter6);

                                if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter6))
                                    continue;

                                const char *str;
                                dbus_message_iter_get_basic(&iter6, &str);
                                if (str && !strcmp(str, "Method")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ipv4->method = strdup(str);
                                    }
                                } else if (str && !strcmp(str, "Address")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ipv4->address = strdup(str);
                                    }
                                } else if (str && !strcmp(str, "Netmask")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *str;
                                        dbus_message_iter_get_basic(&iter7, &str);
                                        svc->ipv4->netmask= strdup(str);
                                    }
                                } else if (str && !strcmp(str, "Gateway")) {
                                    dbus_message_iter_next(&iter6);
                                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter6))
                                        continue;

                                    DBusMessageIter iter7;
                                    dbus_message_iter_recurse(&iter6, &iter7);

                                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter7)) {
                                        const char *gateway;
                                        dbus_message_iter_get_basic(&iter7, &gateway);
                                        svc->ipv4->gateway = strdup(gateway);
                                    }
                                }
                            }
                        } while (dbus_message_iter_next(&iter5));
                    }
                } else if (str && !strcmp(str, "Nameservers")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);

                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter4))
                            continue;

                        DBusMessageIter iter5;
                        dbus_message_iter_recurse(&iter4, &iter5);

                        int i = 0;
                        do {
                            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter5)) {
                                const char *str = NULL;
                                dbus_message_iter_get_basic(&iter5, &str);
                                if (str) {
                                    svc->nameservers[i] = strdup(str);
                                    i++;
                                }
                            }
                        } while (dbus_message_iter_next(&iter5));
                    }
                } else if (str && !strcmp(str, "Timeservers")) {
                    dbus_message_iter_next(&iter3);

                    if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&iter3)) {
                        DBusMessageIter iter4;
                        dbus_message_iter_recurse(&iter3, &iter4);

                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter4))
                            continue;

                        DBusMessageIter iter5;
                        dbus_message_iter_recurse(&iter4, &iter5);

                        int i = 0;
                        do {
                            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter5)) {
                                const char *str = NULL;
                                dbus_message_iter_get_basic(&iter5, &str);
                                if (str) {
                                    svc->timeservers[i] = strdup(str);
                                    i++;
                                }
                            }
                        } while (dbus_message_iter_next(&iter5));
                    }
                }
            } while (dbus_message_iter_next(&iter2));

        }
    } while (dbus_message_iter_next(&iter0));

    data->callback(data->con, NULL, svcs, data->userdata);
    free(data);

    ConnmanService *svc;
    LIST_FREE(svcs, svc) {
        if (svc->path) free(svc->path);
        if (svc->type) free(svc->type);
        if (svc->securities) {
            int i;
            for (i = 0 ; i < 8 ; i++) {
                if (svc->securities[i])
                    free(svc->securities[i]);
            }
        }
        if (svc->state) free(svc->state);
        if (svc->name) free(svc->name);
        if (svc->ethernet) {
            if (svc->ethernet->method) free(svc->ethernet->method);
            if (svc->ethernet->interface) free(svc->ethernet->interface);
            if (svc->ethernet->address) free(svc->ethernet->address);
        }
        if (svc->ipv4) {
            if (svc->ipv4->method) free(svc->ipv4->method);
            if (svc->ipv4->address) free(svc->ipv4->address);
            if (svc->ipv4->netmask) free(svc->ipv4->netmask);
            if (svc->ipv4->gateway) free(svc->ipv4->gateway);
        }
        if (svc->nameservers) {
            int i;
            for (i = 0 ; i < 4 ; i++) {
                if (svc->nameservers[i])
                    free(svc->nameservers[i]);
            }
        }
        if (svc->timeservers) {
            int i;
            for (i = 0 ; i < 4 ; i++) {
                if (svc->timeservers[i])
                    free(svc->timeservers[i]);
            }
        }
        free(svc);
    }

    return;

_get_services_FAIL:
    data->callback(data->con, errmsg, NULL, data->userdata);
    free(data);
    return;
}

static void connman_get_state(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    nemodbus_call_method(con->nemodbus, TARGET, "/", IFACE_MANAGER, "GetProperties",
            _connman_get_properties, data);
}

static void connman_get_technologies(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    nemodbus_call_method(con->nemodbus, TARGET, "/", IFACE_MANAGER, "GetTechnologies",
            _connman_get_technologies, data);
}

static void connman_get_services(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    nemodbus_call_method(con->nemodbus, TARGET, "/", IFACE_MANAGER, "GetServices",
            _connman_get_services, data);
}

static void _connman_callback(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;

    if (err) {
        data->callback(data->con, err->message, NULL, data->userdata);
    } else {
        data->callback(data->con, NULL, NULL, data->userdata);
    }
    free(data);
}

static void connman_disable_ethernet(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, PATH_ETHERNET, IFACE_TECHNOLOGY, "SetProperty");

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);

    char *key = "Powered";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_bool_t value = false;

    DBusMessageIter variant;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_BOOLEAN_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_callback, data);
}

static void connman_enable_ethernet(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, PATH_ETHERNET, IFACE_TECHNOLOGY, "SetProperty");

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);

    char *key = "Powered";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_bool_t value = true;

    DBusMessageIter variant;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_BOOLEAN_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_callback, data);
}

static void connman_disable_wifi(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, PATH_WIFI, IFACE_TECHNOLOGY, "SetProperty");

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);

    char *key = "Powered";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_bool_t value = false;

    DBusMessageIter variant;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_BOOLEAN_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_callback, data);
}

static void connman_enable_wifi(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, PATH_WIFI, IFACE_TECHNOLOGY, "SetProperty");

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);

    char *key = "Powered";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_bool_t value = true;

    DBusMessageIter variant;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_BOOLEAN_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_callback, data);
}

static void _connman_scan_wifi(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;

    if (err) {
        data->callback(data->con, err->message, NULL, data->userdata);
    } else {
        data->callback(data->con, NULL, NULL, data->userdata);
    }
    free(data);
}

static void connman_scan_wifi(Connman *con, ConnmanEventCallback callback, void *userdata)
{
    RET_IF(!con);
    RET_IF(!callback);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    nemodbus_call_method(con->nemodbus, TARGET, PATH_WIFI, IFACE_TECHNOLOGY, "Scan",
            _connman_scan_wifi, data);
}

static void _connman_connect(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanConnectData *data = userdata;
    if (err) {
        data->callback(data->con, err->message, NULL, data->userdata);
    } else {
        data->callback(data->con, NULL, NULL, data->userdata);
    }

    data->con->connects = list_remove(data->con->connects, data);
    free(data->path);
    if (data->passwd) free(data->passwd);
    free(data);
}

static void connman_connect(Connman *con, ConnmanEventCallback callback, void *userdata, const char *path, const char *passwd)
{
    RET_IF(!con);
    RET_IF(!callback);
    RET_IF(!path);

    ConnmanConnectData *data = calloc(sizeof(ConnmanConnectData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;
    data->path = strdup(path);
    if (passwd) data->passwd = strdup(passwd);
    con->connects = list_append(con->connects, data);

    nemodbus_call_method(con->nemodbus, TARGET, path, IFACE_SERVICE, "Connect",
            _connman_connect, data);
}

static void _connman_disconnect(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;

    if (err) {
        data->callback(data->con, err->message, NULL, data->userdata);
    } else {
        data->callback(data->con, NULL, NULL, data->userdata);
    }
    free(data);
}

static void connman_disconnect(Connman *con, ConnmanEventCallback callback, void *userdata, const char *path)
{
    RET_IF(!con);
    RET_IF(!callback);
    RET_IF(!path);

    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    nemodbus_call_method(con->nemodbus, TARGET, path, IFACE_SERVICE, "Disconnect",
            _connman_disconnect, data);
}

static void _connman_config(NemoDbus *nemodbus, DBusError *err, DBusMessage *msg, void *userdata)
{
    ConnmanEventData *data = userdata;

    if (err) {
        data->callback(data->con, err->message, NULL, data->userdata);
    } else {
        data->callback(data->con, NULL, NULL, data->userdata);
    }
    free(data);
}

static void connman_config_nameserver(Connman *con, ConnmanEventCallback callback, void *userdata, const char *path, const char *nameserver, const char *nameserver2)
{
    RET_IF(!con);
    RET_IF(!callback);
    RET_IF(!path);
    RET_IF(!nameserver);

    ERR("%s %s", nameserver, nameserver2);
    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, path, IFACE_SERVICE, "SetProperty");

    DBusMessageIter iter;
    DBusMessageIter variant;

    dbus_message_iter_init_append(msg, &iter);
    DBusMessageIter array;
    const char *key = "Nameservers.Configuration";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_ARRAY_AS_STRING
            DBUS_TYPE_STRING_AS_STRING,
            &variant);
    dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
            DBUS_TYPE_STRING_AS_STRING,
            &array);
    dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &nameserver);
    if (nameserver2)
        dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &nameserver2);
    dbus_message_iter_close_container(&variant, &array);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_config, data);
}

static void connman_config_ipv4(Connman *con, ConnmanEventCallback callback, void *userdata, const char *path, const char *address, const char *netmask, const char *gateway)
{
    RET_IF(!con);
    RET_IF(!callback);
    RET_IF(!path);

    ERR("%s %s %s", address, netmask, gateway);
    ConnmanEventData *data = calloc(sizeof(ConnmanEventData), 1);
    data->con = con;
    data->callback = callback;
    data->userdata = userdata;

    DBusMessage *msg;
    msg = dbus_message_new_method_call(TARGET, path, IFACE_SERVICE, "SetProperty");

    DBusMessageIter iter;
    DBusMessageIter variant;
    DBusMessageIter dict;

    dbus_message_iter_init_append(msg, &iter);
    const char *key = "IPv4.Configuration";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
            DBUS_TYPE_ARRAY_AS_STRING
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                    DBUS_TYPE_STRING_AS_STRING
                    DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
            &variant);
    dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
            DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
            DBUS_TYPE_STRING_AS_STRING
            DBUS_TYPE_VARIANT_AS_STRING
            DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
            &dict);

    if (address) {
        const char *method = "manual";
        ERR("Method: %s", method);
        nemodbus_set_dict_entry(&dict, "Method", DBUS_TYPE_STRING, &method);
        nemodbus_set_dict_entry(&dict, "Address", DBUS_TYPE_STRING, &address);
        if (netmask)
            nemodbus_set_dict_entry(&dict, "Netmask", DBUS_TYPE_STRING, &netmask);
        if (gateway)
            nemodbus_set_dict_entry(&dict, "Gatewaty", DBUS_TYPE_STRING, &gateway);

    } else {
        const char *method = "dhcp";
        ERR("Method: %s", method);
        nemodbus_set_dict_entry(&dict, "Method", DBUS_TYPE_STRING, &method);
    }
    dbus_message_iter_close_container(&variant, &dict);
    dbus_message_iter_close_container(&iter, &variant);

    nemodbus_send_message(con->nemodbus, msg, _connman_config, data);
}

static void _register(Connman *con, const char *errmsg, void *event, void *userdata)
{
    if (errmsg) {
        ERR("Register failed: %s", errmsg);
    } else {
        ERR("Registered");
    }
}

static void _unregister(Connman *con, const char *errmsg, void *event, void *userdata)
{
    if (errmsg) {
        ERR("Unregister failed: %s", errmsg);
    } else {
        ERR("Unregistered");
        //connman_get_services(con, _get_services, con);
    }
}

static void _message(Connman *con, const char *errmsg, void *event, void *userdata)
{
    if (errmsg) {
        ERR("message failed: %s", errmsg);
    } else {
        ERR("message received");
    }
}

#define OBJECT_PATH "/nemoconnmand"

typedef struct _ConnDaemon ConnDaemon;
struct _ConnDaemon {
    struct nemobus *bus;
    Connman *con;
};

typedef struct _MsgData MsgData;
struct _MsgData
{
    ConnDaemon *daemon;
    char *src;
    char *cmd;
    char *path;
    int refcnt;
};

static void _send(struct nemobus *bus, const char *dst, const char *cmd, const char *path, const char *result, struct busmsg *msg)
{
    nemobus_msg_set_attr(msg, "cmd", cmd);
    nemobus_msg_set_attr(msg, "path", path);
    nemobus_msg_set_attr(msg, "result", result);
    nemobus_send(bus, OBJECT_PATH, dst, msg);
    nemobus_msg_destroy(msg);
}

static void _get_state(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg || !event) {
        ERR("Fail: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");
        char *state = (char *)event;
        ERR("state: %s", state);
        nemobus_msg_set_attr(msg, "state", state);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _scan_wifi(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Wifi scan failed: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Wifi scanned");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }

    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _get_technologies_wifi(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg || !event) {
        ERR("%s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");

        List *technologies = event;
        List *l;
        ConnmanTechnology *tech;

        LIST_FOR_EACH(technologies, l, tech) {
            if (tech->type && !strcasecmp(tech->type, "wifi")) {
                ERR("%s %s %s %d %d %d",
                        tech->path, tech->name, tech->type,
                        tech->is_powered, tech->is_connected, tech->is_tethering);
                nemobus_msg_set_attr(msg, "path", tech->path);
                nemobus_msg_set_attr(msg, "name", tech->name);
                nemobus_msg_set_attr(msg, "type", tech->type);
                nemobus_msg_set_attr(msg, "powered", tech->is_powered ? "true" : "false");
                nemobus_msg_set_attr(msg, "connected", tech->is_connected ? "true" : "false");
                nemobus_msg_set_attr(msg, "connected", tech->is_tethering ? "true" : "false");
                break;

            }
        }
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _get_technologies_ethernet(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg || !event) {
        ERR("%s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");

        List *technologies = event;
        List *l;
        ConnmanTechnology *tech;

        LIST_FOR_EACH(technologies, l, tech) {
            if (tech->type && !strcasecmp(tech->type, "ethernet")) {
                ERR("%s %s %s %d %d %d",
                        tech->path, tech->name, tech->type,
                        tech->is_powered, tech->is_connected, tech->is_tethering);
                nemobus_msg_set_attr(msg, "path", tech->path);
                nemobus_msg_set_attr(msg, "name", tech->name);
                nemobus_msg_set_attr(msg, "type", tech->type);
                nemobus_msg_set_attr(msg, "powered", tech->is_powered ? "true" : "false");
                nemobus_msg_set_attr(msg, "connected", tech->is_connected ? "true" : "false");
                nemobus_msg_set_attr(msg, "connected", tech->is_tethering ? "true" : "false");
                break;

            }
        }
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _get_technologies(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg || !event) {
        ERR("%s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");

        List *technologies = event;
        List *l;
        ConnmanTechnology *tech;

        LIST_FOR_EACH(technologies, l, tech) {
            ERR("%s %s %s %d %d %d",
                    tech->path, tech->name, tech->type,
                    tech->is_powered, tech->is_connected, tech->is_tethering);
            nemobus_msg_set_attr(msg, "path", tech->path);
            nemobus_msg_set_attr(msg, "name", tech->name);
            nemobus_msg_set_attr(msg, "type", tech->type);
            nemobus_msg_set_attr(msg, "powered", tech->is_powered ? "true" : "false");
            nemobus_msg_set_attr(msg, "connected", tech->is_connected ? "true" : "false");
            nemobus_msg_set_attr(msg, "connected", tech->is_tethering ? "true" : "false");
            // FIXME: should be list
        }
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _enable_wifi(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Wifi enable failed: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Wifi enabled");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _disable_wifi(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Wifi disable failed: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Wifi disabled");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _enable_ethernet(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Ethernet enable failed: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Ethernet enabled");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _disable_ethernet(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Ethernet disable failed: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Ethernet disabled");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _get_services(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg || !event) {
        ERR("Fail: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        List *svcs = event;
        List *l;
        ConnmanService *svc;

        // FIXME: should be list
        LIST_FOR_EACH(svcs, l, svc) {
            char *sec = NULL;
            ERR("path:%s type:%s state:%s name:%s", svc->path, svc->type, svc->state, svc->name);
            ERR("str:%d fav:%d immut:%d auto:%d", svc->strength, svc->favorite, svc->immutable, svc->autoconnect);
            if (svc->securities) {
                int i;
                for(i = 0 ; i < 8 ; i++) {
                    if (svc->securities[i] && !strcmp(svc->securities[i], "psk")) {
                        sec = strdup("true");
                        break;
                    }
                }
            }
            if (!sec) {
                sec = strdup("false");
            }

            nemobus_msg_set_attr(msg, "path", svc->path);
            nemobus_msg_set_attr(msg, "name", svc->name);
            nemobus_msg_set_attr(msg, "type", svc->type);
            nemobus_msg_set_attr(msg, "security", sec);
            nemobus_msg_set_attr(msg, "state", svc->state);
            nemobus_msg_set_attr_format(msg, "strength", "%d", svc->strength);
            free(sec);

            if (svc->ipv4) {
                if (svc->ipv4->method) {
                    nemobus_msg_set_attr(msg, "method", svc->ipv4->method);
                }
                if (svc->ipv4->address) {
                    nemobus_msg_set_attr(msg, "method", svc->ipv4->address);
                }
                if (svc->ipv4->netmask) {
                    nemobus_msg_set_attr(msg, "method", svc->ipv4->netmask);
                }
                if (svc->ipv4->gateway) {
                    nemobus_msg_set_attr(msg, "method", svc->ipv4->gateway);
                }
            }

            if (svc->nameservers[0]) {
                char temp[2048];
                snprintf(temp, 2048, "%s", svc->nameservers[0]);

                int i;
                for (i = 1 ; i < 4 ; i++) {
                    if (svc->nameservers[i])  {
                        char temp2[2048];
                        snprintf(temp2, 2048, "; %s", svc->nameservers[i]);
                        strcat(temp, temp2);
                    }
                }
                nemobus_msg_set_attr(msg, "nameservers", temp);
            }
            // Duplicate, list need
            _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
        }
    }

    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _connect(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Fail: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _disconnect(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Fail: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _config_ipv4(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Fail: %s", errmsg);
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "false", msg);
    } else {
        ERR("Succeed");
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, "true", msg);
    }
    free(msgdata->src);
    free(msgdata->cmd);
    free(msgdata->path);
    free(msgdata);
}

static void _config_nameserver(Connman *con, const char *errmsg, void *event, void *userdata)
{
    MsgData *msgdata = userdata;
    ConnDaemon *daemon = msgdata->daemon;
    char buf[1024];

    struct busmsg *msg;
    msg = nemobus_msg_create();
    if (errmsg) {
        ERR("Fail: %s", errmsg);
        snprintf(buf, 1024, "false");
    } else {
        ERR("Succeed");
        snprintf(buf, 1024, "true");
    }
    msgdata->refcnt++;

    if (msgdata->refcnt >= 2) {
        _send(daemon->bus, msgdata->src, msgdata->cmd, msgdata->path, buf, msg);
        free(msgdata->src);
        free(msgdata->cmd);
        free(msgdata->path);
        free(msgdata);
    }
}

static void _conndaemon_bus_callback(void *data, const char *events)
{
    RET_IF(!events);
    if (!strchr(events, 'u')) {
        ERR("SIGHUP received");
        return;
    } else if (!strchr(events, 'e')) {
        ERR("Error received");
        return;
    }

	ConnDaemon *daemon = data;

	struct nemoitem *it;
	struct itemone *one;
    it = nemobus_recv_item(daemon->bus);
    RET_IF(!it);

    const char *from = NULL;
	nemoitem_for_each(one, it) {
        if (nemoitem_one_has_path(one, "/nemoshell")) {
            from = nemoitem_one_get_attr(one, "from");
        }
    }

	nemoitem_for_each(one, it) {
        const char *cmd = nemoitem_one_get_attr(one, "cmd");
        if (!cmd) {
            ERR("No cmd specified");
            continue;
        }
        if (!strcmp(cmd, "get")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/state")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/state");
                connman_get_state(daemon->con, _get_state, msgdata);
            } else if (nemoitem_one_has_path(one, OBJECT_PATH"/services")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/services");
                connman_get_services(daemon->con, _get_services, msgdata);
            } else if (nemoitem_one_has_path(one, OBJECT_PATH"/technology")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/technology");
                connman_get_technologies(daemon->con, _get_technologies, msgdata);
            } else if (nemoitem_one_has_path(one, OBJECT_PATH"/technology/wifi")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/technology/wifi");
                connman_get_technologies(daemon->con, _get_technologies_wifi, msgdata);
            } else if (nemoitem_one_has_path(one, OBJECT_PATH"/technology/ethernet")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/technology/ethernet");
                connman_get_technologies(daemon->con, _get_technologies_ethernet, msgdata);
            }
        } else if (!strcmp(cmd, "set")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/technology/wifi")) {
                const char *value = nemoitem_one_get_attr(one, "powered");
                if (value) {
                    MsgData *msgdata = calloc(sizeof(MsgData), 1);
                    msgdata->daemon = daemon;
                    msgdata->src = strdup(from);
                    msgdata->cmd = strdup(cmd);
                    msgdata->path = strdup(OBJECT_PATH"/technology/wifi");
                    if (!strcasecmp(value, "on")) {
                        connman_enable_wifi(daemon->con, _enable_wifi, msgdata);
                        break;
                    } else {
                        connman_disable_wifi(daemon->con, _disable_wifi, msgdata);
                        break;
                    }
                }
            } else if (nemoitem_one_has_path(one, OBJECT_PATH"/technology/ethernet")) {
                const char *value = nemoitem_one_get_attr(one, "powered");
                if (value) {
                    MsgData *msgdata = calloc(sizeof(MsgData), 1);
                    msgdata->daemon = daemon;
                    msgdata->src = strdup(from);
                    msgdata->cmd = strdup(cmd);
                    msgdata->path = strdup(OBJECT_PATH"/technology/ethernet");
                    if (!strcasecmp(value, "on")) {
                        connman_enable_ethernet(daemon->con, _enable_ethernet, msgdata);
                        break;
                    } else {
                        connman_disable_ethernet(daemon->con, _disable_ethernet, msgdata);
                        break;
                    }
                }
            }
        } else if (!strcmp(cmd, "scan")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/technology/wifi")) {
                MsgData *msgdata = calloc(sizeof(MsgData), 1);
                msgdata->daemon = daemon;
                msgdata->src = strdup(from);
                msgdata->cmd = strdup(cmd);
                msgdata->path = strdup(OBJECT_PATH"/technology/wifi");
                connman_scan_wifi(daemon->con, _scan_wifi, msgdata);
            }
        } else if (!strcmp(cmd, "connect")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/services")) {
                const char *id = nemoitem_one_get_attr(one, "path");
                const char *pw = nemoitem_one_get_attr(one, "passwd");
                if (id) {
                    ERR("%s %s", id, pw);
                    MsgData *msgdata = calloc(sizeof(MsgData), 1);
                    msgdata->daemon = daemon;
                    msgdata->src = strdup(from);
                    msgdata->cmd = strdup(cmd);
                    msgdata->path = strdup(OBJECT_PATH"/services");
                    connman_connect(daemon->con, _connect, msgdata, id, pw);
                }
            }
        } else if (!strcmp(cmd, "disconnect")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/services")) {
                const char *id = nemoitem_one_get_attr(one, "path");
                if (id) {
                    MsgData *msgdata = calloc(sizeof(MsgData), 1);
                    msgdata->daemon = daemon;
                    msgdata->src = strdup(from);
                    msgdata->cmd = strdup(cmd);
                    msgdata->path = strdup(OBJECT_PATH"/services");
                    connman_disconnect(daemon->con, _disconnect, msgdata, id);
                }
            }
        } else if (!strcmp(cmd, "config")) {
            if (nemoitem_one_has_path(one, OBJECT_PATH"/services")) {
                const char *id = nemoitem_one_get_attr(one, "path");
                const char *address = nemoitem_one_get_attr(one, "address");
                const char *netmask = nemoitem_one_get_attr(one, "netmask");
                const char *gateway = nemoitem_one_get_attr(one, "gateway");
                const char *value = nemoitem_one_get_attr(one, "path");
                char *nameserver = NULL, *nameserver2 = NULL;
                if (value) {
                    char *temp, *temp2;
                    temp = strdup(value);
                    temp2 = strtok(temp, ";");
                    if (temp2) nameserver = strdup(temp2);
                    temp2 = strtok(NULL, ";");
                    if (temp2) nameserver2 = strdup(temp2);
                    free(temp);
                }

                if (id) {
                    MsgData *msgdata = calloc(sizeof(MsgData), 1);
                    msgdata->daemon = daemon;
                    msgdata->src = strdup(from);
                    msgdata->cmd = strdup(cmd);
                    msgdata->path = strdup(OBJECT_PATH"/services");
                    if (!nameserver) {
                        connman_config_ipv4(daemon->con, _config_ipv4, msgdata,
                                id, address, netmask, gateway);
                    } else {
                        connman_config_ipv4(daemon->con, _config_nameserver, msgdata,
                                id, address, netmask, gateway);
                        connman_config_nameserver(daemon->con, _config_nameserver, msgdata,
                                id, nameserver, nameserver2);
                        free(nameserver);
                        if (nameserver2) free(nameserver);
                    }
                }

            }
        }
	}

	nemoitem_destroy(it);
}

ConnDaemon *conndaemon_create(struct nemotool *tool, int port)
{
    ConnDaemon *daemon = calloc(sizeof(ConnDaemon), 1);

    Connman *con;
    daemon->con = con = connman_create(tool);
    connman_agent_on(con, OBJECT_PATH, _register, _unregister, _message, con);

    struct nemobus *bus;
    daemon->bus = bus = nemobus_create();
    nemobus_connect(bus, NULL);
    nemobus_advertise(bus, "set", OBJECT_PATH);

	nemotool_watch_source(tool,
			nemobus_get_socket(bus),
			"reh",
            _conndaemon_bus_callback,
            daemon);

    return daemon;
}

void conndaemon_destroy(ConnDaemon *daemon)
{
    connman_destroy(daemon->con);
    nemobus_destroy(daemon->bus);
    free(daemon);
}

int main(int argc, char *argv[])
{
    struct nemotool *tool = TOOL_CREATE();

    ConnDaemon *daemon = conndaemon_create(tool, 30000);

    nemotool_run(tool);

    conndaemon_destroy(daemon);

    TOOL_DESTROY(tool);

    return 0;
}
