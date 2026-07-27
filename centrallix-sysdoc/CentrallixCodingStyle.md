# Centrallix Coding Style

**Author**: Israel Fuller

**Date**: June 24th, 2026

**License**: Copyright (C) 2026 LightSys Technology Services.  See `LICENSE`.


## Introduction
The following style rules should be followed in every applicable file in Centrallix.  Note that many files may break style rules, however, any new changes committed should follow these rules.

Many of them are enforced by the style linter, however, this file is the source of truth.  Thus, where the lint rules and this file disagree, update the lint rules.  Keep in mind that some styles documented here may not have lint rules due to linter limitations, but they should still be followed to the best of your ability.

<!-- TODO: Israel - Add info about the style linter after it's set up. -->

## Module Prefixes
All modules have an assigned prefix. This is usually (but not always!) a two-to-four-character abbreviation of the module name.  These prefixes are listed in [Prefixes.md](Prefixes.md).  This prefix is used frequently in identifiers for functions and globals when they are accessible from outside the module.


## Style Rules
All code should follow a consistent style when possible.  This helps developers to read it quickly and accurately without being tripped up by abrupt changes in style or unfamiliar coding practices.

These rules apply to every language in Centrallix, unless a section states otherwise.  A section that only covers certain file types is marked with a `For:` line listing them.  Examples use C, the most common language in the codebase.

<!-- TODO: Israel - Document the deltas for `js` files and structure files (`.app`, `.cmp`). -->


### Indentation
- Code is always indented using 1 tab, not with spaces.
- You can set your editor tab length to any size you prefer.  However, code should look reasonable with any tab length up to 8 spaces.
- Prefer tabs, but always use spaces for aligning characters, such as in a copyright notice or a column of struct members.  Tabs break alignment when the viewer uses a different tab length.
- Lines should not have trailing whitespace.  Thus, blank lines are not indented.

### Spacing
- Language constructs (e.g. `if ()`, `for ()`, `while ()`, `switch ()`, etc.) should always be separated from their parentheses with a space.
- Function calls (e.g. `mssError()`) and function declarations should *not* be separated from their parentheses by a space.
- Pointer types attach the `*` to the type, not to the identifier: `char* p`, not `char *p`.  A pointer is part of the type, so it belongs with the type.
- The `return` statement should be followed by a space and the return value should only use parentheses when needed.  Do not treat `return` as a function call, it is not a function, and it does not give a return value... well, it kind of does, but I think you get the point :)

### Braces
- Braces use a style similar to the Linux kernel.
	- Braces are placed on the same line as the initiating structure.
	- Braces are terminated at the same indentation as the initiating structure.
- Any `if` statement, `for` loop, `switch` statement, or similar structure must use braces if it contains:
	- More than one line of code.
	- Another such structure.
- An `else` or `else if` starts on the line of the closing brace of the block before it, not on the line after that brace.

### Switch Statements
- `case` and `default` labels are indented one level inside the `switch`.
- Every case body is enclosed in braces, even a body of one line, and the `break` goes inside those braces.
- Fallthrough is allowed, but it must be explicitly marked with a `/** Fallthrough. **/` comment.

For example:
```c
switch (algorithm) {
	case CA_ALG_KMEANS2: {
		preKMeans(&vectors);
		/** Fallthrough. **/
	}
	case CA_ALG_KMEANS: {
		rval = caKMeans(vectors, n_vectors, n_clusters, n_iters, 0.01, &labels, &sims, false);
		break;
	}
	default: {
		mssError(1, "CA", "Unknown clustering algorithm (%d)", algorithm);
		goto error;
	}
}
```

### Comments
- Single line comments to the right of code or declarations use single asterisks, e.g. `/* this is to the right of code */`.
- Single line comments not to the right of code (including section comments) use double asterisks, e.g. `/** this is a single line comment **/`.
- Multiline comments (including function comments) use three asterisks, which continue on each line, e.g.
	```c
	/*** myFunFunction - this is a function
	 *** that does some fun stuff with its
	 *** parameters.
	 ***/
	```

