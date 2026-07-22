/* hmac.h — HMAC (RFC 2104) over SHA-1/SHA-256/SHA-512, one-shot and streaming.
 * Validated against RFC 2202 / RFC 4231 vectors in tests/test_hmac.c. The three
 * variants are deliberately parallel copies rather than one vtable-driven
 * generic: each is ~50 lines, and the explicit forms keep call sites (PBKDF2,
 * vault MAC, DRBG — all SHA-1) free of indirection. */
#ifndef AMIAUTH_HMAC_H
#define AMIAUTH_HMAC_H

#include <stddef.h>
#include <stdint.h>

#include "sha1.h"
#include "sha256.h"
#include "sha512.h"

/* Streaming context: the inner hash runs incrementally; opad is kept for the
 * outer pass in _final. */
typedef struct {
    sha1_ctx inner;
    uint8_t  opad[SHA1_BLOCK_SIZE];
} hmac_sha1_ctx;

void hmac_sha1_init(hmac_sha1_ctx *ctx, const uint8_t *key, size_t keylen);
void hmac_sha1_update(hmac_sha1_ctx *ctx, const void *data, size_t len);
void hmac_sha1_final(hmac_sha1_ctx *ctx, uint8_t out[SHA1_DIGEST_SIZE]);

/* One-shot convenience wrapper over the streaming API. */
void hmac_sha1(const uint8_t *key, size_t keylen,
               const uint8_t *msg, size_t msglen,
               uint8_t out[SHA1_DIGEST_SIZE]);

typedef struct {
    sha256_ctx inner;
    uint8_t    opad[SHA256_BLOCK_SIZE];
} hmac_sha256_ctx;

void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t keylen);
void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len);
void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE]);

void hmac_sha256(const uint8_t *key, size_t keylen,
                 const uint8_t *msg, size_t msglen,
                 uint8_t out[SHA256_DIGEST_SIZE]);

typedef struct {
    sha512_ctx inner;
    uint8_t    opad[SHA512_BLOCK_SIZE];
} hmac_sha512_ctx;

void hmac_sha512_init(hmac_sha512_ctx *ctx, const uint8_t *key, size_t keylen);
void hmac_sha512_update(hmac_sha512_ctx *ctx, const void *data, size_t len);
void hmac_sha512_final(hmac_sha512_ctx *ctx, uint8_t out[SHA512_DIGEST_SIZE]);

void hmac_sha512(const uint8_t *key, size_t keylen,
                 const uint8_t *msg, size_t msglen,
                 uint8_t out[SHA512_DIGEST_SIZE]);

#endif /* AMIAUTH_HMAC_H */
