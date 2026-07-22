/* main.c — AmiAuth CLI front-end.
 *
 * Commands:
 *   CODE <base32-secret> [digits] [period]   one-shot code, no vault
 *   INIT [--open]                            create a vault
 *   ADD  <otpauth://...>                     import an account
 *   LIST                                     list account names
 *   GET  <account>                           print an account's current code
 *   REMOVE <account>                         delete an account
 *   HELP
 *
 * Global option: -v/--vault PATH (or AMIAUTH_VAULT), default per platform.
 *
 * Passphrase policy (see docs/SECURITY.md): an encrypted vault is unlocked by an
 * interactive terminal prompt only — no env/file hatch. Non-interactive use is
 * served by always-unlocked vaults. No-echo passphrase input and random
 * salt/nonce come from POSIX /dev/tty + /dev/urandom on the host, and from the
 * AmigaOS RAW console + entropy source (src/amiga/random.c) on hardware; where
 * no secure RNG exists, encrypted create/save is cleanly refused. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otp.h"
#include "uri.h"
#include "base32.h"
#include "clock.h"
#include "vault.h"
#include "pbkdf2.h"
#include "prefs.h"

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#  include <unistd.h>
#  include <fcntl.h>
#  include <termios.h>
#  define AMIAUTH_POSIX 1
#endif

#if defined(__amigaos__) || defined(AMIGA) || defined(amiga)
#  include <proto/exec.h>
#  include <proto/locale.h>
#  include <proto/dos.h>        /* ReadArgs / FreeArgs / PrintFault / IoErr */
#  include <dos/rdargs.h>
#  define AMIAUTH_AMIGA 1
#  define DEFAULT_VAULT "PROGDIR:AmiAuth.vault"
#  include "entropy.h"          /* amiga_random / amiga_read_passphrase */
#  include "../amiga/crypto_select.h"  /* select crypto hot-loop impl, #47 */
#else
#  define DEFAULT_VAULT "AmiAuth.vault"
#endif

#include "../amiga/guiport.h"     /* forward vault commands to a resident GUI */
#include "../version.h"

#ifdef AMIAUTH_AMIGA
AMIAUTH_VERSTAG("AmiAuth")
#endif

#ifndef AMIAUTH_AMIGA
/* Host: there is never a resident GUI, so forwarding is a no-op (do it locally). */
int gui_forward(int cmd, const char *arg, char *buf, unsigned long buflen, int *result)
{
    (void)cmd; (void)arg; (void)buf; (void)buflen; (void)result;
    return -1;
}
#endif

/* --no-rekey suppresses the adaptive re-key prompts for a single run (the
 * ENVARC:AmiAuth/rekey pref silences them permanently). */
static int g_no_rekey;

/* Program name for help text; set from argv[0] in main(). */
static const char *g_prog = "AmiAuth";

/* Parsed command line, filled by the platform's parser (ReadArgs on Amiga,
 * getopt-style on the host) and consumed by dispatch(). String fields are NULL
 * when absent; iterations is -1 for auto-calibrate. */
typedef struct {
    const char *command;
    const char *value;       /* the command's positional argument: CODE secret,
                              * ADD uri, GET/REMOVE account, SYNC server,
                              * OFFSET seconds */
    const char *digits;      /* CODE (2nd positional) */
    const char *period;      /* CODE (3rd positional) */
    const char *issuer;      /* ISSUER / --issuer (bare-secret ADD) */
    const char *label;       /* LABEL / --label (bare-secret ADD) */
    const char *vault;       /* VAULT / -v (NULL = env / pref / default) */
    int         open;        /* OPEN / --open */
    long        iterations;  /* ITERATIONS / --iterations (-1 = auto) */
    int         no_rekey;    /* NOREKEY / --no-rekey */
} cli_args;

/* ---- platform hooks ---- */

/* Fill buf with n cryptographically-random bytes. Returns 0 on success. */
static int cli_random(uint8_t *buf, size_t n)
{
#ifdef AMIAUTH_POSIX
    int fd = open("/dev/urandom", O_RDONLY);
    size_t got = 0;
    if (fd < 0) return -1;
    while (got < n) {
        ssize_t r = read(fd, buf + got, n - got);
        if (r <= 0) { close(fd); return -1; }
        got += (size_t)r;
    }
    close(fd);
    return 0;
#elif defined(AMIAUTH_AMIGA)
    return amiga_random(buf, n);
#else
    (void)buf; (void)n;
    return -1;
#endif
}

/* Monotonic-ish milliseconds, for timing PBKDF2 during KDF calibration. On the
 * host, clock() (CPU time) tracks wall time closely for a compute-bound probe.
 * Returns 0 where no timer is available -> the caller uses the default count. */
