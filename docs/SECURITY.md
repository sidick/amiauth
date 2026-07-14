# Security note

This document states AmiAuth's security model plainly, without hedging. TOTP
secrets are the crown jewels: anyone who has them can generate your codes.

## What the vault protects

The account database is encrypted at rest with a master passphrase:

- **KDF:** PBKDF2-HMAC-SHA1. The iteration count is chosen at vault creation and
  stored (with the salt) in the file header. See "KDF cost across the hardware
  range" below — the count is a policy choice, not a raw "1 second here"
  measurement, because the vault is portable across machines that differ in speed
  by 100–1000×.
- **Cipher:** ChaCha20 in an encrypt-then-MAC construction with HMAC-SHA1.
  ChaCha20 is fast on 68k (pure 32-bit ARX operations — no tables, no multiplies)
  and modern; SHA-1 is already in the binary for HMAC/OTP.

This defends against an attacker who obtains the **vault file at rest**: a stolen
HD image, a backup, a shared machine's disk.

## KDF cost across the hardware range

The iteration count is frozen into the vault when it is created, but AmiAuth
vaults are meant to travel — copy the drawer to migrate machines (see
[STORAGE.md](STORAGE.md)). Those machines can differ in CPU speed by two to three
orders of magnitude, from a stock 7MHz 68000 to a Vampire/68080 or an emulated
machine on a modern host. Two honest consequences follow:

1. **A vault created on a fast machine can be slow to unlock on a slow one.** A
   count calibrated to ~1s under emulation could take *minutes* on a real 68000.
   Because unlock is on the critical path of a login, AmiAuth therefore does not
   calibrate aggressively to the local CPU: it uses a conservative default and
   **caps** the count so worst-case unlock on the slow end stays bounded (a few
   seconds). A user on fast hardware may opt into a higher count, accepting slower
   portability; re-tuning is a one-step re-encrypt (change passphrase / re-key),
   since the count travels with the vault, not the machine.

2. **On slow hardware the iteration count is modest, so passphrase strength is
   what actually protects you.** A full second of PBKDF2 on a 68000 is only a few
   thousand iterations — far below modern guidance (which is in the millions). No
   single stored count can be both strong on fast machines and usable on slow
   ones. Against an attacker who copies your vault and runs an *offline* guessing
   attack on fast hardware, the iteration count buys only a small factor; a long,
   unpredictable passphrase is the real defence. Choose one accordingly.

## Randomness (salt and nonce)

The vault's salt (per vault) and ChaCha20 nonce (per save) must be
unpredictable, and the nonce must **never repeat** under a fixed key — a repeated
nonce breaks confidentiality outright. The core takes both as parameters and
stays deterministic; generating them is a front-end job.

- **Host CLI:** `/dev/urandom`.
- **AmigaOS:** there is no strong system RNG, so AmiAuth gathers entropy itself
  (`src/amiga/random.c`) and whitens it through an HMAC-DRBG (`src/core/drbg.c`,
  reusing the SHA-1 already in the binary). Sources: timer.device `EClock` timing
  jitter sampled in a tight loop, `DateStamp`, free-memory figures, task and
  allocation addresses plus a fresh allocation's residual bytes, and — when
  creating or unlocking an encrypted vault — the **timing of the user's
  keystrokes** as the passphrase is typed (RAW-mode, no echo).

Be honest about the ceiling: on a quiescent 68000 the passive sources yield
little entropy, and under a deterministic emulator they yield almost none. The
interactive keystroke timing is therefore the main real source at the moment it
matters (vault creation), and — as everywhere in this document — a long,
unpredictable **passphrase** is what actually protects an encrypted vault, not
the salt's entropy.

To keep the nonce safe even when entropy is thin, every request also folds in
`DateStamp` and a monotonic counter, so successive nonces are *distinct*
regardless of entropy quality; real entropy additionally makes them
*unpredictable*.

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
