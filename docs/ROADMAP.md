# Roadmap

Phased delivery. Each phase is scoped to leave the project in a usable,
tested state.

## Phase 1 — OTP core (≈1 weekend)
SHA-1, HMAC, TOTP/HOTP core, Base32 — all validated against RFC test vectors on
the host. CLI tool generating codes from a plaintext secret.

- [x] `sha1.c` + host test (FIPS/RFC vectors)
- [x] `hmac.c` (HMAC-SHA1) + vectors
- [x] `otp.c` — HOTP (RFC 4226 App. D) and TOTP (RFC 6238 App. B) vectors pass
- [x] `base32.c` — tolerant decode + tests
- [x] Minimal CLI: generate a code from a plaintext secret (`CODE`)
- [x] Host build target + CI running vectors on every commit

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
- [x] CSPRNG for salt/nonce (Amiga front-end entropy source; core takes them
      as parameters) — HMAC-DRBG seeded from EClock jitter, volatile system
      state, and interactive keystroke timing. See docs/SECURITY.md "Randomness".

## Phase 3 — Clock (1–2 weekends)
SNTP query, offset model, manual adjustment.

- [x] `clock.c` offset-resolution model (SNTP → explicit offset → manual nudge)
- [x] Portable SNTP packet build/parse + offset computation (host-tested;
      RFC epoch conversion, request/echo round trip, kiss-o'-death rejection)
- [x] Corrected-time path used by `otp.c` (no system-clock side effects) —
      via `clock_now_utc`, already used by the CLI
- [x] Clock-status state (synced / manual / unverified)
- [x] SNTP transport over `bsdsocket` (single UDP exchange) — `src/amiga/sntp.c`;
      CLI `SYNC` command; verified on OS 3.2 under Amiberry (green, true UTC)
- [ ] Offset/status persistence (ENVARC:) + display — front-end (Phase 4)

See [CLOCK.md](CLOCK.md) for the full layered time-resolution design.

## Phase 4 — GUI + commodity (≈2 weekends)
ClassAct GUI with account list, live codes, countdown bars, clipboard copy.
Commodity shell.

Clock groundwork already landed in the CLI (see Phase 3): the **SNTP transport**
(`src/amiga/sntp.c`, `SYNC`) and the **`locale.library` amber offset** + `CLOCK`
status command are done and verified on OS 3.2. Remaining Phase 4 work:

- [x] Settings **persistence** (`ENVARC:` via GetVar/SetVar; host directory
      store) — `SYNC` saves the offset + server, `GET`/`CODE`/`CLOCK` auto-use
      them, `OFFSET` sets a manual one. Verified on OS 3.2 (UTC-correct codes)
- [x] **CSPRNG** for salt/nonce (Amiga entropy → HMAC-DRBG, incl. interactive
      keystroke timing and RAW no-echo passphrase input) — encrypted vaults now
      work on hardware. See docs/SECURITY.md "Randomness".
- [x] **PBKDF2 iteration calibration + adaptive re-key** — calibrate the count to
      ~1s on the creating machine (`--iterations` to override), and offer to
      strengthen/speed-up re-key when a vault is opened on much faster/slower
      hardware (silence via `--no-rekey` / `ENVARC:AmiAuth/rekey`). Measured: a
      stock 68000 does ~14 PBKDF2/s (`make pbkdf2-bench`). See docs/SECURITY.md.
- [ ] ReAction window: `listbrowser` list, large code display, `fuelgauge` bar
- [ ] Clipboard copy (clipboard.device, FTXT)
- [ ] Clock-status **indicator** in the GUI (green/amber/red)
- [ ] CxBroker setup, hotkey filter, popup/hide window lifecycle
- [ ] Exchange messages (show/hide/kill), auto-lock timer
- [ ] WBStartup-friendly tooltypes (`DONOTWAIT`, `CX_POPUP=NO`), passphrase flow
- [ ] (polish) parse CLI args with dos.library `ReadArgs` on Amiga

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
