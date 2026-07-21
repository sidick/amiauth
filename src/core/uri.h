/* uri.h — otpauth:// URI parsing and the account record it yields. */
#ifndef AMIAUTH_URI_H
#define AMIAUTH_URI_H

#include <stddef.h>
#include <stdint.h>

#define OTP_MAX_SECRET  64
#define OTP_MAX_ISSUER  64
#define OTP_MAX_LABEL   128

/* A single OTP account. This is the unit stored in the vault. */
typedef struct {
    char     type[8];                 /* "totp" or "hotp" */
    char     algorithm[8];            /* "SHA1" (v1); SHA256/512 are v2 */
    char     issuer[OTP_MAX_ISSUER];
    char     label[OTP_MAX_LABEL];
    uint8_t  secret[OTP_MAX_SECRET];  /* raw key bytes (Base32-decoded) */
    size_t   secret_len;
    int      digits;                  /* 6 or 8 */
    uint32_t period;                  /* TOTP step, seconds */
    uint64_t counter;                 /* HOTP counter */
} otp_account;

/* Parse an otpauth:// URI into `out`. Returns 0 on success, -1 on malformed
 * input. Unspecified fields are filled with sensible defaults (SHA1/6/30). */
int otpauth_parse(const char *uri, otp_account *out);

#endif /* AMIAUTH_URI_H */
