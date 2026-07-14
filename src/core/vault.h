/* vault.h — encrypted multi-account store (encrypt-then-MAC: ChaCha20 + HMAC-SHA1).
 * On-disk layout is frozen in docs/VAULT_FORMAT.md.
 *
 * The core is deterministic and platform-agnostic: the caller supplies the
 * iteration count and the random salt/nonce (a CSPRNG lives in the Amiga
 * front-end), and an explicit file path (PROGDIR:/ENVARC: resolution is a
 * front-end concern — see docs/STORAGE.md). See also docs/SECURITY.md. */
#ifndef AMIAUTH_VAULT_H
#define AMIAUTH_VAULT_H

#include <stddef.h>
#include <stdint.h>

#include "uri.h"

#define VAULT_MAX_ACCOUNTS   64
#define VAULT_SALT_SIZE      16
#define VAULT_NONCE_SIZE     12
#define VAULT_KEY_SIZE       32          /* each of enc_key / mac_key */
#define VAULT_MAC_SIZE       20
#define VAULT_FORMAT_VERSION  1

/* Provisional policy default for the PBKDF2 iteration count. It is stored per
 * vault, so this only affects newly-created vaults. Chosen conservatively
 * because vaults are portable across a 100-1000x CPU range; the front-end will
 * calibrate-and-cap to the local machine (Phase 4). See SECURITY.md. */
#define VAULT_DEFAULT_ITERATIONS 10000u

/* KDF calibration policy. The front-end measures local PBKDF2 speed and calls
 * vault_calibrate_iterations to pick a per-machine count (~KDF_TARGET_MS unlock);
 * MIN/MAX are only guards, not the policy — a stock 68000 (~14 iters/s) honestly
 * lands near MIN and relies on the passphrase (see docs/SECURITY.md). */
#define KDF_TARGET_MS       1000u      /* aim ~1s unlock on the creating machine */
#define KDF_MIN_ITERATIONS  1u         /* guard against a bad measurement */
#define KDF_MAX_ITERATIONS  4000000u   /* anti-pathological ceiling */

/* Iteration count that would take ~KDF_TARGET_MS, extrapolated from a probe of
 * `probe_iters` rounds measured at `probe_ms` ms; `probe_ms == 0` (too fast to
 * time) yields KDF_MAX_ITERATIONS. Clamped to [MIN, MAX]. Pure and deterministic
 * (host-testable); the timing itself is a front-end concern. */
uint32_t vault_calibrate_iterations(uint32_t probe_iters, uint32_t probe_ms);

typedef enum {
    VAULT_CIPHER_NONE     = 0,   /* always-unlocked mode: stored in the clear */
    VAULT_CIPHER_CHACHA20 = 1
} vault_cipher;

typedef enum {
    VAULT_KDF_NONE            = 0,
    VAULT_KDF_PBKDF2_HMAC_SHA1 = 1
} vault_kdf;

typedef enum {
    VAULT_OK          =    0,
    VAULT_ERR_IO      =   -1,
    VAULT_ERR_FORMAT  =   -2,     /* bad magic/version/length or malformed payload */
    VAULT_ERR_AUTH    =   -3,     /* wrong passphrase / MAC mismatch / tampering */
    VAULT_ERR_LOCKED  =   -4,
    VAULT_ERR_FULL    =   -5,
    VAULT_ERR_RANGE   =   -6,
    VAULT_ERR_NOTIMPL = -100
} vault_result;

typedef struct {
    vault_cipher cipher;
    vault_kdf    kdf;
    uint32_t     iterations;                 /* PBKDF2 rounds (0 when kdf none) */
    uint8_t      salt[VAULT_SALT_SIZE];      /* zero when kdf none */
    int          unlocked;                   /* 1 when accounts/keys are valid */
    size_t       count;
    otp_account  accounts[VAULT_MAX_ACCOUNTS];
    uint8_t      enc_key[VAULT_KEY_SIZE];    /* derived; zeroed on lock/quit */
    uint8_t      mac_key[VAULT_KEY_SIZE];    /* derived; zeroed on lock/quit */
} vault;

/* Create a fresh in-memory (unlocked) vault.
 *   passphrase NULL/empty -> always-unlocked mode (salt/iterations ignored).
 *   otherwise             -> ChaCha20 + PBKDF2; `salt` (16 random bytes) is
 *                            required and `iterations` (0 selects the default)
 *                            is stored in the header. */
vault_result vault_create(vault *v, const char *passphrase,
                          uint32_t iterations, const uint8_t salt[VAULT_SALT_SIZE]);

/* Load from disk. Always parses the header. For an encrypted vault, a NULL
 * passphrase returns VAULT_ERR_LOCKED with the header fields populated; a
 * supplied passphrase derives the key, verifies the MAC (constant time) and
 * decrypts — VAULT_ERR_AUTH on a wrong passphrase or tampering. */
vault_result vault_load(vault *v, const char *path, const char *passphrase);

/* Serialise to disk (atomic replace). `nonce` must be 12 fresh random bytes for
 * an encrypted vault; it is ignored (may be NULL) in always-unlocked mode. */
vault_result vault_save(const vault *v, const char *path,
                        const uint8_t nonce[VAULT_NONCE_SIZE]);

vault_result vault_add(vault *v, const otp_account *acct);
vault_result vault_remove(vault *v, size_t index);

/* Add / change / remove the passphrase, converting between encrypted and
 * always-unlocked in place (a subsequent vault_save writes the new form).
 * Requires the vault to be unlocked. Same argument rules as vault_create. */
vault_result vault_set_passphrase(vault *v, const char *passphrase,
                                  uint32_t iterations,
                                  const uint8_t salt[VAULT_SALT_SIZE]);

/* Zero the derived keys. For an encrypted vault this also clears the decrypted
 * accounts and marks it locked (reopen via vault_load). Always-unlocked vaults
 * have nothing to lock, so this is a no-op there. */
void vault_lock(vault *v);

#endif /* AMIAUTH_VAULT_H */
