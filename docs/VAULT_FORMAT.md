# AmiAuth vault format v1

The on-disk format for the encrypted account store. This is the one part of
AmiAuth with no RFC to anchor it, so it is specified here in full and frozen
before any vault is ever written to disk. Read alongside [SECURITY.md](SECURITY.md).

## Goals

- **Confidentiality + integrity at rest** under a master passphrase
  (encrypt-then-MAC: ChaCha20 + HMAC-SHA1), keyed by PBKDF2-HMAC-SHA1.
- **Tamper-evidence for the whole file**, header included — an attacker cannot
  downgrade the iteration count or swap the nonce/salt undetected.
- **A single uniform layout** for both encrypted and always-unlocked vaults, so
  there is one parser and conversion in either direction is a re-save.
- **68000-friendly**: all integers big-endian and byte-packed — no alignment or
  struct-padding assumptions. Portable between host and Amiga builds.
- **Forward-evolvable**: a version byte and reserved fields allow later change
  without a flag-day.

## Conventions

- Multi-byte integers are **unsigned big-endian** (network byte order).
- The file is **byte-packed**: no padding between fields.
- Byte ranges are written `[start, end)` (end-exclusive).

## File layout

```
+---------+------+-----------------------------------------------------------+
| Offset  | Size | Field                                                     |
+---------+------+-----------------------------------------------------------+
|   0     |  4   | magic = "AAVT"  (0x41 0x41 0x56 0x54)                      |
|   4     |  1   | format_version = 0x01                                     |
|   5     |  1   | cipher_id   (0 = none, 1 = ChaCha20)                      |
|   6     |  1   | kdf_id      (0 = none, 1 = PBKDF2-HMAC-SHA1)              |
|   7     |  1   | flags       (reserved, must be 0)                         |
|   8     |  4   | kdf_iterations (u32)   — 0 when kdf_id = none             |
|  12     | 16   | salt                   — all-zero when kdf_id = none      |
|  28     | 12   | nonce (ChaCha20)       — all-zero when cipher_id = none   |
|  40     |  4   | payload_len (u32)      — length of the plaintext payload  |
+---------+------+-----------------------------------------------------------+  <- header ends (44 bytes)
|  44     | 20   | mac = HMAC-SHA1(mac_key, header[0,44) || ciphertext)      |
+---------+------+-----------------------------------------------------------+
|  64     |  N   | ciphertext  (N = payload_len)                             |
+---------+------+-----------------------------------------------------------+
```

Fixed constants: `HEADER_SIZE = 44`, `MAC_OFFSET = 44`, `MAC_SIZE = 20`,
`CIPHERTEXT_OFFSET = 64`. Total file size = `64 + payload_len`.

The header (bytes `[0, 44)`) is **authenticated but not encrypted** — its fields
are needed to derive the key and verify the MAC, and the MAC covers them so they
cannot be tampered with.

## Cryptographic construction

### Key derivation

```
dk      = PBKDF2-HMAC-SHA1(passphrase, salt, kdf_iterations, dkLen = 64)
enc_key = dk[0, 32)      # ChaCha20 key
mac_key = dk[32, 64)     # HMAC-SHA1 key
```

A single PBKDF2 call produces 64 bytes, split into independent encryption and MAC
keys. `kdf_iterations` is chosen at vault creation and stored in the header so any
machine can open the vault regardless of where it was made.

The count is a **front-end policy decision, not a value `vault.c` measures**:
`vault.c` takes an explicit iteration count and stays deterministic (so the golden
fixture is byte-exact). The front-end calibrates it to ~1s on the
creating machine and adaptively re-keys as the vault migrates to faster or slower
hardware (a stock 68000 does only ~14 PBKDF2/s, so the count is per-machine). See
[SECURITY.md](SECURITY.md) "KDF cost across the hardware range".
`VAULT_DEFAULT_ITERATIONS` (vault.h) is now just the fallback when no timer is
available; `vault_calibrate_iterations` provides the (host-tested) policy math.

> **On SHA-1:** HMAC-SHA1 and PBKDF2-HMAC-SHA1 remain cryptographically sound as
> a MAC and a KDF — SHA-1's collision weaknesses do not translate into attacks on
> either construction. SHA-1 is reused deliberately (it is already in the binary
> for OTP), keeping the crypto footprint small.

### Encryption (cipher_id = 1, ChaCha20)

```
ciphertext = ChaCha20(key = enc_key, nonce = nonce, counter = 0) XOR payload
```

- RFC 8439 ChaCha20 keystream, **initial block counter 0**.
- The `nonce` is freshly generated at **every save** (see RNG note). Under a fixed
  `enc_key`, a nonce must never repeat, so a fresh 96-bit random nonce per save is
  required; salt (hence key) is stable across saves so the resident key is reused.

### MAC (encrypt-then-MAC)

```
mac = HMAC-SHA1(mac_key, header[0, 44) || ciphertext)
```

- Computed over the **whole header and the ciphertext**, so both are
  tamper-evident.
- On load the MAC is verified **before** any decryption, using a **constant-time**
  comparison. A mismatch is reported as `VAULT_ERR_AUTH` — it means either a wrong
  passphrase or a corrupted/tampered file; the two are deliberately
  indistinguishable.

### Always-unlocked mode (cipher_id = 0, kdf_id = 0)

Same layout, no confidentiality:

- `salt` and `nonce` are all-zero; `kdf_iterations = 0`.
- `enc_key` is unused; `ciphertext` **is** the plaintext payload.
- `mac_key` is the **empty (zero-length) key**, so
  `mac = HMAC-SHA1("", header || payload)`.

