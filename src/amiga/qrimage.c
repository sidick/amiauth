/* qrimage.c — load an image file to 8-bit greyscale via datatypes.library.
 *
 * AmigaOS-only glue for QR import; the portable decode is in src/qr. The whole
 * feature is optional: every entry point tolerates datatypes.library being
 * absent (returns QRIMAGE_ERR_UNAVAIL) so the GUI can simply not offer it.
 *
 * Two loaders:
 *   - primary: the clean v43 path — PDTA_SourceMode=PMODE_V43 +
 *     PDTM_READPIXELARRAY/PBPAFMT_GREY8 hands back greyscale directly. Needs
 *     picture.datatype v43+ (WB 3.1.4 / 3.2 / 3.5+).
 *   - fallback: the classic v39 path — read the picture's own bitmap with
 *     ReadPixelArray8 and convert pen -> luminance from its palette. Works on
 *     stock WB 3.0/3.1 (picture.datatype v39/40).
 * The fallback runs whenever the v43 method is unavailable; -DQRIMAGE_FORCE_V39
 * forces it, so the fallback can be exercised on a modern (v45) test system.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>          /* Object, Msg */
#include <graphics/gfx.h>                 /* struct BitMap, struct RastPort */
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <proto/exec.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>

#include "qrimage.h"

extern struct Library *DataTypesBase;    /* opened best-effort by the GUI */
extern struct GfxBase *GfxBase;          /* graphics.library (v39+ on OS 3.0) */

/* Guardrails: refuse absurd images rather than exhaust memory on a small
 * machine (quirc needs roughly another w*h bytes on top of our buffer). */
#define QRIMAGE_MAX_DIM   2048
#define QRIMAGE_MAX_AREA  (2UL * 1024 * 1024)   /* 2 megapixels */

/* Reject a picture whose dimensions are unusable or over the cap. */
static int dims_ok(ULONG w, ULONG h)
{
    return w && h && w <= QRIMAGE_MAX_DIM && h <= QRIMAGE_MAX_DIM &&
           w * h <= QRIMAGE_MAX_AREA;
}

/* Primary: picture.datatype v43 GREY8. Returns QRIMAGE_OK, or QRIMAGE_ERR_READ
 * if the v43 method isn't available (caller then tries the classic path). */
#ifndef QRIMAGE_FORCE_V39
static int load_v43(const char *path, unsigned char **gray, int *w, int *h)
{
    Object *o;
    struct BitMapHeader *bmhd = NULL;
    struct pdtBlitPixelArray pa;
    unsigned char *buf;
    ULONG width, height, pitch, y;

    o = NewDTObject((APTR)path,
        DTA_SourceType,  DTST_FILE,
        DTA_GroupID,     GID_PICTURE,
        PDTA_SourceMode, PMODE_V43,       /* enable PDTM_READPIXELARRAY */
        TAG_END);
    if (!o)
        return QRIMAGE_ERR_LOAD;

    GetDTAttrs(o, PDTA_BitMapHeader, (ULONG)&bmhd, TAG_END);
    if (!bmhd) { DisposeDTObject(o); return QRIMAGE_ERR_READ; }
    width = bmhd->bmh_Width; height = bmhd->bmh_Height;
    if (!dims_ok(width, height)) { DisposeDTObject(o); return QRIMAGE_ERR_TOOBIG; }

    /* picture.datatype writes GREY8 rows at a 16-pixel-aligned stride, not the
     * bare width (it ignores a smaller PixelArrayMod), which would shear the
     * image. Read into a padded buffer at that pitch, then compact to a dense
     * width-wide array for the decoder. */
    pitch = (width + 15u) & ~15u;
    buf = AllocVec(pitch * height, MEMF_ANY);
    if (!buf) { DisposeDTObject(o); return QRIMAGE_ERR_NOMEM; }

    pa.MethodID           = PDTM_READPIXELARRAY;
    pa.pbpa_PixelData     = buf;
    pa.pbpa_PixelFormat   = PBPAFMT_GREY8;
    pa.pbpa_PixelArrayMod = pitch;        /* 16-px-aligned stride (1 byte/pixel) */
    pa.pbpa_Left = 0; pa.pbpa_Top = 0;
    pa.pbpa_Width = width; pa.pbpa_Height = height;

    if (!DoDTMethodA(o, NULL, NULL, (Msg)&pa)) {
        FreeVec(buf);                     /* pre-v43 datatype: fall back */
        DisposeDTObject(o);
        return QRIMAGE_ERR_READ;
    }
    DisposeDTObject(o);

    /* Compact padded rows down to a dense width stride, in place (dest < src). */
    if (pitch != width)
        for (y = 1; y < height; y++)
            CopyMem(buf + y * pitch, buf + y * width, width);

    *gray = buf; *w = (int)width; *h = (int)height;
    return QRIMAGE_OK;
}
#endif /* !QRIMAGE_FORCE_V39 */

