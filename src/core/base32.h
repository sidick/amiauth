/* base32.h — RFC 4648 Base32 decode, tolerant of padding, whitespace and case. */
#ifndef AMIAUTH_BASE32_H
#define AMIAUTH_BASE32_H

#include <stddef.h>
#include <stdint.h>

/* Decode `in` into `out` (capacity `outcap`). Ignores whitespace and '=' padding
 * and accepts either case. Returns the number of bytes written, or -1 on an
 * invalid character or if the result would exceed `outcap`. */
int base32_decode(const char *in, uint8_t *out, size_t outcap);

#endif /* AMIAUTH_BASE32_H */
