/* pbkdf2.c — PBKDF2-HMAC-SHA1 (RFC 2898). STUB: implement in Phase 2, validate
 * against RFC 6070 test vectors (tests/test_pbkdf2.c). */
#include <string.h>

#include "pbkdf2.h"
#include "hmac.h"

void pbkdf2_hmac_sha1(const uint8_t *pass, size_t passlen,
                      const uint8_t *salt, size_t saltlen,
                      uint32_t iterations,
                      uint8_t *out, size_t outlen)
{
    (void)pass; (void)passlen; (void)salt; (void)saltlen; (void)iterations;
    if (out) memset(out, 0, outlen);
    /* TODO: F(P,S,c,i) = U1 ^ U2 ^ ... ^ Uc, block-index i big-endian appended
     * to salt, concatenate blocks to fill outlen. */
}
