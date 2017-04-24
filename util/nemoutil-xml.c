#include <expat.h>
#include "nemoutil-internal.h"
#include "nemoutil.h"
#include "nemoutil-xml.h"

#ifdef XML_LARGE_SIZE
#define XML_FMT_INT_MOD "ll"
#else
#define XML_FMT_INT_MOD "l"
#endif

XmlTag *xml_tag_create(const char *name)
{
    XmlTag *tag;
    tag = (XmlTag *)calloc(sizeof(XmlTag), 1);

    tag->name = strdup(name);

    return tag;
}

void xml_tag_destroy(XmlTag *tag)
{
    XmlTag *child;
    LIST_FREE(tag->children, child) {
        xml_tag_destroy(child);
    }

    XmlAttr *attr;
    LIST_FREE(tag->attrs, attr) {
        free(attr->key);
        free(attr->val);
        free(attr);
    }

    if (tag->content) free(tag->content);
    free(tag->name);
    free(tag);
}

const char *xml_tag_get_attr_val(XmlTag *tag, const char *key)
{
    XmlAttr *attr;
    List *l;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, key)) {
            return attr->val;
        }
    }
    return NULL;
}

void xml_tag_append_attr(XmlTag *tag, const char *key, const char *val)
{
    XmlAttr *attr;
    attr = (XmlAttr *)calloc(sizeof(XmlAttr), 1);
    attr->key = strdup(key);
    attr->val = strdup(val);

    tag->attrs = list_append(tag->attrs, attr);
}

void xml_tag_change_attr(XmlTag *tag, const char *key, const char *val)
{
    XmlAttr *attr;
    List *l;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        if (!strcmp(attr->key, key)) {
            free(attr->val);
            attr->val = strdup(val);
            return;
        }
    }

    // If not exist, add new val
    xml_tag_append_attr(tag, key, val);
}

static void xml_tag_set_content(XmlTag *tag, char *content)
{
    tag->content = content;
}

void xml_tag_add_content(XmlTag *tag, const char *content)
{
    if (tag->content) free(tag->content);
    if (content)
        xml_tag_set_content(tag, strdup(content));
    else
        xml_tag_set_content(tag, NULL);
}

void xml_tag_append_child(XmlTag *parent, XmlTag *child)
{
    if (child->parent) {
        child->parent->children =
            list_remove(child->parent->children, child);
    }

    child->parent = parent;
    parent->children = list_append(parent->children, child);
}

XmlTag *xml_tag_remove_child(XmlTag *parent, XmlTag *child)
{
    if (!parent | !child) {
        ERR("parent(%p) or child(%p) is NULL", parent, child);
        return NULL;
    }
    if (parent != child->parent) {
        ERR("parent(%p,%s) does not have child(%p,%s)",
                parent, parent->name, child, child->name);
        return NULL;
    }

    parent->children = list_remove(parent->children, child);
    return child;
}

#if 0
static void xml_tag_append(XmlTag *tag, XmlTag *new_tag)
{
    if (!tag || !tag->parent) {
        ERR("tag is NULL or tag is root element: %p\n", tag);
        return;
    }

    XmlTag *temp;
    List *l;
    List *children;
    children = tag->parent->children;

    LIST_FOR_EACH(children, l, temp) {
        if (temp == tag) break;
    }
    if (!l || !temp) {
        ERR("something is wrong!, parent(%p,%s) does not have me(%p,%s) \n",
                tag->parent->name, tag, tag->name);
    }

    new_tag->parent = tag->parent;
    list_append(l, new_tag);
}
#endif

XmlTag *xml_tag_dup(XmlTag *tag)
{
    XmlTag *temp;
    List *l;

    XmlTag *ret = xml_tag_create(tag->name);

    LIST_FOR_EACH(tag->children, l, temp) {
        XmlTag *child = xml_tag_dup(temp);
        xml_tag_append_child(ret, child);
    }

    XmlAttr *attr;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        xml_tag_append_attr(ret, attr->key, attr->val);
    }

    if (tag->content) ret->content = strdup(tag->content);
    return ret;
}

#if 0
static void xml_tag_prepend(XmlTag *tag, XmlTag *new_tag)
{
    if (!tag || !tag->parent) {
        ERR("tag is NULL or tag is root element: %p\n", tag);
        return;
    }

    XmlTag *temp;
    List *l;
    List *children;
    children = tag->parent->children;

    LIST_FOR_EACH(children, l, temp) {
        if (temp == tag) break;
    }
    if (!l || !temp) {
        ERR("something is wrong!, parent(%p) does not have me(%p) \n",
                tag->parent, tag);
        return;
    }

    new_tag->parent = tag->parent;
    list_prepend(l, new_tag);
}
#endif

