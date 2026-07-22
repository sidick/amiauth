/* otp.c — HOTP (RFC 4226) / TOTP (RFC 6238), over SHA-1/SHA-256/SHA-512.
 * Validated against RFC 4226 App. D and RFC 6238 App. B (tests/test_otp.c). */
#include <string.h>

#include "otp.h"
#include "uri.h"
#include "hmac.h"
#include "steamguard.h"

int otp_alg_from_name(const char *name)
{
    static const char *names[] = { "SHA1", "SHA256", "SHA512" };
    size_t i;
    if (!name) return -1;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        const char *a = name, *b = names[i];
        for (; *a && *b; a++, b++) {
            int c = *a;
            if (c >= 'a' && c <= 'z') c -= 32;
            if (c != *b) break;
        }
        if (!*a && !*b) return (int)i;
    }
    return -1;
}

const char *otp_alg_name(otp_alg alg)
{
    switch (alg) {
        case OTP_ALG_SHA256: return "SHA256";
        case OTP_ALG_SHA512: return "SHA512";
        default:             return "SHA1";
    }
}

uint32_t otp_truncate(otp_alg alg, const uint8_t *key, size_t keylen,
                      uint64_t counter)
{
    uint8_t msg[8];
    uint8_t mac[SHA512_DIGEST_SIZE];   /* big enough for every variant */
    size_t maclen;
    int i, offset;

    /* 8-byte big-endian counter. */
    for (i = 7; i >= 0; i--) {
        msg[i] = (uint8_t)(counter & 0xff);
        counter >>= 8;
    }

    switch (alg) {
        case OTP_ALG_SHA256:
            hmac_sha256(key, keylen, msg, sizeof(msg), mac);
            maclen = SHA256_DIGEST_SIZE;
            break;
        case OTP_ALG_SHA512:
            hmac_sha512(key, keylen, msg, sizeof(msg), mac);
            maclen = SHA512_DIGEST_SIZE;
            break;
        default:
            hmac_sha1(key, keylen, msg, sizeof(msg), mac);
            maclen = SHA1_DIGEST_SIZE;
            break;
    }

    /* Dynamic truncation (RFC 4226 §5.3): low nibble of the last byte selects a
     * 4-byte window; mask the high bit to stay positive. */
    offset = mac[maclen - 1] & 0x0f;
    return ((uint32_t)(mac[offset] & 0x7f) << 24)
         | ((uint32_t)mac[offset + 1]      << 16)
         | ((uint32_t)mac[offset + 2]      <<  8)
         | ((uint32_t)mac[offset + 3]);
}

uint32_t hotp(otp_alg alg, const uint8_t *key, size_t keylen,
              uint64_t counter, int digits)
{
    uint32_t bin = otp_truncate(alg, key, keylen, counter);
    uint32_t mod = 1;
    int i;

    if (digits < 1 || digits > 9) digits = OTP_DEFAULT_DIGITS;
    for (i = 0; i < digits; i++) mod *= 10;
    return bin % mod;
}

uint32_t totp(otp_alg alg, const uint8_t *key, size_t keylen,
              uint64_t unix_time, uint64_t t0,
              uint32_t period, int digits)
{
    uint64_t counter;
    if (period == 0) period = OTP_DEFAULT_PERIOD;
    counter = (unix_time - t0) / period;
    return hotp(alg, key, keylen, counter, digits);
}

uint32_t hotp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t counter, int digits)
{
    return hotp(OTP_ALG_SHA1, key, keylen, counter, digits);
}

uint32_t totp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t unix_time, uint64_t t0,
                   uint32_t period, int digits)
{
    return totp(OTP_ALG_SHA1, key, keylen, unix_time, t0, period, digits);
}

uint32_t totp_seconds_remaining(uint64_t unix_time, uint64_t t0, uint32_t period)
{
    if (period == 0) period = OTP_DEFAULT_PERIOD;
    return (uint32_t)(period - ((unix_time - t0) % period));
}

void otp_render(const struct otp_account *a, uint64_t unix_time,
                char buf[OTP_CODE_BUF])
{
    int alg, digits;
    uint32_t code;
    int i;

    if (strcmp(a->type, "steam") == 0) {
        steam_totp(a->secret, a->secret_len, unix_time, buf);
        return;
    }

    alg = otp_alg_from_name(a->algorithm);
    digits = (a->digits >= 1 && a->digits <= 9) ? a->digits : OTP_DEFAULT_DIGITS;

    /* Both entry points (otpauth parse, vault load) reject algorithms we don't
     * implement, so this is only a defensive fallback. */
    if (alg < 0) alg = OTP_ALG_SHA1;

    if (strcmp(a->type, "hotp") == 0)
        code = hotp((otp_alg)alg, a->secret, a->secret_len, a->counter, digits);
    else
        code = totp((otp_alg)alg, a->secret, a->secret_len, unix_time, 0,
                    a->period, digits);

    /* Zero-padded by hand: libnix sprintf lacks '*' width. */
    buf[digits] = '\0';
    for (i = digits - 1; i >= 0; i--) {
        buf[i] = (char)('0' + code % 10);
        code /= 10;
    }
}
