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

/* Prompt and read a passphrase with no echo, using RAW console mode. Each
 * keystroke's arrival time is stirred into the entropy pool. Returns 0 on
 * success, -1 if there is no interactive console or on error. */
int amiga_read_passphrase(const char *prompt, char *buf, size_t cap);

#endif /* AMIAUTH_AMIGA_ENTROPY_H */
