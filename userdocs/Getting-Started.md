# Getting Started

This walkthrough takes you from a freshly installed AmiAuth to a working login
code. It assumes you have followed [Installation](Installation.md).

## 1. Sort the clock out first

TOTP codes are computed from UTC time, so a wrong clock means rejected codes.
On a networked machine (or emulator) with a TCP/IP stack running:

    AmiAuth SYNC

reports and saves your clock's offset from true UTC. (The GUI does this sync
by itself at startup, so if you only use `AmiAuthGUI` on a networked machine
there is nothing to do.) Offline machine? See [Time and Clock Sync](Time-and-Clock-Sync.md) for the
offset options. You can check the state any time with:

    AmiAuth CLOCK

## 2. Create a vault

**From Workbench:** just double-click `AmiAuthGUI`. On first launch it offers
to create the vault right there — choose a master passphrase (and confirm it),
or leave it empty for an *always-unlocked* vault (after an explicit warning).
No Shell needed.

**From the Shell:**

    AmiAuth INIT

asks the same question. **Press Enter on an empty passphrase** to create an
*always-unlocked* (unencrypted) vault — convenient for a private machine or
scripting, but read [Security Model](Security-Model.md) first: an always-unlocked vault has no
at-rest protection.

Either way the vault is created as `PROGDIR:AmiAuth.vault` — in AmiAuth's own
drawer — and its location is remembered in `ENVARC:AmiAuth/`.

## 3. Add an account

When you enable 2FA on a website, it shows a QR code and (usually behind a
"can't scan?" link) a Base32 secret or an `otpauth://` URI. Any of these work:

**From the URI** (CLI or GUI — note the quotes, URIs contain special characters):

    AmiAuth ADD "otpauth://totp/GitHub:you@example.com?secret=JBSWY3DPEHPK3PXP&issuer=GitHub"

**From a QR image** (GUI): save the enrolment QR as a PNG/JPEG/GIF/IFF, then
use *Account → Add from QR image…* or drag the image file onto the AmiAuth
window. See [Managing Accounts](Managing-Accounts.md).

**From a bare secret** (the raw "setup key" some services show): supply the
name it doesn't carry:

    AmiAuth ADD JBSWY3DPEHPK3PXP ISSUER GitHub LABEL you@example.com

## 4. Get a code

    AmiAuth LIST
    AmiAuth GET GitHub

`GET` prints the current 6-digit code and how many seconds it remains valid.
Account matching is case-insensitive against the label, the issuer, or
`issuer:label`.

Or start `AmiAuthGUI` for the live view: every account with its current code
and countdown, a large display for the selected account, and double-click to
copy a code to the clipboard.

## 5. (Recommended) Run the GUI as a background commodity

Set `AmiAuthGUI` to start at boot (see [Installation](Installation.md)). It then sits in
Exchange, pops up on **Ctrl-Alt-A**, and holds the unlocked vault — so you
enter your passphrase once per session, and even Shell commands like
`AmiAuth GET GitHub` are answered by the resident GUI without re-prompting.

## One-off codes without a vault

To generate a code directly from a secret, no vault involved:

    AmiAuth CODE JBSWY3DPEHPK3PXP

## Where to next

- [CLI Reference](CLI-Reference.md) — every command in detail.
- [GUI Guide](GUI-Guide.md) — the window, menus and clipboard behaviour.
- [Vault and Passphrases](Vault-and-Passphrases.md) — encryption, backups, moving machines.
