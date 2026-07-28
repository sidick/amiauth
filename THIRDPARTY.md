# Third-party code

AmiAuth ships zero *mandatory runtime* dependencies: everything it needs is
either an AmigaOS-standard shared library or compiled into the binary from
source in this repository. The vendored third-party source is:

## quirc — QR-code recognition library

- **Location:** [`src/qr/`](src/qr/) (`quirc.h`, `quirc_internal.h`, `quirc.c`,
  `decode.c`, `identify.c`, `version_db.c`)
- **Original upstream:** https://github.com/dlbeer/quirc
- **Fork tracked for this project:** https://github.com/sidick/quirc — the
  original isn't actively maintained, so any future fixes needed inside
  quirc itself (as opposed to AmiAuth's own wrapper code) land here rather
  than waiting on upstream. Pinned to commit
  `198897c987f8b1ff055ef65a6bb0ad55f1dbb216` (2026-07-28) — memory-safety
  and undefined-behaviour fixes in `quirc_extract`/`quirc_flip`/
  `quirc_resize` and the `identify.c` geometry code, all reachable from a
  malformed QR image (see #109); before that, an untracked, unpinned manual
  snapshot.
- **Author:** Daniel Beer `<dlbeer@gmail.com>`
- **License:** ISC (permissive; compatible with AmiAuth's BSD 2-Clause)

Used by the GUI to decode an `otpauth://` enrolment QR from an image file. It has
no dependencies of its own (standard C only) and is built with
`-DQUIRC_FLOAT_TYPE=float` so it runs on a plain 68000 (no FPU). The wrapper in
[`src/qr/qr.c`](src/qr/qr.c) (AmiAuth's own code) is the only interface the rest
of the program uses.

Its ISC licence text is preserved verbatim in every vendored source file.

## qrcodegen — QR-code generation library

- **Location:** [`src/qr/`](src/qr/) (`qrcodegen.h`, `qrcodegen.c`)
- **Upstream:** https://github.com/nayuki/QR-Code-generator (the `c/`
  subdirectory), pinned to commit `2c9044de6b049ca25cb3cd1649ed7e27aa055138`.
  Actively maintained, so unlike quirc no fork is tracked for this one.
- **Author:** Project Nayuki
- **License:** MIT (permissive; compatible with AmiAuth's BSD 2-Clause)

Used by the GUI and CLI to render an account's `otpauth://` URI as a QR code
for export (#45) — the encode-side counterpart to quirc's decode. It has no
dependencies of its own (standard C only), does no dynamic allocation (every
buffer is caller-supplied), and is integer-only (no floating point), so it
needs no special build flags for the plain-68000 target. The wrapper in
[`src/qr/qrencode.c`](src/qr/qrencode.c) (AmiAuth's own code) is the only
interface the rest of the program uses.

Its MIT licence text is preserved verbatim in every vendored source file.

**`assert()` is deliberately left active (no `-DNDEBUG` anywhere in the
build).** Unlike quirc, which decodes untrusted external images and had its
own bounds-checking hardened (#109/#110), qrcodegen only ever encodes
already-validated internal data (an `otp_account` built by `otpauth_build`),
so its ~50 `assert()`s are pure internal-invariant checks — a "should never
happen" one firing means the encoder has already produced an inconsistent
QR. `abort()`ing loudly on that is safer than `-DNDEBUG` silently disabling
the check and shipping a corrupted or wrong QR encoding of an account
secret. This is a considered choice, not an oversight — if `-DNDEBUG` is
ever added elsewhere in the build, keep it off this file specifically.
