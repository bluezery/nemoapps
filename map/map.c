#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "util.h"
#include "widgets.h"
#include "nemoui.h"
#include ""
#include "nemohelper.h"

#define TILEITEM_SIZE 255

const char *PIN = "M141.938,324.965L11.819,484.023c-3.387,4.146-3.517,10.24,0,14.535c4.002,4.918,11.247,5.646,16.147,1.625 l159.107-130.131c-8.13-7.24-15.955-14.598-23.244-21.893C156.199,340.533,148.909,332.758,141.938,324.965z M447.89,64.1C403.4,19.598,353.134-2.246,335.581,15.254c-0.903,0.908-92.757,126.197-138.796,188.98 c-30.166-21.5-57.219-34.082-74.656-34.068c-6.374,0-11.485,1.662-14.971,5.18c-19.197,19.164,16.646,86.076,80.028,149.459 c51.797,51.797,105.971,85.223,134.488,85.223c6.387,0,11.472-1.68,14.971-5.178c13.084-13.084,0.548-48.41-28.858-89.658 c62.802-46.043,188.047-137.893,188.932-138.781C514.254,158.893,492.38,108.607,447.89,64.1z";

typedef struct _MapView MapView;
typedef struct _MapTile MapTile;

struct _MapView {
    int width, height;
    struct nemotool *tool;
    struct nemoshow *show;

    NemoWidget *widget;
    struct showone *group;
    char *filepath;
    bool dirty;

    int tx, ty; // tile geometry for viewport
    int dx, dy;

    double zoom;
    double lon, lat;
    Inlist *tiles;
};

struct _MapTile {
    Inlist __inlist;
    MapView *view;
    struct showone *one;

    int x, y; // tile geometry
    int w, h;
    char *url;
    char *path;

    Con *con;
};

// Refer : http://wiki.openstreetmap.org/wiki/FAQ
// meters per pixel when latitude is 0 (equator)
// meters per pixel  = _osm_scale_meter[zoom] * cos (latitude)
const double _osm_scale_meter[] =
{
   78206, 39135.758482, 19567.879241, 9783.939621, 4891.969810,
   2445.984905, 1222.992453, 611.496226, 305.748113, 152.874057, 76.437028,
   38.218514, 19.109257, 9.554629, 4.777314, 2.388657, 1.194329, 0.597164,
   0.29858
};

// Scale in meters
const double _scale_tb[] =
{
   10000000, 5000000, 2000000, 1000000, 500000, 200000, 100000, 50000,
   20000, 10000, 5000, 2000, 1000, 500, 500, 200, 100, 50, 20, 10, 5, 2, 1
};

// URL References
// http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
// http://wiki.openstreetmap.org/wiki/MapTileserver
static double
_scale_cb(double lon, double lat, int zoom)
{
   if ((zoom < 0) ||
       (zoom >= (int)(sizeof(_osm_scale_meter) / sizeof(_osm_scale_meter[0])))
      )
     return 0;
   return _osm_scale_meter[zoom] / cos(lat * M_PI / 180.0);
}

static char *
_map_url_get_mapnik(int x, int y, int zoom)
{
   char buf[PATH_MAX];
   // ((x+y+zoom)%3)+'a' is requesting map images from distributed
   // tile servers (eg., a, b, c)
   snprintf(buf, sizeof(buf),
           "http://%c.tile.openstreetmap.org/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_map_url_get_osmarender(int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://%c.tah.openstreetmap.org/MapTiles/tile/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_map_url_get_cyclemap(int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://%c.tile.opencyclemap.org/cycle/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_map_url_get_mapquest_aerial(int x, int y, int zoom)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
           "http://oatile%d.mqcdn.com/naip/%d/%d/%d.png",
            ((x + y + zoom) % 4) + 1, zoom, x, y);
   return strdup(buf);
}

static char *
_map_url_get_mapquest(unsigned int zoom, unsigned int x, unsigned int y)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf),
            "http://otile%d.mqcdn.com/tiles/1.0.0/osm/%d/%d/%d.png",
            ((x + y + zoom) % 4) + 1, zoom, x, y);
   return strdup(buf);
}

static void
map_tilecoord_to_region(unsigned int zoom,
                         int x, int y,
                         double *lon, double *lat)
{
    double size = pow(2, zoom) * TILEITEM_SIZE;

    LOG("%lf", size);
    // Referece: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    if (lon) *lon = (x / size * 360.0) - 180;
    if (lat) {
        double n = M_PI - (2.0 * M_PI * y / size);
        *lat = 180.0 / M_PI *atan(0.5 * (exp(n) - exp(-n)));
    }
}

