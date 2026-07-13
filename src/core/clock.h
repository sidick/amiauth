/* clock.h — corrected-UTC resolution for TOTP without touching the system clock.
 * Offset model is portable; SNTP is platform glue (bsdsocket).
 * See docs/ROADMAP.md Phase 3. */
#ifndef AMIAUTH_CLOCK_H
#define AMIAUTH_CLOCK_H

#include <stdint.h>

typedef enum {
    CLOCK_UNVERIFIED = 0,   /* red:   no trusted time source */
    CLOCK_MANUAL     = 1,   /* amber: user-supplied offset */
    CLOCK_SYNCED     = 2    /* green: SNTP-synced */
} clock_state;

typedef struct {
    clock_state state;
    long        offset_seconds;   /* added to the system clock to obtain UTC */
} clock_ctx;

/* Initialise to CLOCK_UNVERIFIED with a zero offset. */
void clock_init(clock_ctx *c);

/* Corrected unix time = system time + offset. */
uint64_t clock_now_utc(const clock_ctx *c);

/* Set a manual offset (moves state to CLOCK_MANUAL). */
void clock_set_offset(clock_ctx *c, long seconds);

/* Query an SNTP server (single UDP exchange), storing the measured offset and
 * moving to CLOCK_SYNCED on success. Returns 0 on success, -1 otherwise.
 * Platform glue — the host build stubs this out. */
int clock_sntp_sync(clock_ctx *c, const char *server);

#endif /* AMIAUTH_CLOCK_H */
