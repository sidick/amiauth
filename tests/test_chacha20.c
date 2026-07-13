/* test_chacha20.c — ChaCha20 vectors (RFC 8439 §2.4.2). */
#include <string.h>

#include "test.h"
#include "chacha20.h"

/* RFC 8439 §2.4.2: key = 00..1f, nonce = 00:00:00:00:00:00:00:4a:00:00:00:00,
 * initial counter = 1, over a 114-byte plaintext. */
static const char PLAINTEXT[] =
    "Ladies and Gentlemen of the class of '99: If I could offer you "
    "only one tip for the future, sunscreen would be it.";

static const char CIPHERTEXT_HEX[] =
    "6e2e359a2568f98041ba0728dd0d6981"
    "e97e7aec1d4360c20a27afccfd9fae0b"
    "f91b65c5524733ab8f593dabcd62b357"
    "1639d624e65152ab8f530c359f0861d8"
    "07ca0dbf500d6a6156a38e088a22b65e"
    "52bc514d16ccf806818ce91ab7793736"
    "5af90bbf74a35be6b40b8eedf2785e42"
    "874d";

void run_chacha20_tests(void)
{
    uint8_t key[CHACHA20_KEY_SIZE];
    uint8_t nonce[CHACHA20_NONCE_SIZE] = {
        0, 0, 0, 0, 0, 0, 0, 0x4a, 0, 0, 0, 0
    };
    uint8_t buf[128];
    size_t len = sizeof(PLAINTEXT) - 1;   /* exclude the NUL terminator */
    size_t i;

    for (i = 0; i < CHACHA20_KEY_SIZE; i++) key[i] = (uint8_t)i;

    /* §2.4.2 encryption vector. */
    chacha20_xor(key, nonce, 1, (const uint8_t *)PLAINTEXT, buf, len);
    TEST_CHECK(len == 114);
    TEST_CHECK(hex_eq(buf, len, CIPHERTEXT_HEX));

    /* Round-trip: decrypting the ciphertext restores the plaintext. */
    {
        uint8_t back[128];
        chacha20_xor(key, nonce, 1, buf, back, len);
        TEST_CHECK(memcmp(back, PLAINTEXT, len) == 0);
    }

    /* In-place (in == out) must match the out-of-place result. */
    {
        uint8_t inplace[128];
        memcpy(inplace, PLAINTEXT, len);
        chacha20_xor(key, nonce, 1, inplace, inplace, len);
        TEST_CHECK(hex_eq(inplace, len, CIPHERTEXT_HEX));
    }
}
