# AmiAuth — project instructions

## Documentation lives in userdocs/

**User-facing documentation is `userdocs/` in this repo** — published as a
versioned MkDocs Material site at https://sidick.github.io/amiauth/ (one
docs version per release plus a `latest` alias, deployed by
`.github/workflows/docs.yml` on release tags via `mike`), and converted to
the `AmiAuth.guide` shipped in the release archive
(`tools/docs2guide.py`, `make guide`). The old GitHub wiki is retired and
redirects here; don't edit it.

**Whenever you change user-visible behaviour, update the affected
`userdocs/` pages in the same PR.** That includes: CLI commands/arguments
and their output, GUI gadgets/menus/dialogs, tooltypes, `ENVARC:AmiAuth/`
settings, vault behaviour, clock/SNTP behaviour, error messages worth
documenting, and OS/library requirements. If a change is purely internal,
no docs edit is needed — but say so explicitly. Use standard Markdown links
between pages (`[GUI Guide](GUI-Guide.md)`); `mkdocs build --strict` in CI
validates them. Adding a page means updating the `nav` in `mkdocs.yml` and
`PAGES` in `tools/docs2guide.py`. Keep pages within docs2guide's Markdown
subset (headings, bold, inline/fenced code, pipe tables, lists,
blockquotes, links) so the AmigaGuide stays faithful.

`docs/` in this repo holds developer-facing design notes only
(architecture, vault format, clock design, security model rationale) — keep
those in sync too when the design itself changes.

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
