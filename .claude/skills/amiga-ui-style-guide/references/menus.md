# Menus

From Style Guide ch. 6. Let Intuition run your menus.

## Design rules

- Design on 640×200 + Topaz 8; at run time, if the user's font/screen won't
  fit, revert to Topaz 8.
- Prefer **fixed** menus; big variable sets (fonts) belong in a requester or
  scrolling list. (The User menu is the sanctioned exception.)
- Support **multiple selection** (click several items with the select button
  before releasing the menu button); on conflicts, later choice wins.
- Dark text on light background where the palette allows.
- Uniform font across items (deliberate exceptions OK, e.g. Bold/Italic
  items rendered in their own style).
- **Toggle items**: leading check mark that appears/disappears; indent all
  toggle items to reserve the mark's space; optionally end the label with
  `?` ("Create Icons?"). Never an On/Off submenu.
- **Grouping**: by function, separated with separator bars; keep toggle and
  non-toggle items apart; cluster similar commands (all "Save as ..."
  together); frequent items near the top; keep dangerous items away from
  frequent ones (mis-picks happen).
- **Size**: ~a dozen items per menu max; ~half a dozen per submenu; utility
  falls as length grows.
- **Menu bar order**: outer positions are easiest to reach — put rarely-used
  menus toward the middle. Standard order: Project first (leftmost), Edit
  second, ... Settings second-from-right, User far right.
- **Ghost** inapplicable menus/items.
- **Labels**: one to three words; action items named as actions ("Print",
  not "Printer"); friendly and non-technical; don't repeat the menu title in
  each item ("Load..." inside a Macros menu, not "Load Macros...") unless
  comprehension suffers.
- Item opens a window/requester → append `...`. Item has a submenu → `»`
  flush right (right-justify anything meant to line up on the right, or
  proportional fonts will break it). Submenus appear to the item's right.

## Standard menus

Use these whenever applicable — names, order, and shortcuts are part of the
spec. All shortcuts are Right-Amiga (localize for non-English).

### Project menu (first)

| Item | Shortcut | Behaviour |
|---|---|---|
| New | RA-N | Blank untitled project |
| Open... | RA-O | File requester; modified-project requester first if unsaved changes (unless multi-project) |
| Save | RA-S | Overwrite prior save; file requester if never saved |
| Save As... | RA-A | File requester for name, then save |
| Print | RA-P | Print with current settings (optional) |
| Print As... | — | Requester for print options (optional) |
| Hide | — | Multi-project: remove project window; unsaved-changes requester deferred to Quit |
| Reveal... | — | Multi-project: scrolling-list requester of ALL open projects (not just hidden); chosen window comes to front; ghosted when nothing is hidden |
| Close | — | Multi-project: close current project window, modified-project requester first if needed |
| About... | — | Window with at least the version number; suggested: ARexx port name, project size, tool name |
| Exit [Level] | — | Multi-level programs: leave current level (name it in the label) |
| Quit [Program]... | RA-Q | Exit entirely; one modified-project requester per unsaved project; must be available at any level (non-modal) |

### Edit menu (second)

Anything operating on blocks of data lives here. Use the system clipboard
as the cut/paste buffer so clips flow between applications.

| Item | Shortcut | Behaviour |
|---|---|---|
| Cut | RA-X | Remove selection to clipboard |
| Copy | RA-C | Duplicate selection to clipboard; unhighlight to confirm |
| Paste | RA-V | Insert clipboard at insertion point |
| Erase | — | Remove selection, no clipboard |
| Undo | RA-Z | Revert last action; multi-step undo keeps stepping back |
| Redo | — | Only with multi-step undo: undoes mistaken Undos (single-step undo is cyclical and needs no Redo) |

### Macros menu

Macros should be ARexx, not a private scripting language.

- **Start Learning** — record subsequent actions as an ARexx script into a
  buffer; give subtle-but-findable feedback that recording is on (e.g. a
  glyph in the title bar).
- **Stop Learning** — stop recording; remove the feedback.
- **Assign Macro...** — requester to bind the recorded script (function
  keys, key combos, User menu, or named + saved to disk — reflect whatever
  your program supports).
- **Load... / Save...** — optional pair for per-file macro persistence
  (default assumption: assignments are per-session; Save Settings can also
  persist them).

### Settings menu (second from right)

- **[options]** — app-specific settings, as toggles/submenus/requesters, or
  one "Set Settings..." control panel. Changes apply to the session only
  until saved.
- **Create Icons?** — toggle, on by default: whether project saves also
  write a `.info` icon file.
- **Load Settings...** — optional: switch to a previously saved settings
  file.
- **Save Settings** — persist current settings as the defaults (into the
  current settings file).
- **Save Settings As...** — optional, for multiple settings files (per
  project type, per network user).

### User menu (far right)

Variable-length menu of user-defined macros and user-added items.

## Menu keyboard shortcut summary

Right-Amiga + N/O/S/A/P/Q (Project), X/C/V/Z (Edit). Left-Amiga is reserved
for the system — never use it in application shortcuts.
