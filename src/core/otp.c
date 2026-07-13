/* otp.c — HOTP (RFC 4226) / TOTP (RFC 6238).
 * Validated against RFC 4226 App. D and RFC 6238 App. B (tests/test_otp.c). */
#include "otp.h"
#include "hmac.h"

uint32_t hotp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t counter, int digits)
{
    uint8_t msg[8];
    uint8_t mac[SHA1_DIGEST_SIZE];
    uint32_t bin, mod;
    int i, offset;

    if (digits < 1 || digits > 9) digits = OTP_DEFAULT_DIGITS;

    /* 8-byte big-endian counter. */
    for (i = 7; i >= 0; i--) {
        msg[i] = (uint8_t)(counter & 0xff);
        counter >>= 8;
    }

    hmac_sha1(key, keylen, msg, sizeof(msg), mac);

    /* Dynamic truncation (RFC 4226 §5.3): low nibble of the last byte selects a
     * 4-byte window; mask the high bit to stay positive. */
    offset = mac[SHA1_DIGEST_SIZE - 1] & 0x0f;
    bin = ((uint32_t)(mac[offset] & 0x7f) << 24)
        | ((uint32_t)mac[offset + 1]      << 16)
        | ((uint32_t)mac[offset + 2]      <<  8)
        | ((uint32_t)mac[offset + 3]);

    mod = 1;
    for (i = 0; i < digits; i++) mod *= 10;
    return bin % mod;
}

uint32_t totp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t unix_time, uint64_t t0,
                   uint32_t period, int digits)
{
    uint64_t counter;
    if (period == 0) period = OTP_DEFAULT_PERIOD;
    counter = (unix_time - t0) / period;
    return hotp_sha1(key, keylen, counter, digits);
}

uint32_t totp_seconds_remaining(uint64_t unix_time, uint64_t t0, uint32_t period)
{
    if (period == 0) period = OTP_DEFAULT_PERIOD;
    return (uint32_t)(period - ((unix_time - t0) % period));
}
