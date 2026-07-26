#ifndef RENDER_H
#define RENDER_H

#include "amiga.h"
#include "clock.h"

#define RENDER_FLIP_HALF_FRAME 4
#define RENDER_FLIP_LAST_FRAME 9

/*
 * Tracks the screen area and text last used to draw the clock, so that
 * a redraw can (a) touch only the previous bounding rectangle instead of
 * the whole window, and (b) - when the layout hasn't changed - erase and
 * redraw only the individual characters that actually differ from the
 * previous frame. The latter is what keeps each tick's drawing work (and
 * so the visible flicker on a single-buffered screen) small: on a real
 * clock, typically only the last digit or two changes per second.
 */
struct RenderContext
{
    struct Window *rc_Window;
    WORD rc_PrevX;
    WORD rc_PrevY;
    WORD rc_PrevWidth;
    WORD rc_PrevHeight;
    BOOL rc_HasPrev;
    char rc_PrevTime[CLOCK_TIME_LEN];
    char rc_PrevDate[CLOCK_DATE_LEN];
    WORD rc_PrevTimeX;
    WORD rc_PrevDateX;
    WORD rc_PrevLineY;
    WORD rc_PrevDateY;
    WORD rc_PrevTimeHeight;
    WORD rc_PrevDateHeight;
    struct ViewPort *rc_ViewPort;
    UWORD rc_SavedColor0;
    UWORD rc_SavedColor1;
    BOOL rc_HasSavedPalette;
    BOOL rc_DarkBackground;
    BOOL rc_HasClockPalette;
    BOOL rc_DateVisible;
    BOOL rc_FontReady;
    struct BitMap rc_TimeBufferBitMap;
    struct RastPort rc_TimeBufferRastPort;
    PLANEPTR rc_TimeBufferPlane;
    WORD rc_TimeBufferWidth;
    WORD rc_TimeBufferHeight;
    BOOL rc_TimeBufferReady;
    BOOL rc_AnalogHasPrev;
    UWORD rc_PrevAnalogHourTick;
    UWORD rc_PrevAnalogMinuteTick;
    UWORD rc_PrevAnalogSecondTick;
};

/*
 * Binds the context to the window it will draw into.
 */
BOOL RenderInit(struct RenderContext *rc, struct Window *win, BOOL showSeconds,
                BOOL analog, BOOL flip);

/* Completes renderer shutdown. */
void RenderExit(struct RenderContext *rc);

/*
 * Draws 'time' large in the centre and 'date' smaller near the top,
 * both centered horizontally using the embedded font (see font.h).
 * Only the area occupied by the previous draw (if any) is erased first.
 */
void RenderDigitalClock(struct RenderContext *rc, const char *time,
                        const char *date, BOOL showSeconds,
                        BOOL darkBackground);

/*
 * Draws one intermediate frame of the optional split-flap transition.
 * frame is 1..8; the caller completes the transition with
 * RenderDigitalClock(), which commits the new text to the context.
 */
void RenderDigitalFlipFrame(struct RenderContext *rc, const char *time,
                            BOOL showSeconds, BOOL darkBackground, WORD frame);

/* Draws the full-screen analogue face and its three hands. */
void RenderAnalogClock(struct RenderContext *rc, const struct ClockTime *time,
                       BOOL showSeconds, BOOL darkBackground);

/* Shows or erases the application name/version in the system font. */
void RenderSetInfoVisible(struct RenderContext *rc, BOOL visible);

/* Shows or erases the date without disturbing the centered time. */
void RenderSetDateVisible(struct RenderContext *rc, BOOL visible);

#endif /* RENDER_H */
