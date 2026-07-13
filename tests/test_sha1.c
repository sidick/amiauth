/* test_sha1.c — FIPS 180-1 / RFC 3174 vectors. */
#include "test.h"
#include "sha1.h"

void run_sha1_tests(void)
{
    /* "abc" -> a9993e364706816aba3e25717850c26c9cd0d89d
     * ""    -> da39a3ee5e6b4b0d3255bfef95601890afd80709 */
    TEST_PENDING("SHA-1 \"abc\" vector (implement sha1.c in Phase 1)");
    TEST_PENDING("SHA-1 empty-string vector");
    TEST_PENDING("SHA-1 multi-block / streaming update vector");
}
