#include "font.h"

/*
 * Three native one-bit sizes, drawn by the Amiga blitter in one operation.
 * There is deliberately no run-by-run drawing and no runtime scaling.
 */
#define COMPACT_W       52
#define COMPACT_H       88
#define COMPACT_ADVANCE 62
#define LARGE_W       36
#define LARGE_H       64
#define LARGE_ADVANCE 40
#define SMALL_W       16
#define SMALL_H       24
#define SMALL_ADVANCE 18

#include "font_bitmap.inc"

#define COMPACT_BYTES ((ULONG)sizeof(glyph_compact))
#define LARGE_BYTES ((ULONG)sizeof(glyph_large))
#define SMALL_BYTES ((ULONG)sizeof(glyph_small))

static APTR chipCompact = NULL;
static APTR chipLarge = NULL;
static APTR chipSmall = NULL;

BOOL FontInit(void)
{
    chipCompact = AllocMem(COMPACT_BYTES, MEMF_CHIP);
    if (!chipCompact)
        return FALSE;

    chipLarge = AllocMem(LARGE_BYTES, MEMF_CHIP);
    if (!chipLarge)
    {
        FreeMem(chipCompact, COMPACT_BYTES);
        chipCompact = NULL;
        return FALSE;
    }

    chipSmall = AllocMem(SMALL_BYTES, MEMF_CHIP);
    if (!chipSmall)
    {
        FreeMem(chipLarge, LARGE_BYTES);
        chipLarge = NULL;
        FreeMem(chipCompact, COMPACT_BYTES);
        chipCompact = NULL;
        return FALSE;
    }

    CopyMem((APTR)glyph_compact, chipCompact, COMPACT_BYTES);
    CopyMem((APTR)glyph_large, chipLarge, LARGE_BYTES);
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
    if (chipLarge)
    {
        FreeMem(chipLarge, LARGE_BYTES);
        chipLarge = NULL;
    }
    if (chipCompact)
    {
        FreeMem(chipCompact, COMPACT_BYTES);
        chipCompact = NULL;
    }
}

static WORD glyphIndex(char c)
{
    if (c >= '0' && c <= '9')
        return (WORD)(c - '0');
    if (c == ':')
        return 10;
    if (c == '/')
        return 11;
    return -1;
}

static BOOL isCompact(WORD height)
{
    return height >= COMPACT_H;
}

static BOOL isLarge(WORD height)
{
    return height >= LARGE_H && !isCompact(height);
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
    WORD advance;

    if (isCompact(height))
    {
        width = COMPACT_W;
        advance = COMPACT_ADVANCE;
    }
    else if (isLarge(height))
    {
        width = LARGE_W;
        advance = LARGE_ADVANCE;
    }
    else
    {
        width = SMALL_W;
        advance = SMALL_ADVANCE;
    }

    if (len == 0)
        return 0;
    return (WORD)((len - 1) * advance + width);
}

WORD FontCharAdvance(WORD height)
{
    if (isCompact(height))
        return COMPACT_ADVANCE;
    if (isLarge(height))
        return LARGE_ADVANCE;
    return SMALL_ADVANCE;
}

void FontDrawChar(struct RastPort *rp, char c, WORD x, WORD y, WORD height)
{
    WORD index = glyphIndex(c);

    if (index < 0)
        return;

    SetDrMd(rp, JAM1);
    if (isCompact(height))
        BltTemplate((PLANEPTR)((UBYTE *)chipCompact +
                               index * COMPACT_H * 8),
                    0, 8, rp, x, y, COMPACT_W, COMPACT_H);
    else if (isLarge(height))
        BltTemplate((PLANEPTR)((UBYTE *)chipLarge + index * LARGE_H * 8),
                    0, 8,
                    rp, x, y, LARGE_W, LARGE_H);
    else
        BltTemplate((PLANEPTR)((UBYTE *)chipSmall + index * SMALL_H * 4),
                    0, 4,
                    rp, x, y, SMALL_W, SMALL_H);
}

void FontDrawString(struct RastPort *rp, const char *s, WORD x, WORD y, WORD height)
{
    WORD advance = FontCharAdvance(height);

    while (*s)
    {
        FontDrawChar(rp, *s++, x, y, height);
        x += advance;
    }
}
