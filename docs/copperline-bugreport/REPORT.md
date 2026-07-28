# Copperline: `[[filesys]]` host-directory boot hangs on 68000/68010 + bundled AROS (regression 0.11.0 → 0.12.0)

**Status: root-caused and fixed upstream, not yet released** — see
[Resolution](#resolution) below.

## Summary

A machine using a `[[filesys]]` host-directory mount as the boot volume
(A500 profile, plain 68000, no explicit ROM → bundled AROS) boots and runs
correctly on Copperline **0.11.0**, but hangs indefinitely on **0.12.0** and
**0.13.0** — AROS never gets past its own boot splash animation, never
reaches the mounted volume, and the guest program never runs. `--serial
stdout` produces no output beyond `romtaginit done`, even after 40 emulated
seconds (about 5x longer than 0.11.0 needs to boot *and* run the guest
program to completion).

This was found via [AmiAuth](https://github.com/sidick/amiauth)'s on-target
test suite (`tests/copperline/run.sh`, `make copperline-smoke`), which boots
a minimal AmigaOS environment from a host directory to run a native m68k
test binary. CI still pins Copperline 0.11.0
(`COPPERLINE_VERSION: "0.11.0"` in `.github/workflows/ci.yml`) for unrelated
reasons, which is why this hadn't been noticed before — 0.12.0/0.13.0 were
only tried locally while checking the 0.13.0 release notes for relevant
changes.

## Environment

- macOS (Homebrew `copperline` formula) for the 0.13.0 reproduction; the
  `Copperline-<ver>-macos-universal.dmg` release asset for the 0.11.0/0.12.0
  bisection (both run the bundled `copperline` binary inside `Copperline.app`
  directly, not the GUI).
- Versions tested: 0.11.0 (works), 0.12.0 (hangs), 0.13.0 (hangs).

## Config (`machine.toml`)

```toml
[machine]
profile = "A500"

[cpu]
model = "68000"

[memory]
chip = "512K"
slow = "512K"

[floppy]
drives = 1

[[filesys]]
path = "sys"
volume = "AmiAuthTest"
bootpri = 10

[emulation]
power_on = true
warp_speed = "max"
```

`sys/` is a small host directory: `sys/C/serialtest` (a native m68k
executable) and `sys/S/Startup-Sequence` containing a single line
(`serialtest`) that runs it. No ROM is passed on the command line, so
Copperline boots its bundled AROS Kickstart replacement.

## Command

```sh
copperline --config machine.toml --noaudio --serial stdout --benchmark-until 40
```

## Expected (0.11.0 behaviour)

AROS boots, mounts the `[[filesys]]` host directory as `AmiAuthTest:`
(`bootpri = 10`, outranking the empty floppy), runs
`Startup-Sequence`, `serialtest` executes and emits its results over serial
via `exec/RawPutChar`, then the emulator exits at the `--benchmark-until`
deadline. Total wall-clock: **~8.8s**.

Full serial capture (`0.11.0-serial.log`):

```
[...ROM/autoconfig boot log...]
romtaginit
romtaginit done
BEGIN
HOTP0=755224
HOTP1=287082
HOTP2=359152
HOTP3=969429
HOTP4=338314
HOTP5=254676
HOTP6=287922
HOTP7=162583
HOTP8=399871
HOTP9=520489
DRBG=be491355307bb821bf72d7f115d91156
END
```

## Actual (0.12.0 and 0.13.0 behaviour)

Boot log is **byte-identical** up through `romtaginit done`, then nothing
further is ever emitted. The process runs the full 40 emulated seconds
(wall clock also ~8.8s — `warp_speed = "max"`, so wall time doesn't grow
even though it never finishes booting) and exits without the guest program
ever running.

Full serial capture (`0.12.0-serial.log` / `0.13.0-serial.log`, identical):

```
[...ROM/autoconfig boot log...]
romtaginit
romtaginit done
```
(nothing else — no BEGIN, no HOTP vectors, no END)

A `--screenshot-after 39` at the very end of the 40s window
(`0.13.0-boot-stuck.png`, attached) shows AROS still animating its own boot
logo ("AROS" wordmark + eyes) — alive and rendering, not crashed, just stuck
before ever reaching a state where it looks at the mounted volume.

## Isolation

- Removing `[floppy] drives = 1` (leaving only `[[filesys]]`) does not
  change the outcome — still hangs on 0.12.0. So the floppy section isn't
  the trigger.
- Substituting a real, licensed Kickstart 2.04 ROM (512 KiB) for the
  bundled AROS still hangs — with the real ROM the guest doesn't even
  render its own "Insert disk" boot screen (0 bitplane fetches at 8s
  emulated per the `emu stats` log line), vs. a plain `[machine] profile =
  "A500"` config with **no** `[[filesys]]` section, which renders the
  normal Kickstart 2.0 splash screen correctly and promptly. So the trigger
  is `[[filesys]]` itself, independent of ROM choice (bundled AROS vs. real
  Kickstart) and independent of the floppy section.
- This lines up with the 0.12.0 release notes' description of `[[filesys]]`
  changing from a discard-on-exit RAM snapshot to a live host-directory
  pass-through (`.uaem` sidecar files for protection bits/comments/
  datestamps, new `readonly` flag to opt back into the old behaviour) —
  that refactor is the most likely place this regressed, though I haven't
  bisected inside the 0.12.0 development history itself.

## Files in this report

- `0.11.0-serial.log`, `0.12.0-serial.log`, `0.13.0-serial.log` — full
  stdout (serial) capture for each version, identical config/command.
- `0.11.0-stderr.log`, `0.12.0-stderr.log`, `0.13.0-stderr.log` — matching
  stderr (Copperline's own log output) for each run.
- `0.13.0-boot-stuck.png` — screenshot at 39s emulated time under 0.13.0,
  showing AROS still on its own boot animation, never having reached the
  mounted filesystem.

## Suggested next steps (for the Copperline maintainer)

1. Reproduce with the config above (bundled AROS needs no ROM asset).
2. `git bisect` between the 0.11.0 and 0.12.0 tags, most likely inside
   whatever commit(s) reworked `[[filesys]]` from a RAM-snapshot mount to
   the live host-directory pass-through (see "Isolation" above).
3. Check whether the new `.uaem`-sidecar / live-write-back machinery is
   blocking on something during volume validation/mount (a lock, an fsync,
   a synchronous host filesystem call) that never returns during
   AmigaDOS's disk-change / DOS-list probing, which would explain "stuck
   before ever reaching the mounted volume" rather than a crash.

## Resolution

Root cause found and fixed upstream: [CopperlineHQ/Copperline#312](https://github.com/CopperlineHQ/Copperline/pull/312)
(open, not yet in a release as of 0.13.0).

The hang is **CPU-model-specific to 68000/68010**, not the `[[filesys]]`
machinery itself. `FilesysBoard::write()` only fired the `DIAG_DOORBELL`
(and `REG_DOSPKT`/`REG_MSGPORT`) MMIO doorbell on a single 32-bit access.
A 68020+'s 32-bit data bus satisfies that in one write, but a 68000/68010
has a 16-bit external bus, so the core's `write_move_dest_68000` correctly
splits every `move.l` destination write into two word bus cycles — real
hardware behaviour. The doorbell guard never saw the `size == 4` case on
those CPUs, so the guest ROM's `move.l a0,DIAG_DOORBELL(a0)` in the
DiagPoint stub never reached `FilesysBoard::diag_entry`, expansion init
never ran, and the guest hung forever waiting on a host filesystem that
never mounted. A regression from commit `497845c`, which switched the
services board from a size-independent A-line HLE trap to plain MMIO
registers. Confirmed independent of ROM choice (bundled AROS or a real
Kickstart, matching the "Isolation" section above) and of `[machine]
profile` — reproduced identically on 0.13.0 with both the A500/68000
config above and the real-ROM `pbkdf2-bench` config.

**68020+ is not affected** — its 32-bit bus never splits the write, so
`[[filesys]]` boots normally regardless of Copperline version. Confirmed
locally (2026-07-28): the A500/68000 config above hangs identically on
brew 0.13.0; switching only `[cpu] model` to `"68020"` boots to completion
in the same config. A local build of Copperline off the `#312` branch also
boots the unmodified 68000 config to completion, confirming the fix.

AmiAuth's own tests (`tests/copperline/run.sh`, `tests/copperline/bench.sh`)
stay on 68000 rather than adopting the 68020 workaround, since the whole
point of those tests is validating real 68000/68010 bus-cycle behaviour
(this bug is a good example of why: it's real hardware-accurate 68000
behaviour in the CPU core that the filesys board's doorbell logic didn't
handle). They instead support a `COPPERLINE=` env var to point at a fixed
build until a release ships `#312`.
