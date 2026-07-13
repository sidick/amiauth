/* hmac.h — HMAC-SHA1 (RFC 2104). Stub: see docs/ROADMAP.md Phase 1. */
#ifndef AMIAUTH_HMAC_H
#define AMIAUTH_HMAC_H

#include <stddef.h>
#include <stdint.h>

#include "sha1.h"

void hmac_sha1(const uint8_t *key, size_t keylen,
               const uint8_t *msg, size_t msglen,
               uint8_t out[SHA1_DIGEST_SIZE]);

#endif /* AMIAUTH_HMAC_H */