static void
mapview_region_to_tilecoord(double zoom,
                         double lon, double lat,
                         int *x, int *y)
{
    double size = pow(2, zoom) * TILEITEM_SIZE;

    if (x) *x = floor((lon + 180.0) / 360.0 * size);
    if (y) {
        *y = floor((1.0 - log(tan(lat * M_PI / 180.0)
                              + (1.0 / cos(lat * M_PI / 180.0)))
                           / M_PI) / 2.0 * size);
    }
}

static bool
_mapview_tile_load(MapTile *tile)
{
    MapView *view = tile->view;
    if (tile->con) {
        con_destroy(tile->con);
        tile->con = NULL;
    }
    if (!tile->one) {
        tile->one = IMAGE_CREATE(view->group, tile->w, tile->h,  tile->path);
        if (!tile->one) {
            ERR("tile load failed");
            if (remove(tile->path) < 0)
                ERR("file deletion failed: %s :%s", strerror(errno), tile->path);
            return false;
        }
        //nemoshow_item_set_clip(tile->one, view->clip);
        nemoshow_item_translate(tile->one, tile->x - tile->view->tx, tile->y - tile->view->ty);
        nemoshow_dispatch_frame(view->show);
    }
    return true;
}

static void
_mapview_download_end(Con *con, bool success, char *data, size_t size, void *userdata)
{
    MapTile *tile = userdata;

    if (success) {
        _mapview_tile_load(tile);
    } else {
        ERR("download failed");
        if (remove(tile->path) < 0)
            ERR("file deletion failed: %s :%s", strerror(errno), tile->path);
    }
    tile->con = NULL;
}

static void
mapview_update(MapView *view)
{
    ERR("1.???");
    //if (!view->dirty) return;
    ERR("2.???");
    int x, y;
    // Calculate tile coordinates from the given geometry.
    mapview_region_to_tilecoord(view->zoom, view->lon, view->lat, &x, &y);

    // Calculate tile coordinates of the viewport.
    view->tx = x - view->width/2;
    view->ty = y - view->height/2;

    x = view->tx + view->width - view->dx;
    y = view->ty + view->height - view->dy;
    int tix, tiy;
    int tx, ty, tw, th;

    // Locate left, top first tile crossing with the viewport
    tix = (view->tx - view->dx)/TILEITEM_SIZE;
    if (tix < 0) tix = 0;
    tiy = (view->ty - view->dy)/TILEITEM_SIZE;
    if (tiy < 0) tiy = 0;
    tx = tix * TILEITEM_SIZE;
    ty = tiy * TILEITEM_SIZE;
    tw = TILEITEM_SIZE;
    th = TILEITEM_SIZE;

    // Request tiles right, down direction
    while (x >= tx && (tix <= pow(2, view->zoom) -1 )) {
        int ttiy = tiy;
        while (y >= ty && (tiy <= (pow(2, view->zoom) - 1))) {
            MapTile *tmp;
            MapTile *tile = NULL;
            INLIST_FOR_EACH(view->tiles, tmp) {
                if ((tmp->x == tx) && (tmp->y == ty)) {
                    tile = tmp;
                    break;
                }
            }
            if (tile) {
                if (tile->con && !tile->one) { // Downloading
                    tiy++;
                    ty = tiy * TILEITEM_SIZE;
                    continue;
                }
                else if (_mapview_tile_load(tile)) {
                    tiy++;
                    ty = tiy * TILEITEM_SIZE;
                    continue;
                } else {
                    ERR("Request a file again");
                }
            }

            if (!tile) {
                tile = calloc(sizeof(MapTile), 1);
                tile->x = tx;
                tile->y = ty;
                tile->w = tw;
                tile->h = th;
                tile->view = view;
                tile->path = strdup_printf
                    ("%s/%0.2lf.%d.%d.jpg", view->filepath, view->zoom, tix, tiy);
                tile->url = _map_url_get_mapnik(view->zoom, tix, tiy);
                view->tiles = inlist_append(view->tiles, INLIST(tile));
            }

            if (file_is_exist(tile->path)) {
                if (_mapview_tile_load(tile)) {
                    tiy++;
                    ty = tiy * TILEITEM_SIZE;
                    continue;
                } ERR ("Request file again");
            }
            // Request view tiles
            if (tile->con) con_destroy(tile->con);
            Con *con;
            con = con_create(view->tool);
            con_set_url(con, tile->url);
            con_set_file(con, tile->path);
            con_set_end_callback(con, _mapview_download_end, tile);
            con_run(con);
            tile->con = con;

            tiy++;
            ty = tiy * TILEITEM_SIZE;
        }
        tiy = ttiy;
        ty = tiy * TILEITEM_SIZE;
        tix++;
        tx = tix * TILEITEM_SIZE;
    }

    MapTile *tile, *tmp;
    INLIST_FOR_EACH_SAFE(view->tiles, tmp, tile) {
        if ((x < tile->x) || (y < tile->y)) {
            if (tile->con) con_destroy(tile->con);
            if (tile->one) nemoshow_item_destroy(tile->one);
            free(tile->url);
            free(tile->path);
            view->tiles = inlist_remove(view->tiles, INLIST(tile));
            free(tile);
        }
    }

    view->dirty = false;
    nemoshow_item_translate(view->group, view->dx, view->dy);
}

