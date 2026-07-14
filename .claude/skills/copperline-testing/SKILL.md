---
name: copperline-testing
description: >
  Headless on-target testing of AmigaOS/m68k binaries under the Copperline
  emulator — booting from a host directory, capturing guest output over serial,
  running windowless in CI, and the bundled-AROS ROM. Use when running/booting
  Amiga code under Copperline, adding a copperline test, debugging why a guest
  program produces no output, or deciding how to capture output on-target.
  Encodes what the AmiAuth spike discovered, including approaches that DON'T work
  (AUX: redirection in a minimal boot; symbol-level GDB) so they aren't retried.
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

**1. Boot from a host directory (no ADF/HDF).** Copperline mounts a host dir as
a throwaway RAM FFS volume. `SYS:`/`C:`/`S:`/`L:`/`DEVS:` auto-assign to it, so a
minimal tree (`C/<binary>` + `S/Startup-Sequence`) boots straight into your
program. In `machine.toml`:

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
- **Deterministic assertions.** No RTC on an A500, so avoid time-dependent
  output. For TOTP, a huge `period` forces the counter to 0 on any pre-2033
  clock; better, test the time-independent HOTP path directly against the RFC
  4226 vectors (counters 0-9).
- **`--screenshot-after SECS PATH`** boots, grabs a PNG, and exits — use it to
  see the guest console when diagnosing a boot/redirection failure.

## What does NOT work (don't re-derive these)

- **AUX: output redirection fails in a minimal boot.** `Mount AUX:` + `… >AUX:`
  gives *"unable to open redirection file"* because the Aux-Handler can't open
  without a fuller Workbench environment. `echo >AUX:` works on a full WB boot
  but not a stripped one. `SER:` can't take text at all. → Use `RawPutChar`, or
  boot a real Workbench (HDF/floppy) if you must capture a normal program's
  stdout.
- **Symbol-level GDB debugging is blocked** with the bebbo `amiga-gcc` toolchain
  (`stefanreinauer/amiga-gcc`): its `m68k-amigaos-gdb` can't read the
  `-amiga-debug-hunk` executables, the toolchain has no ELF output target, and
  gdb crashes on the ELF `.o` DWARF. Copperline 0.11's `--gdb` stub also has no
  `monitor` command, so the `monitor segments` relocation fallback is missing.
  GDB still works as a raw remote client (registers/memory/absolute-address
  breakpoints). Filed upstream: LinuxJedi/Copperline#181. Details in
  `tests/copperline/GDB_FEEDBACK.md`.

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

- Kickstart ROMs: `~/Documents/Amiberry/Roms/` (e.g. `amiga-os-310-a600.rom`,
  a 512 KiB 3.1). Workbench/Storage ADFs: `~/Documents/Amiberry/ADF/`.
- `xdftool` (extract files from ADFs): `~/src/amitools/bin/xdftool`.
- m68k cross-build: `make m68k-docker` (the `stefanreinauer/amiga-gcc` container;
  no local toolchain needed).

## Useful flags

`--serial stdout|off|tcp|pty` · `--benchmark-until SECS` (no window, exit) ·
`--screenshot-after SECS PATH` (grab + exit) · `--gdb ADDR` (remote stub) ·
`--noaudio` · `--model/--cpu/--chip/--fast/--slow` · positional `ROM` overrides
the config's `rom`. Boot config is TOML (`--config FILE`).
