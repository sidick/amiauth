# Driving Amiberry via the `mcp__amiberry__*` tools

Findings from a #37 on-hardware verification attempt using
`runtime_send_mouse` / `runtime_send_text` / `runtime_send_key` /
`runtime_screenshot_view` against a live Amiberry instance. Read this before
trying to script interactive Amiberry testing again — several hours went
into re-discovering these.

## Mouse: always split move and click into separate calls

`runtime_send_mouse(dx, dy, buttons)` combining a nonzero move with
`buttons=1` in the **same** call clicks at the **stale pre-move position**,
not the destination — the button state seems to be evaluated before the
motion is applied. This produced a long chain of "clicks that do nothing
visible" and misdirected keystrokes (typed text/hotkeys landing in the wrong
window) that looked like focus bugs but were actually just wrong-position
clicks.

**Always** do three separate calls:
1. `send_mouse(dx, dy, buttons=0)` — pure move
2. (optionally) `screenshot_view` to confirm the pointer sprite is where you expect
3. `send_mouse(dx=0, dy=0, buttons=1)` then `send_mouse(dx=0, dy=0, buttons=0)` — click at the now-current position, as two calls

Clamp to a known origin before any multi-step aim: `send_mouse(-2000, -2000,
0)` pins the pointer to the screen's top-left corner regardless of prior
position, giving you a reliable zero to compute deltas from.

## Small-target clicks (buttons, close gadgets) stayed unreliable

Even with move/click properly separated, clicking a small ReAction button
(~70×17px) repeatedly failed to register — no pressed-state visual, no
window activation change, and occasionally the click seemed to land on
whatever window was *behind* the visible dialog. Root cause not conclusively
diagnosed. One live data point worth following up before the next attempt:
the configured display was `gfx_width=720 gfx_height=568` but the actual
`runtime_screenshot_view` PNGs measured **756×287** (`sips -g pixelWidth -g
pixelHeight`) — a ~2x mismatch on height that smells like an interlaced/laced
screen mode being captured at half vertical resolution. If the coordinate
space `send_mouse` deltas operate in isn't the same space the screenshot is
rendered in, every pixel you read off a screenshot and click on would need a
correction factor (roughly ×2 vertically, ~1x horizontally, unconfirmed) —
this was never proven or fixed this session, just flagged. Large targets
(window bodies, backdrop) were never affected — only small gadgets.

## MovePointer is not an automatic fix under Amiberry

MovePointer (see `tests/tools/movepointer/README.md`) solved the equivalent
precision problem cleanly under **Copperline**, with a clean derived 1:1
linear offset. Trying it under **Amiberry** in this session did not
straightforwardly transfer: its own screen-coordinate calibration needs to be
re-derived per emulator/profile (a probe landed on a dialog's title bar
instead of its OK button, a large systematic miss, not a few-pixel one), and
typing the `MovePointer X Y` command itself hit the same `send_text`
flakiness described below — so the tool that was supposed to route around
imprecise input became gated on the same imprecise input. Any next attempt
should calibrate MovePointer here the same deliberate way the Copperline
skill does (command two well-separated known points, screenshot, measure)
rather than assuming the Copperline offsets carry over.

## `runtime_send_text` corrupts longer/special-character strings

Typing something like an `otpauth://...&issuer=...` URI or a `SRC:path`
Shell command at the default delay produced garbled output — dropped
characters, and at least one confirmed `:` → `;` substitution — leading to
`argument line invalid` / `Unknown command` errors on a line that *looked*
correct when screenshotted mid-type. Raising `delay_ms` to 150 fixed most of
it but not all; treat any `send_text` call with punctuation as needing a
post-type screenshot check before pressing Return, not just a "typed N
characters" success from the tool call itself.

**Workaround that reliably sidesteps this entirely**: write the command(s) to
a file on the host and `Execute` it from the guest Shell instead of typing
the payload directly. `SRC:` is mounted read-write onto the repo checkout
(see `machine.toml`-equivalent `filesystem2=rw,SRC:AmiAuth:...` in the
Amiberry `.uae` config) — write a scratch `.script` file under `build/`
(gitignored) with the exact command sequence, then just type the short
`Execute SRC:build/whatever.script` line, which is far less likely to get
mangled than a long URI. Delete the scratch script afterward.

## A dragged/misplaced Amiberry window is not worth fixing in place

A stray combined move+click-drag (see above) can drag the whole Amiberry
host window out of position — screenshots then show a large dead gray margin
and shifted content. Dragging it back by hand from IPC did not work reliably
in one attempt. **Kill and relaunch** (`kill_amiberry` then
`launch_and_wait_for_ipc`) is faster and fully resets window position; don't
sink time into recovering the old one.

## Running interactive Shell commands without touching the mouse

Add a line to the test volume's `S:User-Startup` (on the host, e.g.
`~/Documents/Amiberry/HardDrives/<profile>/S/User-Startup`) so a Shell window
opens automatically at boot:

    NewShell CON:10/10/620/180/AmiAuth-Shell

This gives a keyboard-only path (via `send_text`/`send_key`) for anything
that doesn't need the mouse at all — CLI-forwarding tests, single-instance
checks, etc. — without ever needing to click to open a Shell.

## This is a shared, live window — coordinate with whoever else is watching

If the person you're working with is also looking at the same running
Amiberry instance (e.g. checking Prefs settings), you're both capable of
sending input to the same guest concurrently. Confirm before assuming
"unexpected" state changes are automation bugs versus a human interacting at
the same time — but don't over-assume this either; in the one session this
was checked, concurrent human input turned out to explain very little of the
observed flakiness. The mouse/text issues above are real, independent of
that.
