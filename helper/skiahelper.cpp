#include <SkData.h>
#include <SkRefCnt.h>
#include <SkTemplates.h>
#include <SkBitmap.h>
#include <SkCodec.h>
#include <SkPaint.h>
#include <SkTypeface.h>

#include "xemoutil.h"
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

double skia_get_font_metrics(const char *fontfamily, double fontsize)
{
	SkPaint paint;
	sk_sp<SkTypeface> face;
	SkPaint::FontMetrics metrics;

    XemoFont *font = xemofont_create(fontfamily, NULL, FC_SLANT_ROMAN, FC_WEIGHT_NORMAL, FC_WIDTH_NORMAL, 0);
    if (!font) {
        ERR("font create failed");
        return 0;
    }

	face = SkTypeface::MakeFromFile(xemofont_get_filepath(font), 0);

	paint.setTypeface(face);
	paint.setAntiAlias(true);
	paint.setTextSize(fontsize);
	paint.getFontMetrics(&metrics, 0);
    ERR("[%lf] %lf %lf %lf %lf", fontsize, metrics.fAvgCharWidth,
            metrics.fMaxCharWidth, metrics.fXMin, metrics.fXMax);
    return 0;
#if 0
	SkPath path;
	paint.getTextPath(text, textlength, x, y - metrics.fAscent - metrics.fDescent, &path);

	SkRect box;
	box = path.getBounds();

	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postTranslate(-box.x(), -box.y());

	path.transform(matrix);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->addPath(path);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->addPath(path);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
#endif
}
