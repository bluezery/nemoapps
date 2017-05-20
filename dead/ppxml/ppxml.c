#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#include "xemoutil.h"

typedef struct _Context Context;
struct _Context {
    char *data;
    size_t size;

    List *defines;
};

static void
_xml_tag_convert_use(XmlTag *use, List *defines)
{
    const char *id = xml_tag_get_attr_val(use, "id");
    const char *src = xml_tag_get_attr_val(use, "src");
    const char **vars = NULL;

    int num = 0;
    const char *var;

    char buf[64];
    snprintf(buf, 64, "var%d", num);

    while ((var = xml_tag_get_attr_val(use, buf))) {
        vars = realloc(vars, sizeof(char *) * (num + 1));
        vars[num] = var;
        num++;
        snprintf(buf, 64, "var%d", num);
    }

    XmlTag *define;
    List *l;
    LIST_FOR_EACH(defines, l, define) {
        const char *_val = xml_tag_get_attr_val(define, "id");
        if (_val && !strcmp(_val, src)) break;
    }

    if (!define) {
        ERR("No definition(id:%s) define for use(id:%s)\n", src, id);
        return;
    }

    XmlTag *child;
    LIST_FOR_EACH(define->children, l, child) {
        XmlTag *new;
        new = xml_tag_dup(child);
        xml_tag_append(use, new);

        char buf[1024];
        const char *_id = xml_tag_get_attr_val(child, "id");
        snprintf(buf, 1024, "%s%s", id, _id);
        xml_tag_change_attr(new, "id", buf);

        // FIXME: Only for 1 depth children
        List *ll;
        XmlTag *cchild;
        LIST_FOR_EACH(new->children, ll, cchild) {
            _id = xml_tag_get_attr_val(cchild, "id");
            snprintf(buf, 1024, "%s%s", id, _id);
            xml_tag_change_attr(cchild, "id", buf);
        }

        int i = 0;
        for (i = 0 ; i < num ; i++) {
            snprintf(buf, 64, "@var%d", i);
            xml_tag_change_string(new, buf, vars[i], NULL);
        }

    }
}


static void
_xml_tag_parse(XmlTag *tag, Context *ctx)
{
    if (!strcmp(tag->name, "define")) {
        return;
    } else if (!strcmp(tag->name, "use")) {
        _xml_tag_convert_use(tag, ctx->defines);
        return;
    } else if (!strcmp(tag->name, "loop")) {
        xml_tag_convert_loop(tag);
        return;
    }

    XmlTag *child;
    List *l;
    LIST_FOR_EACH(tag->children, l, child) {
        _xml_tag_parse(child, ctx);
    }
}

// *** Temporarily Fix **************************//
static char *_file_load(const char *file, size_t *sz)
{
    char *buf;

    FILE *fp;
    long len;
    size_t size;
    fp = fopen(file, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);

    buf = calloc(sizeof(char), len + 1);
    size = fread(buf, 1, len, fp);
    if (size != len) {
        ERR("fread error\n");
        free(buf);
        return NULL;
    }

    fclose(fp);

    if (sz) *sz = size;
    return buf;
}


int main(int argc, const char *argv[])
{
    if ((argc < 2) || !argv[1]) {
        ERR("Usage: %s [xml file]\n", argv[0]);
        return -1;
    }

    Context *ctx;
    ctx = calloc(sizeof(Context), 1);

    ctx->data = _file_load(argv[1], &ctx->size);
    if (!ctx->data || (ctx->size <= 0)) {
        fprintf(stderr, "load file (%s) failed\n", argv[1]);
        return -1;
    }

    Xml *xml = xml_load(ctx->data, ctx->size);
    if (!xml) {
        fprintf(stderr, "xml_load failed\n");
        return -1;
    }

    ctx->defines = xml_search_tags(xml, "show/define");

    _xml_tag_parse(xml->root, ctx);
    xml_tag_printf(stdout, xml->root, 0);

    xml_unload(xml);

    if (ctx->data) free(ctx->data);
    free(ctx);


    return 0;
}
