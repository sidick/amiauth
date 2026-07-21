# Troubleshooting and FAQ

## Troubleshooting

### My codes are rejected by the website

Almost always a **clock problem**, not a secret problem. TOTP tolerates only
~±30 seconds of error.

1. Run `AmiAuth CLOCK`. Red (`unverified`) or a suspicious offset? Fix the
   time first: `AmiAuth SYNC` on a networked machine, otherwise see
   [Time and Clock Sync](Time-and-Clock-Sync.md).
2. Amber and it *used to* work? If your country just changed to/from daylight
   saving, a locale-derived offset is now an hour off — re-`SYNC` or adjust
   `OFFSET`.
3. Clock verified green and still rejected? Now check the secret: re-add the
   account from the service's enrolment page, and check the digits/period
   match what the service expects (6 digits / 30 s for almost everyone).
4. HOTP account? The counter may be out of step — most services re-sync if
   you submit the next one or two consecutive codes.

### `SYNC` fails

`SNTP sync failed (no TCP/IP stack, or no response from <server>)` means
`bsdsocket.library` wasn't found (no TCP/IP stack running) or the UDP exchange
timed out (offline, firewalled, or a dead server — try another:
`AmiAuth SYNC uk.pool.ntp.org`). SNTP needs outbound UDP port 123.

### "wrong passphrase or the file has been tampered with"

Exactly what it says, and the two cases are deliberately indistinguishable
(the file's integrity check covers everything). If you are sure of the
passphrase, the file is damaged — restore from a backup. And remember: there
is **no passphrase recovery** ([Vault and Passphrases](Vault-and-Passphrases.md)).

### The vault has "disappeared"

Usually the `PROGDIR:` gotcha: the executable was copied somewhere new (often
`SYS:WBStartup`), so the default `PROGDIR:AmiAuth.vault` now points at a
different drawer. AmiAuth records the vault's absolute path in
`ENVARC:AmiAuth/vault` at creation to prevent exactly this — check
`GetEnv AmiAuth/vault`, and see the WBStartup advice in [Installation](Installation.md).

### "this vault is encrypted; run from an interactive terminal"

You ran a vault command from a script or non-interactive context. The
passphrase can only be entered interactively, by design. For scripting, use an
always-unlocked vault or keep the GUI resident and let the CLI forward to it
([CLI Reference](CLI-Reference.md)).

### "the running GUI's vault is locked; unlock it there first"

The resident GUI owns the vault but has (auto-)locked it. Pop it up
(hotkey / `AmiAuth SHOW`), enter the passphrase, retry.

### The GUI won't start / items are ghosted

- Won't start at all: the ReAction classes (window, layout, listbrowser,
  fuelgauge, button) are missing — OS 3.0+ with ReAction/ClassAct is required.
  The CLI still works everywhere.
- *Add from QR image* ghosted: `datatypes.library` v39+ or `asl.library`
  missing (QR import needs OS 3.1+ and a picture datatype for the format).
- No menus: `gadtools.library` missing. Copy disabled: `iffparse.library`
  missing. Full table in the [GUI Guide](GUI-Guide.md).

### AmiAuth quits instantly without even asking for the passphrase

A normal quit (Project → Quit, the hotkey/Exchange Kill command, Ctrl-C) always
cleans up fully. But if a previous AmiAuth was killed abnormally or genuinely
crashed while running — not a normal quit — classic AmigaOS has very little
automatic resource tracking and no memory protection, and the commodity
broker AmiAuth registers with `commodities.library` can be left stuck. A new
launch sees that stuck broker, correctly assumes another AmiAuth must already
be resident, and quits immediately without opening a window or prompting for
a passphrase — deferring to an instance that's actually gone.

To confirm this is what's happening: open Exchange and check whether `AmiAuth`
is still listed there. If it is, and Exchange's own **Remove** doesn't clear
it either, the broker registration is genuinely stuck at the OS level, not
just a display glitch. **Reboot** — that's the only thing that reliably clears
it. There is no software fix for this on classic AmigaOS: once a task has been
killed outside its own control, it never gets the chance to unregister itself,
and nothing else on the system can force that cleanup on its behalf.

