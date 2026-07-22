/* test_sha512.c — SHA-512 vectors (FIPS 180-4 / NIST examples). */
#include <string.h>

#include "test.h"
#include "sha512.h"

void run_sha512_tests(void)
{
    uint8_t out[SHA512_DIGEST_SIZE];
    sha512_ctx ctx;
    int i;

    /* Empty message. */
    sha512((const uint8_t *)"", 0, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
        "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"));

    /* "abc" (FIPS 180-4 example 1). */
    sha512((const uint8_t *)"abc", 3, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
        "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));

    /* Two-block message (FIPS 180-4 example 2, the 896-bit message). */
    sha512((const uint8_t *)
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
        "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
        "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909"));

    /* One million 'a', streamed in awkward chunk sizes to exercise buffering. */
    sha512_init(&ctx);
    for (i = 0; i < 10000; i++)
        sha512_update(&ctx, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaaaaaaaaaaaa", 100);
    sha512_final(&ctx, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973eb"
        "de0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b"));

    /* Streaming across the 128-byte block boundary equals the one-shot result. */
    sha512_init(&ctx);
    sha512_update(&ctx, "abcdefghbcdefghicdefghijdefghijkefghijklfghijklm"
                        "ghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrs", 96);
    sha512_update(&ctx, "mnopqrstnopqrstu", 16);
    sha512_final(&ctx, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
        "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909"));
}
