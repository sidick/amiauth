# Third-party code

AmiAuth ships zero *mandatory runtime* dependencies: everything it needs is
either an AmigaOS-standard shared library or compiled into the binary from
source in this repository. The only vendored third-party source is:

## quirc — QR-code recognition library

- **Location:** [`src/qr/`](src/qr/) (`quirc.h`, `quirc_internal.h`, `quirc.c`,
  `decode.c`, `identify.c`, `version_db.c`)
- **Original upstream:** https://github.com/dlbeer/quirc
- **Fork tracked for this project:** https://github.com/sidick/quirc — the
  original isn't actively maintained, so any future fixes needed inside
  quirc itself (as opposed to AmiAuth's own wrapper code) land here rather
  than waiting on upstream.
- **Author:** Daniel Beer `<dlbeer@gmail.com>`
- **License:** ISC (permissive; compatible with AmiAuth's BSD 2-Clause)

Used by the GUI to decode an `otpauth://` enrolment QR from an image file. It has
no dependencies of its own (standard C only) and is built with
`-DQUIRC_FLOAT_TYPE=float` so it runs on a plain 68000 (no FPU). The wrapper in
[`src/qr/qr.c`](src/qr/qr.c) (AmiAuth's own code) is the only interface the rest
of the program uses.

Its ISC licence text is preserved verbatim in every vendored source file.
