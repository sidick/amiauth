/* test_qr.c — QR decode of the portable wrapper (src/qr/qr.c over quirc).
 *
 * Uses checked-in greyscale fixtures (tests/qr/sample_gray.h) rendered from
 * known QR codes, so the decoder is exercised end-to-end with zero platform
 * dependencies — the same float path that ships on m68k. */
#include <string.h>

#include "test.h"
#include "qr.h"
#include "qrencode.h"
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

/* Rasterize a qr_encode_uri() module grid to 8-bit greyscale (module=black,
 * background=white), `scale` pixels per module, no extra margin -- qr.c's own
 * decode_with_quiet_zone() fallback pads it, so this doesn't need to. */
static void rasterize(const uint8_t *modules, int size, int scale,
                      unsigned char *gray, int gw)
{
    int mx, my, py, px;
    for (my = 0; my < size; my++) {
        for (mx = 0; mx < size; mx++) {
            unsigned char v = modules[my * size + mx] ? 0 : 255;
            for (py = 0; py < scale; py++)
                for (px = 0; px < scale; px++)
                    gray[(my * scale + py) * gw + (mx * scale + px)] = v;
        }
    }
}

/* #45 (QR export): qr_encode_uri() round-tripped through the *real* quirc
 * decoder (qr_decode_gray) -- strong end-to-end confidence that whatever this
 * project renders on screen is actually scannable, not just "some bits". */
void run_qr_encode_tests(void)
{
    static uint8_t modules[QR_ENC_MAX_MODULES * QR_ENC_MAX_MODULES];
    /* scale=4, max grid 77*4=308px square: comfortably under any reasonable
     * test buffer; kept static/off-stack like every other large buffer here. */
    static unsigned char gray[QR_ENC_MAX_MODULES * 4 * QR_ENC_MAX_MODULES * 4];
    int size;
    char uri[512];
    otp_account a, b;
    char huge_issuer[OTP_MAX_ISSUER], huge_label[OTP_MAX_LABEL];
    size_t i;

    /* Simple TOTP account, built to a URI, encoded, rasterized, and decoded
     * back by the real quirc path -- must come back byte-for-byte. */
    TEST_CHECK(otpauth_parse(
        "otpauth://totp/GitHub:alice?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"
        "&digits=6&period=30", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(qr_encode_uri(uri, modules, &size) == QRENC_OK);
    TEST_CHECK(size > 0 && size <= QR_ENC_MAX_MODULES);
    rasterize(modules, size, 4, gray, size * 4);
    {
        char decoded[512];
        TEST_CHECK(qr_decode_gray(gray, size * 4, size * 4,
                                  decoded, sizeof(decoded)) == QR_OK);
        TEST_CHECK(strcmp(decoded, uri) == 0);
        TEST_CHECK(otpauth_parse(decoded, &b) == 0);
        TEST_CHECK(strcmp(a.issuer, b.issuer) == 0 && strcmp(a.label, b.label) == 0);
        TEST_CHECK(a.secret_len == b.secret_len &&
                   memcmp(a.secret, b.secret, a.secret_len) == 0);
    }

    /* Steam Guard account (no algorithm/digits/period in the URI) round-trips
     * through the same path. */
    TEST_CHECK(otpauth_parse(
        "otpauth://steam/Steam:you?secret=JBSWY3DPEHPK3PXP&issuer=Steam", &a) == 0);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(qr_encode_uri(uri, modules, &size) == QRENC_OK);
    rasterize(modules, size, 4, gray, size * 4);
    {
        char decoded[512];
        TEST_CHECK(qr_decode_gray(gray, size * 4, size * 4,
                                  decoded, sizeof(decoded)) == QR_OK);
        TEST_CHECK(strcmp(decoded, uri) == 0);
    }

    /* A generously long but realistic account (full OTP_MAX_SECRET=64 bytes,
     * as a SHA-512 secret plausibly is; a longer-than-usual issuer/label)
     * still fits within the version cap. */
    strcpy(huge_issuer, "A Reasonably Long Issuer Name");
    strcpy(huge_label, "someone.with.a-long.email-address@example.co.uk");
    memset(&a, 0, sizeof(a));
    strcpy(a.type, "totp");
    strcpy(a.algorithm, "SHA512");
    strcpy(a.issuer, huge_issuer);
    strcpy(a.label, huge_label);
    a.secret_len = OTP_MAX_SECRET;
    for (i = 0; i < a.secret_len; i++) a.secret[i] = (uint8_t)i;
    a.digits = 8;
    a.period = 30;
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(qr_encode_uri(uri, modules, &size) == QRENC_OK);
    TEST_CHECK(size > 0 && size <= QR_ENC_MAX_MODULES);

    /* The true pathological case -- OTP_MAX_ISSUER, OTP_MAX_LABEL and
     * OTP_MAX_SECRET all maxed out simultaneously -- genuinely doesn't fit
     * even at the version cap. That's fine: it's not a realistic account
     * (see the case above for a generous-but-real one), and the point of
     * this test is that qr_encode_uri() fails *cleanly* rather than
     * overflowing or producing a bad code. */
    for (i = 0; i + 1 < sizeof(huge_issuer); i++) huge_issuer[i] = 'A' + (char)(i % 26);
    huge_issuer[sizeof(huge_issuer) - 1] = '\0';
    for (i = 0; i + 1 < sizeof(huge_label); i++) huge_label[i] = 'a' + (char)(i % 26);
    huge_label[sizeof(huge_label) - 1] = '\0';
    strcpy(a.issuer, huge_issuer);
    strcpy(a.label, huge_label);
    TEST_CHECK(otpauth_build(&a, uri, sizeof(uri)) == 0);
    TEST_CHECK(qr_encode_uri(uri, modules, &size) == QRENC_ERR_TOOLONG);

    /* --- argument guards --- */
    TEST_CHECK(qr_encode_uri(NULL, modules, &size) == QRENC_ERR_ARGS);
    TEST_CHECK(qr_encode_uri(uri, NULL, &size) == QRENC_ERR_ARGS);
    TEST_CHECK(qr_encode_uri(uri, modules, NULL) == QRENC_ERR_ARGS);
}
