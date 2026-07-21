/* test_sha1_asm.c — validates sha1_compress_asm() (#47) against every
 * existing SHA-1/HMAC/PBKDF2 RFC vector already in the test suite, by
 * forcing the dispatch pointer onto the asm before running them. m68k-only:
 * cross-built and run under amitools' vamos in CI (see .github/workflows/
 * ci.yml), on both -C 000 and -C 020, since the asm targets the plain 68000
 * baseline and must be correct on every CPU tier this project supports. */
#include "test.h"
#include "crypto_dispatch.h"

test_ctx g_test = { 0, 0, 0 };

void run_sha1_tests(void);
void run_hmac_tests(void);
void run_pbkdf2_tests(void);
void run_kdf_tests(void);

extern void sha1_compress_asm(uint32_t state[5], const uint8_t block[SHA1_BLOCK_SIZE]);

int main(void)
{
    g_sha1_compress = sha1_compress_asm;

    run_sha1_tests();
    run_hmac_tests();
    run_pbkdf2_tests();
    run_kdf_tests();

    printf("%d passed, %d failed, %d pending\n",
           g_test.passed, g_test.failed, g_test.pending);
    return g_test.failed != 0;
}
