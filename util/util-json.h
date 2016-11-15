#ifndef __JSONPARSE_H__
#define __JSONPARSE_H__

#include <json.h>

#include <util-common.h>
#include <util-log.h>

/********************************************************/
/* Json Parser */
/* ******************************************************/
static inline Values *
_json_find(struct json_object *obj, const char *names)
{
    char *_names = strdup(names);
    char *name = strtok(_names, "/");

    if (!name) return NULL;

    Values *ret = NULL;
    json_type type = json_object_get_type(obj);

    if (json_type_object == type) {
        struct json_object_iterator it;
        struct json_object_iterator itEnd;
        it = json_object_iter_begin(obj);
        itEnd = json_object_iter_end(obj);

        while (!json_object_iter_equal(&it, &itEnd)) {
            json_object *cobj = json_object_iter_peek_value(&it);
            const char *_name = json_object_iter_peek_name(&it);

            if (_name && !strcmp(_name, name)) {
                const char *cnames = names + strlen(name) + 1;
                if (!cnames || !strlen(cnames)) cnames = names;
                ret = _json_find(cobj, cnames);
                break;
            }
            json_object_iter_next(&it);
        }
    } else if (json_type_array == type) {
        int i = 0;
        ret = values_create();
        for (i = 0; i < json_object_array_length(obj) ; i++) {
            Values *vals;
            struct json_object *cobj;
            cobj = json_object_array_get_idx(obj, i);

            vals = _json_find(cobj, names);
            if (vals && values_get_val(vals, 0))
                values_set_val(ret, i, values_get_val(vals, 0));
        }
    } else {
        json_type type = json_object_get_type(obj);
        if (json_type_boolean == type) {
            json_bool val = json_object_get_boolean(obj);
            ret = values_create();
            values_set_int(ret, 0, val);
        } else if (json_type_double == type) {
            double val = json_object_get_double(obj);
            ret = values_create();
            values_set_double(ret, 0, val);
        } else if (json_type_int == type) {
            int val = json_object_get_int(obj);
            ret = values_create();
            values_set_int(ret, 0, val);
        } else if (json_type_string == type) {
            const char *val = json_object_get_string(obj);
            ret = values_create();
            values_set_str(ret, 0, val);
        } else {
            ERR("Wrong json type(%d)", type);
            ret = NULL;
        }
    }

    free(_names);
    return ret;
}
#endif
