/* random.c — AmigaOS CSPRNG for vault salt/nonce, plus RAW no-echo passphrase
 * input that doubles as an entropy source (per-keystroke timing).
 *
 * AmigaOS has no strong built-in RNG, so we gather entropy from timer.device
 * EClock timing jitter and assorted volatile system state, accumulate it in a
 * SHA-1 pool, and whiten/expand through an HMAC-DRBG (src/core/drbg.c). The
 * honest limits (a quiescent 68000 yields little timing entropy; a deterministic
 * emulator yields even less) are documented in docs/SECURITY.md — which is why
 * the interactive keystroke timing during passphrase entry matters, and why each
 * request also folds DateStamp + a monotonic counter so the per-save nonce
 * stream never repeats under a fixed key.
 *
 * Linked into the m68k build only. */
#ifdef __amigaos__

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>

#include <string.h>

#include "sha1.h"
#include "drbg.h"
#include "entropy.h"

struct Device *TimerBase;          /* set while a timer.device unit is open
                                    * (type per proto/timer.h) */

static sha1_ctx   g_pool;          /* running entropy accumulator */
static int        g_pool_ready;
static drbg_state g_drbg;
static int        g_drbg_ready;
static uint32_t   g_calls;         /* monotonic, folded into every request */

static void pool_ensure(void)
{
    if (!g_pool_ready) {
        struct DateStamp ds;
        void *sysbase = (void *)SysBase;
        sha1_init(&g_pool);
        sha1_update(&g_pool, &sysbase, sizeof sysbase);
        DateStamp(&ds);
        sha1_update(&g_pool, &ds, sizeof ds);
        g_pool_ready = 1;
    }
}

void amiga_entropy_stir(const void *p, size_t n)
{
    pool_ensure();
    sha1_update(&g_pool, p, n);
}

/* timer.device UNIT_ECLOCK, opened once and kept for the process lifetime (the
 * OS reclaims it at exit). Shared by the entropy gatherer and amiga_millis, so
 * TimerBase stays valid throughout — hence no per-call open/close. */
static struct MsgPort     *g_tport;
static struct timerequest *g_treq;
static int                 g_timer_tried;

static int timer_ready(void)
{
    if (!g_timer_tried) {
        struct MsgPort *port = CreateMsgPort();
        g_timer_tried = 1;
        if (port) {
            struct timerequest *tr =
                (struct timerequest *)CreateIORequest(port, sizeof *tr);
            if (tr && OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK,
                                 (struct IORequest *)tr, 0) == 0) {
                TimerBase = tr->tr_node.io_Device;
                g_tport = port;
                g_treq = tr;
            } else {
                if (tr) DeleteIORequest((struct IORequest *)tr);
                DeleteMsgPort(port);
            }
        }
    }
    return g_treq != NULL;
}

/* Milliseconds from the E-clock, monotonic (wraps ~every 49 days). 0 if no
 * timer — the caller then falls back to the default iteration count. */
uint32_t amiga_millis(void)
{
    struct EClockVal ev;
    ULONG freq;
    uint64_t ticks;
    if (!timer_ready()) return 0;
    freq = ReadEClock(&ev);
    if (!freq) return 0;
    ticks = ((uint64_t)ev.ev_hi << 32) | ev.ev_lo;
    return (uint32_t)(ticks * 1000u / freq);
}

/* Fold a fresh EClock reading into the pool (TimerBase must be valid). */
static void stir_eclock(void)
{
    struct EClockVal ev;
    ReadEClock(&ev);
    sha1_update(&g_pool, &ev, sizeof ev);
}

/* Fold volatile system state that varies run-to-run. */
static void stir_system_state(void)
{
    struct DateStamp ds;
    ULONG mem[3];
    void *addrs[3];
    void *blk;

    DateStamp(&ds);
    sha1_update(&g_pool, &ds, sizeof ds);

    mem[0] = AvailMem(MEMF_ANY);
    mem[1] = AvailMem(MEMF_CHIP);
    mem[2] = AvailMem(MEMF_FAST);
    sha1_update(&g_pool, mem, sizeof mem);

    addrs[0] = (void *)FindTask(NULL);
    addrs[1] = (void *)&ds;                     /* a stack address */
    addrs[2] = (void *)SysBase;
    sha1_update(&g_pool, addrs, sizeof addrs);

    /* A fresh allocation: its address, plus its residual (uninitialised) bytes. */
    blk = AllocMem(64, MEMF_ANY);               /* no MEMF_CLEAR: keep residue */
    if (blk) {
        sha1_update(&g_pool, &blk, sizeof blk);
        sha1_update(&g_pool, blk, 64);
        FreeMem(blk, 64);
    }
}

