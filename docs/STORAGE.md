# File locations

Where AmiAuth keeps its vault and its settings on AmigaOS, and why. This is a
design note for the Amiga front-end; the portable core is deliberately unaware of
it (see "Core stays path-agnostic" below).

## Summary

| Data | Location | Notes |
|------|----------|-------|
| Vault (secrets) | `PROGDIR:AmiAuth.vault` (default) | user data; path is itself a setting and overridable |
| Settings / prefs | `ENVARC:AmiAuth/` | persistent preferences, incl. the resolved vault path |
| — never — | `ENV:` | RAM-backed; evaporates on reboot — unsafe for the vault |

## Vault: `PROGDIR:` by default

The vault defaults to **`PROGDIR:AmiAuth.vault`** — the drawer the executable
runs from. This completes the proposal's "runs from a single drawer, no
installer" promise:

- **Migrate machines** by copying the drawer; the vault comes with it.
- **Back up** the drawer and the vault is included (AmiSnap would capture it
  automatically).
- **Uninstall** by deleting the drawer; nothing is left behind elsewhere.

It is also where Amiga users instinctively look for an application's own data —
more intuitive than `S:`, and semantically correct where `ENVARC:` is not: a
vault is *user data*, not a preference. `ENV:` specifically would be actively
dangerous, being RAM-backed and lost on reboot.

## Settings: `ENVARC:AmiAuth/`

Preferences live in `ENVARC:AmiAuth/` where prefs belong. The **resolved vault
path is one of those settings**, stored as an absolute path (see first-run
below). Auto-lock timeout, hotkey, default digits/period, etc. also live here.

## Overriding the vault path

A `VAULT=` override covers the cases where `PROGDIR:` is wrong — a read-only or
shared network install, or a vault kept on a separately-encrypted partition.
Resolution precedence, highest first:

1. Explicit `VAULT=` **Shell argument** or **icon tooltype** for this launch.
2. The **absolute path recorded in prefs** (from first run).
3. The default **`PROGDIR:AmiAuth.vault`**.

At **first run** (vault creation), the resolved path is written into the prefs as
an **absolute** path.

## The `PROGDIR:`/WBStartup gotcha

`PROGDIR:` is the drawer the program is *launched from*. If a user copies the
executable into `SYS:WBStartup` to get the commodity at boot, `PROGDIR:` silently
becomes `WBStartup`, and a naive `PROGDIR:`-relative vault would appear to vanish.

Two mitigations, both adopted:

- **Record the absolute path at first run** (above), so the vault location is
  sticky regardless of where the binary later runs from.
- **Recommend the right install shape in the docs:** leave AmiAuth in its own
  drawer and put a *project icon* (or a `Run` line in `user-startup`) into
  WBStartup — do **not** copy the executable itself into WBStartup.

## Unwritable `PROGDIR:`

If `PROGDIR:` is not writable at vault creation (read-only medium, protected
install), fail gracefully:

- **GUI:** open an ASL file requester to choose a writable location, then record
  that absolute path in prefs.
- **CLI:** report the condition and point at `VAULT=` rather than dying opaquely.

## Core stays path-agnostic

`vault.c` (portable core) takes an explicit `path` in `vault_load`/`vault_save`
and knows nothing about `PROGDIR:`, `ENVARC:`, tooltypes or requesters. All of
the resolution above lives in the **Amiga front-end**; the CLI and the host test
build simply pass a path. This keeps the vault code host-testable and portable,
consistent with docs/ARCHITECTURE.md.

Touches, when implemented: `vault.c` (path parameter — Phase 2), the prefs format
and commodity/tooltype handling (Phase 4), and the install section of the docs
(Phase 5).
