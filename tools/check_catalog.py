#!/usr/bin/env python3
"""check_catalog.py - structural sanity checks for locale/AmiAuth.cd and any
.ct translation, catching the class of bug a translator can introduce without
ever reading src/cli/main.c or src/gui/main.c: a dropped/reordered printf
placeholder, a missing/misplaced button mnemonic, two shortcuts sharing one
letter, or a file accidentally saved as UTF-8 instead of the Latin-1 FlexCat
and AmigaOS expect.

This is NOT a translation-quality check - wording/tone/register still needs a
human fluent in the language. It only checks structure: the same mechanical
things that would otherwise surface as a garbled string, a silently-dead
keyboard shortcut, or (for the CLI re-key prompts) a mismatch between what a
prompt displays and what it actually accepts.

Usage:
    tools/check_catalog.py locale/AmiAuth.cd [translation.ct ...]
    tools/check_catalog.py --fix-encoding FILE [FILE ...]

The first form's first argument is always the .cd (the English source of
truth, and the baseline every .ct is checked against); every following
argument is a .ct translation. Exits 1 if any FAIL-level issue is found in
any file; WARN-level issues (see MSG_GUI_BTN_*/MSG_CLI_REKEY_STRENGTHEN_PROMPT
below) are printed but don't fail the build - the code has a documented
runtime fallback for them.

The second form re-saves the given file(s) as Latin-1 if check_encoding()
would flag them - use this instead of a hand-rolled one-liner (see
fix_encoding()'s docstring for why a naive one is dangerous).

Parsing note: this assumes THIS repo's own .cd/.ct convention - one line of
body text per entry, terminated by a lone ';' line (see locale/AmiAuth.cd) -
not FlexCat's full multi-line grammar, which this project doesn't use.
"""
import re
import sys

NAME_RE = re.compile(r'^(MSG_[A-Za-z0-9_]+)\b')
PLACEHOLDER_RE = re.compile(r'%[-+ #0]*[0-9]*l?[a-zA-Z]')
PAREN_LETTER_RE = re.compile(r'\(([^()])\)')

# Entries whose body must carry exactly one '_<letter>' keyboard mnemonic,
# with all such letters distinct within one file (no runtime collision
# protection for these - the FIRST match in the GUI's if/else-if VANILLAKEY
# chain silently wins, and the loser's shortcut becomes keyboard-unreachable).
MNEMONIC_PREFIX = 'MSG_GUI_BTN_'

# The three CLI re-key prompts (src/cli/main.c's prompt_letter()/
# prompt_quoted_word()) and their structural requirements.
REKEY_STRENGTHEN = 'MSG_CLI_REKEY_STRENGTHEN_PROMPT'   # >=3 "(x)"; 1st=yes, 3rd=never
REKEY_LOWER = 'MSG_CLI_REKEY_LOWER_PROMPT'             # >=1 "(x)"; 1st=yes
REKEY_CONFIRM = 'MSG_CLI_CONFIRM_YES_PROMPT'           # one 'word' to type back


def parse_entries(path):
    """.cd/.ct -> {NAME: body_text}, in this repo's one-line-body convention."""
    with open(path, encoding='latin-1') as f:
        lines = f.read().splitlines()
    entries = {}
    i = 0
    while i < len(lines):
        stripped = lines[i].strip()
        if not stripped or stripped.startswith(';') or stripped.startswith('#'):
            i += 1
            continue
        m = NAME_RE.match(stripped)
        if not m:
            i += 1
            continue
        name = m.group(1)
        i += 1
        if i >= len(lines):
            break
        entries[name] = lines[i]
        i += 1
        while i < len(lines) and lines[i].strip() != ';':
            i += 1
        i += 1  # skip the ';' terminator
    return entries


def check_encoding(label, path, fails):
    """FlexCat and AmigaOS expect Latin-1 (ISO-8859-1), not UTF-8 - an editor
    that defaults to UTF-8 will happily save e.g. 'ü' as the two bytes 0xC3
    0xBC instead of the single Latin-1 byte 0xFC, which FlexCat either
    rejects outright (for the .cd's built-in text) or silently mis-renders
    (for a .ct translation - AmigaOS displays each byte as its own Latin-1
    character, turning one accented letter into two garbled ones).

    Heuristic: real Latin-1 prose with high-bit bytes essentially never also
    decodes as valid UTF-8 (an accented letter is usually followed by an
    ordinary ASCII letter, which breaks UTF-8's continuation-byte rules) -
    so "the raw bytes contain a high bit AND decode cleanly as UTF-8" is a
    reliable (not mathematically airtight, but reliable in practice for
    natural-language text) signal that the file is accidentally UTF-8."""
    with open(path, 'rb') as f:
        raw = f.read()
    if not any(b >= 0x80 for b in raw):
        return   # pure ASCII: no Latin-1-vs-UTF-8 question to ask
    try:
        raw.decode('utf-8')
    except UnicodeDecodeError:
        return   # doesn't parse as UTF-8 - consistent with real Latin-1
    fails.append(f"{label}: file has non-ASCII bytes that also decode as "
                 f"valid UTF-8 - it's almost certainly saved as UTF-8, not "
                 f"the Latin-1/ISO-8859-1 FlexCat and AmigaOS expect. Fix "
                 f"with: tools/check_catalog.py --fix-encoding {path}")


