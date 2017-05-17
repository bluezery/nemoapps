//#include <SkData.h>
//#include <SkRefCnt.h>
//#include <SkTemplates.h>
//#include <SkBitmap.h>
//#include <SkCodec.h>

#include <nemoshow.h>
#include <showitem.hpp>
#include "nemowrapper.h"
#include "nemoutil.h"
#include "nemoui-thread.h"
#include "nemoui-image.h"
#include "skiahelper.hpp"

struct _ImageBitmap {
    SkiaBitmap *bitmap;
};

struct _ImageLoader {
    Image *img;
    char *uri;
    int width, height;
    ImageBitmap *bitmap;
    ImageLoadDone done;
    void *userdata;
    Thread *thread;
};

struct _Image {
    ImageLoader *loader;
    struct showone *group;
    struct showone *one;
    ImageBitmap *bitmap;
    struct showone *clip;
};

ImageBitmap *image_bitmap_create(const char *path)
{
    RET_IF(!path, NULL);
    if (!file_is_exist(path)) {
        ERR("file does not exist: %s", path);
        return NULL;
    }
    if (file_is_null(path)) {
        ERR("file is NULL: %s", path);
        return NULL;
    }
    if (!file_is_image(path)) {
        ERR("file is not image ilfe: %s", path);
        return NULL;
    }

    ImageBitmap *bitmap = (ImageBitmap *)calloc(sizeof(ImageBitmap), 1);
    bitmap->bitmap = skia_bitmap_create(path);
    if (!bitmap->bitmap) {
        ERR("image decode skbitmap failed, trying Magickwand");

#if 0 // XXX: Use another decoder
        MagickBooleanType status;
        MagickWand *wand;
        MagickWandGenesis();
        wand = NewMagickWand();
        status = MagickReadImage(wand, argv[1]);
        if (status == MagickFalse) {
            ERR("MagickReadImage faied: %s", path);
            free(bitmap);
            return NULL;
        }

        size_t len;
        unsigned char *blob = NULL;
        blob = MagickGetImageBlob(wand, &len);
        RGBA2ARGB(blob, len);

        bitmap->setInfo(codec->getInfo());
        bitmap->allocPixels();
#endif
        free(bitmap);

        return NULL;
    }

    return bitmap;
}

ImageBitmap *image_bitmap_dup(ImageBitmap *src)
{
    ImageBitmap *dst = (ImageBitmap *)calloc(sizeof(ImageBitmap), 1);
    dst->bitmap = skia_bitmap_dup(src->bitmap);

    return dst;
}

void image_bitmap_destroy(ImageBitmap *bitmap)
{
    skia_bitmap_destroy(bitmap->bitmap);
    free(bitmap);
}

ImageBitmap *image_get_bitmap(Image *img)
{
    return img->bitmap;
}

void image_set_bitmap(Image *img, int width, int height, ImageBitmap *bitmap)
{
    if (img->bitmap) {
        image_bitmap_destroy(img->bitmap);
    }
    img->bitmap = bitmap;
    nemoshow_item_set_width(img->one, width);
    nemoshow_item_set_height(img->one, height);
    nemoshow_item_set_width(img->group, width);
    nemoshow_item_set_height(img->group, height);
    SkBitmap *skbitmap = skia_bitmap_get_bitmap(bitmap->bitmap);
    nemoshow_item_set_bitmap(img->one, skbitmap);
}

static void _image_decode_done(bool cancel, void *userdata)
{
    ImageLoader *loader = (ImageLoader *)userdata;
    Image *img = loader->img;

    if (!cancel) {
        if (!loader->bitmap) {
            ERR("bitmap is not loaded correctly: %s", loader->uri);
        } else {
            image_set_bitmap(img, loader->width, loader->height,
                    loader->bitmap);
            if (loader->done) loader->done(true, loader->userdata);
        }
    } else {
        if (loader->bitmap) {
            image_bitmap_destroy(loader->bitmap);
        }
        if (loader->done) loader->done(false, loader->userdata);
    }
    img->loader = NULL;
    free(loader->uri);
    free(loader);
}

static void *_image_decode(void *userdata)
{
    ImageLoader *loader = (ImageLoader *)userdata;

    loader->bitmap = image_bitmap_create(loader->uri);
    return NULL;
}

static void *_image_decode_full(void *userdata)
{
    ImageLoader *loader = (ImageLoader *)userdata;

    int w, h;
    if (image_get_wh(loader->uri, &w, &h)) {
        _rect_ratio_full(w, h, loader->width, loader->height, &w, &h);
        loader->width = w;
        loader->height = h;
        loader->bitmap = image_bitmap_create(loader->uri);

    } else {
        ERR("image get width/height failed: %s", loader->uri);
    }
    return NULL;
}

static void *_image_decode_fit(void *userdata)
{
    ImageLoader *loader = (ImageLoader *)userdata;

    int w, h;
    if (image_get_wh(loader->uri, &w, &h)) {
        _rect_ratio_fit(w, h, loader->width, loader->height, &w, &h);
        loader->width = w;
        loader->height = h;
        loader->bitmap = image_bitmap_create(loader->uri);
    } else {
        ERR("image get width/height failed: %s", loader->uri);
    }
    return NULL;
}

