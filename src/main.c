#include "compiler.h"
#include "app.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

int main(void)
{
    int rc;

    rc = 0;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 34L);
    if (IntuitionBase != NULL) {
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 34L);
        if (GfxBase != NULL) {
            rc = AppMain();
            CloseLibrary((struct Library *)GfxBase);
        }
        CloseLibrary((struct Library *)IntuitionBase);
    }    
    return rc;
}