int amiga_random(uint8_t *buf, size_t n)
{
    sha1_ctx snap;
    uint8_t seed[SHA1_DIGEST_SIZE];

    pool_ensure();

    g_calls++;
    sha1_update(&g_pool, &g_calls, sizeof g_calls);
    stir_system_state();

    /* EClock jitter: rapid reads with a little work between them. */
    if (timer_ready()) {
        int i;
        for (i = 0; i < 96; i++) {
            stir_eclock();
            (void)AvailMem(MEMF_ANY);           /* perturb timing slightly */
        }
    }

    /* Whiten/expand: seed (first call) or reseed (later) the DRBG from a
     * snapshot of the pool, then emit. The snapshot finalises a copy so the
     * running pool keeps accumulating. */
    snap = g_pool;
    sha1_final(&snap, seed);
    if (!g_drbg_ready) { drbg_init(&g_drbg, seed, sizeof seed); g_drbg_ready = 1; }
    else               { drbg_reseed(&g_drbg, seed, sizeof seed); }
    drbg_generate(&g_drbg, buf, n);

    memset(seed, 0, sizeof seed);
    return 0;
}

int amiga_read_passphrase(const char *prompt, char *buf, size_t cap)
{
    BPTR in = Input(), out = Output();
    size_t len = 0;
    int raw_ok, have_timer;

    if (cap == 0) return -1;
    if (!IsInteractive(in)) return -1;          /* encrypted vaults need a console */

    pool_ensure();
    if (prompt) Write(out, (APTR)prompt, (LONG)strlen(prompt));

    have_timer = timer_ready();                 /* per-keystroke timing source */
    raw_ok = (SetMode(in, 1) != 0);             /* 1 = RAW (unbuffered, no echo) */

    for (;;) {
        char c;
        if (Read(in, &c, 1) <= 0) break;        /* EOF/error ends input */
        if (c == '\n' || c == '\r') break;
        if (have_timer) stir_eclock();          /* inter-keystroke jitter */
        if (c == '\b' || c == 0x7f) {           /* backspace / delete */
            if (len) { len--; Write(out, (APTR)"\b \b", 3); }
            continue;
        }
        if ((unsigned char)c < 0x20) continue;  /* ignore other control chars */
        if (len < cap - 1) { buf[len++] = c; Write(out, (APTR)"*", 1); }
    }
    buf[len] = '\0';

    if (raw_ok) SetMode(in, 0);                 /* restore cooked mode */
    Write(out, (APTR)"\n", 1);
    return 0;
}

int amiga_read_line(const char *prompt, char *buf, size_t cap)
{
    BPTR in = Input(), out = Output();
    size_t len = 0;
    int raw_ok;

    if (cap == 0) return -1;
    if (!IsInteractive(in)) return -1;          /* prompts are interactive-only */
    if (prompt) Write(out, (APTR)prompt, (LONG)strlen(prompt));

    /* Read char-by-char in RAW mode (with echo). Not FGets: this handle is also
     * read unbuffered via Read() for the passphrase, and mixing buffered FGets
     * with unbuffered Read() makes FGets return EOF immediately. */
    raw_ok = (SetMode(in, 1) != 0);
    for (;;) {
        char c;
        if (Read(in, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') break;
        if (c == '\b' || c == 0x7f) {           /* backspace / delete */
            if (len) { len--; Write(out, (APTR)"\b \b", 3); }
            continue;
        }
        if ((unsigned char)c < 0x20) continue;
        if (len < cap - 1) { buf[len++] = c; Write(out, &c, 1); }   /* echo */
    }
    buf[len] = '\0';

    if (raw_ok) SetMode(in, 0);
    Write(out, (APTR)"\n", 1);
    return 0;
}

#endif /* __amigaos__ */
