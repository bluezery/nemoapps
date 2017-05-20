#ifndef __XEMOUTIL_CONFIG_H__
#define __XEMOUTIL_CONFIG_H__

#include <xemoutil-xml.h>

typedef struct _Config Config;
struct _Config {
    char *id; // UUID
    char *path;
    int width, height;
    int min_width, min_height;
    int max_width, max_height;
    int exit_width, exit_height;
    int scene_width, scene_height;
    double sxy;
    int framerate;
    char *layer;
    bool enable_fullscreen;

    char *font_family, *font_style;
    int font_size;
};

#ifdef __cplusplus
extern "C" {
#endif

Xml *xml_load_from_domain(const char *domain, const char *filename);
void config_unload(Config *config);
Config *config_load_from_path(const char *path);
Config *config_load_from_domain(const char *domain, const char *filename);
Config *config_load(const char *domain, const char *filename, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
