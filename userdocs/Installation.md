# Installation

AmiAuth deliberately has **no installer**: it runs from a single drawer, and
uninstalling is deleting that drawer. Copy it where you want it and it works.

## Requirements

**CLI (`AmiAuth`):**

- Any Amiga from a stock 68000 upwards.
- AmigaOS 2.04 (V37) or later.
- No other libraries or archives — all cryptography is built in.

**GUI (`AmiAuthGUI`):**

- AmigaOS 3.0 or later, with the ReAction/ClassAct gadget classes (window,
  layout, listbrowser, fuelgauge, button). OS 3.1.4 / 3.2 / 3.5+ recommended.

**Optional, auto-detected** — the corresponding feature is simply absent if the
library is missing, nothing breaks:

| Library | Feature it enables |
|---------|--------------------|
| `bsdsocket.library` (any TCP/IP stack) | SNTP time sync — see [Time and Clock Sync](Time-and-Clock-Sync.md) |
| `commodities.library` | Hotkey popup and Exchange integration — see [Commodity and Tooltypes](Commodity-and-Tooltypes.md) |
| `datatypes.library` + a picture datatype (PNG/JPEG/…) | QR-image import in the GUI — see [Managing Accounts](Managing-Accounts.md) |

## Installing

1. Copy the drawer (or the `AmiAuth` and `AmiAuthGUI` binaries plus their icons)
   to a location of your choice, e.g. `Work:Tools/AmiAuth/`.
2. That's it. On first use AmiAuth creates its vault **next to the program** as
   `PROGDIR:AmiAuth.vault`, and stores its settings in `ENVARC:AmiAuth/`.

The drawer also contains **`AmiAuth.guide`** — this documentation in
AmigaGuide form; double-click its icon to read it offline in MultiView.
`AmiAuthGUI`'s icon ships with the commodity tooltypes already set (see
[Commodity and Tooltypes](Commodity-and-Tooltypes.md)). The `AmiAuth` CLI deliberately has no icon — it
is a Shell command.

Keeping the vault in the program's own drawer is deliberate:

- **Migrate machines** by copying the drawer — the vault travels with it.
- **Back up** the drawer and the vault is included.
- **Uninstall** by deleting the drawer and `ENVARC:AmiAuth/`; nothing else is
  left behind.

If you want the vault somewhere else (a read-only or shared install, or an
encrypted partition), see "Vault location" in [Vault and Passphrases](Vault-and-Passphrases.md).

## Starting the GUI at boot (WBStartup)

The GUI is designed to run as a background **commodity**: start it at boot,
leave it resident, and pop it up with a hotkey (default **Ctrl-Alt-A**).

The classic Amiga habit of **copying the executable into `SYS:WBStartup`
works** — with one caveat. AmiAuth's only reliance on `PROGDIR:` (the drawer a
program is *launched from*) is as the vault's default location, and the
vault's absolute path is recorded in the settings when it is created — so a
WBStartup copy finds the existing vault regardless of where it runs from.

The caveat: if the *first ever* launch is the WBStartup copy, the vault gets
created in `SYS:WBStartup/` — a poor home for your secrets (missed by
drawer backups, left behind on uninstall). So **run AmiAuth once from its own
drawer first** (creating the vault there), and remember a copied binary must
be re-copied when you upgrade.

The tidier alternatives avoid both issues:

- **Leave AmiAuthGUI in its own drawer** and place a *project icon* pointing at
  it into `SYS:WBStartup`, with the tooltypes below; **or**
- add a line to `S:User-Startup`:

      Run >NIL: Work:Tools/AmiAuth/AmiAuthGUI

Useful icon tooltypes for a WBStartup launch (full list in
[Commodity and Tooltypes](Commodity-and-Tooltypes.md)):

    CX_POPKEY=ctrl alt a
    CX_POPUP=no
    TIMESERVER=pool.ntp.org
    DONOTWAIT

`CX_POPUP=no` starts it hidden — it sits in Exchange until the hotkey summons
it. `TIMESERVER` names the SNTP server for the automatic time sync at startup
(optional — this is the default). `DONOTWAIT` tells Workbench not to wait for
it to exit (it never does).

## Checking which version you have

Both binaries carry a standard AmigaOS version string, so the Shell `Version`
command reports it:

    > Version AmiAuth
    AmiAuth 1.0 (20.07.2026)

(Also works for `AmiAuthGUI`, and matches the release tag on GitHub.)

## Next steps

Continue with [Getting Started](Getting-Started.md) to create a vault and add your first account.
