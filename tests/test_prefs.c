/* test_prefs.c — settings store round trip (host backend, $AMIAUTH_PREFS_DIR). */
#include <string.h>

#include "test.h"
#include "prefs.h"

void run_prefs_tests(void)
{
    char buf[64];
    long v;

    /* string set/get */
    TEST_CHECK(prefs_set("test_server", "pool.ntp.org") == 0);
    TEST_CHECK(prefs_get("test_server", buf, sizeof(buf)) == 0);
    TEST_CHECK(strcmp(buf, "pool.ntp.org") == 0);

    /* long set/get, including a negative value */
    TEST_CHECK(prefs_set_long("test_offset", -3600) == 0);
    TEST_CHECK(prefs_get_long("test_offset", &v) == 0 && v == -3600);

    /* overwrite */
    TEST_CHECK(prefs_set_long("test_offset", 42) == 0);
    TEST_CHECK(prefs_get_long("test_offset", &v) == 0 && v == 42);

    /* missing key -> not found */
    TEST_CHECK(prefs_get("test_no_such_key", buf, sizeof(buf)) == -1);
    TEST_CHECK(prefs_get_long("test_no_such_key", &v) == -1);
}
