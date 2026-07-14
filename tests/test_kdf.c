/* test_kdf.c — PBKDF2 iteration calibration policy (vault_calibrate_iterations).
 * Pure arithmetic; the timing that feeds it is a front-end concern, tested by
 * hand under Amiberry. */
#include "test.h"
#include "vault.h"

void run_kdf_tests(void);
void run_kdf_tests(void)
{
    /* Extrapolation to the ~1s target (KDF_TARGET_MS = 1000). */
    TEST_CHECK(vault_calibrate_iterations(100, 100) == 1000);   /* 100 in 100ms -> 1000 in 1s */
    TEST_CHECK(vault_calibrate_iterations(1000, 500) == 2000);  /* 1000 in 500ms -> 2000 in 1s */
    TEST_CHECK(vault_calibrate_iterations(50, 1000) == 50);     /* already ~1s */

    /* probe_ms == 0 (too fast to time) -> the ceiling. */
    TEST_CHECK(vault_calibrate_iterations(64, 0) == KDF_MAX_ITERATIONS);

    /* Clamping. */
    TEST_CHECK(vault_calibrate_iterations(1000000, 1) == KDF_MAX_ITERATIONS);  /* would be 1e9 */
    TEST_CHECK(vault_calibrate_iterations(1, 100000) == KDF_MIN_ITERATIONS);   /* would round to 0 */

    /* Monotonicity: a faster probe (fewer ms for the same work) -> more iters. */
    TEST_CHECK(vault_calibrate_iterations(1000, 100) > vault_calibrate_iterations(1000, 200));

    /* No overflow at large probe counts (uint64 intermediate). */
    TEST_CHECK(vault_calibrate_iterations(3000000, 1000) == 3000000);  /* in range, unclamped */
}
