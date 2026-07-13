/* vault.c — encrypted account store implementing docs/VAULT_FORMAT.md v1.
 *
 * On-disk: header(44) || mac(20) || ciphertext(N), encrypt-then-MAC with
 * ChaCha20 + HMAC-SHA1, keys split from one PBKDF2-HMAC-SHA1 output.
 *
 * Deterministic and portable: the caller supplies salt/nonce/iterations and a
 * file path. Not reentrant — it uses module-static work buffers (a single
 * vault is operated on at a time), which are zeroed after use. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vault.h"
#include "chacha20.h"
#include "pbkdf2.h"
#include "hmac.h"

/* --- on-disk header layout (byte offsets) --- */
#define OFF_MAGIC        0
#define OFF_VERSION      4
#define OFF_CIPHER       5
#define OFF_KDF          6
#define OFF_FLAGS        7
#define OFF_ITERATIONS   8
#define OFF_SALT        12
#define OFF_NONCE       28
#define OFF_PAYLOAD_LEN 40
#define HEADER_SIZE     44
#define MAC_OFFSET      HEADER_SIZE
#define CIPHERTEXT_OFFSET (HEADER_SIZE + VAULT_MAC_SIZE)   /* 64 */

static const uint8_t VAULT_MAGIC[4] = { 'A', 'A', 'V', 'T' };

/* Worst-case payload: count + max per-account record, times max accounts. */
#define VAULT_MAX_PAYLOAD (2 + VAULT_MAX_ACCOUNTS * 300)
#define VAULT_MAX_FILE    (CIPHERTEXT_OFFSET + VAULT_MAX_PAYLOAD)

static uint8_t g_payload[VAULT_MAX_PAYLOAD];   /* plaintext account list */
static uint8_t g_file[VAULT_MAX_FILE];         /* whole file image */

/* --- big-endian helpers --- */

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)(v);
}

static void put_u64(uint8_t *p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (56 - 8 * i));
}

static uint32_t get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

static uint64_t get_u64(const uint8_t *p)
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < 8; i++) v = (v << 8) | p[i];
    return v;
}

/* Constant-time equality. */
static int ct_eq(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t d = 0;
    size_t i;
    for (i = 0; i < n; i++) d |= (uint8_t)(a[i] ^ b[i]);
    return d == 0;
}

/* --- key derivation & MAC --- */

static void derive_keys(vault *v, const char *passphrase)
{
    uint8_t dk[VAULT_KEY_SIZE * 2];
    size_t plen = passphrase ? strlen(passphrase) : 0;

    pbkdf2_hmac_sha1((const uint8_t *)passphrase, plen,
                     v->salt, VAULT_SALT_SIZE, v->iterations,
                     dk, sizeof(dk));
    memcpy(v->enc_key, dk, VAULT_KEY_SIZE);
    memcpy(v->mac_key, dk + VAULT_KEY_SIZE, VAULT_KEY_SIZE);
    memset(dk, 0, sizeof(dk));
}

/* mac = HMAC-SHA1(mac_key, header || ciphertext). mac_key is empty when the
 * vault is unencrypted (integrity-only, per the format spec). */
static void compute_mac(const vault *v, const uint8_t *header,
                        const uint8_t *ciphertext, size_t clen,
                        uint8_t out[VAULT_MAC_SIZE])
{
    hmac_sha1_ctx c;
    size_t mklen = (v->kdf == VAULT_KDF_NONE) ? 0 : VAULT_KEY_SIZE;
    hmac_sha1_init(&c, v->mac_key, mklen);
    hmac_sha1_update(&c, header, HEADER_SIZE);
    hmac_sha1_update(&c, ciphertext, clen);
    hmac_sha1_final(&c, out);
}

/* --- payload (de)serialisation --- */

