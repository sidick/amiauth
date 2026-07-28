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

    /* digits=7 is accepted (matches the vault format's/GUI's 6-8 range),
     * not silently downgraded to the 6-digit default. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&digits=7", &a) == 0);
    TEST_CHECK(a.digits == 7);

    /* An unsupported digits value is refused outright, same "refuse, don't
     * guess" policy as an unsupported algorithm=; the account is zeroed on
     * any parse failure (see otpauth_parse's wrapper), not left holding
     * whatever partial state the failed attempt built up. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&digits=5", &a) == -1);
    TEST_CHECK(a.label[0] == '\0');
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&digits=9", &a) == -1);

    /* Issuer query parameter overrides the label prefix. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/OldName:acct?secret=JBSWY3DP&issuer=NewName", &a) == 0);
    TEST_CHECK(strcmp(a.issuer, "NewName") == 0);

    /* algorithm= parses case-insensitively to the canonical name. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=SHA256", &a) == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA256") == 0);
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=sha512", &a) == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA512") == 0);
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=Sha1", &a) == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);

    /* An algorithm we don't implement is rejected outright — importing it and
     * generating SHA-1 codes would be silently wrong. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=MD5", &a) == -1);
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=SHA224", &a) == -1);

    /* otpauth://steam/... (#44): forces SHA1/5 regardless of any
     * algorithm=/digits= parameter, since Steam Guard isn't configurable. */
    TEST_CHECK(otpauth_parse(
        "otpauth://steam/Steam:you?secret=JBSWY3DP&issuer=Steam", &a) == 0);
    TEST_CHECK(strcmp(a.type, "steam") == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);
    TEST_CHECK(a.digits == 5);
    TEST_CHECK(otpauth_parse(
        "otpauth://steam/Steam:you?secret=JBSWY3DP&algorithm=SHA256&digits=8", &a) == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);
    TEST_CHECK(a.digits == 5);

    /* Rejections: missing secret, wrong scheme, malformed input. */
    TEST_CHECK(otpauth_parse("otpauth://totp/x?issuer=Y", &a) == -1);
    TEST_CHECK(otpauth_parse("otpauth://weird/x?secret=JBSWY3DP", &a) == -1);
    TEST_CHECK(otpauth_parse("http://example.com/", &a) == -1);
    TEST_CHECK(otpauth_parse(NULL, &a) == -1);
    TEST_CHECK(otpauth_parse("otpauth://totp/x", NULL) == -1);
}

void run_bare_secret_tests(void)
{
    otp_account a;

    /* URI detection routes add-account input. */
    TEST_CHECK(otpauth_is_uri("otpauth://totp/x?secret=JBSWY3DP") == 1);
    TEST_CHECK(otpauth_is_uri("OTPAUTH://totp/x?secret=JBSWY3DP") == 1);
    TEST_CHECK(otpauth_is_uri("JBSWY3DPEHPK3PXP") == 0);
    TEST_CHECK(otpauth_is_uri("") == 0);
    TEST_CHECK(otpauth_is_uri(NULL) == 0);

    /* Bare secret with the common defaults; JBSWY3DPEHPK3PXP = 10 bytes
     * beginning "Hello", same vector as the URI tests above. */
    TEST_CHECK(otp_account_from_secret("GitHub", "alice",
                                       "JBSWY3DPEHPK3PXP", &a) == 0);
    TEST_CHECK(strcmp(a.type, "totp") == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);
    TEST_CHECK(a.digits == OTP_DEFAULT_DIGITS && a.period == OTP_DEFAULT_PERIOD);
    TEST_CHECK(a.counter == 0);
    TEST_CHECK(strcmp(a.issuer, "GitHub") == 0 && strcmp(a.label, "alice") == 0);
    TEST_CHECK(a.secret_len == 10 && memcmp(a.secret, "Hello", 5) == 0);

    /* Spaced/lowercase secret entry works (services show grouped secrets). */
    TEST_CHECK(otp_account_from_secret(NULL, "bob",
                                       "jbsw y3dp ehpk 3pxp", &a) == 0);
    TEST_CHECK(a.issuer[0] == '\0');
    TEST_CHECK(a.secret_len == 10 && memcmp(a.secret, "Hello", 5) == 0);

    /* Rejections: missing label, empty/invalid secret. Scrubbed on failure. */
    TEST_CHECK(otp_account_from_secret("X", "", "JBSWY3DP", &a) == -1);
    TEST_CHECK(otp_account_from_secret("X", NULL, "JBSWY3DP", &a) == -1);
    TEST_CHECK(otp_account_from_secret("X", "y", "", &a) == -1);
    TEST_CHECK(otp_account_from_secret("X", "y", "not!base32", &a) == -1);
    TEST_CHECK(a.secret_len == 0 && a.label[0] == '\0');
    TEST_CHECK(otp_account_from_secret("X", "y", "JBSWY3DP", NULL) == -1);

    /* otp_account_from_secret_steam (#44): same argument rules, but forced
     * "steam"/SHA1/5/30 rather than the ordinary TOTP defaults. */
    TEST_CHECK(otp_account_from_secret_steam("Steam", "friend",
                                             "JBSWY3DPEHPK3PXP", &a) == 0);
    TEST_CHECK(strcmp(a.type, "steam") == 0);
    TEST_CHECK(strcmp(a.algorithm, "SHA1") == 0);
    TEST_CHECK(a.digits == 5 && a.period == 30);
    TEST_CHECK(strcmp(a.issuer, "Steam") == 0 && strcmp(a.label, "friend") == 0);
    TEST_CHECK(a.secret_len == 10 && memcmp(a.secret, "Hello", 5) == 0);
    TEST_CHECK(otp_account_from_secret_steam("X", "", "JBSWY3DP", &a) == -1);
    TEST_CHECK(otp_account_from_secret_steam("X", "y", "not!base32", &a) == -1);
}

