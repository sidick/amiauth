/* base32.c — RFC 4648 Base32 decode. STUB: implement in Phase 1
 * (tests/test_base32.c). */
#include "base32.h"

int base32_decode(const char *in, uint8_t *out, size_t outcap)
{
    (void)in; (void)out; (void)outcap;
    /* TODO: map A-Z2-7 (case-insensitive) to 5-bit groups, skip whitespace and
     * '=' padding, pack into bytes, bounds-check against outcap. */
    return -1;
}
