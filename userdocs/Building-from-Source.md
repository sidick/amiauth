# Building from Source

AmiAuth is plain C, split into a portable core (builds with any host compiler,
fully testable off-Amiga) and Amiga front-ends cross-built with
[amiga-gcc](https://github.com/bebbo/amiga-gcc). The shipped Amiga binaries
have **zero external dependencies**; the build/test tooling below (Docker,
OpenSSL, Python) never touches what ships.

## Quick reference

    make test         # host unit + RFC-vector tests
    make cli          # native host CLI        -> build/amiauth-host
    make smoke        # end-to-end CLI smoke test
    make diff         # differential fuzz vs OpenSSL (opt-in; needs libcrypto)
    make m68k-docker  # AmigaOS CLI via the amiga-gcc container -> build/AmiAuth
    make gui-docker   # AmigaOS ReAction GUI                    -> build/AmiAuthGUI
    make gui-smoke    # headless GUI render test (WB 3.2 under Copperline)
    make pbkdf2-bench # measure PBKDF2 speed (the KDF calibration numbers)
    make dist         # package the Aminet upload pair -> build/dist/ (builds its own lha)

## Host build and tests

`make test` compiles the core natively and runs the unit tests plus the
official RFC test vectors — RFC 6238 (TOTP), RFC 4226 (HOTP), FIPS/RFC SHA-1,
HMAC, RFC 8439 ChaCha20, RFC 6070 PBKDF2 — and a byte-exact golden vault
fixture. Any host C compiler works; there are no dependencies.

`make cli` produces `build/amiauth-host`, a fully working CLI for the host
(handy for preparing a vault to copy to an Amiga):

    build/amiauth-host CODE JBSWY3DPEHPK3PXP

`make diff` (optional, needs OpenSSL's libcrypto) differentially fuzzes every
crypto primitive against OpenSSL as a reference oracle — the same job CI runs.

## Amiga builds

The supported cross-build path uses the containerised amiga-gcc toolchain, so
you need Docker (or a compatible runtime) but no local toolchain install:

    make m68k-docker   # -> build/AmiAuth     (CLI, OS 2.04+, plain 68000)
    make gui-docker    # -> build/AmiAuthGUI  (ReAction GUI, OS 3.0+)

Both binaries target **plain 68000** (`-m68000`) — the project's baseline; no
020+ instructions anywhere.

`make gui-smoke` boots a Workbench 3.2 environment headlessly under the
Copperline emulator and verifies the GUI actually renders — the automated
on-target test CI uses.

## Releases

Releases are tag-driven and semver: a release PR bumps the version in
`src/version.h` (the single source of truth, embedded in both binaries as an
AmigaOS `$VER:` string) and the `Version:` field of `AmiAuth.readme`; pushing
the matching `v<version>` tag then builds the binaries, packages
`build/dist/AmiAuth.lha` (`make dist`), publishes a GitHub release with
generated notes, and — after a manual approval step — uploads the package to
Aminet. The workflow refuses a tag that doesn't match both version fields.

## Continuous integration

Every commit runs: the host RFC-vector/unit tests, the CLI smoke test, the
OpenSSL differential fuzz job, and the m68k cross-build. Releases are packaged
for Aminet. The workflows in
[`.github/workflows/`](https://github.com/sidick/amiauth/tree/main/.github/workflows)
are the authoritative reference.

## Source layout

See
[`docs/ARCHITECTURE.md`](https://github.com/sidick/amiauth/blob/main/docs/ARCHITECTURE.md)
for the module map: `src/core/` (crypto, OTP, vault, clock — portable),
`src/cli/`, `src/gui/`, `src/amiga/` (OS glue: SNTP transport, entropy,
prefs, GUI port), `src/qr/` (vendored quirc), `tests/`.
