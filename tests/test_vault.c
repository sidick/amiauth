/* test_vault.c — vault format round-trip, auth, tamper and conversion tests.
 *
 * These write to a temp file; the path is provided by the harness via an env
 * var (VAULT_TEST_FILE) set in the Makefile, falling back to the CWD. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "vault.h"
#include "hmac.h"

static const char *tmp_path(void)
{
    const char *p = getenv("VAULT_TEST_FILE");
    return p ? p : "amiauth-test.vault";
}

/* Fixed but arbitrary salt/nonce for deterministic tests. */
static const uint8_t SALT[VAULT_SALT_SIZE] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const uint8_t NONCE[VAULT_NONCE_SIZE] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b
};

static void sample_account(otp_account *a)
{
    memset(a, 0, sizeof(*a));
    strcpy(a->type, "totp");
    strcpy(a->algorithm, "SHA1");
    strcpy(a->issuer, "GitHub");
    strcpy(a->label, "alice@example.com");
    /* secret "12345678901234567890" */
    memcpy(a->secret, "12345678901234567890", 20);
    a->secret_len = 20;
    a->digits = 6;
    a->period = 30;
    a->counter = 0;
}

static int accounts_equal(const otp_account *x, const otp_account *y)
{
    return strcmp(x->type, y->type) == 0
        && strcmp(x->algorithm, y->algorithm) == 0
        && strcmp(x->issuer, y->issuer) == 0
        && strcmp(x->label, y->label) == 0
        && x->secret_len == y->secret_len
        && memcmp(x->secret, y->secret, x->secret_len) == 0
        && x->digits == y->digits
        && x->period == y->period
        && x->counter == y->counter;
}

/* Hand-write a minimal always-unlocked vault (cipher/kdf = none) from a
 * pre-built payload, computing the empty-key HMAC-SHA1 MAC the format
 * requires — for exercising parse_payload's per-record rejection rules
 * directly, without vault_add's own validation getting in the way. */
static void write_raw_vault(const char *path, const uint8_t *payload, size_t plen)
{
    uint8_t header[44];
    uint8_t mac[20];
    FILE *f;

    memset(header, 0, sizeof(header));
    memcpy(header, "AAVT", 4);
    header[4] = 0x01;                          /* format_version */
    header[43] = (uint8_t)plen;                /* payload_len (u32 BE, plen < 256 here) */

    {
        hmac_sha1_ctx m;
        hmac_sha1_init(&m, NULL, 0);
        hmac_sha1_update(&m, header, sizeof(header));
        hmac_sha1_update(&m, payload, plen);
        hmac_sha1_final(&m, mac);
    }

    f = fopen(path, "wb");
    TEST_CHECK(f != NULL);
    if (f) {
        fwrite(header, 1, sizeof(header), f);
        fwrite(mac, 1, sizeof(mac), f);
        fwrite(payload, 1, plen, f);
        fclose(f);
    }
}

