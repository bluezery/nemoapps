#include <SkData.h>
#include <SkRefCnt.h>
#include <SkTemplates.h>
#include <SkBitmap.h>
#include <SkCodec.h>

#include "nemoutil.h"
#include "skiahelper.h"
#include "skiahelper.hpp"

struct _SkiaBitmap {
    SkBitmap *skbitmap;
};

SkiaBitmap *skia_bitmap_create(const char *path)
{
    RET_IF(!path, NULL);
    SkiaBitmap *bitmap = (SkiaBitmap *)calloc(sizeof(SkiaBitmap), 1);

    bitmap->skbitmap = new SkBitmap;

    sk_sp<SkData> data(SkData::MakeFromFileName(path));
    SkCodec *codec(SkCodec::NewFromData(data));
    if (!codec) {
        ERR("No codec from data: %s", path);
        return NULL;
    }

    SkImageInfo info = codec->getInfo().makeColorSpace(NULL);

    bitmap->skbitmap->setInfo(info);
    bitmap->skbitmap->allocPixels();

    SkCodec::Result r = codec->getPixels(info, bitmap->skbitmap->getPixels(), bitmap->skbitmap->rowBytes());
    delete codec;
    if (r != SkCodec::kSuccess) {
        ERR("getPixels failed(%d): %s", r, path);
        return NULL;
    }

    return bitmap;
}

void skia_bitmap_destroy(SkiaBitmap *bitmap)
{
    delete bitmap->skbitmap;
    free(bitmap);
}

SkBitmap *skia_bitmap_get_bitmap(SkiaBitmap *bitmap)
{
    return bitmap->skbitmap;
}

SkiaBitmap *skia_bitmap_dup(SkiaBitmap *bitmap)
{
    SkiaBitmap *dst = (SkiaBitmap *)calloc(sizeof(SkiaBitmap), 1);
    bitmap->skbitmap->deepCopyTo(dst->skbitmap);
    return dst;
}

