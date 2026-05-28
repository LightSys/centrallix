# Editing Files

## Styling
Understand and match the surrounding style when editing a file. If that style is unclear, read `centrallix-sysdoc/BeeleyCodingStyle.md` for clarification.

## Copyright notices
When editing a file, update the copyright notice at the top of the file to extend to the current year.

## Generated files
While it may be very useful to read the following generated files, do not edit them since changes will be overwritten by builds.
- Centrallix/Makefile
- Centrallix-lib/Makefile
- Centrallix/configure(.sh)
- Centrallix-lib/configure(.sh)

## Searching
Instead of searching with `grep`, use `rg` (if available). It's faster, but remember that it ignores `.gitignored` files, hidden files, binaries, etc. This project includes symlinks (both to files and directories), so ensure they are traversed when searching (aka. with `rg -L` or alternatives). Many centrallix structure files (e.g. `.app`, `.cmp`, etc.) are treated as binary files by `rg`, add `-a` when searching them. You may need to combine both flags to navigate these challenges, e.g. `rg -Lra "pattern"`, or fall back to `grep` if you encounter issues.

# Terms
- **Centrallix** - An open-source web application server and data management engine developed by LightSys. It provides data abstraction, structural embedding, and dynamic widget-based HTML generation (DHTML), serving as the platform on which apps like Kardia are built.
- **Kardia** - A web-based nonprofit ministry management application built on the Centrallix platform. It provides donor tracking, financial management, and ministry operations for Christian mission organizations.
- **Object System (ObjectSystem)** - Centrallix's high-level filesystem analog: a unified tree of typed objects allowing controlled read and write access to files, database rows, directories, APIs, reports, and more, each accessible through a consistent path-based API (the OSML). ObjectSystem Drivers (OSDs) provide the intelligence to interpret different object types, allowing disparate data sources to appear as a single coherent hierarchy.
- **Widget** - A UI component in Centrallix's DHTML application layer, either visual (e.g. buttons, edit boxes, tables) or nonvisual (e.g. forms, event connectors). Widgets are defined as objects in the ObjectSystem and rendered to HTML/JavaScript by widget drivers on the server side.
For other terminology or to find more detailed info, read `centrallix-sysdoc/Terminology.md`.

# Building Apps

## Where to look
- **Widget properties, children, events, actions**: `centrallix-doc/Widgets/widgets.xml` covers all widgets.
- **Structure file syntax** (.app, .cmp, and others): `centrallix-doc/StructureFile.txt`.
- **Forms** (form modes, element callbacks): `centrallix-sysdoc/HTFormsInterface.md`.
- **Widget tree / Wgtr module** (server-side tree building, verification, component loading): `centrallix-sysdoc/WidgetTree.md`.
- **Practical examples**: `centrallix-os/samples/*.app`.

## Key patterns
**Data flow**:
- User interaction → event → `widget/connector` → action on any widget.
- Form or OSRC updated by action → HTTP to OSML → OSRC replica updated → form or table re-renders.

**Expressions in structure files**: Add `$Version=2$` at the top of a `.app` or `.cmp` file to enable `runclient()` expressions in connector parameters and widget properties. Without it, attribute values are static only.

**Master-slave OSRC relationships** — use `widget/rule` with `ruletype=osrc_relationship` (documented under `widget/osrc` children and `widget/rule` in `centrallix-doc/Widgets/widgets.xml`), not the deprecated `Sync`/`DoubleSync` OSRC actions.

**Three independent focus types**: Keyboard, mouse, and data focus are separate. The `widget/page` properties `kbdfocus1/2`, `mousefocus1/2`, `datafocus1/2` style each independently.
