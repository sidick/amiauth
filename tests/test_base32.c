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
