# Copperline `--gdb` feedback: symbol-level debugging of a guest-launched program

Notes from evaluating Copperline's GDB remote support for automated on-target
testing of AmiAuth. Recorded here for reference and to pass upstream. The
`--serial stdout` / `--benchmark-until` path we ended up using worked well; this
documents only the gdb workflow that didn't, so the emulator's automation story
is understood accurately.

## Resolution (2026-07-19, upstream: LinuxJedi/Copperline#181)

Filing this issue got a detailed, specific reply from the maintainer, and
independent re-testing here confirms most of it against Copperline **0.12.0**
and the same gdb build as the original report:

- **Issue 1 was a false alarm, not a Copperline bug.** `monitor` has answered
  `qRcmd` since before 0.11.0. `"monitor" command not supported by this
  target"` is gdb's *dummy-target* response — it prints that when `target
  remote` never actually connected, and a `-batch` script keeps running past
  a failed `target remote` rather than stopping. Our original repro ran gdb
  **inside the `stefanreinauer/amiga-gcc` Docker container**, whose
  `localhost` is not the host's — so `target remote :2345` silently never
  reached Copperline listening on the host. **Confirmed here**: connecting
  from a gdb *on the host* (`/opt/amiga/bin/m68k-amigaos-gdb` — this
  project's local toolchain copy, not the container's) to a live `--gdb`
  session, `monitor help`/`monitor loadseg-break`/`monitor loadseg-list` all
  work immediately, no networking workaround needed.
- **Issue 2 is fixed as of 0.12.0** with `monitor loadseg-break`: arms a stop
  that fires the moment a new program's seglist installs (post-relocation,
  pre-first-instruction), reporting the program name and base address ready
  for `add-symbol-file`. The stub also no longer exits on gdb detach — it
  stays paused, so a script can `detach` after arming, `file` the now-known
  binary, and `target remote` again to bind breakpoints against the real
  addresses. **Confirmed here**: booting `tests/copperline/machine.toml`
  with `--gdb`, `monitor loadseg-break` armed successfully and `continue`
  stopped exactly as documented on AROS's own first `LoadSeg` (`loadseg:
  Boot Mount first hunk $01AE48 (monitor segments / add-symbol-file FILE
  0x1AE48)`, then `SIGTRAP`) — the mechanism works. (Didn't chase it all the
  way to our own `C:serialtest` binary loading — AROS LoadSegs several of
  its own components first — but the stop/report/detach/reattach loop the
  maintainer describes is real and scriptable.)
- **Issue 3, split in two:**
  - The maintainer attributed the *"not in executable format"* failure to a
    BFD-build difference (macOS bebbo build has the `amiga` BFD target,
    "the Linux container build evidently doesn't"). **This did not
    reproduce here** — today, both `/opt/amiga/bin/m68k-amigaos-gdb`
    (macOS/local) and the container's `m68k-amigaos-gdb` (Linux, same
    `13.0.50.251124-132852-git` version) load a `-g` hunk exe fine (`file
    type amiga`, `break main` resolves a real address) — tested against
    both a trivial program and an actual AmiAuth harness build. Likely the
    `stefanreinauer/amiga-gcc:latest` image has been rebuilt since the
    original report; the specific mechanism doesn't matter much now, since
    it no longer blocks anything either way.
  - The **`-g` gives hunk symbols only, no DWARF, so `break main` resolves
    but `list`/`next`/source-level stepping don't** — this part **is still
    true and reproduces identically** on both builds
    (`list main` → `No symbol table is loaded. Use the "file" command.`
    even right after `break main` succeeded). Real source-level debugging
    needs an ELF-with-DWARF sibling of the running hunk binary — an
    `m68k-elf` toolchain build of the same source plus `elf2hunk`, loaded
    alongside the running program via `add-symbol-file` — not attempted
    here; not needed for anything this project currently debugs this way
    (the serial/`RawPutChar` route below covers assertion-style testing,
    and `break main`-level breakpoints plus register/memory reads cover
    interactive poking).

**Net effect on this project:** symbol-level GDB debugging of a
guest-launched program is no longer a dead end for function-level
breakpoints and register/memory inspection — connect with the **local**
`/opt/amiga/bin/m68k-amigaos-gdb` (not the Docker container's — simpler
networking, and was the actual root cause here even though the BFD
concern didn't independently reproduce), `monitor loadseg-break` +
`continue` to catch a program's load without manual addresses, then
`detach`/`file`/`target remote` again to bind breakpoints. Source-level
stepping/`print`/`list` would need the `m68k-elf` + `elf2hunk` +
`add-symbol-file` route, untried so far. The findings below are kept as
the original record of what was tried and observed.

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