void xml_tag_change_string(XmlTag *tag, ...)
{
    va_list va;
    va_start(va, tag);

    char **olds = NULL;
    char **news = NULL;
    int num;
    for(num = 0 ; ; num++) {
        char *old = va_arg(va, char *);
        if (!old) break;
        char *neww = va_arg(va, char *);
        olds = (char **)realloc(olds, sizeof(char *) * (num + 1));
        news = (char **)realloc(news, sizeof(char *) * (num + 1));
        olds[num] = strdup(old);
        news[num] = strdup(neww);
    }
    va_end(va);

    if (num <= 0) return;

    List *l;
    XmlAttr *attr;
    LIST_FOR_EACH(tag->attrs, l, attr) {
        int j = 0;
        for (j = 0; j < num ; j++) {
            if (olds[j] && strstr(attr->val, olds[j])) {
                attr->val = str_replace(attr->val, olds[j], news[j]);
            }
        }
    }

    XmlTag *child;
    LIST_FOR_EACH(tag->children, l, child) {
        int i = 0;
        for (i = 0 ; i < num ; i++) {
            xml_tag_change_string(child, olds[i], news[i], NULL);
        }
    }

    int i = 0;
    for(i = 0 ; i < num; i++) {
        free(olds[i]);
        free(news[i]); ;
    }
    if (olds) free(olds);
    if (news) free(news);
}


void xml_tag_dump(XmlTag *root, int depth)
{
    int j = 0;
    for (j = 0 ; j < depth ; j++) {
        printf("    ");
    }
    printf("name: %s\n", root->name);

    XmlTag *child;
    XmlAttr *attr;
    List *l;
    LIST_FOR_EACH(root->attrs, l, attr) {
        for (j = 0 ; j < depth ; j++) {
            printf("        ");
        }
        printf("key: %s, value: %s\n", attr->key, attr->val);
    }

    for (j = 0 ; j < depth ; j++) {
        printf("        ");
    }
    printf("content: %s\n", root->content);
    LIST_FOR_EACH(root->children, l, child) {
        xml_tag_dump(child, depth + 1);
    }
}

static void xml_tag_write(FILE *fp, XmlTag *root, int depth)
{
    int j = 0;
    for (j = 0 ; j < depth ; j++) {
        fprintf(fp, "    ");
    }

    fprintf(fp, "<%s", root->name);

    XmlTag *child;
    XmlAttr *attr;
    List *l;
    LIST_FOR_EACH(root->attrs, l, attr) {
        fprintf(fp, " %s=\"%s\"", attr->key, attr->val);
    }

    if (root->children) {
        fprintf(fp, ">\n");
        LIST_FOR_EACH(root->children, l, child) {
            xml_tag_write(fp, child, depth + 1);
        }
        for (j = 0 ; j < depth ; j++) {
            fprintf(fp, "    ");
        }
    } else if (root->content) {
        fprintf(fp, ">");
        fprintf(fp, "%s", root->content);
    } else {
        fprintf(fp, "/>\n");
        return;
    }

    fprintf(fp, "</%s>\n", root->name);
}

static void _start_element(void *data, const char *name, const char **attrs)
{
    Xml *xml = (Xml *)data;

    if (xml->cur) {
        xml->parent = xml->cur;
    }

    xml->cur = xml_tag_create(name);

    if (!xml->root) xml->root = xml->cur;

    if (xml->parent) xml_tag_append_child(xml->parent, xml->cur);

    int i = 0;
    for (i = 0 ; attrs[i] ; i += 2) {
        xml_tag_append_attr(xml->cur, attrs[i], attrs[i+1]);
    }

    /*
    LOG("name: %s, parent: %s, root: %s",
        name, xml->parent?xml->parent->name:"null",
        xml->root?xml->root->name:"null");
        */
}

static void _end_element(void *data, const char *name)
{
    Xml *xml = (Xml *)data;
    if (!xml->cur) {
        xml->cur = xml->parent;
        xml->parent = xml->parent->parent;
    }
    /*
    LOG("name: %s, parent: %s, root: %s",
        name, xml->parent?xml->parent->name:"null",
        xml->root?xml->root->name:"null");
        */
    xml->cur = NULL;
}


static void _content(void *data, const char *content, int length)
{
    Xml *xml = (Xml *)data;
    if (!xml->cur) {
        return;
    }

    //ERR("%s", content);
    char *buf = (char *)malloc(length + 1);
    memset(buf, 0, length + 1);
    memcpy(buf, content, length);

    xml_tag_set_content(xml->cur, buf);
}

