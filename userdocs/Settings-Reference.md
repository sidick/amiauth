# Settings Reference

AmiAuth's preferences are plain AmigaOS environment variables in
**`ENVARC:AmiAuth/`** (persistent, copied to `ENV:` at boot). AmiAuth writes
them itself as you use it — there is no separate prefs program — but they are
ordinary variables you can inspect and change from a Shell:

    GetEnv AmiAuth/offset
    SetEnv SAVE AmiAuth/rekey off

(`SETENV SAVE` writes both `ENV:` and `ENVARC:`; plain `SetEnv` only lasts
until reboot.)

The vault itself is **not** here — it is user data, not a preference, and
lives in `PROGDIR:` by default (see [Vault and Passphrases](Vault-and-Passphrases.md)). `ENV:` is
RAM-backed and would be lost on reboot.

## Variables

| Variable | Type | Written by | Meaning |
|----------|------|------------|---------|
| `AmiAuth/vault` | path | vault creation (GUI first launch or CLI `INIT`), unless the path was explicitly overridden | Absolute path of the vault. Read by both CLI and GUI whenever no explicit override is given. Edit it (or delete it) if you move the vault. |
| `AmiAuth/offset` | signed seconds | `SYNC`, `OFFSET`, the GUI's startup sync | The UTC correction added to the system clock when generating codes. Loaded at startup by both front-ends and reported as amber ("offset applied"/"manual") until re-verified by a fresh sync. |
| `AmiAuth/server` | hostname | `SYNC`, the GUI's startup sync | The last SNTP server used; becomes the default for the next sync (CLI and GUI). Defaults to `pool.ntp.org` when unset; the GUI's `TIMESERVER` tooltype overrides it per launch. |
| `AmiAuth/rekey` | `off` | you, or "Never here"/`ne(v)er` in the re-key prompt | When set to `off`, suppresses the adaptive re-key offers on this machine entirely. Delete the variable to get them back. |
| `AmiAuth/idlelock` | seconds | you | GUI idle auto-lock timeout for encrypted vaults. Default when unset: **120**. `0` disables auto-lock. Only an *open* window ticks the idle timer. |
| `AmiAuth/cryptoasm` | `off` | you | AmiAuth uses hand-written 68000 assembly for its SHA-1 inner loop by default (safe on every supported CPU, including a plain 68000 — it's not an 020+ fast path). Set to `off` to force the portable C implementation instead, e.g. if you suspect the assembly on your particular setup. Delete the variable to get the assembly back. |

Additionally the **`AMIAUTH_VAULT`** environment variable (a conventional
variable read via `getenv`, so `ENV:AMIAUTH_VAULT` on AmigaOS) overrides the
saved vault path — handy for temporarily pointing everything at a test vault.
An explicit `VAULT` — the CLI keyword or the GUI tooltype/argument — beats
both. Precedence details in [Vault and Passphrases](Vault-and-Passphrases.md).

## Resetting AmiAuth

To return to a first-run state without touching your accounts: delete the
variables (`Delete ENVARC:AmiAuth/#? ENV:AmiAuth/#?`). To remove AmiAuth
completely: delete its drawer (which contains the vault — this is the part
that destroys your secrets) and `ENVARC:AmiAuth/`.
