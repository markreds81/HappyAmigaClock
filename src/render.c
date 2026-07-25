#include "render.h"

/* Local, dependency-free string length (avoids pulling in <string.h>) */
static WORD textStrLen(const char *s)
{
    WORD n = 0;

    while (*s++)
        n++;

    return n;
}

void RenderInit(struct RenderContext *rc, struct Window *win)
{
    rc->rc_Window      = win;
    rc->rc_PrevX       = 0;
    rc->rc_PrevY       = 0;
    rc->rc_PrevWidth   = 0;
    rc->rc_PrevHeight  = 0;
    rc->rc_HasPrev     = FALSE;
}

void RenderText(struct RenderContext *rc, const char *text)
{
    struct RastPort *rp   = rc->rc_Window->RPort;
    struct TextFont  *font = rp->Font;
    WORD len         = textStrLen(text);
    WORD textWidth   = TextLength(rp, (STRPTR)text, len);
    WORD textHeight  = font->tf_YSize;
    WORD x           = (rc->rc_Window->Width  - textWidth)  / 2;
    WORD y           = (rc->rc_Window->Height - textHeight) / 2;

    /* Erase only the rectangle used by the previous draw, if any */
    if (rc->rc_HasPrev)
    {
        SetAPen(rp, 0);
        RectFill(rp, rc->rc_PrevX, rc->rc_PrevY,
                 rc->rc_PrevX + rc->rc_PrevWidth  - 1,
                 rc->rc_PrevY + rc->rc_PrevHeight - 1);
    }

    SetAPen(rp, 1);
    Move(rp, x, y + font->tf_Baseline);
    Text(rp, (STRPTR)text, len);

    rc->rc_PrevX      = x;
    rc->rc_PrevY      = y;
    rc->rc_PrevWidth  = textWidth;
    rc->rc_PrevHeight = textHeight;
    rc->rc_HasPrev    = TRUE;
}
