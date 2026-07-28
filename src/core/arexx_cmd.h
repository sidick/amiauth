/* arexx_cmd.h -- portable ARexx command-line parsing + RC policy for the
 * ARexx port (#46). Pure C, no Amiga types: this is the part of arexx.c
 * that can be host-tested. The Amiga-only glue (RexxMsg handling, port
 * creation) lives in src/amiga/arexx.c; the actual vault/window work each
 * command does lives in src/gui/main.c, same split as guiport.h/main.c.
 *
 * VAR/STEM note: earlier design notes on this issue suggested a VAR/STEM
 * keyword on LIST's own template, but VAR/STEM aren't a language-level
 * clause a host command string carries -- a script gets the same
 * stem-like result already, entirely on its own, by PARSEing LIST's
 * plain multi-line RESULT (one "issuer:label" per line). True automatic
 * STEM-variable population would need rexxsyslib.library's separate
 * variable-pool API (SetRexxVarFromMsg) for no capability gain over that,
 * so LIST's template stays argument-free. */
#ifndef AMIAUTH_AREXX_CMD_H
#define AMIAUTH_AREXX_CMD_H

#include <stddef.h>
#include <stdint.h>

/* The 9 commands (#46). */
typedef enum {
    AREXX_CMD_UNKNOWN = 0,
    AREXX_CMD_GETCODE,
    AREXX_CMD_TIMELEFT,
    AREXX_CMD_LIST,
    AREXX_CMD_STATUS,
    AREXX_CMD_LOCK,
    AREXX_CMD_UNLOCK,
    AREXX_CMD_SHOW,
    AREXX_CMD_HIDE,
    AREXX_CMD_QUIT
} arexx_cmd_type;

/* ARexx RC convention (see the issue's review comment / userdocs):
 * 0 success, 5 warning (user cancelled interactively), 10 error (bad
 * argument / unknown command / account not found), 20 failure (couldn't
 * reach the vault - locked, gated off, or a save failed). */
enum {
    AREXX_RC_OK    =  0,
    AREXX_RC_WARN  =  5,
    AREXX_RC_ERROR = 10,
    AREXX_RC_FAIL  = 20
};

#define AREXX_MAX_ACCOUNT 192   /* generous; matches OTP_MAX_ISSUER+LABEL headroom */

typedef struct {
    arexx_cmd_type type;
    char account[AREXX_MAX_ACCOUNT];  /* GETCODE/TIMELEFT; empty otherwise */
    int  force;                       /* QUIT FORCE/S */
} arexx_parsed;

/* Parse one ARexx command line (the raw text a script sends, e.g.
 * "GETCODE github" or "GETCODE \"My Account\"" or "QUIT FORCE") into
 * `out`. Case-insensitive command keyword; the account argument accepts
 * a double-quoted form for names containing spaces. Returns 0 on success,
 * -1 on an unknown command or a missing required argument (map to
 * AREXX_RC_ERROR) -- `out->type` is AREXX_CMD_UNKNOWN on failure. */
int arexx_parse(const char *cmdline, arexx_parsed *out);

/* TOTP: seconds remaining in the current period (wraps
 * totp_seconds_remaining, otp.h). HOTP has no time concept: returns -1,
 * a well-defined "not applicable" answer, not an error (RC stays 0 either
 * way) -- so scripts can branch on RESULT < 0 without special-casing RC. */
long arexx_timeleft(int is_hotp, uint64_t now, uint64_t t0, uint32_t period);

#endif /* AMIAUTH_AREXX_CMD_H */
