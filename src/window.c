#include "window.h"

extern struct IntuitionBase *IntuitionBase;

/* Amiga raw key code (hardware scan code) for Escape. Using IDCMP_RAWKEY
   with an explicit scan code keeps this KS 1.3-safe: the ASCII-translated
   IDCMP_VANILLAKEY class only became available on later Kickstarts. */
#define RAWKEY_ESC 0x45

#define BLANK_POINTER_WORDS 6
#define BLANK_POINTER_BYTES (BLANK_POINTER_WORDS * sizeof(UWORD))

static UWORD *blankPointer = NULL;

struct Window *OpenAppWindow(void)
{
    struct NewWindow nw;
    struct Screen *scr;
    struct Window *win;

    /* ActiveScreen is valid as soon as Intuition is up, back to KS 1.3 */
    scr = IntuitionBase->ActiveScreen;

    nw.LeftEdge   = 0;
    nw.TopEdge    = 0;
    nw.Width      = scr->Width;
    nw.Height     = scr->Height;
    nw.DetailPen  = 0;
    nw.BlockPen   = 1;

    nw.IDCMPFlags = IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS;

    /* Borderless: no title bar, no gadgets, sized to cover the whole screen.
       (WFLG_BACKDROP is deliberately avoided: on WBENCHSCREEN it would place
       the window behind Workbench's own backdrop icon window, hiding it.) */
    nw.Flags = WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_REPORTMOUSE;

    nw.FirstGadget = NULL;
    nw.CheckMark   = NULL;
    nw.Title       = NULL;

    nw.Screen  = NULL;
    nw.BitMap  = NULL;

    nw.MinWidth  = nw.Width;
    nw.MinHeight = nw.Height;
    nw.MaxWidth  = nw.Width;
    nw.MaxHeight = nw.Height;

    nw.Type = WBENCHSCREEN;

    win = OpenWindow(&nw);
    if (win)
        blankPointer = (UWORD *)AllocMem(BLANK_POINTER_BYTES,
                                         MEMF_CHIP | MEMF_CLEAR);

    return win;
}

void CloseAppWindow(struct Window *win)
{
    if (win)
    {
        ClearPointer(win);
        CloseWindow(win);
    }

    if (blankPointer)
    {
        FreeMem(blankPointer, BLANK_POINTER_BYTES);
        blankPointer = NULL;
    }
}

BOOL WindowHidePointer(struct Window *win)
{
    if (blankPointer)
    {
        SetPointer(win, blankPointer, 1, 1, 0, 0);
        return TRUE;
    }

    return FALSE;
}

void WindowShowPointer(struct Window *win)
{
    ClearPointer(win);
}

BOOL WindowProcessMessages(struct Window *win, BOOL *mouseMoved)
{
    struct IntuiMessage *msg;
    BOOL quit = FALSE;

    *mouseMoved = FALSE;

    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)) != NULL)
    {
        if (msg->Class == IDCMP_RAWKEY)
        {
            UWORD code = msg->Code;

            /* React on key-down only; ignore the key-up event (bit 0x80) */
            if (!(code & IECODE_UP_PREFIX) && code == RAWKEY_ESC)
                quit = TRUE;
        }
        else if (msg->Class == IDCMP_MOUSEMOVE)
        {
            *mouseMoved = TRUE;
        }
        else if (msg->Class == IDCMP_MOUSEBUTTONS &&
                 msg->Code == SELECTDOWN)
        {
            quit = TRUE;
        }

        ReplyMsg((struct Message *)msg);
    }

    return quit;
}
