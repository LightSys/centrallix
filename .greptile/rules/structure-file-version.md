---
scope:
  - "**/*.app"
  - "**/*.cmp"
  - "**/*.tpl"
  - "**/*.struct"
  - "**/*.ifc"
  - "**/*.thm"
  - "**/*.qy"
  - "**/*.qyt"
  - "**/*.rpt"
---
A structure file that uses any expressions (e.g. runclient() or runserver() in connector params or widget properties) must have `$Version=2$` as its first line. Without it, all attribute values are treated as static. Version 2 also requires a semicolon after each attribute line.
