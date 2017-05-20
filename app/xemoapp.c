#include "xemoutil.h"
#include "xemoapp.h"

// XXX: should call once!
bool xemoapps_init()
{
    return xemoutil_init();
}

void xemoapps_shutdown()
{
    xemoutil_shutdown();
}

