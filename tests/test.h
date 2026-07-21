/* test.h — tiny host-side test harness for the portable core.
 *
 * A test uses TEST_CHECK(cond) for real assertions and TEST_PENDING(msg) to
 * record an unimplemented case (a scaffolded stub). Pending cases are reported
 * but do NOT fail the run, so CI stays green while modules are filled in.
 * The runner exits non-zero only on a real failure. */
#ifndef AMIAUTH_TEST_H
#define AMIAUTH_TEST_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Compare `len` raw bytes against a lower-case hex string (exactly len*2 chars).
 * Returns non-zero on an exact match. Shared by the crypto vector tests. */
static inline int hex_eq(const uint8_t *buf, size_t len, const char *hex)
{
    static const char H[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < len; i++) {
        if (hex[i * 2]     != H[buf[i] >> 4])   return 0;
        if (hex[i * 2 + 1] != H[buf[i] & 0x0f]) return 0;
    }
    return hex[len * 2] == '\0';
}

typedef struct {
    int passed;
    int failed;
    int pending;
} test_ctx;

extern test_ctx g_test;

#define TEST_CHECK(cond)                                                    \
    do {                                                                    \
        if (cond) {                                                         \
            g_test.passed++;                                                \
        } else {                                                            \
            g_test.failed++;                                                \
            fprintf(stderr, "FAIL    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                   \
    } while (0)

#define TEST_PENDING(msg)                                                   \
    do {                                                                    \
        g_test.pending++;                                                   \
        fprintf(stderr, "PENDING %s:%d: %s\n", __FILE__, __LINE__, (msg));  \
    } while (0)

#endif /* AMIAUTH_TEST_H */
