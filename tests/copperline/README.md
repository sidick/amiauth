# Copperline headless on-target smoke test

Validates the AmiAuth **core** running on a real AmigaOS/68000 runtime, headless
and fully automated — a CI-friendly complement to interactive Amiberry testing.

[Copperline](https://github.com/LinuxJedi/Copperline) is a Rust Amiga emulator
with a strong automation story: TOML config, host-directory boot volumes, warp
speed, and headless serial/screenshot capture.

## What it does

1. Boots a stock **A500 / 68000 / 1 MB** (AmiAuth's baseline target) from the
   `sys/` host directory. Copperline builds a throwaway RAM FFS volume from it,
   so `sys/` acts as a read-only boot disk — no ADF/HDF packaging.
2. `sys/S/Startup-Sequence` runs `C:serialtest`, which computes the RFC 4226
   Appendix D HOTP vectors (secret `"12345678901234567890"`, counters 0–9) and
   emits them over the serial port via **`exec/RawPutChar`**.
3. `run.sh` boots windowless (`--benchmark-until`, which renders no window and
   exits at a fixed emulated time) with `--serial stdout`, captures the output,
   and asserts all ten vectors. Counters are fixed, so no clock/RTC is involved.

This exercises HMAC-SHA1, the big-endian counter packing, and dynamic truncation
**as compiled for the 68000** — the on-target risk host tests can't cover.

## Run it

```sh
make copperline-smoke     # builds serialtest (docker), boots, checks vectors
```

Overridable env: `KICK=` (512 KiB Kickstart 3.1 ROM), `SERIALTEST_M68K=`,
`BENCH=` (emulated seconds to run). Defaults point at the local Amiberry ROM dir.

## CI status

Not yet wired into CI — run it locally for now. It's flagged as a future
addition in `.github/workflows/ci.yml`. Enabling it needs two things on the
runner: a Copperline build (Rust; not packaged) and a 512 KiB Kickstart 3.1
ROM, which is non-redistributable and would have to be provisioned from a
licensed source (e.g. Cloanto/Amiga Forever) via a repo secret.

## Files

| File | Purpose |
|------|---------|
| `serialtest.c` | On-target harness: RFC 4226 vectors → `RawPutChar` |
| `machine.toml` | A500/68000 machine; boots the `sys/` host dir under warp |
| `sys/S/Startup-Sequence` | One line: `serialtest` |
| `sys/C/` | Makefile stages `serialtest` here (gitignored) |
| `run.sh` | Boots Copperline, captures serial, verifies vectors |

## Why RawPutChar (what the spike ruled out)

The original plan was a symbol-level **GDB remote** assertion (Copperline's
standout automation feature). It doesn't work with the installed toolchain:

- `m68k-amigaos-gdb` (13.0.50) can't parse bebbo's `-amiga-debug-hunk`
  executables (`file format not recognized`, even forcing `-b amiga`).
- The toolchain has **no ELF output target** (`m68k-amigaos-ld` supports
  `amiga a.out-amiga srec … ihex`), so there's no DWARF executable to feed gdb,
  and gdb crashes on the ELF *object* files (`internal-error: dwarf from non elf
  file`).
- Copperline 0.11's stub implements no `monitor` command, so the documented
  `monitor segments` relocation fallback isn't available.

gdb still works as a raw remote client (registers, memory, absolute-address
breakpoints), just not as a symbol debugger here.

The next plan, capturing the CLI's stdout via a mounted **AUX:** console, also
failed: in a minimal (non-Workbench) boot the Aux-Handler can't be opened
(`Echo "x" >AUX:` → *"unable to open redirection file"*). It needs a fuller
Workbench environment.

**`RawPutChar`** sidesteps all of it — it writes straight to the Paula serial
registers via the ROM debug path, needing no serial.device handler, no `Mount`,
and no Workbench files. Copperline's `--serial stdout` captures it. The boot
volume is just the harness binary plus a one-line Startup-Sequence.

## Verdict on Copperline for automation

Positive: headless boot, TOML/CLI config, host-directory boot volumes, warp,
and serial/screenshot capture all work cleanly and made this test easy to stand
up. The GDB-remote *symbol* path — the headline pitch — is blocked by the
toolchain, not Copperline itself, but the `monitor` gap is a real 0.11 limitation
to revisit. For **core-level** on-target validation this is a solid, low-friction
CI lane. A CLI-I/O-level test would need either a full-Workbench boot (for AUX:)
or a small serial-debug output mode in the CLI.
