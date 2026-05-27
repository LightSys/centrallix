# Editing

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

# Terms
- **Centrallix** - An open-source web application server and data management engine developed by LightSys. It provides data abstraction, structural embedding, and dynamic widget-based HTML generation (DHTML), serving as the platform on which apps like Kardia are built.
- **Kardia** - A web-based nonprofit ministry management application built on the Centrallix platform. It provides donor tracking, financial management, and ministry operations for Christian mission organizations.
- **Object System (ObjectSystem)** - Centrallix's high-level filesystem analog: a unified tree of typed objects allowing controlled read and write access to files, database rows, directories, APIs, reports, and more, each accessible through a consistent path-based API (the OSML). ObjectSystem Drivers (OSDs) provide the intelligence to interpret different object types, allowing disparate data sources to appear as a single coherent hierarchy.
- **Widget** - A UI component in Centrallix's DHTML application layer, either visual (e.g. buttons, edit boxes, tables) or nonvisual (e.g. forms, event connectors). Widgets are defined as objects in the ObjectSystem and rendered to HTML/JavaScript by widget drivers on the server side. (For info on using specific widgets, see `centrallix-sysdoc/widgets.md`)
For other terminology or to find more detailed info, read `centrallix-sysdoc/Terminology.md`.
