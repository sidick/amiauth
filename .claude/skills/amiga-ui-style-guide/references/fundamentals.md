# GUI Fundamentals

Distilled from "Some Basics" / "Basics of the GUI" (Style Guide ch. 2).

## Interaction model

- **Metaphor**: present complex machinery as familiar real-world objects
  (drawers, clipboards, pasting). Only where it genuinely fits — not every
  interface needs a physical analogue.
- **Object-action**: select the object first, then the operation. This avoids
  modes (e.g. a "delete mode") which are restrictive, confusing, and
  dangerous if the user walks away and comes back.
- **Focus**: identify where the user is looking (cursor, selected object,
  pointer) and use it as an information channel — e.g. the wait pointer works
  because it appears at the focus, not in a title bar.
- **Feedback**: every action gets an immediate reaction. For slow work, use
  surrogate feedback (drag an outline, refresh only the edited line) — the
  illusion of responsiveness is as good as the real thing. Use multiple media
  where appropriate (e.g. hear the pitch change while dragging a note).

## Selection conventions

- Selection button (left) selects/operates; menu button (right) drives menus
  and **aborts an in-progress selection drag** (e.g. snap a dragged window
  back to its original position).
- Actions and tool activation trigger on **release** of the selection button
  so the user can roll off to cancel. Double-click on a tool gadget may open
  that tool's settings editor.
- **Current object**: the selected thing that subsequent actions operate on.
- **Shift-selection** builds a selected group. Only ONE current object OR one
  selected group may exist at a time, application-wide — never both. Keep
  selection to a single context/level (characters or documents, not both).
- **Dragging**: contiguous (anchor point → release, or marquee rectangle on
  Workbench) and, less commonly, non-contiguous (click to deselect items
  inside a drag-selected range). Auto-scroll when the drag passes the view
  boundary.
- **Text highlighting**: four methods — drag; extended selection
  (click anchor, then Shift-click end, scrolling allowed in between);
  multiple-click (double = word, triple = paragraph, fourth click reverts;
  not timed clicks); and Select All via menu (selects contents, not the
  document/window itself).
- Shift+key combinations must never be the *only* way to do something —
  always have a mouse-visible counterpart.

## Colour

- Simple images with few colours beat complicated colourful ones. Avoid
  intense pairings (blue/yellow, red/green, red/blue, green/blue); prefer
  subdued over fully saturated — the user lives with it for hours.
- Let the user **load, save, and edit the palette** for your screens, and
  re-derive your rendering pens when the palette changes.
- Be consistent: window backgrounds in background colour, text in text
  colour, headings in highlight colour.
- **Never colour alone**: monochrome and grey-scale displays exist, and many
  users are colour-blind. Pair colour with position, size, and rendering.
  Check that functionally significant colours contrast in *luminance* with
  their neighbours; verify readability in monochrome/grey-scale if you can
  open on Workbench or a public screen.
- A palette that maps to solid greys on the A2024 grey-scale monitor:
  colour 0 light grey (10,8,6), colour 1 black (0,0,2), colour 2 white
  (15,15,15), colour 3 dark grey (7,7,9).

## Fonts

- The user picks an icon font, a screen font (preferred system font, may be
  proportional) and a system default font (guaranteed monospaced). Adjust
  menus, windows, requesters, and gadgets to fit whatever size is chosen.
- Need monospace (spreadsheets, columnar data)? Honour the **system default
  font**. Can handle proportional? Honour the **screen font**.

## Internationalization

- Expect text to grow 30–50% when translated from English (one of the
  tersest languages). Leave room.
- Centralize all UI strings in one table/file so they can be localized in
  one place; never scatter embedded text. But do NOT localize scripting/ARexx
  keywords — that would break script sharing across countries.
- Date, time, 12/24-hour, decimal separator, and thousands separator formats
  vary by country (e.g. British English dd/mm/yy, 12hr, period decimal,
  comma thousands; German dd.mm.yy, 24hr, comma decimal, period thousands;
  Swedish yy-mm-dd with `hh.mm.ss` times). Build for this from the start.

## The 3-D look (Release 2)

- Light from the **upper left**; shadows lower right. Raised object: light
  lines top/left, dark lines bottom/right. Recessed: reversed.
- Raised = available/modifiable. Recessed = read-only/display-only.
- Clicking an icon/button flips it raised → recessed + highlighted.
  Highlighted vs normal imagery must be distinctly different (Workbench uses
  complementary colours).
- GadTools provides all of this for standard gadgets — use it. For anything
  custom, copy existing system elements' style and behaviour.

## Ghosting

- Anything unavailable must look obviously unselectable — Intuition overlays
  a dot-grid in the shadow colour. Never leave an enabled control that does
  nothing when clicked.

## Pointers

- Custom pointers must keep the standard colour-intensity arrangement
  (colour 0 transparent, 1 medium, 2 darkest, 3 brightest; frame the shape
  in colour 1 or 3 for contrast) since applications share screens.
- Pointers can carry context: reflect the active tool, or change by window
  region (cross-hair over a canvas, arrow over the control panel).

## Waiting and progress

- Busy and can't accept input → show the wait pointer.
- Measurable long job → show a **progress requester with a Stop gadget**
  instead of an animated pointer (the pointer disappears when the user
  clicks into another app's window on a shared screen; a requester stays
  visible).
- Don't give the progress requester's window a wait pointer — the user will
  think Stop can't be clicked.
- One progress bar fills exactly once. Multi-phase jobs get multiple bars
  (if per-phase progress matters) or a single overall bar — never a bar that
  refills per phase under changing captions.

## Resolutions

- Baseline: everything must fit and function on non-interlaced, non-overscan
  NTSC **640×200 with Topaz 8** (not necessarily as default — as the least
  common denominator). Design for this even for PAL-first audiences.
- Respect the user's Preferences for resolution and display size. Text-heavy
  apps use the *text* overscan setting; graphics apps use *standard*
  overscan.
