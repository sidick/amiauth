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

/* Open timer.device UNIT_ECLOCK (sets TimerBase). Returns the request or NULL. */
static struct timerequest *timer_open(struct MsgPort **port_out)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *tr;
    if (!port) return NULL;
    tr = (struct timerequest *)CreateIORequest(port, sizeof *tr);
    if (!tr) { DeleteMsgPort(port); return NULL; }
    if (OpenDevice((STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0) != 0) {
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        return NULL;
    }
    TimerBase = tr->tr_node.io_Device;
    *port_out = port;
    return tr;
}

static void timer_close(struct timerequest *tr, struct MsgPort *port)
{
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
    TimerBase = NULL;
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
    struct MsgPort *port = NULL;
    struct timerequest *tr;
    sha1_ctx snap;
    uint8_t seed[SHA1_DIGEST_SIZE];

    pool_ensure();

    g_calls++;
    sha1_update(&g_pool, &g_calls, sizeof g_calls);
    stir_system_state();

    /* EClock jitter: rapid reads with a little work between them. */
    tr = timer_open(&port);
    if (tr) {
        int i;
        for (i = 0; i < 96; i++) {
            stir_eclock();
            (void)AvailMem(MEMF_ANY);           /* perturb timing slightly */
        }
        timer_close(tr, port);
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
    struct MsgPort *port = NULL;
    struct timerequest *tr;
    BPTR in = Input(), out = Output();
    size_t len = 0;
    int raw_ok;

    if (cap == 0) return -1;
    if (!IsInteractive(in)) return -1;          /* encrypted vaults need a console */

    pool_ensure();
    if (prompt) Write(out, (APTR)prompt, (LONG)strlen(prompt));

    tr = timer_open(&port);                     /* per-keystroke timing source */
    raw_ok = (SetMode(in, 1) != 0);             /* 1 = RAW (unbuffered, no echo) */

    for (;;) {
        char c;
        if (Read(in, &c, 1) <= 0) break;        /* EOF/error ends input */
        if (c == '\n' || c == '\r') break;
        if (tr) stir_eclock();                  /* inter-keystroke jitter */
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
    if (tr) timer_close(tr, port);
    return 0;
}

#endif /* __amigaos__ */
