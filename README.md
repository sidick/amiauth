# AmiAuth

A native two-factor authentication (2FA) code generator for classic AmigaOS.

AmiAuth implements TOTP (RFC 6238) and HOTP (RFC 4226): it stores multiple
accounts, generates 6- or 8-digit codes with a live countdown, and — critically
for the platform — solves the accurate-time problem that TOTP depends on. Small,
self-contained, and designed to run on anything from a stock 68000 A500 up to an
accelerated or emulated machine.

> **Status:** in development. The portable core (crypto, OTP, vault, `otpauth://`
> import) and a working **CLI** are complete and RFC-verified, and cross-build to a
> real AmigaOS binary — validated on OS 3.2 under Amiberry, including SNTP time
> sync. The ReAction **GUI** and commodity are the remaining work. See
> [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Why

No TOTP tool exists for classic AmigaOS. Anyone using an Amiga day-to-day still
reaches for a phone to log into GitHub, forge sites, or their own services.
AmiAuth aims to make "my A1200 is my 2FA device" a real, daily-useful thing.

## Features (v1)

✅ = implemented in the core/CLI · 🚧 = in progress (Amiga front-end)

- ✅ **TOTP & HOTP** — SHA-1, 6/8-digit codes, configurable period (30s default)
  and T0, validated against the official RFC test vectors.
- ✅ **Easy secret entry** — padding/whitespace/case-tolerant Base32 decoding and
  `otpauth://` URI parsing, so secrets from another authenticator paste directly.
- ✅ **Multi-account store** — issuer/label per account, ordered list.
- ✅ **Encrypted vault** — accounts encrypted at rest with a master passphrase
  (PBKDF2 + ChaCha20, encrypt-then-MAC). Optional always-unlocked mode for
  single-user or headless machines. (On-Amiga encrypted-vault creation awaits the
  CSPRNG; always-unlocked works everywhere.)
- ✅ **Accurate time without a working clock** — SNTP sync over `bsdsocket`, a
  `locale.library` offset, and a manual offset/nudge, layered so it works with
  zero config on a networked machine and degrades gracefully to a floppy-booted
  A500. (GUI status indicator is 🚧.)
- ✅ **CLI** — dependency-free, works down to OS 2.x:
  `CODE`, `INIT`, `ADD`, `LIST`, `GET`, `REMOVE`, `CLOCK`, `SYNC`. On Amiga it uses
  standard `ReadArgs` parsing (`AmiAuth GET GitHub`, options as keywords like
  `VAULT`/`ITERATIONS`; `AmiAuth ?` for the template, `HELP` for the command list).
- 🚧 **GUI + commodity** — a ClassAct/ReAction GUI that runs as a proper commodity
  (resident, hotkey popup, Exchange integration).

## Design principles

- **68000 is the baseline target** — everything builds and runs on a plain
  68000 (`-m68000`), so AmiAuth works on stock hardware. Requiring 020+ needs a
  very good reason; anything that does (an optional AmiSSL or hand-written-asm
  crypto provider) is opt-in via runtime CPU dispatch, never the minimum.
- **Zero mandatory dependencies** beyond the OS — all crypto is vendored; no
  AmiSSL requirement. `bsdsocket` is used only opportunistically for SNTP.
- **Portable, testable core** — the OTP and crypto code is plain C with a
  host-side build target, so RFC vectors run in CI without an emulator.
- **Honest security** — the docs state the threat model plainly. The vault
  protects secrets at rest; it does not defend against a compromised running OS
  (AmigaOS has no memory protection). See [`docs/SECURITY.md`](docs/SECURITY.md).

## Building

    make test         # host unit + RFC-vector tests
    make cli          # native CLI  -> build/amiauth-host
    make smoke        # end-to-end CLI smoke test
    make diff         # differential fuzz vs OpenSSL (opt-in; needs libcrypto)
    make m68k-docker  # AmigaOS binary via the amiga-gcc container -> build/AmiAuth

The core is portable C, so `test`/`cli` build with any host compiler. Example:
`build/amiauth-host CODE JBSWY3DPEHPK3PXP` prints a code.

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — module layout and build targets.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — phased delivery plan and v2 candidates.
- [`docs/SECURITY.md`](docs/SECURITY.md) — threat model and the honest security note.
- [`docs/VAULT_FORMAT.md`](docs/VAULT_FORMAT.md) — the frozen on-disk vault format.
- [`docs/CLOCK.md`](docs/CLOCK.md) — layered time-resolution design (SNTP/locale/manual).
- [`docs/STORAGE.md`](docs/STORAGE.md) — where the vault and settings live on AmigaOS.

## Toolchain

C via `amiga-gcc`, GitHub Actions CI running host-side RFC vector tests, a CLI
smoke test, an OpenSSL differential fuzz job, and an m68k build; Aminet packaging
via `aminet-release-action`. The core tool targets plain 68000 to maximise the
audience.

## License

BSD 2-Clause. Copyright (c) 2026 Simon Dick. See [`LICENSE`](LICENSE).

Bundled third-party source (the ISC-licensed `quirc` QR decoder) is listed in
[`THIRDPARTY.md`](THIRDPARTY.md).
