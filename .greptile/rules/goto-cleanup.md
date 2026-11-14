---
scope:
  - "**/*.c"
---
Goto in error handling and cleanup:

1. C functions handle failures by jumping forward to a cleanup label with goto. Goto is allowed in this situation and ONLY in this situation: it is the required pattern here, and not allowed anywhere else. Flag any backward goto used for error handling.

2. A function normally has one cleanup label for the whole function. Name it `end` when cleanup runs on both success and failure, or `error` when it runs only on failure. Accept either name. Some functions also use a separate cleanup label inside a scope like a loop for per-iteration cleanup; do not flag that pattern or its label name.

3. Prefer a single exit: initialize the result to failure (`int rval = -1;`), make every failure `goto end`, and set `rval = 0` only after the last step succeeds. A forgotten assignment then fails safely instead of silently reporting success.

4. Flag a `return` that skips cleanup the label would have done, such as freeing memory, closing an object or file, or releasing a lock. A return before anything needs cleanup is fine.

5. Cleanup at the label must be safe on every path that reaches it. Free, close, or deinitialize a resource only if it was acquired, using a sentinel such as a pointer that stays NULL until acquisition succeeds. Flag cleanup that a failure path can reach before the resource was initialized.

6. Report violations only in lines the PR adds or modifies. When new lines follow the error-handling style already used by the surrounding function, do not ask for the function to be restructured, even if that style differs from this pattern. Flag only defects the new lines themselves introduce, such as a new return that skips cleanup.

Good:
```c
int
mod_internal_Load(char* path)
    {
    int rval = -1;
    pFile fd = NULL;
    char* buf = NULL;

	/** Open the file. **/
	fd = fdOpen(path, O_RDONLY, 0600);
	if (fd == NULL)
	    {
	    mssErrorErrno(1, "MOD", "Failed to open '%s'.", path);
	    goto end;
	    }

	/** Allocate the buffer. **/
	buf = nmSysMalloc(BUF_SIZE);
	if (buf == NULL)
	    {
	    mssError(1, "MOD", "Failed to allocate a buffer for '%s'.", path);
	    goto end;
	    }

	/** Read the contents. **/
	if (fdRead(fd, buf, BUF_SIZE, 0, 0) < 0)
	    {
	    mssError(1, "MOD", "Failed to read '%s'.", path);
	    goto end;
	    }

	rval = 0;

    end:
	if (buf != NULL) nmSysFree(buf);
	if (fd != NULL) fdClose(fd, 0);
	return rval;
    }
```

Bad (the second failure returns without closing fd):
```c
int
mod_internal_Load(char* path)
    {
    pFile fd;
    char* buf;

	fd = fdOpen(path, O_RDONLY, 0600);
	if (fd == NULL) return -1;

	buf = nmSysMalloc(BUF_SIZE);
	if (buf == NULL) return -1;

	...
    }
```
