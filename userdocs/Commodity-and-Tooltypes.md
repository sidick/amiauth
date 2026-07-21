# Commodity and Tooltypes

`AmiAuthGUI` is a proper AmigaOS **Commodity**: it runs resident in the
background, appears in **Exchange**, pops up on a **hotkey**, and is
**single-instance**. This is the recommended way to run it — you unlock the
vault once, and it stays available all session, to both the hotkey and the
Shell CLI.

## Behaviour as a commodity

- **Hotkey** (default **Ctrl-Alt-A**): if the window is hidden, it opens; if
  it's already open, it comes to the front and activates.
- **Closing the window hides it** — the process stays resident and the vault
  stays unlocked. Bring it back with the hotkey, Exchange's *Show*, or
  `AmiAuth SHOW` from a Shell. It reopens at the same position and size you
  last left it (this session — not saved across a full restart).
- **Quitting for real** is *Project → Quit* (or Exchange's *Kill*, or
  removing it in Exchange). Key material is zeroed on the way out.
- **Single instance:** launching `AmiAuthGUI` a second time doesn't start a
  second copy or re-prompt for the passphrase — it just tells the running
  instance to show its window.
- **Exchange** lists it as "AmiAuth" ("TOTP/HOTP authenticator") and all the
  standard messages work: Show / Hide / Enable / Disable (the hotkey) / Kill.
- **CLI forwarding:** while resident, the Shell `AmiAuth` command sends `GET`,
  `LIST`, `ADD`, `REMOVE` and `SHOW` to the running GUI instead of opening the
  vault itself — no second passphrase prompt. See [CLI Reference](CLI-Reference.md).

Without `commodities.library` (a bare OS install), AmiAuthGUI degrades to a
plain application window: no hotkey or Exchange entry, and closing the window
quits. It's still single-instance — a second launch exits immediately without
prompting for a passphrase — but unlike the commodity case above, it does
**not** bring the running instance's window to the front; you'll need to
switch to it yourself.

## Tooltypes

Set these in the icon's **Information** window (Workbench: select the icon,
then *Icons → Information*). The `CX_` tooltypes are the standard commodity
set.

| Tooltype | Default | Meaning |
|----------|---------|---------|
| `CX_POPKEY` | `ctrl alt a` | The hotkey that shows/raises the window. Standard commodities hotkey syntax: qualifiers (`ctrl`, `alt`, `shift`, `lcommand`, …) followed by a key, e.g. `CX_POPKEY=lcommand a`. |
| `CX_POPUP` | `yes` | `no` = start hidden: the commodity loads silently and the window first opens on the hotkey (or Exchange *Show*). The right choice for WBStartup. |
| `CX_PRIORITY` | `0` | Commodity broker priority — the order commodities see input. Rarely needs changing. |
| `TIMESERVER` | *(saved `server` pref, else `pool.ntp.org`)* | SNTP server for the automatic time sync the GUI performs at startup. On success the measured offset (and this server) are saved; offline the sync fails quietly. See [Time and Clock Sync](Time-and-Clock-Sync.md). |
| `PUBSCREEN` | *(the default public screen)* | Open on the named public screen instead — e.g. a custom screen another program opened. Falls back to the default public screen automatically if the named one isn't open when AmiAuth starts. |
| `VAULT` | *(see [Vault and Passphrases](Vault-and-Passphrases.md))* | Use this vault file for this launch, e.g. `VAULT=Work:Secrets/Test.vault`. Same precedence as the CLI's `VAULT` keyword: it beats `AMIAUTH_VAULT` and the saved path, and — like those overrides — is never recorded as the sticky vault location. |
| `DONOTWAIT` | — | Not read by AmiAuth itself: tells **Workbench** not to wait for the program to exit. Set it on any WBStartup icon. |

The `CX_*` names, and `TIMESERVER`/`PUBSCREEN`/`VAULT`, also work as Shell
arguments when starting the GUI from a script
(`Run >NIL: AmiAuthGUI CX_POPUP=no`).

## A WBStartup icon that works

Any of these give you AmiAuth at boot (details in [Installation](Installation.md)):

- a **project icon** in WBStartup pointing at `AmiAuthGUI` in its drawer (the
  tidiest shape);
- a `Run >NIL: …/AmiAuthGUI` line in `S:User-Startup`;
- the classic **copy of the executable (+ icon) into WBStartup** — fine too,
  as long as the vault already exists (its absolute path is recorded in the
  settings at creation, so a WBStartup copy finds it; just don't let a
  first-ever launch create the vault *in* WBStartup).

Suggested tooltypes:

    CX_POPKEY=ctrl alt a
    CX_POPUP=no
    DONOTWAIT

With `CX_POPUP=no` the boot is **silent** in both modes. An always-unlocked
vault opens immediately, so Ctrl-Alt-A summons your codes instantly. An
encrypted vault stays resident but **locked**: the passphrase prompt appears
the first time you summon the window (hotkey, Exchange *Show*, or
`AmiAuth SHOW`) — once per session, at a moment of your choosing rather than
mid-boot. Cancelling the prompt just re-hides it, still locked; until
unlocked, forwarded CLI commands report that the vault is locked. (A first
launch with no vault at all defers its welcome/creation dialog the same way.)

## Choosing a hotkey

`ctrl alt a` is chosen to be unlikely to clash. If it does clash (some
terminal programs and other commodities grab similar combinations), pick
another with `CX_POPKEY` — Exchange will show AmiAuth's current hotkey in its
information view. If two commodities claim the same hotkey, the one with the
higher `CX_PRIORITY` gets it.
