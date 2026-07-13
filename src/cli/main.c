/* main.c — AmiAuth CLI front-end.
 * Dependency-free: works down to OS 2.x and floppy-booted machines.
 * STUB: wiring lands with the vault in Phase 2. Usage: AmiAuth GET <account> */
#include <stdio.h>
#include <string.h>

#include "otp.h"
#include "vault.h"
#include "clock.h"

static int usage(const char *argv0)
{
    fprintf(stderr,
        "AmiAuth - TOTP/HOTP authenticator for AmigaOS\n"
        "\n"
        "Usage:\n"
        "  %s GET <account>     Print the current code for <account>\n"
        "  %s LIST              List account names\n"
        "  %s HELP              Show this help\n",
        argv0, argv0, argv0);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 2) return usage(argv[0]);

    if (strcmp(argv[1], "GET") == 0) {
        if (argc < 3) return usage(argv[0]);
        /* TODO: load vault, resolve account, clock_now_utc(), totp_sha1(). */
        fprintf(stderr, "GET: not implemented yet (see docs/ROADMAP.md Phase 2)\n");
        return 2;
    }
    if (strcmp(argv[1], "LIST") == 0) {
        /* TODO: load vault, print account names (never secrets). */
        fprintf(stderr, "LIST: not implemented yet\n");
        return 2;
    }
    if (strcmp(argv[1], "HELP") == 0) {
        usage(argv[0]);
        return 0;
    }

    return usage(argv[0]);
}
