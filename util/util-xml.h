#ifndef __UTIL_XML_H__
#define __UTIL_XML_H__

#include <util-list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmlAttr XmlAttr;
struct _XmlAttr {
    char *key;
    char *val;
};

typedef struct _XmlTag XmlTag;
struct _XmlTag {
    char *name;

    XmlTag *parent;
    List *children;

    List *attrs;
    char *content;
};

XmlTag *xml_tag_create(const char *name);
void xml_tag_destroy(XmlTag *tag);
const char *xml_tag_get_attr_val(XmlTag *tag, const char *key);
void xml_tag_append_attr(XmlTag *tag, const char *key, const char *val);
void xml_tag_change_attr(XmlTag *tag, const char *key, const char *val);
void xml_tag_change_string(XmlTag *tag, ...);
void xml_tag_add_content(XmlTag *tag, const char *content);
void xml_tag_append_child(XmlTag *parent, XmlTag *child);
XmlTag *xml_tag_remove_child(XmlTag *parent, XmlTag *child);
XmlTag *xml_tag_dup(XmlTag *tag);

void xml_tag_dump(XmlTag *root, int depth);
typedef struct _Xml Xml;
struct _Xml {
    XmlTag *root;
    XmlTag *parent;
    XmlTag *cur;
};

Xml *xml_load(const char *data, size_t size);
Xml *xml_load_from_path(const char *path);
void xml_unload(Xml *xml);
List *xml_search_tags(Xml *xml, const char *xpath);
const char *xml_get_value(Xml *xml, const char *xpath, const char *name);
void xml_append_tag(Xml *xml, const char *xpath, XmlTag *tag);
void xml_remove_tag(Xml *xml, const char *xpath);
void xml_dump(Xml *xml);
void xml_write(Xml *xml, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif

