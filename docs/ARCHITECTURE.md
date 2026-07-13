# Architecture

AmiAuth is split into a portable, testable **core** and Amiga-specific
**front-ends**. Everything below the front-end line is plain C with no OS
dependencies, so it builds and runs under a host test harness against the
official RFC test vectors.

```
+---------------------------------------------+
|  GUI (ClassAct/ReAction) |  CLI front-end   |
|  + commodity shell       |                  |
|    (commodities.library: |                  |
|     hotkey, Exchange)    |                  |
+------------+-------------+---------+---------+
             |                       |
+------------v-----------------------v---------+
|  core: otp.c   (TOTP/HOTP)                   |
|        base32.c                              |
|        uri.c   (otpauth:// parsing)          |
|        vault.c (encrypted store)             |
|        clock.c (SNTP + offset resolution)    |
+------------+---------------------------------+
             |
   sha1.c / hmac.c / chacha20.c / pbkdf2.c
   (self-contained, no external crypto deps)
```

## Modules

### Crypto primitives (`sha1.c`, `hmac.c`, `chacha20.c`, `pbkdf2.c`)
Self-contained, vendored implementations. No external crypto dependency (no
AmiSSL requirement). SHA-1/HMAC power both OTP generation and the vault MAC;
ChaCha20 is the vault cipher; PBKDF2-HMAC-SHA1 is the KDF.

### `otp.c` — TOTP/HOTP
RFC 6238 / RFC 4226: HMAC-SHA1 over an 8-byte counter, dynamic truncation,
modulo to 6 or 8 digits. Configurable period and T0.

### `base32.c`
RFC 4648 Base32 decode, padding-tolerant and whitespace/case insensitive —
pasted secrets are messy in practice.

### `uri.c`
`otpauth://` URI parsing (issuer, label, secret, algorithm, digits, period),
so a secret exported from another authenticator can be pasted directly.

### `vault.c` — encrypted account store
Multi-account store, encrypted at rest. File header stores KDF salt + calibrated
iteration count + cipher marker. Construction is encrypt-then-MAC (ChaCha20 +
HMAC-SHA1). Supports an always-unlocked mode (cipher marked `none`) using the
same file format. The on-disk layout is frozen in [VAULT_FORMAT.md](VAULT_FORMAT.md).
`vault.c` takes an explicit file path and is unaware of `PROGDIR:`/`ENVARC:` —
those live in the Amiga front-end; see [STORAGE.md](STORAGE.md) for where the
vault and settings are kept and why. See also [SECURITY.md](SECURITY.md).

### `clock.c` — time resolution
Resolves corrected UTC without touching the system clock, in priority order:
1. **SNTP** — a single UDP exchange over `bsdsocket` if a TCP/IP stack is up;
   compute offset vs system clock, use corrected time, display measured offset.
2. **Explicit UTC offset** — user-stated offset for offline machines.
3. **Manual nudge** — a ±seconds control, synced by eye against the countdown.

## Front-ends

- **CLI** — no GUI dependency at all; retains full code-generation on OS 2.x and
  floppy-booted machines. Example: `AmiAuth GET github`.
- **GUI (ClassAct/ReAction)** — `listbrowser.gadget` account list, large code
  display, `fuelgauge.gadget` countdown, clipboard copy (clipboard.device, FTXT).
  Uses only classes common to ClassAct 3.3 and ReAction; ClassAct classes bundled
  for OS 3.0/3.1.
- **Commodity shell** — runs resident via `commodities.library`: `CX_POPKEY` /
  `CX_POPUP` / `CX_PRIORITY` tooltypes, default hotkey `ctrl alt a`, window hides
  (not quits) on close/Escape, full Exchange integration. Passphrase requested on
  first show; derived key held while resident with optional idle auto-lock.

## Build targets

- **Host** — portable core compiled natively for CI. Runs RFC 6238 Appendix B
  and RFC 4226 Appendix D vectors without an emulator.
- **m68k** — `amiga-gcc`, targeting plain 68000 for the core tool to maximise
  audience reach.

## Planned optimisation (post-vectors)

Hand-written 68k assembler for the crypto hot loops (SHA-1/HMAC, ChaCha20),
selected at runtime via function pointers set from `AttnFlags` CPU detection. The
C implementations stay permanently as reference and fallback. Asm paths are
validated in CI under amitools' `vamos`. An optional AmiSSL-backed provider
(`CRYPTO=BUILTIN|AMISSL|AUTO`, default `BUILTIN`) plugs into the same dispatch
table; the vault format is provider-agnostic so vaults move freely between
configurations. Faster primitives convert directly into security margin, since
PBKDF2 iterations are calibrated to ~1s of wall time.
