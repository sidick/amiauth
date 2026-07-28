/* uri.c — otpauth:// URI parsing (Key Uri Format).
 *   otpauth://TYPE/LABEL?secret=...&issuer=...&algorithm=...&digits=...
 *            &period=...&counter=...
 * LABEL is "accountname" or "issuer:accountname", percent-encoded.
 * Validated against tests/test_uri.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uri.h"
#include "otp.h"
#include "base32.h"
#include "steamguard.h"

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* ASCII case-insensitive compare of exactly n bytes. */
static int ci_eq(const char *a, const char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        int ca = (unsigned char)a[i], cb = (unsigned char)b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
    }
    return 1;
}

/* Case-insensitive prefix test, NUL-safe on `s`. */
static int ci_startswith(const char *s, const char *prefix)
{
    while (*prefix) {
        int cs = (unsigned char)*s, cp = (unsigned char)*prefix;
        if (!cs) return 0;
        if (cs >= 'A' && cs <= 'Z') cs += 32;
        if (cp >= 'A' && cp <= 'Z') cp += 32;
        if (cs != cp) return 0;
        s++; prefix++;
    }
    return 1;
}

static int key_is(const char *k, size_t klen, const char *name)
{
    return strlen(name) == klen && ci_eq(k, name, klen);
}

