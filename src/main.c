#include "amiga.h"
#include "app.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

/* dos.library is opened by the vbcc kick13 startup code before main() is
   even called (it provides and fills in DOSBase itself), since virtually
   every Amiga program needs it just to run from Shell/Workbench. Only the
   libraries the startup code does NOT open - Intuition and Graphics - are
   opened explicitly here. */

/* Library versions requested match the KS 1.3 ROM libraries exactly
   (Intuition 33, Graphics 33) - see include/amiga.h. */
#define LIB_VERSION 33L

int main(void)
{
    int rc;

    rc = 0;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", LIB_VERSION);
    if (IntuitionBase != NULL) {
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", LIB_VERSION);
        if (GfxBase != NULL) {
            rc = AppMain();
            CloseLibrary((struct Library *)GfxBase);
        }
        CloseLibrary((struct Library *)IntuitionBase);
    }
    return rc;
}
