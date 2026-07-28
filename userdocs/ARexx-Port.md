# ARexx Port

`AmiAuthGUI`, while resident, opens a public ARexx port so any ARexx script
(or other program that speaks the RexxMsg protocol) can drive it — get a
code, check status, lock/unlock, show/hide, quit — the same way Exchange or
the hotkey do. This is separate from the CLI's own forwarding (see
[CLI Reference](CLI-Reference.md)); it's for scripts, not the `AmiAuth`
Shell command.

**The passphrase never crosses this port.** `UNLOCK` is always interactive —
it opens the same GUI requester as the window/hotkey/commodity paths. A
script can ask AmiAuth to unlock, but it can't hand it a passphrase to do so
unattended. See [Security Model](Security-Model.md).

## Port name

The port is named `AMIAUTH.<n>` (uppercase), where `<n>` is the lowest free
slot — normally `AMIAUTH.1`. It's shown in the main window's title bar once
open, e.g. `AmiAuth 1.0 (abc1234) [AMIAUTH.1]`. Override it with the
`PORTNAME` tooltype/argument, the same way as `VAULT`/`PUBSCREEN` (see
[Commodity and Tooltypes](Commodity-and-Tooltypes.md)):

    Run >NIL: AmiAuthGUI PORTNAME=MYAUTH.1

If `rexxsyslib.library` isn't available, or every slot up to `AMIAUTH.99` is
already taken, AmiAuth simply runs without an ARexx port — not a fatal error.

## Commands

Address the port (`ADDRESS AMIAUTH.1`), then issue commands as quoted host
command strings — quoting keeps ARexx from evaluating a bare word like
`GETCODE GitHub` as a REXX expression (string concatenation, which
upper-cases and can mangle it) rather than sending it verbatim. Put
`OPTIONS RESULTS` near the top of the script: without it, ARexx never asks
the host for a `RESULT` string at all, so `RESULT` stays undefined even on
success. Every command still sets `RC` either way.

`RC` follows the standard ARexx convention: **0** success, **5** the user
cancelled an interactive prompt, **10** a bad or missing argument, **20** the
command couldn't be carried out (locked, gated off, or not registered as a
commodity).

| Command | Argument | RESULT | Notes |
|---------|----------|--------|-------|
| `GETCODE` | account name (required) | the current code, with a trailing newline | RC 20 if the vault is locked *or* if [`arexxgetcode`](Settings-Reference.md) is off — deliberately the same RC either way, so a script can't tell which. RC 10 if no account matches. For an HOTP account, RC 20 if persisting the advanced counter fails (the code itself was still generated correctly — see [Vault and Passphrases](Vault-and-Passphrases.md) on why a failed save is reported rather than silently accepted). |
| `TIMELEFT` | account name (required) | seconds remaining; `-1` for HOTP (no time concept — not an error); a Steam Guard account returns seconds like TOTP | Same locked/gated/not-found RCs as `GETCODE`. |
| `LIST` | — | one `issuer:label` (or just `label`) per line, each newline-terminated | RC 20 if locked. |
| `STATUS` | — | `"<mode> <count>"`, e.g. `unlocked 7`, `locked 0`, `always-unlocked 7` | Always answers, even locked — this is how a script decides whether to `UNLOCK` at all. |
| `LOCK` | — | `locked`, or `always-unlocked` | A no-op (RC 0) for an always-unlocked vault. |
| `UNLOCK` | — | `unlocked`, `already-unlocked`, `always-unlocked`, or `cancelled` | **Interactive** — opens the same passphrase requester as the GUI. RC 5 if the user cancels it. |
| `SHOW` | — | — | Only meaningful while running as a registered commodity (RC 20 otherwise) — mirrors the window's own close-gadget behaviour, which hides rather than quits only when a commodity broker is present. RC 5 if the vault is locked and the user cancels the unlock requester. |
| `HIDE` | — | — | Same commodity-only restriction as `SHOW`. |
| `QUIT` | `FORCE` (optional switch) | — | Quits the resident instance. `FORCE` is accepted (the reserved ARexx convention) but is a documented no-op: every vault change already saves immediately, so there's never an unsaved-changes prompt to suppress. |

Unknown commands, or a required argument left out (e.g. `GETCODE` with no
account), return RC 10.

## The `arexxgetcode` setting

`ENVARC:AmiAuth/arexxgetcode` (default: allowed; set to `off` to disable)
restricts the port to control commands — `STATUS`, `LOCK`, `UNLOCK`, `SHOW`,
`HIDE`, `QUIT` — and refuses `GETCODE`/`TIMELEFT` (RC 20, same as a locked
vault). See [Settings Reference](Settings-Reference.md).

    SetEnv SAVE AmiAuth/arexxgetcode off

## Example

```
/* getcode.rexx — print a TOTP code via AmiAuth's ARexx port */
OPTIONS RESULTS
ADDRESS AMIAUTH.1
'GETCODE GitHub'
IF RC = 0 THEN SAY RESULT
ELSE SAY 'AmiAuth: could not get a code (RC' RC')'
```

```
/* status.rexx — unlock AmiAuth if it's locked, then list accounts */
OPTIONS RESULTS
ADDRESS AMIAUTH.1
'STATUS'
PARSE VAR RESULT mode count
IF mode = 'locked' THEN DO
    'UNLOCK'
    IF RC = 5 THEN EXIT   /* user cancelled the prompt */
END
'LIST'
SAY RESULT
```

Quoted account names with spaces work as you'd expect:
`'GETCODE "My Account"'`.
