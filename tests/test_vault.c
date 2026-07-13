/* test_vault.c — account store behaviour. */
#include <string.h>

#include "test.h"
#include "vault.h"

void run_vault_tests(void)
{
    /* Always-unlocked mode is implemented and needs no crypto: exercise the
     * account list operations against it now. */
    vault v;
    otp_account a;
    memset(&a, 0, sizeof(a));
    strcpy(a.type, "totp");
    strcpy(a.issuer, "GitHub");

    TEST_CHECK(vault_create(&v, NULL) == VAULT_OK);
    TEST_CHECK(v.cipher == VAULT_CIPHER_NONE);
    TEST_CHECK(v.unlocked == 1);

    TEST_CHECK(vault_add(&v, &a) == VAULT_OK);
    TEST_CHECK(vault_add(&v, &a) == VAULT_OK);
    TEST_CHECK(v.count == 2);
    TEST_CHECK(vault_remove(&v, 0) == VAULT_OK);
    TEST_CHECK(v.count == 1);
    TEST_CHECK(vault_remove(&v, 5) == VAULT_ERR_RANGE);

    /* Always-unlocked vaults never lock. */
    vault_lock(&v);
    TEST_CHECK(v.unlocked == 1);

    /* Encrypted-mode round trip (create/save/load/unlock) awaits Phase 2. */
    TEST_PENDING("Encrypted vault create+save+load+unlock round trip");
    TEST_PENDING("Wrong passphrase -> VAULT_ERR_AUTH");
    TEST_PENDING("Encrypted vault locks and zeroes key on vault_lock()");
}
