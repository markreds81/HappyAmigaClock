#include "render.h"
#include "font.h"
#include "version.h"

/* Time stays centered; date is placed near the top edge. */
#define DATE_TOP_PERCENT 6
#define INFO_BOTTOM_MARGIN 6

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
        (rc->rc_HasClockPalette && rc->rc_DarkBackground == darkBackground))
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
static void redrawChangedChars(struct RastPort *rp, const char *prevText,
                               const char *newText, WORD x, WORD y, WORD height)
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

BOOL RenderInit(struct RenderContext *rc, struct Window *win, BOOL showSeconds,
                BOOL analog)
{
    struct RastPort *rp = win->RPort;

    rc->rc_Window = win;
    rc->rc_PrevX = 0;
    rc->rc_PrevY = 0;
    rc->rc_PrevWidth = 0;
    rc->rc_PrevHeight = 0;
    rc->rc_HasPrev = FALSE;
    rc->rc_ViewPort = &win->WScreen->ViewPort;
    rc->rc_HasSavedPalette = FALSE;
    rc->rc_HasClockPalette = FALSE;
    rc->rc_DateVisible = TRUE;
    rc->rc_FontReady = FALSE;
    rc->rc_AnalogHasPrev = FALSE;

    if (rc->rc_ViewPort->ColorMap && rc->rc_ViewPort->ColorMap->Count >= 2 &&
        rc->rc_ViewPort->ColorMap->ColorTable)
    {
        UWORD *colors = (UWORD *)rc->rc_ViewPort->ColorMap->ColorTable;

        rc->rc_SavedColor0 = colors[0];
        rc->rc_SavedColor1 = colors[1];
        rc->rc_HasSavedPalette = TRUE;
    }

    SetBPen(rp, 0);
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, win->Width - 1, win->Height - 1);

    if (!analog) rc->rc_FontReady = FontInit(showSeconds);

    return analog || rc->rc_FontReady;
}

void RenderExit(struct RenderContext *rc)
{
    if (rc->rc_HasSavedPalette)
    {
        setRGB4Value(rc->rc_ViewPort, 0, rc->rc_SavedColor0);
        setRGB4Value(rc->rc_ViewPort, 1, rc->rc_SavedColor1);
    }

    if (rc->rc_FontReady) FontExit();
}

void RenderDigitalClock(struct RenderContext *rc, const char *time,
                        const char *date, BOOL showSeconds, BOOL darkBackground)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    WORD screenHeight = rc->rc_Window->Height;
    WORD timeHeight = showSeconds ? FONT_LARGE_HEIGHT : FONT_COMPACT_HEIGHT;
    WORD dateHeight = FONT_SMALL_HEIGHT;
    WORD timeWidth = FontStringWidth(time, timeHeight);
    WORD dateWidth = FontStringWidth(date, dateHeight);
    WORD timeX = (rc->rc_Window->Width - timeWidth) / 2;
    WORD timeY = (screenHeight - timeHeight) / 2;
    WORD dateX = (rc->rc_Window->Width - dateWidth) / 2;
    WORD dateY = (screenHeight * DATE_TOP_PERCENT) / 100;
    WORD blockX = (timeX < dateX) ? timeX : dateX;
    WORD blockY = (timeY < dateY) ? timeY : dateY;
    WORD blockRight = ((timeX + timeWidth) > (dateX + dateWidth))
                          ? timeX + timeWidth
                          : dateX + dateWidth;
    WORD blockBottom = ((timeY + timeHeight) > (dateY + dateHeight))
                           ? timeY + timeHeight
                           : dateY + dateHeight;
    WORD blockWidth = blockRight - blockX;
    WORD blockHeight = blockBottom - blockY;
    BOOL sameLayout = rc->rc_HasPrev && rc->rc_PrevTimeX == timeX &&
                      rc->rc_PrevDateX == dateX && rc->rc_PrevLineY == timeY &&
                      rc->rc_PrevDateY == dateY &&
                      rc->rc_PrevTimeHeight == timeHeight &&
                      rc->rc_PrevDateHeight == dateHeight;

    setClockPalette(rc, darkBackground);

    /* Sync to the vertical blank so the (now small) update lands entirely
       within the blanking interval instead of tearing across a frame
       that's actively being scanned out - this is what removed the
       flicker on the real/emulated single-buffered screen. */
    WaitTOF();

    if (sameLayout)
    {
        redrawChangedChars(rp, rc->rc_PrevTime, time, timeX, timeY, timeHeight);
        if (rc->rc_DateVisible)
            redrawChangedChars(rp, rc->rc_PrevDate, date, dateX, dateY,
                               dateHeight);
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
        if (rc->rc_DateVisible)
            FontDrawString(rp, date, dateX, dateY, dateHeight);
    }

    copyStr(rc->rc_PrevTime, time);
    copyStr(rc->rc_PrevDate, date);

    rc->rc_PrevX = blockX;
    rc->rc_PrevY = blockY;
    rc->rc_PrevWidth = blockWidth;
    rc->rc_PrevHeight = blockHeight;
    rc->rc_PrevTimeX = timeX;
    rc->rc_PrevDateX = dateX;
    rc->rc_PrevLineY = timeY;
    rc->rc_PrevDateY = dateY;
    rc->rc_PrevTimeHeight = timeHeight;
    rc->rc_PrevDateHeight = dateHeight;
    rc->rc_HasPrev = TRUE;
}

