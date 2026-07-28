/* asmbench.c — compare the C reference vs. the hand-written SHA-1 asm (#47)
 * via PBKDF2-HMAC-SHA1, on a real 68000 (under Copperline's A500) using
 * timer.device EClock. Times both paths back-to-back in one run so both
 * numbers come from the same boot/hardware pass. Dev tool only, not shipped
 * (mirrors pbkdf2bench.c's own scope note).
 *
 * ChaCha20 has no asm path to compare - a hand-written attempt measured
 * ~17% slower than its C reference here, so it's not part of this
 * benchmark; see src/core/crypto_dispatch.h. */
#include <exec/types.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include "pbkdf2.h"
#include "crypto_dispatch.h"

struct Device *TimerBase;

extern void sha1_compress_asm(uint32_t state[5], const uint8_t block[SHA1_BLOCK_SIZE]);

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

static ULONG g_freq;

static ULONG ticks_since(struct EClockVal *t0)
{
    struct EClockVal t1;
    ReadEClock(&t1);
    return t1.ev_lo - t0->ev_lo;   /* assumes ev_hi unchanged over the window */
}

int main(void)
{
    struct MsgPort *port;
    struct timerequest *tr;
    struct EClockVal t0;
    const uint8_t pass[8] = { 'p','a','s','s','w','o','r','d' };
    const uint8_t salt[16] = { 0 };
    uint8_t dk[64];
    const uint32_t PBKDF2_N = 600;             /* same as pbkdf2bench.c */

    raw_str("BEGIN\r\n");
    port = CreateMsgPort();
    tr = (struct timerequest *)CreateIORequest(port, sizeof *tr);
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0) != 0) {
        raw_str("ERR: no timer.device\r\n");
        return 1;
    }
    TimerBase = tr->tr_node.io_Device;
    g_freq = ReadEClock(&t0);

    /* --- PBKDF2-HMAC-SHA1 (dominated by sha1_compress calls) --- */
    g_sha1_compress = sha1_compress_c;
    ReadEClock(&t0);
    pbkdf2_hmac_sha1(pass, sizeof pass, salt, sizeof salt, PBKDF2_N, dk, sizeof dk);
    raw_str("PBKDF2 impl=c   iters=");   raw_u32(PBKDF2_N);
    raw_str(" ticks=");                  raw_u32(ticks_since(&t0));
    raw_str(" freq=");                   raw_u32(g_freq);
    raw_str("\r\n");

    g_sha1_compress = sha1_compress_asm;
    ReadEClock(&t0);
    pbkdf2_hmac_sha1(pass, sizeof pass, salt, sizeof salt, PBKDF2_N, dk, sizeof dk);
    raw_str("PBKDF2 impl=asm iters=");   raw_u32(PBKDF2_N);
    raw_str(" ticks=");                  raw_u32(ticks_since(&t0));
    raw_str(" freq=");                   raw_u32(g_freq);
    raw_str("\r\n");

    raw_str("END\r\n");
    CloseDevice((struct IORequest *)tr);
    return 0;
}
