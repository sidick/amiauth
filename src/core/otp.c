/* otp.c — HOTP/TOTP. STUB: implement in Phase 1, validate against RFC 4226
 * Appendix D and RFC 6238 Appendix B (tests/test_otp.c). */
#include "otp.h"
#include "hmac.h"

uint32_t hotp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t counter, int digits)
{
    (void)key; (void)keylen; (void)counter; (void)digits;
    /* TODO: HMAC-SHA1 over the big-endian counter, dynamic truncation
     * (offset = digest[19] & 0x0f), modulo 10^digits. */
    return 0;
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
