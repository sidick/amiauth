/* test_sha256.c — SHA-256 vectors (FIPS 180-4 / NIST examples). */
#include <string.h>

#include "test.h"
#include "sha256.h"

void run_sha256_tests(void)
{
    uint8_t out[SHA256_DIGEST_SIZE];
    sha256_ctx ctx;
    int i;

    /* Empty message. */
    sha256((const uint8_t *)"", 0, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    /* "abc" (FIPS 180-4 example 1). */
    sha256((const uint8_t *)"abc", 3, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

    /* Two-block message (FIPS 180-4 example 2). */
    sha256((const uint8_t *)
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));

    /* One million 'a', streamed in awkward chunk sizes to exercise buffering. */
    sha256_init(&ctx);
    for (i = 0; i < 10000; i++)
        sha256_update(&ctx, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                            "aaaaaaaaaaaaaaaaaaaa", 100);
    sha256_final(&ctx, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));

    /* Streaming across the block boundary equals the one-shot result. */
    sha256_init(&ctx);
    sha256_update(&ctx, "abcdbcdecdefdefgefghfghighijhijk", 32);
    sha256_update(&ctx, "ijkljklmklmnlmnomnopnopq", 24);
    sha256_final(&ctx, out);
    TEST_CHECK(hex_eq(out, sizeof(out),
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
}
