/* vault.c — encrypted account store. STUB: implement in Phase 2.
 * On-disk: header (magic, version, cipher, iterations, salt) + MAC + ciphertext,
 * encrypt-then-MAC with ChaCha20 + HMAC-SHA1. See docs/SECURITY.md. */
#include <string.h>

#include "vault.h"
#include "chacha20.h"
#include "pbkdf2.h"
#include "hmac.h"

vault_result vault_create(vault *v, const char *passphrase)
{
    if (!v) return VAULT_ERR_IO;
    memset(v, 0, sizeof(*v));
    if (passphrase && passphrase[0]) {
        v->cipher = VAULT_CIPHER_CHACHA20;
        /* TODO: generate salt, calibrate iterations, derive key, mark unlocked. */
        return VAULT_ERR_NOTIMPL;
    }
    /* Always-unlocked mode: no at-rest protection (docs/SECURITY.md). */
    v->cipher   = VAULT_CIPHER_NONE;
    v->unlocked = 1;
    return VAULT_OK;
}

vault_result vault_load(vault *v, const char *path, const char *passphrase)
{
    (void)v; (void)path; (void)passphrase;
    /* TODO: read header, derive key (if encrypted), verify MAC, decrypt. */
    return VAULT_ERR_NOTIMPL;
}

vault_result vault_save(const vault *v, const char *path)
{
    (void)v; (void)path;
    /* TODO: serialise accounts, encrypt-then-MAC, write header + body. */
    return VAULT_ERR_NOTIMPL;
}

vault_result vault_add(vault *v, const otp_account *acct)
{
    if (!v || !acct) return VAULT_ERR_IO;
    if (!v->unlocked) return VAULT_ERR_LOCKED;
    if (v->count >= VAULT_MAX_ACCOUNTS) return VAULT_ERR_FULL;
    v->accounts[v->count++] = *acct;
    return VAULT_OK;
}

vault_result vault_remove(vault *v, size_t index)
{
    if (!v) return VAULT_ERR_IO;
    if (!v->unlocked) return VAULT_ERR_LOCKED;
    if (index >= v->count) return VAULT_ERR_RANGE;
    memmove(&v->accounts[index], &v->accounts[index + 1],
            (v->count - index - 1) * sizeof(v->accounts[0]));
    v->count--;
    memset(&v->accounts[v->count], 0, sizeof(v->accounts[0]));
    return VAULT_OK;
}

void vault_lock(vault *v)
{
    if (!v) return;
    memset(v->key, 0, sizeof(v->key));
    if (v->cipher != VAULT_CIPHER_NONE)
        v->unlocked = 0;
}

vault_result vault_unlock(vault *v, const char *passphrase)
{
    (void)passphrase;
    if (!v) return VAULT_ERR_IO;
    if (v->cipher == VAULT_CIPHER_NONE) return VAULT_OK;
    /* TODO: derive key from passphrase+salt, verify against stored MAC. */
    return VAULT_ERR_NOTIMPL;
}
