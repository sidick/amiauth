/* uri.c — otpauth:// URI parsing (Key Uri Format).
 *   otpauth://TYPE/LABEL?secret=...&issuer=...&algorithm=...&digits=...
 *            &period=...&counter=...
 * LABEL is "accountname" or "issuer:accountname", percent-encoded.
 * Validated against tests/test_uri.c. */
#include <stdlib.h>
#include <string.h>

#include "uri.h"
#include "otp.h"
#include "base32.h"

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

int otpauth_parse(const char *uri, otp_account *out)
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
    if      (typelen == 4 && ci_eq(p, "totp", 4)) strcpy(out->type, "totp");
    else if (typelen == 4 && ci_eq(p, "hotp", 4)) strcpy(out->type, "hotp");
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
                    if (n <= 0) return -1;
                    out->secret_len = (size_t)n;
                    have_secret = 1;
                } else if (key_is(q, klen, "issuer")) {
                    copy_str(out->issuer, sizeof(out->issuer), dec);
                } else if (key_is(q, klen, "algorithm")) {
                    copy_str(out->algorithm, sizeof(out->algorithm), dec);
                } else if (key_is(q, klen, "digits")) {
                    int d = atoi(dec);
                    if (d == 6 || d == 8) out->digits = d;
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
    return 0;
}

int otpauth_is_uri(const char *s)
{
    return s != NULL && ci_startswith(s, "otpauth://");
}

int otp_account_from_secret(const char *issuer, const char *label,
                            const char *secret_b32, otp_account *out)
{
    int n;

    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    if (!label || !label[0] || !secret_b32) return -1;

    strcpy(out->type, "totp");
    strcpy(out->algorithm, "SHA1");
    out->digits = OTP_DEFAULT_DIGITS;
    out->period = OTP_DEFAULT_PERIOD;

    n = base32_decode(secret_b32, out->secret, sizeof(out->secret));
    if (n <= 0) { memset(out, 0, sizeof(*out)); return -1; }
    out->secret_len = (size_t)n;

    if (issuer) copy_str(out->issuer, sizeof(out->issuer), issuer);
    copy_str(out->label, sizeof(out->label), label);
    return 0;
}
