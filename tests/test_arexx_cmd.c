/* test_arexx_cmd.c — ARexx command-line parsing + RC policy (#46). */
#include <string.h>

#include "test.h"
#include "arexx_cmd.h"

void run_arexx_cmd_tests(void)
{
    arexx_parsed p;

    /* --- command keyword recognition, case-insensitive --- */
    TEST_CHECK(arexx_parse("STATUS", &p) == 0 && p.type == AREXX_CMD_STATUS);
    TEST_CHECK(arexx_parse("status", &p) == 0 && p.type == AREXX_CMD_STATUS);
    TEST_CHECK(arexx_parse("StAtUs", &p) == 0 && p.type == AREXX_CMD_STATUS);
    TEST_CHECK(arexx_parse("LOCK", &p) == 0 && p.type == AREXX_CMD_LOCK);
    TEST_CHECK(arexx_parse("UNLOCK", &p) == 0 && p.type == AREXX_CMD_UNLOCK);
    TEST_CHECK(arexx_parse("SHOW", &p) == 0 && p.type == AREXX_CMD_SHOW);
    TEST_CHECK(arexx_parse("HIDE", &p) == 0 && p.type == AREXX_CMD_HIDE);

    /* --- GETCODE/TIMELEFT: required ACCOUNT argument --- */
    TEST_CHECK(arexx_parse("GETCODE github", &p) == 0);
    TEST_CHECK(p.type == AREXX_CMD_GETCODE && strcmp(p.account, "github") == 0);
    TEST_CHECK(arexx_parse("TIMELEFT github", &p) == 0);
    TEST_CHECK(p.type == AREXX_CMD_TIMELEFT && strcmp(p.account, "github") == 0);

    /* Quoted account name (spaces). */
    TEST_CHECK(arexx_parse("GETCODE \"My Account\"", &p) == 0);
    TEST_CHECK(strcmp(p.account, "My Account") == 0);

    /* Missing required ACCOUNT is a parse error. */
    TEST_CHECK(arexx_parse("GETCODE", &p) == -1 && p.type == AREXX_CMD_UNKNOWN);
    TEST_CHECK(arexx_parse("GETCODE   ", &p) == -1 && p.type == AREXX_CMD_UNKNOWN);
    TEST_CHECK(arexx_parse("TIMELEFT", &p) == -1);

    /* --- QUIT: optional FORCE/S --- */
    TEST_CHECK(arexx_parse("QUIT", &p) == 0 && p.type == AREXX_CMD_QUIT && p.force == 0);
    TEST_CHECK(arexx_parse("QUIT FORCE", &p) == 0 && p.force == 1);
    TEST_CHECK(arexx_parse("QUIT force", &p) == 0 && p.force == 1);   /* case-insensitive */

    /* --- unknown command --- */
    TEST_CHECK(arexx_parse("BOGUS", &p) == -1 && p.type == AREXX_CMD_UNKNOWN);
    TEST_CHECK(arexx_parse("", &p) == -1);
    TEST_CHECK(arexx_parse("   ", &p) == -1);

    /* --- argument guards --- */
    TEST_CHECK(arexx_parse(NULL, &p) == -1);
    TEST_CHECK(arexx_parse("STATUS", NULL) == -1);

    /* --- leading/trailing whitespace tolerance --- */
    TEST_CHECK(arexx_parse("  STATUS  ", &p) == 0 && p.type == AREXX_CMD_STATUS);
    TEST_CHECK(arexx_parse("  GETCODE   github  ", &p) == 0 &&
              strcmp(p.account, "github") == 0);

    /* --- arexx_timeleft: TOTP delegates to totp_seconds_remaining, HOTP is
     * always -1 regardless of period/time --- */
    TEST_CHECK(arexx_timeleft(0, 30, 0, 30) == 30);   /* fresh period: full time left */
    TEST_CHECK(arexx_timeleft(0, 29, 0, 30) == 1);    /* about to roll over */
    TEST_CHECK(arexx_timeleft(0, 15, 0, 30) == 15);
    TEST_CHECK(arexx_timeleft(1, 15, 0, 30) == -1);   /* HOTP: no time concept */
    TEST_CHECK(arexx_timeleft(1, 0, 0, 0) == -1);
}
