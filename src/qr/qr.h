/* qr.h -- portable QR -> otpauth:// helper (thin wrapper over quirc).
 *
 * Part of AmiAuth. This layer is pure C with no Amiga (or other platform)
 * dependencies, so it builds and is unit-tested on the host exactly as it runs
 * on m68k. The underlying decoder is the vendored `quirc` library (ISC); see
 * src/qr/quirc.h and THIRDPARTY.
 */
#ifndef AMIAUTH_QR_H
#define AMIAUTH_QR_H

#include <stddef.h>

/* Result codes for qr_decode_gray(). */
enum {
    QR_OK          =  0,   /* an otpauth:// code was decoded into uri  */
    QR_ERR_ARGS    = -1,   /* NULL/zero-sized arguments                */
    QR_ERR_NOMEM   = -2,   /* decoder allocation failed                */
    QR_ERR_NOCODE  = -3,   /* no QR code found / none decoded          */
    QR_ERR_NOTOTP  = -4    /* a code decoded, but not an otpauth:// URI */
};

/* Decode the first `otpauth://` QR code found in an 8-bit greyscale image.
 *
 *   gray  : w*h bytes, one byte per pixel, row-major (0 = black, 255 = white;
 *           quirc thresholds internally, so any greyscale ramp is fine).
 *   w, h  : image dimensions in pixels.
 *   uri   : caller buffer receiving the NUL-terminated URI on success.
 *   cap   : size of `uri` in bytes.
 *
 * Returns QR_OK on success, or a negative QR_ERR_* code. On any error `uri`
 * is left as an empty string (when cap > 0).
 */
int qr_decode_gray(const unsigned char *gray, int w, int h,
                   char *uri, size_t cap);

#endif /* AMIAUTH_QR_H */
