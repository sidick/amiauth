# MovePointer

A dev/test-only utility, **not** part of AmiAuth and never shipped to users —
see [`THIRDPARTY.md`](../../../THIRDPARTY.md) for what actually ships in the
binary. Kept here so on-target GUI tests can drive the mouse precisely under
Copperline without redownloading it each time.

- **Source:** [Aminet, `util/batch/MovePointer`](https://aminet.net/package/util/batch/MovePointer)
- **Author:** G.S.G. 9 (Cewy), 10 May 1987
- **License:** explicitly placed in the **Public Domain** by the author (see
  the header comment in `movepointer.c`); redistribution is fine.
- **Binary:** deliberately **not vendored** — only the plaintext source is
  checked in. `make movepointer` (or `make movepointer-docker`, no local
  toolchain needed) cross-builds it fresh with this project's own
  `m68k-amigaos-gcc`, so the only thing ever run is a binary this repo's own
  trusted toolchain produced, never an unreviewed 1987 executable from an
  unmoderated archive. Output: `build/MovePointer`. Compiles clean (old
  K&R-style implicit-declaration warnings only, suppressed with `-w` like
  the vendored `quirc` third-party source) — needs `-noixemul -m68000` to
  behave correctly (confirmed: without `-noixemul` the built binary loads
  but the pointer never visibly moves, presumably from missing
  `ixemul.library` on a stock boot).

## What it does and why it's here

`AmiAuth MovePointer X Y [R] [K]` sends an `IECLASS_RAWMOUSE` event straight
to `input.device` via `IND_WRITEEVENT`, run **from inside the guest**. This
sidesteps Copperline's `--mouse-after DX DY`, whose host→guest delta
translation is non-linear (see the `copperline-testing` skill) and was never
usably precise for clicking a specific gadget in a scripted test — this tool
is what made screenshot-driven GUI click tests practical.

    MovePointer X Y        # clamp to the screen origin, then move to (X,Y)
    MovePointer X Y K      # ...then simulate a left-click there too
    MovePointer 0 0 R K    # click at the CURRENT position (R = relative move,
                            # so (0,0) is a no-op; add a prior Wait to settle)

Stage `build/MovePointer` (after `make movepointer-docker`) on the boot
volume and invoke it from `Startup-Sequence` **after a `Wait`** long enough
for Workbench to finish loading (a call placed immediately after `LoadWB`
with no `Wait` silently does nothing — Workbench hasn't created a pointer
sprite yet) or from a Shell, e.g. `DH0:MovePointer 155 85 K`. The mapping
from commanded (X,Y) to
actual on-screen/screenshot pixels is a clean 1:1 linear offset — re-derive
the two constants for your machine profile/resolution by commanding two
well-separated points and comparing against `--screenshot-after` output, the
same way `copperline-testing`'s worked example does it. See that skill for
the full recipe, caveats (very small targets still need extra calibration
rounds), and the specific offsets already measured for this project's
A1200 test profile.
