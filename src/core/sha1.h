/* sha1.h — SHA-1 (FIPS 180-1), used by HMAC, OTP, PBKDF2 and the vault MAC. */
#ifndef AMIAUTH_SHA1_H
#define AMIAUTH_SHA1_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE  64

typedef struct {
    uint32_t state[5];
    uint64_t count;               /* total bytes processed */
    uint8_t  buf[SHA1_BLOCK_SIZE];
    size_t   buflen;              /* bytes currently in buf */
} sha1_ctx;

void sha1_init(sha1_ctx *ctx);
void sha1_update(sha1_ctx *ctx, const void *data, size_t len);
void sha1_final(sha1_ctx *ctx, uint8_t out[SHA1_DIGEST_SIZE]);

/* One-shot convenience wrapper. */
void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE]);

#endif /* AMIAUTH_SHA1_H */