static size_t serialize_payload(const vault *v, uint8_t *out)
{
    size_t p = 0, i;

    out[p++] = (uint8_t)(v->count >> 8);
    out[p++] = (uint8_t)(v->count);

    for (i = 0; i < v->count; i++) {
        const otp_account *a = &v->accounts[i];
        size_t il = strlen(a->issuer);
        size_t ll = strlen(a->label);

        out[p++] = (uint8_t)(strcmp(a->type, "hotp") == 0 ? 1 : 0);
        out[p++] = 0;                          /* algorithm: SHA1 (v1) */
        out[p++] = (uint8_t)a->digits;
        put_u32(out + p, a->period);   p += 4;
        put_u64(out + p, a->counter);  p += 8;

        out[p++] = (uint8_t)a->secret_len;
        memcpy(out + p, a->secret, a->secret_len); p += a->secret_len;
        out[p++] = (uint8_t)il;
        memcpy(out + p, a->issuer, il); p += il;
        out[p++] = (uint8_t)ll;
        memcpy(out + p, a->label, ll);  p += ll;
    }
    return p;
}

static vault_result parse_payload(vault *v, const uint8_t *in, size_t len)
{
    size_t p = 0, i;
    uint16_t count;

    if (len < 2) return VAULT_ERR_FORMAT;
    count = (uint16_t)((in[0] << 8) | in[1]);
    p = 2;
    if (count > VAULT_MAX_ACCOUNTS) return VAULT_ERR_FORMAT;

    v->count = 0;
    for (i = 0; i < count; i++) {
        otp_account a;
        uint8_t type, sl, il, ll;

        memset(&a, 0, sizeof(a));
        if (p + 15 > len) return VAULT_ERR_FORMAT;   /* fixed fields */
        type      = in[p++];
        p++;                                          /* algorithm (SHA1 in v1) */
        a.digits  = in[p++];
        a.period  = get_u32(in + p); p += 4;
        a.counter = get_u64(in + p); p += 8;
        strcpy(a.type, type ? "hotp" : "totp");
        strcpy(a.algorithm, "SHA1");

        if (p + 1 > len) return VAULT_ERR_FORMAT;
        sl = in[p++];
        if (sl > OTP_MAX_SECRET || p + sl > len) return VAULT_ERR_FORMAT;
        memcpy(a.secret, in + p, sl); a.secret_len = sl; p += sl;

        if (p + 1 > len) return VAULT_ERR_FORMAT;
        il = in[p++];
        if (il >= sizeof(a.issuer) || p + il > len) return VAULT_ERR_FORMAT;
        memcpy(a.issuer, in + p, il); a.issuer[il] = '\0'; p += il;

        if (p + 1 > len) return VAULT_ERR_FORMAT;
        ll = in[p++];
        if (ll >= sizeof(a.label) || p + ll > len) return VAULT_ERR_FORMAT;
        memcpy(a.label, in + p, ll); a.label[ll] = '\0'; p += ll;

        v->accounts[v->count++] = a;
    }

    if (p != len) return VAULT_ERR_FORMAT;           /* no trailing garbage */
    return VAULT_OK;
}

/* --- lifecycle --- */

vault_result vault_create(vault *v, const char *passphrase,
                          uint32_t iterations, const uint8_t salt[VAULT_SALT_SIZE])
{
    if (!v) return VAULT_ERR_IO;
    memset(v, 0, sizeof(*v));

    if (passphrase && passphrase[0]) {
        if (!salt) return VAULT_ERR_IO;
        v->cipher     = VAULT_CIPHER_CHACHA20;
        v->kdf        = VAULT_KDF_PBKDF2_HMAC_SHA1;
        v->iterations = iterations ? iterations : VAULT_DEFAULT_ITERATIONS;
        memcpy(v->salt, salt, VAULT_SALT_SIZE);
        derive_keys(v, passphrase);
    } else {
        v->cipher = VAULT_CIPHER_NONE;   /* always-unlocked: no at-rest secrecy */
        v->kdf    = VAULT_KDF_NONE;
    }
    v->unlocked = 1;
    return VAULT_OK;
}

