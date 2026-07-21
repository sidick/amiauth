# CLI Reference

The `AmiAuth` Shell command works on any Amiga from a stock 68000 with AmigaOS
2.04 up, with no dependencies. On AmigaOS it uses standard `ReadArgs` parsing —
type `AmiAuth ?` for the template, `AmiAuth HELP` for the command list.

## Command summary

| Command | Purpose | Opens the vault? |
|---------|---------|------------------|
| [`CODE`](#code) | One-shot code from a bare secret | No |
| [`INIT`](#init) | Create a new vault | Creates it |
| [`ADD`](#add) | Import an account (`otpauth://` URI or bare secret) | Yes |
| [`LIST`](#list) | List account names | Yes |
| [`GET`](#get) | Print the current code for an account | Yes |
| [`REMOVE`](#remove) | Delete an account | Yes |
| [`SHOW`](#show) | Pop the running GUI's window to the front | No |
| [`CLOCK`](#clock) | Report the UTC offset and clock trust state | No |
| [`SYNC`](#sync) | Measure the offset via SNTP and save it | No |
| [`OFFSET`](#offset) | Set a manual UTC offset and save it | No |
| [`NUDGE`](#nudge) | Adjust the current UTC offset by a relative amount | No |
| [`HELP`](#help) | Print the usage summary | No |

When the GUI is running as a resident commodity, `ADD`, `LIST`, `GET`,
`REMOVE` and `SHOW` are **forwarded to it** automatically — see
[Working with a running GUI](#working-with-a-running-gui) below.

## The ReadArgs template

    AmiAuth ?
    COMMAND,VALUE,DIGITS,PERIOD,ISSUER/K,LABEL/K,VAULT/K,OPEN/S,ITERATIONS/N/K,NOREKEY/S

- `COMMAND` — one of the commands above (case-insensitive).
- `VALUE` — the command's main argument (secret, URI, account name, seconds or
  server, depending on the command).
- `DIGITS`, `PERIOD` — optional positionals for `CODE`.
- `ISSUER <name>`, `LABEL <account>` — keywords for `ADD` with a bare
  secret (both required in that form; unused with a URI).
- `VAULT <path>` — use this vault file instead of the configured one.
- `OPEN` — switch for `INIT`: create an always-unlocked vault, no prompts.
- `ITERATIONS <n>` — keyword for `INIT`: explicit PBKDF2 iteration count.
- `NOREKEY` — switch: suppress the adaptive re-key offer for this run.

The host (development) build takes the equivalent Unix-style options:
`--vault PATH`, `--open`, `--iterations N`, `--no-rekey`, `--issuer S`,
`--label S`.

## Commands

### CODE

    AmiAuth CODE <base32-secret> [digits] [period]

Generates a TOTP code directly from a Base32 secret — no vault involved.
Useful for a quick test or a secret you keep elsewhere. Digits default to 6,
period to 30 seconds. The code is printed to standard output; the seconds
remaining in the current period go to standard error, so a script can capture
the code alone.

Base32 decoding is deliberately tolerant: whitespace, case and `=` padding in
the pasted secret don't matter.

    > AmiAuth CODE JBSWY3DPEHPK3PXP
    282760
    (18 seconds remaining)

### INIT

    AmiAuth INIT [OPEN] [ITERATIONS <n>] [VAULT <path>]

Creates a new vault. Fails if one already exists at the resolved path.

Without `OPEN` it prompts (input hidden, never echoed):

    New passphrase (empty for an always-unlocked vault):
    Confirm passphrase:

- A non-empty passphrase creates an **encrypted** vault. The PBKDF2 iteration
  count is calibrated to take about one second on this machine (override with
  `ITERATIONS`); the chosen count is reported. See [Security Model](Security-Model.md).
- An empty passphrase — or the `OPEN` switch, which skips the prompts entirely
  for scripting — creates an **always-unlocked** vault with no at-rest
  protection.

On success the vault's absolute path is recorded in `ENVARC:AmiAuth/vault`, so
later runs (and the GUI, even from WBStartup) find the same vault — unless the
path was explicitly overridden with `VAULT` or `AMIAUTH_VAULT`, in which case
nothing is recorded (scratch vaults stay scratch). If `PROGDIR:` is not
writable (read-only medium, protected install), `INIT` reports it and points
you at the `VAULT` keyword.

### ADD

    AmiAuth ADD "otpauth://totp/GitHub:you@example.com?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"
    AmiAuth ADD JBSWY3DPEHPK3PXP ISSUER GitHub LABEL you@example.com

Imports an account, in either of two forms:

- **`otpauth://` URI** (the format behind enrolment QR codes; most services
  show it under a "can't scan the code?" link). Issuer, label, secret,
  algorithm, digits, period/counter are all taken from the URI. **Quote the
  URI** — it contains `?` and `&`.
- **Bare Base32 secret** — the raw "setup key" some services show instead.
  A bare secret carries no name, so `ISSUER` and `LABEL` are required, and
  the account gets the defaults nearly every service issues: TOTP, SHA-1,
  6 digits, 30 seconds. Anything else (HOTP, 8 digits, a custom period)
  still needs the URI form.

Prints `Added issuer:label`. The vault holds up to 64 accounts. See
[Managing Accounts](Managing-Accounts.md) for URI details.

### LIST

    AmiAuth LIST

Prints each account, one per line, as `issuer:label` (or just the label if no
issuer is set):

    GitHub:you@example.com
    Fastmail:you

### GET

    AmiAuth GET <account>

Prints the current code for one account, plus the seconds remaining (on
standard error). The account name is matched **case-insensitively** against
the label, the issuer, or the combined `issuer:label` — so
`AmiAuth GET github` finds `GitHub:you@example.com`.

For **HOTP** accounts, `GET` uses the stored counter, then increments it and
saves the vault, as RFC 4226 requires — each `GET` produces the next code in
the sequence.

### REMOVE

    AmiAuth REMOVE <account>

Deletes an account (same name matching as `GET`) and saves the vault. There is
no undo — the secret is gone from the vault; keep backups
([Vault and Passphrases](Vault-and-Passphrases.md)).

### SHOW

    AmiAuth SHOW

Asks a resident GUI to bring its window to the front (the Shell equivalent of
the hotkey). Reports `no running GUI to show` if none is running.

### CLOCK

    AmiAuth CLOCK

Reports the active UTC offset, the trust state, and the corrected UTC time:

    UTC offset : +3600 seconds (+60 min)
    status     : offset applied (amber)
    corrected  : 2026-07-19 14:30:45 UTC

States: `synced (green)` — just verified by `SYNC`; `offset applied (amber)` —
a stored or locale-derived offset is in use; `unverified (red)` — nothing is
correcting the clock. See [Time and Clock Sync](Time-and-Clock-Sync.md).

### SYNC

    AmiAuth SYNC [server]

Measures your clock's offset from true UTC with a single SNTP exchange and
**saves it** (`ENVARC:AmiAuth/offset`), so every later `GET`/`CODE` — and the
GUI — uses corrected time. Needs a running TCP/IP stack
(`bsdsocket.library`); your system clock is never modified.

The server defaults to the last one used, then to `pool.ntp.org`; a server you
name is remembered for next time.

### OFFSET

    AmiAuth OFFSET <seconds>

Sets and saves a manual UTC offset (added to the system clock), for machines
without a network. Positive or negative, e.g. `OFFSET -3600` if your clock
runs one hour ahead of UTC. **Replaces** whatever offset was previously
stored. See [Time and Clock Sync](Time-and-Clock-Sync.md) for choosing the value.

### NUDGE

    AmiAuth NUDGE <+/-seconds>

Adjusts and saves the **current** offset by a relative amount — unlike
`OFFSET`, which sets an absolute value, `NUDGE` adds to (or subtracts from)
whatever's already stored. Handy for dialling an offline clock in a step at a
time while checking a code against a known-good source, without having to
work out the new absolute offset yourself:

    > AmiAuth OFFSET 7200
    UTC offset : +7200 seconds (+120 min)
    ...
    > AmiAuth NUDGE 30
    UTC offset : +7230 seconds (+120 min)
    ...

See [Time and Clock Sync](Time-and-Clock-Sync.md) for a full worked example, and the GUI's
`-10s`/`+10s` buttons for the same thing without the Shell.

### HELP

    AmiAuth HELP

Prints the command list and usage. `AmiAuth ?` shows the ReadArgs template.

## Vault selection

The vault path is resolved in this order (first match wins):

1. The `VAULT <path>` keyword on the command line.
2. The `AMIAUTH_VAULT` environment variable.
3. The path saved in `ENVARC:AmiAuth/vault` (recorded at `INIT`).
4. The default: `PROGDIR:AmiAuth.vault`.

## Passphrases and scripting

An encrypted vault prompts `Passphrase:` (hidden input) whenever it must be
opened. The passphrase can **only** be entered interactively — there is
deliberately no command-line, environment or file mechanism, so it can never
leak into a script, Shell history or process list. Two consequences:

- Run from an interactive Shell; without one, the CLI refuses with a message
  rather than hanging.
- For unattended scripting, use an **always-unlocked vault**
  (`INIT OPEN`) — or run the GUI resident and let the CLI forward to it.

While an encrypted vault opens, AmiAuth may offer to **re-key** it if this
machine is far faster or slower than the one that created it (~8× either way).
Strengthening is one `y`; weakening warns and requires typing `yes`. Suppress
the offer with the `NOREKEY` switch, or permanently with
`SetEnv SAVE AmiAuth/rekey off`. Details in [Vault and Passphrases](Vault-and-Passphrases.md).

## Working with a running GUI

On AmigaOS, if `AmiAuthGUI` is resident (see [Commodity and Tooltypes](Commodity-and-Tooltypes.md)), the
CLI does not open the vault itself: `ADD`, `LIST`, `GET`, `REMOVE` and `SHOW`
are forwarded to the GUI over its public `AmiAuth` message port, and the GUI
answers from its already-unlocked vault. You get your code with **no second
passphrase prompt**, and there is never a conflict between two processes
writing one vault file.

If the GUI's vault is currently locked (auto-lock, or not yet unlocked), the
CLI reports: `the running GUI's vault is locked; unlock it there first`.
The passphrase itself never crosses the port — see [Security Model](Security-Model.md).

`CODE`, `INIT`, `CLOCK`, `SYNC` and `OFFSET` never involve the vault contents
and always run locally.

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Usage error (unknown command, missing argument) |
| 2 | Runtime error (vault I/O, wrong passphrase, account not found, sync failure…) |
| 20 | AmigaOS: `ReadArgs` could not parse the arguments |

Wrong passphrase and a tampered/corrupted vault file are deliberately
indistinguishable (both report
`wrong passphrase or the file has been tampered with`) — the file's integrity
check covers everything.