ImageLoader *image_load(Image *img, struct nemotool *tool,
        const char *uri, int width, int height,
        ImageLoadDone done, void *userdata)
{
    ImageLoader *loader = (ImageLoader *)calloc(sizeof(ImageLoader), 1);
    img->loader = loader;
    loader->img = img;
    loader->uri = strdup(uri);
    loader->width = width;
    loader->height = height;
    loader->done = done;
    loader->userdata = userdata;
    loader->thread = thread_create(tool,
                _image_decode, _image_decode_done, loader);
    return loader;
}

ImageLoader *image_load_full(Image *img, struct nemotool *tool,
        const char *uri, int width, int height,
        ImageLoadDone done, void *userdata)
{
    ImageLoader *loader = (ImageLoader *)calloc(sizeof(ImageLoader), 1);
    img->loader = loader;
    loader->img = img;
    loader->uri = strdup(uri);
    loader->width = width;
    loader->height = height;
    loader->done = done;
    loader->userdata = userdata;
    loader->thread = thread_create(tool,
                _image_decode_full, _image_decode_done, loader);
    return loader;
}

ImageLoader *image_load_fit(Image *img, struct nemotool *tool,
        const char *uri, int width, int height,
        ImageLoadDone done, void *userdata)
{
    ImageLoader *loader = (ImageLoader *)calloc(sizeof(ImageLoader), 1);
    img->loader = loader;
    loader->img = img;
    loader->uri = strdup(uri);
    loader->width = width;
    loader->height = height;
    loader->done = done;
    loader->userdata = userdata;
    loader->thread = thread_create(tool,
                _image_decode_fit, _image_decode_done, loader);
    return loader;
}

void imageloader_cancel(ImageLoader *loader)
{
    thread_destroy(loader->thread);
}

Image *image_create(struct showone *parent)
{
    Image *img = (Image *)calloc(sizeof(Image), 1);

    struct showone *one;
    struct showone *group;
    img->group = group = GROUP_CREATE(parent);

    ITEM_CREATE(one, group, NEMOSHOW_IMAGE_ITEM);
    img->one = one;

    return img;
}

void image_load_cancel(Image *img)
{
    if (img->loader) {
        imageloader_cancel(img->loader);
        img->loader = NULL;
    }
}

void image_unload(Image *img)
{
    image_load_cancel(img);
    if (img->bitmap) {
        nemoshow_item_set_bitmap(img->one, NULL);
        image_bitmap_destroy(img->bitmap);
        img->bitmap = NULL;
    }
}

void image_destroy(Image *img)
{
    if (img->clip) nemoshow_one_destroy(img->clip);
    image_unload(img);
    nemoshow_item_destroy(img->one);
    nemoshow_item_destroy(img->group);
    free(img);
}

int image_get_width(Image *img)
{
    return nemoshow_item_get_width(img->one);
}

int image_get_height(Image *img)
{
    return nemoshow_item_get_height(img->one);
}

void image_load_sync(Image *img, const char *uri, int width, int height)
{
    ImageBitmap *bitmap = image_bitmap_create(uri);
    if (!bitmap) {
        ERR("image_bitmap_create failed");
        return;
    }
    image_set_bitmap(img, width, height, bitmap);
}

void image_translate(Image *img, uint32_t easetype, int duration, int delay, double tx, double ty)
{
    if (duration > 0) {
        _nemoshow_item_motion(img->group, easetype, duration, delay,
                "tx", tx, "ty", ty, NULL);
    } else {
        nemoshow_item_translate(img->group, tx, ty);
    }
}

void image_scale(Image *img, uint32_t easetype, int duration, int delay, double sx, double sy)
{
    if (duration > 0) {
        _nemoshow_item_motion(img->group, easetype, duration, delay,
                "sx", sx, "sy", sy, NULL);
    } else {
        nemoshow_item_scale(img->group, sx, sy);
    }
}

void image_rotate(Image *img, uint32_t easetype, int duration, int delay, double ro)
{
    if (duration > 0) {
        _nemoshow_item_motion(img->group, easetype, duration, delay,
                "ro", ro, NULL);
    } else {
        nemoshow_item_rotate(img->group, ro);
    }
}

void image_set_alpha(Image *img, uint32_t easetype, int duration, int delay, double alpha)
{
    if (duration > 0) {
        _nemoshow_item_motion(img->group, easetype, duration, delay,
                "alpha", alpha, NULL);
    } else {
        nemoshow_item_set_alpha(img->group, alpha);
    }
}

void image_set_anchor(Image *img, double ax, double ay)
{
    nemoshow_item_set_anchor(img->one, ax, ay);
}


void image_set_clip(Image *img, struct showone *clip)
{
    img->clip = clip;
    nemoshow_item_set_clip(img->one, clip);
}

struct showone *image_get_one(Image *img)
{
    return img->one;
}

struct showone *image_get_group(Image *img)
{
    return img->group;
}

void image_below(Image *img, struct showone *one)
{
    nemoshow_item_below_one(img->group, one);
}

void image_above(Image *img, struct showone *one)
{
    nemoshow_item_above_one(img->group, one);
}
