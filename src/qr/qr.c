/* qr.c -- decode the first otpauth:// QR code in a greyscale image.
 *
 * Wraps the vendored quirc decoder (src/qr/quirc.h, ISC). The whole quirc API
 * surface is confined to this file so the rest of AmiAuth only sees
 * qr_decode_gray(). Portable C: builds and is host-tested identically to the
 * m68k build (which additionally defines -DQUIRC_FLOAT_TYPE=float, no FPU).
 */
#include <stdlib.h>
#include <string.h>

#include "qr.h"
#include "quirc.h"

/* Is `s` (length `len`) an otpauth:// URI? Scheme match is case-insensitive
 * per RFC 3986; the rest is left for otpauth_parse() to validate. */
static int is_otpauth(const unsigned char *s, int len)
{
    static const char scheme[] = "otpauth://";
    int i;

    if (len < (int)(sizeof scheme - 1))
        return 0;
    for (i = 0; scheme[i]; i++) {
        int c = s[i];
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        if (c != scheme[i])
            return 0;
    }
    return 1;
}

int qr_decode_gray(const unsigned char *gray, int w, int h,
                   char *uri, size_t cap)
{
    struct quirc *q;
    unsigned char *ibuf;
    int iw, ih, count, i;
    int result = QR_ERR_NOCODE;
    /* quirc_code (~3.9 KB) and quirc_data (~8.9 KB) are large; keep them off the
     * stack (the Amiga shell hands a program only a few KB). quirc allocates its
     * own working struct on the heap; quirc_decode still uses a ~9 KB datastream
     * on the stack internally, which the caller covers with a raised stack. */
    struct quirc_code *code;
    struct quirc_data *data;

    if (cap > 0)
        uri[0] = '\0';
    if (!gray || !uri || cap == 0 || w <= 0 || h <= 0)
        return QR_ERR_ARGS;

    q = quirc_new();
    if (!q)
        return QR_ERR_NOMEM;
    code = malloc(sizeof *code);
    data = malloc(sizeof *data);
    if (!code || !data || quirc_resize(q, w, h) < 0) {
        free(code);
        free(data);
        quirc_destroy(q);
        return QR_ERR_NOMEM;
    }

    /* Hand quirc the greyscale pixels (it owns/thresholds its own buffer). */
    ibuf = quirc_begin(q, &iw, &ih);
    memcpy(ibuf, gray, (size_t)iw * (size_t)ih);
    quirc_end(q);

    count = quirc_count(q);
    for (i = 0; i < count; i++) {
        quirc_extract(q, i, code);

        /* Try as-is; on an ECC failure retry mirrored (ISO 18004:2015). */
        if (quirc_decode(code, data) != QUIRC_SUCCESS) {
            quirc_flip(code);
            if (quirc_decode(code, data) != QUIRC_SUCCESS)
                continue;
        }

        if (is_otpauth(data->payload, data->payload_len)) {
            int n = data->payload_len;
            if (n > (int)cap - 1)
                n = (int)cap - 1;
            memcpy(uri, data->payload, (size_t)n);
            uri[n] = '\0';
            result = QR_OK;
            break;
        }
        /* Decoded, but not an otpauth URI: remember, keep looking. */
        result = QR_ERR_NOTOTP;
    }

    free(code);
    free(data);
    quirc_destroy(q);
    return result;
}
