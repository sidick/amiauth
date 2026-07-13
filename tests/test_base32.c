/* test_base32.c — RFC 4648 Base32 decode vectors. */
#include "test.h"
#include "base32.h"

void run_base32_tests(void)
{
    /* RFC 4648 test vectors:
     *   "MY======"       -> "f"
     *   "MZXW6==="       -> "foo"
     *   "MZXW6YTBOI======" -> "foobar"
     * Plus tolerance: lower-case, embedded spaces, and missing padding all
     * decode identically. */
    TEST_PENDING("Base32 \"MY======\" -> \"f\" (implement base32.c in Phase 1)");
    TEST_PENDING("Base32 \"MZXW6YTBOI======\" -> \"foobar\"");
    TEST_PENDING("Base32 lower-case + whitespace tolerance");
    TEST_PENDING("Base32 missing-padding tolerance");
    TEST_PENDING("Base32 invalid char -> -1");
    TEST_PENDING("Base32 output exceeding outcap -> -1");
}
