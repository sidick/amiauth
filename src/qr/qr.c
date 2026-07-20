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

/* One decode attempt over a greyscale buffer - no quiet-zone handling, just
 * quirc itself. Factored out so qr_decode_gray() can retry once on a padded
 * copy without duplicating the quirc setup/extract/decode loop. */
static int decode_once(const unsigned char *gray, int w, int h,
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

/* QR codes need a "quiet zone" (blank margin) around them to be reliably
 * found (ISO/IEC 18004 calls for >= 4 modules); some export tools crop the
 * image tight to the code with none at all, which quirc - unlike more
 * tolerant camera-scanner decoders - won't find without help. A fixed 40px
 * white border comfortably covers that for realistic QR-enrolment-image
 * sizes without the retry buffer's memory cost scaling with the original
 * image (bounded regardless of how large `gray` already is, up to
 * qrimage.c's existing 2048x2048/2MP load cap). */
#define QUIET_ZONE_PAD 40

static int decode_with_quiet_zone(const unsigned char *gray, int w, int h,
                                  char *uri, size_t cap)
{
    int pw = w + 2 * QUIET_ZONE_PAD, ph = h + 2 * QUIET_ZONE_PAD;
    unsigned char *padded = malloc((size_t)pw * (size_t)ph);
    int result, y;

    if (!padded)
        return QR_ERR_NOMEM;
    memset(padded, 255, (size_t)pw * (size_t)ph);
    for (y = 0; y < h; y++)
        memcpy(padded + (size_t)(y + QUIET_ZONE_PAD) * pw + QUIET_ZONE_PAD,
               gray + (size_t)y * w, (size_t)w);

    result = decode_once(padded, pw, ph, uri, cap);
    free(padded);
    return result;
}

int qr_decode_gray(const unsigned char *gray, int w, int h,
                   char *uri, size_t cap)
{
    int result;

    if (cap > 0)
        uri[0] = '\0';
    if (!gray || !uri || cap == 0 || w <= 0 || h <= 0)
        return QR_ERR_ARGS;

    result = decode_once(gray, w, h, uri, cap);
    if (result == QR_ERR_NOCODE)
        result = decode_with_quiet_zone(gray, w, h, uri, cap);
    return result;
}
