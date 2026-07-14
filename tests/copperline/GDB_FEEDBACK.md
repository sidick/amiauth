# Copperline `--gdb` feedback: symbol-level debugging of a guest-launched program

Notes from evaluating Copperline's GDB remote support for automated on-target
testing of AmiAuth. Recorded here for reference and to pass upstream. The
`--serial stdout` / `--benchmark-until` path we ended up using worked well; this
documents only the gdb workflow that didn't, so the emulator's automation story
is understood accurately.

## Environment

- Copperline **0.11.0** (Homebrew, macOS/arm64)
- Toolchain: bebbo `amiga-gcc` (the `stefanreinauer/amiga-gcc:latest` container) —
  `m68k-amigaos-gcc` 6.5.0b, and its `m68k-amigaos-gdb`:
  `GNU gdb (GDB) 13.0.50.251124-132852-git`
- Target program: an AmigaOS **hunk** executable built with `-g`, `LoadSeg`'d by
  AmigaDOS during boot (i.e. started *inside* the guest, after gdb connects)

**Goal:** connect gdb, break on a C function by name, read its args/return — the
workflow described in `docs/debugger/gdb.md`.

## What worked

The stub is fine at the protocol level. `--gdb :PORT` logs
`gdb: listening on 127.0.0.1:PORT`, `target remote` connects, and register/memory
reads and absolute-address breakpoints all work. The problems are specifically in
the **symbol / relocation** workflow.

## Issue 1 (Copperline) — `monitor` commands are not implemented

`docs/debugger/gdb.md` recommends `monitor segments` to get segment base
addresses for manual `add-symbol-file prog.elf ADDR`. In 0.11.0 the stub doesn't
support `qRcmd`:

```
(gdb) target remote :2345
(gdb) monitor segments
"monitor" command not supported by this target.
(gdb) monitor help
"monitor" command not supported by this target.
```

So the documented manual-relocation fallback is unavailable. Most actionable
item: implement `monitor segments` (and `monitor help`), or update the docs.

## Issue 2 (Copperline) — no way to relocate a program loaded *after* connect

The docs say the stub answers gdb's `qOffsets` so symbols relocate automatically.
But `qOffsets` is queried once at attach. For a program AmigaDOS `LoadSeg`s later
(during boot, after gdb has connected), there's no mechanism for gdb to learn the
runtime segment base afterwards — no stop/library-load event, and (per Issue 1)
no `monitor` query. A pending breakpoint therefore never binds to the real load
address. It would help to either emit a library-load/stop event when a new
segment is `LoadSeg`'d (so pending breakpoints can resolve), or expose the
segment table via `monitor`. Clarifying in the docs whether `qOffsets` is only
intended for a ROM-time / pre-known program would also help.

## Issue 3 (toolchain/doc mismatch) — gdb can't read the debug-hunk executable

Largely a toolchain issue, but it directly undercuts the docs' claim that "with
bebbo's toolchain, source-level debugging works automatically," so worth
flagging. With the standard amiga-gcc above:

- `m68k-amigaos-gcc` emits a hunk exe (collect2 `-amiga-debug-hunk`), and this
  gdb **can't parse it**: `"…": not in executable format: file format not
  recognized` — even with `set gnutarget amiga` / `objdump -b amiga`.
- The toolchain's `ld` has **no ELF output target**
  (`supported targets: amiga a.out-amiga srec symbolsrec verilog tekhex binary
  ihex plugin`), so there's no ELF-with-DWARF executable to feed gdb instead.
- Loading the intermediate ELF **object** files (which do have
  `.debug_info`/`.debug_line`) crashes gdb:
  `internal-error: read_comp_unit_head: dwarf from non elf file`.

Net: on a stock amiga-gcc install, none of {load the hunk exe, produce an ELF,
load the `.o`} gives gdb usable symbols. If the docs were validated against a
specific gdb/BFD build that *can* read Amiga debug-hunks, please state exactly
which gdb build/version and where to get it — the default container's gdb can't.

## Minimal repro

```sh
copperline --config machine.toml --noaudio --gdb :2345 <kick31.rom>
# other terminal:
m68k-amigaos-gdb -batch -ex "target remote :2345" -ex "monitor segments"
#   -> "monitor" command not supported by this target.
m68k-amigaos-gdb -batch -ex "file prog"          # prog = -g hunk exe
#   -> not in executable format: file format not recognized
```

## Workaround

Dropped gdb entirely and had the guest emit results over serial via
`exec/RawPutChar`, captured with `--serial stdout --benchmark-until`. Easy to
automate; the serial/headless side of Copperline is excellent. See `serialtest.c`
and `run.sh` in this directory.
