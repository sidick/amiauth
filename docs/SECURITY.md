# Security note

This document states AmiAuth's security model plainly, without hedging. TOTP
secrets are the crown jewels: anyone who has them can generate your codes.

## What the vault protects

The account database is encrypted at rest with a master passphrase:

- **KDF:** PBKDF2-HMAC-SHA1 with a calibrated iteration count. The count is
  measured at first run (target ~1 second on the host CPU) so it self-tunes
  across everything from a 68000 to a Vampire or emulated 68040. Salt and
  iteration count are stored in the file header.
- **Cipher:** ChaCha20 in an encrypt-then-MAC construction with HMAC-SHA1.
  ChaCha20 is fast on 68k (pure 32-bit ARX operations — no tables, no multiplies)
  and modern; SHA-1 is already in the binary for HMAC/OTP.

This defends against an attacker who obtains the **vault file at rest**: a stolen
HD image, a backup, a shared machine's disk.

## What the vault does NOT protect against

It does **not** defend against a compromised running OS. AmigaOS has no memory
protection — any running program can read another's memory. Pretending otherwise
would be dishonest. Specifically:

- While the vault is unlocked and resident (the commodity's normal state), the
  derived key and decrypted secrets are in memory and readable by any process.
- Mitigations reduce, but do not eliminate, this exposure window:
  - Optional **auto-lock timeout** re-requires the passphrase after N idle minutes.
  - Explicit **lock** action (menu and Exchange).
  - Key material is **zeroed** on lock and on quit.

## Always-unlocked mode

The passphrase is optional at vault creation. Without one, the vault is stored
**unencrypted** (same file format, cipher marked `none`) and every entry point —
GUI, hotkey popup, CLI, and (v2) ARexx — works with zero prompts.

This is the right trade for a single-user machine at home, a dedicated emulator
instance, or scripted/headless use where the CLI must run non-interactively.
Forcing a passphrase there just pushes people toward weak passphrases or
plaintext exports anyway.

State it plainly: **in this mode there is no at-rest protection — anyone with the
file has the secrets.** It is a deliberate opt-out, never the default. It is
convertible in both directions from settings (add/change/remove passphrase,
re-encrypting or decrypting the vault on disk). Auto-lock and (v2) ARexx
`LOCK`/`UNLOCK` become no-ops; `STATUS` reports the mode explicitly.

## ARexx port (v2)

If/when an ARexx port ships, one hard rule governs it: **the port never carries
the passphrase.** Unlocking is exclusively interactive (GUI requester); scripts
operate against a vault the user has already unlocked. A per-vault "allow ARexx
`GETCODE`" setting (default on) lets cautious users restrict the port to control
commands only. The security note will state plainly that any running program can
drive the port while unlocked — not materially worse than the no-memory-protection
baseline, but worth saying.

## Scope discipline

AmiAuth is an authenticator, not a password manager. It will not grow into a
general secret vault.
