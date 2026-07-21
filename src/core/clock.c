/* clock.c — corrected-UTC resolution. The offset model and SNTP packet/offset
 * logic are portable and host-tested; only the bsdsocket UDP transport is Amiga
 * glue (clock_sntp_sync). */
#include <string.h>
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

void clock_nudge(clock_ctx *c, long delta_seconds)
{
    if (!c) return;
    c->offset_seconds += delta_seconds;
    c->state = CLOCK_MANUAL;
}

/* --- SNTP packet helpers --- */

static uint32_t rd_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

static void wr_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)(v);
}

/* Whole-second NTP timestamp (bytes [0,4)) -> Unix seconds. The 32-bit fraction
 * is ignored (sub-second precision is irrelevant to TOTP). Timestamps before
 * the Unix epoch (or a zero timestamp) map to 0. Note the NTP era-0 seconds
 * field wraps in 2036; acceptable for this tool's lifetime. */
static uint64_t ntp_ts_to_unix(const uint8_t *p)
{
    uint32_t sec = rd_be32(p);
    if (sec < NTP_UNIX_DELTA) return 0;
    return (uint64_t)(sec - NTP_UNIX_DELTA);
}

void clock_ntp_build_request(uint8_t pkt[NTP_PACKET_SIZE], uint64_t client_tx_unix)
{
    memset(pkt, 0, NTP_PACKET_SIZE);
    pkt[0] = 0x1B;   /* LI = 0, VN = 3, Mode = 3 (client) */
    /* transmit timestamp (bytes [40,48)): seconds only, fraction left zero */
    wr_be32(pkt + 40, (uint32_t)(client_tx_unix + NTP_UNIX_DELTA));
}

int clock_ntp_parse_response(const uint8_t pkt[NTP_PACKET_SIZE],
                             uint64_t *originate, uint64_t *receive,
                             uint64_t *transmit)
{
    uint8_t mode    = pkt[0] & 0x07;
    uint8_t stratum = pkt[1];

    if (mode != 4) return -1;                 /* must be a server response */
    if (stratum == 0 || stratum > 15) return -1;  /* kiss-o'-death / invalid */

    if (originate) *originate = ntp_ts_to_unix(pkt + 24);
    if (receive)   *receive   = ntp_ts_to_unix(pkt + 32);
    if (transmit)  *transmit  = ntp_ts_to_unix(pkt + 40);
    return 0;
}

void clock_apply_offset(clock_ctx *c, uint64_t t1, uint64_t t2,
                        uint64_t t3, uint64_t t4)
{
    long long off;
    if (!c) return;
    off = (((long long)t2 - (long long)t1) + ((long long)t3 - (long long)t4)) / 2;
    c->offset_seconds = (long)off;
    c->state = CLOCK_SYNCED;
}

#ifndef __amigaos__
/* Portable/host build has no UDP transport. The AmigaOS implementation lives in
 * src/amiga/sntp.c (bsdsocket) and is linked only into the Amiga build. */
int clock_sntp_sync(clock_ctx *c, const char *server)
{
    (void)c; (void)server;
    return -1;
}
#endif
