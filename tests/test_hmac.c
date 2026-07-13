/* test_hmac.c — HMAC-SHA1 vectors (RFC 2202). */
#include <string.h>

#include "test.h"
#include "hmac.h"

void run_hmac_tests(void)
{
    uint8_t mac[SHA1_DIGEST_SIZE];
    uint8_t key[80];
    uint8_t data[50];

    /* RFC 2202 case 1: key = 0x0b x20, data = "Hi There". */
    memset(key, 0x0b, 20);
    hmac_sha1(key, 20, (const uint8_t *)"Hi There", 8, mac);
    TEST_CHECK(hex_eq(mac, sizeof(mac), "b617318655057264e28bc0b6fb378c8ef146be00"));

    /* RFC 2202 case 2: key = "Jefe" (shorter than block). */
    hmac_sha1((const uint8_t *)"Jefe", 4,
              (const uint8_t *)"what do ya want for nothing?", 28, mac);
    TEST_CHECK(hex_eq(mac, sizeof(mac), "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"));

    /* RFC 2202 case 3: key = 0xaa x20, data = 0xdd x50. */
    memset(key, 0xaa, 20);
    memset(data, 0xdd, 50);
    hmac_sha1(key, 20, data, 50, mac);
    TEST_CHECK(hex_eq(mac, sizeof(mac), "125d7342b9ac11cd91a39af48aa17b4f63f175d3"));

    /* RFC 2202 case 6: key = 0xaa x80 (longer than block -> hashed first). */
    memset(key, 0xaa, 80);
    hmac_sha1(key, 80,
              (const uint8_t *)"Test Using Larger Than Block-Size Key - Hash Key First",
              54, mac);
    TEST_CHECK(hex_eq(mac, sizeof(mac), "aa4ae5e15272d00e95705637ce8a3b55ed402112"));
}
