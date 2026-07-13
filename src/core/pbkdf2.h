/* pbkdf2.h — PBKDF2-HMAC-SHA1 (RFC 2898), the vault KDF.
 * Stub: see docs/ROADMAP.md Phase 2. */
#ifndef AMIAUTH_PBKDF2_H
#define AMIAUTH_PBKDF2_H

#include <stddef.h>
#include <stdint.h>

/* Derive `outlen` bytes into `out` from the passphrase and salt.
 * `iterations` is calibrated at first run (target ~1s on the host CPU). */
void pbkdf2_hmac_sha1(const uint8_t *pass, size_t passlen,
                      const uint8_t *salt, size_t saltlen,
                      uint32_t iterations,
                      uint8_t *out, size_t outlen);

#endif /* AMIAUTH_PBKDF2_H */