void run_vault_tests(void)
{
    const char *path = tmp_path();
    otp_account a;
    vault v, w;

    sample_account(&a);

    /* --- encrypted round trip --- */
    TEST_CHECK(vault_create(&v, "correct horse", 4096, SALT) == VAULT_OK);
    TEST_CHECK(v.cipher == VAULT_CIPHER_CHACHA20 && v.iterations == 4096);
    TEST_CHECK(vault_add(&v, &a) == VAULT_OK);
    TEST_CHECK(vault_save(&v, path, NONCE) == VAULT_OK);

    TEST_CHECK(vault_load(&w, path, "correct horse") == VAULT_OK);
    TEST_CHECK(w.count == 1);
    TEST_CHECK(accounts_equal(&w.accounts[0], &a));
    TEST_CHECK(w.iterations == 4096);

    /* --- wrong passphrase --- */
    TEST_CHECK(vault_load(&w, path, "wrong passphrase") == VAULT_ERR_AUTH);

    /* --- locked: header known, no accounts, until a passphrase is given --- */
    TEST_CHECK(vault_load(&w, path, NULL) == VAULT_ERR_LOCKED);

    /* --- tamper detection: flip one ciphertext byte --- */
    {
        FILE *f = fopen(path, "r+b");
        long sz;
        int c;
        TEST_CHECK(f != NULL);
        fseek(f, 0, SEEK_END); sz = ftell(f);
        fseek(f, sz - 1, SEEK_SET);         /* last ciphertext byte */
        c = fgetc(f);
        fseek(f, sz - 1, SEEK_SET);
        fputc(c ^ 0x01, f);
        fclose(f);
        TEST_CHECK(vault_load(&w, path, "correct horse") == VAULT_ERR_AUTH);
    }

    /* --- always-unlocked round trip --- */
    TEST_CHECK(vault_create(&v, NULL, 0, NULL) == VAULT_OK);
    TEST_CHECK(v.cipher == VAULT_CIPHER_NONE && v.unlocked == 1);
    TEST_CHECK(vault_add(&v, &a) == VAULT_OK);
    TEST_CHECK(vault_save(&v, path, NULL) == VAULT_OK);
    TEST_CHECK(vault_load(&w, path, NULL) == VAULT_OK);   /* no passphrase needed */
    TEST_CHECK(w.count == 1 && accounts_equal(&w.accounts[0], &a));
    /* always-unlocked vaults never lock */
    vault_lock(&w);
    TEST_CHECK(w.unlocked == 1);

    /* --- convert unlocked -> encrypted, and back --- */
    TEST_CHECK(vault_set_passphrase(&v, "new secret", 2048, SALT) == VAULT_OK);
    TEST_CHECK(v.cipher == VAULT_CIPHER_CHACHA20 && v.iterations == 2048);
    TEST_CHECK(vault_save(&v, path, NONCE) == VAULT_OK);
    TEST_CHECK(vault_load(&w, path, "new secret") == VAULT_OK);
    TEST_CHECK(w.count == 1 && accounts_equal(&w.accounts[0], &a));

    TEST_CHECK(vault_set_passphrase(&v, NULL, 0, NULL) == VAULT_OK);
    TEST_CHECK(v.cipher == VAULT_CIPHER_NONE);
    TEST_CHECK(vault_save(&v, path, NULL) == VAULT_OK);
    TEST_CHECK(vault_load(&w, path, NULL) == VAULT_OK);
    TEST_CHECK(w.count == 1 && accounts_equal(&w.accounts[0], &a));

    /* --- SHA-256/512 accounts round-trip via algorithm ids 1/2 (#43) --- */
    {
        otp_account b, c;
        sample_account(&b);
        strcpy(b.algorithm, "SHA256");
        sample_account(&c);
        strcpy(c.algorithm, "SHA512");
        strcpy(c.label, "bob@example.com");

        TEST_CHECK(vault_create(&v, NULL, 0, NULL) == VAULT_OK);
        TEST_CHECK(vault_add(&v, &b) == VAULT_OK);
        TEST_CHECK(vault_add(&v, &c) == VAULT_OK);
        TEST_CHECK(vault_save(&v, path, NULL) == VAULT_OK);
        TEST_CHECK(vault_load(&w, path, NULL) == VAULT_OK);
        TEST_CHECK(w.count == 2);
        TEST_CHECK(accounts_equal(&w.accounts[0], &b));
        TEST_CHECK(accounts_equal(&w.accounts[1], &c));
    }

    /* --- a Steam Guard account round-trips via type id 2 (#44) --- */
    {
        otp_account s;
        sample_account(&s);
        strcpy(s.type, "steam");
        s.digits = 5;
        s.period = 30;

        TEST_CHECK(vault_create(&v, NULL, 0, NULL) == VAULT_OK);
        TEST_CHECK(vault_add(&v, &s) == VAULT_OK);
        TEST_CHECK(vault_save(&v, path, NULL) == VAULT_OK);
        TEST_CHECK(vault_load(&w, path, NULL) == VAULT_OK);
        TEST_CHECK(w.count == 1);
        TEST_CHECK(accounts_equal(&w.accounts[0], &s));
    }

    /* --- an algorithm id we don't implement is refused (format rule) --- */
    {
        /* One record claiming algorithm id 9, with a valid (empty-key) MAC so
         * only the id check can reject it. Payload: count=1; type 0, alg 9,
         * digits 6, period 30, counter 0, secret "K", no issuer, label "x". */
        static const uint8_t payload[] = {
            0x00, 0x01,
            0x00, 0x09, 0x06,
            0x00, 0x00, 0x00, 0x1e,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 'K',
            0x00,
            0x01, 'x'
        };
        write_raw_vault(path, payload, sizeof(payload));
        TEST_CHECK(vault_load(&w, path, NULL) == VAULT_ERR_FORMAT);
    }

    /* --- a type id we don't implement is refused the same way (#44) --- */
    {
        /* Same record, but type 9 (valid algorithm 0) instead. */
        static const uint8_t payload[] = {
            0x00, 0x01,
            0x09, 0x00, 0x06,
            0x00, 0x00, 0x00, 0x1e,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 'K',
            0x00,
            0x01, 'x'
        };
        write_raw_vault(path, payload, sizeof(payload));
        TEST_CHECK(vault_load(&w, path, NULL) == VAULT_ERR_FORMAT);
    }

    /* --- encrypted lock clears secrets --- */
    TEST_CHECK(vault_create(&v, "pw", 1024, SALT) == VAULT_OK);
    TEST_CHECK(vault_add(&v, &a) == VAULT_OK);
    vault_lock(&v);
    TEST_CHECK(v.unlocked == 0 && v.count == 0);

    /* --- golden fixture: byte-exact format lock ---
     * Guards against silent format drift. The expected bytes were verified
     * independently against Python's PBKDF2/HMAC-SHA1 and OpenSSL's ChaCha20.
     * Inputs: passphrase "golden", 4096 iters, SALT, NONCE, and one TOTP
     * account (issuer "GH", label "a", secret "12345678901234567890"). */
    {
        static const char GOLDEN_HEX[] =
            "414156540101010000001000"                 /* magic|ver|cipher|kdf|flags|iters */
            "101112131415161718191a1b1c1d1e1f"         /* salt */
            "202122232425262728292a2b"                 /* nonce */
            "0000002b"                                  /* payload_len = 43 */
            "8d5b3358ef283c53655e55edbb96a4f00fdf7b4a" /* mac */
            "af19c1d4b53d4d6c6efba7b5aaaae8345fe10ba7"
            "f4cf5eabbc24eefd414b2747d8356c6c621c1ec3"
            "05460c";                                   /* ciphertext (43 bytes) */
        otp_account g;
        uint8_t filebuf[128];
        FILE *f;
        size_t n;

        memset(&g, 0, sizeof(g));
        strcpy(g.type, "totp"); strcpy(g.algorithm, "SHA1");
        strcpy(g.issuer, "GH"); strcpy(g.label, "a");
        memcpy(g.secret, "12345678901234567890", 20); g.secret_len = 20;
        g.digits = 6; g.period = 30; g.counter = 0;

        TEST_CHECK(vault_create(&v, "golden", 4096, SALT) == VAULT_OK);
        TEST_CHECK(vault_add(&v, &g) == VAULT_OK);
        TEST_CHECK(vault_save(&v, path, NONCE) == VAULT_OK);

        f = fopen(path, "rb");
        TEST_CHECK(f != NULL);
        n = f ? fread(filebuf, 1, sizeof(filebuf), f) : 0;
        if (f) fclose(f);
        TEST_CHECK(n == 107);
        TEST_CHECK(hex_eq(filebuf, n, GOLDEN_HEX));
    }

    remove(path);
}
