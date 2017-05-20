#ifndef __XEMOUTIL_PATH_H__
#define __XEMOUTIL_PATH_H__

#include <stdbool.h>
#include <xemoutil-list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PathItem PathItem;
struct _PathItem {
    char type;
    double rxy[2]; // temporary
    double xy[6];
    int cnt;
};

typedef struct _Path Path;
struct _Path {
    char *uri;
    int x, y, width, height;
    bool is_fill;
    uint32_t fill;
    bool is_stroke;
    uint32_t stroke;
    int stroke_width;

    List *items;
    char *d;
};

typedef struct _PathCoord PathCoord;
struct _PathCoord
{
    double x;
    double y;
    double z;
};

void path_destroy(Path *path);
PathCoord bezier(PathCoord *p, int n, double u);
PathCoord quad_bezier(PathCoord p0, PathCoord p1, PathCoord p2, double u);
List *path_get_median(Path *path0, Path *path1, int cnt);
Path *path_get_near(Path *path0, Path *path1);
List *svg_parse_items(const char *str);
uint32_t svg_parse_color(const char *str);
Path *svg_get_path(const char *uri);

#ifdef __cplusplus
}
#endif

#endif
