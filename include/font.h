#ifndef FONT_H
#define FONT_H

#include "amiga.h"

/*
 * Native bitmap metrics. Keep these dimensions in sync with SIZES in
 * tools/generate_font_bitmap.py when regenerating font_bitmap.inc.
 */
#define FONT_ROW_BYTES(width) ((((width) + 31) / 32) * 4)

#define FONT_COMPACT_WIDTH    56
#define FONT_COMPACT_HEIGHT   88
#define FONT_COMPACT_ADVANCE  62
#define FONT_COMPACT_ROW_BYTES FONT_ROW_BYTES(FONT_COMPACT_WIDTH)

#define FONT_LARGE_WIDTH      44
#define FONT_LARGE_HEIGHT     72
#define FONT_LARGE_ADVANCE    48
#define FONT_LARGE_ROW_BYTES  FONT_ROW_BYTES(FONT_LARGE_WIDTH)

#define FONT_SMALL_WIDTH      18
#define FONT_SMALL_HEIGHT     18
#define FONT_SMALL_ADVANCE    12
#define FONT_SMALL_ROW_BYTES  FONT_ROW_BYTES(FONT_SMALL_WIDTH)

BOOL FontInit(BOOL showSeconds);
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
