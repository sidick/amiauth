# Changelog

What changed in each AmiAuth release, most recent first. For the full
commit-level history see the
[GitHub releases](https://github.com/sidick/amiauth/releases).

## 1.1 (28.07.2026)

### New features

- **SHA-256 and SHA-512 TOTP variants** — accounts using
  `algorithm=SHA256`/`SHA512` in their otpauth URI now work; see
  [Managing Accounts](Managing-Accounts.md).
- **Steam Guard support** — Steam's 5-character alphanumeric TOTP variant.
- **QR code export** — display an account as a QR code to scan with a phone:
  on-screen in the GUI, ASCII-art in the CLI.
- **ARexx port** — automate the running GUI from scripts; see
  [ARexx Port](ARexx-Port.md).
- **Localization** — all CLI and GUI strings can now come from
  locale.library catalogs, with fallback through each of your preferred
  languages; see [Localization](Localization.md).
- **Bare Base32 secrets** — when adding an account you can paste just the
  Base32 secret, without an `otpauth://` wrapper.
- **GUI `VAULT` argument** — the GUI accepts a vault path as a tooltype or
  Shell argument, like the CLI.

### Improvements

- Faster code generation on plain 68000s: the SHA-1 hot loop is now
  hand-written m68k assembly.
- The GUI opens its window immediately and runs the startup SNTP sync
  afterwards, instead of blocking on the network first.

### Fixes

- Security hardening: all findings from the July 2026 independent code
  audits fixed (high, medium and low severity).
- Updated the vendored quirc QR decoder, fixing memory-safety bugs on
  malformed QR input.

## 1.0 (20.07.2026)

Initial release: TOTP (RFC 6238) and HOTP (RFC 4226) code generation,
encrypted vault, CLI for AmigaOS 2.04+ and ReAction GUI for AmigaOS 3.0+,
QR import, SNTP clock sync, and commodity support.