/*
 * Sine values for 0, 6, ... 90 degrees, scaled by 1024.  Folding the
 * other three quadrants around this small table avoids floating point
 * maths (and its sizeable runtime library) on a 68000.
 */
static const UWORD sineQuarter[16] = {0,   107,  213,  316, 416, 512,
                                      602, 685,  761,  828, 887, 934,
                                      974, 1002, 1018, 1024};

static LONG sine60(WORD tick)
{
    tick %= 60;
    if (tick < 0) tick += 60;

    if (tick <= 15) return (LONG)sineQuarter[tick];
    if (tick <= 30) return (LONG)sineQuarter[30 - tick];
    if (tick <= 45) return -(LONG)sineQuarter[tick - 30];
    return -(LONG)sineQuarter[60 - tick];
}

static WORD clockX(WORD centre, WORD radius, WORD tick)
{
    return centre + (WORD)((sine60(tick) * (LONG)radius) / 1024L);
}

static WORD clockY(WORD centre, WORD radius, WORD tick)
{
    return centre - (WORD)((sine60(tick + 15) * (LONG)radius) / 1024L);
}

/*
 * Draws a hand as parallel one-pixel lines.  Offsetting on the minor axis
 * gives a visually even stroke without relying on newer graphics calls.
 */
static void drawThickLine(struct RastPort *rp, WORD x1, WORD y1, WORD x2,
                          WORD y2, WORD width, WORD pixelAspectX)
{
    WORD first = -(width / 2);
    WORD last = width / 2;
    WORD offset;

    if ((x2 - x1 < 0 ? x1 - x2 : x2 - x1) > (y2 - y1 < 0 ? y1 - y2 : y2 - y1))
    {
        for (offset = first; offset <= last; offset++)
        {
            Move(rp, x1, y1 + offset);
            Draw(rp, x2, y2 + offset);
        }
    }
    else
    {
        for (offset = first; offset <= last; offset++)
        {
            Move(rp, x1 + offset * pixelAspectX, y1);
            Draw(rp, x2 + offset * pixelAspectX, y2);
        }
    }
}

static void drawCentreDisc(struct RastPort *rp, WORD x, WORD y, WORD radius,
                           WORD pixelAspectX)
{
    WORD row;

    for (row = -radius; row <= radius; row++)
    {
        WORD half;
        WORD distance = row < 0 ? -row : row;

        /* A compact integer approximation of a filled circle. */
        if (distance * 3 < radius * 2)
            half = radius;
        else if (distance < radius)
            half = (radius * 3) / 4;
        else
            half = radius / 2;
        half *= pixelAspectX;
        RectFill(rp, x - half, y + row, x + half, y + row);
    }
}

