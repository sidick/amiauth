/* diff_main.c — differential fuzz harness.
 *
 * Cross-checks AmiAuth's crypto primitives against OpenSSL over many randomised
 * inputs, asserting byte-equality. This is the complement to the RFC
 * known-answer tests: KATs prove authority/interop on fixed inputs; this proves
 * agreement with an independent implementation across the input space
 * (lengths, boundaries, key sizes) that fixed vectors cannot cover.
 *
 * Host-only and opt-in (`make diff`) — it links OpenSSL, which is fine as a
 * build/test dependency; the shipped Amiga binary stays dependency-free.
 *
 * Usage: run-diff [iterations] [seed]
 * The seed is printed so any failure is exactly reproducible.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#include "hmac.h"
#include "chacha20.h"
#include "pbkdf2.h"

/* ---- deterministic PRNG (xorshift64*) -------------------------------------- */

static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static uint32_t rng_u32(void) { return (uint32_t)(rng_next() >> 32); }

static size_t rng_len(size_t max) { return (size_t)(rng_u32() % (uint32_t)(max + 1)); }

static void rng_bytes(uint8_t *p, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) p[i] = (uint8_t)rng_u32();
}

/* ---- harness bookkeeping --------------------------------------------------- */

static long g_checks;
static long g_fails;

static void report_fail(const char *prim, const char *detail)
{
    g_fails++;
    if (g_fails <= 10)
        fprintf(stderr, "MISMATCH [%s] %s\n", prim, detail);
}

/* ---- per-primitive differential passes ------------------------------------- */

static void diff_sha1(int iters)
{
    uint8_t buf[4096], a[20], b[20];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t n = rng_len(sizeof(buf));
        rng_bytes(buf, n);
        sha1(buf, n, a);
        EVP_Digest(buf, n, b, &blen, EVP_sha1(), NULL);
        g_checks++;
        if (memcmp(a, b, 20) != 0) {
            char d[64];
            sprintf(d, "len=%lu", (unsigned long)n);
            report_fail("sha1", d);
        }
    }
}

static void diff_sha256(int iters)
{
    uint8_t buf[4096], a[32], b[32];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t n = rng_len(sizeof(buf));
        rng_bytes(buf, n);
        sha256(buf, n, a);
        EVP_Digest(buf, n, b, &blen, EVP_sha256(), NULL);
        g_checks++;
        if (memcmp(a, b, 32) != 0) {
            char d[64];
            sprintf(d, "len=%lu", (unsigned long)n);
            report_fail("sha256", d);
        }
    }
}

static void diff_sha512(int iters)
{
    uint8_t buf[4096], a[64], b[64];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t n = rng_len(sizeof(buf));
        rng_bytes(buf, n);
        sha512(buf, n, a);
        EVP_Digest(buf, n, b, &blen, EVP_sha512(), NULL);
        g_checks++;
        if (memcmp(a, b, 64) != 0) {
            char d[64];
            sprintf(d, "len=%lu", (unsigned long)n);
            report_fail("sha512", d);
        }
    }
}

static void diff_hmac(int iters)
{
    uint8_t key[128], msg[4096], a[20], b[20];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t kl = rng_len(sizeof(key));   /* spans <, =, > the 64-byte block */
        size_t ml = rng_len(sizeof(msg));
        rng_bytes(key, kl);
        rng_bytes(msg, ml);
        hmac_sha1(key, kl, msg, ml, a);
        HMAC(EVP_sha1(), key, (int)kl, msg, ml, b, &blen);
        g_checks++;
        if (memcmp(a, b, 20) != 0) {
            char d[64];
            sprintf(d, "keylen=%lu msglen=%lu", (unsigned long)kl, (unsigned long)ml);
            report_fail("hmac-sha1", d);
        }
    }
}

static void diff_hmac_sha256(int iters)
{
    uint8_t key[137], msg[4096], a[32], b[32];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t kl = rng_len(sizeof(key));   /* spans <, =, > the 64-byte block */
        size_t ml = rng_len(sizeof(msg));
        rng_bytes(key, kl);
        rng_bytes(msg, ml);
        hmac_sha256(key, kl, msg, ml, a);
        HMAC(EVP_sha256(), key, (int)kl, msg, ml, b, &blen);
        g_checks++;
        if (memcmp(a, b, 32) != 0) {
            char d[64];
            sprintf(d, "keylen=%lu msglen=%lu", (unsigned long)kl, (unsigned long)ml);
            report_fail("hmac-sha256", d);
        }
    }
}

