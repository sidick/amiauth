# AmiAuth

A native two-factor authentication (2FA) code generator for classic AmigaOS.

AmiAuth implements TOTP (RFC 6238) and HOTP (RFC 4226): it stores multiple
accounts, generates 6- or 8-digit codes with a live countdown, and — critically
for the platform — solves the accurate-time problem that TOTP depends on. Small,
self-contained, and designed to run on anything from a stock 68000 A500 up to an
accelerated or emulated machine.

> **Status:** [v1.0 released](https://github.com/sidick/amiauth/releases/tag/v1.0)
> (see the [v1.0 milestone](https://github.com/sidick/amiauth/milestone/1)).
> The portable core (crypto, OTP, vault, `otpauth://` import), the **CLI**, and
> a resident **ReAction GUI commodity** (live all-accounts view,
> add/remove/edit, clipboard copy, QR-image import, hotkey/Exchange/WBStartup,
> single-instance) are all RFC-verified, cross-build to real AmigaOS binaries,
> and have passed an interactive on-hardware verification pass on OS 3.2,
> including logging into GitHub with a code generated on real hardware.
> Post-v1.0 ideas are tracked in the
> [v2 milestone](https://github.com/sidick/amiauth/milestone/2).

## AI-assisted development

Be aware: **AmiAuth was written largely by an AI coding agent** (Anthropic's
Claude, via Claude Code), working under human direction. The scope, design
decisions, and on-hardware testing were human-directed and reviewed; most of the
code itself was AI-generated.

Because this is a security tool, that disclosure matters — please weigh your
trust accordingly rather than taking it on faith. To make the code auditable
instead of asking for blind trust, the cryptographic primitives (SHA-1, HMAC,
PBKDF2, ChaCha20) are checked against their published RFC test vectors and
differentially fuzzed against OpenSSL in CI, and the entire source is
BSD-licensed and open for review. Read [`docs/SECURITY.md`](docs/SECURITY.md)
and judge it for yourself.

## Why

No TOTP tool exists for classic AmigaOS. Anyone using an Amiga day-to-day still
reaches for a phone to log into GitHub, forge sites, or their own services.
AmiAuth aims to make "my A1200 is my 2FA device" a real, daily-useful thing.

## Features (v1)

All implemented:

- **TOTP & HOTP** — SHA-1, 6/8-digit codes, configurable period (30s default)
  and T0, validated against the official RFC test vectors.
- **Easy secret entry** — padding/whitespace/case-tolerant Base32 decoding and
  `otpauth://` URI parsing, so secrets from another authenticator paste directly.
- **Multi-account store** — issuer/label per account, ordered list.
- **Encrypted vault** — accounts encrypted at rest with a master passphrase
  (PBKDF2 + ChaCha20, encrypt-then-MAC), with a per-machine KDF calibration and
  adaptive re-key, or an optional always-unlocked mode for single-user/headless
  machines. Encrypted create/save works on real hardware (AmigaOS CSPRNG).
- **Accurate time without a working clock** — SNTP sync over `bsdsocket`, a
  `locale.library` offset, and a manual offset/nudge, layered so it works with
  zero config on a networked machine and degrades gracefully to a floppy-booted
  A500, with a red/amber/green trust indicator in the GUI.
- **CLI** — dependency-free, works down to OS 2.x:
  `CODE`, `INIT`, `ADD`, `LIST`, `GET`, `REMOVE`, `SHOW`, `CLOCK`, `SYNC`,
  `OFFSET`. On Amiga it uses standard `ReadArgs` parsing (`AmiAuth GET GitHub`,
  options as keywords like `VAULT`/`ITERATIONS`; `AmiAuth ?` for the template,
  `HELP` for the command list).
- **ReAction GUI** — a live all-accounts list with per-account codes + countdown,
  a big selected-code display and fuelgauge, add / remove / edit, clipboard copy
  (with auto-clear), the clock-status LED, idle auto-lock for encrypted vaults,
  and **QR-image import** (decode an `otpauth://` enrolment QR from a
  PNG/JPEG/GIF/IFF via a file requester or drag-and-drop).
- **Background commodity** — the GUI lives in Exchange, pops up on a hotkey, runs
  from WBStartup, and is single-instance: one resident process holds the unlocked
  vault, and the CLI forwards commands to it (no second passphrase prompt) rather
  than opening the vault independently.

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
    make m68k-docker  # AmigaOS CLI via the amiga-gcc container -> build/AmiAuth
    make gui-docker   # AmigaOS ReAction GUI  -> build/AmiAuthGUI
    make gui-smoke    # headless GUI render test (WB 3.2 under Copperline)

The core is portable C, so `test`/`cli` build with any host compiler. Example:
`build/amiauth-host CODE JBSWY3DPEHPK3PXP` prints a code.

## Running the GUI as a commodity

`AmiAuthGUI` registers as an AmigaOS **Commodity**: it appears in **Exchange**
(Show/Hide/Enable/Disable/Kill), pops up on a **hotkey**, and is **single
instance** — one resident process holds the unlocked vault, and a second launch
just makes the running one appear. Closing the window **hides** it (the process
stays resident, vault unlocked); the hotkey or Exchange "Show" brings it back.
Quit for real from **Project → Quit** or Exchange's "Kill".

Drop it in **WBStartup** (or run from the Startup-Sequence) with these icon
**tooltypes**:

| Tooltype       | Default        | Meaning                                        |
|----------------|----------------|------------------------------------------------|
| `CX_POPKEY`    | `ctrl alt a`   | hotkey that shows/raises the window            |
| `CX_POPUP`     | `yes`          | `no` = start hidden (window opens on the hotkey) |
| `CX_PRIORITY`  | `0`            | commodity broker priority                      |
| `TIMESERVER`   | `pool.ntp.org` | SNTP server for the startup time sync (else the saved `server` pref) |
| `DONOTWAIT`    | —              | (WBStartup) don't make Workbench wait for it   |

On startup the GUI does one SNTP time sync (if a TCP/IP stack is up) so its clock
is accurate — this is what lights the status LED green; it fails quietly offline
and falls back to the saved offset. Without `commodities.library` the GUI
degrades to a plain window (close = quit).

## Documentation

**User documentation lives at [sidick.github.io/amiauth](https://sidick.github.io/amiauth/)** —
installation, getting started, the CLI command reference, the GUI and
commodity/tooltypes, vault and passphrases, time sync, settings, and
troubleshooting/FAQ. The site is versioned per release (version picker in
the header); its source is [`userdocs/`](userdocs/) in this repository, and
the same pages become the `AmiAuth.guide` shipped in the release archive.

Developer-facing design notes live in `docs/`:

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — module layout and build targets.
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