static void
mapview_move(MapView *view, double x, double y)
{
    view->dx += x;
    view->dy += y;
    view->dirty = true;
}

static void
mapview_resize(MapView *view, int width, int height)
{
#if 0
    nemotale_path_translate
        (view->group,
         NTPATH_TRANSFORM_X(view->group) + (w - view->width)/2.0,
         NTPATH_TRANSFORM_Y(view->group) + (h - view->height)/2.0);
    nemotale_path_scale
        (view->group,
         NTPATH_TRANSFORM_SX(view->group) * (double)w/view->width,
         NTPATH_TRANSFORM_SY(view->group) * (double)h/view->height);

    nemotale_path_scale
        (view->clip,
         NTPATH_TRANSFORM_SX(view->clip) * (double)w/view->width,
         NTPATH_TRANSFORM_SY(view->clip) * (double)h/view->height);
#endif

    view->width = width;
    view->height = height;
    view->dirty = true;

    ERR("resizing");
    // While resizing, dispatch frame callack will not be called.
    mapview_update(view);
}

static void
mapview_set_zoom(MapView *view, int zoom)
{
    if (view->zoom == zoom) return;
    if (zoom > 19) return;
    if (zoom < 1) return;
    ERR("%d", zoom);
    view->zoom = zoom;
    view->dirty = true;
}

static void
mapview_set_geo(MapView *view, double lon, double lat)
{
    view->lon = lon;
    view->lat = lat;
    view->dirty = true;
}

static void
mapview_destroy(MapView *view)
{
    free(view->filepath);
    nemoshow_one_destroy(view->group);
    nemowidget_destroy(view->widget);

    MapTile *tile, *tmp;
    INLIST_FOR_EACH_SAFE(view->tiles, tmp, tile) {
        if (tile->con) con_destroy(tile->con);
        if (tile->one) nemoshow_item_destroy(tile->one);
        free(tile->url);
        free(tile->path);
        view->tiles = inlist_remove(view->tiles, INLIST(tile));
        free(tile);
    }

    free(view);
}

void mapview_show(MapView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_show(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 1.0);
    nemoshow_dispatch_frame(view->show);
    mapview_update(view);
}

void mapview_hide(MapView *view, uint32_t easetype, int duration, int delay)
{
    nemowidget_hide(view->widget, 0, 0, 0);
    nemowidget_set_alpha(view->widget, easetype, duration, delay, 0.0);
    nemoshow_dispatch_frame(view->show);
}

MapView *mapview_create(NemoWidget *parent, int width, int height)
{
    MapView *view = calloc(sizeof(MapView), 1);
    view->tool = nemowidget_get_tool(parent);
    view->show = nemowidget_get_show(parent);
    view->width = width;
    view->height = height;

    // Set default download direcotry
    const char *home = getenv("HOME");
    char *filepath;
    if (home) filepath = strdup_printf("%s/.%s", home, APPNAME);
    else      filepath = strdup_printf("/tmp/.%s", APPNAME);
    if (!file_mkdir(filepath, 0755)) {
        ERR("file mkdir failed: %s", filepath);
        free(filepath);
        free(view);
        return NULL;
    }
    view->filepath = filepath;

    NemoWidget *widget;
    struct showone *group;

    view->widget = widget = nemowidget_create_vector(parent, width, height);
    nemowidget_set_alpha(widget, 0, 0, 0, 0.0);
    view->group = group = GROUP_CREATE(nemowidget_get_canvas(widget));

    struct showone *one;
    one = RRECT_CREATE(group, width, height, 3, 3);
    nemoshow_item_set_fill_color(one, RGBA(WHITE));

#if 0
    double r = 0;
    view->clip = clip = PATH_CREATE(NULL);
    nemoshow_item_set_fill_color(clip, RGBA(WHITE));
    nemoshow_item_set_alpha(clip, 0.0);

    nemoshow_item_path_moveto (clip, x+r, y, 1, 1);
    nemoshow_item_path_lineto (clip, x+w-r, y, 1, 1);
    nemoshow_item_path_cubicto(clip, x+w, y, x+w, y, x+w, y+r, 1, 1);
    nemoshow_item_path_lineto (clip, x+w, y+h-r, 1, 1);
    nemoshow_item_path_cubicto(clip, x+w, y+h, x+w, y+h, x+w-r, y+h, 1, 1);
    nemoshow_item_path_lineto (clip, x+r, y+h, 1, 1);
    nemoshow_item_path_cubicto(clip, x, y+h, x, y+h, x, y+h-r, 1, 1);
    nemoshow_item_path_lineto (clip, x, y+r, 1, 1);
    nemoshow_item_path_cubicto(clip, x, y, x, y, x+r, y, 1, 1);
    nemoshow_item_path_close(clip, 1, 1);
#endif

    return view;
}

