#ifndef CLOCK_H
#define CLOCK_H

#include "amiga.h"

/*
 * Plain calendar/time snapshot. Deliberately independent from any Exec/DOS
 * structure so the rest of the application never needs to know that the
 * data actually comes from DateStamp().
 */
struct ClockTime
{
    UWORD ct_Day;            /* 1..31   */
    UWORD ct_Month;          /* 1..12   */
    UWORD ct_Year;           /* e.g. 2026 */
    UWORD ct_Hour;           /* 0..23   */
    UWORD ct_Min;            /* 0..59   */
    UWORD ct_Sec;            /* 0..59   */
    UWORD ct_Weekday;        /* 0=Sunday .. 6=Saturday */
    ULONG ct_AbsoluteMinute; /* minutes since 1 Jan 1978 */
};

/* Length of the buffers required by ClockFormatTime()/ClockFormatDate() */
#define CLOCK_TIME_LEN 9  /* "hh:mm:ss" + NUL */
#define CLOCK_DATE_LEN 15 /* "DDD dd/mm/yyyy" + NUL */

/*
 * Reads the current date and time (via DateStamp()) and fills 'ct'.
 * The day count returned by DOS (days since 1 Jan 1978) is converted to
 * day/month/year with a small local algorithm; no AmigaOS 2.x-only
 * date API (e.g. Amiga2Date()) is used.
 */
void ClockNow(struct ClockTime *ct);

/*
 * Formats 'ct' as "hh:mm:ss" into 'buffer'.
 * 'buffer' must be at least CLOCK_TIME_LEN bytes long.
 * Does not depend on DateToStr() or any sprintf-style formatter.
 */
void ClockFormatTime(const struct ClockTime *ct, char *buffer,
                     BOOL showSeconds);

/*
 * Formats 'ct' as an Italian abbreviated weekday plus "dd/mm/yyyy".
 * 'buffer' must be at least CLOCK_DATE_LEN bytes long.
 * Does not depend on DateToStr() or any sprintf-style formatter.
 */
void ClockFormatDate(const struct ClockTime *ct, char *buffer);

/*
 * TRUE if 'a' represents a later second than 'b'. Used by the caller to
 * decide whether a redraw is actually needed.
 */
BOOL ClockChanged(const struct ClockTime *a, const struct ClockTime *b);

#endif /* CLOCK_H */
