#ifndef TIMER_H
#define TIMER_H

#include "compiler.h"

/*
 * Encapsulates every timer.device resource (message port and I/O request)
 * so the rest of the application never has to deal with Exec I/O requests
 * directly - it only calls the four functions below.
 */
struct TimerContext
{
    struct MsgPort      *tc_Port;
    struct timerequest  *tc_Request;
    BOOL                 tc_Running; /* TRUE while a request is in flight */
};

/* Opens timer.device (UNIT_MICROHZ) and allocates its port/request.
   Returns TRUE on success; on FALSE, no cleanup call is required. */
BOOL openTimer(struct TimerContext *tc);

/* Aborts any pending request and releases every resource opened above.
   Safe to call even if openTimer() partially failed. */
void closeTimer(struct TimerContext *tc);

/* Submits an asynchronous delay request of 'seconds' + 'micros'.
   Does not block: the request completes later, signalled through
   1L << tc->tc_Port->mp_SigBit. */
void startTimer(struct TimerContext *tc, ULONG seconds, ULONG micros);

/* To be called once the timer's signal bit has been observed as set
   (e.g. after Wait()). Collects the finished request. Returns FALSE if
   no request was pending. */
BOOL completeTimer(struct TimerContext *tc);

#endif /* TIMER_H */