#if 0
static void _tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
    nemotale_event_update_node_taps(tale, node, event, type);
    int id = nemotale_node_get_id(node);
    if (id != 1) return;

    struct nemoshow *show = nemotale_get_userdata(tale);
    struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);
    Context *ctx = nemoshow_get_userdata(show);
    MapAppView *view = ctx->view;
    MapView *map = view->map;
    if (ctx->noevent) return;

    if (!view->pinned) {
        if (nemoshow_event_is_down(tale, event, type) ||
                nemoshow_event_is_up(tale, event, type)) {
            if (nemoshow_event_is_single_tap(tale, event, type)) {
                nemocanvas_move(canvas, event->taps[0]->serial);
            } else if (nemotale_is_double_taps(tale, event, type)) {
                nemocanvas_pick(canvas,
                        event->taps[0]->serial,
                        event->taps[1]->serial,
                        (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_SCALE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
            } else if (nemotale_is_many_taps(tale, event, type)) {
                nemotale_event_update_faraway_taps(tale, event);
                nemocanvas_pick(canvas,
                        event->tap0->serial,
                        event->tap1->serial,
                        (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) |
                        (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
            }
        }
    }

    if (nemoshow_event_is_down(tale, event, type)) {
        if (nemoshow_event_is_single_tap(tale, event, type)) {
            ctx->prev_event_x = -9999;
            ctx->prev_event_y = -9999;
        } else if (nemotale_is_double_taps(tale, event, type)) {
            ERR("zoom start");
            ctx->zoom = map->zoom;
            ctx->diff_start =
                sqrt(pow(event->taps[0]->x - event->taps[1]->y, 2) +
                        pow(event->taps[0]->y - event->taps[1]->y, 2));
        }
    } else if (nemotale_is_motion_event(tale, event, type)) {
        if (ctx->prev_event_x == -9999 ||
                ctx->prev_event_y == -9999) {
            ctx->prev_event_x = event->taps[0]->x;
            ctx->prev_event_y = event->taps[0]->y;
            return;
        }

        if (view->pinned) {
            double pos_x = event->taps[0]->x - ctx->prev_event_x;
            double pos_y = event->taps[0]->y - ctx->prev_event_y;
            ctx->prev_event_x = event->taps[0]->x;
            ctx->prev_event_y = event->taps[0]->y;
            mapview_move(map, pos_x, pos_y);
            if (event->tapcount >= 2) {
                double diff =
                    sqrt(pow(event->taps[0]->x - event->taps[1]->y, 2) +
                            pow(event->taps[0]->y - event->taps[1]->y, 2));
                double zoom_diff = diff - ctx->diff_start;
                mapview_set_zoom(map, ctx->zoom + (int)(zoom_diff/25));
            }
        }
    }
    if (nemoshow_event_is_single_click(tale, event, type)) {
        if (_is_pin(view, event->x, event->y)) {
            _pin_change(view);
        }

    }
}
#endif

#if 0
static void _dispatch(struct nemoshow *show, void *userdata)
{
    Context *ctx = userdata;
    if (!ctx->view) return;
    mapview_update(ctx->view->map);
    nemoshow_render_one(ctx->view->show);
}
#endif


int main(int argc, char *argv[])
{
    Config *config = config_load(PROJECT_NAME, APPNAME, CONFXML, argc, argv);
    RET_IF(!config, -1);

    con_init();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win_base(tool, APPNAME, config);
    RET_IF(!win, -1);

    MapView *view = mapview_create(win, config->width, config->height);
    mapview_show(view, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 0);
#if 0

    ctx = calloc(sizeof(Context), 1);
    RET_IF(!ctx, -1);

    if (!_config_load(ctx, &width, &height)) return -1;

    NemoWidget *win;
    win = nemowidget_create_win(width, height);
    nemowidget_win_load_scene(win, ctx->width, ctx->height);
    ctx->win = win;
    ctx->tool = nemowidget_get_tool(win);

    NemoWidget *empty;
    empty = nemowidget_create_empty(win, ctx->width, ctx->height);
    nemowidget_empty_set_type(empty, NEMOWIDGET_EMPTY_TYPE_VECTOR);

    struct showone *canvas;
    show = nemowidget_get_show(empty);
    canvas = nemowidget_get_canvas(empty);

    ctx->view = mapview_create(tool, show, canvas, show->width, show->height);


    free(ctx);
#endif
    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    con_shutdown();
    config_unload(config);

    return 0;
}

