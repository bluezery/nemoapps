#ifndef __MAP_H__
#define __MAP_H__

#define TOK_TO_MEMBER(ptr, member, func, tok) do { \
    tok = strtok(NULL, ","); \
    if (!tok) return ptr; \
    ptr->member = func(tok); \
}  while (0);


#define CHECK_TOK(tok, free0, free1) do { \
    if (!tok) { \
        ERR("No token; %s", str); \
        free(free0); \
        free(free1); \
        return NULL; \
    }  \
} while (0);


#define CSV_TOK_INIT(str, ptr, member, func) \
    char *__tmp = strdup(str); \
    char *__tok = strtok(__tmp, ","); \
    CHECK_TOK(__tok, ptr, __tmp); \
    ptr->member = func(__tok);

#define CSV_TOK(ptr, member, func) do { \
    TOK_TO_MEMBER(ptr, member, func, __tok); \
    CHECK_TOK(__tok, ptr, __tmp); \
} while (0);

#define CSV_TOK_SHUTDOWN() do {\
    free(__tmp); \
} while (0);

typedef struct _DataMapItem DataMapItem;
struct _DataMapItem {
    int id;
    char *name;
    int depth;
    struct {
        char *path;
        int x, y, w, h;
    } bg;
    struct {
        char *path;
        int x, y, w, h;
    } text;

    DataMapItem *parent;
    List *children;

    NemoWidgetItem *it;
    NemoWidgetItem *it_txt;
    int _new;

    uint32_t color;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline void data_map_item_dump(DataMapItem *it)
{
    printf("id: %d\n", it->id);
    printf("======================================\n");
    printf("name: %s\n", it->name);
    printf("depth: %d\n", it->depth);
    printf("bg path: %s\n",  it->bg.path);
    printf("x(%d) y(%d) w(%d) h(%d)\n", it->bg.x, it->bg.y, it->bg.w, it->bg.h);
    printf("text path: %s\n",  it->text.path);
    printf("x(%d) y(%d) w(%d) h(%d)\n", it->text.x, it->text.y, it->text.w, it->text.h);
    printf("new(%d)\n", it->_new);
    printf("======================================\n");
}

static inline DataMapItem *_csv_convert_mapdata_item(const char *str)
{
    DataMapItem *it = calloc(sizeof(DataMapItem), 1);

    CSV_TOK_INIT(str, it, id, atoi);
    CSV_TOK(it, name, strdup);
    CSV_TOK(it, depth, atoi);
    CSV_TOK(it, bg.path, strdup);
    CSV_TOK(it, bg.x, atoi);
    CSV_TOK(it, bg.y, atoi);
    CSV_TOK(it, bg.w, atoi);
    CSV_TOK(it, bg.h, atoi);
    CSV_TOK(it, text.path, strdup);
    CSV_TOK(it, text.x, atoi);
    CSV_TOK(it, text.y, atoi);
    CSV_TOK(it, text.w, atoi);
    CSV_TOK(it, text.h, atoi);
    CSV_TOK(it, _new, atoi);
    CSV_TOK_SHUTDOWN();
    //data_map_item_dump(it);

    return it;
}

static inline List *csv_load_datamap_items(const char *filename)
{
    List *items = NULL;

    int line_len;
    char **lines;
    lines = file_read_line(filename, &line_len);
    int i = 0 ;
    for (i = 0; i < line_len ; i++) {
        DataMapItem *it = _csv_convert_mapdata_item(lines[i]);
        items = list_append(items, it);
    }

    LOG("CSV file loaded: %s", filename);
    return items;
}

static inline void csv_calculate_datamap_items(List *items)
{
    List *l;
    DataMapItem *it_data;
    LIST_FOR_EACH(items, l, it_data) {
        List *ll;
        DataMapItem *itt_data;
        LIST_FOR_EACH(items, ll, itt_data) {
            if (it_data == itt_data) continue;

            // Temporary for MBC
            if (it_data->id == 1800) {
                int zc = itt_data->id/100;
                int ac = itt_data->id%100;
                if (ac == 0) continue;

                if (zc == 5 || zc == 13 || zc == 14) {
                    it_data->children = list_append(it_data->children, itt_data);
                    //itt_data->parent = it_data;
                }
                continue;
            } else if (it_data->id == 1900) {
                int zc = itt_data->id/100;
                int ac = itt_data->id%100;
                if (ac == 0) continue;

                if (zc == 3 || zc == 15) {
                    it_data->children = list_append(it_data->children, itt_data);
                    //itt_data->parent = it_data;
                }
                continue;
            } else if (it_data->depth == 23) {
                if (it_data->id == 961) { // Bucheon
                    if (itt_data->id == 915 || itt_data->id == 916 ||
                                itt_data->id == 917 || itt_data->id == 918) {
                        it_data->children = list_append(it_data->children, itt_data);
                    }
                } else if (it_data->id == 962) { // Ansan
                    if (itt_data->id == 925 || itt_data->id == 926 ||
                                itt_data->id == 927 || itt_data->id == 928) {
                        it_data->children = list_append(it_data->children, itt_data);
                    }
                } else if (it_data->id == 963) { // Anyang
                    if (itt_data->id == 912 || itt_data->id == 913 ||
                                itt_data->id == 914) {
                        it_data->children = list_append(it_data->children, itt_data);
                    }
                } else if (it_data->id == 964) { // Suwon
                    if (itt_data->id == 901 || itt_data->id == 902 ||
                                itt_data->id == 903 || itt_data->id == 904 ||
                                itt_data->id == 905) {
                        it_data->children = list_append(it_data->children, itt_data);
                    }
                } else if (it_data->id == 965) { // Sungnam
                    if (itt_data->id == 906 || itt_data->id == 907 ||
                                itt_data->id == 908 || itt_data->id == 909) {
                        it_data->children = list_append(it_data->children, itt_data);
                    }
                }
                continue;
            }

            if (it_data->depth + 1 == itt_data->depth) {
                int zc = itt_data->id/100;
                int ac = itt_data->id%100;

                if (ac == 0) continue;

                if (it_data->id/100 == zc) {
                    it_data->children = list_append(it_data->children, itt_data);
                    itt_data->parent = it_data;
                }
            }
        }
    }
    LOG("CSV file calculated");
}

#ifdef __cplusplus
}
#endif


#endif
