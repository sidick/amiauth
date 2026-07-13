/* hmac.h — HMAC-SHA1 (RFC 2104), one-shot and streaming.
 * Validated against RFC 2202 vectors in tests/test_hmac.c. */
#ifndef AMIAUTH_HMAC_H
#define AMIAUTH_HMAC_H

#include <stddef.h>
#include <stdint.h>

#include "sha1.h"

/* Streaming context: the inner SHA-1 runs incrementally; opad is kept for the
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

#endif /* AMIAUTH_HMAC_H */