### Include Files
For: `.c`, `.h`

- If a `.c` file uses an identifier from a `.h` or C standard library not available by default, that file must `#include` the correct `.h` file or library.
- For example, the `mssError()` function is defined in `mtsession.h`, so that file must have a `#include` in every C file that uses `mssError()`
	```c
	#include "mtsession.h" /* Required. */

	...

	if (rval < 0) {
		mssError(0, "Example", "The function failed (error code: %d)", rval);
		return rval;
	}
	```
- Angle bracket includes (`<...>`) come first, then quoted includes (`"..."`), separated by a blank line between the two groups.
- Within each group, includes are sorted in ascending alphabetical order.
	```c
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>

	#include "cxlib/xstring.h"
	#include "mtsession.h"
	#include "obj.h"
	```
- Every `.h` file must wrap all content in an include guard, using the standard `#ifndef` / `#define` / `#endif` form.
	- `#pragma once` is not part of the C standard and must not be used.
- The guard is named for the file in SCREAMING_SNAKE_CASE with a leading underscore, so `mtsession.h` uses `_MTSESSION_H`.
	```c
	#ifndef _MTSESSION_H
	#define _MTSESSION_H

	/** Copyright notice, then includes, declarations, etc. **/

	#endif
	```

### Naming Identifiers
All identifiers should be spelled correctly and avoid using non-obvious abbreviations.  The very common `i`, `j`, and `rval` local variables are exceptions to this general rule.
- Functions are named with camelCase.
	- The module prefix is prepended if the function is reachable from outside the module. (The prefix is optional for internal & unreachable functions.)
	- Internal functions prepend `_internal_` or `_i_`. (e.g. `xyz_i_HiddenStuff()`)
- Local variables and function parameters are named with snake_case.
- Every global variable in a module lives in a single module-wide global struct named with SCREAMING_SNAKE_CASE with the module name prepended (for easy identification).  The recommended name is `MOD_GLOBALS` (where `MOD` is the module prefix).
- `Typedef` names & structs:
	- `Typedef`ed names use PascalCase (except for the module prefix).
	- Both the struct type (`XxxxYyy`) and a pointer alias (`pXxxxYyy`) are declared in the same typedef.
	- Structs reachable from outside the module start their name with the module prefix.  (As with functions, the prefix is optional for internal structs.)
	- Members of a struct or union use PascalCase.
		- **Exception**: A count member may take a lowercase `n` prefix, and a pointer member a lowercase `p` prefix, before the PascalCase name.  (e.g. `nDatas`, `pCluster`)
- In C, value macros are treated as globals and follow the naming style of the global struct.
- In C, function macros are treated as functions, following those styles.

### Struct & Union Declarations
For: `.c`, `.h`

