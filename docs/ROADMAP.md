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
- [x] Iteration calibration + cap (landed in Phase 4; `vault.c` takes an explicit
      count, default `VAULT_DEFAULT_ITERATIONS`, calibrated per-machine by the
      front-end).
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
- [x] Offset/status persistence (ENVARC:) + display — done in Phase 4 (CLI
      `OFFSET`/`SYNC` save; GUI clock-status LED)

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
- [x] ReAction window: multi-column `listbrowser` (all accounts, live codes +
      countdown), large selected-code display, `fuelgauge` bar
- [x] Passphrase unlock + idle auto-lock; add / remove / edit accounts
- [x] Clipboard copy (iffparse FTXT, auto-clear after 30s)
- [x] Clock-status **indicator** in the GUI (green/amber/red LED)
- [x] **QR-image import** — decode an `otpauth://` QR from a PNG/JPEG/GIF/IFF via
      datatypes.library + a vendored quirc decoder (file requester + drag-and-drop)
- [x] CxBroker setup, hotkey filter, popup/hide window lifecycle
- [x] Exchange messages (show/hide/enable/disable/kill), auto-lock timer
- [x] WBStartup-friendly tooltypes (`DONOTWAIT`, `CX_POPKEY`, `CX_POPUP`)
- [x] Single-instance + **CLI forwards to the resident GUI** over a public port
      (no second passphrase prompt; the GUI stays the one vault owner)
- [x] (polish) parse CLI args with dos.library `ReadArgs` on Amiga

The commodity/hotkey/Exchange and CLI-forwarding flows still want an interactive
on-hardware verification pass (they are input-/two-process-driven, so not
headless-scriptable) — tracked as
[#37](https://github.com/sidick/amiauth/issues/37).

## Phase 5 — Release
- [x] Docs, including the honest security note (+ an AI-use disclosure)
- [x] **Comprehensive, organised user documentation before release** — live on
      the [GitHub wiki](https://github.com/sidick/amiauth/wiki): install /
      getting-started, CLI command reference, GUI guide, commodity + tooltypes,
      account management, vault + passphrases, time sync, settings reference,
      security model, troubleshooting/FAQ, building from source. `docs/` keeps
      the developer-facing design notes. The **AmigaGuide version** is
      generated from the wiki pages (`tools/wiki2guide.py`, `make guide`) —
      one source of truth — shipped in the Aminet drawer and verified
      rendering in MultiView on OS 3.2. Icons (drawer, GUI tool with preset
      tooltypes, guide project; deliberately none for the Shell-only CLI) are
      generated by `tools/mkicons.py` into `icons/` and packaged by
      `make dist`.
- [x] Aminet packaging — the `.readme` is to spec and the tag-driven release
      workflow is wired (`release.yml`: version-drift check against
      `src/version.h` + the `.readme`, m68k build, `make dist` lha packaging,
      GitHub release with auto-notes, and an approval-gated
      `aminet-release-action` FTP upload)

The remaining pre-release work is tracked in the
[v1.0 milestone](https://github.com/sidick/amiauth/milestone/1):
the on-hardware verification pass (#37), a release-workflow rehearsal (#38),
the installer decision (#39), the GitHub-login demo (#40), and the v1.0
tag/upload itself (#41).

## Success criteria
- All RFC 6238/4226 test vectors pass in CI on every commit.
- A code generated on real hardware with SNTP sync is accepted by GitHub on the
  first attempt.
- Runs from a single drawer, no installer: full GUI on OS 3.0+ and the CLI on
  anything down to a stock 68000 OS 2.x machine.

## v2 candidates (explicitly out of v1 scope)

Tracked in the [v2 milestone](https://github.com/sidick/amiauth/milestone/2),
one issue per candidate with the design notes:

- `VAULT` tooltype/argument for the GUI (#42)
- SHA-256/SHA-512 TOTP variants (#43)
- Steam Guard's 5-character alphanumeric variant (#44)
- QR code *display* for exporting accounts to a phone (#45)
- ARexx port for automation — the port never carries the passphrase (#46)
- 68k assembler crypto hot loops + optional AmiSSL provider (#47)
- Translations via locale.library catalogs + broader locale-aware date/time/
  decimal formatting, beyond the existing GMT-offset use (#67)
