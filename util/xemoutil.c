#include "xemoutil.h"

// XXX: should call once!
bool xemoutil_init()
{
    return xemofont_init();
}

void xemoutil_shutdown()
{
    xemofont_shutdown();
}