Xml *xml_load(const char *data, size_t size)
{
    Xml *xml = (Xml *)calloc(sizeof(Xml), 1);

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetEncoding(parser, "utf-8");
    XML_SetUserData(parser, xml);
    XML_SetElementHandler(parser, _start_element, _end_element);
    XML_SetCharacterDataHandler(parser, _content);
    if (XML_Parse(parser, data, size, XML_TRUE) == XML_STATUS_ERROR) {
        LOG("%s at line %lu\n",
                XML_ErrorString(XML_GetErrorCode(parser)),
                XML_GetCurrentLineNumber(parser));
        free(xml);
        return NULL;
    }
    XML_ParserFree(parser);

    return xml;
}

void xml_unload(Xml *xml)
{
    xml_tag_destroy(xml->root);
    free(xml);
}

Xml *xml_load_from_path(const char *path)
{
    RET_IF(!path, NULL);
    size_t size;

    if (!file_is_exist(path) || file_is_null(path)) return NULL;
    char *data = file_load(path, &size);
    if (!data) {
        ERR("file load failed: %s", path);
        return NULL;
    }

    Xml *xml = xml_load(data, size);
    if (!xml || !xml->root) {
        ERR("Something wrong with xml: %s", path);
        free(data);
        return NULL;
    }
    free(data);

    return xml;
}

List *_xml_search_tags(XmlTag *tag, List *names, List *matches)
{
    if (!strcmp(tag->name, (char *)names->data)) {
        if (names->next) {
            XmlTag *child;
            List *l;
            LIST_FOR_EACH(tag->children, l, child) {
                matches = _xml_search_tags(child, names->next, matches);
            }
        } else {
            matches = list_append(matches, tag);
        }
    }
    return matches;
}

List *xml_search_tags(Xml *xml, const char *xpath)
{
    List *names = NULL;

    char *_name;
    char *tok;
    _name = strdup(xpath);
    tok = strtok(_name, "/");
    while (tok) {
        names = list_append(names, strdup(tok));
        tok = strtok(NULL, "/");
    }
    free(_name);

    if (!names) return NULL;

    List *matches = NULL;
    matches = _xml_search_tags(xml->root, LIST_FIRST(names), NULL);

    LIST_FREE(names, _name) free(_name);

    return matches;
}

const char *xml_get_value(Xml *xml, const char *xpath, const char *name)
{
    List *temp;
    temp = xml_search_tags(xml, xpath);
    temp = LIST_LAST(temp);

    if (!temp) {
        //ERR("No %s in %s in configuration file", name, xpath);
        return NULL;
    }

    XmlTag *tag = (XmlTag *)LIST_DATA(temp);
    list_free(temp);

    return xml_tag_get_attr_val(tag, name);
}

void xml_append_tag(Xml *xml, const char *xpath, XmlTag *tag)
{
    List *tags = xml_search_tags(xml, xpath);
    if (!tags) {
        ERR("No %s exist", xpath);
        return;
    }
    if (list_count(tags) > 1) {
        ERR("xpath(%s)'s count is above one: %d", xpath, list_count(tags));
        return;
    }
    XmlTag *parent = LIST_DATA(LIST_FIRST(tags));
    xml_tag_append_child(parent, tag);
}

void xml_remove_tag(Xml *xml, const char *xpath)
{
    List *tags;
    tags = xml_search_tags(xml, xpath);
    if (tags) {
        XmlTag *tag;
        LIST_FREE(tags, tag) {
            xml_tag_remove_child(xml->root, tag);
            xml_tag_destroy(tag);
        }
    } else {
        ERR("No %s exists", xpath);
    }
}

void xml_dump(Xml *xml)
{
    xml_tag_dump(xml->root, 0);
}

void xml_write(Xml *xml, FILE *fp)
{
    xml_tag_write(fp, xml->root, 0);
}

#if 0
void xml_tag_convert_loop(XmlTag *loop)
{
    int begin = 0;
    int end = 0;
    const char *var = xml_tag_get_attr_val(loop, "var");
    const char *b = xml_tag_get_attr_val(loop, "begin");
    const char *e = xml_tag_get_attr_val(loop, "end");
    begin = atoi(b);
    end = atoi(e);

    if (!var) var = strdup("@loop");

    if (begin < 0 || end < 0) {
        ERR("begin and end value is not correct (should be bigger than 0), begin=%d, end=%d\n",
                begin, end);
        return;
    } else if (begin > end) {
        ERR("begin(%d) is bigger than end(%d)\n", begin, end);
        return;
    }

    while (begin <= end) {
        List *l;
        XmlTag *child;
        LIST_FOR_EACH(loop->children, l, child) {
            XmlTag *tag;
            tag = xml_tag_dup(child);
            xml_tag_append(loop, tag);

            const char *id = xml_tag_get_attr_val(tag, "id");
            char buf[1024];
            snprintf(buf, 1024, "%s%d", id, begin);
            xml_tag_change_attr(tag, "id", buf);

            snprintf(buf, 1024, "%d", begin);
            xml_tag_change_string(tag, var, buf, NULL);
        }
        begin++;
    }
}
#endif
