# The settings menu in another language

The settings menu can be translated by anyone who can edit a text file. Nothing is translated by
building BlueWake again, the game's disc is not touched, and a translation that is half finished still
works: every line it has not reached stays in English, so it is usable from its first sentence.

This is the menu's own text (the tabs, every setting's name and its quiet line, what the sections are
called). The game's dialogue and item text are Wind Waker's, come off your own disc, and are a separate
piece of work.

## What a player does

**It picks your language for you.** **F1 › Config › Language** starts on *System*, which follows the
language your computer is set to when there is a catalog for it, and English when there is not. Nothing
has to be chosen, and the menu is in your language the first time it opens.

1. Open **F1 › Config › Your files** and take the folder it names. Your settings file is in that folder;
   the catalogs go in a `languages` folder beside it.
   - Linux: `~/.config/BlueWake/languages`
   - Windows: `%APPDATA%\BlueWake\languages`
   - Mac: `~/Library/Application Support/BlueWake/languages`
2. Put the catalog there (`es.txt`, `de.txt`, ...).
3. Open the menu, **Config › Language**, and pick it by the name it gives itself. The menu changes as
   you pick, with no restart. *System* goes back to following your computer's language, and *English*
   stays English whatever the system says.

A file that cannot be read leaves the menu in English and says so in the session log
(`[language] es: cannot read …`), rather than half a language.

## What a translator does

Start from `languages/en.txt` in the repository. It is every string the menu can show, in English, in
the order the menu draws it.

    language.name = Español
    setting.BLUEWAKE_ASPECT.label = Panorámica
    tab.display = Pantalla

- Translate the text after each `=`. The key before it is the menu's own name for the string and must
  not be changed: it is how the menu finds your line.
- Keep every `%s`, `%d` and `%.2f` exactly as it is, and out of any quoting your language might put
  around it. Each one is filled in with a number or a name when the menu draws the line.
- Leave out any line you have not done yet; it stays English.
- `#` starts a comment, blank lines do nothing, and the spaces either side of `=` are trimmed.
- `language.name` is what the Language list calls your translation.
- `\n` in a line makes a line break in the menu.

Check it with `python3 tests/test_i18n_strings.py` from the repository. It reads every catalog in
`languages/` and reports a key English does not have, a `%s`/`%d` that no longer matches the English
line, a key written twice, or a file with no `language.name`.

Two things the checker cannot see, and only you can:

- **The menu is laid out for English.** A much longer line is wrapped, but a tab name or a setting's
  name that runs half again as long can crowd the row it sits in. Keep a label as short as your
  language allows.
- **The menu's font has to have your letters.** It covers Latin (with accents), Greek and Cyrillic. A
  script those fonts lack — Chinese, Japanese, Korean, Arabic, Hebrew, Thai — needs a font file beside
  the app as well, and Arabic and Hebrew also need the menu to lay text out right to left, which it
  does not do.

The list shows sixteen names at most (*System*, *English* and fourteen catalogs); past that the row says
how many it left out. Keeping the ones you read in that folder is the fix.

## What is translated today

| Translated | Not yet |
| --- | --- |
| Every tab's name | The paragraphs inside a control that draws itself: the texture pack's, the Better Wind Waker options', the key and button tables, the Climbing and Jump & Sprint panes |
| Every setting's name | The menu's own buttons and footers (*Resume*, *Restart now*, …) |
| Every setting's quiet line | The first-run and setup windows, the crash notices |
| The section headings, and *Experimental* | The iPhone, iPad, Android and tvOS menus, which draw their own |
| The language picker itself | |

The untranslated remainder is the follow-up: the mechanism is the same, and each paragraph is a
`bw_lang::text("key", "English")` call where the string is drawn.

## Adding a string to the menu (contributors)

A row in `settings_table.h` is translatable as soon as it has a `.label` or `.note`: the keys are
`setting.<the row's key>.label`, `.note`, `.group` and `.experimental`, filled in from the row, so
nothing has to be added to a catalog by hand. For a string that is not a row, wrap it where it is drawn:

    ImGui::TextDisabled("%s", bw_lang::text("menu.something", "The English it falls back to."));

and, where a sentence has one value in it that a language may want to move:

    bw_lang::fill("menu.turn_on", "Turn on \"%s\" to change this.", bw_lang::label(setting(off_need)))

Then regenerate the English catalog the translator starts from, and check the lot:

    python3 tests/test_i18n_strings.py --write
    python3 tests/test_i18n_strings.py

The `audit` job runs the check on every pull request, so a row that is reworded and a catalog that
still has the old key is a failure rather than a stale line nobody notices.

## Where catalogues are kept (a separate repository)

Translations are the translators' work and do not need a BlueWake release to change, so they belong in
their own repository, one folder a language, with a person who reads it named for each. The application
reads a catalog from the player's own languages folder, so a corrected sentence reaches a player by
copying one file over — no build, and no waiting on this repository's release.

The keys live here: `languages/en.txt` is generated from the sources and checked against them in CI, so
this repository is the authority for *what* can be said, and the translations repository for how it is
said in each language.