def fix_encoding(path):
    """Re-save `path` as Latin-1 if it's currently (accidentally) UTF-8.
    Reads the WHOLE file into memory before opening it for writing - opening
    the same path in 'wb' mode truncates it immediately, so a naive
    read-and-write-in-one-expression one-liner destroys the file before the
    read ever happens. Don't inline this as a shell one-liner; call this."""
    with open(path, encoding='utf-8') as f:
        text = f.read()
    with open(path, 'wb') as f:
        f.write(text.encode('latin-1'))


def placeholders(text):
    return PLACEHOLDER_RE.findall(text)


def paren_letters(text):
    return [m.group(1) for m in PAREN_LETTER_RE.finditer(text)]


def quoted_word(text):
    start = text.find("'")
    if start < 0:
        return None
    end = text.find("'", start + 1)
    if end <= start + 1:
        return None
    return text[start + 1:end]


def check_mnemonics(label, entries, fails, warns):
    seen = {}
    for name, text in entries.items():
        if not name.startswith(MNEMONIC_PREFIX):
            continue
        marks = [i for i, c in enumerate(text) if c == '_']
        if len(marks) != 1:
            fails.append(f"{label}: {name} has {len(marks)} '_' markers "
                         f"(need exactly 1): {text!r}")
            continue
        pos = marks[0]
        # src/gui/main.c's WMHI_VANILLAKEY handler reads LBL_ADD[1] etc. - a
        # HARDCODED index, not "find the underscore" - so the '_' MUST be
        # the very first character (index 0) or the wrong letter (whatever
        # happens to sit at index 1) gets matched instead, silently.
        if pos != 0:
            fails.append(f"{label}: {name}'s '_' is at position {pos}, not the "
                         f"start of the string - the code always reads index 1 "
                         f"as the shortcut letter regardless of where '_' "
                         f"actually is, so this silently marks the wrong "
                         f"letter (or none): {text!r}")
            continue
        if pos + 1 >= len(text) or not text[pos + 1].isalpha():
            fails.append(f"{label}: {name}'s '_' isn't followed by a letter: {text!r}")
            continue
        letter = text[pos + 1].lower()
        if letter in seen:
            fails.append(f"{label}: {name} and {seen[letter]} both use mnemonic "
                         f"'{letter}' - one becomes keyboard-unreachable "
                         f"(first match wins in the VANILLAKEY handler)")
        else:
            seen[letter] = name


def check_rekey_prompts(label, entries, fails, warns):
    if REKEY_STRENGTHEN in entries:
        text = entries[REKEY_STRENGTHEN]
        letters = paren_letters(text)
        if len(letters) < 3:
            fails.append(f"{label}: {REKEY_STRENGTHEN} has {len(letters)} \"(x)\" "
                         f"markers, need >=3 (accept=1st, never-ask=3rd): {text!r}")
        else:
            yes_c, never_c = letters[0].lower(), letters[2].lower()
            if yes_c == never_c:
                warns.append(f"{label}: {REKEY_STRENGTHEN}'s accept and never-ask "
                             f"letters are both '{yes_c}' - runtime fallback makes "
                             f"'never ask again' unreachable this run (safe, but "
                             f"the shortcut is lost); distinct letters preferred")
    if REKEY_LOWER in entries:
        text = entries[REKEY_LOWER]
        letters = paren_letters(text)
        if len(letters) < 1:
            fails.append(f"{label}: {REKEY_LOWER} has no \"(x)\" accept marker: {text!r}")
    if REKEY_CONFIRM in entries:
        text = entries[REKEY_CONFIRM]
        if quoted_word(text) is None:
            fails.append(f"{label}: {REKEY_CONFIRM} has no '...'-quoted confirm "
                         f"word: {text!r}")


def check_placeholders(label, source, translation, fails):
    for name, src_text in source.items():
        if name not in translation:
            continue   # untranslated entry: falls back to English, not an error
        src_ph = placeholders(src_text)
        tr_ph = placeholders(translation[name])
        if src_ph != tr_ph:
            fails.append(f"{label}: {name} placeholder mismatch\n"
                         f"    source:      {src_ph} <- {src_text!r}\n"
                         f"    translation: {tr_ph} <- {translation[name]!r}")


def main(argv):
    if len(argv) < 2:
        sys.exit(f"usage: {argv[0]} locale/AmiAuth.cd [translation.ct ...]\n"
                 f"       {argv[0]} --fix-encoding FILE [FILE ...]")

    if argv[1] == '--fix-encoding':
        if len(argv) < 3:
            sys.exit(f"usage: {argv[0]} --fix-encoding FILE [FILE ...]")
        for path in argv[2:]:
            fix_encoding(path)
            print(f"re-saved as Latin-1: {path}")
        return 0

    cd_path, ct_paths = argv[1], argv[2:]
    source = parse_entries(cd_path)
    if not source:
        sys.exit(f"{cd_path}: parsed zero entries - check the file/parser")

    fails, warns = [], []
    check_encoding(cd_path, cd_path, fails)
    check_mnemonics(cd_path, source, fails, warns)
    check_rekey_prompts(cd_path, source, fails, warns)

    for ct_path in ct_paths:
        check_encoding(ct_path, ct_path, fails)
        entries = parse_entries(ct_path)
        check_mnemonics(ct_path, entries, fails, warns)
        check_rekey_prompts(ct_path, entries, fails, warns)
        check_placeholders(ct_path, source, entries, fails)

    for w in warns:
        print(f"WARN: {w}")
    for f in fails:
        print(f"FAIL: {f}")

    checked = 1 + len(ct_paths)
    if fails:
        print(f"\n{len(fails)} failure(s), {len(warns)} warning(s) across "
              f"{checked} file(s).")
        return 1
    print(f"OK: {len(warns)} warning(s), 0 failures across {checked} file(s).")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
