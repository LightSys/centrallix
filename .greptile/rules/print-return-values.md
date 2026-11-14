---
scope:
  - "**/*.c"
---
Do not flag unchecked return values of printf/fprintf used for output or diagnostics. Flag unchecked `snprintf`/`vsnprintf` when the result is then used as a path, query, URL, or identifier, since truncation silently changes it.
