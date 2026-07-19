# Gadgets

From Style Guide ch. 5. Use GadTools gadgets wherever they fit; never make
a gadget that *looks* like a standard type but behaves differently.

## General rules

- Layout for 640×200 + Topaz 8; at run time check the user's font/resolution
  and revert to Topaz 8 if the layout doesn't fit.
- **Label** anything not obvious: one to three terse words, capitalized per
  the language's normal style. Test labels on people if you can.
- **Ghost** unavailable gadgets (dot-grid overlay); never selectable no-ops.
- **Keyboard equivalents**: bind a logical letter from the label
  ("Spacing" → S, "Get Fonts" → F), underline that letter in the label.
  Lower-case letter is the default action; shifted often reverses direction.
  Same visual feedback as mouse use; action on key **downpress**.
  - Never give keyboard equivalents to **asynchronous requesters** (ones the
    app raises on its own, e.g. "insert next disk") — the user may be typing
    in another app. The system provides Left-Amiga-V / Left-Amiga-B for the
    leftmost / rightmost bottom gadgets of a requester.
  - Never bind Return to OK, and avoid assigning Enter to gadgets.
- **Grouping**: by function; common controls in easy reach; dangerous
  controls (Delete, Format) away from common ones; layout follows work
  order, upper-left → lower-right.

## Gadget types and when to use them

### Action gadget (button)
- Performs the named activity on select-button **release** (roll-off escape).
- Descriptive, friendly labels: "Stop" not "Abort"; "OK"/"Cancel" aren't
  always best.
- Opens another window/requester → label ends in `...`
- "Cancel" only when it truly restores the pre-requester state; during an
  operation (e.g. printing) use "Stop".
- Positive gadget lower-left, negative lower-right.
- Keystroke: appears pressed on key downpress, released on key up.

### Check box
- Single on/off option. Keystroke toggles the check.

### Scroll gadget (scroller)
- Bar + box + arrows; adjusts a view over larger content. Any partial view
  needs one; add horizontal too if content is wider than the window.
- Update the display live while the bar is dragged.
- Click in the trough (box, not bar) → move by a **viewful**, keeping one
  line of the previous view visible so nothing is missed.

### Scrolling list
- Variable lists of files/objects (e.g. file requesters); mandatory once
  options exceed ~a dozen.
- System list = single selection; custom multi-select lists must follow the
  ch. 2 multiple-selection conventions.
- Keystroke: unshifted cycles forward, shifted backward.

### Radio buttons
- Mutually exclusive, exactly one always selected; small fixed sets; all
  options visible; the most intuitive one-of-N control.
- Keystroke: unshifted cycles forward, shifted backward.

### Cycle gadget
- One-of-N showing only the selected choice; cleaner, scales to ordered
  lists (months). **Attributes only — never actions**, and never a two-state
  on/off (that's a check box).
- Keystroke: unshifted forward, shifted backward.

### Colour selection (palette) gadget
- Pick from a fixed palette. Keystroke: unshifted forward, shifted backward.

### Slider
- Value in a range (volume, intensity). Trough clicks move **one increment**
  (vs a scroller's viewful).
- Keystroke: unshifted increases, shifted decreases — but when the bar hits
  an end it should auto-reverse direction; don't rely on Shift alone.

### Text (string) gadget
- Alphanumeric entry. Auto-activate for typing when the requester appeared
  in direct response to the user AND no other keyboard-activatable gadgets
  exist; otherwise activate via its keyboard equivalent. Never force
  keyboard → mouse → keyboard round trips.
- Multiple fields: activate the top-left one first (per language scan
  direction); **Tab** advances (wrapping), **Shift-Tab** goes back —
  Intuition supports this.
- Standard editing keys: Right-Amiga-X erase, Right-Amiga-Q undo. Custom
  multi-line text gadgets must support the same keys.

### Display box
- Read-only text/numbers; looks like a text gadget but **recessed**.

### Icon drop box
- Outlined rectangle receiving dragged icons (AppWindows); one per drop
  function.

### Custom gadgets and icons
- Extensions of a standard type must emulate that type's behaviour.
- Minimum three images: normal, selected, disabled; must work in monochrome.
- If draggable custom icons coexist with graphic-labelled action gadgets,
  make them distinguishable at a glance (spatial grouping, colour cues, or
  boxing) so users don't trigger actions they meant to move.

## Keystroke feedback table

| Gadget | Unshifted key | Shifted key |
|---|---|---|
| Action button | press in (down), release out (up) | — |
| Check box | toggle check | — |
| Scrolling list / radio / cycle / selection / scroll | cycle forward | cycle backward |
| Slider | +1 unit (auto-reverse at ends) | −1 unit |
| Text / numeric | activate for entry | — |
