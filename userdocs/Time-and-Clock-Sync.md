# Time and Clock Sync

TOTP codes are computed from the current **UTC time**, and must be right to
within roughly ±30 seconds or every code is rejected. On classic Amigas this is
the real engineering problem:

- Many machines have **no battery-backed clock** (stock A500/A600/A1200) and
  boot to the wrong date entirely.
- Machines that do have an RTC conventionally keep **local wall-clock time**,
  not UTC — so even a perfectly set clock is hours off UTC.
- AmigaOS has no reliable native timezone concept before 3.2, and only a
  partial one after.

AmiAuth solves this with a **layered model** that degrades gracefully from a
networked machine down to a floppy-booted A500 — and it **never touches your
system clock**; it computes a correction and uses corrected time internally.

## The layers

Highest-confidence source wins. Each maps to a trust colour, shown as the LED
in the GUI and reported by the CLI `CLOCK` command:

| Priority | Source | Colour | Meaning |
|----------|--------|--------|---------|
| 1 | **SNTP sync** over the network | 🟢 Green | Offset measured against an internet time server — trustworthy |
| 2 | **Locale timezone** (`locale.library`) | 🟠 Amber | A sensible first guess from your Locale prefs — unverified |
| 3 | **Explicit UTC offset** you set | 🟠 Amber | Trusted as much as you trust your own clock |
| 4 | **Manual nudge** (± seconds, by eye) | 🟠 Amber | Hand-tuned |
| 5 | Nothing | 🔴 Red | Codes are being generated from an unverified clock — they may be wrong |

**Green** means codes should be accepted first try. **Amber** means AmiAuth is
applying a correction it cannot verify — usually fine, but watch out for DST
(below). **Red** means nothing has confirmed or corrected the clock at all.

## SNTP sync (green)

If a TCP/IP stack is running (`bsdsocket.library` present — AmiTCP, Roadshow,
Miami, an emulator's bsdsocket, …), AmiAuth can measure your clock's offset
with a single small UDP exchange against an NTP server. Zero configuration.

- **GUI:** performs one SNTP sync automatically at startup, so a resident
  commodity has verified (green) time for its whole session with no
  configuration. The server can be chosen with the `TIMESERVER` tooltype (see
  [Commodity and Tooltypes](Commodity-and-Tooltypes.md)); offline it fails quietly and falls back to the
  stored offset.
- **CLI:** run `SYNC` (see [CLI Reference](CLI-Reference.md)). In both cases the measured
  offset and the server are saved to `ENVARC:AmiAuth/`, so subsequent
  `GET`/`CODE` calls use the corrected time even after the stack goes down.
- The sync does **not** set your system clock; it only records the offset.
- **Green means freshly verified.** The green state applies to the session in
  which the sync happened; a *stored* offset loaded on a later run is honestly
  reported as amber, because AmiAuth can no longer vouch for it — your clock
  may have drifted since. The corrected time is still applied either way;
  re-run `SYNC` (or restart the GUI) to re-verify.

If you have no network, the amber layers below cover you.

## Locale timezone (amber, automatic)

If you have set your timezone in the **Locale** (or TimeZone) Prefs, AmiAuth
uses that offset as an automatic first guess — an offline machine with correct
local time and a configured locale "just works" with no setup.

Two caveats, which is why this is amber, not green:

- **No automatic daylight-saving.** The classic locale offset is fixed
  year-round, so it can be an hour wrong in summer — far outside TOTP's
  tolerance. If your codes stop working when the clocks change, this is why:
  run `SYNC`, or set an explicit offset.
- It assumes your Amiga clock holds **local time** (the convention). If you
  deliberately keep your clock on UTC, set an explicit offset of 0 instead.

## Explicit offset and manual nudge (amber)

With no network and no locale setting, tell AmiAuth the offset yourself. Two
ways, both amber, both persisted to `ENVARC:AmiAuth/` across reboots:

- **Set an absolute offset** if you already know it — `AmiAuth OFFSET
  <seconds>` (see [CLI Reference](CLI-Reference.md)) replaces whatever offset is currently
  stored. Positive means your Amiga's clock is *behind* UTC (add seconds to
  correct it); negative means it's *ahead*.
- **Nudge the existing offset** by a relative amount instead, when you don't
  know the exact figure but can tell the codes are off by roughly how much:
  - **CLI:** `AmiAuth NUDGE <±seconds>` adds (or subtracts) from the current
    offset — unlike `OFFSET`, which replaces it, `NUDGE` composes with
    whatever's already stored.
  - **GUI:** the **`-10s`** / **`+10s`** buttons flanking the clock-status
    line do the same thing, one click per ten seconds, with the status text
    updating immediately so you can watch the correction land.

**Worked example — dialling in an offline machine by eye:** you have no
network and no Locale timezone set, but you do have a phone (or another
device) showing the correct code for the same account. Your Amiga's clock
reads local time and you roughly know your timezone, but not to the second:

    AmiAuth OFFSET 7200        # a rough first guess: 2 hours behind UTC
    AmiAuth GET GitHub         # 148213 — compare against your phone's code

If your phone shows a *different* code, the guess is off by more than the
±30s window. Since TOTP codes step every 30 seconds, an adjacent wrong code
usually means you're roughly one period off — nudge and recheck:

    AmiAuth NUDGE 30           # try 30s later
    AmiAuth GET GitHub         # 519402 — matches the phone: done

`NUDGE` is the right tool here specifically *because* it's relative — you
don't need to re-derive "7200 + 30 = 7230" by hand, and each attempt is
independent of the last. In the GUI, watching the fuelgauge/countdown while
clicking `+10s`/`-10s` a few times against a known-good code achieves the
same thing without leaving the window. Once the code matches, you're within
tolerance — no need to chase the offset to the exact second.

## Checking the current state

- **CLI:** `AmiAuth CLOCK` reports the corrected time, the active source and
  the trust state.
- **GUI:** the LED shows green/amber/red at all times — see the [GUI Guide](GUI-Guide.md).

## Practical advice

| Your setup | What to do |
|------------|-----------|
| Networked machine or emulator | Nothing — run `SYNC` once (or let the GUI sync); re-run after big clock changes |
| Offline, RTC on local time, Locale configured | Nothing — amber locale guess applies automatically; mind DST changes |
| Offline, no RTC (clock wrong at boot) | Set the clock at boot (even roughly by hand), then `OFFSET`/nudge until codes work |
| Clock kept on UTC | Set an explicit offset of 0 |

A wrong code at a website is almost always a **time problem**, not a secret
problem — check `CLOCK` first. See [Troubleshooting and FAQ](Troubleshooting-and-FAQ.md).
