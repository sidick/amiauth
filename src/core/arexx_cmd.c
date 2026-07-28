/* arexx_cmd.c -- see arexx_cmd.h. */
#include <string.h>

#include "arexx_cmd.h"
#include "otp.h"

/* ASCII case-insensitive full-string compare, same shape as the CLI's own
 * ci_streq (src/cli/main.c) - kept as a separate copy since this file must
 * stay a portable core/ module with no dependency on the CLI front-end. */
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

static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

/* Read one token starting at p. A leading '"' reads a quoted token up to
 * the closing '"' (no embedded-quote escaping - not needed for this app's
 * simple templates); otherwise reads up to the next whitespace. Copies
 * into dst (cap bytes, NUL-terminated, silently truncates if needed) and
 * returns a pointer just past the token. */
static const char *read_token(const char *p, char *dst, size_t cap)
{
    size_t n = 0;
    if (*p == '"') {
        p++;
        while (*p && *p != '"') {
            if (n + 1 < cap) dst[n++] = *p;
            p++;
        }
        if (*p == '"') p++;
    } else {
        while (*p && *p != ' ' && *p != '\t') {
            if (n + 1 < cap) dst[n++] = *p;
            p++;
        }
    }
    dst[n] = '\0';
    return p;
}

int arexx_parse(const char *cmdline, arexx_parsed *out)
{
    char kw[16];
    const char *p;

    if (!cmdline || !out) return -1;
    memset(out, 0, sizeof *out);
    out->type = AREXX_CMD_UNKNOWN;

    p = skip_ws(cmdline);
    p = read_token(p, kw, sizeof kw);

    if      (ci_streq(kw, "GETCODE"))  out->type = AREXX_CMD_GETCODE;
    else if (ci_streq(kw, "TIMELEFT")) out->type = AREXX_CMD_TIMELEFT;
    else if (ci_streq(kw, "LIST"))     out->type = AREXX_CMD_LIST;
    else if (ci_streq(kw, "STATUS"))   out->type = AREXX_CMD_STATUS;
    else if (ci_streq(kw, "LOCK"))     out->type = AREXX_CMD_LOCK;
    else if (ci_streq(kw, "UNLOCK"))   out->type = AREXX_CMD_UNLOCK;
    else if (ci_streq(kw, "SHOW"))     out->type = AREXX_CMD_SHOW;
    else if (ci_streq(kw, "HIDE"))     out->type = AREXX_CMD_HIDE;
    else if (ci_streq(kw, "QUIT"))     out->type = AREXX_CMD_QUIT;
    else { out->type = AREXX_CMD_UNKNOWN; return -1; }

    switch (out->type) {
        case AREXX_CMD_GETCODE:
        case AREXX_CMD_TIMELEFT: {
            char acct[AREXX_MAX_ACCOUNT];
            p = skip_ws(p);
            if (!*p) { out->type = AREXX_CMD_UNKNOWN; return -1; }  /* ACCOUNT/A missing */
            read_token(p, acct, sizeof acct);
            strcpy(out->account, acct);
            break;
        }
        case AREXX_CMD_QUIT: {
            char sw[16];
            p = skip_ws(p);
            if (*p) {
                read_token(p, sw, sizeof sw);
                if (ci_streq(sw, "FORCE")) out->force = 1;
                /* Anything else trailing FORCE is ignored, matching the
                 * general leniency of not needing exact ReadArgs parity
                 * for a single optional switch. */
            }
            break;
        }
        default:
            break;  /* LIST/STATUS/LOCK/UNLOCK/SHOW/HIDE take no arguments */
    }
    return 0;
}

long arexx_timeleft(int is_hotp, uint64_t now, uint64_t t0, uint32_t period)
{
    if (is_hotp) return -1;
    return (long)totp_seconds_remaining(now, t0, period);
}