### The window opens but ignores clicks for a few seconds after unlocking

The automatic SNTP sync at startup blocks the GUI briefly: normally well
under a second, but with a TCP/IP stack running and the network down or the
DNS server unreachable it can take many seconds to give up. The window opens
first and stays visible the whole time (this used to look like a hang, with
no window at all until the sync timed out); the clock status line and LED
update the moment the sync finishes. If it bothers you regularly, take the
stack offline (the sync then skips instantly) or fix the name server.

### The hotkey does nothing

Another commodity may own the combination — check in Exchange, and change
AmiAuth's with the `CX_POPKEY` tooltype ([Commodity and Tooltypes](Commodity-and-Tooltypes.md)). If
AmiAuth isn't listed in Exchange at all, it isn't running (or
`commodities.library` is missing and it ran as a plain window).

### QR image won't decode

Use a clean, reasonably large screenshot or export of the QR (a sharp
screen-grab beats a photo). The image must be in a format your installed
picture datatypes can load — PNG and JPEG datatypes are standard on 3.1.4/3.2,
IFF everywhere. (AmiAuth automatically retries with an added white margin if
the QR image has none — some export tools crop right up to the code's edge,
which many decoders, including AmiAuth's, otherwise can't locate — so this
usually isn't the cause of a failure.) Failing that, use the `otpauth://` URI
shown behind the service's "can't scan?" link instead.

## FAQ

**Is it really safe to keep 2FA secrets on an Amiga?**
Read [Security Model](Security-Model.md) — it answers this honestly. Short version: the
encrypted vault protects the file at rest as well as your passphrase deserves;
a *running*, unlocked AmiAuth is readable by any program, because AmigaOS has
no memory protection. It is a convenience tool for classic hardware, not a
hardware security key — decide per account how much that matters.

**Why does AmiAuth need my clock to be right?**
TOTP codes *are* the time, cryptographically mixed with your secret. See
[Time and Clock Sync](Time-and-Clock-Sync.md).

**Does it work without a network?**
Yes — that's a design goal. Codes are generated entirely offline; the network
(SNTP) is only an optional way to verify the clock. A floppy-booted A500 with
a manually-set offset works.

**Which machines/OS versions are supported?**
CLI: any 68000+, AmigaOS 2.04+. GUI: OS 3.0+ with ReAction/ClassAct. See
[Installation](Installation.md).

**Can I use the same account on my phone and my Amiga?**
Yes — enrol the same secret in both (see [Managing Accounts](Managing-Accounts.md)). Both then
generate identical codes.

**Where is everything stored?**
Vault: `PROGDIR:AmiAuth.vault` (one file — copy it to back up). Settings:
`ENVARC:AmiAuth/`. Nothing else, anywhere. See [Vault and Passphrases](Vault-and-Passphrases.md) and
[Settings Reference](Settings-Reference.md).

**I forgot my passphrase. Now what?**
The data is unrecoverable — that is the point of the encryption. Re-enrol each
account using the services' recovery codes or a second enrolled device.

**Steam Guard? SHA-256 codes? Exporting QR codes?**
On the v2 candidate list, along with an ARexx port and translations. See the
[v2 milestone](https://github.com/sidick/amiauth/milestone/2).

**Why SHA-1? Isn't that broken?**
SHA-1's collision attacks do not affect HMAC-SHA1 as used by TOTP/HOTP (it
remains the algorithm virtually every service issues secrets for), nor the
vault's HMAC/PBKDF2 constructions. See [Security Model](Security-Model.md).

**Was this really written by an AI?**
Largely, yes — under human direction, review and on-hardware testing; it's
disclosed prominently because for a security tool you deserve to know. The
crypto is RFC-vector-tested and differentially fuzzed against OpenSSL in CI,
and the source is open — audit it rather than trusting anyone's word:
[Security Model](Security-Model.md).
