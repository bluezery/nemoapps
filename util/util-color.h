#ifndef __COLOR_H__
#define __COLOR_H__

#define ARGB2RGBA(color) \
    (color & 0xFFFFFF) << 8 | (color & 0xFF000000) >> 24

#define ARGB(color) \
    (color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, (color & 0xFF), (color & 0xFF000000) >> 24

#define RGBA(color) \
    (color & 0xFF000000) >> 24, (color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, (color & 0xFF)

#define RGB(color) \
    RGBA((color << 8 | 0xFF))

#define RGBA_D(color) \
    ((color & 0xFF000000) >> 24)/255.0, ((color & 0xFF0000) >> 16)/255.0, ((color & 0xFF00) >> 8)/255.0, ((color & 0xFF)/255.0)

#define RGBA2UINT(r, g, b, a) \
    (((r) << 24) + ((g) << 16) + ((b) << 8) + (a))

#define BLACK   0x000000FF
#define WHITE   0xECF0F1FF

#define MIDNIGHTBLUE 0x2C3E50FF


// reference: flatcolorsui.com

#define LIGHTRED     0xFFCCBCFF
#define LIGHTPINK    0xFFBCD8FF
#define LIGHTPURPLE  0xDCC6E0FF
#define LIGHTBLUE    0x39D5FFFF
#define LIGHTCYAN    0x5EFAF7FF
#define LIGHTGREEN   0x8EFFC1FF
#define LIGHTYELLOW  0xFDE3A7FF
#define LIGHTORANGE  0xFFDCB5FF
#define LIGHTBROWN   0xF6C4A3FF
#define LIGHTAQUA     0xC5D3E2FF
#define LIGHTSILVER  0xD5E5E6FF
#define LIGHTGRAY    0xE0E0E0FF

#define COLOR_4
#ifdef COLOR_4
    #define RED     0xFF7C6CFF
    #define PINK    0xFF6CA8FF
    #define PURPLE  0xAB69C6FF
    #define BLUE    0x22A7F0FF
    #define CYAN    0x37DBD0FF
    #define GREEN   0x3EDC81FF
    #define YELLOW  0xF9B32FFF
    #define ORANGE  0xFFA27BFF
    #define BROWN   0xD4A281FF
    #define AQUA     0x9CAAB9FF
    #define SILVER  0xA5B5B6FF
    #define GRAY    0xA0A0A0FF
    #define MAGENTA 0xFF0097FF
#else
    #define RED     0xff0000FF
    #define BLUE    0x1edcdcFF
    #define GREEN   0xff00ffFF
    #define YELLOW 0xebf564FF
    #define ORANGE  0xff8c32FF
#endif

#endif
