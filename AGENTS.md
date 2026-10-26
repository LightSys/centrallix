# Editing Files

## Styling
Match the surrounding style when editing a file, but keep in mind that many files are styled badly or incorrectly. If user instructions, coding best practices, or your memory conflict with styles in a file, they take precedence over the file. If styling is unclear, read `centrallix-sysdoc/BeeleyCodingStyle.md` for clarification.

### Comments
Inline comments are one short line, not blocks. They say what the code does now: never history ("used to"), alternatives ("rather than"), or other things it does not do. Function preambles are the only multi-line comments.

### Errors vs warnings
`mssError()` is only for failures that propagate up the call chain. A condition the program continues past is a warning: use `fprintf(stderr, "Warning: ...\n")`.

## Copyright notices
When making meaningful changes to a file (more than a few lines), update the copyright notice at the top of the file to extend to the current year.

## Generated files
Read these generated files freely, but don't edit them: builds overwrite them.
- centrallix/Makefile
- centrallix-lib/Makefile
- centrallix/configure
- centrallix-lib/configure

## Line endings
Some files use CRLF line endings (e.g. `centrallix-doc/Widgets/widgets.xml`). Preserve them: a scripted edit that reads in text mode silently converts the whole file.

## Searching
Use `rg` (if available) instead of `grep`. It's faster, but skips gitignored, hidden, and binary files. The project symlinks files and directories, so add `-L`. `rg` treats structure files (e.g. `.app`, `.cmp`) as binary, so add `-a` for them. Combine flags as needed (e.g. `rg -La "pattern"`), or fall back to `grep`.


# Terms

- **Centrallix** - An open-source web application server and data management engine developed by LightSys. It provides data abstraction, structural embedding, and dynamic widget-based HTML generation (DHTML), serving as the platform on which apps like Kardia are built.

- **Kardia** - A web-based system built on the Centrallix platform.

- **Object System (ObjectSystem)** - Centrallix's high-level filesystem analog: a tree of typed objects that allows read/write access to files, database rows, APIs, reports, etc. through a consistent path-based API (the OSML). ObjectSystem Drivers (OSDs) provide the interface to handle these diverse data sources in a single, coherent hierarchy. On dev machines, the `centrallix-os` folder in this project is the object system root node so the path `/samples` is `centrallix-os/samples`.

- **Widget** - A UI component in Centrallix DHTML, either visual (e.g. buttons, tables) or nonvisual (e.g. forms, connectors), defined as objects in the ObjectSystem and rendered to HTML/JavaScript by widget drivers.

For other terminology or to find more detailed info, read `centrallix-sysdoc/Terminology.md`.


# Building Apps

## Where to look
- **Widget properties, children, events, actions**: `centrallix-doc/Widgets/widgets.xml` covers all widgets. `centrallix-doc/Report/report.xml` covers report widgets.
- **Structure file syntax** (.app, .cmp, and others): `centrallix-doc/StructureFile.txt`.
- **Forms** (form modes, element callbacks): `centrallix-sysdoc/HTFormsInterface.md`.
- **Widget tree / Wgtr module** (server-side tree building, verification, component loading): `centrallix-sysdoc/WidgetTree.md`.
- **Practical examples**: `centrallix-os/samples/*.app`.

## Key patterns

Widget names must be unique for the whole app, but each component (`.cmp`) has its own namespace.

**Data flow**:
- User interaction → event → `widget/connector` → action on any widget.
- Form or OSRC updated by action → HTTP to OSML → OSRC replica updated → form or table re-renders.

## Common traps
- Without `$Version=2$` on the first line, a structure file's expressions (e.g. `runclient()`) are never evaluated.
- Structure files only support `//` comments. Other syntax (e.g. `/* */`) is a parse error that may look like a missing attribute name.
- Child widgets inside a `widget/template` break the whole app's JavaScript, so templates should only set properties.
- `runclient()` causes expressions to evaluate on the client, so it can only call functions that have `cxjs_*` implementations in `centrallix-os/sys/js/ht_render.js`.
