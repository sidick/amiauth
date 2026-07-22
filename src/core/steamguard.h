/* steamguard.h — Steam Guard's TOTP variant (#44).
 *
 * Not an RFC scheme: Steam's mobile authenticator uses the same RFC 6238
 * HMAC-SHA1/30s construction as ordinary TOTP, but renders the truncated
 * value as 5 characters from Steam's own 26-symbol alphabet instead of
 * decimal digits (no vowels, no 0/1/O/I — avoids mistaken-identity typos).
 * Reverse-engineered and used identically by every third-party Steam Guard
 * client; validated in tests/test_steamguard.c against vectors derived
 * independently via Python's hmac/hashlib, not this codebase's own HMAC. */
#ifndef AMIAUTH_STEAMGUARD_H
#define AMIAUTH_STEAMGUARD_H

#include <stddef.h>
#include <stdint.h>

#define STEAM_CODE_DIGITS 5
#define STEAM_PERIOD       30

/* Render the current Steam Guard code into out[STEAM_CODE_DIGITS + 1]
 * (NUL-terminated). Always HMAC-SHA1; `key`/`keylen` is the raw shared
 * secret (Base32-decoded already, as stored in otp_account). */
void steam_totp(const uint8_t *key, size_t keylen, uint64_t unix_time,
                char out[STEAM_CODE_DIGITS + 1]);

#endif /* AMIAUTH_STEAMGUARD_H */
