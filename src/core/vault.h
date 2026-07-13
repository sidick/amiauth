/* vault.h — encrypted multi-account store (encrypt-then-MAC: ChaCha20 + HMAC-SHA1).
 * Stub: see docs/ROADMAP.md Phase 2 and docs/SECURITY.md. */
#ifndef AMIAUTH_VAULT_H
#define AMIAUTH_VAULT_H

#include <stddef.h>
#include <stdint.h>

#include "uri.h"

#define VAULT_MAX_ACCOUNTS 64
#define VAULT_SALT_SIZE    16
#define VAULT_KEY_SIZE     32

typedef enum {
    VAULT_CIPHER_NONE     = 0,   /* always-unlocked mode: stored in the clear */
    VAULT_CIPHER_CHACHA20 = 1
} vault_cipher;

typedef enum {
    VAULT_OK          =    0,
    VAULT_ERR_IO      =   -1,
    VAULT_ERR_FORMAT  =   -2,
    VAULT_ERR_AUTH    =   -3,     /* wrong passphrase / MAC mismatch */
    VAULT_ERR_LOCKED  =   -4,
    VAULT_ERR_FULL    =   -5,
    VAULT_ERR_RANGE   =   -6,
    VAULT_ERR_NOTIMPL = -100
} vault_result;

typedef struct {
    vault_cipher cipher;
    uint32_t     iterations;                 /* PBKDF2 rounds (from header) */
    uint8_t      salt[VAULT_SALT_SIZE];
    int          unlocked;                   /* 1 when key is valid */
    size_t       count;
    otp_account  accounts[VAULT_MAX_ACCOUNTS];
    uint8_t      key[VAULT_KEY_SIZE];        /* derived; zeroed on lock/quit */
} vault;

/* Create a fresh in-memory vault. NULL/empty passphrase selects always-unlocked
 * mode (VAULT_CIPHER_NONE). */
vault_result vault_create(vault *v, const char *passphrase);

/* Load from disk. For an encrypted vault, `passphrase` derives the key and
 * verifies the MAC; VAULT_ERR_AUTH indicates a wrong passphrase. */
vault_result vault_load(vault *v, const char *path, const char *passphrase);

/* Serialise to disk in the current mode. */
vault_result vault_save(const vault *v, const char *path);

vault_result vault_add(vault *v, const otp_account *acct);
vault_result vault_remove(vault *v, size_t index);

/* Zero the derived key and mark locked. Always safe. */
void         vault_lock(vault *v);

/* Re-derive the key from `passphrase` and verify against the stored MAC. */
vault_result vault_unlock(vault *v, const char *passphrase);

#endif /* AMIAUTH_VAULT_H */
