/* test_catalog.c — host catalog stub contract (#67). The host build has no
 * locale.library concept at all: catalog_get() must always return the
 * caller's default, and catalog_open()/catalog_close() must be safe no-ops.
 * This is the contract every MSG() call site relies on; the real
 * locale.library lookup (src/amiga/catalog.c) is only exercised on-target
 * (tests/gui/catalog-onhw.sh), since it needs a real Amiga. */
#include <string.h>

#include "test.h"
#include "catalog.h"

void run_catalog_tests(void)
{
    /* Safe before catalog_open(), safe called more than once, safe after
     * catalog_close() - callers never need to guard these. */
    TEST_CHECK(strcmp(catalog_get(MSG_CLI_ALREADY_EXISTS, MSG_CLI_ALREADY_EXISTS_STR), MSG_CLI_ALREADY_EXISTS_STR) == 0);
    catalog_open();
    TEST_CHECK(strcmp(catalog_get(MSG_CLI_ALREADY_EXISTS, MSG_CLI_ALREADY_EXISTS_STR), MSG_CLI_ALREADY_EXISTS_STR) == 0);
    TEST_CHECK(strcmp(MSG(MSG_GUI_OK), MSG_GUI_OK_STR) == 0);
    catalog_close();
    TEST_CHECK(strcmp(catalog_get(MSG_CLI_ALREADY_EXISTS, MSG_CLI_ALREADY_EXISTS_STR), MSG_CLI_ALREADY_EXISTS_STR) == 0);

    /* An unknown id and an arbitrary default still just round-trip - the
     * host stub never looks at id at all. */
    TEST_CHECK(strcmp(catalog_get(9999, "arbitrary default"), "arbitrary default") == 0);
}
