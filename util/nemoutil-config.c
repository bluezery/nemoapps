#include "nemoutil-internal.h"
#include "nemoutil.h"
#include "nemoutil-config.h"

Xml *xml_load_from_domain(const char *domain, const char *filename)
{
    char buf[PATH_MAX];
    const char *homedir = file_get_homedir();

    snprintf(buf, PATH_MAX, "%s/.config/%s/%s", homedir, domain, filename);
    Xml *xml = xml_load_from_path(buf);

    if (!xml) {
        snprintf(buf, PATH_MAX, "/etc/%s/%s", domain, filename);
        xml = xml_load_from_path(buf);
    }

    return xml;
}

void config_unload(Config *config)
{
    if (config->id) free(config->id);
    if (config->path) free(config->path);
    if (config->layer) free(config->layer);
    if (config->font_family) free(config->font_family);
    if (config->font_style) free(config->font_style);
    free(config);
}

static void _config_override(Config *config, Xml *xml)
{
    double sx = 1.0, sy = 1.0;
    double max_sx = 0.0, max_sy = 0.0;
    double min_sx = 0.0, min_sy = 0.0;
    double exit_sx = 0.0, exit_sy = 0.0;

    const char *prefix = "config";
    char buf[PATH_MAX];
    const char *temp;

    if (!xml_search_tags(xml, prefix)) {
        ERR("%s tag does not exist in the xml", prefix);
        return;
    }

    snprintf(buf, PATH_MAX, "%s/window", prefix);
    temp = xml_get_value(xml, buf, "layer");
    if (temp && strlen(temp) > 0) {
        if (config->layer) free(config->layer);
        config->layer = strdup(temp);
    }
    temp = xml_get_value(xml, buf, "enable_fullscreen");
    if (temp && strlen(temp) > 0) {
        if (!strcasecmp(temp, "true")) {
            config->enable_fullscreen = true;
        }
    }
    temp = xml_get_value(xml, buf, "width");
    if (temp && strlen(temp) > 0)  config->width = atoi(temp);
    temp = xml_get_value(xml, buf, "height");
    if (temp && (strlen(temp) > 0)) config->height = atoi(temp);

    temp = xml_get_value(xml, buf, "sx");
    if (temp && (strlen(temp) > 0)) sx = atof(temp);
    temp = xml_get_value(xml, buf, "sy");
    if (temp && (strlen(temp) > 0)) sy = atof(temp);

    temp = xml_get_value(xml, buf, "min_sx");
    if (temp && (strlen(temp) > 0)) min_sx = atof(temp);
    temp = xml_get_value(xml, buf, "min_sy");
    if (temp && (strlen(temp) > 0)) min_sy = atof(temp);

    temp = xml_get_value(xml, buf, "max_sx");
    if (temp && (strlen(temp) > 0)) max_sx = atof(temp);
    temp = xml_get_value(xml, buf, "max_sy");
    if (temp && (strlen(temp) > 0)) max_sy = atof(temp);

    temp = xml_get_value(xml, buf, "exit_sx");
    if (temp && (strlen(temp) > 0)) exit_sx = atof(temp);
    temp = xml_get_value(xml, buf, "exit_sy");
    if (temp && (strlen(temp) > 0)) exit_sy = atof(temp);

    snprintf(buf, PATH_MAX, "%s/scene", prefix);
    temp = xml_get_value(xml, buf, "width");
    if (temp && (strlen(temp) > 0)) config->scene_width = atoi(temp);
    temp = xml_get_value(xml, buf, "height");
    if (temp && (strlen(temp) > 0)) config->scene_height = atoi(temp);

    snprintf(buf, PATH_MAX, "%s/frame", prefix);
    temp = xml_get_value(xml, buf, "rate");
    if (temp && (strlen(temp) > 0)) config->framerate = atoi(temp);

    snprintf(buf, PATH_MAX, "%s/font", prefix);
    temp = xml_get_value(xml, buf, "family");
    if (temp && (strlen(temp) > 0)) config->font_family = strdup(temp);
    temp = xml_get_value(xml, buf, "style");
    if (temp && (strlen(temp) > 0)) config->font_style = strdup(temp);
    temp = xml_get_value(xml, buf, "size");
    if (temp && (strlen(temp) > 0)) config->font_size = atoi(temp);

    config->width = (double)config->width * sx;
    config->height = (double)config->height * sy;
    config->min_width = (double)config->width * min_sx;
    config->min_height = (double)config->height * min_sy;
    config->max_width = (double)config->width * max_sx;
    config->max_height = (double)config->height * max_sy;
    config->exit_width = (double)config->width * exit_sx;
    config->exit_height = (double)config->height * exit_sy;
}

