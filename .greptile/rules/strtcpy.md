---
scope:
  - "**/*.c"
---
What: Use `strtcpy()` instead of `strcpy()` or `strncpy()`.

Why: It respects the buffer size and guarantees null-termination.

Good:
```c
char dest[10];
strtcpy(dest, "Hello", sizeof(dest));
```

Bad:
```c
char dest[10];
strcpy(dest, "Hello");
```
