/* crypto_select.c — see crypto_select.h. */
#include "crypto_select.h"
#include "../core/crypto_dispatch.h"
#include "../core/prefs.h"

/* Hand-written asm (src/core/sha1_asm.s / chacha20_asm.s), validated under
 * vamos on both plain 68000 and 68020+ - restricted to instructions
 * available on every target this project supports, so unlike the AmiSSL
 * provider (#85) there's no CPU tier where using it is unsafe. */
extern void sha1_compress_asm(uint32_t state[5], const uint8_t block[64]);
extern void chacha20_block_asm(const uint32_t in[16], uint8_t out[64]);

void crypto_select_init(void)
{
    char buf[8];

    /* ENVARC:AmiAuth/cryptoasm = off forces the portable C reference back on
     * - a safety valve if the asm is ever suspected of a bug on real
     * hardware. Unset, or any other value, uses the asm. */
    if (prefs_get("cryptoasm", buf, sizeof buf) == 0 &&
        (buf[0] == 'o' || buf[0] == 'O') &&
        (buf[1] == 'f' || buf[1] == 'F'))
        return;

    g_sha1_compress  = sha1_compress_asm;
    g_chacha20_block = chacha20_block_asm;
}
