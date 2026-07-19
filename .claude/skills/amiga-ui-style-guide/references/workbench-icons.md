# Workbench, Icons, and Argument Passing

From Style Guide ch. 7. Workbench is the doorway to your app — behave well
in it.

## Icons

- Ship icons for **everything the user can access**: the program, docs, and
  bundled tools. Imagery lives in `<name>.info`.
- A good icon communicates its function at a glance. Prefer symbolism to
  words — text limits international distribution.
- **Max ~80×40 pixels.** Oversized icons waste screen/disk space and look
  unprofessional.
- 3-D look with light from the upper left; must be viewable in one bitplane
  as well as two.
- **Tool icons** = executables (double-click runs). **Project icons** = data
  files (double-click launches the creating app and loads the data; a
  system default project icon exists).
- **Create icons by default** when saving projects — but only *after* a
  successful save (no orphan icons) — and give the user the standard
  Settings-menu "Create Icons?" toggle (enabled by default) to turn it off.
- **Never save new project icons with a fixed position** — let Workbench
  place them (NO_ICON_POSITION); same when you rewrite an existing icon's
  imagery.

## Tool Types and Default Tool

- Workbench-started apps get a WBStartup message carrying the icon's Tool
  Types (`KEYWORD=value`) and, for project icons, the Default Tool (the app
  to run).
- **Altering icons**: change only what you must; preserve imagery, position,
  Default Tool, and — critically — any Tool Types entries you don't
  recognize. Never rewrite Tool Types from scratch.
- Support **project-level** Tool Types so networked users can override the
  tool icon's least-common-denominator settings.
- Standard Tool Types to support (or at least not clash with):
  `WINDOW=CON:<spec>`, `DONOTWAIT`, `TOOLPRI=<pri>`, `STARTPRI=<pri>`,
  `PUBSCREEN=<name>`, `STARTUP=<arexx script>`, `PORTNAME=<name>`,
  `SETTINGS=<file>`, `UNIT=<n>`, `DEVICE=<name>`, `FILE=<path>`,
  `WAIT=<seconds>`, `PREFS=<type>`, `CX_POPUP=<yes/no>`,
  `CX_POPKEY=<key>`, `CX_PRIORITY=<n>`.

## The Apps (graphical argument passing while running)

- **AppWindow**: window that accepts dragged icons — a graphical file
  requester. Mark each drop area with an icon drop box gadget (one per
  function); activate the window when an icon is dragged in. Only works on
  the Workbench screen — on other screens, fall back to an AppMenu.
  Appropriate when you need a window anyway.
- **AppIcon**: an icon that accepts drops; good for background programs
  with little other UI (print spooler). Its imagery should hint at what it
  accepts; double-click opens a status/control window.
- **AppMenu**: adds your item to Workbench's Tools menu.

## Preferences interplay

- Respect global Preferences (fonts, colours, overscan, etc.) as your
  defaults; see `shell-arexx-prefs.md` for app-specific settings handling.
