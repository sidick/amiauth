/* test_clock.c — offset model, manual nudge, and portable SNTP helpers. */
#include <string.h>

#include "test.h"
#include "clock.h"

/* Write a whole-second NTP timestamp (unix + delta) into 8 bytes. */
static void put_ntp(uint8_t *p, uint64_t unix_sec)
{
    uint32_t s = (uint32_t)(unix_sec + NTP_UNIX_DELTA);
    p[0] = (uint8_t)(s >> 24); p[1] = (uint8_t)(s >> 16);
    p[2] = (uint8_t)(s >>  8); p[3] = (uint8_t)(s);
    p[4] = p[5] = p[6] = p[7] = 0;
}

static uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

void run_clock_tests(void)
{
    clock_ctx c;

    /* --- offset model --- */
    clock_init(&c);
    TEST_CHECK(c.state == CLOCK_UNVERIFIED);
    TEST_CHECK(c.offset_seconds == 0);

    clock_set_offset(&c, 3600);
    TEST_CHECK(c.offset_seconds == 3600 && c.state == CLOCK_MANUAL);

    /* A +3600s offset advances corrected time by ~3600s vs an unadjusted
     * context read back-to-back (allow 1s for the clock ticking between calls). */
    {
        clock_ctx zero;
        long delta;
        clock_init(&zero);
        delta = (long)(clock_now_utc(&c) - clock_now_utc(&zero));
        TEST_CHECK(delta >= 3599 && delta <= 3601);
    }

    /* --- manual nudge --- */
    clock_init(&c);
    clock_nudge(&c, 5);
    TEST_CHECK(c.offset_seconds == 5 && c.state == CLOCK_MANUAL);
    clock_set_offset(&c, 100);
    clock_nudge(&c, -10);
    TEST_CHECK(c.offset_seconds == 90 && c.state == CLOCK_MANUAL);

    /* --- NTP request build --- */
    {
        uint8_t pkt[NTP_PACKET_SIZE];
        clock_ntp_build_request(pkt, 1000000000ULL);
        TEST_CHECK(pkt[0] == 0x1B);                     /* LI0 VN3 client */
        /* transmit timestamp seconds == unix + delta */
        TEST_CHECK(get_be32(pkt + 40) == (uint32_t)(1000000000ULL + NTP_UNIX_DELTA));
        /* unix epoch maps to the canonical 0x83AA7E80 */
        clock_ntp_build_request(pkt, 0);
        TEST_CHECK(get_be32(pkt + 40) == 0x83AA7E80UL);
    }

    /* --- NTP response parse --- */
    {
        uint8_t pkt[NTP_PACKET_SIZE];
        uint64_t o = 0, r = 0, t = 0;
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = 0x24;                                  /* LI0 VN4 Mode4 (server) */
        pkt[1] = 2;                                     /* stratum */
        put_ntp(pkt + 24, 1700000000ULL);              /* originate (T1) */
        put_ntp(pkt + 32, 1700000005ULL);              /* receive   (T2) */
        put_ntp(pkt + 40, 1700000006ULL);              /* transmit  (T3) */
        TEST_CHECK(clock_ntp_parse_response(pkt, &o, &r, &t) == 0);
        TEST_CHECK(o == 1700000000ULL && r == 1700000005ULL && t == 1700000006ULL);

        /* rejects a non-server mode */
        pkt[0] = 0x1B;                                  /* client mode */
        TEST_CHECK(clock_ntp_parse_response(pkt, &o, &r, &t) == -1);
        /* rejects stratum 0 (kiss o' death) */
        pkt[0] = 0x24; pkt[1] = 0;
        TEST_CHECK(clock_ntp_parse_response(pkt, &o, &r, &t) == -1);
    }

    /* --- request/echo round trip: server echoes our tx into originate --- */
    {
        uint8_t req[NTP_PACKET_SIZE], resp[NTP_PACKET_SIZE];
        uint64_t o = 0;
        clock_ntp_build_request(req, 1700000123ULL);
        memset(resp, 0, sizeof(resp));
        resp[0] = 0x24; resp[1] = 3;
        memcpy(resp + 24, req + 40, 8);                 /* echo tx -> originate */
        TEST_CHECK(clock_ntp_parse_response(resp, &o, NULL, NULL) == 0);
        TEST_CHECK(o == 1700000123ULL);
    }

    /* --- offset computation --- */
    /* t1=1000, t2=1010, t3=1012, t4=1004 -> ((10)+(8))/2 = 9 */
    clock_init(&c);
    clock_apply_offset(&c, 1000, 1010, 1012, 1004);
    TEST_CHECK(c.offset_seconds == 9 && c.state == CLOCK_SYNCED);
    /* symmetric round trip, server exactly N seconds ahead */
    clock_init(&c);
    clock_apply_offset(&c, 2000, 2100, 2100, 2000);     /* +100, 0 delay */
    TEST_CHECK(c.offset_seconds == 100);
    /* server behind: negative offset */
    clock_init(&c);
    clock_apply_offset(&c, 5000, 4950, 4950, 5000);
    TEST_CHECK(c.offset_seconds == -50);

    /* --- host build has no SNTP transport --- */
    clock_init(&c);
    TEST_CHECK(clock_sntp_sync(&c, "pool.ntp.org") == -1);
    TEST_CHECK(c.state == CLOCK_UNVERIFIED);
}
