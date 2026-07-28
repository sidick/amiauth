/* arexxtest.c — on-target (m68k/AmigaOS) relay for the ARexx port test (#46).
 *
 * The actual probe of AmiAuthGUI's AMIAUTH.<n> port is a real ARexx script
 * (arexx-probe.rexx) run by the resident RexxMast under `rx`, redirected to
 * a RAM: file (see tests/gui/arexx-onhw.sh) — a genuine ARexx interpreter
 * task is what's needed here, not a hand-rolled one: rexxsyslib.library's
 * IsRexxMsg()/CHECKREXXMSG() validate rm_TaskBlock, which only a message
 * built from within a live ARexx task's own context ever has populated
 * (confirmed empirically — CreateRexxMsg() called from a plain external C
 * program, even with every documented field set correctly, produces a
 * message IsRexxMsg() rejects; this is by design, not a bug in arexx.c).
 *
 * Copperline's [ide] host-directory mount is an in-memory snapshot (guest
 * writes never reach the host, see the copperline-testing skill), so the
 * RAM: result file must be relayed out over serial *before* the emulator
 * exits. This program does exactly that: read the file, emit its bytes via
 * exec/RawPutChar (the same ROM debug path serialtest.c uses — no
 * serial.device handler or Mount needed), wrapped in BEGIN/END markers.
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>

static void raw_put(char c)
{
    void *SysBase = *(void **)4UL;
    register long d0 __asm__("d0") = (unsigned char)c;
    register void *a6 __asm__("a6") = SysBase;
    __asm__ volatile("jsr -516(%%a6)" : : "r"(d0), "r"(a6)
                     : "d1", "a0", "a1", "cc", "memory");
}

static void raw_str(const char *s) { while (*s) raw_put(*s++); }

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "RAM:arexx-result.txt";
    BPTR fh;
    char buf[256];
    LONG n;

    raw_str("BEGIN\r\n");

    fh = Open((STRPTR)path, MODE_OLDFILE);
    if (!fh) { raw_str("NOFILE\r\n"); raw_str("END\r\n"); return 1; }

    while ((n = Read(fh, buf, sizeof buf)) > 0) {
        LONG i;
        for (i = 0; i < n; i++) raw_put(buf[i]);
    }
    Close(fh);

    raw_str("END\r\n");
    return 0;
}
