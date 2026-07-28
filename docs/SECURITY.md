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

The iteration count is stored in the vault, but AmiAuth vaults are meant to
travel — copy the drawer to migrate machines (see [STORAGE.md](STORAGE.md)).
Those machines differ in CPU speed by two to three orders of magnitude, and
PBKDF2 is *brutally* slow on the low end: measured on a stock 68000 (7 MHz A500,
`make pbkdf2-bench`), AmiAuth manages only about **14 PBKDF2 iterations per
second** (dkLen=64). A fixed high count is therefore impossible — 10,000
iterations, a modest desktop figure, would take about **12 minutes** to unlock
on a 68000.

So AmiAuth **calibrates the count to the machine that creates the vault** (aiming
~1 s unlock there), and **adapts** as the vault moves:

- **At creation** it times PBKDF2 locally and picks a count for ~1 s on that CPU
  (override with `--iterations N`). A fast machine gets a large count; a 68000
  honestly gets only ~14.
- **At unlock** it times the KDF. If the current machine is *much* faster than
  the stored count assumes, it offers to **strengthen** the vault — re-key to a
  higher, freshly-calibrated count (one confirmation; the passphrase is already
  in hand). If *much* slower (e.g. a vault made on an accelerator, opened on a
  stock 68000), it offers to **speed up** by re-keying lower, behind a security
  warning and a typed confirmation, since that weakens protection. The threshold
  is deliberately generous (~8×) so ordinary variation — emulator warp speed
  included — never nags; only a clear hardware-class change does. Silence the
  prompts with `--no-rekey` or by setting `ENVARC:AmiAuth/rekey` to `off`.

Two honest consequences remain:

1. **A vault is only as strong as the machine that last (re-)keyed it.** A
   68000-created vault carries a tiny count; strengthening it means opening it on
   faster hardware and accepting the re-key.
2. **On slow hardware the count is negligible, so the passphrase is what actually
   protects you.** ~14 iterations buys essentially nothing against an offline
   attacker on fast hardware. No stored count can be both strong on fast machines
   and usable on slow ones, so a long, unpredictable **passphrase** is the real
   defence — choose one accordingly.

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

To keep the nonce safe even when entropy is thin, every request folds in
`DateStamp` and a per-process counter (distinct nonces within one run), and
every save first folds the previous vault file's header+MAC — which includes
the last nonce — into the pool, chaining each save's nonce to the one before
it. So even two runs started from an identical cold-boot state (deterministic
emulator, frozen RTC) produce distinct nonces from their second-ever save
onward, and once any real entropy has influenced a saved nonce that
divergence persists across every later boot. The residual case — the very
first save of each of two runs from a byte-identical cold state — is
documented rather than solved: real entropy additionally makes nonces
*unpredictable*, which no counter can.

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
GUI, hotkey popup, CLI, and ARexx — works with zero prompts.

This is the right trade for a single-user machine at home, a dedicated emulator
instance, or scripted/headless use where the CLI must run non-interactively.
Forcing a passphrase there just pushes people toward weak passphrases or
plaintext exports anyway.

State it plainly: **in this mode there is no at-rest protection — anyone with the
file has the secrets.** It is a deliberate opt-out, never the default. It is
convertible in both directions from settings (add/change/remove passphrase,
re-encrypting or decrypting the vault on disk). Auto-lock and ARexx
`LOCK`/`UNLOCK` are no-ops (RC 0, `RESULT "always-unlocked"`) for it;
`STATUS` reports the mode explicitly.

## ARexx port

AmiAuth exposes a public `AMIAUTH.<n>` ARexx port (GUI only; see
[ARexx Port](../userdocs/ARexx-Port.md) for the full command reference). One
hard rule governs it: **the port never carries the passphrase.** `UNLOCK` is
exclusively interactive — it reuses the same GUI passphrase requester as the
window/hotkey/commodity paths, never accepting one over the port. Scripts
otherwise operate against a vault the user has already unlocked.

The `ENVARC:AmiAuth/arexxgetcode` setting (default on; `off` disables) lets
cautious users restrict the port to control commands (`STATUS`, `LOCK`,
`UNLOCK`, `SHOW`, `HIDE`, `QUIT`) and deny `GETCODE`/`TIMELEFT`. `GETCODE`
returns the same RC (20, failure) whether the vault is locked or the pref is
off — deliberately indistinguishable, so a script can't use the RC alone to
probe *why* it was refused.

Any running program on the system can drive the port while the vault is
unlocked — not materially worse than the no-memory-protection baseline
above, but worth saying plainly.

## Scope discipline

AmiAuth is an authenticator, not a password manager. It will not grow into a
general secret vault.
