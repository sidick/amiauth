/* sha1.c — SHA-1 (FIPS 180-1 / RFC 3174).
 * Validated against tests/test_sha1.c. */
#include <string.h>

#include "sha1.h"

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void sha1_compress(uint32_t state[5], const uint8_t block[SHA1_BLOCK_SIZE])
{
    uint32_t w[80];
    uint32_t a, b, c, d, e;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4]     << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] <<  8)
             | ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 80; i++)
        w[i] = ROTL32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

    for (i = 0; i < 80; i++) {
        uint32_t f, k, tmp;
        if (i < 20)      { f = (b & c) | (~b & d);          k = 0x5A827999; }
        else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
        else             { f = b ^ c ^ d;                   k = 0xCA62C1D6; }

        tmp = ROTL32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = ROTL32(b, 30); b = a; a = tmp;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

void sha1_init(sha1_ctx *ctx)
{
    if (!ctx) return;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
    ctx->buflen = 0;
}

void sha1_update(sha1_ctx *ctx, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    if (!ctx || !p) return;

    ctx->count += len;

    /* Top up a partial block first. */
    if (ctx->buflen) {
        size_t need = SHA1_BLOCK_SIZE - ctx->buflen;
        size_t take = len < need ? len : need;
        memcpy(ctx->buf + ctx->buflen, p, take);
        ctx->buflen += take;
        p += take;
        len -= take;
        if (ctx->buflen == SHA1_BLOCK_SIZE) {
            sha1_compress(ctx->state, ctx->buf);
            ctx->buflen = 0;
        }
    }

    /* Whole blocks straight from the input. */
    while (len >= SHA1_BLOCK_SIZE) {
        sha1_compress(ctx->state, p);
        p += SHA1_BLOCK_SIZE;
        len -= SHA1_BLOCK_SIZE;
    }

    /* Stash the remainder. */
    if (len) {
        memcpy(ctx->buf, p, len);
        ctx->buflen = len;
    }
}

void sha1_final(sha1_ctx *ctx, uint8_t out[SHA1_DIGEST_SIZE])
{
    uint64_t bits;
    int i;

    if (!ctx || !out) return;
    bits = ctx->count * 8;

    /* Append 0x80, then zero-pad. If the length field won't fit in this block,
     * finish the block and pad into a fresh one. */
    ctx->buf[ctx->buflen++] = 0x80;
    if (ctx->buflen > 56) {
        while (ctx->buflen < SHA1_BLOCK_SIZE) ctx->buf[ctx->buflen++] = 0;
        sha1_compress(ctx->state, ctx->buf);
        ctx->buflen = 0;
    }
    while (ctx->buflen < 56) ctx->buf[ctx->buflen++] = 0;

    /* 64-bit big-endian bit length. */
    for (i = 0; i < 8; i++)
        ctx->buf[56 + i] = (uint8_t)(bits >> (56 - 8 * i));
    sha1_compress(ctx->state, ctx->buf);

    for (i = 0; i < 5; i++) {
        out[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(ctx->state[i] >>  8);
        out[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE])
{
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, out);
}
