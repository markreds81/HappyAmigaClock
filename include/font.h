#ifndef FONT_H
#define FONT_H

#include "amiga.h"

BOOL FontInit(void);
void FontExit(void);

/*
 * Self-contained one-bit sans-serif font embedded in the executable.
 * Glyphs are rendered as exact horizontal spans, without system fonts,
 * polygon filling or dependencies on the target machine.
 */

/* Width, in pixels, that 's' would occupy if drawn 'height' pixels tall */
WORD FontStringWidth(const char *s, WORD height);

/* Horizontal distance, in pixels, from one character's origin to the
   next when drawn 'height' pixels tall. Lets callers address individual
   character cells (e.g. to redraw only the characters that changed). */
WORD FontCharAdvance(WORD height);

/* Draws a single character with its top-left corner at (x, y), 'height'
   pixels tall, using the RastPort's current foreground pen. */
void FontDrawChar(struct RastPort *rp, char c, WORD x, WORD y, WORD height);

/* Draws 's' with its top-left corner at (x, y), 'height' pixels tall,
   using the RastPort's current foreground pen (set via SetAPen()). */
void FontDrawString(struct RastPort *rp, const char *s, WORD x, WORD y, WORD height);

#endif /* FONT_H */
