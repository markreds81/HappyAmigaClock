#include "render.h"
#include "font.h"
#include "version.h"

/* Time stays centered; date is placed near the top edge. */
#define DATE_TOP_PERCENT    6
#define INFO_BOTTOM_MARGIN  6

static const char infoText[] = APP_NAME " " APP_VERSION_STRING;

/* No string.h in this codebase; NUL-terminated copy, like strcpy(). */
static void copyStr(char *dst, const char *src)
{
    while ((*dst++ = *src++) != '\0')
        ;
}

static WORD textLength(const char *text)
{
    WORD length = 0;

    while (*text++)
        length++;
    return length;
}

static void setRGB4Value(struct ViewPort *vp, WORD pen, UWORD rgb)
{
    SetRGB4(vp, pen, (rgb >> 8) & 0x0f, (rgb >> 4) & 0x0f, rgb & 0x0f);
}

/*
 * Swapping only pens 0 and 1 reverses every clock pixel instantly, without
 * touching the bitmap. The rest of the Workbench palette is left intact.
 */
static void setClockPalette(struct RenderContext *rc, BOOL darkBackground)
{
    if (!rc->rc_HasSavedPalette ||
        (rc->rc_HasClockPalette &&
         rc->rc_DarkBackground == darkBackground))
        return;

    if (darkBackground)
    {
        SetRGB4(rc->rc_ViewPort, 0, 0, 0, 0);
        SetRGB4(rc->rc_ViewPort, 1, 15, 15, 15);
    }
    else
    {
        SetRGB4(rc->rc_ViewPort, 0, 15, 15, 15);
        SetRGB4(rc->rc_ViewPort, 1, 0, 0, 0);
    }

    rc->rc_DarkBackground = darkBackground;
    rc->rc_HasClockPalette = TRUE;
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
    WORD cx = x;

    while (*newText)
    {
        WORD advance = FontCharAdvance(*newText, height);

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

BOOL RenderInit(struct RenderContext *rc, struct Window *win,
                BOOL showSeconds)
{
    struct RastPort *rp = win->RPort;

    rc->rc_Window      = win;
    rc->rc_PrevX       = 0;
    rc->rc_PrevY       = 0;
    rc->rc_PrevWidth   = 0;
    rc->rc_PrevHeight  = 0;
    rc->rc_HasPrev     = FALSE;
    rc->rc_ViewPort    = &win->WScreen->ViewPort;
    rc->rc_HasSavedPalette = FALSE;
    rc->rc_HasClockPalette = FALSE;

    if (rc->rc_ViewPort->ColorMap &&
        rc->rc_ViewPort->ColorMap->Count >= 2 &&
        rc->rc_ViewPort->ColorMap->ColorTable)
    {
        UWORD *colors =
            (UWORD *)rc->rc_ViewPort->ColorMap->ColorTable;

        rc->rc_SavedColor0 = colors[0];
        rc->rc_SavedColor1 = colors[1];
        rc->rc_HasSavedPalette = TRUE;
    }

    SetBPen(rp, 0);
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, win->Width - 1, win->Height - 1);

    return FontInit(showSeconds);
}

void RenderExit(struct RenderContext *rc)
{
    if (rc->rc_HasSavedPalette)
    {
        setRGB4Value(rc->rc_ViewPort, 0, rc->rc_SavedColor0);
        setRGB4Value(rc->rc_ViewPort, 1, rc->rc_SavedColor1);
    }

    FontExit();
}

void RenderClock(struct RenderContext *rc, const char *time, const char *date,
                 BOOL showSeconds, BOOL darkBackground)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    WORD screenHeight = rc->rc_Window->Height;
    WORD timeHeight = showSeconds ? FONT_LARGE_HEIGHT
                                  : FONT_COMPACT_HEIGHT;
    WORD dateHeight = FONT_SMALL_HEIGHT;
    WORD timeWidth  = FontStringWidth(time, timeHeight);
    WORD dateWidth  = FontStringWidth(date, dateHeight);
    WORD timeX  = (rc->rc_Window->Width - timeWidth) / 2;
    WORD timeY  = (screenHeight - timeHeight) / 2;
    WORD dateX  = (rc->rc_Window->Width - dateWidth) / 2;
    WORD dateY  = (screenHeight * DATE_TOP_PERCENT) / 100;
    WORD blockX = (timeX < dateX) ? timeX : dateX;
    WORD blockY = (timeY < dateY) ? timeY : dateY;
    WORD blockRight = ((timeX + timeWidth) > (dateX + dateWidth))
                    ? timeX + timeWidth : dateX + dateWidth;
    WORD blockBottom = ((timeY + timeHeight) > (dateY + dateHeight))
                     ? timeY + timeHeight : dateY + dateHeight;
    WORD blockWidth = blockRight - blockX;
    WORD blockHeight = blockBottom - blockY;
    BOOL sameLayout = rc->rc_HasPrev &&
                       rc->rc_PrevTimeX == timeX && rc->rc_PrevDateX == dateX &&
                       rc->rc_PrevLineY == timeY && rc->rc_PrevDateY == dateY &&
                       rc->rc_PrevTimeHeight == timeHeight && rc->rc_PrevDateHeight == dateHeight;

    setClockPalette(rc, darkBackground);

    /* Sync to the vertical blank so the (now small) update lands entirely
       within the blanking interval instead of tearing across a frame
       that's actively being scanned out - this is what removed the
       flicker on the real/emulated single-buffered screen. */
    WaitTOF();

    if (sameLayout)
    {
        redrawChangedChars(rp, rc->rc_PrevTime, time, timeX, timeY, timeHeight);
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
        FontDrawString(rp, time, timeX, timeY, timeHeight);
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
    rc->rc_PrevLineY     = timeY;
    rc->rc_PrevDateY     = dateY;
    rc->rc_PrevTimeHeight = timeHeight;
    rc->rc_PrevDateHeight = dateHeight;
    rc->rc_HasPrev       = TRUE;
}

void RenderSetInfoVisible(struct RenderContext *rc, BOOL visible)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    struct TextFont *font = rp->Font;
    WORD length = textLength(infoText);
    WORD width = TextLength(rp, (STRPTR)infoText, length);
    WORD x = (rc->rc_Window->Width - width) / 2;
    WORD y = rc->rc_Window->Height - font->tf_YSize - INFO_BOTTOM_MARGIN;

    WaitTOF();

    if (visible)
    {
        SetAPen(rp, 1);
        SetDrMd(rp, JAM1);
        Move(rp, x, y + font->tf_Baseline);
        Text(rp, (STRPTR)infoText, length);
    }
    else
    {
        SetAPen(rp, 0);
        RectFill(rp, x, y, x + width - 1, y + font->tf_YSize - 1);
    }
}
