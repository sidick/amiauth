/* test_otp.c — HOTP (RFC 4226 App. D) and TOTP (RFC 6238 App. B) vectors. */
#include "test.h"
#include "otp.h"

void run_otp_tests(void)
{
    /* Both RFCs use the ASCII secret "12345678901234567890" (20 bytes). */
    const uint8_t *secret = (const uint8_t *)"12345678901234567890";
    const size_t   slen   = 20;

    /* RFC 4226 App. D — HOTP, 6 digits, counters 0..9. */
    static const uint32_t hotp_expected[10] = {
        755224, 287082, 359152, 969429, 338314,
        254676, 287922, 162583, 399871, 520489
    };
    int i;
    for (i = 0; i < 10; i++)
        TEST_CHECK(hotp_sha1(secret, slen, (uint64_t)i, 6) == hotp_expected[i]);

    /* RFC 6238 App. B — TOTP (SHA-1), 8 digits, T0=0, step=30. Note the
     * T=1111111109 vector is 07081804: a leading zero, i.e. value 7081804. */
    TEST_CHECK(totp_sha1(secret, slen, 59ULL,          0, 30, 8) == 94287082u);
    TEST_CHECK(totp_sha1(secret, slen, 1111111109ULL,  0, 30, 8) ==  7081804u);
    TEST_CHECK(totp_sha1(secret, slen, 1111111111ULL,  0, 30, 8) == 14050471u);
    TEST_CHECK(totp_sha1(secret, slen, 1234567890ULL,  0, 30, 8) == 89005924u);
    TEST_CHECK(totp_sha1(secret, slen, 2000000000ULL,  0, 30, 8) == 69279037u);
    TEST_CHECK(totp_sha1(secret, slen, 20000000000ULL, 0, 30, 8) == 65353130u);

    /* totp_seconds_remaining is pure arithmetic and already implemented. */
    TEST_CHECK(totp_seconds_remaining(0, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(1, 0, 30) == 29);
    TEST_CHECK(totp_seconds_remaining(29, 0, 30) == 1);
    TEST_CHECK(totp_seconds_remaining(30, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(45, 0, 30) == 15);
    /* period==0 falls back to the 30s default. */
    TEST_CHECK(totp_seconds_remaining(5, 0, 0) == 25);
}
