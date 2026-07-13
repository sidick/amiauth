/* otp.h — HOTP (RFC 4226) and TOTP (RFC 6238) code generation.
 * Stub: see docs/ROADMAP.md Phase 1. */
#ifndef AMIAUTH_OTP_H
#define AMIAUTH_OTP_H

#include <stddef.h>
#include <stdint.h>

#define OTP_DEFAULT_DIGITS 6
#define OTP_DEFAULT_PERIOD 30

/* HOTP: HMAC-SHA1 over an 8-byte counter, dynamic truncation, modulo 10^digits.
 * Returns the code as an integer (zero-pad to `digits` when displaying). */
uint32_t hotp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t counter, int digits);

/* TOTP: HOTP with counter = (unix_time - t0) / period. */
uint32_t totp_sha1(const uint8_t *key, size_t keylen,
                   uint64_t unix_time, uint64_t t0,
                   uint32_t period, int digits);

/* Seconds remaining in the current TOTP step for the given time. */
uint32_t totp_seconds_remaining(uint64_t unix_time, uint64_t t0, uint32_t period);

#endif /* AMIAUTH_OTP_H */
