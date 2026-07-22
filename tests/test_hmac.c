/* test_hmac.c — HMAC-SHA1 vectors (RFC 2202) and HMAC-SHA256/512 (RFC 4231). */
#include <string.h>

#include "test.h"
#include "hmac.h"

void run_hmac_tests(void)
{
    uint8_t mac[SHA512_DIGEST_SIZE];
    uint8_t key[131];
    uint8_t data[50];

    /* RFC 2202 case 1: key = 0x0b x20, data = "Hi There". */
    memset(key, 0x0b, 20);
    hmac_sha1(key, 20, (const uint8_t *)"Hi There", 8, mac);
    TEST_CHECK(hex_eq(mac, SHA1_DIGEST_SIZE, "b617318655057264e28bc0b6fb378c8ef146be00"));

    /* RFC 2202 case 2: key = "Jefe" (shorter than block). */
    hmac_sha1((const uint8_t *)"Jefe", 4,
              (const uint8_t *)"what do ya want for nothing?", 28, mac);
    TEST_CHECK(hex_eq(mac, SHA1_DIGEST_SIZE, "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"));

    /* RFC 2202 case 3: key = 0xaa x20, data = 0xdd x50. */
    memset(key, 0xaa, 20);
    memset(data, 0xdd, 50);
    hmac_sha1(key, 20, data, 50, mac);
    TEST_CHECK(hex_eq(mac, SHA1_DIGEST_SIZE, "125d7342b9ac11cd91a39af48aa17b4f63f175d3"));

    /* RFC 2202 case 6: key = 0xaa x80 (longer than block -> hashed first). */
    memset(key, 0xaa, 80);
    hmac_sha1(key, 80,
              (const uint8_t *)"Test Using Larger Than Block-Size Key - Hash Key First",
              54, mac);
    TEST_CHECK(hex_eq(mac, SHA1_DIGEST_SIZE, "aa4ae5e15272d00e95705637ce8a3b55ed402112"));

    /* RFC 4231 case 1: key = 0x0b x20, data = "Hi There". */
    memset(key, 0x0b, 20);
    hmac_sha256(key, 20, (const uint8_t *)"Hi There", 8, mac);
    TEST_CHECK(hex_eq(mac, SHA256_DIGEST_SIZE,
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"));
    hmac_sha512(key, 20, (const uint8_t *)"Hi There", 8, mac);
    TEST_CHECK(hex_eq(mac, SHA512_DIGEST_SIZE,
        "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
        "daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854"));

    /* RFC 4231 case 2: key = "Jefe" (shorter than block). */
    hmac_sha256((const uint8_t *)"Jefe", 4,
                (const uint8_t *)"what do ya want for nothing?", 28, mac);
    TEST_CHECK(hex_eq(mac, SHA256_DIGEST_SIZE,
        "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
    hmac_sha512((const uint8_t *)"Jefe", 4,
                (const uint8_t *)"what do ya want for nothing?", 28, mac);
    TEST_CHECK(hex_eq(mac, SHA512_DIGEST_SIZE,
        "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
        "9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737"));

    /* RFC 4231 case 3: key = 0xaa x20, data = 0xdd x50. */
    memset(key, 0xaa, 20);
    memset(data, 0xdd, 50);
    hmac_sha256(key, 20, data, 50, mac);
    TEST_CHECK(hex_eq(mac, SHA256_DIGEST_SIZE,
        "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe"));
    hmac_sha512(key, 20, data, 50, mac);
    TEST_CHECK(hex_eq(mac, SHA512_DIGEST_SIZE,
        "fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
        "bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb"));

    /* RFC 4231 case 6: key = 0xaa x131 (longer than even the SHA-512 block). */
    memset(key, 0xaa, 131);
    hmac_sha256(key, 131,
                (const uint8_t *)"Test Using Larger Than Block-Size Key - Hash Key First",
                54, mac);
    TEST_CHECK(hex_eq(mac, SHA256_DIGEST_SIZE,
        "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"));
    hmac_sha512(key, 131,
                (const uint8_t *)"Test Using Larger Than Block-Size Key - Hash Key First",
                54, mac);
    TEST_CHECK(hex_eq(mac, SHA512_DIGEST_SIZE,
        "80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f352"
        "6b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598"));
}
