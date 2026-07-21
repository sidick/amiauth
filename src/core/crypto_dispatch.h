/* crypto_dispatch.h — runtime-selectable hot-loop implementations.
 *
 * sha1_compress() and chacha20_block() are the only real "hot loops" in the
 * crypto core (HMAC and PBKDF2 have none of their own - they're built
 * entirely on calls into SHA-1, so speeding up sha1_compress() transitively
 * speeds up both). Each has a portable C reference (always present, the
 * host/test path and the permanent 68000 fallback) and a function-pointer
 * seam an Amiga front-end can repoint at runtime-selected 68020+ assembly
 * after checking AttnFlags. Nothing outside sha1.c/chacha20.c and the
 * front-end's CPU-detection code should need this header. */
#ifndef AMIAUTH_CRYPTO_DISPATCH_H
#define AMIAUTH_CRYPTO_DISPATCH_H

#include <stdint.h>

#include "sha1.h"

typedef void (*sha1_compress_fn)(uint32_t state[5], const uint8_t block[SHA1_BLOCK_SIZE]);
typedef void (*chacha20_block_fn)(const uint32_t in[16], uint8_t out[64]);

extern sha1_compress_fn  g_sha1_compress;
extern chacha20_block_fn g_chacha20_block;

/* The portable C reference implementations, always linked in as the default
 * and the 68000 fallback. */
void sha1_compress_c(uint32_t state[5], const uint8_t block[SHA1_BLOCK_SIZE]);
void chacha20_block_c(const uint32_t in[16], uint8_t out[64]);

#endif /* AMIAUTH_CRYPTO_DISPATCH_H */
