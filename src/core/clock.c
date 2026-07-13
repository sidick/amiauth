/* clock.c — corrected-UTC resolution. The offset model is portable and real;
 * SNTP is platform glue implemented in the Amiga build (Phase 3). */
#include <time.h>

#include "clock.h"

void clock_init(clock_ctx *c)
{
    if (!c) return;
    c->state = CLOCK_UNVERIFIED;
    c->offset_seconds = 0;
}

uint64_t clock_now_utc(const clock_ctx *c)
{
    time_t now = time(NULL);
    long off = c ? c->offset_seconds : 0;
    return (uint64_t)((long)now + off);
}

void clock_set_offset(clock_ctx *c, long seconds)
{
    if (!c) return;
    c->offset_seconds = seconds;
    c->state = CLOCK_MANUAL;
}

int clock_sntp_sync(clock_ctx *c, const char *server)
{
    (void)c; (void)server;
    /* TODO (Amiga build): single UDP exchange over bsdsocket, compute offset,
     * set CLOCK_SYNCED. The host build has no SNTP and reports failure. */
    return -1;
}
