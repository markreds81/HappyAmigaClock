#include "amiga.h"
#include "app.h"
#include "config.h"
#include "version.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

const char AppVersionTag[] = APP_VERSION_TAG;

#define LIB_VERSION 33L

int main(int argc, char **argv)
{
    int rc;
    struct AppConfig config;

    rc = 0;
    IntuitionBase =
        (struct IntuitionBase *)OpenLibrary("intuition.library", LIB_VERSION);

    if (IntuitionBase != NULL)
    {
        GfxBase =
            (struct GfxBase *)OpenLibrary("graphics.library", LIB_VERSION);
        if (GfxBase != NULL)
        {
            rc = ConfigLoad(&config, argc, argv) ? AppMain(&config) : 10;
            CloseLibrary((struct Library *)GfxBase);
        }
        CloseLibrary((struct Library *)IntuitionBase);
    }

    return rc;
}
