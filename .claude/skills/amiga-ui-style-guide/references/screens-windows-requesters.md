# Screens, Windows, and Requesters

From Style Guide ch. 3–4. Screens and windows are the user's primary
"where am I?" context cues on a multitasking system.

## Screens

Three places an application can open:

1. **Workbench screen** — default; inherits the user's palette, resolution,
   fonts. Prefer this when you have no special requirements.
2. **Public custom screen** — when you need a different resolution/palette
   but others could still share it. Keeping screens public lets the user run
   supporting apps alongside yours without screen-flipping.
3. **Private custom screen** — only for genuinely unusual needs (rapid
   viewport switching, direct whole-screen rendering).

Rules:

- Let the **user choose** where to open (Workbench vs custom; new screen vs
  an existing public one). Custom screens default to the parameters in the
  user's Workbench Preferences unless you have special requirements.
- If you open a custom screen, **redirect all requesters to it** — including
  DOS requesters.
- Screens should have a depth gadget; try not to cover it with your windows.
- Your screen opens in front of existing screens.
- Screens larger than the display must **auto-scroll** when the mouse hits a
  display edge.
- **Naming public screens**: `BASENAME.<invocation#>` in upper case, e.g.
  `AXELTERM.1`, second copy `AXELTERM.2`. Two screens in one invocation get
  distinct derived names, e.g. `VGOGHPAD.1` and `VGOGHPANEL.1`.
- Honour the user's Overscan Preferences.

## Windows

- **Safe place to click**: activating a window must not disturb work — the
  title bar serves this. Title-bar-less full-canvas windows: the first click
  on an inactive window only activates it, never performs an operation.
- Every window must fit on 640×200 with Topaz 8 (guideline, not default).
- Provide sensible default size/position, opening within the *current view*
  of the screen (virtual screens exist) and scaled to the resolution.
- **Session memory**: if the user moves/resizes a window and reopens it in
  the same session, restore their geometry. **Save Settings** persists
  geometry across sessions — including screen width/height so the rectangle
  can be rescaled if the app later runs at a different resolution (move
  first; scale if moving isn't enough; account for the title-bar font in
  min/max sizes). If a window can't fit a resolution at all, consider
  restricting the screen mode — but respect user choices wherever possible.
- On virtual screens: default position inside the visible area, but if the
  user saved an off-screen position, respect it.
- Successive project windows: cascade each new one down by title-bar height
  + 1 pixel, keeping depth gadgets reachable.
- **Draggable whenever possible**; support windows and requesters always get
  a drag bar — an immovable one could hide the very information it concerns
  (e.g. a Find window covering the word being searched).
- View/edit areas get a **sizing gadget**; full-window view areas get a
  scroll gadget in the right border.
- **Zoom gadget**: first click switches to the alternate size, second click
  restores. Opens full-size → alternate is the minimum size; opens small →
  alternate is full size. Unsizable windows: alternate = title bar height ×
  title width, or omit the gadget.
- AppWindow drop areas are marked with outlined-rectangle icon drop box
  gadgets, one per distinct drop function.

### System window gadgets (reference)

- **Close** (top left): removes window, quits what it ran.
- **Depth** (top right, windows and screens): toggles front/back.
- **Sizing** (bottom right): drag to resize. Optional but strongly
  encouraged.
- **Zoom** (next to depth): toggles between two sizes. Optional but strongly
  encouraged.

## Requesters

- **Modal vs non-modal**: prefer non-modal (support windows) — the user can
  ignore them and keep working. Reserve modal for real blockers (e.g. "this
  will destroy all data on the disk" warnings). While a modal requester is
  up, the parent window/screen shows the **wait pointer**.
- **Modified project requester**: on New/Open/Quit (or Close/Hide) with
  unsaved changes, ask "Save changes?" — one requester per unsaved project.
- Use the **ASL requesters** for files etc.; if you must roll your own,
  follow the ASL design. Requesters are base-level operations — users
  shouldn't relearn them per application.
- Both kinds are **draggable**.
- **Placement**: open adjoining or within the parent window, positioned
  relative to the parent (not absolute coordinates). On big virtual screens
  or high-res monitors an absolute-positioned requester may be far away or
  entirely off-view, leaving the user wondering whether anything happened.
- **Always a safe way out**: normally a Cancel gadget at the lower right.
- **Wording**: name your program in the title bar and the specific file in
  the body when reporting a missing file — "Please insert MyApp:Fonts disk"
  troubleshoots; a bare "Can't find file" doesn't. Choose labels so the user
  knows the outcome of each choice without reading an essay.
- Never two identical action gadgets (OK / OK).
- Positive action lower-left, negative lower-right.

## Requester checklist

- [ ] Non-modal unless blocking is genuinely required
- [ ] Draggable, placed relative to parent window
- [ ] Cancel (true revert) or Stop (mid-operation) available
- [ ] Positive left, negative right; no duplicate gadgets
- [ ] Program name in title; specifics (filenames) in body text
- [ ] Modal ⇒ wait pointer on parent
- [ ] Redirected to your custom screen if you have one
