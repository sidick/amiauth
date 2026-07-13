# Time resolution

TOTP needs UTC, within roughly ±30s. This is the genuine engineering problem on
AmigaOS, and AmiAuth solves it with a layered model that degrades gracefully from
a networked machine down to a floppy-booted A500.

## Why it's hard on AmigaOS

- Many machines have **no battery-backed RTC** (stock A1200/A600/A500) and boot
  to the wrong date.
- Those that do keep an RTC conventionally hold **local wall-clock time**, not
  UTC (like classic Windows, unlike Unix). So the C library's `time()` returns
  **local** time on a real Amiga — to feed TOTP we must *remove* the local
  timezone to get UTC. (On the host build `time()` is already UTC, so the offset
  is zero and everything below is a no-op.)
- AmigaOS has **no native timezone concept pre-3.2**, and only a partial one
  after, so there is often no reliable UTC metadata to lean on.

## Layered resolution

Highest-confidence source wins; each maps to a status colour the UI shows
(the "make the failure mode visible" mitigation):

1. **SNTP** — *green (`CLOCK_SYNCED`)*. A single UDP exchange over bsdsocket when
   a TCP/IP stack is up; compute the offset and use corrected time without
   touching the system clock. Zero configuration.
2. **locale.library timezone** — *amber (`CLOCK_MANUAL`)*. A smart **default** for
   the offline case (see below).
3. **Explicit user offset** — *amber*. The user states their clock's offset from
   UTC.
4. **Manual nudge** — *amber*. A ±seconds control, synced by eye against the
   countdown bar.
5. **Nothing** — *red (`CLOCK_UNVERIFIED`)*.

## locale.library as a first-guess offset (Phase 4)

If the user has set their timezone in Locale/TimeZone prefs, `struct Locale`
(from `OpenLocale(NULL)`) exposes `loc_GMTOffset` — the offset **in minutes**.
Since the Amiga clock is local time, this is exactly a ready-made UTC offset: the
front-end can seed `clock_set_offset()` from it at startup with **zero user
interaction**, so an offline user whose locale is configured "just works".

It is a **guess, not a trusted source**, and must be presented as amber and be
trivially overridable:

- **No automatic DST.** The classic `loc_GMTOffset` is a fixed value that does
  not shift for summer time — so it can be an hour wrong by season, and an hour
  is far outside TOTP's ±30s window. (OS 3.2+ has better TZ handling; this fixed
  offset is the broad-compat mechanism.)
- **Assumes a local-time RTC.** If a user has deliberately set their clock to
  UTC, applying the locale offset would double-correct.
- **Often unset** (0 = GMT), in which case it simply contributes nothing.

SNTP overrides it whenever available; the nudge corrects it otherwise.

> **Implementation footgun:** the sign convention of `loc_GMTOffset` (minutes west
> of GMT, and the local→UTC direction) is a classic Amiga gotcha. Nail it down
> against the autodocs and an Amiberry test rather than from memory.

## Portable core vs Amiga front-end

- **Portable (`clock.c`, host-tested):** the offset/status model
  (`clock_init/now_utc/set_offset/nudge`), and the SNTP packet build/parse +
  offset computation (`clock_ntp_build_request` / `clock_ntp_parse_response` /
  `clock_apply_offset`). Corrected time is served by `clock_now_utc` with no
  system-clock side effects.
- **Amiga front-end (Phase 4):** the bsdsocket UDP transport (`clock_sntp_sync`),
  the `locale.library` query above, offset/status **persistence** (`ENVARC:`),
  and the green/amber/red **display**.
