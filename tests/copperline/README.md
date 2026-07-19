# Copperline headless on-target smoke test

Validates the AmiAuth **core** running on a real AmigaOS/68000 runtime, headless
and fully automated — a CI-friendly complement to interactive Amiberry testing.

[Copperline](https://github.com/LinuxJedi/Copperline) is a Rust Amiga emulator
with a strong automation story: TOML config, host-directory boot volumes, warp
speed, and headless serial/screenshot capture.

## What it does

1. Boots a stock **A500 / 68000 / 1 MB** (AmiAuth's baseline target) from the
   `sys/` host directory (`[[filesys]]`, no ADF/HDF packaging). As of
   Copperline 0.12 this mount is actually **writable** back to the host
   directory — it just happens that nothing in this test writes anything, so
   `sys/` is read-only in effect, not by the mount's design. (Older versions
   of this doc said the mount itself was throwaway/read-only; that's no
   longer true in general — see the `copperline-testing` skill.)
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

Overridable env: `KICK=` (a 512 KiB Kickstart ROM; if unset, boots the bundled
AROS), `SERIALTEST_M68K=`, `BENCH=` (emulated seconds to run).

## CI status

Wired in as the `copperline-smoke` job in `.github/workflows/ci.yml`, gated to
PRs and pushes to `main`. It downloads a pinned prebuilt Copperline AppImage and
boots its bundled **AROS** Kickstart replacement — redistributable, so no
licensed ROM or secret is needed. Locally, `make copperline-smoke` uses your
installed `copperline` and also defaults to AROS (set `KICK=` for a real
Kickstart).

CI is currently pinned to Copperline **0.11.0** (`COPPERLINE_VERSION` in
`ci.yml`); this project's day-to-day local/interactive Copperline use has
since moved to **0.12.0** (Homebrew) for the features noted below. Nothing in
*this* test needs 0.12 — worth bumping the CI pin at some point mainly to stay
current and to pick up 0.12's faster AROS boot, not because anything here is
broken on 0.11.

## Files

| File | Purpose |
|------|---------|
| `serialtest.c` | On-target harness: RFC 4226 vectors → `RawPutChar` |
| `machine.toml` | A500/68000 machine; boots the `sys/` host dir under warp |
| `sys/S/Startup-Sequence` | One line: `serialtest` |
| `sys/C/` | Makefile stages `serialtest` here (gitignored) |
| `run.sh` | Boots Copperline, captures serial, verifies vectors |

## Why RawPutChar (what the spike ruled out — and what's since changed)

The original plan for this specific test was a symbol-level **GDB remote**
assertion (Copperline's standout automation feature). It didn't pan out for a
fixed-vector CI check, for reasons investigated in depth and written up in
[`GDB_FEEDBACK.md`](GDB_FEEDBACK.md) (filed upstream as
[LinuxJedi/Copperline#181](https://github.com/LinuxJedi/Copperline/issues/181)):

- The original attempt ran gdb **inside the amiga-gcc Docker container**,
  whose `localhost` isn't the host's — `target remote` silently never
  connected, and the resulting `"monitor" command not supported by this
  target"` was gdb's own dummy-target response, not a real Copperline
  limitation. Connecting from a **local** gdb instead works immediately.
- **As of Copperline 0.12, `monitor loadseg-break` genuinely solves** the
  original "no way to relocate a program `LoadSeg`'d after connect" problem —
  verified working end-to-end in this repo's follow-up testing. Symbol-level
  GDB debugging of a guest-launched program (function-level breakpoints,
  register/memory inspection, no manual addresses) is a real option now; see
  `GDB_FEEDBACK.md`'s Resolution section and the `copperline-testing` skill
  for the working recipe (use the local `/opt/amiga` gdb, not the container's).
- **Still true:** `-g` only gives hunk-level function symbols, no DWARF, so
  `break main` works but source-line stepping (`list`/`next`) doesn't. Real
  source-level debugging would need an `m68k-elf` + `elf2hunk` ELF sibling —
  untried, and not needed for anything this project currently debugs this way.

None of that changes the right call for *this* test, though: GDB is an
interactive/scripted-session tool, and this is a fixed-vector pass/fail
assertion in CI — `RawPutChar` remains simpler and faster for that job.

The next plan tried, capturing the CLI's stdout via a mounted **AUX:**
console, also failed and stays a genuine dead end: in a minimal
(non-Workbench) boot the Aux-Handler can't be opened (`Echo "x" >AUX:` →
*"unable to open redirection file"*). It needs a fuller Workbench environment.

**`RawPutChar`** sidesteps all of it — it writes straight to the Paula serial
registers via the ROM debug path, needing no serial.device handler, no `Mount`,
and no Workbench files. Copperline's `--serial stdout` captures it. The boot
volume is just the harness binary plus a one-line Startup-Sequence.

## Verdict on Copperline for automation

Positive: headless boot, TOML/CLI config, host-directory boot volumes, warp,
and serial/screenshot capture all work cleanly and made this test easy to stand
up. As of 0.12, the emulator also gained writable `[[filesys]]` mounts, a
JSON-RPC control protocol, deterministic `--rtc-time` clock seeding, and a
working GDB symbol-debugging path (`monitor loadseg-break`) — see the
`copperline-testing` skill for all of it, kept up to date as the project's
Copperline usage grows beyond this one test (GUI first-run/unlock flows are
now driven with `--script`/scripted input, for instance). For **core-level**
on-target validation, `RawPutChar` + `--benchmark-until` remains the
lowest-friction CI lane. A CLI-I/O-level test would still need either a
full-Workbench boot (for AUX:) or a small serial-debug output mode in the CLI.
