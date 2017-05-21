#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <hb-ft.h>
#include <hb-ot.h>
#include <fontconfig/fontconfig.h>
#include "xemoutil.h"
#include "xemoutil-font.h"

FT_Library _ft_lib;
FcConfig *_font_config;

typedef struct _File_Map File_Map;
struct _File_Map {
    char *data;
    size_t len;
};

struct _XemoFont {
    // font config proerties
    char *filepath;
    int idx;
    char *font_family;
    char *font_style;
    unsigned int font_slant;
    unsigned int font_weight;
    unsigned int font_spacing;
    unsigned int font_width;

    // free type
    FT_Face ft_face;

    // Harfbuzz
    hb_font_t *hb_font;
    const char **shapers;

#ifdef CAIRO_FONT
    cairo_scaled_font_t *cairo_font;
#endif
};

typedef struct _Glyph Glyph;
struct _Glyph {
    FT_Vector *points;
    unsigned long unicode;
    int num_points;
    short *contours;
    int num_contours;
    int height;
    double r, g, b, a;
    int line_width;
    char *tags;

    unsigned long code;
};


static void
_file_map_destroy(File_Map *fmap)
{
    RET_IF(!fmap);
    if (munmap(fmap->data, fmap->len) < 0)
        ERR("munmap failed");
}

static File_Map *
_file_map_create(const char *file)
{
    RET_IF(!file, NULL);

    char *data;
    struct stat st;
    int fd;
    size_t len;

    if ((fd = open(file, O_RDONLY)) <= 0) {
        ERR("open failed");
        return NULL;
    }

    if (fstat(fd, &st) == -1) {
        ERR("fstat failed");
        close(fd);
        return NULL;
    }

    if (st.st_size == 0 && S_ISREG(st.st_mode)) {
        ERR("faile size is 0");
        close(fd);
        return NULL;
    }
    len = st.st_size;
    data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        ERR("mmap failed: %s, len:%lu, fd:%d", strerror(errno), len, fd);
        close(fd);
        return NULL;
    }
    close(fd);

    File_Map *fmap = (File_Map *)malloc(sizeof(File_Map));
    fmap->data = data;
    fmap->len = len;
    return fmap;
}

bool xemofont_init()
{
    if (_ft_lib) return true;
    if (FT_Init_FreeType(&_ft_lib)) return false;
    _font_config = FcInitLoadConfigAndFonts();
    if (!_font_config) {
        ERR("FcInitLoadConfigAndFonts failed");
        FT_Done_FreeType(_ft_lib);
        return false;
    }
    if (!FcConfigSetRescanInterval(_font_config, 0)) {
        ERR("FcConfigSetRescanInterval failed");
    }

    return true;
}

void xemofont_shutdown()
{
    // FIXME: crash!!!!
    _font_config = NULL;
    if (_ft_lib) FT_Done_FreeType(_ft_lib);
    _ft_lib = NULL;
    if (_font_config) FcFini();
}

void xemofont_destroy(XemoFont *font)
{
    RET_IF(!font);
#ifdef CAIRO_FONT
    if (font->cairo_font) cairo_scaled_font_destroy(font->cairo_font);
#endif
    if (font->hb_font) hb_font_destroy(font->hb_font);
    if (font->ft_face) FT_Done_Face(font->ft_face);
    if (font->filepath) free(font->filepath);
    free(font);
}

static FT_Face
_font_ft_create(const char *file, unsigned int idx)
{
    FT_Face ft_face;

    if (FT_New_Face(_ft_lib, file, idx, &ft_face)) return NULL;

    return ft_face;
}

// if backend is 1, it's freetype, else opentype
static hb_font_t *
_font_hb_create(const char *file, unsigned int idx)
{
    File_Map *map = NULL;
    hb_blob_t *blob;
    hb_face_t *face;
    hb_font_t *font;
    unsigned int upem;

    RET_IF(!file, NULL);

    map = _file_map_create(file);
    if (!map->data || !map->len) return NULL;

    blob = hb_blob_create(map->data, map->len,
            HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE, map,
            (hb_destroy_func_t)_file_map_destroy);
    if (!blob) {
        _file_map_destroy(map);
        return NULL;
    }

    face = hb_face_create (blob, idx);
    hb_blob_destroy(blob);
    if (!face) return NULL;
    upem = hb_face_get_upem(face);

    font = hb_font_create(face);
    hb_face_destroy(face);
    if (!font) return NULL;

    // harfbuzz should scaled up at least upem
    hb_font_set_scale(font, upem, upem);

    hb_ft_font_set_funcs(font);
    // FIXME: glyph poses (offset, advance) is not correct for old version (below than 0.3.38)!!!
    //hb_ot_font_set_funcs(font);
    return font;
}