/* Percent-decode src[0,srclen) into dst (NUL-terminated, bounded by dstcap). */
static void url_decode(const char *src, size_t srclen, char *dst, size_t dstcap)
{
    size_t i = 0, o = 0;
    while (i < srclen && o + 1 < dstcap) {
        int c = (unsigned char)src[i];
        if (c == '%' && i + 2 < srclen) {
            int h1 = hexval((unsigned char)src[i + 1]);
            int h2 = hexval((unsigned char)src[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                dst[o++] = (char)((h1 << 4) | h2);
                i += 3;
                continue;
            }
        }
        dst[o++] = (char)c;
        i++;
    }
    dst[o] = '\0';
}

static void copy_str(char *dst, size_t dstcap, const char *src)
{
    size_t i = 0;
    while (src[i] && i + 1 < dstcap) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* Append `src` to *cur (advancing it and shrinking *remain), bounds-checked.
 * Used to build a URI without ever risking an overflow. */
static int str_append(char **cur, size_t *remain, const char *src)
{
    size_t len = strlen(src);
    if (len >= *remain) return -1;
    memcpy(*cur, src, len);
    *cur += len;
    *remain -= len;
    return 0;
}

/* Percent-encode src and append it (RFC 3986 unreserved set passes through
 * unescaped; everything else becomes %XX). Inverse of url_decode(). */
static int url_encode_append(char **cur, size_t *remain, const char *src)
{
    static const char hex[] = "0123456789ABCDEF";
    for (; *src; src++) {
        unsigned char c = (unsigned char)*src;
        int unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                         (c >= '0' && c <= '9') ||
                         c == '-' || c == '.' || c == '_' || c == '~';
        if (unreserved) {
            if (*remain < 2) return -1;      /* this char + eventual NUL */
            *(*cur)++ = (char)c;
            (*remain)--;
        } else {
            if (*remain < 4) return -1;      /* %XX + eventual NUL */
            *(*cur)++ = '%';
            *(*cur)++ = hex[c >> 4];
            *(*cur)++ = hex[c & 0xf];
            *remain -= 3;
        }
    }
    return 0;
}

/* Manual decimal conversion: libnix's sprintf() can't be trusted with a
 * uint64_t (no %llu), so the HOTP counter — the one field that can exceed 32
 * bits — gets its own portable formatter. buf must hold at least 21 bytes. */
static void u64_to_str(uint64_t v, char *buf)
{
    char tmp[20];
    int i = 0, j = 0;
    if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (v > 0) { tmp[i++] = (char)('0' + (int)(v % 10)); v /= 10; }
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

/* Worker for otpauth_parse: on failure `out` may hold a partially-decoded
 * secret - the public wrapper below scrubs it. */
static int otpauth_parse_fields(const char *uri, otp_account *out)
{
    const char *p, *slash, *label_start, *label_end, *query;
    size_t typelen;
    char label[256];
    char *colon;
    int have_secret = 0;

    if (!uri || !out) return -1;

    memset(out, 0, sizeof(*out));
    strcpy(out->type, "totp");
    strcpy(out->algorithm, "SHA1");
    out->digits = OTP_DEFAULT_DIGITS;
    out->period = OTP_DEFAULT_PERIOD;

    if (!ci_startswith(uri, "otpauth://")) return -1;
    p = uri + 10;

    /* TYPE up to the next '/' */
    slash = strchr(p, '/');
    if (!slash) return -1;
    typelen = (size_t)(slash - p);
    if      (typelen == 4 && ci_eq(p, "totp", 4))  strcpy(out->type, "totp");
    else if (typelen == 4 && ci_eq(p, "hotp", 4))  strcpy(out->type, "hotp");
    else if (typelen == 5 && ci_eq(p, "steam", 5)) strcpy(out->type, "steam");
    else return -1;

    /* LABEL up to '?' (or end) */
    label_start = slash + 1;
    query = strchr(label_start, '?');
    label_end = query ? query : label_start + strlen(label_start);
    url_decode(label_start, (size_t)(label_end - label_start), label, sizeof(label));

    /* Split "issuer:accountname" on the first ':' (a bare label is the account). */
    colon = strchr(label, ':');
    if (colon) {
        char *acct = colon + 1;
        *colon = '\0';
        copy_str(out->issuer, sizeof(out->issuer), label);
        while (*acct == ' ') acct++;               /* tolerate "issuer: acct" */
        copy_str(out->label, sizeof(out->label), acct);
    } else {
        copy_str(out->label, sizeof(out->label), label);
    }

    /* Query parameters */
    if (query) {
        const char *q = query + 1;
        while (*q) {
            const char *amp = strchr(q, '&');
            const char *pair_end = amp ? amp : q + strlen(q);
            const char *eq = (const char *)memchr(q, '=', (size_t)(pair_end - q));
            if (eq) {
                size_t klen = (size_t)(eq - q);
                const char *val = eq + 1;
                size_t vlen = (size_t)(pair_end - val);
                char dec[512];
                url_decode(val, vlen, dec, sizeof(dec));

                if (key_is(q, klen, "secret")) {
                    int n = base32_decode(dec, out->secret, OTP_MAX_SECRET);
                    memset(dec, 0, sizeof(dec));   /* held the Base32 secret */
                    if (n <= 0) return -1;
                    out->secret_len = (size_t)n;
                    have_secret = 1;
                } else if (key_is(q, klen, "issuer")) {
                    copy_str(out->issuer, sizeof(out->issuer), dec);
                } else if (key_is(q, klen, "algorithm")) {
                    /* Only SHA-1/256/512 exist (RFC 6238); refuse anything
                     * else rather than silently mint wrong codes with SHA-1. */
                    int alg = otp_alg_from_name(dec);
                    if (alg < 0) return -1;
                    strcpy(out->algorithm, otp_alg_name((otp_alg)alg));
                } else if (key_is(q, klen, "digits")) {
                    /* 6-8, matching what the vault format and GUI edit form
                     * accept (vault.c's parse_payload, MSG_GUI_LABEL_REQUIRED_
                     * FULL) - reject anything else rather than silently
                     * keeping the 6-digit default, the same "refuse, don't
                     * guess" policy as algorithm= just above. */
                    int d = atoi(dec);
                    if (d < 6 || d > 8) return -1;
                    out->digits = d;
                } else if (key_is(q, klen, "period")) {
                    long pr = atol(dec);
                    if (pr > 0) out->period = (uint32_t)pr;
                } else if (key_is(q, klen, "counter")) {
                    out->counter = (uint64_t)strtoull(dec, NULL, 10);
                }
            }
            if (!amp) break;
            q = amp + 1;
        }
    }

    if (!have_secret) return -1;   /* the secret is mandatory */

    /* Steam Guard is not itself configurable: force SHA1/5 regardless of any
     * algorithm=/digits= query parameter, rather than storing values the
     * renderer (otp_render, which dispatches on type before consulting
     * either) will never actually use. */
    if (strcmp(out->type, "steam") == 0) {
        strcpy(out->algorithm, "SHA1");
        out->digits = STEAM_CODE_DIGITS;
    }
    return 0;
}

int otpauth_parse(const char *uri, otp_account *out)
{
    int rc = otpauth_parse_fields(uri, out);
    if (rc != 0 && out)
        memset(out, 0, sizeof(*out));   /* may hold a partly-parsed secret */
    return rc;
}

int otpauth_is_uri(const char *s)
{
    return s != NULL && ci_startswith(s, "otpauth://");
}

/* Build an otpauth:// URI from `a` (the inverse of otpauth_parse — see the
 * grammar comment at the top of this file). Percent-encodes issuer/label
 * separately and joins them with a literal ':', matching how real-world
 * otpauth:// generators write the label rather than percent-encoding the
 * whole "issuer:label" as one blob. Always emits algorithm=/digits= and
 * period= (TOTP) or counter= (HOTP) explicitly rather than relying on
 * otpauth_parse's defaults, so the URI is self-describing even if those
 * defaults ever change. Steam accounts carry none of those three — Steam
 * Guard isn't configurable and otpauth_parse ignores them for type=steam
 * anyway. Returns 0 on success, -1 if `out` (capacity outcap) is too small
 * or the secret fails to Base32-encode. */
int otpauth_build(const otp_account *a, char *out, size_t outcap)
{
    char *cur = out;
    size_t remain = outcap;
    char secretb32[OTP_MAX_SECRET * 8 / 5 + 8];
    char numbuf[21];              /* fits any uint64_t decimal + NUL */
    int is_steam, is_hotp;

    if (!a || !out || outcap == 0) return -1;
    out[0] = '\0';
    if (base32_encode(a->secret, a->secret_len, secretb32, sizeof(secretb32)) < 0)
        return -1;

    is_steam = strcmp(a->type, "steam") == 0;
    is_hotp  = strcmp(a->type, "hotp")  == 0;

    if (str_append(&cur, &remain, "otpauth://") != 0) goto fail;
    if (str_append(&cur, &remain, is_steam ? "steam" : is_hotp ? "hotp" : "totp") != 0)
        goto fail;
    if (str_append(&cur, &remain, "/") != 0) goto fail;
    if (a->issuer[0]) {
        if (url_encode_append(&cur, &remain, a->issuer) != 0) goto fail;
        if (str_append(&cur, &remain, ":") != 0) goto fail;
    }
    if (url_encode_append(&cur, &remain, a->label) != 0) goto fail;

    if (str_append(&cur, &remain, "?secret=") != 0) goto fail;
    if (str_append(&cur, &remain, secretb32) != 0) goto fail;
    if (a->issuer[0]) {
        if (str_append(&cur, &remain, "&issuer=") != 0) goto fail;
        if (url_encode_append(&cur, &remain, a->issuer) != 0) goto fail;
    }

    if (!is_steam) {
        if (str_append(&cur, &remain, "&algorithm=") != 0) goto fail;
        if (str_append(&cur, &remain, a->algorithm) != 0) goto fail;

        sprintf(numbuf, "%lu", (unsigned long)a->digits);   /* small value: safe */
        if (str_append(&cur, &remain, "&digits=") != 0) goto fail;
        if (str_append(&cur, &remain, numbuf) != 0) goto fail;

        if (is_hotp) {
            u64_to_str(a->counter, numbuf);
            if (str_append(&cur, &remain, "&counter=") != 0) goto fail;
            if (str_append(&cur, &remain, numbuf) != 0) goto fail;
        } else {
            sprintf(numbuf, "%lu", (unsigned long)a->period);
            if (str_append(&cur, &remain, "&period=") != 0) goto fail;
            if (str_append(&cur, &remain, numbuf) != 0) goto fail;
        }
    }

    *cur = '\0';
    memset(secretb32, 0, sizeof(secretb32));
    return 0;

fail:
    memset(secretb32, 0, sizeof(secretb32));   /* held the Base32 secret */
    out[0] = '\0';                             /* and out may hold part of it */
    return -1;
}

static int account_from_secret(const char *issuer, const char *label,
                               const char *secret_b32, otp_account *out)
{
    int n;

    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    if (!label || !label[0] || !secret_b32) return -1;

    n = base32_decode(secret_b32, out->secret, sizeof(out->secret));
    if (n <= 0) { memset(out, 0, sizeof(*out)); return -1; }
    out->secret_len = (size_t)n;

    if (issuer) copy_str(out->issuer, sizeof(out->issuer), issuer);
    copy_str(out->label, sizeof(out->label), label);
    return 0;
}

int otp_account_from_secret(const char *issuer, const char *label,
                            const char *secret_b32, otp_account *out)
{
    if (account_from_secret(issuer, label, secret_b32, out) != 0) return -1;
    strcpy(out->type, "totp");
    strcpy(out->algorithm, "SHA1");
    out->digits = OTP_DEFAULT_DIGITS;
    out->period = OTP_DEFAULT_PERIOD;
    return 0;
}

int otp_account_from_secret_steam(const char *issuer, const char *label,
                                  const char *secret_b32, otp_account *out)
{
    if (account_from_secret(issuer, label, secret_b32, out) != 0) return -1;
    strcpy(out->type, "steam");
    strcpy(out->algorithm, "SHA1");
    out->digits = STEAM_CODE_DIGITS;
    out->period = STEAM_PERIOD;
    return 0;
}
