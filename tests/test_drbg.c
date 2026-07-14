/* test_drbg.c — HMAC-DRBG (SHA-1) tests.
 *
 * The known-answer vectors are produced by an independent Python HMAC-DRBG
 * reference (hmac/hashlib) mirroring SP 800-90A 10.1.2, so this is a genuine
 * cross-implementation check, not a self-consistency snapshot. Plus structural
 * properties: determinism, seed independence, reseed effect, and distribution. */
#include <string.h>

#include "test.h"
#include "drbg.h"

static void seed_0_to_31(uint8_t seed[32])
{
    int i;
    for (i = 0; i < 32; i++) seed[i] = (uint8_t)i;
}

void run_drbg_tests(void);
void run_drbg_tests(void)
{
    uint8_t seed[32];
    uint8_t out[64], out2[64];
    drbg_state st, st2;

    /* --- Known-answer (Python HMAC-DRBG oracle, seed = 00 01 .. 1f) --- */
    seed_0_to_31(seed);
    drbg_init(&st, seed, sizeof seed);
    drbg_generate(&st, out, 64);
    TEST_CHECK(hex_eq(out, 64,
        "be491355307bb821bf72d7f115d91156de1562e28edb0fd1f2270730e4195cb8"
        "60d32e13c9a8c541514cc9b450563057cd4e221df0b562cdd371dce9ad76eb38"));

    /* Second draw: state has advanced, so a different block. */
    drbg_generate(&st, out, 16);
    TEST_CHECK(hex_eq(out, 16, "97e6b33424af4ece315f255b1353feb6"));

    /* --- Determinism: same seed -> same stream. --- */
    seed_0_to_31(seed);
    drbg_init(&st, seed, sizeof seed);
    drbg_generate(&st, out, 64);
    drbg_init(&st2, seed, sizeof seed);
    drbg_generate(&st2, out2, 64);
    TEST_CHECK(memcmp(out, out2, 64) == 0);

    /* --- Seed independence: a different seed -> a different, known stream. --- */
    drbg_init(&st2, (const uint8_t *)"amiauth", 7);
    drbg_generate(&st2, out2, 8);
    TEST_CHECK(hex_eq(out2, 8, "08ba018ad43f35af"));
    TEST_CHECK(memcmp(out, out2, 8) != 0);         /* differs from the 0..1f stream */

    /* --- Reseed changes the stream. --- */
    seed_0_to_31(seed);
    drbg_init(&st, seed, sizeof seed);
    drbg_reseed(&st, (const uint8_t *)"\xaa\xbb\xcc\xdd", 4);
    drbg_generate(&st, out2, 64);
    TEST_CHECK(memcmp(out, out2, 64) != 0);

    /* --- Distribution sanity: a large draw is non-degenerate. --- */
    {
        static uint8_t big[4096];
        int seen[256];
        int i, distinct = 0, nonzero = 0;
        memset(seen, 0, sizeof seen);
        seed_0_to_31(seed);
        drbg_init(&st, seed, sizeof seed);
        drbg_generate(&st, big, sizeof big);
        for (i = 0; i < (int)sizeof big; i++) {
            if (!seen[big[i]]) { seen[big[i]] = 1; distinct++; }
            if (big[i]) nonzero++;
        }
        TEST_CHECK(distinct == 256);               /* every byte value appears */
        TEST_CHECK(nonzero > 4000);                /* not a run of zeros */
    }
}