/* otpauth_build (#45, QR export): the inverse of otpauth_parse. */
void run_uri_build_tests(void)
{
    otp_account a, b;
    char uri[512];

    /* Exact string for a simple, fully-specified TOTP account. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/GitHub:alice?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"
        "&digits=6&period=30", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(strcmp(uri,
        "otpauth://totp/GitHub:alice?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"
        "&algorithm=SHA1&digits=6&period=30") == 0);

    /* Round-trip (parse -> build -> parse) preserves every field, across
     * TOTP/HOTP, every algorithm, and Steam. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/x?secret=JBSWY3DP&algorithm=SHA256&digits=8&period=60", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(otpauth_parse(uri, &b) == 0);
    TEST_CHECK(strcmp(a.type, b.type) == 0 && strcmp(a.algorithm, b.algorithm) == 0);
    TEST_CHECK(a.digits == b.digits && a.period == b.period);
    TEST_CHECK(a.secret_len == b.secret_len &&
               memcmp(a.secret, b.secret, a.secret_len) == 0);

    TEST_CHECK(otpauth_parse(
        "otpauth://hotp/Acme:bob?secret=JBSWY3DPEHPK3PXP&algorithm=SHA512"
        "&digits=8&counter=4294967297", &a) == 0);      /* counter > 32 bits */
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(otpauth_parse(uri, &b) == 0);
    TEST_CHECK(strcmp(b.type, "hotp") == 0 && strcmp(b.algorithm, "SHA512") == 0);
    TEST_CHECK(b.digits == 8 && b.counter == 4294967297ULL);

    TEST_CHECK(otpauth_parse(
        "otpauth://steam/Steam:you?secret=JBSWY3DP&issuer=Steam", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(otpauth_parse(uri, &b) == 0);
    TEST_CHECK(strcmp(b.type, "steam") == 0 && b.digits == 5);
    TEST_CHECK(strcmp(b.issuer, "Steam") == 0 && strcmp(b.label, "you") == 0);

    /* Issuer/label containing space and '@' round-trip through percent
     * encoding (build) and percent decoding (parse). */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/Example%20Co:john%40example.com?secret=JBSWY3DP"
        "&issuer=Example%20Co", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(otpauth_parse(uri, &b) == 0);
    TEST_CHECK(strcmp(b.issuer, "Example Co") == 0);
    TEST_CHECK(strcmp(b.label, "john@example.com") == 0);

    /* No issuer: no leading "issuer:" on the label, no &issuer= param. */
    TEST_CHECK(otpauth_parse("otpauth://totp/x?secret=JBSWY3DP", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(strstr(uri, "issuer") == NULL);
    TEST_CHECK(strncmp(uri, "otpauth://totp/x?", 17) == 0);

    /* Too-small buffer is an error, not a truncation. */
    TEST_CHECK(otpauth_build(&a, uri, 5) == -1);

    /* NULL arguments. */
    TEST_CHECK(otpauth_build(NULL, uri, sizeof(uri)) == -1);
    TEST_CHECK(otpauth_build(&a, NULL, sizeof(uri)) == -1);
}
