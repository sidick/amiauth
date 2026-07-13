/* test_sha1.c — FIPS 180-1 / RFC 3174 vectors. */
#include <string.h>

#include "test.h"
#include "sha1.h"

static int digest_is(const uint8_t d[SHA1_DIGEST_SIZE], const char *hex)
{
    static const char H[] = "0123456789abcdef";
    char got[SHA1_DIGEST_SIZE * 2 + 1];
    int i;
    for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
        got[i * 2]     = H[d[i] >> 4];
        got[i * 2 + 1] = H[d[i] & 0x0f];
    }
    got[SHA1_DIGEST_SIZE * 2] = '\0';
    return strcmp(got, hex) == 0;
}

void run_sha1_tests(void)
{
    uint8_t d[SHA1_DIGEST_SIZE];

    /* RFC 3174 TEST1 */
    sha1("abc", 3, d);
    TEST_CHECK(digest_is(d, "a9993e364706816aba3e25717850c26c9cd0d89d"));

    /* Empty string */
    sha1("", 0, d);
    TEST_CHECK(digest_is(d, "da39a3ee5e6b4b0d3255bfef95601890afd80709"));

    /* RFC 3174 TEST2 — 56 bytes, spans two compression blocks */
    sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, d);
    TEST_CHECK(digest_is(d, "84983e441c3bd26ebaae4aa1f95129e5e54670f1"));

    /* Streaming byte-at-a-time must equal the one-shot digest. */
    {
        sha1_ctx c;
        const char *msg = "The quick brown fox jumps over the lazy dog";
        size_t i, n = strlen(msg);
        sha1_init(&c);
        for (i = 0; i < n; i++) sha1_update(&c, msg + i, 1);
        sha1_final(&c, d);
        TEST_CHECK(digest_is(d, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"));
    }

    /* 1,000,000 x 'a' streamed in chunks — exercises many-block buffering. */
    {
        sha1_ctx c;
        uint8_t chunk[256];
        long remaining = 1000000;
        memset(chunk, 'a', sizeof(chunk));
        sha1_init(&c);
        while (remaining > 0) {
            size_t take = remaining < (long)sizeof(chunk)
                        ? (size_t)remaining : sizeof(chunk);
            sha1_update(&c, chunk, take);
            remaining -= (long)take;
        }
        sha1_final(&c, d);
        TEST_CHECK(digest_is(d, "34aa973cd4c4daa4f61eeb2bdbad27316534016f"));
    }
}
