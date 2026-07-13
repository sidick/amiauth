/* sha1.c — SHA-1. STUB: implement in Phase 1 and validate against FIPS 180-1
 * / RFC 3174 vectors (tests/test_sha1.c). */
#include <string.h>

#include "sha1.h"

void sha1_init(sha1_ctx *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    /* TODO: seed H0..H4. */
}

void sha1_update(sha1_ctx *ctx, const void *data, size_t len)
{
    (void)ctx; (void)data; (void)len;
    /* TODO: buffer and compress full 64-byte blocks. */
}

void sha1_final(sha1_ctx *ctx, uint8_t out[SHA1_DIGEST_SIZE])
{
    (void)ctx;
    if (out) memset(out, 0, SHA1_DIGEST_SIZE);
    /* TODO: pad, append length, emit big-endian digest. */
}

void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE])
{
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, out);
}
