---
scope:
  - "**/*.c"
---
Recursion guards:

1. Every recursion cycle (a function that calls itself, or a chain of calls that leads back to its start) must check thExcessiveRecursion() somewhere on the cycle before it recurses. Only one check per cycle is required: a check in a central function that every cycle passes through, such as a dispatcher, covers every function it calls. Flag a cycle with no check, even if callers appear to bound the depth today. Do not flag a function for lacking its own check when every cycle through it already has one.

2. The check must raise mssError(1, "<MODULE>", ...) and return failure, and every function on the cycle must propagate that failure (see the error/warning rules). Flag a check that returns success, continues recursing, or only prints a warning, and flag any void function on a cycle, since it cannot report the failure.

3. Report an unguarded cycle only when the PR adds or modifies a function or call on it, or removes its check. Do not flag a check that differs from the Good example only in wording or in omitting UNLIKELY() (from cxlib/expect.h). Do not flag functions outside the cycle for dropping an error from it (this is incorrect but too common to fix now, see the error/warning rules).

Good (the check in mod_internal_Eval() covers mod_internal_EvalList()):
```c
int
mod_internal_Eval(pNode node)
    {
    int rval = -1;

	/** Guard against excessive recursion. **/
	if (UNLIKELY(thExcessiveRecursion()))
	    {
	    mssError(1, "MOD", "Failed to evaluate node: resource exhaustion occurred.");
	    goto end;
	    }

	/** Dispatch on node type. **/
	if (node->Type == MOD_T_LIST && mod_internal_EvalList(node) < 0) goto end;

	rval = 0;

    end:
	return rval;
    }

int
mod_internal_EvalList(pNode node)
    {
    int rval = -1;
    int i;

	/** Evaluate the children. **/
	for (i = 0; i < node->nChildren; i++)
	    if (mod_internal_Eval(node->Children[i]) < 0) goto end;

	rval = 0;

    end:
	return rval;
    }
```

Bad (no check on the cycle, and void so it cannot report failure):
```c
void
mod_internal_Walk(pNode node)
    {
    int i;

	for (i = 0; i < node->nChildren; i++)
	    mod_internal_Walk(node->Children[i]);
    }
```
