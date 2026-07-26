#ifndef RENDER_H
#define RENDER_H

#include "amiga.h"
#include "clock.h"

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
    struct Window   *rc_Window;
    WORD             rc_PrevX;
    WORD             rc_PrevY;
    WORD             rc_PrevWidth;
    WORD             rc_PrevHeight;
    BOOL             rc_HasPrev;
    char             rc_PrevTime[CLOCK_TIME_LEN];
    char             rc_PrevDate[CLOCK_DATE_LEN];
    WORD             rc_PrevTimeX;
    WORD             rc_PrevDateX;
    WORD             rc_PrevLineY;
    WORD             rc_PrevDateY;
    WORD             rc_PrevTimeHeight;
    WORD             rc_PrevDateHeight;
    struct ViewPort  *rc_ViewPort;
    UWORD             rc_SavedColor0;
    UWORD             rc_SavedColor1;
    BOOL              rc_HasSavedPalette;
    BOOL              rc_DarkBackground;
    BOOL              rc_HasClockPalette;
};

/*
 * Binds the context to the window it will draw into.
 */
BOOL RenderInit(struct RenderContext *rc, struct Window *win,
                BOOL showSeconds);

/* Completes renderer shutdown. */
void RenderExit(struct RenderContext *rc);

/*
 * Draws 'time' large and 'date' smaller below it, both centered
 * horizontally as a block in the window using the embedded font (see
 * font.h). Only the area occupied by the previous draw (if any) is
 * erased first.
 */
void RenderClock(struct RenderContext *rc, const char *time, const char *date,
                 BOOL showSeconds, BOOL darkBackground);

#endif /* RENDER_H */
