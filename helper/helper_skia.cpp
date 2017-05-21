// XXX: define, Before include skia headers
#define SK_RELEASE

#include <SkData.h>
#include <SkRefCnt.h>
#include <SkTemplates.h>
#include <SkBitmap.h>
#include <SkCodec.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkTypeface.h>

#include "xemoutil.h"
#include "helper_skia.h"
#include "helper_skia.hpp"

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

double skia_calculate_text_width(const char *fontfamily, const char *fontstyle, double fontsize, const char *str)
{
    RET_IF(!fontfamily, !str);
	SkPaint paint;
	sk_sp<SkTypeface> face;
	SkPaint::FontMetrics metrics;

    XemoFont *font = xemofont_create(fontfamily, fontstyle,
            FC_SLANT_ROMAN, FC_WEIGHT_NORMAL, FC_WIDTH_NORMAL, 0);
    if (!font) {
        ERR("font create failed");
        return 0;
    }

	face = SkTypeface::MakeFromFile(xemofont_get_filepath(font), 0);
    xemofont_destroy(font);

	paint.setTypeface(face);
	paint.setAntiAlias(true);
	paint.setTextSize(fontsize);
	paint.getFontMetrics(&metrics, 0);

	SkPath path;
	paint.getTextPath(str, strlen(str), 0, -metrics.fAscent - metrics.fDescent, &path);

	SkRect box;
	box = path.getBounds();

    return box.width();
}
