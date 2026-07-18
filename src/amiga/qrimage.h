/* qrimage.h — load an image file to 8-bit greyscale for QR decoding.
 *
 * AmigaOS-only glue over datatypes.library. The whole QR-import feature is
 * OPTIONAL: if datatypes.library isn't available the loader returns
 * QRIMAGE_ERR_UNAVAIL and the GUI simply doesn't offer QR import. See qr.h
 * (portable decoder) for the next step.
 */
#ifndef AMIAUTH_QRIMAGE_H
#define AMIAUTH_QRIMAGE_H

enum {
    QRIMAGE_OK          =  0,
    QRIMAGE_ERR_UNAVAIL = -1,  /* datatypes.library not open, or no path      */
    QRIMAGE_ERR_LOAD    = -2,  /* datatypes couldn't open the file as a picture */
    QRIMAGE_ERR_TOOBIG  = -3,  /* dimensions beyond the sane cap               */
    QRIMAGE_ERR_NOMEM   = -4,  /* buffer allocation failed                     */
    QRIMAGE_ERR_READ    = -5   /* picture.datatype too old for GREY8 read      */
};

/* Load `path` into a freshly-allocated 8-bit greyscale buffer (one byte per
 * pixel, row-major, 0=black..255=white). On success returns 0, sets *gray plus
 * the width/height, and the caller must release it with qrimage_free(). On error
 * returns a
 * negative QRIMAGE_ERR_* and leaves *gray NULL. Safe to call unconditionally —
 * returns QRIMAGE_ERR_UNAVAIL when datatypes.library is absent. */
int  qrimage_load_gray(const char *path, unsigned char **gray, int *w, int *h);
void qrimage_free(unsigned char *gray);

#endif /* AMIAUTH_QRIMAGE_H */
