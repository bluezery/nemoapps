#include "xemoutil.h"
#include "xemoapp.h"

// XXX: should call once!
bool xemoapp_init()
{
    return xemoutil_init();
}

// XXX: should call once!
void xemoapp_shutdown()
{
    xemoutil_shutdown();
}

