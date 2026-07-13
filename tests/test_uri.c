/* test_uri.c — otpauth:// parsing. */
#include <string.h>

#include "test.h"
#include "uri.h"
#include "otp.h"

void run_uri_tests(void)
{
    otp_account a;

    /* Full TOTP URI. Secret JBSWY3DPEHPK3PXP = 10 bytes beginning "Hello". */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/GitHub:alice?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"
        "&digits=6&period=30", &a) == 0);
    TEST_CHECK(strcmp(a.type, "totp") == 0);
    TEST_CHECK(strcmp(a.issuer, "GitHub") == 0);
    TEST_CHECK(strcmp(a.label, "alice") == 0);
    TEST_CHECK(a.digits == 6 && a.period == 30);
    TEST_CHECK(a.secret_len == 10 && memcmp(a.secret, "Hello", 5) == 0);

    /* HOTP URI with a counter. */
    TEST_CHECK(otpauth_parse(
        "otpauth://hotp/Acme:bob?secret=JBSWY3DPEHPK3PXP&counter=42", &a) == 0);
    TEST_CHECK(strcmp(a.type, "hotp") == 0);
    TEST_CHECK(strcmp(a.issuer, "Acme") == 0 && strcmp(a.label, "bob") == 0);
    TEST_CHECK(a.counter == 42);

    /* Percent-decoding of issuer/label (%20 space, %40 '@'). */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/Example%20Co:john%40example.com?secret=JBSWY3DP"
        "&issuer=Example%20Co", &a) == 0);
    TEST_CHECK(strcmp(a.issuer, "Example Co") == 0);
    TEST_CHECK(strcmp(a.label, "john@example.com") == 0);
    TEST_CHECK(a.secret_len == 5 && memcmp(a.secret, "Hello", 5) == 0);

    /* Defaults applied when digits/period/algorithm omitted; bare label. */
    TEST_CHECK(otpauth_parse("otpauth://totp/x?secret=JBSWY3DP", &a) == 0);
    TEST_CHECK(a.digits == OTP_DEFAULT_DIGITS && a.period == OTP_DEFAULT_PERIOD);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);
    TEST_CHECK(strcmp(a.label, "x") == 0 && a.issuer[0] == '\0');

    /* Issuer query parameter overrides the label prefix. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/OldName:acct?secret=JBSWY3DP&issuer=NewName", &a) == 0);
    TEST_CHECK(strcmp(a.issuer, "NewName") == 0);

    /* Rejections: missing secret, wrong scheme, malformed input. */
    TEST_CHECK(otpauth_parse("otpauth://totp/x?issuer=Y", &a) == -1);
    TEST_CHECK(otpauth_parse("otpauth://weird/x?secret=JBSWY3DP", &a) == -1);
    TEST_CHECK(otpauth_parse("http://example.com/", &a) == -1);
    TEST_CHECK(otpauth_parse(NULL, &a) == -1);
    TEST_CHECK(otpauth_parse("otpauth://totp/x", NULL) == -1);
}
