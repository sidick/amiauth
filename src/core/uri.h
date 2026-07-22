/* uri.h — otpauth:// URI parsing and the account record it yields. */
#ifndef AMIAUTH_URI_H
#define AMIAUTH_URI_H

#include <stddef.h>
#include <stdint.h>

#define OTP_MAX_SECRET  64
#define OTP_MAX_ISSUER  64
#define OTP_MAX_LABEL   128

/* A single OTP account. This is the unit stored in the vault. (The struct tag
 * lets otp.h's renderer take a pointer without pulling in this header.) */
typedef struct otp_account {
    char     type[8];                 /* "totp", "hotp" or "steam" (#44) */
    char     algorithm[8];            /* "SHA1", "SHA256" or "SHA512" */
    char     issuer[OTP_MAX_ISSUER];
    char     label[OTP_MAX_LABEL];
    uint8_t  secret[OTP_MAX_SECRET];  /* raw key bytes (Base32-decoded) */
    size_t   secret_len;
    int      digits;                  /* 6 or 8 (ignored for "steam": fixed 5) */
    uint32_t period;                  /* TOTP/steam step, seconds (steam fixed 30) */
    uint64_t counter;                 /* HOTP counter */
} otp_account;

/* Parse an otpauth:// URI into `out`. Returns 0 on success, -1 on malformed
 * input. Unspecified fields are filled with sensible defaults (SHA1/6/30).
 * TYPE accepts "totp", "hotp" or "steam" (#44); for "steam" the algorithm and
 * digits are forced to SHA1/5 regardless of any algorithm=/digits= query
 * parameter, since Steam Guard is not itself configurable. */
int otpauth_parse(const char *uri, otp_account *out);

/* Does `s` look like an otpauth:// URI (case-insensitive prefix test)? Used
 * to route add-account input between URI parsing and the bare-secret path. */
int otpauth_is_uri(const char *s);

/* Build an account directly from a bare Base32 secret, with the defaults
 * nearly every service issues (TOTP, SHA-1, 6 digits, 30 s) — the shortcut
 * for services that show only the raw secret, no URI. `label` must be
 * non-empty; `issuer` may be NULL/empty. Overlong issuer/label truncate,
 * matching the URI path. Returns 0 on success, -1 on a missing label or an
 * empty/undecodable secret (out is scrubbed on failure). */
int otp_account_from_secret(const char *issuer, const char *label,
                            const char *secret_b32, otp_account *out);

/* Same as otp_account_from_secret, but builds a Steam Guard account (#44):
 * Steam's shared secret is typically extracted from its own maFile format
 * and never carries an otpauth:// URI in practice, so this is the primary
 * entry point rather than a fallback. */
int otp_account_from_secret_steam(const char *issuer, const char *label,
                                  const char *secret_b32, otp_account *out);

#endif /* AMIAUTH_URI_H */
