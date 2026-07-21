#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>

#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>

#include "app.h"

struct ExecBase *SysBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

int main(void)
{
    int rc;

    SysBase = *(struct ExecBase **)4UL;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
    if (!IntuitionBase)
        return 20;

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    if (!GfxBase)
    {
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    rc = AppMain();

    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);

    return rc;
}
