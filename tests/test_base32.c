/* test_base32.c — RFC 4648 Base32 decode vectors + tolerance/error cases. */
#include <string.h>

#include "test.h"
#include "base32.h"

void run_base32_tests(void)
{
    uint8_t out[32];
    int n;

    /* RFC 4648 vectors. */
    n = base32_decode("MY======", out, sizeof(out));
    TEST_CHECK(n == 1 && memcmp(out, "f", 1) == 0);

    n = base32_decode("MZXW6YTBOI======", out, sizeof(out));
    TEST_CHECK(n == 6 && memcmp(out, "foobar", 6) == 0);

    /* Empty input decodes to zero bytes. */
    TEST_CHECK(base32_decode("", out, sizeof(out)) == 0);

    /* Lower-case and embedded whitespace are tolerated. */
    n = base32_decode("mzxw6 ytb\toi\n", out, sizeof(out));
    TEST_CHECK(n == 6 && memcmp(out, "foobar", 6) == 0);

    /* Missing '=' padding still decodes (pasted secrets often drop it). */
    n = base32_decode("MZXW6YTBOI", out, sizeof(out));
    TEST_CHECK(n == 6 && memcmp(out, "foobar", 6) == 0);

    /* Symbols outside the alphabet (0/1/8/9) are rejected. */
    TEST_CHECK(base32_decode("MZXW6YT1", out, sizeof(out)) == -1);
    TEST_CHECK(base32_decode("MZXW6Y0B", out, sizeof(out)) == -1);

    /* Output exceeding the buffer is an error, not a truncation. */
    TEST_CHECK(base32_decode("MZXW6YTBOI======", out, 3) == -1);
}

/* base32_encode (#45, QR export): RFC 4648 vectors, uppercase, no padding. */
void run_base32_encode_tests(void)
{
    char buf[64];

    TEST_CHECK(base32_encode((const uint8_t *)"f", 1, buf, sizeof(buf)) == 2);
    TEST_CHECK(strcmp(buf, "MY") == 0);

    TEST_CHECK(base32_encode((const uint8_t *)"foobar", 6, buf, sizeof(buf)) == 10);
    TEST_CHECK(strcmp(buf, "MZXW6YTBOI") == 0);

    /* Zero-length input encodes to an empty string. */
    TEST_CHECK(base32_encode((const uint8_t *)"", 0, buf, sizeof(buf)) == 0);
    TEST_CHECK(buf[0] == '\0');

    /* Round-trip through decode for every byte value 0-255, in one buffer. */
    {
        uint8_t raw[256];
        uint8_t back[256];
        char enc[512];
        int i, n;
        for (i = 0; i < 256; i++) raw[i] = (uint8_t)i;
        n = base32_encode(raw, sizeof(raw), enc, sizeof(enc));
        TEST_CHECK(n > 0);
        TEST_CHECK(base32_decode(enc, back, sizeof(back)) == 256);
        TEST_CHECK(memcmp(raw, back, 256) == 0);
    }

    /* Too-small buffer is an error, not a truncation. */
    TEST_CHECK(base32_encode((const uint8_t *)"foobar", 6, buf, 4) == -1);

    /* NULL arguments. */
    TEST_CHECK(base32_encode(NULL, 1, buf, sizeof(buf)) == -1);
    TEST_CHECK(base32_encode((const uint8_t *)"f", 1, NULL, sizeof(buf)) == -1);
}