Config *config_load_from_path(const char *path)
{
    Config *config = calloc(sizeof(Config), 1);
    config->width = 512;
    config->height = 512;
    config->framerate = 60;

    Xml *xml;
    xml = xml_load_from_path(path);
    if (!xml) {
        ERR("Load configuration failed: %s", path);
    } else {
        _config_override(config, xml);
        xml_unload(xml);
    }

    if (config->min_width > config->exit_width) {
        config->exit_width = config->min_width;
    }
    if (config->min_height > config->exit_height) {
        config->exit_height = config->min_height;
    }

    return config;
}

Config *config_load_from_domain(const char *domain, const char *filename)
{
    Config *config = calloc(sizeof(Config), 1);
    config->width = 512;
    config->height = 512;
    config->framerate = 60;
    config->sxy = 1.0;

    Xml *xml;
    xml = xml_load_from_domain(domain, "base.conf");
    if (!xml) {
        ERR("Load configuration failed: %s", "base.conf");
    } else {
        _config_override(config, xml);
        xml_unload(xml);
    }

    xml = xml_load_from_domain(domain, filename);
    if (!xml) {
        ERR("Load configuration failed: %s:%s", domain, filename);
    } else {
        _config_override(config, xml);
        xml_unload(xml);
    }

    if (config->min_width > config->exit_width) {
        config->exit_width = config->min_width;
    }
    if (config->min_height > config->exit_height) {
        config->exit_height = config->min_height;
    }

    return config;
}

static char *get_configpath_from_parameter(int argc, char *argv[])
{
    char *configpath = NULL;
    struct option options[] = {
        {"config", required_argument, NULL, 'c'},
        { NULL }
    };

    char **_argv = malloc(sizeof(char *) * argc);
    int i = 0;
    for (i = 0 ; i < argc ; i++) {
        _argv[i] = strdup(argv[i]);
    }

    optind = 1;
    opterr = 0;
    int opt;
    while ((opt = getopt_long(argc, _argv, "c:", options, NULL)) != -1) {
        switch(opt) {
            case 0:
                ERR("xxx");
                break;
            case 'c':
                configpath = strdup(optarg);
                break;
            default:
                break;
        }
    }
    for (i = 0 ; i < argc ; i++) {
        free(_argv[i]);
    }
    free(_argv);

    optind = 1;
    return configpath;

}

static void config_override_from_parameter(Config *config, int argc, char *argv[])
{
    struct option options[] = {
        {"id", required_argument, NULL, 'i'},
        {"width", required_argument, NULL, 'w'},
        {"height", required_argument, NULL, 'h'},
        {"framerate", required_argument, NULL, 'r'},
        {"layer", required_argument, NULL, 'y'},
        { NULL }
    };

    char **_argv = malloc(sizeof(char *) * argc);
    int i = 0;
    for (i = 0 ; i < argc ; i++) {
        _argv[i] = strdup(argv[i]);
    }

    int width = 0;
    int height = 0;
    optind = 1;
    opterr = 0;
    int opt;
    while ((opt = getopt_long(argc, _argv, "i:w:h:r:y:", options, NULL)) != -1) {
        switch(opt) {
            case 'i':
                if (config->id) free(config->id);
                config->id = strdup(optarg);
                break;
            case 'w':
                width = atoi(optarg);
                break;
            case 'h':
                height = atoi(optarg);
                break;
            case 'r':
                config->framerate = atoi(optarg);
                break;
            case 'y':
                if (config->layer) free(config->layer);
                config->layer = strdup(optarg);
                break;
            default:
                break;
        }
    }
    for (i = 0 ; i < argc ; i++) {
        free(_argv[i]);
    }
    free(_argv);

    optind = 1;

    double sx = 1.0;
    double sy = 1.0;
    if (width > 0) {
        sx = (double)width/config->width;
        config->width = width;
    }
    if (height > 0) {
        sy = (double)height/config->height;
        config->height = height;
    }
    if (sx > sy) config->sxy = sy;
    else config->sxy = sx;
}

Config *config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    char *configpath = get_configpath_from_parameter(argc, argv);

    Config *config;
    if (configpath) {
        config = config_load_from_path(configpath);
    } else {
        config = config_load_from_domain(domain, filename);
    }
    config->path = configpath;
    config_override_from_parameter(config, argc, argv);

    return config;
}

