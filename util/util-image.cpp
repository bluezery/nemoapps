#include <pthread.h>
#include <wand/MagickWand.h>
#include <showitem.hpp>

#include "pixmanhelper.h"
#include "util-log.h"
#include "util-file.h"
#include "util-image.h"
#include "util-common.h"

void RGBA2ARGB(unsigned char *blob, size_t len)
{
    unsigned int i = 0;
    for (i = 0 ; i < len ; i+=4) {
        unsigned char temp = blob[i];
        blob[i] = blob[i+2];
        blob[i+2] = temp;
    }
}

void pixman_composite(pixman_image_t *target, pixman_image_t *src)
{
    int sw, sh;
    sw = pixman_image_get_width(src);
    sh = pixman_image_get_height(src);

    int tw, th;
    tw = pixman_image_get_width(target);
    th = pixman_image_get_height(target);

    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);
    pixman_transform_scale(&transform, NULL,
            pixman_double_to_fixed((double)sw/tw),
            pixman_double_to_fixed((double)sh/th));
    pixman_image_set_transform(src, &transform);

    /*
    pixman_image_composite32(PIXMAN_OP_CLEAR,
            src,
            NULL,
            nemowidget_get_pixman(pixman),
            0, 0, 0, 0, 0, 0, width, height);
            */
    pixman_image_composite32(PIXMAN_OP_SRC,
            src,
            NULL,
            target,
            0, 0, 0, 0, 0, 0, tw, th);
}

void pixman_composite_fit(pixman_image_t *target, pixman_image_t *src)
{
    int sw, sh;
    sw = pixman_image_get_width(src);
    sh = pixman_image_get_height(src);

    int tw, th;
    tw = pixman_image_get_width(target);
    th = pixman_image_get_height(target);

    int w, h;
    _rect_ratio_fit(sw, sh, tw, th, &w, &h);

    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);
    pixman_transform_scale(&transform, NULL,
            pixman_double_to_fixed((double)sw/w),
            pixman_double_to_fixed((double)sh/h));
    pixman_image_set_transform(src, &transform);

    /*
    pixman_image_composite32(PIXMAN_OP_CLEAR,
            src,
            NULL,
            target,
            0, 0, 0, 0, 0, 0, sw, sh);
            */
    pixman_image_composite32(PIXMAN_OP_SRC,
            src,
            NULL,
            target,
            0, 0, 0, 0, (tw - w)/2, (th - h)/2, tw, th);
}

/**********************************************************/
/* Image */
/**********************************************************/
struct _ImageBitmap {
    SkBitmap *bitmap;
};

static int __wand_init = 0;
pthread_mutex_t __wand_mutex;

bool image_get_wh(const char *path, int *w, int *h)
{
    MagickWand *wand;

    if (!file_is_exist(path)) {
        ERR("file does not exist: %s", path);
        return false;
    }
    if (file_is_null(path)) {
        ERR("file is NULL: %s", path);
        return false;
    }

    pthread_mutex_lock(&__wand_mutex);
    if (__wand_init <= 0) {
        MagickWandGenesis();
        __wand_init++;
    }
    pthread_mutex_unlock(&__wand_mutex);

    wand = NewMagickWand();
    if (!wand) {
        ERR("NewMagickWand() failed");
        return false;
    }

    MagickBooleanType status;
    status = MagickPingImage(wand, path);
    if (status == MagickFalse) {
        char *desc;
        ExceptionType type;
        desc = MagickGetException(wand, &type);
        ERR("%s %s %lu %s", GetMagickModule(), desc);
        desc = (char *)MagickRelinquishMemory(desc);
        return false;
    }

    if (w) *w = MagickGetImageWidth(wand);
    if (h) *h = MagickGetImageHeight(wand);
    status = MagickRemoveImage(wand);
    if (status == MagickFalse) {
        char *desc;
        ExceptionType type;
        desc = MagickGetException(wand, &type);
        ERR("%s %s %lu %s", GetMagickModule(), desc);
        desc = (char *)MagickRelinquishMemory(desc);
    }

    return true;
}

static SkBitmap *_image_decode_skbitmap(const char *path)
{
    SkBitmap *bitmap = new SkBitmap;

    SkAutoTUnref<SkData> data(SkData::NewFromFileName(path));
    SkAutoTDelete<SkCodec> codec(SkCodec::NewFromData(data, NULL));
    if (!codec) {
        ERR("No codec from data: %s", path);
        return NULL;
    }

    bitmap->setInfo(codec->getInfo());
    bitmap->allocPixels();

    SkCodec::Result r = codec->getPixels(codec->getInfo(), bitmap->getPixels(), bitmap->rowBytes());
    if (r != SkCodec::kSuccess) {
        ERR("getPixels failed(%d): %s", r, path);
        return NULL;
    }

    return bitmap;
}

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
    bitmap->bitmap = _image_decode_skbitmap(path);
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

void image_bitmap_attach(ImageBitmap *bitmap, struct showone *one)
{
    nemoshow_item_set_bitmap(one, bitmap->bitmap);
}

void image_bitmap_detach(struct showone *one)
{
    nemoshow_item_set_bitmap(one, NULL);
}

ImageBitmap *image_bitmap_dup(ImageBitmap *src)
{
    ImageBitmap *dst = (ImageBitmap *)calloc(sizeof(ImageBitmap), 1);
    src->bitmap->deepCopyTo(dst->bitmap);

    return dst;
}

void image_bitmap_destroy(ImageBitmap *bitmap)
{
    delete bitmap->bitmap;
    free(bitmap);
}
