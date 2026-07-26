#include "font.h"

/*
 * Three native one-bit sizes, drawn by the Amiga blitter in one operation.
 * There is deliberately no run-by-run drawing and no runtime scaling.
 */
#include "font_bitmap.inc"

#define COMPACT_BYTES ((ULONG)sizeof(glyph_compact))
#define LARGE_BYTES ((ULONG)sizeof(glyph_large))
#define SMALL_BYTES ((ULONG)sizeof(glyph_small))

static APTR chipTime = NULL;
static ULONG chipTimeBytes = 0;
static APTR chipSmall = NULL;
static const char timeGlyphChars[] = "0123456789:";
static const char dateGlyphChars[] = "0123456789/ LUNMAREGIOVDSB";

BOOL FontInit(BOOL showSeconds)
{
    if (showSeconds)
        chipTimeBytes = LARGE_BYTES;
    else
        chipTimeBytes = COMPACT_BYTES;

    chipTime = AllocMem(chipTimeBytes, MEMF_CHIP);
    if (!chipTime)
    {
        chipTimeBytes = 0;
        return FALSE;
    }

    chipSmall = AllocMem(SMALL_BYTES, MEMF_CHIP);
    if (!chipSmall)
    {
        FreeMem(chipTime, chipTimeBytes);
        chipTime = NULL;
        chipTimeBytes = 0;
        return FALSE;
    }

    if (showSeconds)
        CopyMem((APTR)glyph_large, chipTime, LARGE_BYTES);
    else
        CopyMem((APTR)glyph_compact, chipTime, COMPACT_BYTES);
    CopyMem((APTR)glyph_small, chipSmall, SMALL_BYTES);
    return TRUE;
}

void FontExit(void)
{
    if (chipSmall)
    {
        FreeMem(chipSmall, SMALL_BYTES);
        chipSmall = NULL;
    }
    if (chipTime)
    {
        FreeMem(chipTime, chipTimeBytes);
        chipTime = NULL;
        chipTimeBytes = 0;
    }
}

static WORD glyphIndex(char c, const char *glyphChars)
{
    WORD index = 0;

    while (*glyphChars)
    {
        if (*glyphChars++ == c) return index;
        index++;
    }

    return -1;
}

static BOOL isCompact(WORD height)
{
    return height >= FONT_COMPACT_HEIGHT;
}

static BOOL isLarge(WORD height)
{
    return height >= FONT_LARGE_HEIGHT && !isCompact(height);
}

static WORD textStrLen(const char *s)
{
    WORD n = 0;

    while (*s++)
        n++;
    return n;
}

WORD FontStringWidth(const char *s, WORD height)
{
    WORD len = textStrLen(s);
    WORD width;
    WORD total = 0;
    const char *p = s;

    if (isCompact(height))
    {
        width = FONT_COMPACT_WIDTH;
    }
    else if (isLarge(height))
    {
        width = FONT_LARGE_WIDTH;
    }
    else
    {
        width = FONT_SMALL_WIDTH;
    }

    if (len == 0) return 0;

    while (p[1] != '\0')
    {
        total += FontCharAdvance(*p, height);
        p++;
    }

    return total + width;
}

WORD FontCharAdvance(char c, WORD height)
{
    if (isCompact(height)) return FONT_COMPACT_ADVANCE;
    if (isLarge(height)) return FONT_LARGE_ADVANCE;

    if (c >= 'A' && c <= 'Z') return FONT_SMALL_LETTER_ADVANCE;
    if (c == ' ') return FONT_SMALL_SPACE_ADVANCE;

    return FONT_SMALL_ADVANCE;
}

void FontDrawChar(struct RastPort *rp, char c, WORD x, WORD y, WORD height)
{
    BOOL small = !isCompact(height) && !isLarge(height);
    WORD index = glyphIndex(c, small ? dateGlyphChars : timeGlyphChars);

    if (index < 0) return;

    SetDrMd(rp, JAM1);
    if (isCompact(height))
        BltTemplate((PLANEPTR)((UBYTE *)chipTime + index * FONT_COMPACT_HEIGHT *
                                                       FONT_COMPACT_ROW_BYTES),
                    0, FONT_COMPACT_ROW_BYTES, rp, x, y, FONT_COMPACT_WIDTH,
                    FONT_COMPACT_HEIGHT);
    else if (isLarge(height))
        BltTemplate((PLANEPTR)((UBYTE *)chipTime + index * FONT_LARGE_HEIGHT *
                                                       FONT_LARGE_ROW_BYTES),
                    0, FONT_LARGE_ROW_BYTES, rp, x, y, FONT_LARGE_WIDTH,
                    FONT_LARGE_HEIGHT);
    else
        BltTemplate((PLANEPTR)((UBYTE *)chipSmall + index * FONT_SMALL_HEIGHT *
                                                        FONT_SMALL_ROW_BYTES),
                    0, FONT_SMALL_ROW_BYTES, rp, x, y, FONT_SMALL_WIDTH,
                    FONT_SMALL_HEIGHT);
}

void FontDrawString(struct RastPort *rp, const char *s, WORD x, WORD y,
                    WORD height)
{
    while (*s)
    {
        char c = *s++;

        FontDrawChar(rp, c, x, y, height);
        x += FontCharAdvance(c, height);
    }
}