static uint32_t cli_millis(void)
{
#ifdef AMIAUTH_POSIX
    return (uint32_t)((uint64_t)clock() * 1000u / CLOCKS_PER_SEC);
#elif defined(AMIAUTH_AMIGA)
    return amiga_millis();
#else
    return 0;
#endif
}

/* Pick the PBKDF2 iteration count for a new vault. An explicit count (>0, from
 * --iterations) wins; otherwise probe local speed and calibrate to ~1s here.
 * The probe grows x4 until it is long enough to time; on a stock 68000 the first
 * tiny probe already exceeds the resolution floor (~14 iters/s), so it stays a
 * second or two. Falls back to the default if no timer is available. */
static uint32_t cli_calibrate(long explicit_iters)
{
    static const uint8_t salt[16] = { 0 };
    uint8_t dk[64];
    uint32_t probe = 4, t0, ms;

    if (explicit_iters > 0) return (uint32_t)explicit_iters;

    for (;;) {
        t0 = cli_millis();
        pbkdf2_hmac_sha1((const uint8_t *)"calibration", 11, salt, sizeof salt,
                         probe, dk, sizeof dk);
        ms = cli_millis() - t0;
        if (ms >= 50) break;                       /* enough to extrapolate */
        if (probe >= KDF_MAX_ITERATIONS) {         /* can't get resolution... */
            if (ms == 0) { memset(dk, 0, sizeof dk); return VAULT_DEFAULT_ITERATIONS; }
            break;                                 /* ...just very fast; use it */
        }
        probe *= 4;
    }
    memset(dk, 0, sizeof dk);
    return vault_calibrate_iterations(probe, ms);
}

/* Seconds to add to the system clock to obtain UTC. On the host, time() is
 * already UTC (0). On AmigaOS the RTC holds local time, so we read the timezone
 * from locale.library: loc_GMTOffset is minutes west of GMT, and
 *   UTC = local + (minutes_west * 60),
 * which is exactly clock_ctx's offset convention (offset added to system time).
 * A first-guess only — DST is not handled and the value is 0 if the user has
 * not set a timezone in Locale prefs. Returns 0 on any failure. */
static long cli_utc_offset(void)
{
#ifdef AMIAUTH_AMIGA
    struct Library *LocaleBase;
    long offset = 0;
    LocaleBase = OpenLibrary((STRPTR)"locale.library", 38);   /* OS 2.1+ */
    if (LocaleBase) {
        struct Locale *loc = OpenLocale(NULL);
        if (loc) {
            offset = loc->loc_GMTOffset * 60;
            CloseLocale(loc);
        }
        CloseLibrary(LocaleBase);
    }
    return offset;
#else
    return 0;
#endif
}

/* Initialise a clock context and apply the best available UTC offset:
 * a persisted offset (from SYNC or OFFSET) wins over the locale first-guess. */
static void cli_clock_init(clock_ctx *c)
{
    long off;
    clock_init(c);
    if (prefs_get_long("offset", &off) == 0) {
        clock_set_offset(c, off);        /* stored offset, not freshly verified */
    } else {
        off = cli_utc_offset();          /* locale first-guess */
        if (off) clock_set_offset(c, off);
    }
}

#ifndef AMIAUTH_AMIGA
static void strip_eol(char *s)      /* only the fgets-based paths need this */
{
    size_t l = strlen(s);
    while (l && (s[l - 1] == '\n' || s[l - 1] == '\r')) s[--l] = '\0';
}
#endif

/* Prompt on the controlling terminal and read a passphrase without echo.
 * Returns 0 on success, -1 if there is no terminal or on error. */
static int read_passphrase(const char *prompt, char *buf, size_t cap)
{
#ifdef AMIAUTH_POSIX
    struct termios old, quiet;
    FILE *tty = fopen("/dev/tty", "r+");
    int fd, ok;
    if (!tty) return -1;                    /* no controlling terminal */
    fd = fileno(tty);
    if (tcgetattr(fd, &old) != 0) { fclose(tty); return -1; }
    quiet = old;
    quiet.c_lflag &= ~(tcflag_t)ECHO;
    fputs(prompt, tty); fflush(tty);
    tcsetattr(fd, TCSANOW, &quiet);
    ok = fgets(buf, (int)cap, tty) != NULL;
    tcsetattr(fd, TCSANOW, &old);
    fputc('\n', tty);
    fclose(tty);
    if (!ok) return -1;
    strip_eol(buf);
    return 0;
#elif defined(AMIAUTH_AMIGA)
    return amiga_read_passphrase(prompt, buf, cap);
#else
    (void)prompt;
    if (!fgets(buf, (int)cap, stdin)) return -1;
    strip_eol(buf);
    return 0;
#endif
}

/* Prompt and read a line with normal echo, for interactive re-key confirmations.
 * Returns 0 on success, -1 if there is no terminal/console or on EOF. */
