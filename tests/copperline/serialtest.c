/* serialtest.c — on-target (m68k/AmigaOS) smoke test for the AmiAuth core.
 *
 * Runs the RFC 4226 Appendix D HOTP vectors (secret "12345678901234567890",
 * counters 0..9) and emits the results over the serial port via exec/RawPutChar
 * — the ROM debug path, which needs no serial.device handler, no Mount, and no
 * Workbench files, so it works in the most minimal HOSTFS boot. Copperline's
 * `--serial stdout` forwards it to the host, where run.sh checks the vectors.
 *
 * This exercises the crypto/OTP core (HMAC-SHA1, big-endian counter packing,
 * dynamic truncation) as actually compiled for the 68000 — the on-target risk
 * the host tests can't cover. Counters are fixed, so no clock is involved. */

#include "otp.h"
#include "drbg.h"

/* exec RawPutChar: char in d0, SysBase (absolute 4) in a6, LVO -516. */
static void raw_put(char c)
{
    void *SysBase = *(void **)4UL;
    register long d0 __asm__("d0") = (unsigned char)c;
    register void *a6 __asm__("a6") = SysBase;
    __asm__ volatile("jsr -516(%%a6)" : : "r"(d0), "r"(a6)
                     : "d1", "a0", "a1", "cc", "memory");
}

static void raw_str(const char *s)
{
    while (*s)
        raw_put(*s++);
}

/* Emit v as decimal, zero-padded to at least `width` digits. */
static void raw_u32(uint32_t v, int width)
{
    char buf[12];
    int n = 0;
    if (v == 0)
        buf[n++] = '0';
    while (v) {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (n < width)
        buf[n++] = '0';
    while (n)
        raw_put(buf[--n]);
}

/* Emit n bytes as lower-case hex. */
static void raw_hex(const uint8_t *p, int n)
{
    static const char H[] = "0123456789abcdef";
    int i;
    for (i = 0; i < n; i++) {
        raw_put(H[p[i] >> 4]);
        raw_put(H[p[i] & 0x0f]);
    }
}

int main(void)
{
    /* RFC 4226 test key: ASCII "12345678901234567890". */
    static const uint8_t key[20] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
    };
    uint64_t counter;

    raw_str("BEGIN\r\n");
    for (counter = 0; counter < 10; counter++) {
        raw_str("HOTP");
        raw_u32((uint32_t)counter, 1);
        raw_put('=');
        raw_u32(hotp_sha1(key, sizeof key, counter, 6), 6);
        raw_str("\r\n");
    }

    /* HMAC-DRBG known-answer on the 68000 (seed = 00 01 .. 1f; first 16 bytes).
     * Same vector as tests/test_drbg.c, so codegen of drbg.c is checked here. */
    {
        drbg_state st;
        uint8_t seed[32], out[16];
        int i;
        for (i = 0; i < 32; i++) seed[i] = (uint8_t)i;
        drbg_init(&st, seed, sizeof seed);
        drbg_generate(&st, out, sizeof out);
        raw_str("DRBG=");
        raw_hex(out, 16);
        raw_str("\r\n");
    }

    raw_str("END\r\n");
    return 0;
}
