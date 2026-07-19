# AmiAuth — project instructions

## Documentation lives on the wiki

**User-facing documentation is on the GitHub wiki**
(https://github.com/sidick/amiauth/wiki), not in this repo. It is a separate
git repo — `git@github.com:sidick/amiauth.wiki.git` — conventionally cloned as
a sibling of this checkout (`../amiauth.wiki`, branch `master`).

**Whenever you change user-visible behaviour, update the affected wiki pages
as part of the same piece of work** — commit and push the wiki clone. That
includes: CLI commands/arguments and their output, GUI gadgets/menus/dialogs,
tooltypes, `ENVARC:AmiAuth/` settings, vault behaviour, clock/SNTP behaviour,
error messages worth documenting, and OS/library requirements. If a change is
purely internal, no wiki edit is needed — but say so explicitly.

The wiki is the single source of truth for user docs; a planned AmigaGuide
(`.guide`) version will be generated from it. `docs/` in this repo holds
developer-facing design notes only (architecture, vault format, clock design,
security model rationale) — keep those in sync too when the design itself
changes.

## Key project rules

- **Baseline target is a plain 68000** (`-m68000`), AmigaOS 2.04+ for the CLI,
  3.0+ for the GUI. Requiring 020+ needs a very good reason.
- **Zero mandatory runtime dependencies** — this constrains the shipped Amiga
  binaries, not build/test tooling (OpenSSL, Docker etc. are fine in CI).
- The vault **passphrase is interactive-only** — never via CLI args, env vars,
  files, or the commodity port.
- The **on-disk vault format is frozen** (`docs/VAULT_FORMAT.md`).
- Amiga shell stack is ~4 KB — keep large objects off the stack.
- Local Amiga builds: `make m68k-docker` / `make gui-docker` (amiga-gcc
  container). Tests: `make test`, `make smoke`; headless GUI: `make gui-smoke`.
