---
name: copperline-testing
description: >
  Headless on-target testing of AmigaOS/m68k binaries under the Copperline
  emulator — booting from a host directory, capturing guest output over serial,
  running windowless in CI, and the bundled-AROS ROM. Use when running/booting
  Amiga code under Copperline, adding a copperline test, debugging why a guest
  program produces no output, deciding how to capture output on-target, or
  attaching gdb to a guest-launched program. Encodes what the AmiAuth spike
  discovered, including a dead end that stays dead (AUX: redirection in a
  minimal boot) and one that got resolved upstream (symbol-level GDB — now
  has a working recipe, see below; don't re-flag it as broken).
---

# Testing AmigaOS binaries under Copperline

[Copperline](https://github.com/LinuxJedi/Copperline) is a Rust Amiga emulator
with a strong headless-automation story. This skill captures the working
approach and the dead-ends found while building AmiAuth's on-target test.

## Start here

The working example lives in `tests/copperline/` — read it before changing
anything:

- `serialtest.c` — a harness that emits results over serial via `RawPutChar`
- `machine.toml` — A500/68000, boots the `sys/` host directory
- `sys/S/Startup-Sequence` — one line that runs the harness
- `run.sh` — boots windowless, captures serial, asserts
- `README.md` / `GDB_FEEDBACK.md` — rationale and the GDB findings

Run it: `make copperline-smoke` (builds the m68k harness in the amiga-gcc
container, then boots and checks). Local prereq: `brew install copperline`.

## The four things that make headless capture work

**1. Boot from a host directory (no ADF/HDF).** `[[filesys]]` mounts a host
dir as a live `HOSTFS<n>:` volume — as of **0.12, guest writes go straight
back to the host directory** (protection bits/comments/datestamps in
`.uaem` sidecars; `readonly = true` opts back into a write-protected export
if you want the old throwaway behaviour). `SYS:`/`C:`/`S:`/`L:`/`DEVS:`
auto-assign to it, so a minimal tree (`C/<binary>` + `S/Startup-Sequence`)
boots straight into your program. In `machine.toml`:

```toml
[[filesys]]
path = "sys"          # relative to the config file's dir
volume = "AmiAuthTest"
bootpri = 10          # outranks the (empty) floppy => this is the boot volume
```

**2. Run windowless + fixed-time exit.** `--benchmark-until SECS` runs with **no
window** and exits at emulated time SECS (self-terminating — no background
process/kill needed). Pair with `--serial stdout` (serial → host stdout; the
emulator's own logs go to stderr) and `--noaudio`.

```sh
copperline --config machine.toml --noaudio --serial stdout --benchmark-until 40 [ROM]
```

**3. Emit output with `exec/RawPutChar`, not a DOS handler.** This is the key
discovery. `RawPutChar` (exec LVO -516) writes straight to the Paula serial
registers via the ROM debug path — **no serial.device handler, no `Mount`, no
Workbench files** — so it works in the most minimal boot, and `--serial stdout`
captures it. Header-independent snippet (`-std=c99` needs `__asm__`, not `asm`):

```c
static void raw_put(char c)
{
    void *SysBase = *(void **)4UL;               /* ExecBase at absolute 4 */
    register long d0 __asm__("d0") = (unsigned char)c;
    register void *a6 __asm__("a6") = SysBase;
    __asm__ volatile("jsr -516(%%a6)" : : "r"(d0), "r"(a6)
                     : "d1", "a0", "a1", "cc", "memory");
}
```

**4. Boot the bundled AROS ROM — no licensed Kickstart needed.** Omit `rom` from
the config / pass no ROM arg → Copperline boots its redistributable AROS
Kickstart replacement. This is what makes CI possible (no ROM secret). AROS
boots **slower** than Kickstart, so give `--benchmark-until` more time (~40 emu
seconds). Override the AROS location with `COPPERLINE_AROS_DIR=<dir>` (dir with
`aros-amiga-m68k-rom.bin` + `-ext.bin`); the source tree / packaging keeps them
in `assets/aros`.

## Gotchas

- **Serial sends CRLF.** Strip `\r` before matching (`tr -d '\r'`), or an exact
  `grep '…=CODE$'` fails on the trailing carriage return.
- **Deterministic assertions.** No RTC on an A500 by default, so avoid
  time-dependent output unless you seed one. `--rtc-time "2026-01-01
  00:00:00"` (0.12+, `TIME` also accepts Unix seconds) **fits a battery
  clock even on a model that wouldn't otherwise have one** and seeds it,
  after which it ticks in emulated time; add `--rtc-frozen` to pin it
  exactly instead of ticking. This is the guest's battery-backed clock
  (`DateStamp()`/`time()`), not `timer.device` EClock (irrelevant here;
  that's what `make pbkdf2-bench` needs a real Kickstart for). With a
  seeded RTC the real TOTP path is testable deterministically, not just
  the time-independent HOTP path against the RFC 4226 vectors (counters
  0-9) — the previous workaround for having no clock at all.
- **`--screenshot-after SECS PATH`** boots, grabs a PNG, and exits — use it to
  see the guest console when diagnosing a boot/redirection failure.

## What does NOT work (don't re-derive these)

- **AUX: output redirection fails in a minimal boot.** `Mount AUX:` + `… >AUX:`
  gives *"unable to open redirection file"* because the Aux-Handler can't open
  without a fuller Workbench environment. `echo >AUX:` works on a full WB boot
  but not a stripped one. `SER:` can't take text at all. → Use `RawPutChar`, or
  boot a real Workbench (HDF/floppy) if you must capture a normal program's
  stdout.
- **Symbol-level GDB debugging — RESOLVED as of 0.12.0, kept here for
  context; use the recipe below instead of re-deriving.** The original 0.11 report
  (LinuxJedi/Copperline#181) turned out to be mostly a testing artifact, per
  the maintainer's reply and independent re-verification here (2026-07-19,
  full details in `tests/copperline/GDB_FEEDBACK.md`):
  - The `"monitor" command not supported by this target"` failure was gdb's
    **dummy-target** response to a `target remote` that never actually
    connected — the original gdb ran **inside the `stefanreinauer/amiga-gcc`
    Docker container**, whose `localhost` isn't the host's. Connect from
    **`/opt/amiga/bin/m68k-amigaos-gdb`** (this project's local toolchain
    copy) instead — no networking workaround needed, and `monitor
    help`/`loadseg-break`/`loadseg-list` all answer immediately.
  - **`monitor loadseg-break`** (new in 0.12.0) is the fix for "no way to
    relocate a program `LoadSeg`'d after connect": arm it, `continue`, and
    the stub stops the moment a new program's seglist installs, reporting
    the base address for `add-symbol-file` — no manual address-hunting, no
    `monitor segments` fallback needed. Verified working end-to-end here
    against a real boot.
  - `break main`-level (function-symbol) breakpoints work fine from a `-g`
    hunk exe on **both** the local and container gdb builds (the
    macOS-vs-Linux BFD gap the maintainer described didn't reproduce here —
    probably a since-rebuilt container image). But **`-g` alone is still
    hunk-symbols-only, no DWARF** — `list`/`next`/source-stepping fail
    (`No symbol table is loaded`) even right after a successful `break
    main`. Real source-level stepping needs an ELF-with-DWARF sibling built
    via an `m68k-elf` toolchain + `elf2hunk`, loaded alongside the running
    hunk binary with `add-symbol-file` — untried so far, not yet needed.

  **Recipe** (function-level breakpoints + register/memory poking, no
  manual addresses): connect with the local gdb, `monitor loadseg-break`,
  `continue` to the load event, `detach` (stub stays paused, doesn't exit),
  `file <the-now-known-binary>`, `target remote` again, set breakpoints,
  `continue`.

## CI (GitHub Actions)

Use the **prebuilt AppImage** release asset, not a from-source build (no Rust
toolchain, no build deps, and it bundles AROS):

```yaml
- run: |
    url="https://github.com/LinuxJedi/Copperline/releases/download/v${VER}/Copperline-${VER}-x86_64.AppImage"
    curl -fsSL "$url" -o copperline.AppImage
    chmod +x copperline.AppImage
    ./copperline.AppImage --appimage-extract >/dev/null   # extract: no FUSE on runners
    mkdir -p "$HOME/.local/bin"
    ln -sf "$PWD/squashfs-root/AppRun" "$HOME/.local/bin/copperline"
    echo "$HOME/.local/bin" >> "$GITHUB_PATH"
    aros_bin=$(find "$PWD/squashfs-root" -name aros-amiga-m68k-rom.bin | head -1)
    [ -n "$aros_bin" ] && echo "COPPERLINE_AROS_DIR=$(dirname "$aros_bin")" >> "$GITHUB_ENV"
```

- Install `mesa-vulkan-drivers` as insurance (software Vulkan) in case the
  emulator initialises wgpu at start-up.
- `--appimage-extract` avoids the FUSE requirement (GitHub runners lack it).
- The live job is `copperline-smoke` in `.github/workflows/ci.yml`.

## Local environment (this machine)

- `copperline` (Homebrew): **0.12.0** as of 2026-07 (`brew upgrade
  linuxjedi/copperline/copperline`; was 0.11.0 — see version-specific notes
  above tagged 0.12+).
- Kickstart ROMs: `~/Documents/Amiberry/Roms/` (e.g. `amiga-os-310-a600.rom`,
  a 512 KiB 3.1). Workbench/Storage ADFs: `~/Documents/Amiberry/ADF/`.
- `xdftool` (extract files from ADFs): `~/src/amitools/bin/xdftool`.
- m68k cross-build: `make m68k-docker` (the `stefanreinauer/amiga-gcc` container;
  no local toolchain needed).
- `/opt/amiga/bin/m68k-amigaos-gdb` also exists locally — **use this one, not
  the Docker container's gdb**, for the GDB workflow above: it's what worked
  in the confirmed recipe, and running gdb inside the container was the
  actual root cause of the original "monitor not supported" false alarm
  (container `localhost` ≠ host `localhost`, so `target remote` silently
  never connects). Opposite of the *build* guidance elsewhere — builds
  should use `make m68k-docker` for reproducibility, but a debugger needs a
  direct route to Copperline's TCP stub on the host.

## Useful flags

`--serial stdout|off|tcp|pty` · `--benchmark-until SECS` (no window, exit) ·
`--screenshot-after SECS PATH` (grab + exit) · `--gdb ADDR` (remote stub) ·
`--rtc-time TIME` / `--rtc-frozen` (seed/pin the guest clock, 0.12+) ·
`--noaudio` · `--model/--cpu/--chip/--fast/--slow` · positional `ROM` overrides
the config's `rom`. Boot config is TOML (`--config FILE`).

Debugging a guest that expects a register Copperline doesn't model: set
`[debug] log_unmapped = true` (optionally scoped to an address range) in the
config to log every CPU access nothing decodes (0.12+).

## Scripted input: driving interactive GUIs headlessly

Copperline CAN drive interactive flows — no Amiberry needed. Time-based input
directives, combinable with `--screenshot-after` (all verified working on the
AmiAuthGUI first-run flow, 2026-07):

- `--key-after SECS KEY MS` — press KEY at SECS, hold MS, release. Key names:
  `a`-`z`, `return`, `ctrl`, `lalt`, `lami`, `rami`, `f1`… Integer seconds.
- `--press-after SECS KEY` — one press/release.
- `--click-after SECS BTN MS`, `--mouse-after SECS DX DY` — mouse, but prefer
  keyboard: **system requesters answer LAmiga+V (leftmost gadget) / LAmiga+B
  (rightmost)**, and menu shortcuts are RAmiga+letter — no coordinates needed.
- `--script FILE` — the same directives, one per line, without the leading
  dashes (`key-after 52 lami 1500`); `#` comments allowed. `--record-input`
  or Cmd+Shift+R records a live session into this format.

Hold a modifier by overlapping windows: `key-after 52 lami 1500` +
`key-after 53 v 200` = LAmiga+V at t≈53. Emulation is deterministic, so the
same script hits the same frames — probe timings with one `--screenshot-after`
per run and then trust them.

**When fixed timestamps aren't enough** (branching on what's on screen,
adaptive retries, string.gadget text entry — which still can't be driven
reliably by scripted keys), reach for the **Control Protocol** instead
(0.12+): `--control ADDR` serves a headless machine over JSON-RPC (auth via
`--control-token`/`--control-info`); the `copperline-ctl` client — or a
script talking JSON-RPC 2.0 directly — can inject input, take screenshots,
save/load state, and subscribe to frame/serial/interrupt events live,
mid-session. `--control-gui ADDR` attaches the same server to a windowed run
for interactive debugging. Not yet used in this project; `docs/debugger/control.md`
in the Copperline repo is the reference. `--record-input PATH` (or a live
`--control` session) journals input into the same `.clscript` format
`--script` reads, so a working interactive run can be captured as a
regression script.

**`[ide]`/`[scsi]` directory mounts are still throwaway — this is different
from `[[filesys]]` above.** Giving `[ide] master = { path = ..., name = ... }`
(or `[scsi]`) a host directory builds an in-memory FFS **snapshot** from it
at boot; guest writes never reach the host dir, so a file "saved" in one boot
is gone on the next. This is what booting a cloned Workbench install for
GUI testing uses (`tests/gui/gui-smoke.sh` and the interactive first-run
scripts: `cp -Rc` the WB dir fresh, boot from `[ide]`, throw the clone away
after). For those, multi-phase interactive tests (create → relaunch →
verify) must still happen within ONE boot: run the program twice
*synchronously* from a script started in Startup-Sequence (`Execute` a file
with two launch lines; quit the first instance with a scripted RAmiga+Q, the
second then starts) — see `firstrun-drive.sh`-style harnesses.

If a test only needs a plain host-dir boot (no full Workbench/ReAction), switch
it to `[[filesys]]` instead of `[ide]`/`[scsi]` with a directory path — writes
persist across separate `copperline` invocations, so multi-phase tests no
longer need the synchronous-double-launch trick at all.