static void drawAnalogIndex(struct RastPort *rp, WORD centreX, WORD centreY,
                            WORD outerX, WORD outerY, WORD innerX, WORD innerY,
                            WORD tick, WORD pixelAspectX)
{
    WORD x1 = clockX(centreX, innerX, tick);
    WORD y1 = clockY(centreY, innerY, tick);
    WORD x2 = clockX(centreX, outerX, tick);
    WORD y2 = clockY(centreY, outerY, tick);

    if ((tick % 15) == 0)
    {
        WORD tangentX = (WORD)(sine60(tick + 15) / 340L) * pixelAspectX;
        WORD tangentY = (WORD)(sine60(tick) / 340L);

        Move(rp, x1 - tangentX, y1 - tangentY);
        Draw(rp, x2 - tangentX, y2 - tangentY);
        Move(rp, x1 + tangentX, y1 + tangentY);
        Draw(rp, x2 + tangentX, y2 + tangentY);
    }
    else
    {
        drawThickLine(rp, x1, y1, x2, y2, 3, pixelAspectX);
    }
}

static void drawAnalogIndices(struct RastPort *rp, WORD centreX, WORD centreY,
                              WORD outerX, WORD outerY, WORD innerX,
                              WORD innerY, WORD pixelAspectX)
{
    WORD tick;

    for (tick = 0; tick < 60; tick += 5)
        drawAnalogIndex(rp, centreX, centreY, outerX, outerY, innerX, innerY,
                        tick, pixelAspectX);
}

static void drawHourHand(struct RastPort *rp, WORD centreX, WORD centreY,
                         WORD radiusX, WORD radiusY, WORD tick,
                         WORD pixelAspectX)
{
    drawThickLine(rp, centreX, centreY,
                  clockX(centreX, (radiusX * 47) / 100, tick),
                  clockY(centreY, (radiusY * 47) / 100, tick), 5, pixelAspectX);
}

static void drawMinuteHand(struct RastPort *rp, WORD centreX, WORD centreY,
                           WORD radiusX, WORD radiusY, WORD tick,
                           WORD pixelAspectX)
{
    drawThickLine(rp, centreX, centreY,
                  clockX(centreX, (radiusX * 72) / 100, tick),
                  clockY(centreY, (radiusY * 72) / 100, tick), 3, pixelAspectX);
}

static void drawSecondHand(struct RastPort *rp, WORD centreX, WORD centreY,
                           WORD radiusX, WORD radiusY, WORD tick)
{
    Move(rp, centreX, centreY);
    Draw(rp, clockX(centreX, (radiusX * 87) / 100, tick),
         clockY(centreY, (radiusY * 87) / 100, tick));
}

