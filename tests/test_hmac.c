/* test_hmac.c — HMAC-SHA1 vectors (RFC 2202). */
#include "test.h"
#include "hmac.h"

void run_hmac_tests(void)
{
    /* key=0x0b*20, data="Hi There"
     *   -> b617318655057264e28bc0b6fb378c8ef146be00
     * key="Jefe", data="what do ya want for nothing?"
     *   -> effcdf6ae5eb2fa2d27416d5f184df9c259a7c79 */
    TEST_PENDING("HMAC-SHA1 RFC 2202 case 1 (implement hmac.c in Phase 1)");
    TEST_PENDING("HMAC-SHA1 RFC 2202 case 2 (key shorter than block)");
    TEST_PENDING("HMAC-SHA1 long-key case (key hashed to fit block)");
}