static void diff_hmac_sha512(int iters)
{
    uint8_t key[137], msg[4096], a[64], b[64];
    unsigned int blen;
    int i;
    for (i = 0; i < iters; i++) {
        size_t kl = rng_len(sizeof(key));   /* spans <, =, > the 128-byte block */
        size_t ml = rng_len(sizeof(msg));
        rng_bytes(key, kl);
        rng_bytes(msg, ml);
        hmac_sha512(key, kl, msg, ml, a);
        HMAC(EVP_sha512(), key, (int)kl, msg, ml, b, &blen);
        g_checks++;
        if (memcmp(a, b, 64) != 0) {
            char d[64];
            sprintf(d, "keylen=%lu msglen=%lu", (unsigned long)kl, (unsigned long)ml);
            report_fail("hmac-sha512", d);
        }
    }
}

static void diff_chacha20(int iters)
{
    uint8_t key[32], nonce[12], in[2048], a[2048], b[2048], iv[16];
    int i;
    for (i = 0; i < iters; i++) {
        size_t n = rng_len(sizeof(in));
        uint32_t counter = rng_u32();
        int outl;
        EVP_CIPHER_CTX *ctx;

        rng_bytes(key, sizeof(key));
        rng_bytes(nonce, sizeof(nonce));
        rng_bytes(in, n);

        chacha20_xor(key, nonce, counter, in, a, n);

        /* OpenSSL EVP_chacha20 IV = 4-byte LE counter || 12-byte nonce. */
        iv[0] = (uint8_t)(counter);
        iv[1] = (uint8_t)(counter >> 8);
        iv[2] = (uint8_t)(counter >> 16);
        iv[3] = (uint8_t)(counter >> 24);
        memcpy(iv + 4, nonce, 12);

        ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv);
        EVP_EncryptUpdate(ctx, b, &outl, in, (int)n);
        EVP_CIPHER_CTX_free(ctx);

        g_checks++;
        if ((size_t)outl != n || memcmp(a, b, n) != 0) {
            char d[64];
            sprintf(d, "len=%lu counter=%u", (unsigned long)n, counter);
            report_fail("chacha20", d);
        }
    }
}

static void diff_pbkdf2(int iters)
{
    uint8_t pass[64], salt[64], a[64], b[64];
    int i;
    for (i = 0; i < iters; i++) {
        size_t pl = rng_len(sizeof(pass));
        size_t sl = rng_len(sizeof(salt));
        uint32_t iter = 1 + (rng_u32() % 64);   /* kept small so fuzzing is fast */
        size_t dk = 1 + rng_len(63);            /* 1..64, spans SHA-1 blocks */

        rng_bytes(pass, pl);
        rng_bytes(salt, sl);

        pbkdf2_hmac_sha1(pass, pl, salt, sl, iter, a, dk);
        PKCS5_PBKDF2_HMAC((const char *)pass, (int)pl, salt, (int)sl,
                          (int)iter, EVP_sha1(), (int)dk, b);
        g_checks++;
        if (memcmp(a, b, dk) != 0) {
            char d[96];
            sprintf(d, "passlen=%lu saltlen=%lu iter=%u dkLen=%lu",
                    (unsigned long)pl, (unsigned long)sl, iter, (unsigned long)dk);
            report_fail("pbkdf2", d);
        }
    }
}

int main(int argc, char **argv)
{
    int iters = argc > 1 ? atoi(argv[1]) : 5000;
    uint64_t seed = argc > 2 ? strtoull(argv[2], NULL, 0) : 0x0BADC0DECAFEULL;

    if (iters <= 0) iters = 5000;
    rng_state = seed ? seed : 1;   /* xorshift needs a non-zero seed */

    printf("differential fuzz: %d iterations/primitive, seed=0x%llx\n",
           iters, (unsigned long long)seed);

    diff_sha1(iters);
    diff_sha256(iters);
    diff_sha512(iters);
    diff_hmac(iters);
    diff_hmac_sha256(iters);
    diff_hmac_sha512(iters);
    diff_chacha20(iters);
    diff_pbkdf2(iters);

    printf("%ld checks, %ld mismatches\n", g_checks, g_fails);
    if (g_fails)
        printf("FAILED (reproduce with: run-diff %d 0x%llx)\n",
               iters, (unsigned long long)seed);
    else
        printf("OK\n");

    return g_fails ? 1 : 0;
}
