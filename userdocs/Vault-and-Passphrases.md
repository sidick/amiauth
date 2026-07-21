# Vault and Passphrases

All accounts live in a single **vault file**. This page covers where it lives,
encrypted vs always-unlocked mode, passphrases and re-keying, and backups.
The security reasoning behind all of it is in [Security Model](Security-Model.md); the on-disk
format is frozen and documented in
[`docs/VAULT_FORMAT.md`](https://github.com/sidick/amiauth/blob/main/docs/VAULT_FORMAT.md).

## Where the vault lives

By default: **`PROGDIR:AmiAuth.vault`** — next to the program, in its own
drawer. That is deliberate: copy the drawer and you've migrated or backed up
everything; delete it and AmiAuth is gone. Settings live separately in
`ENVARC:AmiAuth/` (see [Settings Reference](Settings-Reference.md)).

At vault creation (GUI first launch or CLI `INIT`) the resolved path is
recorded as an **absolute** path in `ENVARC:AmiAuth/vault`, so the vault stays
found even if the binary later runs from somewhere else. Creation with an
explicitly overridden path (`VAULT` keyword / `AMIAUTH_VAULT`) deliberately
does not touch the recorded pref. To use a different location — a shared or read-only
install, a separately-encrypted partition — the precedence is:

1. `VAULT <path>` on the CLI command line, or the GUI's `VAULT` icon
   tooltype / Shell argument (this run only);
2. the `AMIAUTH_VAULT` environment variable;
3. the saved `ENVARC:AmiAuth/vault` path;
4. the `PROGDIR:AmiAuth.vault` default.

To move an existing vault: copy the file, then update the saved path
(`SetEnv SAVE AmiAuth/vault <newpath>`).

## Encrypted vaults

`AmiAuth INIT` with a non-empty passphrase creates an encrypted vault:
accounts are encrypted at rest (ChaCha20, keyed from your passphrase via
PBKDF2, with the whole file tamper-protected by a MAC). Every save is atomic —
a crash mid-save can never destroy the existing vault.

What to expect day-to-day:

- Anything that opens the vault prompts `Passphrase:` (input hidden). With
  the GUI resident, that's **once per session** — the CLI forwards to it.
- The passphrase is only ever entered interactively — never on a command line,
  in a script, or over the commodity port.
- Unlocking takes about a second *on the machine class that created the
  vault* — that is calibrated, not accidental (see below).
- A wrong passphrase and a corrupted/tampered file are deliberately
  indistinguishable: both report
  `wrong passphrase or the file has been tampered with`.

**There is no passphrase recovery.** The passphrase *is* the key; without it
the vault contents are gone. Keep your services' recovery codes somewhere
safe, and choose a passphrase you won't lose — a long, memorable one beats a
short, complex one (see [Security Model](Security-Model.md) for why the passphrase, not the
KDF, is what really protects you on slow hardware).

## Always-unlocked vaults

Press Enter at `INIT`'s passphrase prompt (or use `INIT OPEN` for scripts) and
the vault is stored **unencrypted** — same file format, no prompts anywhere,
ever. This is a deliberate opt-out for single-user machines, dedicated
emulator instances, and unattended scripting.

Be clear-eyed about it: **anyone who gets the file has your secrets.** The
file still carries an integrity check (corruption is detected), but no
confidentiality. See [Security Model](Security-Model.md).

### Converting between modes

The format is identical either way, so conversion is just a re-save —
supported through vault settings (add, change or remove the passphrase); the
vault is re-encrypted or decrypted on disk accordingly.

## Adaptive re-keying (the "faster/slower machine" prompt)

Amiga hardware spans a ~1000× speed range, so the vault's PBKDF2 work factor
is calibrated to *the machine that created it* and adapted as it travels.
When you unlock a vault on a machine at least ~8× faster or slower than the
one that keyed it, AmiAuth offers to re-key:

- **"Strengthen"** (faster machine — e.g. vault created on an A500, opened on
  a 68060 or an emulator): accepts in one keypress, re-keys to a count
  calibrated for the new machine. **Say yes** — it only makes the vault
  stronger, and costs one save.
- **"Re-key lower"** (slower machine — e.g. vault created on an emulator,
  opened on a stock A500 where unlocking now takes ages): this *weakens*
  at-rest protection, so it warns and requires typing `yes` to confirm.
  Legitimate when that slow machine is where the vault will live.

Not interested? `NOREKEY` on a CLI command suppresses the offer for that run;
`SetEnv SAVE AmiAuth/rekey off` (or "Never here" in the GUI dialog) silences
it permanently on that machine.

## Backups and migration

The vault is one ordinary file — treat it like one:

- **Back up** by copying `AmiAuth.vault` (or the whole drawer) anywhere: disk,
  network, another machine. An encrypted vault is safe to store on untrusted
  media *to the extent your passphrase is strong*; an always-unlocked vault is
  plaintext — treat it like a written-down password list.
- **Migrate machines** by copying the drawer. The vault format is
  byte-identical across all Amigas and the host build — nothing to convert.
  Expect a re-key offer if the new machine is a different hardware class.
- **Both front-ends share the one vault** — CLI and GUI read the same file
  (and coordinate through the resident GUI when it's running), so there is
  nothing to sync.
