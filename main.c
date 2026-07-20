#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

struct ExecBase *SysBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

int main(void)
{
    struct Window *win;
    struct IntuiMessage *msg;
    BOOL done = FALSE;

    struct NewWindow nw = {
        40, 30,                     /* Left, Top */
        320, 100,                   /* Width, Height */
        0, 1,                       /* DetailPen, BlockPen */

        IDCMP_CLOSEWINDOW,

        WFLG_CLOSEGADGET |
        WFLG_DRAGBAR |
        WFLG_DEPTHGADGET |
        WFLG_ACTIVATE,

        NULL,
        NULL,

        (UBYTE *)"Happy Amiga Clock",

        NULL,
        NULL,

        100, 50,
        640, 256,

        WBENCHSCREEN
    };

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

    win = OpenWindow(&nw);

    if (!win)
    {
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    SetAPen(win->RPort, 1);
    Move(win->RPort, 20, 30);
    Text(win->RPort, "Hello World!", 12);

    while (!done)
    {
        Wait(1L << win->UserPort->mp_SigBit);

        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            if (msg->Class == IDCMP_CLOSEWINDOW)
                done = TRUE;

            ReplyMsg((struct Message *)msg);
        }
    }

    CloseWindow(win);

    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);

    return 0;
}
