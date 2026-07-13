/* base32.c — RFC 4648 Base32 decode, tolerant of case, whitespace and padding.
 * Validated against tests/test_base32.c. */
#include "base32.h"

int base32_decode(const char *in, uint8_t *out, size_t outcap)
{
    uint32_t buf = 0;   /* bit accumulator (MSB-first) */
    int bits = 0;       /* valid bits currently in buf */
    size_t n = 0;

    if (!in || !out) return -1;

    for (; *in; in++) {
        int c = (unsigned char)*in;
        int val;

        /* Tolerate whitespace, '-' separators and '=' padding anywhere. */
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
            c == '-' || c == '=')
            continue;

        if      (c >= 'A' && c <= 'Z') val = c - 'A';
        else if (c >= 'a' && c <= 'z') val = c - 'a';        /* case-insensitive */
        else if (c >= '2' && c <= '7') val = c - '2' + 26;
        else return -1;                                       /* invalid symbol */

        buf = (buf << 5) | (uint32_t)val;
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            if (n >= outcap) return -1;                       /* would overflow */
            out[n++] = (uint8_t)((buf >> bits) & 0xff);
        }
    }

    /* Any leftover < 8 bits are padding and discarded. */
    return (int)n;
}
