#include "compiler.h"
#include "app.h"

extern struct IntuitionBase *IntuitionBase;

int AppMain(void)
{
    struct Window *win;
    struct Screen *scr;
    struct IntuiMessage *msg;
    struct NewWindow nw;
    BOOL done = FALSE;
    UBYTE *greeting = (UBYTE *)"Hello World - Ciao Mondo!";
    WORD greetingLen = 25;
    WORD textX, textY;

    /* ActiveScreen is valid as soon as Intuition is up, back to KS 1.3 */
    scr = IntuitionBase->ActiveScreen;

    nw.LeftEdge    = 0;
    nw.TopEdge     = 0;
    nw.Width       = scr->Width;
    nw.Height      = scr->Height;
    nw.DetailPen   = 0;
    nw.BlockPen    = 1;

    nw.IDCMPFlags  = IDCMP_RAWKEY;

    /* Borderless: no title bar, no gadgets, sized to cover the whole screen.
       (WFLG_BACKDROP is deliberately avoided: on WBENCHSCREEN it would place
       the window behind Workbench's own backdrop icon window, hiding it.) */
    nw.Flags       = WFLG_BORDERLESS |
                      WFLG_ACTIVATE;

    nw.FirstGadget = NULL;
    nw.CheckMark   = NULL;

    nw.Title       = NULL;

    nw.Screen      = NULL;
    nw.BitMap      = NULL;

    nw.MinWidth    = nw.Width;
    nw.MinHeight   = nw.Height;
    nw.MaxWidth    = nw.Width;
    nw.MaxHeight   = nw.Height;

    nw.Type        = WBENCHSCREEN;

    win = OpenWindow(&nw);
    if (!win)
        return 20;

    textX = (win->Width - TextLength(win->RPort, (char *)greeting, greetingLen)) / 2;
    textY = (win->Height - win->RPort->Font->tf_YSize) / 2 + win->RPort->Font->tf_Baseline;

    SetAPen(win->RPort, 1);
    Move(win->RPort, textX, textY);
    Text(win->RPort, (char *)greeting, greetingLen);

    while (!done)
    {
        Wait(1L << win->UserPort->mp_SigBit);

        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            /* No close gadget on a borderless window: any key quits */
            if (msg->Class == IDCMP_RAWKEY)
                done = TRUE;

            ReplyMsg((struct Message *)msg);
        }
    }

    CloseWindow(win);

    return 0;
}
