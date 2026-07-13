/* test_pbkdf2.c — PBKDF2-HMAC-SHA1 vectors (RFC 6070). */
#include "test.h"
#include "pbkdf2.h"

void run_pbkdf2_tests(void)
{
    uint8_t dk[32];

    /* c = 1, dkLen = 20 */
    pbkdf2_hmac_sha1((const uint8_t *)"password", 8,
                     (const uint8_t *)"salt", 4, 1, dk, 20);
    TEST_CHECK(hex_eq(dk, 20, "0c60c80f961f0e71f3a9b524af6012062fe037a6"));

    /* c = 2, dkLen = 20 */
    pbkdf2_hmac_sha1((const uint8_t *)"password", 8,
                     (const uint8_t *)"salt", 4, 2, dk, 20);
    TEST_CHECK(hex_eq(dk, 20, "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957"));

    /* c = 4096, dkLen = 20 (the calibration-scale case) */
    pbkdf2_hmac_sha1((const uint8_t *)"password", 8,
                     (const uint8_t *)"salt", 4, 4096, dk, 20);
    TEST_CHECK(hex_eq(dk, 20, "4b007901b765489abead49d926f721d065a429c1"));

    /* Longer inputs, dkLen = 25 — spans two SHA-1 output blocks. */
    pbkdf2_hmac_sha1((const uint8_t *)"passwordPASSWORDpassword", 24,
                     (const uint8_t *)"saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
                     4096, dk, 25);
    TEST_CHECK(hex_eq(dk, 25,
        "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038"));

    /* Embedded NUL bytes in password and salt (dkLen = 16). */
    pbkdf2_hmac_sha1((const uint8_t *)"pass\0word", 9,
                     (const uint8_t *)"sa\0lt", 5, 4096, dk, 16);
    TEST_CHECK(hex_eq(dk, 16, "56fa6aa75548099dcc37d7f03425e0c3"));
}
