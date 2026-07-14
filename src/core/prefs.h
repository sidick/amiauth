/* prefs.h — small persisted settings store.
 *
 * On AmigaOS these are ENV:/ENVARC:AmiAuth/<name> variables (dos.library
 * GetVar/SetVar with GVF_SAVE), so they persist across reboots and are
 * Shell-inspectable — see docs/STORAGE.md. On the host they are one file per key
 * under $AMIAUTH_PREFS_DIR (default: ./amiauth-prefs), which mirrors that model
 * for testing. Keys are bare names like "server" or "offset". */
#ifndef AMIAUTH_PREFS_H
#define AMIAUTH_PREFS_H

#include <stddef.h>

/* Read a setting into buf (NUL-terminated). Returns 0 if found, -1 otherwise. */
int prefs_get(const char *name, char *buf, size_t buflen);

/* Write a setting, persisting it. Returns 0 on success, -1 on failure. */
int prefs_set(const char *name, const char *value);

/* Signed-integer convenience (stored as decimal text). */
int prefs_get_long(const char *name, long *out);
int prefs_set_long(const char *name, long value);

#endif /* AMIAUTH_PREFS_H */