Because the MAC key is empty, anyone with the file can recompute the MAC: in this
mode it is an **integrity check (corruption detection), not authentication**. This
matches [SECURITY.md](SECURITY.md): always-unlocked means no at-rest protection,
stated without hedging.

## Plaintext payload

The payload (the plaintext that is encrypted, or stored directly when
always-unlocked) is the ordered account list:

```
+------+------------------------------------------------------------+
| Size | Field                                                      |
+------+------------------------------------------------------------+
|  2   | account_count (u16)                                        |
+------+------------------------------------------------------------+
        then account_count records, each:
+------+------------------------------------------------------------+
|  1   | type       (0 = TOTP, 1 = HOTP)                            |
|  1   | algorithm  (0 = SHA1, 1 = SHA256, 2 = SHA512)              |
|  1   | digits     (6 or 8)                                         |
|  4   | period     (u32, seconds; TOTP)                            |
|  8   | counter    (u64; HOTP)                                     |
|  1   | secret_len (S)                                             |
|  S   | secret     (raw key bytes)                                 |
|  1   | issuer_len (I)                                             |
|  I   | issuer     (UTF-8)                                         |
|  1   | label_len  (L)                                             |
|  L   | label      (UTF-8)                                         |
+------+------------------------------------------------------------+
```

- `period` is ignored for HOTP; `counter` is ignored for TOTP. Both are always
  present for a fixed record shape.
- Length prefixes are single bytes; the field caps (`OTP_MAX_SECRET = 64`,
  `issuer ≤ 64`, `label ≤ 128`, `VAULT_MAX_ACCOUNTS = 64`) all fit in a byte /
  the u16 count.
- There is no trailing padding; `payload_len` covers the payload exactly.

## Load algorithm

1. Read the file. Require at least `CIPHERTEXT_OFFSET` (64) bytes.
2. Verify `magic`. Reject unknown `magic`/`format_version` with `VAULT_ERR_FORMAT`.
3. Parse the header. Require file size == `64 + payload_len` (else
   `VAULT_ERR_FORMAT` — truncation/corruption).
4. Derive keys:
   - `kdf_id = pbkdf2`: a passphrase is required; if none is supplied return
     `VAULT_ERR_LOCKED`. Derive `enc_key`/`mac_key`.
   - `kdf_id = none`: `mac_key` is empty; there is no `enc_key`.
5. Recompute the MAC over `header || ciphertext` and compare in constant time.
   Mismatch → `VAULT_ERR_AUTH`.
6. Recover the payload: `cipher_id = chacha20` → decrypt; `none` → payload is the
   ciphertext bytes.
7. Parse the payload into accounts. Any inconsistency (bad count, a length prefix
   running past `payload_len`, or a per-record `algorithm` id the reader does
   not implement) → `VAULT_ERR_FORMAT`.
8. Mark the vault unlocked; retain `salt`, `enc_key`, `mac_key` while resident.

## Save algorithm

1. Serialise the accounts into the payload; set `payload_len`.
2. Encrypted: generate a **fresh random 12-byte nonce** and encrypt the payload
   (salt/keys are unchanged and reused from the resident state). Always-unlocked:
   `ciphertext = payload`, `nonce = 0`.
3. Build the header.
4. `mac = HMAC-SHA1(mac_key, header || ciphertext)` (`mac_key` empty when
   always-unlocked).
5. Write `header || mac || ciphertext` to a **temporary file in the same
   directory**, flush it, then **atomically rename** it over the target. A crash
   mid-save therefore never destroys the existing vault.

## Passphrase operations

- **Add / change passphrase:** generate a **new salt**, (re)calibrate or keep
  `kdf_iterations`, derive fresh keys, set `cipher_id/kdf_id` to ChaCha20/PBKDF2,
  then save (which picks a fresh nonce).
- **Remove passphrase:** set `cipher_id/kdf_id = none`, zero salt/nonce/iterations,
  save the payload in the clear.
- Both are a full re-save; the format makes conversion in either direction a
  normal write.

## Versioning and forward compatibility

- `format_version` gates the whole layout. A reader must **refuse** a version it
  does not understand (`VAULT_ERR_FORMAT`) rather than guess.
- New `cipher_id` / `kdf_id` / `algorithm` values extend the scheme without a
  version bump, as long as the layout is unchanged; a reader rejects ids it does
  not implement.
- `flags` (header byte 7) is reserved for future boolean options and must be
  written as 0 and, for v1, ignored on read unless a bit is defined later.

## Implementation notes (not part of the format)

- **Randomness is security-critical.** Salt and, especially, the per-save nonce
  must come from a good CSPRNG (a repeated nonce under a fixed key breaks
  confidentiality). AmigaOS has no strong built-in source, so the front-end
  (`src/amiga/random.c`) gathers entropy — `EClock` timing jitter, volatile
  system state, and interactive keystroke timing — and whitens it through an
  HMAC-DRBG (`src/core/drbg.c`); each request also folds a counter/timestamp so
  the nonce never repeats under a fixed key. The core takes salt/nonce as
  parameters and stays deterministic. See [SECURITY.md](SECURITY.md)
  "Randomness" for the sources and honest limits.
- **Key hygiene:** `enc_key`/`mac_key` are zeroed on lock and quit (see
  `vault_lock`). The resident-key exposure window is covered in SECURITY.md.
- **`vault` struct impact:** the resident state needs `enc_key[32]` and
  `mac_key[32]` (the current single `key[32]` becomes two), plus the parsed
  `cipher_id`/`kdf_id`/`iterations`/`salt` already present. The nonce is not
  retained — it is regenerated per save.
- **Golden test vector:** once implemented, commit one fixed known vault (fixed
  passphrase, salt, nonce, iteration count, one account) as a byte-exact
  regression fixture so the format cannot silently drift.
