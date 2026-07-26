#include "amiga.h"
#include "app.h"
#include "window.h"
#include "timer.h"
#include "clock.h"
#include "render.h"

/*
 * Timer tick period: ~200 ms rather than a full second. Polling this
 * often means the second actually displayed can lag reality by at most
 * ~200 ms, and - since the loop always resyncs against DateStamp() - no
 * drift accumulates over time, unlike naively resubmitting a 1-second
 * request after each redraw.
 */
#define TICK_SECONDS 0L
#define TICK_MICROS  200000UL

static BOOL darkBackgroundFor(const struct ClockTime *time,
                              const struct AppConfig *config,
                              ULONG startMinute)
{
    ULONG phase;

    if (config->ac_InvertMinutes == 0)
        return config->ac_StartDark;

    phase = ((time->ct_AbsoluteMinute - startMinute) /
             config->ac_InvertMinutes) & 1UL;
    return (BOOL)(config->ac_StartDark ^ (phase != 0));
}

int AppMain(const struct AppConfig *config)
{
    struct Window         *win;
    struct TimerContext    timer;
    struct RenderContext   render;
    struct ClockTime       current;
    struct ClockTime       previous;
    char                   timeText[CLOCK_TIME_LEN];
    char                   dateText[CLOCK_DATE_LEN];
    ULONG                  winSig;
    ULONG                  timerSig;
    ULONG                  paletteStartMinute;
    BOOL                   done;

    win = OpenAppWindow();
    if (!win)
        return 20;

    if (!openTimer(&timer))
    {
        CloseAppWindow(win);
        return 20;
    }

    if (!RenderInit(&render, win, config->ac_ShowSeconds))
    {
        closeTimer(&timer);
        CloseAppWindow(win);
        return 20;
    }

    /* Paint the initial state right away, don't wait for the first tick */
    ClockNow(&previous);
    paletteStartMinute = previous.ct_AbsoluteMinute;
    ClockFormatTime(&previous, timeText, config->ac_ShowSeconds);
    ClockFormatDate(&previous, dateText);
    RenderClock(&render, timeText, dateText, config->ac_ShowSeconds,
                darkBackgroundFor(&previous, config, paletteStartMinute));

    winSig   = 1L << win->UserPort->mp_SigBit;
    timerSig = 1L << timer.tc_Port->mp_SigBit;

    startTimer(&timer, TICK_SECONDS, TICK_MICROS);

    done = FALSE;
    while (!done)
    {
        /* Wait simultaneously on Intuition, the timer and a Ctrl+C break
           from the shell - the loop never blocks on just one source. */
        ULONG sig = Wait(winSig | timerSig | SIGBREAKF_CTRL_C);

        if (sig & timerSig)
        {
            completeTimer(&timer);

            ClockNow(&current);
            if (ClockChanged(&current, &previous))
            {
                ClockFormatTime(&current, timeText, config->ac_ShowSeconds);
                ClockFormatDate(&current, dateText);
                RenderClock(&render, timeText, dateText,
                            config->ac_ShowSeconds,
                            darkBackgroundFor(&current, config,
                                              paletteStartMinute));
                previous = current;
            }

            /* Rearm immediately so the next tick is still ~200 ms away */
            startTimer(&timer, TICK_SECONDS, TICK_MICROS);
        }

        if (sig & winSig)
        {
            if (WindowProcessMessages(win))
                done = TRUE;
        }

        if (sig & SIGBREAKF_CTRL_C)
            done = TRUE;
    }

    RenderExit(&render);
    closeTimer(&timer);
    CloseAppWindow(win);

    return 0;
}
