# Roadmap

Phased delivery. Each phase is scoped to leave the project in a usable,
tested state.

## Phase 1 — OTP core (≈1 weekend)
SHA-1, HMAC, TOTP/HOTP core, Base32 — all validated against RFC test vectors on
the host. CLI tool generating codes from a plaintext secret.

- [ ] `sha1.c` + host test (FIPS/RFC vectors)
- [ ] `hmac.c` (HMAC-SHA1) + vectors
- [ ] `otp.c` — HOTP (RFC 4226 App. D) and TOTP (RFC 6238 App. B) vectors pass
- [ ] `base32.c` — tolerant decode + tests
- [ ] Minimal CLI: generate a code from a plaintext secret
- [ ] Host build target + CI running vectors on every commit

## Phase 2 — Vault (≈1 weekend)
PBKDF2, ChaCha20, encrypted account store, `otpauth://` import. CLI complete.

- [x] `chacha20.c` + vectors (RFC 8439)
- [x] `pbkdf2.c` (PBKDF2-HMAC-SHA1) + vectors (RFC 6070)
- [x] `vault.c` — encrypt-then-MAC store, header (salt/iters/cipher), load/save,
      atomic replace, constant-time MAC verify, golden fixture ([VAULT_FORMAT.md](VAULT_FORMAT.md))
- [x] Always-unlocked mode (cipher `none`), convertible both directions
- [x] Differential test harness: fuzz all crypto primitives (SHA-1, HMAC,
      ChaCha20, PBKDF2) against an OpenSSL reference oracle. Separate opt-in
      target/CI job; default suite stays dependency-free RFC vectors. (Host-only
      test dependency — the shipped binary stays dependency-free.)
- [x] `uri.c` — `otpauth://` parsing + import
- [x] CLI: `INIT`/`ADD`/`LIST`/`GET`/`REMOVE` (+ `CODE`); interactive-TTY
      passphrase for encrypted vaults, always-unlocked for scripting
- [ ] Iteration calibration + cap (deferred to Phase 4 front-end; `vault.c`
      takes an explicit count and the default is `VAULT_DEFAULT_ITERATIONS`).
      See [SECURITY.md](SECURITY.md) "KDF cost across the hardware range".
- [ ] CSPRNG for salt/nonce (Amiga front-end entropy source; core takes them
      as parameters — flagged in VAULT_FORMAT.md implementation notes)

## Phase 3 — Clock (1–2 weekends)
SNTP query, offset model, manual adjustment.

- [ ] `clock.c` offset-resolution model (SNTP → explicit offset → manual nudge)
- [ ] SNTP client over `bsdsocket` (single UDP exchange, configurable server)
- [ ] Corrected-time path used by `otp.c` (no system-clock side effects)
- [ ] Offset display / clock-status state (synced / manual / unverified)

## Phase 4 — GUI + commodity (≈2 weekends)
ClassAct GUI with account list, live codes, countdown bars, clipboard copy.
Commodity shell.

- [ ] ReAction window: `listbrowser` list, large code display, `fuelgauge` bar
- [ ] Clipboard copy (clipboard.device, FTXT)
- [ ] Clock-status indicator (green/amber/red)
- [ ] CxBroker setup, hotkey filter, popup/hide window lifecycle
- [ ] Exchange messages (show/hide/kill), auto-lock timer
- [ ] WBStartup-friendly tooltypes (`DONOTWAIT`, `CX_POPUP=NO`), passphrase flow

## Phase 5 — Release
- [ ] Docs, including the honest security note
- [ ] Aminet packaging (`aminet-release-action`)
- [ ] Demo: log into GitHub using a code from real hardware

## Success criteria
- All RFC 6238/4226 test vectors pass in CI on every commit.
- A code generated on real hardware with SNTP sync is accepted by GitHub on the
  first attempt.
- Runs from a single drawer, no installer: full GUI on OS 3.0+ and the CLI on
  anything down to a stock 68000 OS 2.x machine.

## v2 candidates (explicitly out of v1 scope)
- SHA-256/SHA-512 TOTP variants.
- Steam Guard's 5-character alphanumeric variant.
- QR code *display* for exporting accounts to a phone (scanning is out — no camera).
- ARexx port for automation, designed so **the port never carries the
  passphrase** (unlock is exclusively interactive). Command set: `GETCODE`,
  `TIMELEFT`, `LIST` (names only), `STATUS`, `LOCK`, `UNLOCK`, `SHOW`/`HIDE`/`QUIT`.
- Hand-written 68k assembler crypto primitives + optional AmiSSL provider (see
  [ARCHITECTURE.md](ARCHITECTURE.md)).
