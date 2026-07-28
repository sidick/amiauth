/* base32.h — RFC 4648 Base32 encode/decode, tolerant of padding, whitespace
 * and case on decode. */
#ifndef AMIAUTH_BASE32_H
#define AMIAUTH_BASE32_H

#include <stddef.h>
#include <stdint.h>

/* Decode `in` into `out` (capacity `outcap`). Ignores whitespace and '=' padding
 * and accepts either case. Returns the number of bytes written, or -1 on an
 * invalid character or if the result would exceed `outcap`. */
int base32_decode(const char *in, uint8_t *out, size_t outcap);

/* Encode `inlen` bytes of `in` into `out` (capacity `outcap`), NUL-terminated.
 * Uppercase RFC 4648 alphabet, no '=' padding (matches how services usually
 * display a secret). Returns the number of characters written (excluding the
 * NUL), or -1 if the result (plus NUL) would exceed `outcap`. */
int base32_encode(const uint8_t *in, size_t inlen, char *out, size_t outcap);

#endif /* AMIAUTH_BASE32_H */