void RenderAnalogClock(struct RenderContext *rc, const struct ClockTime *time,
                       BOOL showSeconds, BOOL darkBackground)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    WORD width = rc->rc_Window->Width;
    WORD height = rc->rc_Window->Height;
    WORD centreX = width / 2;
    WORD centreY = height / 2;
    WORD pixelAspectX = (width >= height * 2) ? 2 : 1;
    WORD radiusY = (height * 36) / 100;
    WORD radiusX;
    WORD outerX;
    WORD outerY;
    WORD innerX;
    WORD innerY;
    WORD minuteTick = time->ct_Min;
    WORD hourTick = (WORD)((time->ct_Hour % 12) * 5 + time->ct_Min / 12);
    BOOL hourChanged;
    BOOL minuteChanged;
    BOOL secondCrossedHour;
    BOOL secondCrossedMinute;

    if (radiusY < 24) radiusY = 24;

    /*
     * A non-interlaced high-resolution Amiga pixel is roughly half as
     * wide as it is tall.  Doubling X distances makes the face circular
     * on the display; low-resolution and interlaced screens keep 1:1
     * bitmap geometry.
     */
    radiusX = radiusY * pixelAspectX;
    if (radiusX > width / 2 - 8) radiusX = width / 2 - 8;

    outerX = radiusX;
    outerY = radiusY;
    innerX = (radiusX * 82) / 100;
    innerY = (radiusY * 82) / 100;
    hourChanged =
        !rc->rc_AnalogHasPrev || rc->rc_PrevAnalogHourTick != (UWORD)hourTick;
    minuteChanged = !rc->rc_AnalogHasPrev ||
                    rc->rc_PrevAnalogMinuteTick != (UWORD)minuteTick;
    secondCrossedHour =
        rc->rc_AnalogHasPrev && showSeconds &&
        rc->rc_PrevAnalogSecondTick == rc->rc_PrevAnalogHourTick;
    secondCrossedMinute =
        rc->rc_AnalogHasPrev && showSeconds &&
        rc->rc_PrevAnalogSecondTick == rc->rc_PrevAnalogMinuteTick;

    setClockPalette(rc, darkBackground);
    WaitTOF();

    SetDrMd(rp, JAM1);

    if (!rc->rc_AnalogHasPrev)
    {
        SetAPen(rp, 0);
        RectFill(rp, centreX - radiusX - 8, centreY - radiusY - 8,
                 centreX + radiusX + 8, centreY + radiusY + 8);
    }
    else
    {
        /* Remove only hands whose position has actually changed. */
        SetAPen(rp, 0);
        if (hourChanged)
            drawHourHand(rp, centreX, centreY, radiusX, radiusY,
                         rc->rc_PrevAnalogHourTick, pixelAspectX);
        if (minuteChanged)
            drawMinuteHand(rp, centreX, centreY, radiusX, radiusY,
                           rc->rc_PrevAnalogMinuteTick, pixelAspectX);
        if (showSeconds)
            drawSecondHand(rp, centreX, centreY, radiusX, radiusY,
                           rc->rc_PrevAnalogSecondTick);
    }

    SetAPen(rp, 1);
    if (!rc->rc_AnalogHasPrev)
        drawAnalogIndices(rp, centreX, centreY, outerX, outerY, innerX, innerY,
                          pixelAspectX);
    else if (showSeconds && (rc->rc_PrevAnalogSecondTick % 5) == 0)
    {
        /* Only the long second hand can reach an hour index. */
        drawAnalogIndex(rp, centreX, centreY, outerX, outerY, innerX, innerY,
                        rc->rc_PrevAnalogSecondTick, pixelAspectX);
    }

    /*
     * Redrawing an unchanged hand with the same pen does not alter visible
     * pixels, but repairs the rare overlap erased by a moving hand.
     */
    if (hourChanged || secondCrossedHour)
        drawHourHand(rp, centreX, centreY, radiusX, radiusY, hourTick,
                     pixelAspectX);
    if (minuteChanged || secondCrossedMinute)
        drawMinuteHand(rp, centreX, centreY, radiusX, radiusY, minuteTick,
                       pixelAspectX);
    if (showSeconds)
        drawSecondHand(rp, centreX, centreY, radiusX, radiusY, time->ct_Sec);
    drawCentreDisc(rp, centreX, centreY, radiusY / 12, pixelAspectX);

    rc->rc_PrevAnalogHourTick = (UWORD)hourTick;
    rc->rc_PrevAnalogMinuteTick = (UWORD)minuteTick;
    rc->rc_PrevAnalogSecondTick = time->ct_Sec;
    rc->rc_AnalogHasPrev = TRUE;
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

void RenderSetDateVisible(struct RenderContext *rc, BOOL visible)
{
    struct RastPort *rp = rc->rc_Window->RPort;
    WORD width;

    if (!rc->rc_HasPrev || rc->rc_DateVisible == visible) return;

    width = FontStringWidth(rc->rc_PrevDate, rc->rc_PrevDateHeight);
    WaitTOF();

    if (visible)
    {
        SetAPen(rp, 1);
        FontDrawString(rp, rc->rc_PrevDate, rc->rc_PrevDateX, rc->rc_PrevDateY,
                       rc->rc_PrevDateHeight);
    }
    else
    {
        SetAPen(rp, 0);
        RectFill(rp, rc->rc_PrevDateX, rc->rc_PrevDateY,
                 rc->rc_PrevDateX + width,
                 rc->rc_PrevDateY + rc->rc_PrevDateHeight);
    }

    rc->rc_DateVisible = visible;
}
