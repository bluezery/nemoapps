#include <pthread.h>
#include <wand/MagickWand.h>

#include "pixmanhelper.h"
#include "nemoutil.h"
#include "nemoutil-image.h"

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

