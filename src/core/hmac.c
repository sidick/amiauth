/* hmac.c — HMAC-SHA1 (RFC 2104), one-shot and streaming.
 * Validated against RFC 2202 vectors in tests/test_hmac.c. */
#include <string.h>

#include "hmac.h"

void hmac_sha1_init(hmac_sha1_ctx *ctx, const uint8_t *key, size_t keylen)
{
    uint8_t k[SHA1_BLOCK_SIZE];
    uint8_t ipad[SHA1_BLOCK_SIZE];
    size_t i;

    if (!ctx) return;

    /* K' = key, zero-padded to the block size; keys longer than a block are
     * first hashed down. */
    memset(k, 0, sizeof(k));
    if (keylen > SHA1_BLOCK_SIZE) {
        sha1(key, keylen, k);
    } else if (keylen) {
        memcpy(k, key, keylen);
    }

    for (i = 0; i < SHA1_BLOCK_SIZE; i++) {
        ipad[i]      = k[i] ^ 0x36;
        ctx->opad[i] = k[i] ^ 0x5c;
    }

    sha1_init(&ctx->inner);
    sha1_update(&ctx->inner, ipad, SHA1_BLOCK_SIZE);
}

void hmac_sha1_update(hmac_sha1_ctx *ctx, const void *data, size_t len)
{
    if (!ctx) return;
    sha1_update(&ctx->inner, data, len);
}

void hmac_sha1_final(hmac_sha1_ctx *ctx, uint8_t out[SHA1_DIGEST_SIZE])
{
    uint8_t inner[SHA1_DIGEST_SIZE];
    sha1_ctx outer;

    if (!ctx || !out) return;

    sha1_final(&ctx->inner, inner);          /* SHA1((K'^ipad) || msg) */
    sha1_init(&outer);
    sha1_update(&outer, ctx->opad, SHA1_BLOCK_SIZE);
    sha1_update(&outer, inner, SHA1_DIGEST_SIZE);
    sha1_final(&outer, out);                 /* SHA1((K'^opad) || inner) */
}

void hmac_sha1(const uint8_t *key, size_t keylen,
               const uint8_t *msg, size_t msglen,
               uint8_t out[SHA1_DIGEST_SIZE])
{
    hmac_sha1_ctx ctx;
    hmac_sha1_init(&ctx, key, keylen);
    hmac_sha1_update(&ctx, msg, msglen);
    hmac_sha1_final(&ctx, out);
}
