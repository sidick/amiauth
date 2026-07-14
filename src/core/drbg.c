/* drbg.c — HMAC-DRBG over SHA-1 (NIST SP 800-90A, section 10.1.2).
 *
 * additional_input and prediction resistance are not used: the caller reseeds
 * explicitly via drbg_reseed when it has fresh entropy. State is (K, V), each
 * one SHA-1 digest wide. */
#include "drbg.h"

#include <string.h>

#include "hmac.h"

/* V = HMAC(K, V). */
static void hmac_kv(drbg_state *st)
{
    uint8_t v[SHA1_DIGEST_SIZE];
    hmac_sha1(st->K, sizeof st->K, st->V, sizeof st->V, v);
    memcpy(st->V, v, sizeof st->V);
}

/* HMAC_DRBG_Update (SP 800-90A 10.1.2.2). With empty data this is the
 * post-generate state advance; with data it is instantiate/reseed. */
static void drbg_update(drbg_state *st, const uint8_t *data, size_t len)
{
    hmac_sha1_ctx h;
    uint8_t sep;

    /* K = HMAC(K, V || 0x00 || data);  V = HMAC(K, V) */
    hmac_sha1_init(&h, st->K, sizeof st->K);
    hmac_sha1_update(&h, st->V, sizeof st->V);
    sep = 0x00;
    hmac_sha1_update(&h, &sep, 1);
    if (len) hmac_sha1_update(&h, data, len);
    hmac_sha1_final(&h, st->K);
    hmac_kv(st);

    if (len == 0) return;         /* no provided data -> stop after the first round */

    /* K = HMAC(K, V || 0x01 || data);  V = HMAC(K, V) */
    hmac_sha1_init(&h, st->K, sizeof st->K);
    hmac_sha1_update(&h, st->V, sizeof st->V);
    sep = 0x01;
    hmac_sha1_update(&h, &sep, 1);
    hmac_sha1_update(&h, data, len);
    hmac_sha1_final(&h, st->K);
    hmac_kv(st);
}

void drbg_init(drbg_state *st, const uint8_t *seed, size_t seedlen)
{
    memset(st->K, 0x00, sizeof st->K);
    memset(st->V, 0x01, sizeof st->V);
    drbg_update(st, seed, seedlen);
}

void drbg_reseed(drbg_state *st, const uint8_t *in, size_t inlen)
{
    drbg_update(st, in, inlen);
}

void drbg_generate(drbg_state *st, uint8_t *out, size_t n)
{
    size_t got = 0;
    while (got < n) {
        size_t take = n - got;
        hmac_kv(st);
        if (take > sizeof st->V) take = sizeof st->V;
        memcpy(out + got, st->V, take);
        got += take;
    }
    drbg_update(st, NULL, 0);      /* advance state (additional_input empty) */
}
