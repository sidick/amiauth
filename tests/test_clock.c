/* test_clock.c — offset model (implemented) + SNTP stub behaviour. */
#include "test.h"
#include "clock.h"

void run_clock_tests(void)
{
    clock_ctx c;
    clock_init(&c);
    TEST_CHECK(c.state == CLOCK_UNVERIFIED);
    TEST_CHECK(c.offset_seconds == 0);

    clock_set_offset(&c, 3600);
    TEST_CHECK(c.offset_seconds == 3600);
    TEST_CHECK(c.state == CLOCK_MANUAL);

    /* A +3600s offset must advance corrected time by ~3600s vs an unadjusted
     * context read back-to-back (allow 1s for the clock ticking between calls). */
    clock_ctx zero;
    clock_init(&zero);
    uint64_t adjusted = clock_now_utc(&c);
    uint64_t plain    = clock_now_utc(&zero);
    long delta = (long)(adjusted - plain);
    TEST_CHECK(delta >= 3599 && delta <= 3601);

    /* Host build has no SNTP: sync must fail without changing state. */
    TEST_CHECK(clock_sntp_sync(&c, "pool.ntp.org") == -1);
    TEST_CHECK(c.state == CLOCK_MANUAL);
}
