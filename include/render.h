#ifndef RENDER_H
#define RENDER_H

#include "compiler.h"

/*
 * Tracks the screen area last used to draw a string, so that the next
 * redraw only has to clear that rectangle instead of the whole window.
 */
struct RenderContext
{
    struct Window *rc_Window;
    WORD           rc_PrevX;
    WORD           rc_PrevY;
    WORD           rc_PrevWidth;
    WORD           rc_PrevHeight;
    BOOL           rc_HasPrev;
};

/* Binds the context to the window it will draw into. */
void RenderInit(struct RenderContext *rc, struct Window *win);

/*
 * Centers 'text' horizontally and vertically in the window, using the
 * RastPort's current font and TextLength() to measure it. Only the area
 * occupied by the previous draw (if any) is erased first.
 */
void RenderText(struct RenderContext *rc, const char *text);

#endif /* RENDER_H */
