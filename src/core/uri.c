/* uri.c — otpauth:// parsing. STUB: implement in Phase 2 (tests/test_uri.c).
 * Format: otpauth://TYPE/LABEL?secret=...&issuer=...&digits=...&period=...  */
#include <string.h>

#include "uri.h"
#include "otp.h"
#include "base32.h"

int otpauth_parse(const char *uri, otp_account *out)
{
    if (!uri || !out) return -1;

    memset(out, 0, sizeof(*out));
    /* Sensible v1 defaults; the parser overrides these from the query string. */
    strcpy(out->type, "totp");
    strcpy(out->algorithm, "SHA1");
    out->digits = OTP_DEFAULT_DIGITS;
    out->period = OTP_DEFAULT_PERIOD;

    /* TODO: split scheme/type/label, URL-decode label & issuer, read the query
     * params, Base32-decode `secret` into out->secret via base32_decode(). */
    return -1;
}
