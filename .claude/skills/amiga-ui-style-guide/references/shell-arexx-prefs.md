# Shell, ARexx, Data Sharing, and Preferences

From Style Guide ch. 8–9, 11–12. A native-feeling Amiga app is drivable
from the Shell and ARexx, shares data via IFF/clipboard, and handles
settings the standard way.

## Basename (ch. 2, foundational)

A short one-word name, ideally the executable name, from which ALL
user-accessible names derive: public screens (`MBASE.1`), ARexx ports
(`MBASE.1`), settings files (`MBase.prefs`). Logical, memorable, short —
but not so short it collides with other apps.

## Shell interface

### Command form and parsing

- `COMMAND [redirection] [<arg1>, <arg2>, ...]` — parse with the standard
  **command template** method via DOS `ReadArgs()`; the same style applies
  to your ARexx commands. Shorter code, automatic errors/help, consistent
  Shell.
- Typing `command ?` must display the template as syntax help.

### Template modifiers

| Modifier | Meaning |
|---|---|
| `,` | separates arguments (null argument) |
| `=` | keyword abbreviation equivalents (`PS=PUBSCREEN/K`) |
| `/A` | always required |
| `/F` | rest of line is one final argument |
| `/K` | keyword must be typed for the argument to count |
| `/M` | multiple values (one per template; leftovers attach to it; lends values to unfilled /A args) |
| `/N` | decimal number (otherwise strings) |
| `/S` | switch: present = on |
| `/T` | toggle: present = flip current state |

Keywords and modifiers are case-insensitive.

### Standard argument names — use these, with these meanings

| Argument | Purpose |
|---|---|
| `FILES/M` | Files to open as projects |
| `PUBSCREEN/K` | Public screen to open on |
| `PORTNAME/K` | ARexx port name override |
| `STARTUP/K` | ARexx script to run at startup |
| `NOGUI/S` | Run without opening a GUI |
| `SETTINGS/K` | Preferences file to load at startup |

New keywords: reuse an existing one with the same meaning if any fits; never
name one after a common command (`list`).

### Embedded version ID

Embed in the executable (and ARexx scripts and config files):

```
$VER: <name> <version>.<revision> (<d>.<m>.<y>)
```

so the Shell `VERSION` command reports your software's version.

## ARexx interface

- If you support scripts/macros at all, do it via ARexx; even without
  macros, add an ARexx port. Expose (at least) the command set available
  through your menus and action gadgets.
- Command style mirrors AmigaDOS: `COMMAND [<args>]`, standard template
  parsing, AmigaDOS pattern matching (`#?`) where sensible,
  case-insensitive, no spaces in keywords, quoted arguments accepted,
  verbose keywords (`OPEN`, not `O`) with abbreviations allowed for common
  ones.

### Return codes (in ARexx's RC variable)

| Code | Meaning |
|---|---|
| 0 | success |
| 5 | warning (e.g. aborted by user) |
| 10 | error (e.g. wrong file type) |
| 20 | failure (e.g. couldn't open clipboard) |

Extended code sets are fine, but 0 = success and warnings stay below 10.

### Returning data

- Results go in the RESULT variable (script must issue OPTIONS RESULTS);
  support `VAR` and `STEM` switches so the user can redirect the value into
  a named simple or stem variable.
- Multi-record strings: quote records containing spaces, separate with
  spaces. With a stem: `List.count` holds the record count, `List.0`,
  `List.1`, ... the unquoted records.

### Port naming

- `<BASENAME>.<slot#>`, upper-case, unique per project and per instance:
  first document `GWORD.1`, second `GWORD.2`, a concurrently launched copy
  continues `GWORD.3`.
- Show the port name in the About... window; let the user override it via
  `PORTNAME` (Tool Types or CLI).

### Standard ARexx command set (minimum 15 — reserved names)

Project: `NEW PORTNAME/K` (returns new port name) · `CLEAR FORCE/S` ·
`OPEN FILENAME/K,FORCE/S` (no name → file requester) · `SAVE` (unnamed →
file requester) · `SAVEAS NAME/K` · `CLOSE FORCE/S` · `PRINT PROMPT/S` ·
`QUIT FORCE/S`.

Block: `CUT` · `COPY` · `PASTE` · `ERASE FORCE/S`.

Other: `HELP COMMAND,PROMPT/S` (text list of all commands from non-GUI
contexts; PROMPT opens graphical help) · `FAULT /N` (error text for a code,
via RESULT) · `RX CONSOLE/S,ASYNC/S,COMMAND/F` (run an ARexx macro).

`FORCE/S` always means "suppress the modified-project / are-you-sure
requester". Don't repurpose any of these names.

### Command shell

Offer an in-application console (Shell-style window or an Execute
Command-style one-liner) for direct command control; its size/position
should be snapshotable like any window.

## Data sharing

- **IFF everywhere.** All clipboard data must be IFF: `ILBM` for graphics,
  `FTXT` for text. Never blindly assume clip structure when reading — the
  user may have clipped anything from any app.
- Multiple representations of one clip (e.g. `SMUS` + `FTXT` + `ILBM` for a
  bar of music) are wrapped in a `CAT CLIP` form.
- Default to **clipboard unit 0** for interactive cut/copy/paste; let the
  user pick another unit (0–255).
- Register new/modified IFF FORMs with Commodore (historically — for modern
  work, document them publicly).

## Preferences

### Prefs in moderation

- Every option adds complexity. Add a preference only if it has real user
  value — not to dodge a design decision, not because it's feasible.
- **The 90% rule**: if 90% prefer A and nearly everyone could live with A,
  don't make it an option. Adding a control later is far easier than
  removing one.

### Settings files

- Save the user's settings and initialize to them on startup. Controls live
  in the app, reached via the Settings menu. Recommended format: an IFF
  FORM (ASCII/binary acceptable).
- **Load order** (first hit wins): 1) file named by a `SETTINGS` argument
  (Tool Types or CLI); 2) `<basename>.prefs` in the application's own
  directory; 3) `ENV:<basename>/<basename>.prefs`; 4) built-in defaults.
  (Per-aspect settings use a subdirectory of files instead of one file.)
- **Save order**: 1) back to wherever they were loaded from; 2) else
  `<basename>.prefs` in the app directory; 3) if unwritable (e.g.
  write-protected on a network), `ENVARC:<basename>/<basename>.prefs` AND
  `ENV:<basename>/<basename>.prefs`.

### Stand-alone preferences editors

For complex cases (multiple invocations sharing settings, system-wide
settings like a multi-serial card):

- Embed sane defaults in the editor; on start, show current settings (or a
  preset given as an argument), falling back to embedded defaults.
- **Bottom gadgets, in order: Save (left), Use (middle), Cancel (right).**
  - Save: apply + write to `ENVARC:` and `ENV:` (`<basename>/…prefs`;
    only system editors use `ENV:sys`), then exit.
  - Use: apply + write to `ENV:` only (lost on reset), then exit.
  - Cancel: exit, discarding changes.
- Apps sharing a stand-alone editor read `ENV:<basename>/<basename>.prefs`
  and use file **notification** to react to changes live.
- **Menus**: Project (Open RA-O, Save As... RA-A, Quit RA-Q), Edit (Reset
  to Defaults, Last Saved, Restore, Undo), Options (Create Icons?).
- **CLI/Tool Types template**: `FROM,EDIT/S,USE/S,SAVE/S,PUBSCREEN/K` —
  USE/SAVE act on the FROM preset without opening the window (handy in
  Startup-sequence); EDIT (default) opens the window.