static int cli_readline(const char *prompt, char *buf, size_t cap)
{
#ifdef AMIAUTH_POSIX
    FILE *tty = fopen("/dev/tty", "r+");
    int ok;
    if (!tty) return -1;
    fputs(prompt, tty); fflush(tty);
    ok = fgets(buf, (int)cap, tty) != NULL;
    fclose(tty);
    if (!ok) return -1;
    strip_eol(buf);
    return 0;
#elif defined(AMIAUTH_AMIGA)
    return amiga_read_line(prompt, buf, cap);
#else
    (void)prompt;
    if (!fgets(buf, (int)cap, stdin)) return -1;
    strip_eol(buf);
    return 0;
#endif
}

/* ---- helpers ---- */

static int ci_streq(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        int ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static const char *vault_err(vault_result rc)
{
    switch (rc) {
        case VAULT_OK:         return "ok";
        case VAULT_ERR_IO:     return "cannot read/write the vault file";
        case VAULT_ERR_FORMAT: return "not a valid vault file";
        case VAULT_ERR_AUTH:   return "wrong passphrase or the file has been tampered with";
        case VAULT_ERR_LOCKED: return "vault is locked";
        case VAULT_ERR_FULL:   return "vault is full";
        case VAULT_ERR_RANGE:  return "no such account";
        default:               return "error";
    }
}

static vault_result save_vault(const vault *v, const char *path);

/* After a successful unlock we know the stored iteration count and how long the
 * KDF took, so we can tell whether this machine is much faster or slower than the
 * one that secured the vault, and offer to re-key (recalibrate to ~1s here and
 * re-save). Interactive encrypted vaults only; --no-rekey or ENVARC:AmiAuth/rekey
 * = off silences it. The 8x threshold is generous so emulator/warp variance never
 * nags — only a clear hardware-class jump does. */
static void maybe_rekey(vault *v, const char *path, const char *pass, uint32_t unlock_ms)
{
    uint32_t ideal;
    char prefbuf[8], line[16];

    if (v->cipher != VAULT_CIPHER_CHACHA20) return;   /* encrypted only */
    if (g_no_rekey || unlock_ms == 0) return;         /* opted out / no timer */
    if (prefs_get("rekey", prefbuf, sizeof prefbuf) == 0 && ci_streq(prefbuf, "off"))
        return;

    ideal = vault_calibrate_iterations(v->iterations, unlock_ms);

    if (unlock_ms < KDF_TARGET_MS / 8 && ideal > v->iterations) {
        /* Much faster machine -> offer to strengthen (safe; one confirm). */
        int c;
        if (cli_readline("This machine is much faster than the one that secured "
                "this vault.\nStrengthen it now? [(y)es/(N)o/ne(v)er ask here] ",
                line, sizeof line) != 0)
            return;
        c = line[0] | 0x20;
        if (c == 'v') { prefs_set("rekey", "off"); return; }
        if (c != 'y') return;
    } else if (unlock_ms > KDF_TARGET_MS * 8) {
        /* Much slower machine -> offer to speed up, but this weakens it: friction. */
        fprintf(stderr, "AmiAuth: unlock took ~%lus; this vault was tuned for "
                "faster hardware.\n", (unsigned long)((unlock_ms + 500) / 1000));
        if (cli_readline("Re-key LOWER for quicker unlocks here? This REDUCES "
                "security. [y/N] ", line, sizeof line) != 0
            || (line[0] | 0x20) != 'y')
            return;
        if (cli_readline("Type 'yes' to confirm: ", line, sizeof line) != 0
            || !ci_streq(line, "yes"))
            return;
    } else {
        return;                                       /* within range; nothing to do */
    }

    {   /* Shared re-key: fresh salt + calibrated count, same passphrase. */
        uint8_t salt[VAULT_SALT_SIZE];
        if (cli_random(salt, sizeof salt) == 0
            && vault_set_passphrase(v, pass, ideal, salt) == VAULT_OK
            && save_vault(v, path) == VAULT_OK)
            fprintf(stderr, "AmiAuth: re-keyed to %lu iterations\n", (unsigned long)ideal);
        else
            fprintf(stderr, "AmiAuth: re-key failed; vault left unchanged\n");
        memset(salt, 0, sizeof salt);
    }
}

/* Load a vault, prompting for the passphrase if it is encrypted. */
static vault_result open_vault(vault *v, const char *path)
{
    vault_result rc = vault_load(v, path, NULL);
    if (rc == VAULT_ERR_LOCKED) {
        char pass[256];
        uint32_t t0, unlock_ms;
        if (read_passphrase("Passphrase: ", pass, sizeof(pass)) != 0) {
            fprintf(stderr,
                "AmiAuth: this vault is encrypted; run from an interactive "
                "terminal, or use an always-unlocked vault for scripting\n");
            return VAULT_ERR_AUTH;
        }
        t0 = cli_millis();
        rc = vault_load(v, path, pass);
        unlock_ms = cli_millis() - t0;
        if (rc == VAULT_OK)
            maybe_rekey(v, path, pass, unlock_ms);
        memset(pass, 0, sizeof(pass));
    }
    return rc;
}

/* Save a vault, generating a fresh nonce for an encrypted one. */
static vault_result save_vault(const vault *v, const char *path)
{
    if (v->cipher == VAULT_CIPHER_CHACHA20) {
        uint8_t nonce[VAULT_NONCE_SIZE];
        if (cli_random(nonce, sizeof(nonce)) != 0) {
            fprintf(stderr,
                "AmiAuth: no secure random source to save an encrypted vault "
                "(Phase 4)\n");
            return VAULT_ERR_IO;
        }
        return vault_save(v, path, nonce);
    }
    return vault_save(v, path, NULL);
}

static int find_account(const vault *v, const char *q)
{
    size_t i;
    for (i = 0; i < v->count; i++) {
        const otp_account *a = &v->accounts[i];
        char combo[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
        sprintf(combo, "%s:%s", a->issuer, a->label);
        if (ci_streq(q, a->label) || ci_streq(q, a->issuer) || ci_streq(q, combo))
            return (int)i;
    }
    return -1;
}

/* ---- commands ---- */

static int cmd_code(const char *secret_b32, const char *digits_s, const char *period_s)
{
    uint8_t key[OTP_MAX_SECRET];
    int keylen, digits, period;
    clock_ctx clk;
    uint64_t now;

    digits = digits_s ? atoi(digits_s) : OTP_DEFAULT_DIGITS;
    period = period_s ? atoi(period_s) : OTP_DEFAULT_PERIOD;
    if (digits < 1 || digits > 9) digits = OTP_DEFAULT_DIGITS;
    if (period < 1)               period = OTP_DEFAULT_PERIOD;

    keylen = base32_decode(secret_b32, key, sizeof(key));
    if (keylen <= 0) {
        fprintf(stderr, "AmiAuth: invalid or empty Base32 secret\n");
        return 2;
    }

    cli_clock_init(&clk);
    now = clock_now_utc(&clk);
    printf("%0*lu\n", digits,
           (unsigned long)totp_sha1(key, (size_t)keylen, now, 0, (uint32_t)period, digits));
    fprintf(stderr, "(%u seconds remaining)\n",
            totp_seconds_remaining(now, 0, (uint32_t)period));
    memset(key, 0, sizeof(key));
    return 0;
}

static void print_clock(const clock_ctx *c)
{
    uint64_t utc = clock_now_utc(c);
    time_t t = (time_t)utc;
    struct tm *tm = gmtime(&t);
    const char *status = c->state == CLOCK_SYNCED ? "synced (green)"
                       : c->state == CLOCK_MANUAL ? "offset applied (amber)"
                       :                            "unverified (red)";

    printf("UTC offset : %+ld seconds (%+ld min)\n", c->offset_seconds,
           c->offset_seconds / 60);
    printf("status     : %s\n", status);
    if (tm)
        printf("corrected  : %04d-%02d-%02d %02d:%02d:%02d UTC\n",
               tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
               tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static int cmd_clock(void)
{
    clock_ctx c;
    cli_clock_init(&c);
    print_clock(&c);
    return 0;
}

static int cmd_sync(const char *server)
{
    clock_ctx c;
    char cfg[128];

    /* server arg > saved server > default */
    if (!server) {
        if (prefs_get("server", cfg, sizeof(cfg)) == 0 && cfg[0]) server = cfg;
        else server = "pool.ntp.org";
    }

    cli_clock_init(&c);                 /* baseline, overridden on success */
    printf("Querying %s ...\n", server);
    if (clock_sntp_sync(&c, server) != 0) {
        fprintf(stderr,
            "AmiAuth: SNTP sync failed (no TCP/IP stack, or no response from %s)\n",
            server);
        return 2;
    }
    /* Persist so GET/CODE use the corrected time without a per-call sync. */
    prefs_set("server", server);
    prefs_set_long("offset", c.offset_seconds);
    print_clock(&c);
    return 0;
}

static int cmd_offset(const char *arg)
{
    clock_ctx c;
    if (prefs_set_long("offset", atol(arg)) != 0) {
        fprintf(stderr, "AmiAuth: could not save the offset\n");
        return 2;
    }
    cli_clock_init(&c);
    print_clock(&c);
    return 0;
}

/* Adjust the current offset by a relative delta and save it — for dialling
 * an offline machine's clock in by eye against a known-good code, a step at a
 * time, without having to work out (or remember) the absolute offset. */
static int cmd_nudge(const char *arg)
{
    clock_ctx c;
    cli_clock_init(&c);
    clock_nudge(&c, atol(arg));
    if (prefs_set_long("offset", c.offset_seconds) != 0) {
        fprintf(stderr, "AmiAuth: could not save the offset\n");
        return 2;
    }
    print_clock(&c);
    return 0;
}

/* Record where the vault lives (absolute path) in the prefs at creation, so
 * every later launch - and the GUI, even started from WBStartup where
 * PROGDIR: differs - finds the same vault (docs/STORAGE.md). Only called when
 * the path was not explicitly overridden, so scratch `VAULT=` vaults created
 * for tests or experiments never hijack the pref. */
static void record_vault_path(const char *path)
{
#ifdef AMIAUTH_AMIGA
    static char abs[512];              /* static: off the ~4 KB shell stack */
    BPTR lock;
    strncpy(abs, path, sizeof abs - 1);
    abs[sizeof abs - 1] = '\0';
    lock = Lock((CONST_STRPTR)path, ACCESS_READ);
    if (lock) {
        if (!NameFromLock(lock, (STRPTR)abs, sizeof abs)) {
            strncpy(abs, path, sizeof abs - 1);
            abs[sizeof abs - 1] = '\0';
        }
        UnLock(lock);
    }
    prefs_set("vault", abs);
#else
    char *abs = realpath(path, NULL);
    prefs_set("vault", abs ? abs : path);
    free(abs);
#endif
}

static int cmd_init(const char *path, int always_unlocked, long iterations,
                    int record_path)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc;
    char pass[256];
    FILE *exists;

    exists = fopen(path, "rb");
    if (exists) {
        fclose(exists);
        fprintf(stderr, "AmiAuth: %s already exists\n", path);
        return 2;
    }

    if (always_unlocked) {
        rc = vault_create(&v, NULL, 0, NULL);
    } else {
        char confirm[256];
        if (read_passphrase("New passphrase (empty for an always-unlocked vault): ",
                             pass, sizeof(pass)) != 0) {
            fprintf(stderr,
                "AmiAuth: INIT needs an interactive terminal "
                "(use 'INIT --open' for a non-interactive always-unlocked vault)\n");
            return 2;
        }
        if (pass[0] == '\0') {
            rc = vault_create(&v, NULL, 0, NULL);
        } else {
            uint8_t salt[VAULT_SALT_SIZE];
            if (read_passphrase("Confirm passphrase: ", confirm, sizeof(confirm)) != 0
                || strcmp(pass, confirm) != 0) {
                memset(pass, 0, sizeof(pass));
                memset(confirm, 0, sizeof(confirm));
                fprintf(stderr, "AmiAuth: passphrases did not match\n");
                return 2;
            }
            memset(confirm, 0, sizeof(confirm));
            if (cli_random(salt, sizeof(salt)) != 0) {
                memset(pass, 0, sizeof(pass));
                fprintf(stderr,
                    "AmiAuth: no secure random source for an encrypted vault "
                    "(Phase 4); create an always-unlocked vault with 'INIT --open'\n");
                return 2;
            }
            {
                uint32_t iters = cli_calibrate(iterations);
                fprintf(stderr, "AmiAuth: KDF iterations = %lu\n", (unsigned long)iters);
                rc = vault_create(&v, pass, iters, salt);
            }
            memset(salt, 0, sizeof(salt));
        }
        memset(pass, 0, sizeof(pass));
    }

    if (rc == VAULT_OK) rc = save_vault(&v, path);
    if (rc != VAULT_OK) {
        vault_lock(&v);
        fprintf(stderr, "AmiAuth: %s\n", vault_err(rc));
        return 2;
    }
    printf("Created %s vault at %s\n",
           v.cipher == VAULT_CIPHER_NONE ? "always-unlocked" : "encrypted", path);
    if (record_path) record_vault_path(path);
    vault_lock(&v);
    return 0;
}

/* If a GUI is resident it owns the (unlocked) vault: forward the command there
 * instead of opening the vault a second time. Returns the process exit code when
 * the GUI handled it, or -1 if no GUI is running (do it locally). */
static int try_forward(int cmd, const char *arg)
{
    static char buf[2600];   /* off the ~4 KB shell stack; holds a LIST reply */
    int result = AAR_OK;

    if (gui_forward(cmd, arg, buf, sizeof buf, &result) != 0)
        return -1;                            /* no resident GUI -> local path */

    switch (result) {
        case AAR_OK:
            if (buf[0]) fputs(buf, stdout);   /* GET code / LIST names (with \n) */
            return 0;
        case AAR_LOCKED:
            fprintf(stderr, "AmiAuth: the running GUI's vault is locked; "
                            "unlock it there first\n");
            return 2;
        case AAR_NOTFOUND:
            fprintf(stderr, "AmiAuth: no account matching '%s'\n", arg ? arg : "");
            return 2;
        case AAR_FULL:
            fprintf(stderr, "AmiAuth: the vault is full (max 64 accounts)\n");
            return 2;
        case AAR_BADARG:
            fprintf(stderr, "AmiAuth: not a valid otpauth:// URI\n");
            return 2;
        case AAR_SAVEFAIL:
            fprintf(stderr, "AmiAuth: applied in the GUI but the re-save failed\n");
            return 2;
        default:
            fprintf(stderr, "AmiAuth: the running GUI could not handle that\n");
            return 2;
    }
}

static int cmd_add(const char *path, const char *uri)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    otp_account acct;
    vault_result rc;
    int fc = try_forward(AAP_ADD, uri);       /* resident GUI owns the vault? */
    if (fc >= 0) return fc;

    if (otpauth_parse(uri, &acct) != 0) {
        fprintf(stderr, "AmiAuth: could not parse otpauth:// URI\n");
        return 2;
    }
    rc = open_vault(&v, path);
    if (rc != VAULT_OK) { fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    rc = vault_add(&v, &acct);
    if (rc == VAULT_OK) rc = save_vault(&v, path);
    memset(&acct, 0, sizeof(acct));
    if (rc != VAULT_OK) { vault_lock(&v); fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    {
        const otp_account *added = &v.accounts[v.count - 1];
        if (added->issuer[0]) printf("Added %s:%s\n", added->issuer, added->label);
        else                  printf("Added %s\n", added->label);
    }
    vault_lock(&v);
    return 0;
}

/* ADD with a bare Base32 secret (no otpauth:// wrapper): the common case for
 * services that show only the raw secret. Defaults to a SHA-1/6-digit/30s
 * TOTP; anything else still needs the full URI form (#83). */
static int cmd_add_secret(const char *path, const char *secret,
                          const char *issuer, const char *label)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    static char packed[320];   /* issuer \n label \n secret for the GUI port */
    otp_account acct;
    vault_result rc;
    int fc;

    if (!issuer || !issuer[0] || !label || !label[0]) {
        fprintf(stderr,
            "AmiAuth: adding a bare secret needs ISSUER and LABEL, e.g.\n"
            "  %s ADD %s ISSUER GitHub LABEL you@example.com\n"
            "(makes a 6-digit/30s TOTP; use the otpauth:// URI form for "
            "anything else)\n", g_prog, secret);
        return 2;
    }
    if (otp_account_from_secret(issuer, label, secret, &acct) != 0) {
        fprintf(stderr, "AmiAuth: that does not look like a Base32 secret\n");
        return 2;
    }

    /* Resident GUI owns the vault? Same routing as the URI form. */
    if (strlen(issuer) + strlen(label) + strlen(secret) + 3 <= sizeof packed) {
        sprintf(packed, "%s\n%s\n%s", issuer, label, secret);
        fc = try_forward(AAP_ADD_SECRET, packed);
        memset(packed, 0, sizeof packed);
        if (fc >= 0) { memset(&acct, 0, sizeof acct); return fc; }
    }

    rc = open_vault(&v, path);
    if (rc != VAULT_OK) {
        memset(&acct, 0, sizeof acct);
        fprintf(stderr, "AmiAuth: %s\n", vault_err(rc));
        return 2;
    }
    rc = vault_add(&v, &acct);
    if (rc == VAULT_OK) rc = save_vault(&v, path);
    memset(&acct, 0, sizeof acct);
    if (rc != VAULT_OK) { vault_lock(&v); fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    printf("Added %s:%s\n", issuer, label);
    vault_lock(&v);
    return 0;
}

static int cmd_list(const char *path)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc;
    size_t i;
    int fc = try_forward(AAP_LIST, NULL);
    if (fc >= 0) return fc;
    rc = open_vault(&v, path);
    if (rc != VAULT_OK) { fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    for (i = 0; i < v.count; i++) {
        const otp_account *a = &v.accounts[i];
        if (a->issuer[0]) printf("%s:%s\n", a->issuer, a->label);
        else              printf("%s\n", a->label);
    }
    vault_lock(&v);
    return 0;
}

static int cmd_get(const char *path, const char *account)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc;
    int idx;
    int fc = try_forward(AAP_GET, account);
    if (fc >= 0) return fc;
    rc = open_vault(&v, path);
    otp_account *a;
    clock_ctx clk;
    uint64_t now;

    if (rc != VAULT_OK) { fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }
    idx = find_account(&v, account);
    if (idx < 0) { vault_lock(&v); fprintf(stderr, "AmiAuth: no account matching '%s'\n", account); return 2; }
    a = &v.accounts[idx];

    {
        char code[OTP_CODE_BUF];
        if (strcmp(a->type, "hotp") == 0) {
            otp_render(a, 0, code);
            printf("%s\n", code);
            a->counter++;                   /* HOTP is stateful: advance and persist */
            rc = save_vault(&v, path);
            if (rc != VAULT_OK)
                fprintf(stderr, "AmiAuth: warning: could not persist HOTP counter (%s)\n",
                        vault_err(rc));
        } else {
            cli_clock_init(&clk);
            now = clock_now_utc(&clk);
            otp_render(a, now, code);
            printf("%s\n", code);
            fprintf(stderr, "(%u seconds remaining)\n",
                    totp_seconds_remaining(now, 0, a->period));
        }
    }
    vault_lock(&v);
    return 0;
}

static int cmd_remove(const char *path, const char *account)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc;
    int idx;
    int fc = try_forward(AAP_REMOVE, account);
    if (fc >= 0) return fc;
    rc = open_vault(&v, path);
    if (rc != VAULT_OK) { fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    idx = find_account(&v, account);
    if (idx < 0) { vault_lock(&v); fprintf(stderr, "AmiAuth: no account matching '%s'\n", account); return 2; }
    rc = vault_remove(&v, (size_t)idx);
    if (rc == VAULT_OK) rc = save_vault(&v, path);
    if (rc != VAULT_OK) { vault_lock(&v); fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }

    printf("Removed '%s'\n", account);
    vault_lock(&v);
    return 0;
}

static int usage(void)
{
    const char *p = g_prog;
    fprintf(stderr,
        "AmiAuth - TOTP/HOTP authenticator for AmigaOS\n"
        "\n"
        "Run as '%s <COMMAND> ...'  ('%s ?' shows the arg template):\n"
        "  CODE   <secret> [digits] [period]  Print a code (no vault)\n"
        "  INIT   [OPEN]                      Create a vault\n"
        "  ADD    \"<otpauth://...>\"           Import an account from a URI\n"
        "  ADD    <secret> ISSUER <name> LABEL <acct>  Import a bare Base32\n"
        "                                     secret (6-digit/30s SHA-1 TOTP)\n"
        "  LIST                               List account names\n"
        "  GET    <account>                   Print an account's code\n"
        "  REMOVE <account>                   Delete an account\n"
        "  SHOW                               Pop the running GUI to front\n"
        "  CLOCK                              Show UTC offset + status\n"
        "  SYNC   [server]                    SNTP sync + save\n"
        "  OFFSET <seconds>                   Set + save a UTC offset\n"
        "  NUDGE  <+/-seconds>                Adjust + save the current offset\n"
        "  HELP\n"
        "\n",
        p, p);
#ifdef AMIAUTH_AMIGA
    fprintf(stderr,
        "Options: ISSUER/K LABEL/K (bare-secret ADD)  VAULT <path>  OPEN/S\n"
        "         ITERATIONS/N/K (INIT)  NOREKEY/S\n"
        "Quote URIs (they contain '?'). Default vault (first set wins):\n"
        "  VAULT <path>; env AMIAUTH_VAULT; pref AmiAuth/vault\n"
        "  (SetEnv SAVE AmiAuth/vault <path>); else %s\n",
        DEFAULT_VAULT);
#else
    fprintf(stderr,
        "Options: --issuer S --label S (bare-secret ADD)  -v/--vault PATH\n"
        "         --iterations N  --no-rekey\n"
        "Default vault: -v, else $AMIAUTH_VAULT, else the 'vault' pref, else %s\n",
        DEFAULT_VAULT);
#endif
    fprintf(stderr, "Encrypted vaults prompt for the passphrase on the terminal.\n");
    return 1;
}

/* Resolve the vault path: explicit VAULT/-v, else env, else the saved 'vault'
 * pref (ENVARC:AmiAuth/vault), else the built-in default. */
static const char *resolve_vault(const cli_args *a)
{
    static char pref[256];
    if (a->vault) return a->vault;
    {
        const char *env = getenv("AMIAUTH_VAULT");
        if (env && env[0]) return env;
    }
    if (prefs_get("vault", pref, sizeof pref) == 0 && pref[0]) return pref;
    return DEFAULT_VAULT;
}

/* Run the parsed command. Shared by both parser backends. */
/* SHOW: pop the resident GUI to the front (no local equivalent). */
static int cmd_show(void)
{
    int fc = try_forward(AAP_SHOW, NULL);
    if (fc >= 0) return fc;
    fprintf(stderr, "AmiAuth: no running GUI to show\n");
    return 2;
}

static int dispatch(const cli_args *a)
{
    const char *vault = resolve_vault(a);
    g_no_rekey = a->no_rekey;

    if (!a->command)                    return usage();
    if (ci_streq(a->command, "CODE"))
        return a->value ? cmd_code(a->value, a->digits, a->period) : usage();
    if (ci_streq(a->command, "INIT")) {
        const char *env = getenv("AMIAUTH_VAULT");
        int defaulted = !a->vault && !(env && env[0]);
        return cmd_init(vault, a->open, a->iterations, defaulted);
    }
    if (ci_streq(a->command, "ADD")) {
        if (!a->value) return usage();
        return otpauth_is_uri(a->value)
             ? cmd_add(vault, a->value)
             : cmd_add_secret(vault, a->value, a->issuer, a->label);
    }
    if (ci_streq(a->command, "LIST"))   return cmd_list(vault);
    if (ci_streq(a->command, "GET"))    return a->value ? cmd_get(vault, a->value) : usage();
    if (ci_streq(a->command, "REMOVE")) return a->value ? cmd_remove(vault, a->value) : usage();
    if (ci_streq(a->command, "SHOW"))   return cmd_show();
    if (ci_streq(a->command, "CLOCK"))  return cmd_clock();
    if (ci_streq(a->command, "SYNC"))   return cmd_sync(a->value);   /* NULL = default pool */
    if (ci_streq(a->command, "OFFSET")) return a->value ? cmd_offset(a->value) : usage();
    if (ci_streq(a->command, "NUDGE"))  return a->value ? cmd_nudge(a->value) : usage();
    if (ci_streq(a->command, "HELP"))   { usage(); return 0; }
    return usage();
}

#ifdef AMIAUTH_AMIGA

/* AmigaOS: parse the whole command line with dos.library ReadArgs. The command's
 * argument is positional (GET GitHub); options are keywords. `?` template help
 * and quoting come for free. */
int main(int argc, char **argv)
{
    static const char TMPL[] =
        "COMMAND,VALUE,DIGITS,PERIOD,ISSUER/K,LABEL/K,VAULT/K,OPEN/S,"
        "ITERATIONS/N/K,NOREKEY/S";
    enum { P_COMMAND, P_VALUE, P_DIGITS, P_PERIOD, P_ISSUER, P_LABEL, P_VAULT,
           P_OPEN, P_ITERATIONS, P_NOREKEY, P_N };
    LONG opt[P_N];
    struct RDArgs *rda;
    cli_args a;
    int rc;

    if (argc > 0 && argv[0] && argv[0][0]) g_prog = argv[0];
    crypto_select_init();
    memset(opt, 0, sizeof opt);
    rda = ReadArgs((STRPTR)TMPL, opt, NULL);
    if (!rda) { PrintFault(IoErr(), (STRPTR)g_prog); return 20; }  /* RETURN_FAIL */

    memset(&a, 0, sizeof a);
    a.command    = (const char *)opt[P_COMMAND];
    a.value      = (const char *)opt[P_VALUE];
    a.digits     = (const char *)opt[P_DIGITS];
    a.period     = (const char *)opt[P_PERIOD];
    a.issuer     = (const char *)opt[P_ISSUER];
    a.label      = (const char *)opt[P_LABEL];
    a.vault      = (const char *)opt[P_VAULT];
    a.open       = opt[P_OPEN] ? 1 : 0;
    a.iterations = opt[P_ITERATIONS] ? *(LONG *)opt[P_ITERATIONS] : -1;
    a.no_rekey   = opt[P_NOREKEY] ? 1 : 0;

    rc = dispatch(&a);
    FreeArgs(rda);
    return rc;
}

#else

/* Host: a small getopt-style parser (kept for the build/tests). */
int main(int argc, char **argv)
{
    cli_args a;
    const char *pos[8];
    int npos = 0, i;

    if (argc > 0 && argv[0] && argv[0][0]) g_prog = argv[0];
    memset(&a, 0, sizeof a);
    a.iterations = -1;

    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--vault") == 0) && i + 1 < argc)
            a.vault = argv[++i];
        else if (strncmp(argv[i], "--vault=", 8) == 0)
            a.vault = argv[i] + 8;
        else if (strcmp(argv[i], "--open") == 0)
            a.open = 1;
        else if ((strcmp(argv[i], "--iterations") == 0 || strcmp(argv[i], "-i") == 0)
                 && i + 1 < argc)
            a.iterations = atol(argv[++i]);
        else if (strncmp(argv[i], "--iterations=", 13) == 0)
            a.iterations = atol(argv[i] + 13);
        else if (strcmp(argv[i], "--no-rekey") == 0)
            a.no_rekey = 1;
        else if (strcmp(argv[i], "--issuer") == 0 && i + 1 < argc)
            a.issuer = argv[++i];
        else if (strncmp(argv[i], "--issuer=", 9) == 0)
            a.issuer = argv[i] + 9;
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc)
            a.label = argv[++i];
        else if (strncmp(argv[i], "--label=", 8) == 0)
            a.label = argv[i] + 8;
        else if (npos < (int)(sizeof pos / sizeof pos[0]))
            pos[npos++] = argv[i];
    }

    a.command = npos > 0 ? pos[0] : NULL;
    a.value   = npos > 1 ? pos[1] : NULL;   /* the command's positional argument */
    a.digits  = npos > 2 ? pos[2] : NULL;   /* CODE only */
    a.period  = npos > 3 ? pos[3] : NULL;   /* CODE only */
    return dispatch(&a);
}

#endif
