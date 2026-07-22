/* steamguard.c — Steam Guard's TOTP variant (#44). See steamguard.h. */
#include "steamguard.h"
#include "otp.h"

static const char STEAM_ALPHABET[] = "23456789BCDFGHJKMNPQRTVWXY";

void steam_totp(const uint8_t *key, size_t keylen, uint64_t unix_time,
                char out[STEAM_CODE_DIGITS + 1])
{
    uint64_t counter = unix_time / STEAM_PERIOD;
    uint32_t v = otp_truncate(OTP_ALG_SHA1, key, keylen, counter);
    int i;

    for (i = 0; i < STEAM_CODE_DIGITS; i++) {
        out[i] = STEAM_ALPHABET[v % 26];
        v /= 26;
    }
    out[STEAM_CODE_DIGITS] = '\0';
}
