/* prefs.c — AmigaOS settings backend: ENV:/ENVARC:AmiAuth/<name> variables via
 * dos.library GetVar/SetVar. SetVar uses GVF_SAVE so values also land in
 * ENVARC: and survive a reboot (see docs/STORAGE.md). Linked into the m68k build
 * only; the host backend is in src/core/prefs.c. */
#ifdef __amigaos__

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/var.h>

#include <string.h>

#include "prefs.h"

#define PREFS_PREFIX "AmiAuth/"

/* Build "AmiAuth/<name>" into var. */
static void make_var(char *var, size_t varsz, const char *name)
{
    size_t plen = sizeof(PREFS_PREFIX) - 1;
    strncpy(var, PREFS_PREFIX, varsz);
    var[varsz - 1] = '\0';
    strncat(var, name, varsz - plen - 1);
}

int prefs_get(const char *name, char *buf, size_t buflen)
{
    char var[128];
    LONG len;
    if (!name || !buf || buflen == 0) return -1;
    make_var(var, sizeof(var), name);
    len = GetVar((STRPTR)var, (STRPTR)buf, (LONG)buflen, GVF_GLOBAL_ONLY);
    if (len < 0) return -1;
    buf[buflen - 1] = '\0';
    return 0;
}

int prefs_set(const char *name, const char *value)
{
    char var[128];
    if (!name || !value) return -1;
    make_var(var, sizeof(var), name);
    /* GVF_GLOBAL_ONLY targets the global env; GVF_SAVE_VAR also writes ENVARC:. */
    return SetVar((STRPTR)var, (STRPTR)value, -1,
                  GVF_GLOBAL_ONLY | GVF_SAVE_VAR) ? 0 : -1;
}

#endif /* __amigaos__ */
