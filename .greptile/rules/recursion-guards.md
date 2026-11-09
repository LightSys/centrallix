---
scope:
  - "**/*.c"
---
Recursion guards:

1. Every recursive function, whether it calls itself directly or through other functions (mutual recursion), must check thExcessiveRecursion() before it recurses, usually as its first statement. Flag a recursive function without this check, even if its callers appear to bound the depth today. The check prevents stack exhaustion, especially on input from unprivileged users.

2. When the check trips, the function must raise mssError(1, "<MODULE>", ...) and return its failure value, and callers must propagate that failure (see the error/warning rules). Flag a guard that returns success, continues, or only prints a warning. A recursive function cannot be void, since its contract must allow this failure.

3. Report missing or incorrect guards in functions the PR adds or modifies. Do not ask a PR to add guards to recursive functions it does not touch. The Good example below shows the preferred form; do not flag a guard that differs from it only in wording or in omitting UNLIKELY() (from cxlib/expect.h).

Good:
```c
int
mod_internal_Walk(pNode node)
    {
    int rval = -1;
    int i;

	/** Guard against excessive recursion. **/
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1, "MOD", "Failed to walk node tree: resource exhaustion occurred.");
	    goto end;
	    }

	/** Walk the children. **/
	for (i = 0; i < node->nChildren; i++)
	    if (mod_internal_Walk(node->Children[i]) < 0) goto end;

	rval = 0;

    end:
	return rval;
    }
```

Bad (no guard, and void so it cannot report failure):
```c
void
mod_internal_Walk(pNode node)
    {
    int i;

	for (i = 0; i < node->nChildren; i++)
	    mod_internal_Walk(node->Children[i]);
    }
```
