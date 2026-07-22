/* sha512.h — SHA-512 (FIPS 180-4), for the RFC 6238 HMAC-SHA512 TOTP variant. */
#ifndef AMIAUTH_SHA512_H
#define AMIAUTH_SHA512_H

#include <stddef.h>
#include <stdint.h>

#define SHA512_DIGEST_SIZE 64
#define SHA512_BLOCK_SIZE  128

typedef struct {
    uint64_t state[8];
    uint64_t count;               /* total bytes processed */
    uint8_t  buf[SHA512_BLOCK_SIZE];
    size_t   buflen;              /* bytes currently in buf */
} sha512_ctx;

void sha512_init(sha512_ctx *ctx);
void sha512_update(sha512_ctx *ctx, const void *data, size_t len);
void sha512_final(sha512_ctx *ctx, uint8_t out[SHA512_DIGEST_SIZE]);

/* One-shot convenience wrapper. */
void sha512(const void *data, size_t len, uint8_t out[SHA512_DIGEST_SIZE]);

#endif /* AMIAUTH_SHA512_H */