#ifdef CAIRO_FONT
static cairo_scaled_font_t *
_font_cairo_create(FT_Face ft_face, double size)
{
    RET_IF(!ft_face, NULL);
    RET_IF(size <= 0, NULL);

    cairo_font_face_t *cairo_face;
    cairo_matrix_t ctm, font_matrix;
    cairo_font_options_t *font_options;
    cairo_scaled_font_t *scaled_font;

    cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);

    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&font_matrix, size, size);

    font_options = cairo_font_options_create();
    cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_OFF);
    cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_DEFAULT);
    // CAIRO_ANTIALIAS_DEFAULT, CAIRO_ANTIALIAS_NONE, CAIRO_ANTIALIAS_GRAY,CAIRO_ANTIALIAS_SUBPIXEL,         CAIRO_ANTIALIAS_FAST, CAIRO_ANTIALIAS_GOOD, CAIRO_ANTIALIAS_BEST

    if (cairo_font_options_get_antialias(font_options) == CAIRO_ANTIALIAS_SUBPIXEL) {
        cairo_font_options_set_subpixel_order(font_options, CAIRO_SUBPIXEL_ORDER_DEFAULT);
        /* cairo internal functions
        _cairo_font_options_set_lcd_filter(font_options, CAIRO_LCD_FILTER_DEFAULT);
        _cairo_font_options_set_round_glyph_positions(font_options, CAIRO_ROUND_GLYPH_POS_DEFAULT);
        */
    }

    scaled_font = cairo_scaled_font_create (cairo_face, &font_matrix, &ctm, font_options);
    if (CAIRO_STATUS_SUCCESS != cairo_scaled_font_status(scaled_font)) {
        ERR("cairo scaled font create");
        cairo_scaled_font_destroy(scaled_font);
        return NULL;
    }
    cairo_font_options_destroy(font_options);
    cairo_font_face_destroy(cairo_face);

    return scaled_font;
}
#endif

static XemoFont *
_font_create(const char *filepath, unsigned int idx, const char *font_family, const char *font_style, unsigned int font_slant, unsigned int font_weight, unsigned int font_spacing, unsigned int font_width)

 {
    RET_IF(!filepath || !font_family || !font_style, NULL);
    RET_IF(!file_is_exist(filepath), NULL);

    XemoFont *font;
    FT_Face ft_face;
    hb_font_t *hb_font;
#ifdef CAIRO_FONT
    cairo_scaled_font_t *cairo_font;
#endif

    ft_face = _font_ft_create(filepath, 0);
    if (!ft_face) return NULL;

    hb_font = _font_hb_create(filepath, idx);
    if (!hb_font) return NULL;

#ifdef CAIRO_FONT
    // Usally, upem is 1000 for OpenType Shape font, 2048 for TrueType Shape font.
    // upem(Unit per em) is used for master (or Em) space.
    // cairo font size is noralized as 1
    cairo_font = _font_cairo_create(ft_face,
            ft_face->units_per_EM / (double)ft_face->max_advance_height);
#endif

    font = (XemoFont *)calloc(sizeof(XemoFont), 1);
    font->filepath = strdup(filepath);
    font->idx = idx;
    font->font_family = strdup(font_family);
    font->font_style = strdup(font_style);
    font->font_slant = font_slant;
    font->font_weight = font_weight;
    font->font_spacing = font_spacing;
    font->font_width = font_width;
    font->ft_face = ft_face;
    font->hb_font = hb_font;
#ifdef CAIRO_FONT
    font->cairo_font = cairo_font;
#endif
    font->shapers = NULL;  //e.g. {"ot", "fallback", "graphite2", "coretext_aat"}

    return font;
}

List *
xemofont_list_get(int *num)
{
    FcPattern *pat;
    FcObjectSet *os;
    FcFontSet *fs;
    pat = FcPatternCreate();
    os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_SLANT,
            FC_WEIGHT, FC_WIDTH, FC_SPACING, NULL);
    fs = FcFontList(0, pat, os);
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);

    List *fonts = NULL;
    if (fs) {
        int i = 0;
        for (i = 0 ; i < fs->nfont ; i++) {
            XemoFont *font;
            FcChar8 *_family = NULL, *_style = NULL;
            int _slant, _weight, _spacing, _width;
            FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &_family);
            FcPatternGetString(fs->fonts[i], FC_STYLE, 0, &_style);
            FcPatternGetInteger(fs->fonts[i], FC_SLANT, 0, &_slant);
            FcPatternGetInteger(fs->fonts[i], FC_WEIGHT, 0, &_weight);
            FcPatternGetInteger(fs->fonts[i], FC_WIDTH, 0, &_width);
            FcPatternGetInteger(fs->fonts[i], FC_SPACING, 0, &_spacing);
            font = xemofont_create((char *)_family, (char *)_style, _slant, _weight,
                    _width, _spacing);
            fonts = list_append(fonts, font);
        }
        FcFontSetDestroy(fs);
    }
    if (num) *num = fs->nfont;
    return fonts;
}

