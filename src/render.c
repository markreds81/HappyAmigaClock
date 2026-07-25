#include "render.h"
#include "font.h"

/* Time fills most of the screen height; date sits smaller below it. */
#define TIME_HEIGHT_PIXELS 64
#define DATE_HEIGHT_PIXELS 24
#define LINE_GAP_PERCENT    4

/* No string.h in this codebase; NUL-terminated copy, like strcpy(). */
static void copyStr(char *dst, const char *src)
{
    while ((*dst++ = *src++) != '\0')
        ;
}

/*
 * Erases and redraws only the characters where 'newText' differs from
 * 'prevText' (both assumed same length - true here, since the clock's
 * time/date formats are fixed-width). This is what keeps a per-second
 * redraw cheap: normally just the last digit or two actually changed.
 */
static void redrawChangedChars(struct RastPort *rp, const char *prevText, const char *newText,
                                WORD x, WORD y, WORD height)
{
    WORD advance = FontCharAdvance(height);
    WORD cx = x;

    while (*newText)
    {
        if (*prevText != *newText)
        {
            /* +1 on the bottom row: glyph coordinates are closed on
               [0, CELL_H], so e.g. the 'D' segment's flat bottom edge
               sits exactly at row y+height, one past the naive y+height-1
               bound - leaving a stray horizontal sliver behind otherwise. */
            SetAPen(rp, 0);
            RectFill(rp, cx, y, cx + advance - 1, y + height);
            SetAPen(rp, 1);
            FontDrawChar(rp, *newText, cx, y, height);
        }

        cx += advance;
        newText++;
        prevText++;
    }
}

BOOL RenderInit(struct RenderContext *rc, struct Window *win)
{
    struct RastPort *rp = win->RPort;

    rc->rc_Window      = win;
    rc->rc_PrevX       = 0;
    rc->rc_PrevY       = 0;
    rc->rc_PrevWidth   = 0;
    rc->rc_PrevHeight  = 0;
    rc->rc_HasPrev     = FALSE;

    SetBPen(rp, 0);

    return FontInit();
}

void RenderExit(struct RenderContext *rc)
{
    FontExit();
}

void RenderClock(struct RenderContext *rc, const char *time, const char *date)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    WORD screenHeight = rc->rc_Window->Height;
    WORD timeHeight = TIME_HEIGHT_PIXELS;
    WORD dateHeight = DATE_HEIGHT_PIXELS;
    WORD lineGap    = (screenHeight * LINE_GAP_PERCENT)    / 100;
    WORD timeWidth  = FontStringWidth(time, timeHeight);
    WORD dateWidth  = FontStringWidth(date, dateHeight);
    WORD blockWidth  = (timeWidth > dateWidth) ? timeWidth : dateWidth;
    WORD blockHeight = timeHeight + lineGap + dateHeight;
    WORD blockX = (rc->rc_Window->Width  - blockWidth)  / 2;
    WORD blockY = (rc->rc_Window->Height - blockHeight) / 2;
    WORD timeX  = blockX + (blockWidth - timeWidth) / 2;
    WORD dateX  = blockX + (blockWidth - dateWidth) / 2;
    WORD dateY  = blockY + timeHeight + lineGap;
    BOOL sameLayout = rc->rc_HasPrev &&
                       rc->rc_PrevTimeX == timeX && rc->rc_PrevDateX == dateX &&
                       rc->rc_PrevLineY == blockY && rc->rc_PrevDateY == dateY &&
                       rc->rc_PrevTimeHeight == timeHeight && rc->rc_PrevDateHeight == dateHeight;

    /* Sync to the vertical blank so the (now small) update lands entirely
       within the blanking interval instead of tearing across a frame
       that's actively being scanned out - this is what removed the
       flicker on the real/emulated single-buffered screen. */
    WaitTOF();

    if (sameLayout)
    {
        redrawChangedChars(rp, rc->rc_PrevTime, time, timeX, blockY, timeHeight);
        redrawChangedChars(rp, rc->rc_PrevDate, date, dateX, dateY,  dateHeight);
    }
    else
    {
        /* Layout changed (or first draw ever): fall back to a full
           erase-and-redraw of the whole block. */
        if (rc->rc_HasPrev)
        {
            /* +1 on both edges: same closed-interval glyph coordinates
               as in redrawChangedChars() above - the last character's
               rightmost segment column and the bottom row both land
               exactly on the naive -1 bound otherwise. */
            SetAPen(rp, 0);
            RectFill(rp, rc->rc_PrevX, rc->rc_PrevY,
                     rc->rc_PrevX + rc->rc_PrevWidth,
                     rc->rc_PrevY + rc->rc_PrevHeight);
        }

        SetAPen(rp, 1);
        FontDrawString(rp, time, timeX, blockY, timeHeight);
        FontDrawString(rp, date, dateX, dateY,  dateHeight);
    }

    copyStr(rc->rc_PrevTime, time);
    copyStr(rc->rc_PrevDate, date);

    rc->rc_PrevX         = blockX;
    rc->rc_PrevY         = blockY;
    rc->rc_PrevWidth     = blockWidth;
    rc->rc_PrevHeight    = blockHeight;
    rc->rc_PrevTimeX     = timeX;
    rc->rc_PrevDateX     = dateX;
    rc->rc_PrevLineY     = blockY;
    rc->rc_PrevDateY     = dateY;
    rc->rc_PrevTimeHeight = timeHeight;
    rc->rc_PrevDateHeight = dateHeight;
    rc->rc_HasPrev       = TRUE;
}
