---
name: amiga-ui-style-guide
description: Design and review user interfaces for classic AmigaOS (2.0+) applications following the Commodore Amiga User Interface Style Guide (1991). Use this skill whenever writing, reviewing, or designing Intuition/GadTools GUIs, requesters, menus, gadgets, Workbench icons, Tool Types, ARexx ports/command sets, Shell command templates, preferences/settings handling, or keyboard shortcuts for Amiga software — even if the user just says "make this app feel native", "add a GUI to my Amiga program", "what should this requester say", or asks about AmigaOS conventions. Also use it when naming public screens, ARexx ports, or settings files for an Amiga application.
---

# Amiga User Interface Style Guide (Commodore, 1991)

Condensed, actionable rules from the official Commodore-Amiga style guide
("ROM Kernel Reference Manual: User Interface Style Guide", © 1991) for making
Amiga applications look and behave like native, well-mannered citizens of
AmigaOS Release 2+.

## Core philosophy

- **Power to the user.** The interface should be simple, predictable,
  consistent, adaptable, and intuitive. When in doubt, give the user the
  choice and respect the choices they've already made in Preferences.
- **Support all three interfaces**: the GUI (Intuition/Workbench), the Shell
  (CLI), and ARexx. A full-featured Amiga app is drivable from all three.
- **Design for the novice; conserve the user's energy for creative tasks.**
- **Let the system work for you**: use GadTools gadgets, Intuition menus,
  ASL requesters, and `ReadArgs()` parsing instead of custom code. If you
  must build custom elements, model them on the system's.
- **Economize shared resources** (RAM, serial/parallel ports, drives): grab
  late, release early, and let the user suspend use of non-shareable devices
  without quitting.
- **Object-action, not action-object.** The user selects a thing, then picks
  what to do to it. Avoid modes; they confuse and restrict.
- **Always give feedback.** Every action gets an immediate visible reaction;
  long jobs get a progress requester with a Stop gadget.
- **Never let the user select something that does nothing** — ghost (disable)
  unavailable gadgets and menu items.
- **One basename rules everything.** Pick a short one-word basename (ideally
  the executable name) and derive all user-visible names from it: public
  screens `BASENAME.1`, ARexx ports `BASENAME.1`, settings
  `<basename>.prefs`.

## Quick decision rules

| Situation | Rule |
|---|---|
| One on/off option | Check box (menu: indented toggle item with check mark, optionally ending in `?`). Never a cycle gadget or an On/Off submenu. |
| One-of-N, N small (~2–5) | Radio buttons (all choices visible, most intuitive). |
| One-of-N, N up to ~a dozen, esp. ordered | Cycle gadget (attributes only, never actions). |
| One-of-N, N > ~a dozen or variable | Scrolling list, not a menu. |
| Pick a value in a range | Slider (clicks in the trough move by single increments, unlike scrollers which move by viewfuls). |
| Editable text/number | Text (string) gadget; read-only info uses a recessed display box. |
| Gadget/menu item opens another window or requester | Label ends in an ellipsis `...` |
| Confirm/deny in a requester | Positive action lower-LEFT, negative (Cancel) lower-RIGHT. Never two identical gadgets. "Cancel" only if it truly reverts everything; use "Stop" mid-operation. |
| Unsaved changes + New/Open/Close/Quit | Show the modified-project ("Save changes?") requester — one per unsaved project. |
| Long operation | Progress requester with a Stop gadget, not an animated pointer. One bar = one fill; don't reuse a bar for successive phases. |
| Blocking input needed? | Prefer non-modal requesters. If modal, set the parent window's pointer to the wait pointer. |

## Layout & rendering baseline

- Design and verify every window, requester, and menu on a **640×200 screen
  with Topaz 8**. At run time, check the user's font/resolution; if things
  don't fit, fall back to Topaz 8.
- **3-D look**: light comes from the upper left. Raised = selectable/editable;
  recessed = read-only/display. Clicking a button shows it recessed +
  highlighted. Every custom gadget needs normal, selected, and disabled
  imagery, and must work in monochrome.
- **Colour is an extra cue, never the only cue** (monochrome displays,
  colour-blind users). Use subdued colours; ensure luminance contrast.
  Same for sound/speech: never the sole feedback channel, and always
  user-disableable.
- Group gadgets by function; keep frequent controls handy; keep dangerous
  controls (Delete, Format) away from common ones; order work flow
  upper-left → lower-right.
- Actions trigger on mouse-button **release** (roll-off escape); keyboard
  activation acts on key **downpress** with the same visual feedback.

## Reference files — read the one matching the task

- `references/fundamentals.md` — metaphor, focus, feedback, colour/palettes,
  fonts, internationalization, 3-D look details, ghosting, mouse/selection
  conventions, pointers, wait/progress behaviour, resolutions.
- `references/screens-windows-requesters.md` — screen types & naming,
  window sizing/positioning/zooming, Save Settings behaviour, requester
  types, placement, and wording.
- `references/gadgets.md` — every gadget type with usage rules, labelling,
  keyboard equivalents, and keystroke feedback table.
- `references/menus.md` — menu design rules and the full standard menu
  layouts (Project, Edit, Macros, Settings, User) with Right-Amiga shortcuts.
- `references/keyboard.md` — reserved system shortcuts, cursor-key modifier
  semantics, function/Help key rules.
- `references/workbench-icons.md` — icon design, .info files, Create Icons?,
  Tool Types, Default Tool, AppWindows/AppIcons/AppMenus.
- `references/shell-arexx-prefs.md` — command templates & standard
  arguments, `$VER` strings, ARexx port naming, the 15 standard ARexx
  commands, return codes, clipboard/IFF rules, settings file search/save
  order, preferences editor conventions.

When reviewing an existing UI, walk the checklist in each relevant reference
file and report deviations with the fix. When designing a new UI, start from
the standard menus and gadget decision rules above, then consult the
references for the specifics.
