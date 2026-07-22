/* sha256.c — SHA-256 (FIPS 180-4).
 * Validated against tests/test_sha256.c and the OpenSSL differential harness.
 * Portable C only: unlike SHA-1 there is no asm/dispatch seam — OTP does one
 * short HMAC per code, so this is never a hot loop (PBKDF2/vault stay SHA-1). */
#include <string.h>

#include "sha256.h"

#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_compress(uint32_t state[8], const uint8_t block[SHA256_BLOCK_SIZE])
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4]     << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] <<  8)
             | ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROTR32(w[i - 15], 7) ^ ROTR32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROTR32(w[i - 2], 17) ^ ROTR32(w[i - 2],  19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (i = 0; i < 64; i++) {
        uint32_t S1 = ROTR32(e, 6) ^ ROTR32(e, 11) ^ ROTR32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = ROTR32(a, 2) ^ ROTR32(a, 13) ^ ROTR32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_init(sha256_ctx *ctx)
{
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
    ctx->buflen = 0;
}

void sha256_update(sha256_ctx *ctx, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    if (!ctx || !p) return;

    ctx->count += len;

    /* Top up a partial block first. */
    if (ctx->buflen) {
        size_t need = SHA256_BLOCK_SIZE - ctx->buflen;
        size_t take = len < need ? len : need;
        memcpy(ctx->buf + ctx->buflen, p, take);
        ctx->buflen += take;
        p += take;
        len -= take;
        if (ctx->buflen == SHA256_BLOCK_SIZE) {
            sha256_compress(ctx->state, ctx->buf);
            ctx->buflen = 0;
        }
    }

    /* Whole blocks straight from the input. */
    while (len >= SHA256_BLOCK_SIZE) {
        sha256_compress(ctx->state, p);
        p += SHA256_BLOCK_SIZE;
        len -= SHA256_BLOCK_SIZE;
    }

    /* Stash the remainder. */
    if (len) {
        memcpy(ctx->buf, p, len);
        ctx->buflen = len;
    }
}

void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE])
{
    uint64_t bits;
    int i;

    if (!ctx || !out) return;
    bits = ctx->count * 8;

    /* Append 0x80, then zero-pad. If the length field won't fit in this block,
     * finish the block and pad into a fresh one. */
    ctx->buf[ctx->buflen++] = 0x80;
    if (ctx->buflen > 56) {
        while (ctx->buflen < SHA256_BLOCK_SIZE) ctx->buf[ctx->buflen++] = 0;
        sha256_compress(ctx->state, ctx->buf);
        ctx->buflen = 0;
    }
    while (ctx->buflen < 56) ctx->buf[ctx->buflen++] = 0;

    /* 64-bit big-endian bit length. */
    for (i = 0; i < 8; i++)
        ctx->buf[56 + i] = (uint8_t)(bits >> (56 - 8 * i));
    sha256_compress(ctx->state, ctx->buf);

    for (i = 0; i < 8; i++) {
        out[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(ctx->state[i] >>  8);
        out[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

void sha256(const void *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE])
{
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}