- A struct or union is declared together with its `typedef` in a single statement (see [naming identifiers](#naming-identifiers) for the names used).
- C syntax requires the `*` of the pointer alias to attach to the name, so this is an exception to the [pointer spacing rule](#spacing).
- Each member is declared on its own line.
- Member names are aligned in a column, padded with spaces (never tabs, see [indentation](#indentation)).
	- Leave a few extra spaces of padding where a longer type may be added later, especially in a struct whose types are all short or which has many members.  This allows adding the type later without a reflow on every line that buries the real change in git-blame.  In isolation of other context, padding to column 20 is probably a good idea.

For example:
```c
typedef struct _ClusterSource {
	Magic_t          Magic;
	unsigned int     nDatas;
	char*            Name;
	pVector*         Vectors;
	DateTime         DateCreated;
} ClusterSource, *pClusterSource;
```

### File Organization
For: `.c`, `.h`

It is recommended to order the top level of a file in the following order, for consistency with other files, unless there is a good reason to do otherwise (such as a forward reference):
1. Copyright notice (after the include guard in a `.h` file).
2. `#include` groups.
3. Macros.
4. `Typedef`s, structs, and unions.
5. Prototypes for functions defined later in the same file.
6. Function definitions (`.c` files).

**Note**: Many files may not have all of these sections.

### Types
For: `.c`, `.h`

- C code should work for any C of C99 or later, so `<stdbool.h>` and variable declarations inside a `for` statement are always available.
- Use `NULL` for null pointers, never `0`.
- Use `bool` with `true` and `false` for boolean values, never an `int` holding `0` or `1`.
- `int` and `unsigned int` are the default integer types.  Use the fixed-width types in `<stdint.h>` (e.g. `uint32_t`) only where the exact width matters, such as data written to a file, a database, or the network.

For example:
```c
bool auto_seed = false;    /* Not an int holding 0. */
char* name = NULL;         /* Not 0. */
unsigned int n_items = 0;  /* Default integer type. */
uint32_t wire_value;       /* Width matters on the wire. */
```

### Line Length
- There's no hard line length limit, but it is recommended to wrap lines at 80 characters.
- Code should rarely be indented more than 3-4 nested blocks within the enclosing function.  For example, if you write an `if` statement in an `if` statement in a `for` loop in an `if` statement, consider refactoring code into a helper function and double check that you're using [guard clauses](#error-checking-format) properly.
- **Exception**:  Markdown files should use longer lines, otherwise reflowing a line in a markdown file can quickly turn a small edit into a huge git-blame.

### Function Declarations
For: `.c`, `.h`

*The layout rules below (short declarations on one line, long parameter lists one per line) are worth following in any language.  The rest is specific to C.*

- A function's return type should be written on its own line, and the function name begins on the line below it.  This makes a definition easy to find, because a search for `^name` matches the definition and nothing else.
	- This applies to definitions only.  Prototype declarations, such as those in a `.h` file, keep the return type on the same line as the name because they should not appear in searches for definitions.
- Macro functions are declared on one line because they do not have an explicit return type.
- Apart from the return type, short function declarations should be placed on one line, for example:
```c
int
getTime(pTimer timer) {
	...
}
```
- Function declarations with many parameters may be extended to a line for each parameter, for example:
```c
int
caKMeans(
	pVector* vectors,
	const unsigned int num_vectors,
	const unsigned int num_clusters,
	const unsigned int max_iter,
	const double min_improvement,
	unsigned int* labels,
	double* vector_sims,
	bool auto_seed
) {
	...
}
```
**Note**: This should be a rare occurrence because, in many cases, it's better to avoid functions with large numbers of parameters.  Calling such functions requires extra care to avoid confusing calls like: `caKMeans(v, 32, 256, 12, 0.01, &l, &s, false);`.  If large numbers of parameters are truly the best solution in your situation, using clear variable names for even *some* parameters, as opposed to passing magic numbers, reduces this issue, e.g.: `caKMeans(vectors, n_vectors, n_clusters, n_iters, 0.01, &labels, &sims, false);`

### Function Calls
*When in doubt, use similar styling to [function declarations](#function-declarations).*

- Short function calls should also be placed on one line: `getTime(&timer)`.
- `sizeof` is an operator, not a function, but it is styled as a function call: write `sizeof(ModDataT)`, with no space before the parenthesis.
- Function calls with many or long parameters may span multiple lines.  Literal parameters (especially string literals) may even be split across lines.  Situations vary, so use discernment and consistency to write the most clear code.  The most common example of this is the C calls to write inline `HTML` and `CSS`:
	```c
	htrAddStylesheetItem_va(s,
		"\t\t#tbld%POSsub%POS { "
			"position:absolute; "
			"visibility:hidden; "
			"left:0px; "
			"top:0px; "
			"width:"ht_flex_format"; "
			"height:%POSpx; "
			"z-index:%POS; "
		"}\n",
		t->id, detail_id,
		ht_flex_w(t->w, tree),
		h,
		z + 1
	);
	```
	- Unsurprisingly, writing such calls clearly can be a real struggle!

### Local Variables
- Local variables should be declared as close to where they are used as possible, and especially within the narrowest scope.  This reduces the amount of worrying about side effects and scrolling around that a programmer must do when seeing them.
- Variables should be declared using `const` (or the equivalent keyword for the relevant language) when their value is not changed.  This serves as a note to the reader so they know that the value won't change.
- **Exception**: C dynamic arrays are their own flavor of fun that sometimes require exceptions to the above rules.

### Truthiness
- Do not check the truthiness of any value other than a boolean.
- This includes using the not operator (`!` in C), which also checks truthiness.
- Use explicit equality checks to reduce confusion, especially for programmers less familiar with the language.
- Correct example:
	```c
	if (source->Name != NULL) printName(source->Name);
	if (strcmp(source->Title, "None") == 0) queryTitle(source->Title);
	```
- Incorrect example:
	```c
	if (source->Name) printName(source->Name);
	if (!strcmp(source->Title, "None")) queryTitle(source->Title);
	```
- **Exception**:  This rule may be held loosely or ignored in `js`, where explicit truthiness checks can be cumbersome and confusing.

### Returning
- Any function that returns a non-void type must terminate all paths with a `return`.
- Functions that do return void may allow execution to reach the end of scope without an explicit return.
- Note: This does not require a trailing `return` that cannot be reached.  If every code path already returns, a `return` at the end of the function is [dead code](#dead-code) that should be omitted.

### Dead Code
All code is "tech debt", although I prefer the term "tech cost".  "Code clutter" degrades both human and LLM work in the codebase by forcing them to read and often remember likely irrelevant information.
- Code should be removed whenever possible.
- Commented-out code should be deleted before code is reviewed (preferably before it is even committed).
- Unreachable code should be removed, or clearly marked in the rare case that immediate removal is not possible.
	- This does not apply to code that only *happens* to be unreachable.  For example, the default case on a switch statement with cases for all defined values isn't considered "unreachable" here if the list of values is owned by another module.  In fact, this default case (usually an error case) would serve as a valuable warning if a new possible value was defined and the switch statement is not updated.

### File Encoding
- All files should be encoded using UTF8.
- All files should have LF line endings.
- All files should end with a newline, so the last line of content is complete (this also makes writing to the end easier).

### Flags
For: `.c`, `.h`

- Flags are always stored in full-length integers, and never as single-bit bitfields (i.e., `Flag:1;`).  This allows bulk editing and passing multiple flags as one parameter.
- Modules should define flag values using macros.  These macros follow the naming scheme `MOD_STRUCT_F_XXX` where "`MOD`" is the module prefix, "`STRUCT`" is a short abbreviation for the structure that the flags serve, and "`XXX`" is the name of the individual flag value itself.  Use these macro values; DO NOT USE MAGIC VALUES.
- Flag comparisons should use `flags & MOD_STRUCT_F_XXX`, which is considered a boolean for the purposes of the [Truthiness](#truthiness) rule.

### Multi-Type Structures
For: `.c`, `.h`

- If a structure can represent kinds or types (not data types, this is a conceptual thing), macros are used to list those types.
- These use the naming scheme: `MOD_STRUCT_T_XXX` (see above [flags](#flags) for more info).


## Error Handling

### Always Check Errors
When calling a function that could fail (including functions that return `int`), you must check for failure.  If a function doesn't promise that it won't error, it could be modified in the future to error, and your code should be ready for that.

### Promise Success When Possible
If a function cannot fail, specify that with its return type.  A `void` return cannot give an error, and an unsigned type (e.g. `unsigned int`) cannot carry `-1` or any other negative failure code.  If the return type cannot express this on its own, *explicitly* document it in the function comment.  This promise frees callers from the need to error-check your function.

### Error Checking Format
Error checks should use guard clauses.  These reduce indentation, and with it, they reduce the amount of context that a programmer has to keep in mind.  In addition, this style allows error checks to be added and removed without reindenting large amounts of code, leading to cleaner git-blame histories.

**Correct**
```c
void* data = nmMalloc(sizeof(ModDataT));
if (data == NULL) goto error;
void* data2 = nmMalloc(sizeof(ModDataT));
if (data2 == NULL) goto error;
void* data3 = nmMalloc(sizeof(ModDataT));
if (data3 == NULL) goto error;
/** Some logic... **/
```

**Incorrect** ("The Pyramid of Doom")
```c
void* data = nmMalloc(sizeof(ModDataT));
if (data != NULL) {
	void* data2 = nmMalloc(sizeof(ModDataT));
	if (data2 != NULL) {
		void* data3 = nmMalloc(sizeof(ModDataT));
		if (data3 != NULL) {
			/** Some logic... **/
		} else goto error;
	} else goto error;
} else goto error;
```

Typically, errors in C code will `goto` an error handler at the end of the current scope or function.  This code is responsible for clearing up memory.  It also usually logs an error message with any helpful info available in that scope so that individual error sites don't need to specify that info.

In C, the error handler follows these rules:
- The handler is labeled `error:` or `end:` (if it also handles a success case), placed at the end of the function or scope, and indented only once (typically the level of the function body).
- The handler's code is indented one level inside its label.

```c
int
ciLoadSource(char* path) {
	pClusterSource src = nmMalloc(sizeof(ClusterSource));
	if (src == NULL) goto error;

	/** Some logic... **/

	return 0;

	error:
		mssError(0, "CI", "Could not load source '%s'", path);
		nmFree(src, sizeof(ClusterSource));
		return -1;
}
```

### If an error occurs...
- Always print an error message if your context can add information about the error.
- Do not print an error message if you are not able to add any new information about an already-detected error.
	- In most cases, you can add new information, but avoid cluttering the error log when you genuinely can't.
- Use `mssError()` to print errors and only to print errors.  (For warnings, use `fprintf(stderr, ...)`).
	- See the [function comment](#function-comments) above the `mssError_internal()` definition in [mtsession.c](../centrallix-lib/src/mtsession.c) for info about calling `mssError()`.
- Use `check.h` shortcut functions only for unlikely library failures (especially memory allocation).
	- These functions do not add values to the error stack so they are invisible to non-console users.

### Recovering From Errors
When you gracefully handle an error, if `mssError()` or similar functions are called (or likely to be called) during an error, call `mssClearError()` to mark the error as handled.  This prevents messages from this error from appearing at the start of the error stack for a later error.


## Comments & Documentation
Code should use a few types of documentation, where applicable. See [comments](#comments) for commenting style.

Any English sentence should use two spaces (or a newline/similar whitespace) between a period and the following sentence.  This makes it easy to visually jump between sentences and skim text.  This is not required for string literals, such as error messages in code, although these rarely have more than a single sentence anyway.

### Warning Comments
- Unintuitive warnings and "foot-guns" should be called out in brief, concise comments.
- These should be very rare, only used if there isn't a better way to communicate the information.

### Section Comments
- Divide code into logical sections that start with brief comments for skimming code.
- Sections should be separated by blank lines.
- These comments should start with a capital letter and end with a period.
- These comments should be concise, only a few words.  They are to make code more skimmable, not to describe every detail.
- Code that is intuitively obvious when skimming (such as variable declarations or simple error checks at the start of a scope) does not need a section comment.
- Avoid overusing section comments when they do not actually make code easier to skim.
- For example:
	```c
	/** Find the parameter. **/
	for (unsigned int i = 0; i < node_data->nParams; i++) {
		const pParam param = check_ptr(node_data->Params[i]);
		if (param == NULL) continue;
		if (strcmp(param->Name, attr_name) != 0) continue;

		/** Parameter found. **/
		return (param->Value == NULL) ? MOD_DATA_T_UNAVAILABLE : param->Value->DataType;
	}
	```

### Function Comments
- Functions should begin with a javadoc-style comment.
- These tell a developer how to call the function without reading its implementation.
- For example:
	```c
	/*** Allocates a new pNodeData struct with data from `inf`.
	 ***
	 *** @param inf A parsed pStructInf, representing the top-level group in a
	 *** 	.cluster structure file.
	 *** @returns A new pNodeData struct on success, or NULL on failure.
	 ***/
	pNodeData
	ci_ParseNodeData(pStructInf inf) {
		...
	}
	```

### `centrallix-sysdoc` Markdown Files
- These document what a system does at a higher level than individual functions.
- For example, they might document how to create a driver, how to use a specific driver, or how a system was architected.
- These files do not document individual functions: use [function comments](#function-comments), because low-level docs far away from the code they document are easily forgotten and can quickly become outdated.

### `centrallix-doc` Markdown Files
- These document how to use the Centrallix language.
- These do not include implementation details (how the system works), which are documented in [`centrallix-sysdoc` Markdown Files](#centrallix-sysdoc-markdown-files).


## Copyright Notices
Also called *license comments*.

All code should begin with a copyright notice using the language's commenting syntax.  Copyright comments use their own style (documented below) which may be different from the general commenting style.

**Exception**: A copyright notice is optional for any file in `centrallix-os`.

Copyright notices should follow these rules:
- The copyright notice is placed as early in the file as possible (to the extent the language allows), even before `#include` or other import statements.  The only exception is the `#ifndef` header in a `.h` file (see [file organization](#file-organization)).
- Copyright notices are 74 characters wide, so a cursor on the right edge of one is in column 75.  This may need to vary slightly depending on the language's comment syntax.
- Use spaces when spacing out a copyright notice, not tabs (see [indentation](#indentation)).

This is the format in C:
```c
/************************************************************************/
/* Centrallix Application Server System                                 */
/* <Subsystem>                                                          */
/*                                                                      */
/* Copyright (C) <created>-<year> LightSys Technology Services, Inc.    */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             */
/* 02111-1307  USA                                                      */
/*                                                                      */
/* A copy of the GNU General Public License has been included in this   */
/* distribution in the file "COPYING".                                  */
/*                                                                      */
/* Module:      <file_name.c>                                           */
/* Author:      <author name>                                           */
/* Date:        <date created, e.g. April 4th, 2003>                    */
/*                                                                      */
/* Description: <Module description>                                    */
/*              <Can wrap to multiple lines>                            */
/************************************************************************/
```
- Replace the values in angle brackets (`<...>`).
- `<Subsystem>` names the part of the project the file belongs to, such as `Centrallix Core` for `centrallix/` or `Centrallix Base Library` for `centrallix-lib/`.  The line above it, `Centrallix Application Server System`, is the same in every file.
- `<created>` is the year the file was created and `<year>` is the year it was last modified, so update the copyright notice when modifying a file with anything more than trivial changes.  When both are the same year, only write that year itself.

### Markdown Files
Markdown files use their own copyright notices which are always placed at the start of the file.  Interestingly enough, these are not comments because the entire markdown file is considered to be documentation so markdown comments are rarely needed.

```md
# <Document Title>

**Author**: <author name>

**Date**: <date created, e.g. June 24th, 2026>

**License**: Copyright (C) <year> LightSys Technology Services.  See `LICENSE`.
```
- Replace the values in angle brackets (`<...>`).  For more info, see above.


## Examples
- `objdrv_cluster.c`: A good example of how to follow these styles, even in complex situations.


## AI Agents
For AI Agents reading this document for the first time, I recommend saving a memory saying where this doc is (so it can be used for reference), as well as brief notes on a couple frequently used styles like Indentation, Braces, Comments, Naming, and Error Handling so you don't have to reread it with every request.


## Todo
Styles that still need to be decided and documented:
- Breaking a long expression or condition across lines: whether the operator ends the broken line or starts the continuation line, and how far continuation lines are indented.
