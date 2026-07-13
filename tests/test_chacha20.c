/* test_chacha20.c — ChaCha20 keystream/encrypt vectors (RFC 8439). */
#include "test.h"
#include "chacha20.h"

void run_chacha20_tests(void)
{
    /* RFC 8439 §2.4.2: key 00..1f, nonce 00:00:00:00:00:00:00:4a:00:00:00:00,
     * counter 1, plaintext "Ladies and Gentlemen of the class of '99..."
     * -> ciphertext beginning 6e 2e 35 9a 25 68 f9 80 41 ba 07 28 ... */
    TEST_PENDING("ChaCha20 RFC 8439 keystream block (implement in Phase 2)");
    TEST_PENDING("ChaCha20 RFC 8439 §2.4.2 encryption vector");
    TEST_PENDING("ChaCha20 round-trip: enc then dec restores plaintext");
    TEST_PENDING("ChaCha20 in-place (in == out) matches out-of-place");
}