// font_family: e.g. NULL, "LiberationMono", "Times New Roman", "Arial", etc.
// font_style: e.g. NULL, "Regular"(Normal), "Bold", "Italic", "Bold Italic", etc.
// font_slant: e.g. FC_SLANT_ROMAN, FC_SLANT_ITALIC, etc.
// font_weight: e.g. FC_WEIGHT_LIGHT, FC_WEIGHT_REGULAR, FC_WEIGHT_BOLD, etc.
// font_width e.g. FC_WIDTH_NORMAL, FC_WIDTH_CONDENSED, FC_WIDTH_EXPANDED, etc.
// font_spacing e.g. FC_PROPORTIONAL, FC_MONO, etc.
XemoFont *
xemofont_create(const char *font_family, const char *font_style, int font_slant, int font_weight, int font_width, int font_spacing)
{
    XemoFont *font = NULL;
    FcBool ret;
    FcPattern *pattern;
    FcFontSet *set;

    if (!_font_config) return NULL;

#if 0
    // CACHE POP: Find from already inserted font list
    nemolist_for_each(temp, &_font_list, link) {
        if ((font_family && !strcmp(temp->font_family, font_family)) &&
            (font_style && !strcmp(temp->font_style, font_style)) &&
            (temp->font_slant == font_slant) &&
            (temp->font_weight == font_weight) &&
            (temp->font_width == font_width) &&
            (temp->font_spacing == font_spacing)) {
            return temp;
        }
    }
#endif
    // Create pattern
    pattern = FcPatternCreate();
    if (!pattern) {
        ERR("FcPatternBuild");
        return NULL;
    }
    if (font_family) FcPatternAddString(pattern, FC_FAMILY, (unsigned char *)font_family);
    if (font_style) FcPatternAddString(pattern, FC_STYLE, (unsigned char *)font_style);
    if (font_slant >= 0)  FcPatternAddInteger(pattern, FC_SLANT, font_slant);
    if (font_weight >= 0)  FcPatternAddInteger(pattern, FC_WEIGHT, font_weight);
    if (font_width >= 0)  FcPatternAddInteger(pattern, FC_WIDTH, font_width);
    if (font_spacing >= 0)  FcPatternAddInteger(pattern, FC_SPACING, font_spacing);

    FcConfigSubstitute(_font_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    set = FcFontSetCreate();
    FcPattern *match;
    // Find just exactly matched one
    match = FcFontMatch(_font_config, pattern, &result);
    if (match) FcFontSetAdd(set, match);

    if (!match || !set->nfont) {  // Get fallback
        LOG("Search fallback fonts");
        FcFontSet *tset;
        tset = FcFontSort(_font_config, pattern, FcTrue, NULL, &result);
        if (!tset || !tset->nfont) {
            ERR("There is no installed fonts");
            FcFontSetDestroy(set);
            return NULL;
        }
        int i = 0;
        for (i = 0 ; i < tset->nfont ; i++) {
            FcPattern *temp = FcFontRenderPrepare(NULL, pattern, tset->fonts[i]);
            if (temp) FcFontSetAdd(set, temp);
        }
        FcFontSetDestroy(tset);
    }
    FcPatternDestroy(pattern);

    if (!set->nfont || !set->fonts[0]) {
        ERR("There is no installed fonts");
        FcFontSetDestroy(set);
        return NULL;
    }

    FcChar8 *filepath = NULL;
    ret = FcPatternGetString(set->fonts[0], FC_FILE, 0, &filepath);
    if (ret != FcResultMatch || !filepath) {
        ERR("file path is not correct");
        FcFontSetDestroy(set);
        return NULL;
    }

#if 0
    // CACHE POP: Search already inserted list
    nemolist_for_each(temp, &_font_list, link) {
        if (!strcmp(temp->filepath, (char *)filepath)) {
            FcFontSetDestroy(set);
            return temp;
        }
    }
#endif

    FcChar8 *_family = NULL, *_style = NULL;
    int _slant, _weight, _spacing, _width;
    FcPatternGetString(set->fonts[0], FC_FAMILY, 0, &_family);
    FcPatternGetString(set->fonts[0], FC_STYLE, 0, &_style);
    FcPatternGetInteger(set->fonts[0], FC_SLANT, 0, &_slant);
    FcPatternGetInteger(set->fonts[0], FC_WEIGHT, 0, &_weight);
    FcPatternGetInteger(set->fonts[0], FC_SPACING, 0, &_spacing);
    FcPatternGetInteger(set->fonts[0], FC_WIDTH, 0, &_width);

    font = _font_create((char *)filepath, 0, (char *)_family, (char *)_style,
            _slant, _weight, _spacing, _width);
#if 0
    // CACHE PUSH
    if (font) nemolist_insert_tail(&_font_list, &font->link);
#endif
    FcFontSetDestroy(set);

    return font;
}

const char *
xemofont_get_family(XemoFont *font)
{
    RET_IF(!font, NULL);
    return font->font_family;
}

const char *
xemofont_get_style(XemoFont *font)
{
    RET_IF(!font, NULL);
    return font->font_style;
}

const char *
xemofont_get_filepath(XemoFont *font)
{
    RET_IF(!font, NULL);
    return font->filepath;
}
