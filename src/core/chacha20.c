/* chacha20.c — ChaCha20 stream cipher (RFC 8439).
 * Validated against RFC 8439 §2.4.2 in tests/test_chacha20.c. */
#include "chacha20.h"

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define QUARTERROUND(a, b, c, d)         \
    do {                                 \
        a += b; d ^= a; d = ROTL32(d, 16); \
        c += d; b ^= c; b = ROTL32(b, 12); \
        a += b; d ^= a; d = ROTL32(d,  8); \
        c += d; b ^= c; b = ROTL32(b,  7); \
    } while (0)

static uint32_t load32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* Produce one 64-byte keystream block from the 16-word state. */
static void chacha20_block(const uint32_t in[16], uint8_t out[64])
{
    uint32_t x[16];
    int i;

    for (i = 0; i < 16; i++) x[i] = in[i];

    for (i = 0; i < 10; i++) {          /* 20 rounds = 10 column+diagonal pairs */
        QUARTERROUND(x[0], x[4], x[ 8], x[12]);
        QUARTERROUND(x[1], x[5], x[ 9], x[13]);
        QUARTERROUND(x[2], x[6], x[10], x[14]);
        QUARTERROUND(x[3], x[7], x[11], x[15]);
        QUARTERROUND(x[0], x[5], x[10], x[15]);
        QUARTERROUND(x[1], x[6], x[11], x[12]);
        QUARTERROUND(x[2], x[7], x[ 8], x[13]);
        QUARTERROUND(x[3], x[4], x[ 9], x[14]);
    }

    for (i = 0; i < 16; i++) {
        uint32_t v = x[i] + in[i];       /* add the original input word back */
        out[i * 4]     = (uint8_t)(v);
        out[i * 4 + 1] = (uint8_t)(v >>  8);
        out[i * 4 + 2] = (uint8_t)(v >> 16);
        out[i * 4 + 3] = (uint8_t)(v >> 24);
    }
}

void chacha20_xor(const uint8_t key[CHACHA20_KEY_SIZE],
                  const uint8_t nonce[CHACHA20_NONCE_SIZE],
                  uint32_t counter,
                  const uint8_t *in, uint8_t *out, size_t len)
{
    uint32_t state[16];
    uint8_t block[64];
    size_t off = 0;
    int i;

    /* "expand 32-byte k" | key (8 words) | counter | nonce (3 words). */
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;
    for (i = 0; i < 8; i++) state[4 + i] = load32_le(key + i * 4);
    state[12] = counter;
    state[13] = load32_le(nonce + 0);
    state[14] = load32_le(nonce + 4);
    state[15] = load32_le(nonce + 8);

    while (off < len) {
        size_t n = len - off < 64 ? len - off : 64;
        size_t j;
        chacha20_block(state, block);
        /* `in` may be NULL to emit the raw keystream. */
        for (j = 0; j < n; j++)
            out[off + j] = (uint8_t)((in ? in[off + j] : 0) ^ block[j]);
        state[12]++;                     /* next block counter */
        off += n;
    }
}
