/* test_otp.c — HOTP (RFC 4226 App. D) and TOTP (RFC 6238 App. B) vectors. */
#include "test.h"
#include "otp.h"

void run_otp_tests(void)
{
    /* RFC 4226 App. D, secret "12345678901234567890" (ASCII), 6 digits:
     *   counter 0 -> 755224, 1 -> 287082, 2 -> 359152, ... */
    TEST_PENDING("HOTP RFC 4226 counter 0 -> 755224 (implement otp.c/hmac.c)");
    TEST_PENDING("HOTP RFC 4226 counters 1..9");

    /* RFC 6238 App. B, SHA-1 secret, 8 digits, T0=0, step=30:
     *   T=59         -> 94287082
     *   T=1111111109 -> 07081804 */
    TEST_PENDING("TOTP RFC 6238 T=59 -> 94287082");
    TEST_PENDING("TOTP RFC 6238 T=1111111109 -> 07081804");

    /* totp_seconds_remaining is pure arithmetic and already implemented. */
    TEST_CHECK(totp_seconds_remaining(0, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(1, 0, 30) == 29);
    TEST_CHECK(totp_seconds_remaining(29, 0, 30) == 1);
    TEST_CHECK(totp_seconds_remaining(30, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(45, 0, 30) == 15);
    /* period==0 falls back to the 30s default. */
    TEST_CHECK(totp_seconds_remaining(5, 0, 0) == 25);
}
