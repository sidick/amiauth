/* hmac.c — HMAC (RFC 2104) over SHA-1/SHA-256/SHA-512, one-shot and streaming.
 * Validated against RFC 2202 / RFC 4231 vectors in tests/test_hmac.c.
 * The SHA-256/512 variants are line-for-line parallels of the SHA-1 one
 * (see the note in hmac.h); keep the three in step when changing any. */
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

/* --- HMAC-SHA256 ----------------------------------------------------------- */

void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t keylen)
{
    uint8_t k[SHA256_BLOCK_SIZE];
    uint8_t ipad[SHA256_BLOCK_SIZE];
    size_t i;

    if (!ctx) return;

    memset(k, 0, sizeof(k));
    if (keylen > SHA256_BLOCK_SIZE) {
        sha256(key, keylen, k);
    } else if (keylen) {
        memcpy(k, key, keylen);
    }

    for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
        ipad[i]      = k[i] ^ 0x36;
        ctx->opad[i] = k[i] ^ 0x5c;
    }

    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, ipad, SHA256_BLOCK_SIZE);
}

void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len)
{
    if (!ctx) return;
    sha256_update(&ctx->inner, data, len);
}

void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE])
{
    uint8_t inner[SHA256_DIGEST_SIZE];
    sha256_ctx outer;

    if (!ctx || !out) return;

    sha256_final(&ctx->inner, inner);
    sha256_init(&outer);
    sha256_update(&outer, ctx->opad, SHA256_BLOCK_SIZE);
    sha256_update(&outer, inner, SHA256_DIGEST_SIZE);
    sha256_final(&outer, out);
}

void hmac_sha256(const uint8_t *key, size_t keylen,
                 const uint8_t *msg, size_t msglen,
                 uint8_t out[SHA256_DIGEST_SIZE])
{
    hmac_sha256_ctx ctx;
    hmac_sha256_init(&ctx, key, keylen);
    hmac_sha256_update(&ctx, msg, msglen);
    hmac_sha256_final(&ctx, out);
}

/* --- HMAC-SHA512 ----------------------------------------------------------- */

void hmac_sha512_init(hmac_sha512_ctx *ctx, const uint8_t *key, size_t keylen)
{
    uint8_t k[SHA512_BLOCK_SIZE];
    uint8_t ipad[SHA512_BLOCK_SIZE];
    size_t i;

    if (!ctx) return;

    memset(k, 0, sizeof(k));
    if (keylen > SHA512_BLOCK_SIZE) {
        sha512(key, keylen, k);
    } else if (keylen) {
        memcpy(k, key, keylen);
    }

    for (i = 0; i < SHA512_BLOCK_SIZE; i++) {
        ipad[i]      = k[i] ^ 0x36;
        ctx->opad[i] = k[i] ^ 0x5c;
    }

    sha512_init(&ctx->inner);
    sha512_update(&ctx->inner, ipad, SHA512_BLOCK_SIZE);
}

void hmac_sha512_update(hmac_sha512_ctx *ctx, const void *data, size_t len)
{
    if (!ctx) return;
    sha512_update(&ctx->inner, data, len);
}

void hmac_sha512_final(hmac_sha512_ctx *ctx, uint8_t out[SHA512_DIGEST_SIZE])
{
    uint8_t inner[SHA512_DIGEST_SIZE];
    sha512_ctx outer;

    if (!ctx || !out) return;

    sha512_final(&ctx->inner, inner);
    sha512_init(&outer);
    sha512_update(&outer, ctx->opad, SHA512_BLOCK_SIZE);
    sha512_update(&outer, inner, SHA512_DIGEST_SIZE);
    sha512_final(&outer, out);
}

void hmac_sha512(const uint8_t *key, size_t keylen,
                 const uint8_t *msg, size_t msglen,
                 uint8_t out[SHA512_DIGEST_SIZE])
{
    hmac_sha512_ctx ctx;
    hmac_sha512_init(&ctx, key, keylen);
    hmac_sha512_update(&ctx, msg, msglen);
    hmac_sha512_final(&ctx, out);
}
