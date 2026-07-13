/* chacha20.h — ChaCha20 stream cipher (RFC 8439), the vault cipher.
 * Stub: see docs/ROADMAP.md Phase 2. */
#ifndef AMIAUTH_CHACHA20_H
#define AMIAUTH_CHACHA20_H

#include <stddef.h>
#include <stdint.h>

#define CHACHA20_KEY_SIZE   32
#define CHACHA20_NONCE_SIZE 12

/* XOR `len` bytes of `in` with the ChaCha20 keystream into `out`
 * (in-place permitted). `counter` is the initial 32-bit block counter. */
void chacha20_xor(const uint8_t key[CHACHA20_KEY_SIZE],
                  const uint8_t nonce[CHACHA20_NONCE_SIZE],
                  uint32_t counter,
                  const uint8_t *in, uint8_t *out, size_t len);

#endif /* AMIAUTH_CHACHA20_H */
