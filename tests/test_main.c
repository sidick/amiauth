/* test_main.c — runner. Each module contributes a run_*_tests() entry point. */
#include "test.h"

test_ctx g_test = { 0, 0, 0 };

void run_sha1_tests(void);
void run_hmac_tests(void);
void run_otp_tests(void);
void run_base32_tests(void);
void run_chacha20_tests(void);
void run_pbkdf2_tests(void);
void run_uri_tests(void);
void run_vault_tests(void);
void run_clock_tests(void);
void run_prefs_tests(void);

int main(void)
{
    run_sha1_tests();
    run_hmac_tests();
    run_otp_tests();
    run_base32_tests();
    run_chacha20_tests();
    run_pbkdf2_tests();
    run_uri_tests();
    run_vault_tests();
    run_clock_tests();
    run_prefs_tests();

    printf("\n%d passed, %d failed, %d pending\n",
           g_test.passed, g_test.failed, g_test.pending);

    return g_test.failed ? 1 : 0;
}
