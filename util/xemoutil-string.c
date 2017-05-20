#include "xemoutil-internal.h"
#include "xemoutil.h"
#include "xemoutil-string.h"

size_t string_count_utf(const char *str)
{
    RET_IF(!str, 0);

    size_t cnt = 0;
    size_t len = 0;
    while (str[len]) {
        if (cnt >= 10) break;
        if ((str[len] & 0xF0) == 0xF0)  { // 4 bytes
            len+=4;
        } else if ((str[len] & 0xE0) == 0xE0) { // 3 bytes
            len+=3;
        } else if ((str[len] & 0xC0) == 0xC0) { // 2 bytes
            len+=2;
        } else { // a byte
            len+=1;
        }
        cnt++;
    }
    return cnt;
}
