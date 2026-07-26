#include "amiga.h"
#include "app.h"
#include "window.h"
#include "timer.h"
#include "clock.h"
#include "render.h"
#include "audio.h"

#define TICK_SECONDS 0L
#define TICK_MICROS 200000UL
#define FLIP_TICK_MICROS 60000UL
#define FLIP_LAST_FRAME 4
#define POINTER_HIDE_MICROS 10000000UL

static BOOL darkBackgroundFor(const struct ClockTime *time,
                              const struct AppConfig *config, ULONG startMinute)
{
    ULONG phase;

    if (config->ac_InvertMinutes == 0) return config->ac_StartDark;

    phase =
        ((time->ct_AbsoluteMinute - startMinute) / config->ac_InvertMinutes) &
        1UL;
    return (BOOL)(config->ac_StartDark ^ (phase != 0));
}

int AppMain(const struct AppConfig *config)
{
    struct Window *win;
    struct TimerContext timer;
    struct RenderContext render;
    struct AudioContext audio;
    struct ClockTime current;
    struct ClockTime previous;
    char timeText[CLOCK_TIME_LEN];
    char dateText[CLOCK_DATE_LEN];
    ULONG winSig;
    ULONG timerSig;
    ULONG paletteStartMinute;
    ULONG pointerIdleMicros;
    BOOL pointerHidden;
    BOOL audioReady;
    BOOL flipActive;
    WORD flipFrame;
    struct ClockTime flipTarget;
    BOOL done;

    win = OpenAppWindow();
    if (!win) return 20;

    if (!openTimer(&timer))
    {
        CloseAppWindow(win);
        return 20;
    }

    if (!RenderInit(&render, win, config->ac_ShowSeconds, config->ac_Analog))
    {
        closeTimer(&timer);
        CloseAppWindow(win);
        return 20;
    }

    audioReady = config->ac_Chime && AudioInit(&audio);

    /* Paint the initial state right away, don't wait for the first tick */
    ClockNow(&previous);
    paletteStartMinute = previous.ct_AbsoluteMinute;
    ClockFormatTime(&previous, timeText, config->ac_ShowSeconds);
    ClockFormatDate(&previous, dateText);
    if (config->ac_Analog)
        RenderAnalogClock(
            &render, &previous, config->ac_ShowSeconds,
            darkBackgroundFor(&previous, config, paletteStartMinute));
    else
        RenderDigitalClock(
            &render, timeText, dateText, config->ac_ShowSeconds,
            darkBackgroundFor(&previous, config, paletteStartMinute));
    RenderSetInfoVisible(&render, TRUE);

    winSig = 1L << win->UserPort->mp_SigBit;
    timerSig = 1L << timer.tc_Port->mp_SigBit;

    startTimer(&timer, TICK_SECONDS, TICK_MICROS);

    pointerIdleMicros = 0;
    pointerHidden = FALSE;
    flipActive = FALSE;
    flipFrame = 0;
    done = FALSE;
    while (!done)
    {
        ULONG sig = Wait(winSig | timerSig | SIGBREAKF_CTRL_C);

        if (sig & timerSig)
        {
            completeTimer(&timer);

            if (!pointerHidden)
            {
                pointerIdleMicros +=
                    flipActive ? FLIP_TICK_MICROS : TICK_MICROS;
                if (pointerIdleMicros >= POINTER_HIDE_MICROS)
                {
                    if (WindowHidePointer(win))
                    {
                        pointerHidden = TRUE;
                        RenderSetInfoVisible(&render, FALSE);
                        if (!config->ac_Analog && !config->ac_DateAlwaysVisible)
                            RenderSetDateVisible(&render, FALSE);
                    }
                    pointerIdleMicros = POINTER_HIDE_MICROS;
                }
            }

            ClockNow(&current);
            if (flipActive)
            {
                flipFrame++;
                if (flipFrame < FLIP_LAST_FRAME)
                    RenderDigitalFlipFrame(
                        &render, timeText, config->ac_ShowSeconds,
                        darkBackgroundFor(&flipTarget, config,
                                          paletteStartMinute),
                        flipFrame);
                else
                {
                    RenderDigitalClock(&render, timeText, dateText,
                                       config->ac_ShowSeconds,
                                       darkBackgroundFor(&flipTarget, config,
                                                         paletteStartMinute));
                    previous = flipTarget;
                    flipActive = FALSE;
                }
            }

            if (!flipActive && ClockChanged(&current, &previous))
            {
                if (audioReady && current.ct_Min == 0 &&
                    current.ct_Hour != previous.ct_Hour)
                    AudioPlayChime(&audio);

                ClockFormatTime(&current, timeText, config->ac_ShowSeconds);
                ClockFormatDate(&current, dateText);
                if (config->ac_Analog)
                    RenderAnalogClock(&render, &current, config->ac_ShowSeconds,
                                      darkBackgroundFor(&current, config,
                                                        paletteStartMinute));
                else if (config->ac_Flip && (config->ac_ShowSeconds ||
                                             current.ct_AbsoluteMinute !=
                                                 previous.ct_AbsoluteMinute))
                {
                    flipTarget = current;
                    flipFrame = 1;
                    flipActive = TRUE;
                    RenderDigitalFlipFrame(
                        &render, timeText, config->ac_ShowSeconds,
                        darkBackgroundFor(&flipTarget, config,
                                          paletteStartMinute),
                        flipFrame);
                }
                else
                    RenderDigitalClock(&render, timeText, dateText,
                                       config->ac_ShowSeconds,
                                       darkBackgroundFor(&current, config,
                                                         paletteStartMinute));
                if (!flipActive) previous = current;
            }

            startTimer(&timer, TICK_SECONDS,
                       flipActive ? FLIP_TICK_MICROS : TICK_MICROS);
        }

        if (sig & winSig)
        {
            BOOL mouseMoved;

            if (WindowProcessMessages(win, &mouseMoved)) done = TRUE;

            if (mouseMoved)
            {
                pointerIdleMicros = 0;
                if (pointerHidden)
                {
                    WindowShowPointer(win);
                    pointerHidden = FALSE;
                    RenderSetInfoVisible(&render, TRUE);
                    if (!config->ac_Analog && !config->ac_DateAlwaysVisible)
                        RenderSetDateVisible(&render, TRUE);
                }
            }
        }

        if (sig & SIGBREAKF_CTRL_C) done = TRUE;
    }

    if (audioReady) AudioExit(&audio);
    RenderExit(&render);
    closeTimer(&timer);
    CloseAppWindow(win);

    return 0;
}
