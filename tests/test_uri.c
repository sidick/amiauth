/* test_uri.c — otpauth:// parsing. */
#include "test.h"
#include "uri.h"

void run_uri_tests(void)
{
    /* otpauth://totp/GitHub:alice?secret=JBSWY3DPEHPK3PXP&issuer=GitHub&digits=6&period=30
     *   type=totp, issuer=GitHub, label contains "alice",
     *   secret decodes to 10 bytes, digits=6, period=30. */
    TEST_PENDING("otpauth totp parse: fields + secret (implement uri.c in Phase 2)");
    TEST_PENDING("otpauth hotp parse: counter param");
    TEST_PENDING("otpauth URL-decoding of issuer/label");
    TEST_PENDING("otpauth defaults applied when digits/period omitted");

    /* Malformed input must be rejected. */
    otp_account acct;
    TEST_CHECK(otpauth_parse(NULL, &acct) == -1);
    TEST_CHECK(otpauth_parse("otpauth://totp/x", NULL) == -1);
}
