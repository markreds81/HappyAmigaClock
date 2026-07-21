#include <exec/types.h>

#include <intuition/intuition.h>
#include <graphics/rastport.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "app.h"

int AppMain(void)
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

    win = OpenWindow(&nw);
    if (!win)
        return 20;

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

    return 0;
}
