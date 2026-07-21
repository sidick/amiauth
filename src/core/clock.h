/* clock.h — corrected-UTC resolution for TOTP without touching the system clock.
 *
 * The offset model, SNTP packet build/parse and offset computation are portable
 * and host-tested. Only the bsdsocket UDP transport is Amiga glue (see
 * clock_sntp_sync). */
#ifndef AMIAUTH_CLOCK_H
#define AMIAUTH_CLOCK_H

#include <stddef.h>
#include <stdint.h>

#define NTP_PACKET_SIZE 48

/* Seconds between the NTP epoch (1900-01-01) and the Unix epoch (1970-01-01). */
#define NTP_UNIX_DELTA 2208988800UL

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

/* Set an absolute manual offset (-> CLOCK_MANUAL). */
void clock_set_offset(clock_ctx *c, long seconds);

/* Adjust the offset by a relative delta (-> CLOCK_MANUAL). The countdown bar
 * makes it easy to sync by eye against a known-good code. */
void clock_nudge(clock_ctx *c, long delta_seconds);

/* --- portable SNTP helpers (host-tested) --- */

/* Build a 48-byte SNTP client request. `client_tx_unix` is written into the
 * transmit timestamp so the server echoes it back as the originate timestamp. */
void clock_ntp_build_request(uint8_t pkt[NTP_PACKET_SIZE], uint64_t client_tx_unix);

/* Parse a 48-byte SNTP server response. Fills the originate (T1, echoed),
 * receive (T2) and transmit (T3) timestamps as Unix seconds (NULL to skip).
 * Returns 0 on success, -1 if it is not a valid server response (wrong mode,
 * or a stratum-0 "kiss o' death" / out-of-range stratum). */
int clock_ntp_parse_response(const uint8_t pkt[NTP_PACKET_SIZE],
                             uint64_t *originate, uint64_t *receive,
                             uint64_t *transmit);

/* Compute and apply the clock offset from the four SNTP timestamps
 * (offset = ((T2-T1) + (T3-T4)) / 2), moving to CLOCK_SYNCED. T1 is the client
 * transmit time and T4 the client receive time (both local Unix seconds); T2/T3
 * come from the server response. */
void clock_apply_offset(clock_ctx *c, uint64_t t1, uint64_t t2,
                        uint64_t t3, uint64_t t4);

/* --- Amiga glue --- */

/* Perform a full SNTP exchange against `server` over bsdsocket and apply the
 * offset. Portable build has no transport and returns -1; the Amiga front-end
 * implements the UDP round trip using the helpers above. */
int clock_sntp_sync(clock_ctx *c, const char *server);

#endif /* AMIAUTH_CLOCK_H */
