/* test_qr.c — QR decode of the portable wrapper (src/qr/qr.c over quirc).
 *
 * Uses checked-in greyscale fixtures (tests/qr/sample_gray.h) rendered from
 * known QR codes, so the decoder is exercised end-to-end with zero platform
 * dependencies — the same float path that ships on m68k. */
#include <string.h>

#include "test.h"
#include "qr.h"
#include "uri.h"
#include "otp.h"
#include "qr/sample_gray.h"

/* A large, clearly-off-stack blank canvas for the no-code case. */
static unsigned char g_blank[128 * 128];

void run_qr_tests(void)
{
    char uri[512];
    otp_account a;
    size_t i;

    /* --- an otpauth:// QR decodes to the exact enrolment URI --- */
    TEST_CHECK(qr_decode_gray(qr_otp_gray, QR_OTP_W, QR_OTP_H,
                              uri, sizeof uri) == QR_OK);
    TEST_CHECK(strcmp(uri, QR_OTP_URI) == 0);

    /* ...and that URI feeds the real add path (otpauth_parse) cleanly. */
    TEST_CHECK(otpauth_parse(uri, &a) == 0);
    TEST_CHECK(strcmp(a.type, "totp") == 0);
    TEST_CHECK(strcmp(a.issuer, "AmiAuth") == 0);
    TEST_CHECK(strcmp(a.label, "demo") == 0);
    TEST_CHECK(a.secret_len == 10 && memcmp(a.secret, "Hello", 5) == 0);

    /* --- a valid QR that isn't otpauth:// is decoded but rejected --- */
    uri[0] = 'x';
    TEST_CHECK(qr_decode_gray(qr_hello_gray, QR_HELLO_W, QR_HELLO_H,
                              uri, sizeof uri) == QR_ERR_NOTOTP);
    TEST_CHECK(uri[0] == '\0');   /* left empty on failure */

    /* --- a blank image contains no code --- */
    for (i = 0; i < sizeof g_blank; i++)
        g_blank[i] = 255;
    TEST_CHECK(qr_decode_gray(g_blank, 128, 128, uri, sizeof uri) == QR_ERR_NOCODE);

    /* --- a QR with zero quiet zone (some export tools crop this tight) still
     * decodes, via the padded-retry fallback --- */
    TEST_CHECK(qr_decode_gray(qr_noquietzone_gray, QR_NOQUIETZONE_W, QR_NOQUIETZONE_H,
                              uri, sizeof uri) == QR_OK);
    TEST_CHECK(strcmp(uri, QR_OTP_URI) == 0);

    /* --- argument guards --- */
    TEST_CHECK(qr_decode_gray(NULL, QR_OTP_W, QR_OTP_H, uri, sizeof uri) == QR_ERR_ARGS);
    TEST_CHECK(qr_decode_gray(qr_otp_gray, 0, QR_OTP_H, uri, sizeof uri) == QR_ERR_ARGS);
    TEST_CHECK(qr_decode_gray(qr_otp_gray, QR_OTP_W, QR_OTP_H, uri, 0) == QR_ERR_ARGS);

    /* --- a too-small output buffer truncates but still succeeds --- */
    TEST_CHECK(qr_decode_gray(qr_otp_gray, QR_OTP_W, QR_OTP_H, uri, 10) == QR_OK);
    TEST_CHECK(strlen(uri) == 9 && strncmp(uri, QR_OTP_URI, 9) == 0);
}
