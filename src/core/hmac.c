/* hmac.c — HMAC-SHA1. STUB: implement in Phase 1, validate against RFC 2202
 * (tests/test_hmac.c). */
#include <string.h>

#include "hmac.h"

void hmac_sha1(const uint8_t *key, size_t keylen,
               const uint8_t *msg, size_t msglen,
               uint8_t out[SHA1_DIGEST_SIZE])
{
    (void)key; (void)keylen; (void)msg; (void)msglen;
    if (out) memset(out, 0, SHA1_DIGEST_SIZE);
    /* TODO: K' = key (hashed if > block), ipad/opad, two-pass SHA-1. */
}
