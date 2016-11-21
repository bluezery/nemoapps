#include "nemoutil-internal.h"
#include "nemoutil.h"
#include "nemoutil-svg.h"

const char *svg_get_value(Xml *xml, const char *xpath, const char *name)
{
    List *temp;
    temp = xml_search_tags(xml, xpath);
    temp = LIST_LAST(temp);
    if (!temp) {
        ERR("There is xpath: %s in the xml", xpath);
        return NULL;
    }

    XmlTag *tag = (XmlTag *)LIST_DATA(temp);
    list_free(temp);

    List *l;
    XmlAttr *attr;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, name)) {
            return attr->val;
        }
    }
    return NULL;
}

bool svg_get_wh(const char *uri, double *w, double *h)
{
    if (!file_is_exist(uri) || file_is_null(uri)) {
        ERR("file does not exist or file is NULL: %s", uri);
        return false;
    }
    size_t size;
    char *data;
    data = file_load(uri, &size);
    if (!data) {
        ERR("file has no data: %s", uri);
        return false;
    }
    Xml *xml = xml_load(data, size);
    if (!xml || !xml->root) {
        ERR("Something wrong with xml file: %s", uri);
        free(data);
        return false;
    }
    free(data);

    const char *temp;
    temp = svg_get_value(xml, "/svg", "width");
    if (temp) {
        *w = atof(temp);
    }
    temp = svg_get_value(xml, "/svg", "height");
    if (temp) {
        *h = atof(temp);
    }

    xml_unload(xml);
    return true;
}
