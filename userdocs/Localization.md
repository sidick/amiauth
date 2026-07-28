# Localization

AmiAuth reads its user-visible text through AmigaOS's standard
**locale.library catalog** system (OS 2.1+), so a community translation can
be dropped in as a single file — no rebuild, no reinstall of AmiAuth itself.

**Status:** this is an early, in-progress rollout. A representative slice of
strings (CLI status/error messages, a few GUI requesters, the account list's
column headers) already goes through the catalog system; most of the
application's text is still English-only pending a fuller migration. If your
language isn't fully covered yet, that's why — check back in a future
release, or see [Building from Source](Building-from-Source.md) if you'd
like to help.

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

## Baseline

Locale.library catalogs require AmigaOS 2.1 (V38) — one step above AmiAuth's
own CLI baseline of 2.04 (V37). This is handled the same way as every other
optional library in AmiAuth: absence of locale.library itself (a plain 2.04
system) is not an error, just English text, same as an absent or
non-matching catalog.
