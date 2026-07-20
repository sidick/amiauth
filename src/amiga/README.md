# Amiga front-end (Phase 4)

Amiga-specific code lives here, kept out of the portable `core/` so the host
test harness never needs OS headers:

- **GUI** — ClassAct/ReAction window: `listbrowser.gadget` account list, large
  code display, `fuelgauge.gadget` countdown, clipboard copy (clipboard.device,
  FTXT), clock-status indicator (green/amber/red).
- **Commodity shell** — `commodities.library`: CxBroker, hotkey filter,
  popup/hide window lifecycle, Exchange messages, auto-lock timer,
  WBStartup-friendly tooltypes.
- **SNTP** — the `bsdsocket` implementation of `clock_sntp_sync()` (declared in
  `core/clock.h`, stubbed in the host build).

See docs/ARCHITECTURE.md.
