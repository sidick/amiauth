/* prefs.c — portable integer wrappers + the host (directory-store) backend.
 * The AmigaOS backend (ENV:/ENVARC: via GetVar/SetVar) is in src/amiga/prefs.c;
 * exactly one backend is compiled per platform. */
#include <stdio.h>
#include <stdlib.h>

#include "prefs.h"

/* --- portable integer helpers (both platforms) --- */

int prefs_get_long(const char *name, long *out)
{
    char buf[32];
    char *end;
    long v;
    if (prefs_get(name, buf, sizeof(buf)) != 0) return -1;
    v = strtol(buf, &end, 10);
    if (end == buf) return -1;               /* not a number */
    if (out) *out = v;
    return 0;
}

int prefs_set_long(const char *name, long value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", value);
    return prefs_set(name, buf);
}

#ifndef __amigaos__
/* --- host backend: one file per key under $AMIAUTH_PREFS_DIR --- */
#include <sys/stat.h>

static const char *prefs_dir(void)
{
    const char *d = getenv("AMIAUTH_PREFS_DIR");
    return (d && d[0]) ? d : "amiauth-prefs";
}

static int prefs_path(const char *name, char *buf, size_t buflen)
{
    int n = snprintf(buf, buflen, "%s/%s", prefs_dir(), name);
    return (n > 0 && (size_t)n < buflen) ? 0 : -1;
}

int prefs_get(const char *name, char *buf, size_t buflen)
{
    char path[512];
    FILE *f;
    size_t n;
    if (!name || !buf || buflen == 0) return -1;
    if (prefs_path(name, path, sizeof(path)) != 0) return -1;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = fread(buf, 1, buflen - 1, f);
    fclose(f);
    buf[n] = '\0';
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
    return 0;
}

int prefs_set(const char *name, const char *value)
{
    char path[512];
    FILE *f;
    if (!name || !value) return -1;
    mkdir(prefs_dir(), 0700);                /* create the store dir if needed */
    if (prefs_path(name, path, sizeof(path)) != 0) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    fputs(value, f);
    if (fclose(f) != 0) return -1;
    return 0;
}
#endif /* !__amigaos__ */
