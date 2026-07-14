/* drbg.h — HMAC-DRBG over SHA-1 (NIST SP 800-90A).
 *
 * A deterministic random bit generator: seed it with entropy, then pull as many
 * pseudo-random bytes as needed. It reuses the SHA-1/HMAC already in the binary
 * (no new primitive) and is fully deterministic given its seed, so it is
 * host-testable with a known-answer vector.
 *
 * This is the whitening/expansion stage only. The *entropy* that seeds it is a
 * platform concern — see src/amiga/random.c for the AmigaOS source. */
#ifndef AMIAUTH_DRBG_H
#define AMIAUTH_DRBG_H

#include <stddef.h>
#include <stdint.h>

#include "sha1.h"                 /* SHA1_DIGEST_SIZE */

typedef struct {
    uint8_t K[SHA1_DIGEST_SIZE];
    uint8_t V[SHA1_DIGEST_SIZE];
} drbg_state;

/* Instantiate from seed material (entropy || nonce || personalisation, in any
 * combination the caller has gathered). */
void drbg_init(drbg_state *st, const uint8_t *seed, size_t seedlen);

/* Fold additional entropy into the running state (SP 800-90A reseed). */
void drbg_reseed(drbg_state *st, const uint8_t *in, size_t inlen);

/* Emit n pseudo-random bytes and advance the state. */
void drbg_generate(drbg_state *st, uint8_t *out, size_t n);

#endif /* AMIAUTH_DRBG_H */
