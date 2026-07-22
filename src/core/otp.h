/* otp.h — HOTP (RFC 4226) and TOTP (RFC 6238) code generation. */
#ifndef AMIAUTH_OTP_H
#define AMIAUTH_OTP_H

#include <stddef.h>
#include <stdint.h>

#define OTP_DEFAULT_DIGITS 6
#define OTP_DEFAULT_PERIOD 30

/* HMAC hash for code generation (RFC 6238 allows SHA-256/512 alongside SHA-1).
 * The values are also the vault's on-disk `algorithm` ids — keep them in step
 * with docs/VAULT_FORMAT.md. */
typedef enum {
    OTP_ALG_SHA1   = 0,
    OTP_ALG_SHA256 = 1,
    OTP_ALG_SHA512 = 2
} otp_alg;

/* Map an otpauth algorithm name (case-insensitive) to its otp_alg; -1 if it is
 * not one we implement. The reverse always yields the canonical upper-case
 * name (unknown values fall back to "SHA1"). */
int otp_alg_from_name(const char *name);
const char *otp_alg_name(otp_alg alg);

/* RFC 4226 §5.3 dynamic truncation: HMAC(alg, key, 8-byte BE counter), then
 * select/mask a 31-bit value. Shared by hotp() (reduced mod 10^digits below)
 * and other code-rendering schemes built on the same primitive but a
 * different final encoding (Steam Guard's base-26, src/core/steamguard.c). */
uint32_t otp_truncate(otp_alg alg, const uint8_t *key, size_t keylen,
                      uint64_t counter);

/* HOTP: HMAC over an 8-byte counter, dynamic truncation, modulo 10^digits.
 * Returns the code as an integer (zero-pad to `digits` when displaying). */
uint32_t hotp(otp_alg alg, const uint8_t *key, size_t keylen,
              uint64_t counter, int digits);

/* TOTP: HOTP with counter = (unix_time - t0) / period. */
uint32_t totp(otp_alg alg, const uint8_t *key, size_t keylen,
              uint64_t unix_time, uint64_t t0,
              uint32_t period, int digits);

/* SHA-1 shorthands (the overwhelmingly common variant). */
uint32_t hotp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t counter, int digits);
uint32_t totp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t unix_time, uint64_t t0,
                   uint32_t period, int digits);

/* Seconds remaining in the current TOTP step for the given time. */
uint32_t totp_seconds_remaining(uint64_t unix_time, uint64_t t0, uint32_t period);

/* Render an account's current code as a display string: dispatches on the
 * account's type and algorithm and zero-pads to its digit count. Uses
 * a->counter for HOTP (advancing/persisting it stays the caller's job) and
 * `unix_time` for TOTP. The one code path every front-end shares. */
#define OTP_CODE_BUF 16
struct otp_account;
void otp_render(const struct otp_account *a, uint64_t unix_time,
                char buf[OTP_CODE_BUF]);

#endif /* AMIAUTH_OTP_H */
