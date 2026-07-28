/* entropy.h — AmigaOS CSPRNG + secure passphrase input (front-end hooks).
 *
 * Implemented in src/amiga/random.c and linked into the m68k build only; the
 * host CLI uses /dev/urandom and /dev/tty instead. The core stays deterministic
 * and takes salt/nonce as parameters (see src/core/vault.h). */
#ifndef AMIAUTH_AMIGA_ENTROPY_H
#define AMIAUTH_AMIGA_ENTROPY_H

#include <stddef.h>
#include <stdint.h>

/* Fill buf with n cryptographically-random bytes. Returns 0 (always succeeds:
 * best-effort entropy whitened through an HMAC-DRBG — see docs/SECURITY.md). */
int amiga_random(uint8_t *buf, size_t n);

/* Fold an application-supplied sample into the entropy pool. */
void amiga_entropy_stir(const void *p, size_t n);

/* Fold the first n bytes (capped at 128) of a file into the entropy pool.
 * Called with the vault path before each save: the previous file's
 * header+MAC region includes the last random nonce, so every save's fresh
 * nonce is chained to the one before it - two runs from an identical
 * cold-boot state (deterministic emulator, frozen RTC) still diverge at
 * their second-ever save, and any save after real entropy existed keeps
 * that divergence forever. A missing file folds nothing. */
void amiga_stir_file(const char *path, size_t n);

/* Fold a fresh E-clock reading into the entropy pool — call this on every
 * keystroke of any UI's own passphrase input loop (amiga_read_passphrase()
 * already does this internally; front ends with their own event-driven
 * keystroke handling, e.g. the GUI, must call it explicitly per keystroke). */
void amiga_stir_keystroke(void);

/* Prompt and read a passphrase with no echo, using RAW console mode. Each
 * keystroke's arrival time is stirred into the entropy pool. Returns 0 on
 * success, -1 if there is no interactive console or on error. */
int amiga_read_passphrase(const char *prompt, char *buf, size_t cap);

/* Monotonic milliseconds from timer.device E-clock (for KDF calibration).
 * Returns 0 if no timer is available. */
uint32_t amiga_millis(void);

/* Close the shared timer.device unit/port and scrub the RNG state. Exec has
 * no resource tracking, so both front-ends must call this on exit — without
 * it every run leaks the port + IORequest until reboot. Safe to call when
 * nothing was ever opened; entropy calls after it simply reopen. */
void amiga_entropy_cleanup(void);

/* Prompt and read a line with normal echo (for y/N re-key confirmations).
 * Returns 0 on success, -1 if there is no interactive console or on EOF. */
int amiga_read_line(const char *prompt, char *buf, size_t cap);

#endif /* AMIAUTH_AMIGA_ENTROPY_H */
