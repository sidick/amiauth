/* main.c — AmiAuth CLI front-end.
 * Dependency-free: works down to OS 2.x and floppy-booted machines.
 *
 * Phase 1: CODE generates a TOTP directly from a Base32 secret. The vault-backed
 * GET/LIST commands land in Phase 2 (see docs/ROADMAP.md). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otp.h"
#include "uri.h"     /* OTP_MAX_SECRET */
#include "base32.h"
#include "clock.h"

static int usage(const char *argv0)
{
    fprintf(stderr,
        "AmiAuth - TOTP/HOTP authenticator for AmigaOS\n"
        "\n"
        "Usage:\n"
        "  %s CODE <base32-secret> [digits] [period]\n"
        "        Print the current TOTP code for a Base32 secret\n"
        "        (defaults: %d digits, %d-second period).\n"
        "  %s GET <account>     Print the code for a stored account   (Phase 2)\n"
        "  %s LIST              List stored account names              (Phase 2)\n"
        "  %s HELP              Show this help\n",
        argv0, OTP_DEFAULT_DIGITS, OTP_DEFAULT_PERIOD, argv0, argv0, argv0);
    return 1;
}

static int cmd_code(int argc, char **argv)
{
    uint8_t key[OTP_MAX_SECRET];
    int keylen, digits, period;
    clock_ctx clk;
    uint64_t now;
    uint32_t code, left;

    if (argc < 3) return usage(argv[0]);

    digits = argc > 3 ? atoi(argv[3]) : OTP_DEFAULT_DIGITS;
    period = argc > 4 ? atoi(argv[4]) : OTP_DEFAULT_PERIOD;
    if (digits < 1 || digits > 9) digits = OTP_DEFAULT_DIGITS;
    if (period < 1)               period = OTP_DEFAULT_PERIOD;

    keylen = base32_decode(argv[2], key, sizeof(key));
    if (keylen <= 0) {
        fprintf(stderr, "AmiAuth: invalid or empty Base32 secret\n");
        return 2;
    }

    /* Phase 1 uses the raw system clock (no offset). Phase 3 adds SNTP/offset
     * correction via the same clock_ctx. */
    clock_init(&clk);
    now  = clock_now_utc(&clk);
    code = totp_sha1(key, (size_t)keylen, now, 0, (uint32_t)period, digits);
    left = totp_seconds_remaining(now, 0, (uint32_t)period);

    /* Code to stdout (zero-padded, pipe-friendly); status to stderr. */
    printf("%0*lu\n", digits, (unsigned long)code);
    fprintf(stderr, "(%u second%s remaining)\n", left, left == 1 ? "" : "s");

    memset(key, 0, sizeof(key));
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) return usage(argv[0]);

    if (strcmp(argv[1], "CODE") == 0) return cmd_code(argc, argv);
    if (strcmp(argv[1], "HELP") == 0) { usage(argv[0]); return 0; }

    if (strcmp(argv[1], "GET") == 0 || strcmp(argv[1], "LIST") == 0) {
        fprintf(stderr, "%s: needs the vault (Phase 2, see docs/ROADMAP.md)\n",
                argv[1]);
        return 2;
    }

    return usage(argv[0]);
}
