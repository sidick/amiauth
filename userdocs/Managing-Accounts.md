# Managing Accounts

An AmiAuth *account* is one 2FA enrolment: a secret key plus its metadata
(issuer, label, digits, period or counter). The vault holds up to **64**
accounts. This page covers getting them in, changing them, and getting them
out.

## What a service gives you at enrolment

When you enable 2FA on a website, it shows a **QR code**, and almost always —
behind a link like *"can't scan the code?"* — either a **Base32 secret** (e.g.
`JBSW Y3DP EHPK 3PXP`) or a full **`otpauth://` URI**:

    otpauth://totp/GitHub:you@example.com?secret=JBSWY3DPEHPK3PXP&issuer=GitHub

AmiAuth accepts all three forms.

## Adding an account

### From an `otpauth://` URI

- **CLI:** `AmiAuth ADD "otpauth://…"` — quote it, URIs contain `?` and `&`.
- **GUI:** *Account → Add (type URI/secret)…*, or copy the URI to the clipboard and
  use *Account → Add from clipboard*.

The URI's issuer, label, secret, algorithm, digits, period (TOTP) or counter
(HOTP) are all imported. AmiAuth supports SHA-1 (the near-universal default),
SHA-256 and SHA-512 (`algorithm=SHA256`/`SHA512` in the URI — rare, but some
services offer it), 6 or 8 digits, and any period. A URI naming an algorithm
AmiAuth doesn't implement is rejected rather than silently imported as SHA-1.

### From a bare Base32 secret

Some services show the raw secret instead of (or alongside) a QR code —
usually behind a *"can't scan?"* or *"enter this text code"* link, e.g.
GitHub's setup key. That string **is** the Base32 secret already; nothing
needs decoding or converting. Add it directly, choosing your own issuer and
account name:

    AmiAuth ADD JGB5H6F66YNZKVM4 ISSUER GitHub LABEL you@example.com

In the GUI, *Account → Add (type URI/secret)…* accepts the same bare secret;
a follow-up requester asks for the issuer and label.

A bare secret gets the defaults that GitHub (and almost every other service)
actually issues: TOTP, SHA-1, 6 digits, 30 seconds. Anything different —
HOTP, 8 digits, a custom period — still needs the full `otpauth://` URI
form, where those appear as query parameters.

Base32 handling is tolerant: spaces, lower case and `=` padding are all fine.

### From a QR image (GUI)

If you can get the enrolment QR as an image file — a screenshot from the
enrolment page, a saved PNG, a photo — AmiAuth can decode it on the Amiga:

- *Account → Add from QR image…* opens a file requester
  (pattern `#?.(png|jpg|jpeg|gif|iff|ilbm|lbm|bmp)`), **or**
- **drag the image file's icon onto the AmiAuth window** from Workbench.

Requirements: `datatypes.library` v39+ with a picture datatype for the format
(OS 3.1+; PNG/JPEG datatypes are a standard install on 3.1.4/3.2). The decode
runs on-Amiga using the bundled quirc decoder — the image never leaves your
machine. A busy pointer shows while decoding; clear, screen-sized screenshots
decode most reliably.

## Listing and generating codes

See [CLI Reference](CLI-Reference.md) (`LIST`, `GET`) and the [GUI Guide](GUI-Guide.md) (live list,
double-click to copy). Account name matching in the CLI is case-insensitive
against the label, the issuer, or `issuer:label`.

## Editing

*Account → Edit selected…* in the GUI lets you change the **issuer**, **label**
(required), **digits** (6–8) and **period** (1–86400 seconds).

The **secret and the account type (TOTP/HOTP) are deliberately not editable** —
if a service issues you a new secret, remove the account and re-add the new
enrolment. There is no CLI edit command; edit in the GUI, or remove and
re-add.

## Removing

- **CLI:** `AmiAuth REMOVE <account>`
- **GUI:** *Account → Remove selected…* (asks for confirmation)

Removal is immediate and there is no undo. If you might need the account
again, make sure you still have the service's recovery codes — or a vault
backup ([Vault and Passphrases](Vault-and-Passphrases.md)) — before removing it.

## TOTP vs HOTP

- **TOTP** (the overwhelmingly common kind): the code is derived from the
  current time and changes every period (usually 30 s). Needs a correct clock
  — see [Time and Clock Sync](Time-and-Clock-Sync.md).
- **HOTP**: the code is derived from a per-account **counter**. Every `GET`
  advances the counter and saves the vault, producing the next code in the
  sequence. If the
  server and AmiAuth drift apart (codes rejected), most services re-sync after
  you enter one or two consecutive codes.

## Practical notes

- **One secret, two devices:** at enrolment you can add the same secret to
  AmiAuth *and* a phone app — both generate the same codes. A good hedge while
  you decide how much you trust a 30-year-old computer with your logins.
- **Keep recovery codes** offline regardless. An authenticator — any
  authenticator — can be lost.
- **Migrating from another app:** if you can export `otpauth://` URIs (or
  per-account QR images) from your current authenticator, both import paths
  above work. Bulk `otpauth-migration://` exports (Google Authenticator's
  batch format) are not supported in v1 — export accounts individually.
