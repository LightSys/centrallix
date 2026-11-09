---
scope:
  - "**/*.c"
---
Errors and warnings in the codebase:

1. Errors use mssError(). An error is a failure the function propagates by returning its failure value (-1, NULL, or another documented failure result). Flag a failure return that no mssError() explains, either raised here or already raised by the failing callee. Flag mssError(1, ...) on a path that already raised an error, since it clears the original error.

2. Warnings use mssWarn(module, format, ...), not mssError() and not fprintf(stderr, ...). A warning is a condition the program continues past without switching to an error path that returns a failure value, such as a truncation, a fallback, or a skipped optional step. Flag mssError() used for a warning, and flag fprintf(stderr, "Warning: ...") or similar direct prints of warnings. The only exception is the error and warning machinery inside centrallix-lib/src/mtsession.c, because calling the normal error/warning functions there could recurse. Clean up failures on an error path may be either warnings or errors.

3. A void function promises in its contract that it cannot fail. Flag any failure path in a void function (an mssError() call, or an early return that skips its work because something failed); it should return a failure value instead. Using mssWarn() for a condition the function continues past is fine. A recursive function can always fail on thExcessiveRecursion(), so flag a void recursive function: it must return a failure value that callers propagate.

4. Resolving errors is required. When code goes from an error path back to a success path after a call that raised mssError() (e.g. a fallback after a failed attempt, a probe where failure was one of the expected possibilities, a best-effort loop that continues past a failure, or a tool that reports the error to the user and carries on), it must call mssClearError() at that point. Flag a recovery point that is missing mssClearError(): the stale error stack might otherwise be reported with a later, unrelated failure. Also flag code that ignores a callee's failure and continues as if it succeeded (this is a dropped error: always incorrect).

5. Much of the existing codebase breaks some of these rules, especially dropped errors. Report violations in lines the PR adds or modifies. Report pre-existing violations in unchanged lines only when the PR's change depends on them or makes them worse, so a PR is not forced to expand its scope to fix adjacent, pre-existing issues.
