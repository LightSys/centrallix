# Editing Files

## Styling
When editing/writing files, follow the best practices of the language in use. For styling, understand and match the surrounding style when editing a file, but keep in mind that many files are styled badly or incorrectly. If user instructions, coding best practices, or your memory conflict with styles in a file, they take precedence over the file. If styling is unclear, read `centrallix-sysdoc/BeeleyCodingStyle.md` for clarification.

## Copyright notices
When making meaningful changes to a file (more than a few lines), update the copyright notice at the top of the file to extend to the current year.

## Generated files
While it may be very useful to read the following generated files, do not edit them since changes will be overwritten by builds.
- centrallix/Makefile
- centrallix-lib/Makefile
- centrallix/configure
- centrallix-lib/configure

## Searching
Instead of searching with `grep`, use `rg` (if available). It's faster, but remember that it ignores `.gitignored` files, hidden files, binaries, etc. This project includes symlinks (both to files and directories), so ensure they are traversed when searching (aka. with `rg -L` or alternatives). Many centrallix structure files (e.g. `.app`, `.cmp`, etc.) are treated as binary files by `rg`, add `-a` when searching them. You may need to combine both flags to navigate these challenges, e.g. `rg -La "pattern"`, or fall back to `grep` if you encounter issues.


# Terms

- **Centrallix** - An open-source web application server and data management engine developed by LightSys. It provides data abstraction, structural embedding, and dynamic widget-based HTML generation (DHTML), serving as the platform on which apps like Kardia are built.

- **Kardia** - A web-based system built on the Centrallix platform.

- **Object System (ObjectSystem)** - Centrallix's high-level filesystem analog: a tree of typed objects that allows read/write access to files, database rows, APIs, reports, etc. through a consistent path-based API (the OSML). ObjectSystem Drivers (OSDs) provide the interface to handle these diverse data sources in a single, coherent hierarchy.

- **Widget** - A UI component in Centrallix DHTML, either visual (e.g. buttons, tables) or nonvisual (e.g. forms, connectors), defined as objects in the ObjectSystem and rendered to HTML/JavaScript by widget drivers.

For other terminology or to find more detailed info, read `centrallix-sysdoc/Terminology.md`.


# Building Apps

## Where to look
- **Widget properties, children, events, actions**: `centrallix-doc/Widgets/widgets.xml` covers all widgets. `report.xml` in that dir covers report widgets.
- **Structure file syntax** (.app, .cmp, and others): `centrallix-doc/StructureFile.txt`.
- **Forms** (form modes, element callbacks): `centrallix-sysdoc/HTFormsInterface.md`.
- **Widget tree / Wgtr module** (server-side tree building, verification, component loading): `centrallix-sysdoc/WidgetTree.md`.
- **Practical examples**: `centrallix-os/samples/*.app`.

## Key patterns

**Data flow**:
- User interaction → event → `widget/connector` → action on any widget.
- Form or OSRC updated by action → HTTP to OSML → OSRC replica updated → form or table re-renders.

**Expressions in structure files**: Always add `$Version=2$` at the top of a `.app` or `.cmp` files.
