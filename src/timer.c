#include "timer.h"

BOOL openTimer(struct TimerContext *tc)
{
    tc->tc_Port = NULL;
    tc->tc_Request = NULL;
    tc->tc_Running = FALSE;

    tc->tc_Port = CreatePort(NULL, 0);
    if (!tc->tc_Port) return FALSE;

    tc->tc_Request = (struct timerequest *)CreateExtIO(
        tc->tc_Port, sizeof(struct timerequest));
    if (!tc->tc_Request)
    {
        DeletePort(tc->tc_Port);
        tc->tc_Port = NULL;
        return FALSE;
    }

    if (OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ,
                   (struct IORequest *)tc->tc_Request, 0) != 0)
    {
        DeleteExtIO((struct IORequest *)tc->tc_Request);
        tc->tc_Request = NULL;
        DeletePort(tc->tc_Port);
        tc->tc_Port = NULL;
        return FALSE;
    }

    return TRUE;
}

void closeTimer(struct TimerContext *tc)
{
    if (tc->tc_Request)
    {
        if (tc->tc_Running)
        {
            /* Correct Exec shutdown sequence for an in-flight request:
               ask the device to cancel it, then wait for it to actually
               come back before touching the request again. */
            AbortIO((struct IORequest *)tc->tc_Request);
            WaitIO((struct IORequest *)tc->tc_Request);
            tc->tc_Running = FALSE;
        }

        CloseDevice((struct IORequest *)tc->tc_Request);
        DeleteExtIO((struct IORequest *)tc->tc_Request);
        tc->tc_Request = NULL;
    }

    if (tc->tc_Port)
    {
        DeletePort(tc->tc_Port);
        tc->tc_Port = NULL;
    }
}

void startTimer(struct TimerContext *tc, ULONG seconds, ULONG micros)
{
    tc->tc_Request->tr_node.io_Command = TR_ADDREQUEST;
    tc->tc_Request->tr_time.tv_secs = seconds;
    tc->tc_Request->tr_time.tv_micro = micros;

    SendIO((struct IORequest *)tc->tc_Request);
    tc->tc_Running = TRUE;
}

BOOL completeTimer(struct TimerContext *tc)
{
    if (!tc->tc_Running) return FALSE;

    /* The caller only gets here after Wait() reported the timer port's
       signal bit as set, so the request has already completed and this
       WaitIO() merely collects it without blocking. */
    WaitIO((struct IORequest *)tc->tc_Request);
    tc->tc_Running = FALSE;

    return TRUE;
}
