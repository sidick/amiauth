/* qr_onhw.c — on-target (m68k/AmigaOS) end-to-end test of QR-image import.
 *
 * Loads a staged image file via datatypes.library (src/amiga/qrimage.c) and
 * decodes it with the portable quirc wrapper (src/qr), emitting the result over
 * serial via exec/RawPutChar for the Copperline harness to check. This exercises
 * the datatypes glue against the *real* picture.datatype and the decoder on real
 * m68k — the risk the host tests can't cover. Needs a full WB boot (datatypes
 * present). The decode is slow on the FPU-less CPU (soft-float geometry), so the
 * harness allows a generous run time.
 */
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>

#include "qr.h"
#include "qrimage.h"

/* Larger stack (libnix): quirc's decode uses ~10 KB (heaped structs + its
 * internal datastream), over the shell default. Matches the GUI. */
unsigned long __stack = 32768;

/* Library bases referenced by qrimage.c; defined here for this standalone
 * harness (the GUI supplies its own). */
struct Library *DataTypesBase = NULL;
struct GfxBase *GfxBase        = NULL;

/* exec RawPutChar: char in d0, SysBase (absolute 4) in a6, LVO -516. */
static void raw_put(char c)
{
    void *SysBase = *(void **)4UL;
    register long d0 __asm__("d0") = (unsigned char)c;
    register void *a6 __asm__("a6") = SysBase;
    __asm__ volatile("jsr -516(%%a6)" : : "r"(d0), "r"(a6)
                     : "d1", "a0", "a1", "cc", "memory");
}

static void raw_str(const char *s) { while (*s) raw_put(*s++); }

static void raw_int(long v)
{
    char buf[12];
    int n = 0;
    unsigned long u;
    if (v < 0) { raw_put('-'); u = (unsigned long)(-v); }
    else u = (unsigned long)v;
    if (u == 0) buf[n++] = '0';
    while (u) { buf[n++] = (char)('0' + (u % 10)); u /= 10; }
    while (n) raw_put(buf[--n]);
}

int main(void)
{
    const char *path = "SYS:qr-sample.png";
    unsigned char *gray = NULL;
    int w = 0, h = 0, lr, dr;
    static char uri[300];

    DataTypesBase = OpenLibrary((STRPTR)"datatypes.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 39);

    raw_str("DATATYPES="); raw_str(DataTypesBase ? "yes" : "no"); raw_put('\n');

    lr = qrimage_load_gray(path, &gray, &w, &h);
    raw_str("LOAD="); raw_int(lr);
    raw_str(" W="); raw_int(w); raw_str(" H="); raw_int(h); raw_put('\n');

    if (lr == QRIMAGE_OK) {
        dr = qr_decode_gray(gray, w, h, uri, sizeof uri);
        raw_str("DEC="); raw_int(dr); raw_put('\n');
        raw_str("URI="); raw_str(dr == QR_OK ? uri : "(none)"); raw_put('\n');
        qrimage_free(gray);
    }
    raw_str("END\n");

    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    return 0;
}
