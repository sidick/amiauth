# Keyboard

From Style Guide ch. 10. Everything selectable by mouse should also be
selectable from the keyboard; let users bind any function or ARexx macro to
keys.

## Key groups

- **Standard keys**: national layouts differ (QWERTY/QWERTZ/...) — always go
  through the **keymap** facility, never raw positions.
- **Special keys**: F1–F10, Help, cursor keys, Del, Backspace, Esc.
- **Modifiers**: Ctrl, Shift, Alt, Amiga. They modify keys and mouse clicks
  (Shift-click = multi-select).
- **Dead keys** (e.g. Alt-H caret on US layout): vital in many languages —
  handle them if you do your own raw key mapping.

## Reserved system shortcuts — never reuse

Mouse emulation:

| Combination | Function |
|---|---|
| Either-Amiga + Left-Alt | Left (selection) button click |
| Either-Amiga + Right-Alt | Right (menu) button click |
| Either-Amiga + cursor key | Move pointer |
| Either-Amiga + Shift + cursor | Move pointer in larger steps |

System functions (second key user-configurable via IControl Prefs):

| Combination | Function |
|---|---|
| Left-Amiga + N | Workbench screen to front |
| Left-Amiga + M | Front screen to back |
| Left-Amiga + B | Requester: leftmost bottom gadget (OK) |
| Left-Amiga + V | Requester: rightmost bottom gadget (Cancel) |
| Left-Amiga + selection button | Drag screen from anywhere |

**Left-Amiga is reserved for the system at all times** — application
shortcuts use Right-Amiga (menu defaults) or plain/underlined letters
(gadgets).

## Three rules for keyboard-activated gadgets

1. Action on the key **downpress**.
2. Identical visual feedback to mouse activation.
3. Don't assign the Enter key to a gadget (and never Return to OK).

## Cursor key semantics

| Modifier | Meaning |
|---|---|
| none | Small move — one unit (character, pixel) or a few pixels where navigation matters more than precision |
| Shift | To the window extreme, or one windowful further if already there; asymmetric apps may page vertically and do line-start/line-end horizontally |
| Alt | Application-specific semantic units (word, spreadsheet field) |
| Ctrl | To the project extreme (start/end/leftmost/rightmost) |

## Function keys and Help

- F-keys belong to the **user** — reserve them for user-defined bindings; if
  the app uses them, allow redefinition.
- If you have built-in help, wire it to the **Help key**. Nothing is worse
  than pressing Help and getting silence.
