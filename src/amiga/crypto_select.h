/* crypto_select.h — repoint the crypto hot-loop dispatch (#47) at the
 * hand-written 68000-baseline asm, unless overridden. Amiga-only; call once
 * at startup, before any crypto use. */
#ifndef AMIAUTH_CRYPTO_SELECT_H
#define AMIAUTH_CRYPTO_SELECT_H

void crypto_select_init(void);

#endif /* AMIAUTH_CRYPTO_SELECT_H */