vault_result vault_set_passphrase(vault *v, const char *passphrase,
                                  uint32_t iterations,
                                  const uint8_t salt[VAULT_SALT_SIZE])
{
    if (!v) return VAULT_ERR_IO;
    if (!v->unlocked) return VAULT_ERR_LOCKED;   /* need plaintext to re-key */

    memset(v->enc_key, 0, sizeof(v->enc_key));
    memset(v->mac_key, 0, sizeof(v->mac_key));

    if (passphrase && passphrase[0]) {
        if (!salt) return VAULT_ERR_IO;
        v->cipher     = VAULT_CIPHER_CHACHA20;
        v->kdf        = VAULT_KDF_PBKDF2_HMAC_SHA1;
        v->iterations = iterations ? iterations : VAULT_DEFAULT_ITERATIONS;
        memcpy(v->salt, salt, VAULT_SALT_SIZE);
        derive_keys(v, passphrase);
    } else {
        v->cipher     = VAULT_CIPHER_NONE;
        v->kdf        = VAULT_KDF_NONE;
        v->iterations = 0;
        memset(v->salt, 0, sizeof(v->salt));
    }
    return VAULT_OK;
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
    memset(v->enc_key, 0, sizeof(v->enc_key));
    memset(v->mac_key, 0, sizeof(v->mac_key));
    if (v->cipher != VAULT_CIPHER_NONE) {
        memset(v->accounts, 0, sizeof(v->accounts));
        v->count = 0;
        v->unlocked = 0;
    }
}

/* --- persistence --- */

vault_result vault_save(const vault *v, const char *path,
                        const uint8_t nonce[VAULT_NONCE_SIZE])
{
    uint8_t header[HEADER_SIZE];
    uint8_t mac[VAULT_MAC_SIZE];
    size_t plen;
    FILE *f;
    char *tmp;
    size_t pathlen;
    int ok;

    if (!v || !path) return VAULT_ERR_IO;
    if (!v->unlocked) return VAULT_ERR_LOCKED;
    if (v->cipher == VAULT_CIPHER_CHACHA20 && !nonce) return VAULT_ERR_IO;

    plen = serialize_payload(v, g_payload);

    /* header */
    memset(header, 0, sizeof(header));
    memcpy(header + OFF_MAGIC, VAULT_MAGIC, 4);
    header[OFF_VERSION] = VAULT_FORMAT_VERSION;
    header[OFF_CIPHER]  = (uint8_t)v->cipher;
    header[OFF_KDF]     = (uint8_t)v->kdf;
    header[OFF_FLAGS]   = 0;
    put_u32(header + OFF_ITERATIONS, v->iterations);
    memcpy(header + OFF_SALT, v->salt, VAULT_SALT_SIZE);
    if (v->cipher == VAULT_CIPHER_CHACHA20)
        memcpy(header + OFF_NONCE, nonce, VAULT_NONCE_SIZE);
    put_u32(header + OFF_PAYLOAD_LEN, (uint32_t)plen);

    /* encrypt in place (payload -> ciphertext) */
    if (v->cipher == VAULT_CIPHER_CHACHA20)
        chacha20_xor(v->enc_key, nonce, 0, g_payload, g_payload, plen);

    compute_mac(v, header, g_payload, plen, mac);

    /* assemble the file image */
    memcpy(g_file + OFF_MAGIC, header, HEADER_SIZE);
    memcpy(g_file + MAC_OFFSET, mac, VAULT_MAC_SIZE);
    memcpy(g_file + CIPHERTEXT_OFFSET, g_payload, plen);

    /* atomic replace: write a temp file then rename over the target */
    pathlen = strlen(path);
    tmp = (char *)malloc(pathlen + 5);
    if (!tmp) { memset(g_payload, 0, plen); return VAULT_ERR_IO; }
    memcpy(tmp, path, pathlen);
    memcpy(tmp + pathlen, ".tmp", 5);

    ok = 0;
    f = fopen(tmp, "wb");
    if (f) {
        size_t total = CIPHERTEXT_OFFSET + plen;
        ok = (fwrite(g_file, 1, total, f) == total);
        if (fclose(f) != 0) ok = 0;
    }

    memset(g_payload, 0, plen);
    memset(g_file, 0, CIPHERTEXT_OFFSET + plen);

    if (!ok) { remove(tmp); free(tmp); return VAULT_ERR_IO; }

    if (rename(tmp, path) != 0) {
        remove(path);                     /* AmigaOS Rename won't overwrite */
        if (rename(tmp, path) != 0) { remove(tmp); free(tmp); return VAULT_ERR_IO; }
    }
    free(tmp);
    return VAULT_OK;
}

