---
scope:
  - "**/*.c"
  - "**/*.h"
---
The code targets GNU C99 (`-std=gnu99`): C99 plus GNU extensions such as statement expressions, `__typeof__`, and `__attribute__`. Flag features added in C11 or later, even though GCC accepts many of them even in gnu99 mode. Examples: `_Static_assert`, `_Generic`, `_Noreturn`, `_Alignas`, `_Alignof`, `_Thread_local`, `_Atomic`, `<stdatomic.h>`, `<threads.h>`, `<stdalign.h>`, `aligned_alloc()`, and C23's `nullptr`, `constexpr`, `[[...]]` attributes, `bool`/`true`/`false` without `<stdbool.h>`, and `#embed`. Suggest the GNU equivalent where one exists (e.g. `__attribute__((noreturn))`).
