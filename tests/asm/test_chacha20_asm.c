/* test_chacha20_asm.c — validates chacha20_block_asm() (#47) against the
 * existing RFC 8439 vectors (encryption, round-trip decryption, in-place),
 * by forcing the dispatch pointer onto the asm before running them. m68k-
 * only: cross-built and run under amitools' vamos in CI (see
 * .github/workflows/ci.yml), on both -C 000 and -C 020. */
#include "test.h"
#include "crypto_dispatch.h"

test_ctx g_test = { 0, 0, 0 };

void run_chacha20_tests(void);
extern void chacha20_block_asm(const uint32_t in[16], uint8_t out[64]);

int main(void)
{
    g_chacha20_block = chacha20_block_asm;

    run_chacha20_tests();

    printf("%d passed, %d failed, %d pending\n",
           g_test.passed, g_test.failed, g_test.pending);
    return g_test.failed != 0;
}
