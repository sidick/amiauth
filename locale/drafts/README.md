# Draft translations - NOT reviewed, NOT installed by anything

Everything in this directory is a **machine-generated first pass** at a
translation of `locale/AmiAuth.cd`, produced by an AI coding assistant with
no native-speaker review. Nothing here is built, tested, or referenced by
any `make` target - that's deliberate, so a draft can never end up in a
release or get picked up by AmigaOS's catalog search just by existing in
the tree.

**Do not install a `.ct` file from here as-is.** Treat it the same as any
other unreviewed contribution: it needs a native (or at least fluent)
speaker to check it, especially the security-relevant prompts (unencrypted
storage, no-RNG, re-key strengthen/weaken) where a subtly wrong translation
could mislead someone about what they're agreeing to.

## Promoting a draft to a real translation

1. A fluent speaker reviews the `.ct` file line by line against
   `locale/AmiAuth.cd` (the English source of truth) - fixing wording,
   register/tone, and any button-mnemonic collisions (see the underscore
   note in `AmiAuth.cd`'s header).
2. Build and smoke-test it locally:
   ```
   FlexCat ../AmiAuth.cd yourlanguage.ct CATALOG AmiAuth.catalog
   ```
   then drop the result at `LOCALE:Catalogs/<language>/AmiAuth.catalog` and
   exercise the CLI/GUI to see it in practice (see
   [Localization](../../userdocs/Localization.md#installing-a-translation)).
3. Move the reviewed `.ct` out of `drafts/` and open a pull request per the
   normal [translator workflow](../../userdocs/Localization.md#contributing-a-translation).

## The three CLI re-key prompts are now fully localizable

`maybe_rekey()`'s three re-key confirmation prompts in `src/cli/main.c`
originally parsed the user's typed answer against hardcoded English
letters/words in C, found and fixed while drafting this translation:
`prompt_letter()`/`prompt_quoted_word()` (`src/cli/main.c`) now extract the
accepted letter(s)/word straight out of the *translated* prompt at runtime
- the same self-describing-string technique `src/gui/main.c`'s `LBL_*`
already uses for button mnemonics. See the convention documented in
`locale/AmiAuth.cd`'s header (search for "MSG_CLI_REKEY_STRENGTHEN_PROMPT"):
a translation is free to pick its own letters/word (they need not be
English `y`/`N`/`v`/`yes`, and a letter need not be the first letter of the
translated word) - just keep exactly one `"(x)"` per choice, in the same
left-to-right order, with the accept and never-ask letters distinct from
each other, and the confirm word inside `'...'` quotes. `deutsch.ct` below
already uses real German letters/word (`j`/`N`/`i`, `'ja'`) as a worked
example.
