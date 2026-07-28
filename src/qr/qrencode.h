/* qrencode.h -- encode an otpauth:// URI as a QR code (#45, export/display).
 *
 * Portable C wrapper over the vendored qrcodegen library (src/qr/qrcodegen.h,
 * MIT); see THIRDPARTY.md. Mirrors qr.h's decode-side shape: the vendored
 * library's own types never leak past this header.
 */
#ifndef AMIAUTH_QRENCODE_H
#define AMIAUTH_QRENCODE_H

#include <stddef.h>
#include <stdint.h>

/* Result codes for qr_encode_uri(). */
enum {
    QRENC_OK          =  0,
    QRENC_ERR_ARGS    = -1,   /* NULL/zero-sized arguments        */
    QRENC_ERR_TOOLONG = -2    /* uri doesn't fit even at the version cap */
};

/* Version cap: higher versions pack more data into a finer module grid, which
 * scans worse at typical Amiga-screen viewing distance/resolution and needs a
 * bigger output buffer. 15 (77x77 modules) comfortably fits any realistic
 * otpauth:// URI (secret up to OTP_MAX_SECRET=64 raw bytes, i.e. ~103 Base32
 * chars, plus a percent-encoded issuer/label) while staying easily scannable. */
#define QR_ENC_MAX_VERSION 15
#define QR_ENC_MAX_MODULES (QR_ENC_MAX_VERSION * 4 + 17)   /* 77 */

/* Encode `uri` (a NUL-terminated otpauth:// URI, e.g. from otpauth_build())
 * as a QR code. On success, fills modules[0, *size * *size) — row-major, one
 * byte per module (1 = dark, 0 = light) — and sets *size to the code's side
 * length. `modules` must have room for QR_ENC_MAX_MODULES*QR_ENC_MAX_MODULES
 * bytes regardless of the actual *size returned (~5.9 KB; keep it off the
 * Amiga shell's small stack, e.g. `static`, matching this project's other
 * large-buffer locals). Returns QRENC_OK or a negative QRENC_ERR_* code; on
 * error *size is left at 0. */
int qr_encode_uri(const char *uri, uint8_t *modules, int *size);

/* Room for the largest ASCII render qr_render_ascii() can produce: each of
 * up to QR_ENC_MAX_MODULES rows becomes QR_ENC_MAX_MODULES*2 characters plus
 * a newline, plus a NUL. ~11.9 KB — static/heap only, not the Amiga stack. */
#define QR_ENC_ASCII_BUF_LEN \
    (QR_ENC_MAX_MODULES * (QR_ENC_MAX_MODULES * 2 + 1) + 1)

/* Render a qr_encode_uri() module grid as plain-ASCII block art: two
 * characters per module horizontally ("##" dark, "  " light space) to
 * roughly square the aspect ratio on a typical character-cell console, one
 * newline-terminated line per module row. Deliberately not Unicode block
 * characters (U+2580 etc.) — a stock Amiga console's charset (Topaz/ANSI)
 * doesn't have them. Shared by the CLI's QR command and the GUI's AAP_QR
 * forward-request handler, so a resident GUI and a fresh CLI process render
 * identically. Returns the number of bytes written (excluding the NUL), or
 * -1 on NULL/invalid arguments or if `out` (capacity outcap) is too small —
 * QR_ENC_ASCII_BUF_LEN is always enough. */
int qr_render_ascii(const uint8_t *modules, int size, char *out, size_t outcap);

#endif /* AMIAUTH_QRENCODE_H */
