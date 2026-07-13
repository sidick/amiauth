/* test_pbkdf2.c — PBKDF2-HMAC-SHA1 vectors (RFC 6070). */
#include "test.h"
#include "pbkdf2.h"

void run_pbkdf2_tests(void)
{
    /* RFC 6070: P="password", S="salt", c=1, dkLen=20
     *   -> 0c60c80f961f0e71f3a9b524af6012062fe037a6
     * P="password", S="salt", c=4096, dkLen=20
     *   -> 4b007901b765489abead49d926f721d065a429c1 */
    TEST_PENDING("PBKDF2 RFC 6070 c=1 (implement pbkdf2.c in Phase 2)");
    TEST_PENDING("PBKDF2 RFC 6070 c=4096");
    TEST_PENDING("PBKDF2 dkLen spanning multiple blocks");
}
