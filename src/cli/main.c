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
#  define AMIAUTH_AMIGA 1
#  define DEFAULT_VAULT "PROGDIR:AmiAuth.vault"
#  include "entropy.h"          /* amiga_random / amiga_read_passphrase */
#else
#  define DEFAULT_VAULT "AmiAuth.vault"
#endif

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

/* Load a vault, prompting for the passphrase if it is encrypted. */
static vault_result open_vault(vault *v, const char *path)
{
    vault_result rc = vault_load(v, path, NULL);
    if (rc == VAULT_ERR_LOCKED) {
        char pass[256];
        if (read_passphrase("Passphrase: ", pass, sizeof(pass)) != 0) {
            fprintf(stderr,
                "AmiAuth: this vault is encrypted; run from an interactive "
                "terminal, or use an always-unlocked vault for scripting\n");
            return VAULT_ERR_AUTH;
        }
        rc = vault_load(v, path, pass);
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

static int cmd_init(const char *path, int always_unlocked)
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
            rc = vault_create(&v, pass, 0, salt);
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
    vault_lock(&v);
    return 0;
}

static int cmd_add(const char *path, const char *uri)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    otp_account acct;
    vault_result rc;

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

static int cmd_list(const char *path)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc = open_vault(&v, path);
    size_t i;
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
    vault_result rc = open_vault(&v, path);
    int idx;
    otp_account *a;
    clock_ctx clk;
    uint64_t now;

    if (rc != VAULT_OK) { fprintf(stderr, "AmiAuth: %s\n", vault_err(rc)); return 2; }
    idx = find_account(&v, account);
    if (idx < 0) { vault_lock(&v); fprintf(stderr, "AmiAuth: no account matching '%s'\n", account); return 2; }
    a = &v.accounts[idx];

    if (strcmp(a->type, "hotp") == 0) {
        printf("%0*lu\n", a->digits,
               (unsigned long)hotp_sha1(a->secret, a->secret_len, a->counter, a->digits));
        a->counter++;                       /* HOTP is stateful: advance and persist */
        rc = save_vault(&v, path);
        if (rc != VAULT_OK)
            fprintf(stderr, "AmiAuth: warning: could not persist HOTP counter (%s)\n",
                    vault_err(rc));
    } else {
        cli_clock_init(&clk);
        now = clock_now_utc(&clk);
        printf("%0*lu\n", a->digits,
               (unsigned long)totp_sha1(a->secret, a->secret_len, now, 0, a->period, a->digits));
        fprintf(stderr, "(%u seconds remaining)\n",
                totp_seconds_remaining(now, 0, a->period));
    }
    vault_lock(&v);
    return 0;
}

static int cmd_remove(const char *path, const char *account)
{
    static vault v;   /* ~19 KB: keep it off the (small, ~4 KB) AmigaShell stack */
    vault_result rc = open_vault(&v, path);
    int idx;
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

static int usage(const char *argv0)
{
    fprintf(stderr,
        "AmiAuth - TOTP/HOTP authenticator for AmigaOS\n"
        "\n"
        "Usage:\n"
        "  %s CODE <base32-secret> [digits] [period]   Print a code (no vault)\n"
        "  %s INIT [--open]                            Create a vault\n"
        "  %s ADD <otpauth://...>                      Import an account\n"
        "  %s LIST                                     List account names\n"
        "  %s GET <account>                            Print an account's code\n"
        "  %s REMOVE <account>                         Delete an account\n"
        "  %s CLOCK                                    Show the UTC offset and status\n"
        "  %s SYNC [server]                            SNTP-sync + save (default pool.ntp.org)\n"
        "  %s OFFSET <seconds>                         Set+save a manual UTC offset\n"
        "  %s HELP\n"
        "\n"
        "Options: -v/--vault PATH (or AMIAUTH_VAULT). Default: %s\n"
        "An encrypted vault prompts for its passphrase on the terminal.\n",
        argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0,
        DEFAULT_VAULT);
    return 1;
}

int main(int argc, char **argv)
{
    const char *vpath = getenv("AMIAUTH_VAULT");
    const char *pos[8];
    int npos = 0, i, open_flag = 0;

    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--vault") == 0) && i + 1 < argc)
            vpath = argv[++i];
        else if (strncmp(argv[i], "--vault=", 8) == 0)
            vpath = argv[i] + 8;
        else if (strcmp(argv[i], "--open") == 0)
            open_flag = 1;
        else if (npos < (int)(sizeof(pos) / sizeof(pos[0])))
            pos[npos++] = argv[i];
    }
    if (!vpath) vpath = DEFAULT_VAULT;
    if (npos == 0) return usage(argv[0]);

    if (ci_streq(pos[0], "CODE"))
        return npos >= 2 ? cmd_code(pos[1], npos > 2 ? pos[2] : NULL,
                                    npos > 3 ? pos[3] : NULL) : usage(argv[0]);
    if (ci_streq(pos[0], "INIT"))   return cmd_init(vpath, open_flag);
    if (ci_streq(pos[0], "ADD"))    return npos >= 2 ? cmd_add(vpath, pos[1]) : usage(argv[0]);
    if (ci_streq(pos[0], "LIST"))   return cmd_list(vpath);
    if (ci_streq(pos[0], "GET"))    return npos >= 2 ? cmd_get(vpath, pos[1]) : usage(argv[0]);
    if (ci_streq(pos[0], "REMOVE")) return npos >= 2 ? cmd_remove(vpath, pos[1]) : usage(argv[0]);
    if (ci_streq(pos[0], "CLOCK"))  return cmd_clock();
    if (ci_streq(pos[0], "SYNC"))   return cmd_sync(npos > 1 ? pos[1] : NULL);
    if (ci_streq(pos[0], "OFFSET")) return npos >= 2 ? cmd_offset(pos[1]) : usage(argv[0]);
    if (ci_streq(pos[0], "HELP"))   { usage(argv[0]); return 0; }

    return usage(argv[0]);
}
