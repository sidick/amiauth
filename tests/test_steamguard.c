/* test_steamguard.c — Steam Guard TOTP variant (#44).
 *
 * There is no RFC for this scheme, so the vectors below were derived
 * independently via Python's stdlib hmac/hashlib (not this codebase's own
 * HMAC-SHA1), reimplementing the well-known truncation + base-26 encoding:
 *
 *   counter = unix_time // 30
 *   mac     = hmac.new(secret, struct.pack(">Q", counter), hashlib.sha1).digest()
 *   offset  = mac[19] & 0x0f
 *   bin     = (mac[offset]&0x7f)<<24 | mac[offset+1]<<16 | mac[offset+2]<<8 | mac[offset+3]
 *   code    = ''.join(alphabet[(bin // 26**i) % 26] for i in range(5))
 *
 * using the RFC 4226/6238 standard test secret "12345678901234567890" (20
 * bytes) so the underlying HMAC step is independently cross-checkable
 * against the RFC vectors already in test_otp.c. */
#include <string.h>

#include "test.h"
#include "steamguard.h"

void run_steamguard_tests(void)
{
    const uint8_t *secret = (const uint8_t *)"12345678901234567890";
    const size_t   slen   = 20;
    char code[STEAM_CODE_DIGITS + 1];

    steam_totp(secret, slen, 0, code);
    TEST_CHECK(strcmp(code, "GG5F5") == 0);

    steam_totp(secret, slen, 59, code);            /* counter = 59/30 = 1 */
    TEST_CHECK(strcmp(code, "PV9M4") == 0);

    steam_totp(secret, slen, 1111111109ULL, code);
    TEST_CHECK(strcmp(code, "PY4YB") == 0);

    steam_totp(secret, slen, 1111111111ULL, code);
    TEST_CHECK(strcmp(code, "5PP3V") == 0);

    steam_totp(secret, slen, 1234567890ULL, code);
    TEST_CHECK(strcmp(code, "VHHQY") == 0);

    steam_totp(secret, slen, 2000000000ULL, code);
    TEST_CHECK(strcmp(code, "9N776") == 0);

    /* Every character comes from the 26-symbol Steam alphabet (no vowels,
     * no 0/1/O/I), and the code is always exactly 5 characters. */
    {
        static const char alphabet[] = "23456789BCDFGHJKMNPQRTVWXY";
        int i;
        steam_totp(secret, slen, 42, code);
        TEST_CHECK(strlen(code) == STEAM_CODE_DIGITS);
        for (i = 0; i < STEAM_CODE_DIGITS; i++)
            TEST_CHECK(strchr(alphabet, code[i]) != NULL);
    }
}
