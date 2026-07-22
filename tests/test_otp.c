/* test_otp.c — HOTP (RFC 4226 App. D) and TOTP (RFC 6238 App. B) vectors. */
#include <string.h>

#include "test.h"
#include "otp.h"
#include "uri.h"

void run_otp_tests(void)
{
    /* Both RFCs use the ASCII secret "12345678901234567890" (20 bytes); the
     * RFC 6238 SHA-256/512 vectors extend it to the hash's digest length. */
    const uint8_t *secret = (const uint8_t *)"12345678901234567890";
    const size_t   slen   = 20;
    const uint8_t *secret32 = (const uint8_t *)
        "12345678901234567890123456789012";
    const uint8_t *secret64 = (const uint8_t *)
        "1234567890123456789012345678901234567890123456789012345678901234";

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

    /* RFC 6238 App. B — TOTP (SHA-256), 8 digits, 32-byte seed. */
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 59ULL,          0, 30, 8) == 46119246u);
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 1111111109ULL,  0, 30, 8) == 68084774u);
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 1111111111ULL,  0, 30, 8) == 67062674u);
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 1234567890ULL,  0, 30, 8) == 91819424u);
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 2000000000ULL,  0, 30, 8) == 90698825u);
    TEST_CHECK(totp(OTP_ALG_SHA256, secret32, 32, 20000000000ULL, 0, 30, 8) == 77737706u);

    /* RFC 6238 App. B — TOTP (SHA-512), 8 digits, 64-byte seed. */
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 59ULL,          0, 30, 8) == 90693936u);
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 1111111109ULL,  0, 30, 8) == 25091201u);
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 1111111111ULL,  0, 30, 8) == 99943326u);
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 1234567890ULL,  0, 30, 8) == 93441116u);
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 2000000000ULL,  0, 30, 8) == 38618901u);
    TEST_CHECK(totp(OTP_ALG_SHA512, secret64, 64, 20000000000ULL, 0, 30, 8) == 47863826u);

    /* The generic entry point with OTP_ALG_SHA1 matches the sha1 shorthand. */
    TEST_CHECK(totp(OTP_ALG_SHA1, secret, slen, 59ULL, 0, 30, 8) == 94287082u);
    TEST_CHECK(hotp(OTP_ALG_SHA1, secret, slen, 0, 6) == 755224u);

    /* Algorithm-name mapping (case-insensitive; unknown rejected). */
    TEST_CHECK(otp_alg_from_name("SHA1") == OTP_ALG_SHA1);
    TEST_CHECK(otp_alg_from_name("sha256") == OTP_ALG_SHA256);
    TEST_CHECK(otp_alg_from_name("Sha512") == OTP_ALG_SHA512);
    TEST_CHECK(otp_alg_from_name("SHA224") == -1);
    TEST_CHECK(otp_alg_from_name("SHA25") == -1);
    TEST_CHECK(otp_alg_from_name("") == -1);
    TEST_CHECK(otp_alg_from_name(NULL) == -1);
    TEST_CHECK(strcmp(otp_alg_name(OTP_ALG_SHA256), "SHA256") == 0);

    /* totp_seconds_remaining is pure arithmetic and already implemented. */
    TEST_CHECK(totp_seconds_remaining(0, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(1, 0, 30) == 29);
    TEST_CHECK(totp_seconds_remaining(29, 0, 30) == 1);
    TEST_CHECK(totp_seconds_remaining(30, 0, 30) == 30);
    TEST_CHECK(totp_seconds_remaining(45, 0, 30) == 15);
    /* period==0 falls back to the 30s default. */
    TEST_CHECK(totp_seconds_remaining(5, 0, 0) == 25);

    /* otp_render: the account-level dispatch + zero-padded formatting every
     * front-end shares. Reuses the RFC vectors above. */
    {
        otp_account a;
        char buf[OTP_CODE_BUF];

        memset(&a, 0, sizeof(a));
        strcpy(a.type, "totp");
        strcpy(a.algorithm, "SHA1");
        memcpy(a.secret, secret, slen);
        a.secret_len = slen;
        a.digits = 8;
        a.period = 30;
        otp_render(&a, 1111111109ULL, buf);
        TEST_CHECK(strcmp(buf, "07081804") == 0);   /* keeps the leading zero */

        strcpy(a.algorithm, "SHA256");
        memcpy(a.secret, secret32, 32);
        a.secret_len = 32;
        otp_render(&a, 59ULL, buf);
        TEST_CHECK(strcmp(buf, "46119246") == 0);

        strcpy(a.algorithm, "SHA512");
        memcpy(a.secret, secret64, 64);
        a.secret_len = 64;
        otp_render(&a, 59ULL, buf);
        TEST_CHECK(strcmp(buf, "90693936") == 0);

        /* HOTP renders from the stored counter; the time argument is unused. */
        memset(&a, 0, sizeof(a));
        strcpy(a.type, "hotp");
        strcpy(a.algorithm, "SHA1");
        memcpy(a.secret, secret, slen);
        a.secret_len = slen;
        a.digits = 6;
        a.counter = 5;
        otp_render(&a, 999999ULL, buf);
        TEST_CHECK(strcmp(buf, "254676") == 0);

        /* Steam Guard dispatch (#44) — algorithm/digits stored on the account
         * are irrelevant; otp_render always renders the 5-char base-26 code.
         * Same vector as tests/test_steamguard.c's independent derivation. */
        memset(&a, 0, sizeof(a));
        strcpy(a.type, "steam");
        strcpy(a.algorithm, "SHA1");
        memcpy(a.secret, secret, slen);
        a.secret_len = slen;
        a.digits = 5;
        a.period = 30;
        otp_render(&a, 1234567890ULL, buf);
        TEST_CHECK(strcmp(buf, "VHHQY") == 0);
    }
}
