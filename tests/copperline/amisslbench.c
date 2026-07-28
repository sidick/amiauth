/* amisslbench.c — compare AmiSSL's PKCS5_PBKDF2_HMAC_SHA1 against our own
 * pbkdf2_hmac_sha1 on the same boot, same CPU, same iteration count. Dev-only
 * groundwork for issue #85 ("optional AmiSSL crypto provider") — this is not
 * shipped, and AmiAuth stays zero-dependency at runtime regardless of what
 * this benchmark finds. See tests/copperline/amissl-bench.sh.
 *
 * AmiSSL init mirrors the (in-production) pattern in
 * micropython/ports/amiga/amiga_ssl.c: OpenLibrary("amisslmaster.library"),
 * then OpenAmiSSLTagList. AmiSSL_SocketBase is omitted deliberately — the
 * SDK's own Autodoc (amissl.doc/InitAmiSSLA) says it "can be omitted" when
 * the caller "doesn't need any networking functionality" (defaults to NULL),
 * which is our case: PBKDF2 never touches the network.
 *
 * Timing follows pbkdf2bench.c exactly (timer.device EClock, same params,
 * same RawPutChar serial output) so the two numbers are directly comparable. */
#include <exec/types.h>
#include <devices/timer.h>
#include <libraries/amisslmaster.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/amisslmaster.h>
#include <amissl/tags.h>
#include <openssl/evp.h>

#include "pbkdf2.h"

struct Device *TimerBase;
struct Library *AmiSSLMasterBase;
struct Library *AmiSSLBase;
struct Library *AmiSSLExtBase;

/* --- serial output via exec RawPutChar (LVO -516) --- */
static void raw_put(char c)
{
    void *SysBase = *(void **)4UL;
    register long d0 __asm__("d0") = (unsigned char)c;
    register void *a6 __asm__("a6") = SysBase;
    __asm__ volatile("jsr -516(%%a6)" : : "r"(d0), "r"(a6)
                     : "d1", "a0", "a1", "cc", "memory");
}
static void raw_str(const char *s) { while (*s) raw_put(*s++); }
static void raw_u32(uint32_t v)
{
    char b[12]; int n = 0;
    if (!v) b[n++] = '0';
    while (v) { b[n++] = (char)('0' + v % 10); v /= 10; }
    while (n) raw_put(b[--n]);
}

/* Same inputs pbkdf2bench.c uses, so the two runs are apples-to-apples. */
static const uint8_t pass[8] = { 'p','a','s','s','w','o','r','d' };
static const uint8_t salt[16] = { 0 };
static const uint32_t N = 600;

static void report(const char *label, uint32_t iters, uint32_t ticks, uint32_t freq)
{
    raw_str(label);   raw_str(" iters=");   raw_u32(iters);
    raw_str(" ticks=");                     raw_u32(ticks);
    raw_str(" freq=");                      raw_u32(freq);
    raw_str("\r\n");
}

int main(void)
{
    struct MsgPort *port;
    struct timerequest *tr;
    struct EClockVal t0, t1;
    ULONG freq;
    uint8_t dk[64];
    LONG rc;

    raw_str("BEGIN\r\n");

    port = CreateMsgPort();
    tr = (struct timerequest *)CreateIORequest(port, sizeof *tr);
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0) != 0) {
        raw_str("ERR: no timer.device\r\n");
        return 1;
    }
    TimerBase = tr->tr_node.io_Device;

    /* --- our own implementation --- */
    freq = ReadEClock(&t0);
    pbkdf2_hmac_sha1(pass, sizeof pass, salt, sizeof salt, N, dk, sizeof dk);
    ReadEClock(&t1);
    report("BUILTIN", N, t1.ev_lo - t0.ev_lo, freq);

    /* --- AmiSSL --- */
    AmiSSLMasterBase = OpenLibrary((STRPTR)"amisslmaster.library", 5);
    if (AmiSSLMasterBase == NULL) {
        raw_str("ERR: no amisslmaster.library\r\n");
        goto done;
    }

    rc = OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
        AmiSSL_UsesOpenSSLStructs, FALSE,
        AmiSSL_GetAmiSSLBase,      (ULONG)&AmiSSLBase,
        AmiSSL_GetAmiSSLExtBase,   (ULONG)&AmiSSLExtBase,
        TAG_DONE);
    if (rc != 0 || AmiSSLBase == NULL) {
        raw_str("ERR: OpenAmiSSLTags failed rc="); raw_u32((uint32_t)rc); raw_str("\r\n");
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
        goto done;
    }

    freq = ReadEClock(&t0);
    PKCS5_PBKDF2_HMAC_SHA1((const char *)pass, sizeof pass, salt, sizeof salt,
        (int)N, sizeof dk, dk);
    ReadEClock(&t1);
    report("AMISSL", N, t1.ev_lo - t0.ev_lo, freq);

    CloseAmiSSL();
    AmiSSLBase = NULL;
    AmiSSLExtBase = NULL;
    CloseLibrary(AmiSSLMasterBase);
    AmiSSLMasterBase = NULL;

done:
    raw_str("END\r\n");
    CloseDevice((struct IORequest *)tr);
    return 0;
}
