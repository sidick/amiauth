/* qrencode.c -- encode an otpauth:// URI as a QR code (#45).
 *
 * Wraps the vendored qrcodegen library (src/qr/qrcodegen.h, MIT). The whole
 * qrcodegen API surface is confined to this file so the rest of AmiAuth only
 * sees qr_encode_uri(). Portable C, no malloc, no floating point -- builds
 * and is host-tested identically to the m68k build.
 */
#include <string.h>

#include "qrencode.h"
#include "qrcodegen.h"

int qr_encode_uri(const char *uri, uint8_t *modules, int *size)
{
    /* qrcodegen needs two working buffers of this size; ~743 bytes each at
     * the version cap. Static: keep them off the small Amiga shell stack,
     * same reasoning as this project's other large-buffer locals. */
    static uint8_t tempbuf[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_ENC_MAX_VERSION)];
    static uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_ENC_MAX_VERSION)];
    int n, x, y;
    bool ok;

    if (!uri || !modules || !size) return QRENC_ERR_ARGS;
    *size = 0;

    /* A fixed mask, and no boostEcl: qrcodegen_Mask_AUTO tries all 8 masks and
     * penalty-scores each to pick the most scan-reliable one, and boostEcl
     * makes a second pass at a higher ECC level when the chosen version has
     * spare capacity - both are pure quality-of-scan polish, and both cost
     * real time. On real 68k hardware "real time" turned out to be on the
     * order of a minute for a modest-length URI (confirmed on-target via
     * Copperline) - unacceptable for an interactive GUI action. Mask 2 is an
     * arbitrary but fixed, valid-per-spec choice (any of 0-7 is a correct QR
     * code; AUTO only optimises which one scans most reliably). */
    ok = qrcodegen_encodeText(uri, tempbuf, qrcode, qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, QR_ENC_MAX_VERSION, qrcodegen_Mask_2, false);
    if (!ok) return QRENC_ERR_TOOLONG;

    n = qrcodegen_getSize(qrcode);
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++)
            modules[y * n + x] = qrcodegen_getModule(qrcode, x, y) ? 1 : 0;

    *size = n;
    return QRENC_OK;
}

int qr_render_ascii(const uint8_t *modules, int size, char *out, size_t outcap)
{
    size_t pos = 0;
    int y, x;

    if (!modules || !out || size <= 0) return -1;

    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            if (pos + 2 >= outcap) return -1;
            if (modules[y * size + x]) { out[pos++] = '#'; out[pos++] = '#'; }
            else                       { out[pos++] = ' '; out[pos++] = ' '; }
        }
        if (pos + 1 >= outcap) return -1;
        out[pos++] = '\n';
    }
    out[pos] = '\0';
    return (int)pos;
}
