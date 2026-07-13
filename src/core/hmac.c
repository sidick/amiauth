/* hmac.c — HMAC-SHA1 (RFC 2104).
 * Validated against RFC 2202 vectors in tests/test_hmac.c. */
#include <string.h>

#include "hmac.h"

void hmac_sha1(const uint8_t *key, size_t keylen,
               const uint8_t *msg, size_t msglen,
               uint8_t out[SHA1_DIGEST_SIZE])
{
    uint8_t k[SHA1_BLOCK_SIZE];
    uint8_t pad[SHA1_BLOCK_SIZE];
    uint8_t inner[SHA1_DIGEST_SIZE];
    sha1_ctx c;
    size_t i;

    if (!out) return;

    /* K' = key, zero-padded to the block size; keys longer than a block are
     * first hashed down. */
    memset(k, 0, sizeof(k));
    if (keylen > SHA1_BLOCK_SIZE) {
        sha1(key, keylen, k);
    } else if (keylen) {
        memcpy(k, key, keylen);
    }

    /* Inner: SHA1((K' ^ ipad) || msg). */
    for (i = 0; i < SHA1_BLOCK_SIZE; i++) pad[i] = k[i] ^ 0x36;
    sha1_init(&c);
    sha1_update(&c, pad, SHA1_BLOCK_SIZE);
    sha1_update(&c, msg, msglen);
    sha1_final(&c, inner);

    /* Outer: SHA1((K' ^ opad) || inner). */
    for (i = 0; i < SHA1_BLOCK_SIZE; i++) pad[i] = k[i] ^ 0x5c;
    sha1_init(&c);
    sha1_update(&c, pad, SHA1_BLOCK_SIZE);
    sha1_update(&c, inner, SHA1_DIGEST_SIZE);
    sha1_final(&c, out);
}
