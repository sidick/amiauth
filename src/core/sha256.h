/* sha256.h — SHA-256 (FIPS 180-4), for the RFC 6238 HMAC-SHA256 TOTP variant. */
#ifndef AMIAUTH_SHA256_H
#define AMIAUTH_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE  64

typedef struct {
    uint32_t state[8];
    uint64_t count;               /* total bytes processed */
    uint8_t  buf[SHA256_BLOCK_SIZE];
    size_t   buflen;              /* bytes currently in buf */
} sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE]);

/* One-shot convenience wrapper. */
void sha256(const void *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]);

#endif /* AMIAUTH_SHA256_H */
