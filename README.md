# AmiAuth

A native two-factor authentication (2FA) code generator for classic AmigaOS.

AmiAuth implements TOTP (RFC 6238) and HOTP (RFC 4226): it stores multiple
accounts, generates 6- or 8-digit codes with a live countdown, and — critically
for the platform — solves the accurate-time problem that TOTP depends on. Small,
self-contained, and designed to run on anything from a stock 68000 A500 up to an
accelerated or emulated machine.

> **Status:** early development. This repository currently holds design docs and
> the initial project scaffold. See [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Why

No TOTP tool exists for classic AmigaOS. Anyone using an Amiga day-to-day still
reaches for a phone to log into GitHub, forge sites, or their own services.
AmiAuth aims to make "my A1200 is my 2FA device" a real, daily-useful thing.

## Features (planned for v1)

- **TOTP & HOTP** — SHA-1, 6/8-digit codes, configurable period (30s default)
  and T0, validated against the official RFC test vectors.
- **Easy secret entry** — padding/whitespace/case-tolerant Base32 decoding and
  `otpauth://` URI parsing, so secrets from another authenticator paste directly.
- **Multi-account store** — issuer/label per account, ordered list.
- **Encrypted vault** — accounts encrypted at rest with a master passphrase
  (PBKDF2 + ChaCha20, encrypt-then-MAC). Optional always-unlocked mode for
  single-user or headless machines.
- **Accurate time without a working clock** — optional SNTP sync over bsdsocket,
  an explicit UTC-offset setting, and a manual nudge, layered so it works with
  zero config on a networked machine and degrades gracefully to a floppy-booted
  A500.
- **GUI + CLI** — a ClassAct/ReAction GUI that runs as a proper commodity
  (resident, hotkey popup, Exchange integration), plus a dependency-free CLI
  (`AmiAuth GET github`) that works down to OS 2.x.

## Design principles

- **Zero mandatory dependencies** beyond the OS — all crypto is vendored; no
  AmiSSL requirement. `bsdsocket` is used only opportunistically for SNTP.
- **Portable, testable core** — the OTP and crypto code is plain C with a
  host-side build target, so RFC vectors run in CI without an emulator.
- **Honest security** — the docs state the threat model plainly. The vault
  protects secrets at rest; it does not defend against a compromised running OS
  (AmigaOS has no memory protection). See [`docs/SECURITY.md`](docs/SECURITY.md).

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — module layout and build targets.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — phased delivery plan and v2 candidates.
- [`docs/SECURITY.md`](docs/SECURITY.md) — threat model and the honest security note.

## Toolchain

C via `amiga-gcc`, GitHub Actions CI running host-side RFC vector tests plus an
m68k build, Aminet packaging via `aminet-release-action`. The core tool targets
plain 68000 to maximise the audience.

## License

BSD 2-Clause. Copyright (c) 2026 Simon Dick. See [`LICENSE`](LICENSE).