vault_result vault_load(vault *v, const char *path, const char *passphrase)
{
    FILE *f;
    long fsize;
    size_t nread, plen;
    uint8_t mac[VAULT_MAC_SIZE];
    uint8_t cipher, kdf;
    vault_result rc;

    if (!v || !path) return VAULT_ERR_IO;

    f = fopen(path, "rb");
    if (!f) return VAULT_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return VAULT_ERR_IO; }
    fsize = ftell(f);
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return VAULT_ERR_IO; }
    if (fsize < CIPHERTEXT_OFFSET || fsize > VAULT_MAX_FILE) {
        fclose(f);
        return VAULT_ERR_FORMAT;
    }
    nread = fread(g_file, 1, (size_t)fsize, f);
    fclose(f);
    if (nread != (size_t)fsize) return VAULT_ERR_FORMAT;

    if (memcmp(g_file + OFF_MAGIC, VAULT_MAGIC, 4) != 0 ||
        g_file[OFF_VERSION] != VAULT_FORMAT_VERSION)
        return VAULT_ERR_FORMAT;

    cipher = g_file[OFF_CIPHER];
    kdf    = g_file[OFF_KDF];
    if (cipher > VAULT_CIPHER_CHACHA20 || kdf > VAULT_KDF_PBKDF2_HMAC_SHA1)
        return VAULT_ERR_FORMAT;
    /* cipher and kdf must both be present or both absent */
    if ((cipher == VAULT_CIPHER_NONE) != (kdf == VAULT_KDF_NONE))
        return VAULT_ERR_FORMAT;

    plen = get_u32(g_file + OFF_PAYLOAD_LEN);
    if ((size_t)fsize != CIPHERTEXT_OFFSET + plen || plen > VAULT_MAX_PAYLOAD)
        return VAULT_ERR_FORMAT;

    /* populate header-derived fields */
    memset(v, 0, sizeof(*v));
    v->cipher     = (vault_cipher)cipher;
    v->kdf        = (vault_kdf)kdf;
    v->iterations = get_u32(g_file + OFF_ITERATIONS);
    memcpy(v->salt, g_file + OFF_SALT, VAULT_SALT_SIZE);

    if (v->kdf == VAULT_KDF_PBKDF2_HMAC_SHA1) {
        if (!passphrase) return VAULT_ERR_LOCKED;   /* header known, still locked */
        derive_keys(v, passphrase);
    }

    /* verify MAC over header || ciphertext before touching the ciphertext */
    compute_mac(v, g_file + OFF_MAGIC, g_file + CIPHERTEXT_OFFSET, plen, mac);
    if (!ct_eq(mac, g_file + MAC_OFFSET, VAULT_MAC_SIZE)) {
        memset(v, 0, sizeof(*v));
        return VAULT_ERR_AUTH;
    }

    /* recover the plaintext payload */
    if (v->cipher == VAULT_CIPHER_CHACHA20)
        chacha20_xor(v->enc_key, g_file + OFF_NONCE, 0,
                     g_file + CIPHERTEXT_OFFSET, g_payload, plen);
    else
        memcpy(g_payload, g_file + CIPHERTEXT_OFFSET, plen);

    rc = parse_payload(v, g_payload, plen);

    memset(g_payload, 0, plen);
    memset(g_file, 0, CIPHERTEXT_OFFSET + plen);

    if (rc != VAULT_OK) { memset(v, 0, sizeof(*v)); return rc; }
    v->unlocked = 1;
    return VAULT_OK;
}
