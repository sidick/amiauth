# Security Model

This page states AmiAuth's security model plainly, without hedging. TOTP secrets
are the crown jewels: anyone who has them can generate your codes.

## What the vault protects

The account database is encrypted at rest with a master passphrase:

- **KDF:** PBKDF2-HMAC-SHA1. The iteration count is chosen at vault creation,
  calibrated to the creating machine, and stored (with the salt) in the file
  header.
- **Cipher:** ChaCha20 in an encrypt-then-MAC construction with HMAC-SHA1.
  ChaCha20 is fast on 68k (pure 32-bit operations — no tables, no multiplies)
  and modern; SHA-1 is already in the binary for HMAC/OTP.

This defends against an attacker who obtains the **vault file at rest**: a
stolen hard-drive image, a backup, a shared machine's disk.

> **On SHA-1:** HMAC-SHA1 and PBKDF2-HMAC-SHA1 remain cryptographically sound as
> a MAC and a KDF — SHA-1's collision weaknesses do not translate into attacks
> on either construction. SHA-1 is reused deliberately to keep the crypto
> footprint small.

## What the vault does NOT protect against

It does **not** defend against a compromised running OS. AmigaOS has no memory
protection — any running program can read any other program's memory.
Pretending otherwise would be dishonest. Specifically:

- While the vault is unlocked and resident (the commodity's normal state), the
  derived key and decrypted secrets are in memory and readable by any process.
- Mitigations reduce, but do not eliminate, this exposure window:
  - the optional **auto-lock timeout** re-requires the passphrase after idle time;
  - the explicit **lock** action;
  - key material is **zeroed** on lock and on quit.

AmiAuth is a convenience tool for classic hardware, not a hardware security
key. Please weigh that before trusting it with high-value accounts.

## KDF cost across the hardware range

AmiAuth vaults are meant to travel — copy the drawer to migrate machines. Those
machines differ in CPU speed by two to three orders of magnitude, and PBKDF2 is
*brutally* slow on the low end: a stock 7 MHz 68000 manages only about **14
PBKDF2 iterations per second**. A fixed high count is therefore impossible —
10,000 iterations, a modest desktop figure, would take about **12 minutes** to
unlock on a 68000.

So AmiAuth **calibrates the count to the machine that creates the vault**
(aiming for ~1 second to unlock there) and **adapts** as the vault moves:

- **At creation** it times PBKDF2 locally and picks a count for ~1 s on that
  CPU. A fast machine gets a large count; a 68000 honestly gets a small one.
- **At unlock** it times the KDF again. If the current machine is *much* faster
  than the stored count assumes, it offers to **strengthen** the vault (re-key
  to a higher count — one confirmation, since the passphrase is already in
  hand). If *much* slower, it offers to **speed up** by re-keying lower, behind
  a security warning and a typed confirmation, since that weakens protection.
  The threshold is deliberately generous (~8×) so ordinary variation — emulator
  warp speed included — never nags; only a clear hardware-class change does.

See [Vault and Passphrases](Vault-and-Passphrases.md) for the user-facing side of re-keying and how to
silence the prompts.

Two honest consequences remain:

1. **A vault is only as strong as the machine that last (re-)keyed it.** A
   68000-created vault carries a tiny count; strengthening it means opening it
   on faster hardware and accepting the re-key.
2. **On slow hardware the iteration count buys essentially nothing**, so a
   long, unpredictable **passphrase is what actually protects you**. No stored
   count can be both strong on fast machines and usable on slow ones — choose
   your passphrase accordingly.

## Always-unlocked mode

The passphrase is optional at vault creation. Without one, the vault is stored
**unencrypted** (same file format, cipher marked `none`) and every entry point
works with zero prompts.

This is the right trade for a single-user machine at home, a dedicated emulator
instance, or scripted use where the CLI must run non-interactively. Forcing a
passphrase there just pushes people toward weak passphrases or plaintext
exports anyway.

Stated plainly: **in this mode there is no at-rest protection — anyone with the
file has the secrets.** It is a deliberate opt-out, never the default, and is
convertible in both directions (add or remove the passphrase later; the vault
is re-encrypted or decrypted on disk).

## The passphrase never travels

The vault passphrase is only ever entered **interactively** — at a Shell prompt
(RAW mode, no echo) or in a GUI requester. There is deliberately no way to
supply it on the command line, in an environment variable, in a script, or over
the commodity's message port. Scripted/headless use is served by always-unlocked
vaults instead. When the CLI forwards commands to a resident GUI, the GUI is
the one that holds the unlocked vault — the passphrase never crosses the port.

## Randomness

The vault's salt (per vault) and ChaCha20 nonce (per save) must be
unpredictable. AmigaOS has no strong system RNG, so AmiAuth gathers entropy
itself — timer `EClock` jitter, volatile system state, and, when creating or
unlocking an encrypted vault, the **timing of your keystrokes** as the
passphrase is typed — and whitens it through an HMAC-DRBG.

Honestly stated: on a quiescent 68000 the passive sources yield little entropy,
and under a deterministic emulator almost none. The keystroke timing is the
main real source at the moment it matters (vault creation), and — as everywhere
on this page — a long, unpredictable passphrase is what actually protects an
encrypted vault. To keep the nonce safe even when entropy is thin, every
request also folds in a timestamp and a monotonic counter, so nonces are always
*distinct*; real entropy additionally makes them *unpredictable*.

## Scope discipline

AmiAuth is an authenticator, not a password manager. It will not grow into a
general secret vault.

## Verifying the crypto yourself

- All primitives (SHA-1, SHA-256, SHA-512, HMAC, PBKDF2, ChaCha20) and both
  OTP modes (TOTP/HOTP) across all three hash algorithms are validated
  against their published RFC test vectors in CI on every commit.
- An opt-in CI job differentially fuzzes the primitives against OpenSSL.
- The full source is BSD-licensed:
  [github.com/sidick/amiauth](https://github.com/sidick/amiauth). The frozen
  on-disk vault format is specified in
  [`docs/VAULT_FORMAT.md`](https://github.com/sidick/amiauth/blob/main/docs/VAULT_FORMAT.md).
