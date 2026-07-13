/* pbkdf2.c — PBKDF2-HMAC-SHA1 (RFC 2898 §5.2).
 * Validated against RFC 6070 vectors in tests/test_pbkdf2.c. */
#include <string.h>

#include "pbkdf2.h"
#include "hmac.h"

/* One derived block: T_i = U_1 ^ U_2 ^ ... ^ U_c, where
 *   U_1 = HMAC(P, S || INT_BE32(i)),  U_n = HMAC(P, U_{n-1}). */
static void pbkdf2_block(const uint8_t *pass, size_t passlen,
                         const uint8_t *salt, size_t saltlen,
                         uint32_t iterations, uint32_t blockidx,
                         uint8_t out[SHA1_DIGEST_SIZE])
{
    uint8_t idx[4];
    uint8_t u[SHA1_DIGEST_SIZE];
    uint8_t t[SHA1_DIGEST_SIZE];
    hmac_sha1_ctx c;
    uint32_t n;
    int i;

    idx[0] = (uint8_t)(blockidx >> 24);
    idx[1] = (uint8_t)(blockidx >> 16);
    idx[2] = (uint8_t)(blockidx >>  8);
    idx[3] = (uint8_t)(blockidx);

    /* U_1 = HMAC(P, S || idx) — streamed to avoid concatenating into a buffer. */
    hmac_sha1_init(&c, pass, passlen);
    hmac_sha1_update(&c, salt, saltlen);
    hmac_sha1_update(&c, idx, sizeof(idx));
    hmac_sha1_final(&c, u);
    memcpy(t, u, SHA1_DIGEST_SIZE);

    for (n = 1; n < iterations; n++) {
        hmac_sha1(pass, passlen, u, SHA1_DIGEST_SIZE, u);
        for (i = 0; i < SHA1_DIGEST_SIZE; i++) t[i] ^= u[i];
    }

    memcpy(out, t, SHA1_DIGEST_SIZE);
}

void pbkdf2_hmac_sha1(const uint8_t *pass, size_t passlen,
                      const uint8_t *salt, size_t saltlen,
                      uint32_t iterations,
                      uint8_t *out, size_t outlen)
{
    uint8_t block[SHA1_DIGEST_SIZE];
    uint32_t blockidx = 1;
    size_t got = 0;

    if (!out) return;
    if (iterations == 0) iterations = 1;

    while (got < outlen) {
        size_t n = outlen - got < SHA1_DIGEST_SIZE
                 ? outlen - got : SHA1_DIGEST_SIZE;
        pbkdf2_block(pass, passlen, salt, saltlen, iterations, blockidx, block);
        memcpy(out + got, block, n);
        got += n;
        blockidx++;
    }
}
