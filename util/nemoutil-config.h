#ifndef __UTIL_CONFIG_H__
#define __UTIL_CONFIG_H__

#include <nemoutil-xml.h>

typedef struct _Config Config;
struct _Config {
    char *id; // UUID
    char *path;
    int width, height;
    int min_width, min_height;
    int max_width, max_height;
    int exit_width, exit_height;
    int scene_width, scene_height;
    int framerate;
    char *font_family, *font_style;
    int font_size;
    char *layer;
    double sxy;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline void config_dump(Config *config)
{
    ERR("[%s][%s](%d, %d),(%d, %d),(%d, %d),(%d, %d),(%d, %d), %d, %s",
            config->id, config->path,
            config->width, config->height,
            config->min_width, config->min_height,
            config->max_width, config->max_height,
            config->exit_width, config->exit_height,
            config->scene_width, config->scene_height,
            config->framerate, config->layer);
}

Xml *xml_load_from_domain(const char *domain, const char *filename);
void config_unload(Config *config);
Config *config_load_from_path(const char *path);
Config *config_load_from_domain(const char *domain, const char *filename);
Config *config_load(const char *domain, const char *filename, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
