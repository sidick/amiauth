/* chacha20.c — ChaCha20 (RFC 8439). STUB: implement in Phase 2, validate
 * against the RFC 8439 test vectors (tests/test_chacha20.c). */
#include "chacha20.h"

void chacha20_xor(const uint8_t key[CHACHA20_KEY_SIZE],
                  const uint8_t nonce[CHACHA20_NONCE_SIZE],
                  uint32_t counter,
                  const uint8_t *in, uint8_t *out, size_t len)
{
    (void)key; (void)nonce; (void)counter; (void)in; (void)out; (void)len;
    /* TODO: 16-word state (constants|key|counter|nonce), 20 rounds of the
     * quarter-round (ARX), serialise keystream little-endian, XOR with input. */
}
