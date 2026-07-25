#include "clock.h"

/* Days per month, indexed [isLeapYear][month0] */
static const UWORD daysInMonth[2][12] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, /* common year */
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }  /* leap year   */
};

/* Gregorian leap year rule, implemented locally (no utility.library needed) */
static BOOL isLeapYear(UWORD year)
{
    return (BOOL)(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

/*
 * Converts a day count relative to 1 Jan 1978 (as returned in
 * DateStamp.ds_Days) into day/month/year.
 *
 * This is a plain, self-contained calendar calculation: no Amiga2Date(),
 * CheckDate() or any other utility.library call introduced with AmigaOS
 * 2.x is used, keeping the module usable under Kickstart 1.3.
 */
static void daysToDate(LONG days, UWORD *day, UWORD *month, UWORD *year)
{
    UWORD y = 1978;
    UWORD m;
    BOOL leap;

    /* Consume whole years until we land in the correct one */
    for (;;)
    {
        UWORD yearLength = isLeapYear(y) ? 366 : 365;

        if (days < (LONG)yearLength)
            break;

        days -= yearLength;
        y++;
    }

    /* Consume whole months until we land in the correct one */
    leap = isLeapYear(y);
    for (m = 0; m < 12; m++)
    {
        UWORD monthLength = daysInMonth[leap][m];

        if (days < (LONG)monthLength)
            break;

        days -= monthLength;
    }

    *year  = y;
    *month = m + 1;
    *day   = (UWORD)days + 1;
}

void ClockNow(struct ClockTime *ct)
{
    struct DateStamp ds;

    /* DateStamp() is a DOS 33-era call, valid back to Kickstart 1.3 */
    DateStamp(&ds);

    daysToDate(ds.ds_Days, &ct->ct_Day, &ct->ct_Month, &ct->ct_Year);

    ct->ct_Hour = (UWORD)(ds.ds_Minute / 60);
    ct->ct_Min  = (UWORD)(ds.ds_Minute % 60);
    ct->ct_Sec  = (UWORD)(ds.ds_Tick / TICKS_PER_SECOND);
}

/* Writes 'value' as 'digits' decimal characters, zero-padded, no NUL */
static char *putDigits(char *dst, UWORD value, WORD digits)
{
    char tmp[6];
    WORD i;

    for (i = digits - 1; i >= 0; i--)
    {
        tmp[i] = (char)('0' + (value % 10));
        value /= 10;
    }

    for (i = 0; i < digits; i++)
        *dst++ = tmp[i];

    return dst;
}

void ClockFormat(const struct ClockTime *ct, char *buffer)
{
    char *p = buffer;

    p = putDigits(p, ct->ct_Day, 2);
    *p++ = '/';
    p = putDigits(p, ct->ct_Month, 2);
    *p++ = '/';
    p = putDigits(p, ct->ct_Year, 4);
    *p++ = ' ';
    p = putDigits(p, ct->ct_Hour, 2);
    *p++ = ':';
    p = putDigits(p, ct->ct_Min, 2);
    *p++ = ':';
    p = putDigits(p, ct->ct_Sec, 2);
    *p   = '\0';
}

BOOL ClockChanged(const struct ClockTime *a, const struct ClockTime *b)
{
    /* DateStamp() has one-second granularity, so the seconds field alone
       is enough to detect that a new second has begun - this also covers
       minute/hour/day rollovers, since ct_Sec always changes with them. */
    return (BOOL)(a->ct_Sec != b->ct_Sec);
}