/* Fallback: read the picture's own bitmap (v39+) and convert to greyscale via
 * its palette. Works on stock WB 3.0/3.1. */
static int load_v39(const char *path, unsigned char **gray, int *w, int *h)
{
    Object *o;
    struct BitMapHeader *bmhd = NULL;
    struct BitMap *bm = NULL;
    struct ColorRegister *cregs = NULL;
    struct RastPort rp, trp;
    struct BitMap *tbm;
    unsigned char *buf;
    ULONG width, height, pitch, i, n, y;

    if (!GfxBase || GfxBase->LibNode.lib_Version < 39)
        return QRIMAGE_ERR_READ;          /* AllocBitMap/ReadPixelArray8 are v39 */

    o = NewDTObject((APTR)path,
        DTA_SourceType, DTST_FILE,
        DTA_GroupID,    GID_PICTURE,
        PDTA_Remap,     FALSE,            /* keep the picture's own bitmap/palette */
        TAG_END);
    if (!o)
        return QRIMAGE_ERR_LOAD;

    /* Force the class to build its bitmap (no screen: Remap is off). */
    DoDTMethod(o, NULL, NULL, DTM_PROCLAYOUT, NULL, TRUE);
    GetDTAttrs(o,
        PDTA_BitMapHeader,   (ULONG)&bmhd,
        PDTA_BitMap,         (ULONG)&bm,
        PDTA_ColorRegisters, (ULONG)&cregs,
        TAG_END);
    if (!bmhd || !bm || !cregs) { DisposeDTObject(o); return QRIMAGE_ERR_READ; }
    width = bmhd->bmh_Width; height = bmhd->bmh_Height;
    if (!dims_ok(width, height)) { DisposeDTObject(o); return QRIMAGE_ERR_TOOBIG; }

    /* ReadPixelArray8 writes its pen rows at a 16-pixel-aligned stride (matching
     * its scratch raster), not the bare width — so allocate/read at that pitch
     * and compact afterwards, else the image is sheared (and the buffer would
     * overflow). */
    pitch = (width + 15u) & ~15u;
    buf = AllocVec(pitch * height, MEMF_ANY);
    if (!buf) { DisposeDTObject(o); return QRIMAGE_ERR_NOMEM; }

    /* Read pen indices into buf via a 1-row scratch raster (pitch-wide). */
    InitRastPort(&rp); rp.BitMap = bm;
    tbm = AllocBitMap(pitch, 1, bm->Depth, 0, bm);
    if (!tbm) { FreeVec(buf); DisposeDTObject(o); return QRIMAGE_ERR_NOMEM; }
    trp = rp; trp.Layer = NULL; trp.BitMap = tbm;

    ReadPixelArray8(&rp, 0, 0, width - 1, height - 1, (UBYTE *)buf, &trp);
    FreeBitMap(tbm);

    /* Compact pen rows from the aligned pitch down to a dense width stride. */
    if (pitch != width)
        for (y = 1; y < height; y++)
            CopyMem(buf + y * pitch, buf + y * width, width);

    /* Map pen indices to greyscale luminance over the dense width*height area. */
    n = width * height;
    for (i = 0; i < n; i++) {
        const struct ColorRegister *c = &cregs[buf[i]];
        buf[i] = (unsigned char)((c->red * 77 + c->green * 150 + c->blue * 29) >> 8);
    }

    DisposeDTObject(o);
    *gray = buf; *w = (int)width; *h = (int)height;
    return QRIMAGE_OK;
}

int qrimage_load_gray(const char *path, unsigned char **gray, int *w, int *h)
{
    int rc;

    *gray = NULL; *w = 0; *h = 0;
    if (!DataTypesBase || !path)
        return QRIMAGE_ERR_UNAVAIL;

#ifndef QRIMAGE_FORCE_V39
    rc = load_v43(path, gray, w, h);
    if (rc != QRIMAGE_ERR_READ)           /* only the "no v43 method" case falls back */
        return rc;
#endif
    rc = load_v39(path, gray, w, h);
    return rc;
}

void qrimage_free(unsigned char *gray)
{
    if (gray) FreeVec(gray);
}
