/* pbkdf2bench.c — measure PBKDF2-HMAC-SHA1 throughput on a real 68000 (under
 * Copperline's A500) to set the vault KDF iteration cap. Times a fixed number
 * of iterations (dkLen=64, as the vault uses) with timer.device EClock and
 * emits the result over serial via RawPutChar. Linked only for this benchmark. */
#include <exec/types.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include "pbkdf2.h"

struct Device *TimerBase;

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

int main(void)
{
    struct MsgPort *port;
    struct timerequest *tr;
    struct EClockVal t0, t1;
    ULONG freq;
    const uint8_t pass[8] = { 'p','a','s','s','w','o','r','d' };
    const uint8_t salt[16] = { 0 };
    uint8_t dk[64];
    const uint32_t N = 600;           /* iterations to time */

    raw_str("BEGIN\r\n");
    port = CreateMsgPort();
    tr = (struct timerequest *)CreateIORequest(port, sizeof *tr);
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0) != 0) {
        raw_str("ERR: no timer.device\r\n");
        return 1;
    }
    TimerBase = tr->tr_node.io_Device;

    freq = ReadEClock(&t0);
    pbkdf2_hmac_sha1(pass, sizeof pass, salt, sizeof salt, N, dk, sizeof dk);
    ReadEClock(&t1);

    /* Assume ev_hi unchanged over a ~1-2 s window (delta < 2^32 ticks). */
    raw_str("PBKDF2 iters=");   raw_u32(N);
    raw_str(" ticks=");         raw_u32(t1.ev_lo - t0.ev_lo);
    raw_str(" freq=");          raw_u32(freq);
    raw_str("\r\nEND\r\n");

    CloseDevice((struct IORequest *)tr);
    return 0;
}
