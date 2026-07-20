# QR test fixtures

`sample_gray.h` holds two QR codes as 8-bit greyscale byte arrays (one byte per
pixel, row-major), used by [`tests/test_qr.c`](../test_qr.c) to exercise the
decoder with no platform dependencies:

| array                  | size  | payload                                   |
|------------------------|-------|-------------------------------------------|
| `qr_otp_gray`          | 86×86 | `QR_OTP_URI` — an `otpauth://` TOTP URI   |
| `qr_hello_gray`        | 54×54 | `"HELLO"` — a valid QR that is *not* otpauth |
| `qr_noquietzone_gray`  | 74×74 | `QR_OTP_URI`, rendered with `border=0` — zero quiet zone, exercising `qr_decode_gray()`'s padded-retry fallback |

They are generated **once** (not at build time) and checked in, so the test tree
stays dependency-free.

## Regenerating

Needs Python `qrcode` (+ Pillow) and ImageMagick:

```sh
pip install "qrcode[pil]"

# otpauth fixture (adjust the URI as needed; keep it short to stay a small QR):
python - <<'PY'
import qrcode
uri = "otpauth://totp/AmiAuth:demo?secret=JBSWY3DPEHPK3PXP&issuer=AmiAuth"
qr = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_M,
                   box_size=2, border=3)
qr.add_data(uri); qr.make(fit=True)
qr.make_image(fill_color="black", back_color="white").convert("L").save("otp.png")
PY
magick otp.png -depth 8 gray:otp.gray     # -> raw greyscale bytes

# ...then emit the C arrays (W×H = magick identify) into sample_gray.h.
```

If you change `QR_OTP_URI`, update the expected string in both `sample_gray.h`
and the assertions in `tests/test_qr.c`.
