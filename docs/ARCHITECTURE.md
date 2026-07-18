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
Resolves corrected UTC without touching the system clock (full design in
[CLOCK.md](CLOCK.md)), in priority order:
1. **SNTP** — a single UDP exchange over `bsdsocket` if a TCP/IP stack is up;
   compute offset vs system clock, use corrected time, display measured offset.
2. **Explicit UTC offset** — user-stated offset for offline machines.
3. **Manual nudge** — a ±seconds control, synced by eye against the countdown.

## Front-ends

- **CLI** — no GUI dependency at all; retains full code-generation on OS 2.x and
  floppy-booted machines. Commands: `CODE`, `INIT`, `ADD`, `LIST`, `GET`,
  `REMOVE`, `SHOW`, `CLOCK`, `SYNC`, `OFFSET`. Amiga-only front-end glue (bsdsocket
  SNTP, entropy, the GUI-forward client, ...) lives in `src/amiga/` and is linked
  into the m68k build only; the host build stubs it. When a GUI is resident the
  CLI **forwards** vault commands to it over a public message port
  (`src/amiga/guiport.[ch]`) instead of opening the vault a second time.
- **GUI (ClassAct/ReAction)** — a multi-column `listbrowser.gadget` (all accounts
  with live codes + countdown), a large selected-code display, a
  `fuelgauge.gadget` countdown, add / remove / edit, clipboard copy (iffparse
  FTXT, auto-clear), the clock-status LED, and **QR-image import** (decode an
  `otpauth://` QR from an image file via `datatypes.library` + a vendored quirc
  decoder in `src/qr/`). Uses only classes common to ClassAct 3.3 and ReAction.
- **Commodity shell** — runs resident via `commodities.library`: `CX_POPKEY` /
  `CX_POPUP` / `CX_PRIORITY` tooltypes, default hotkey `ctrl alt a`, window hides
  (not quits) on close, full Exchange integration (show/hide/enable/disable/kill),
  single-instance. The unlocked vault is held while resident (with optional idle
  auto-lock), so a hotkey pop-up or a forwarded CLI command needs no re-prompt.

## Build targets

- **Host** — portable core compiled natively for CI. Runs RFC 6238 Appendix B
  and RFC 4226 Appendix D vectors without an emulator.
- **m68k** — `amiga-gcc`. **Plain 68000 (`-m68000`) is the baseline target**:
  all code (CLI and GUI) must build and run on a stock 68000 to maximise reach.
  Requiring 020+ needs a very good reason; the only 020+ code is the *optional*
  AmiSSL/hand-asm crypto providers below, selected by runtime CPU detection and
  never required.

## Planned optimisation (post-vectors)

Hand-written 68k assembler for the crypto hot loops (SHA-1/HMAC, ChaCha20),
selected at runtime via function pointers set from `AttnFlags` CPU detection. The
C implementations stay permanently as reference and fallback. Asm paths are
validated in CI under amitools' `vamos`. An optional AmiSSL-backed provider
(`CRYPTO=BUILTIN|AMISSL|AUTO`, default `BUILTIN`) plugs into the same dispatch
table; the vault format is provider-agnostic so vaults move freely between
configurations. Faster primitives convert directly into security margin, since
PBKDF2 iterations are calibrated to ~1s of wall time.
