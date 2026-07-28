# Localization

AmiAuth reads its user-visible text through AmigaOS's standard
**locale.library catalog** system (OS 2.1+), so a community translation can
be dropped in as a single file — no rebuild, no reinstall of AmiAuth itself.

**Status:** every user-visible string in both the CLI and the GUI goes
through the catalog system — prompts, error messages, menus, buttons, form
labels, and requester text. A handful of things are deliberately left in
English only: CLI subcommand keywords, `ReadArgs` field names and
[ARexx port](ARexx-Port.md) command keywords (the scripting protocol, not
prose — see below), file paths, and the CLI `usage()` command-list table and
`ADD`'s bare-secret help text (too tightly interleaved with literal
command/argument syntax to translate safely). If a string you'd expect to be
translatable isn't, it likely falls into one of those categories.

## Installing a translation

Drop a `<language>.catalog` file — named after your language, e.g.
`deutsch.catalog` — into:

    LOCALE:Catalogs/<language>/AmiAuth.catalog

(`LOCALE:` is normally assigned to `SYS:Locale`.) AmiAuth picks it up
automatically next time it starts, based on your system's default language
(**Locale** in Prefs). No translation installed, or none for your language,
is not an error — AmiAuth just uses its built-in English text.

## Contributing a translation

AmiAuth's strings are hand-off from the *catalog description* file,
[`locale/AmiAuth.cd`](https://github.com/sidick/amiauth/blob/main/locale/AmiAuth.cd)
in the source repository — the standard `catcomp`/`FlexCat` format used
across the AmigaOS ecosystem, so existing translator tooling works
unmodified:

1. Get [FlexCat](https://github.com/adtools/flexcat) (or another
   `.cd`/`.ct`-aware tool, e.g. CatEditor).
2. Generate a starting `.ct` translation file from `AmiAuth.cd`, or edit an
   existing one for your language.
3. Translate each string, keeping any `%s`/`%lu`-style placeholders in
   place and in the same order as the English original — they're filled in
   with values like account names or error details at runtime.
4. Compile it to a `.catalog` file (`FlexCat AmiAuth.cd yourlanguage.ct
   CATALOG AmiAuth.catalog`) and test it by installing it as described
   above.
5. Open a pull request with your `.ct` file, or attach it to an issue, at
   [github.com/sidick/amiauth](https://github.com/sidick/amiauth).

**Do not translate:** CLI subcommand keywords (`CODE`, `GET`, `INIT`, ...),
`ReadArgs` template field names, or [ARexx port](ARexx-Port.md) command
keywords (`GETCODE`, `STATUS`, ...) — none of these are in `AmiAuth.cd` in
the first place, since they're the scripting protocol, not user-facing
prose, and must stay identical across every locale so scripts keep working
everywhere.

**GUI button mnemonics:** the account-list toolbar's button labels (`_Add`,
`_Edit`, `_Remove`, `_Copy`, `_D -10s`, `_U +10s`) carry a keyboard shortcut
as a leading underscore before one letter — AmiAuth reads that letter back
out of the string at runtime, from a **fixed position**, not by searching
for the underscore. That means the `_` must be the very *first* character
of the string, immediately followed by an ASCII letter — you can pick
whichever translated word you like for the button (the shortcut is always
that word's first letter, not a letter of your choosing further in), but
you can't mark a letter in the middle of a word the way the CLI re-key
prompts below let you. Getting this wrong (underscore missing, doubled, or
not at the very start) silently breaks or mismarks that button's shortcut
rather than erroring — `tools/check_catalog.py` (`make check-catalog`)
checks for it.

**CLI re-key prompts:** the three interactive re-key confirmations (offered
after unlocking a vault on much faster/slower hardware than it was tuned
for) work the same way — the accepted answer is read back out of the
translated prompt itself, not hardcoded. `Strengthen it now? [(y)es/(N)o/
ne(v)er ask here]`'s 1st `"(x)"` is the accept letter and its 3rd is the
"never ask again" letter (the 2nd, "no", is never checked); `[(y)es/(N)o]`'s
1st `"(x)"` is its accept letter; and `Type 'yes' to confirm:`'s accepted
answer is whatever word sits between the first pair of `'` quotes. Translate
freely — the letters don't have to be `y`/`N`/`v`, and don't have to be the
first letter of the translated word — just keep exactly one `"(x)"` per
choice (same left-to-right order, accept and never-ask letters distinct from
each other) and one `'...'`-quoted word. If the accept and never-ask letters
do end up the same, AmiAuth degrades safely rather than misreading your
answer: the never-ask shortcut (which writes a persisted preference) simply
becomes unreachable, it never fires on a "yes" by mistake.

## Draft translations awaiting review

[`locale/drafts/`](https://github.com/sidick/amiauth/tree/main/locale/drafts)
in the source repository holds **machine-generated first-pass translations**
— a starting point for a fluent speaker to review and correct, not something
to install as-is. They exist so contributing a translation can start from
"fix this" rather than "write 145 strings from a blank file." Currently
drafted: German
([`deutsch.ct`](https://github.com/sidick/amiauth/blob/main/locale/drafts/deutsch.ct)),
French
([`francais.ct`](https://github.com/sidick/amiauth/blob/main/locale/drafts/francais.ct)),
Italian
([`italiano.ct`](https://github.com/sidick/amiauth/blob/main/locale/drafts/italiano.ct)),
and Polish
([`polski.ct`](https://github.com/sidick/amiauth/blob/main/locale/drafts/polski.ct)).

None of these are wired into any build — they can't end up in a release, or
get picked up by AmigaOS's catalog search, just by existing in the repo.
Every draft has passed `tools/check_catalog.py`'s structural checks
(placeholders, button mnemonics, re-key prompt markers, file encoding — see
above), so what's left to fix is wording, tone, and register, not mechanics.
Polish specifically drops its usual diacritics (`a`/`c`/`e`/`l`/`n`/`o`/`s`/`z`
instead of the accented forms) — not a stylistic choice, but because the
catalog format's `##codeset` field only supports the Latin-1 character set,
which contains just one of the nine Polish diacritics.

To help: pick a draft, read it against `locale/AmiAuth.cd` (the English
source of truth), fix what needs fixing, build and test it locally (see
[Contributing a translation](#contributing-a-translation) above), then open
a pull request moving it out of `locale/drafts/`.

## Baseline

Locale.library catalogs require AmigaOS 2.1 (V38) — one step above AmiAuth's
own CLI baseline of 2.04 (V37). This is handled the same way as every other
optional library in AmiAuth: absence of locale.library itself (a plain 2.04
system) is not an error, just English text, same as an absent or
non-matching catalog.
