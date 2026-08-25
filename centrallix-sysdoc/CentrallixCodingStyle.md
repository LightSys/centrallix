# Centrallix Coding Style

**Author**: Israel Fuller

**Date**: June 24th, 2026

**License**: Copyright (C) 2026 LightSys Technology Services.  See `LICENSE`.


<!-- TODO: Israel - Add table of contents. -->

## Introduction
The number one rule is to write readable code, but "readable code" is an inconsistent and subjective concept, so the rules in this document should help you do that effectively.  The following style rules should be followed in every applicable file in Centrallix.  Note that many files may break style rules, however, any new changes committed should still follow them.

Many of these style rules are enforced by the style linter, however, this file is the source of truth.  Thus, where the lint rules and this file disagree, update the lint rules.  Keep in mind that some styles documented here may not have lint rules due to linter limitations, but they should still be followed to the best of your ability.

<!-- TODO: Israel - Add info about the style linter after it's set up. -->

## Supported Languages
This document should, ideally, provide standards for every text file type and programming language used in Centrallix.  However, it currently only supports the languages listed below:
- `.c`/`.h`
- `.js`
- `.xml`/`.xsl`
- Makefile
- Structure files (`.app`, `.cmp`, `.rpt`, `.qy`, etc.)

When using a language not explicitly covered here, use discernment to attempt to apply these styles within the restrictions of that language so that your code will be styled consistently with other code in the codebase written using supported languages.


## Module Prefixes
All modules have an assigned prefix. This is usually (but not always!) a two-to-four-character abbreviation of the module name.  These prefixes are listed in [Prefixes.md](Prefixes.md).  This prefix is used frequently in identifiers for functions and globals when they are accessible from outside the module.

In this document, we use `PRE` in examples to indicate when an example should include a prefix, such as `PRE_internal_xyz()`.


## Style Rules
All code should follow a consistent style when possible.  This helps developers to read it quickly and accurately without being tripped up by abrupt changes in style or unfamiliar coding practices.

These rules apply to every language in Centrallix, unless a section states otherwise.  A section that only covers certain file types is marked with a `For:` line listing them.  Examples use C, the most common language in the codebase.


### Indentation
- Each level of indentation should use a single tab.  Do not use spaces for general indentation.
- It is highly recommended to set your editor tab length to 4 spaces.  When writing code, you may assume that all readers will view it using 4-space tabs.
- Use spaces for aligning characters, such as in a copyright notice or a column of struct members.
- Lines should not have trailing whitespace.  Thus, blank lines are not indented.
- **Exception**:  Language syntax that requires literal tabs and/or spaces must be followed.

### Spacing
- Language constructs (e.g. `if ()`, `for ()`, `while ()`, `switch ()`, etc.) should always be separated from their parentheses with a space.
- Function calls (e.g. `mssError()`) and function declarations should *not* be separated from their parentheses by a space.
- `sizeof` is an operator, not a function, but it behaves like a function call so it is styled as one: write `sizeof(PreDataT)`, with no space before the parenthesis.
- Pointer types attach the `*` to the type, not to the identifier: `char* p`, not `char *p`.  A pointer is part of the type, so it belongs with the type.
- In C, a pointer must not be declared on the same line as another variable.  For example, `char* a, b;` is not allowed because it's easy to think that `b` is a pointer here and it's not.
- Do not put a space just inside parentheses or brackets.
- Commas and semicolons are followed by a space (or line break), never preceded by one.
- Unary operators, `->`, `.`, `[]`, and casts are written against their operand, e.g. `!found`, `&data`, `node->Name`, `list[i]`, `(char*)ptr`.
- Binary operators and assignments should have a space on either side of the operator.
- The `return` statement should be followed by a space and the return value should only use parentheses when needed.  Do not treat `return` as a function call, it is not a function, and it does not give a return value... well, it kind of does, but I think you get the point :)

### Braces
- Braces use a style similar to the Linux kernel.
	- Braces are placed on the same line as the initiating structure.
	- Braces are terminated at the same indentation as the initiating structure.
- Any `if` statement, `for` loop, `switch` statement, or similar structure must use braces if it contains:
	- More than one line of code.
	- Another such structure.
- It's recommended to include braces even when they are not required, but use your discernment and skip them if it improves readability.
- An `else` or `else if` starts on the line of the closing brace of the block before it, not on the line after that brace.
- For example:
	```c
	if (node->Type == CA_NODE_T_LEAF) { /* Required braces. */
		for (unsigned int i = 0; i < node->nItems; i++) { /* Required braces. */
			if (node->Items[i] == NULL) continue; /* Optional braces (skipped). */
			caFreeItem(node->Items[i]);
		}
	} else if (node->Type == CA_NODE_T_BRANCH) { /* Optional braces (included). */
		caFreeNode(node->Child);
	} else return 0; /* Optional braces (skipped). */
	```
- **Exception**:  In a structure file, braces group data rather than code, so every group is braced regardless of its number of attributes.
	- A group with no child groups may be written on one line, which is sometimes clearer in data-heavy files.
		```
		id "system/querypivot" { type=integer; usage=key; }
		```

### Switch Statements
- `case` and `default` labels are indented one level inside the `switch`.
- Every case body is enclosed in braces, even a body of one line, and the `break` goes inside those braces.
- Fallthrough is allowed, but it must be explicitly marked with a `/** Fallthrough. **/` comment.

For example:
```c
switch (access) {
	case EX_ACCESS_READ_WRITE: {
		can_write = true;
		/** Fallthrough. **/
	}
	case EX_ACCESS_READ: {
		can_read = true;
		break;
	}
	default: {
		mssError(1, "EX", "Unknown access level (%d)", access);
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
- In a `.c` or `.h` file, never use a `//` comment.  C99 permits it, but it does not match with the asterisk tiers above.

**Exception**:  While several languages (e.g. `js`) support C-style multi-line comment syntax, many languages in the codebase do not.  In such languages (listed below), fall back to the specified comment syntax for all comments:
- `xml`/`xsl` files: `<!--` & `-->`
- Makefiles: `#`
- Structure files\*: `//`

\*Fall back only when the parser is configured to not allow C-style multi-line comments.  This is usually the case, but the parser might be configured to support this syntax, so it is still required in those cases.

### File Organization
For: `.c`, `.h`, structure files

In a structure file, `$Version=2$` is always the first line, placed before the copyright notice (optional for files in `centrallix-os`).

In a `.c` or `.h` file, it is recommended to order the top level of the file as follows (for consistency):
1. `#include` guards (in a `.h` file).
2. Copyright notice.
3. `#include` groups.
4. Macros.
5. `Typedef`, `struct`, and `union` declarations.
6. Function declarations.
7. Function definitions (`.c` files).

**Note**: Many files may not have all of these sections.

**Exception**: Some patterns may force you to break this order, such as forward declarations.

### Include Files
For: `.c`, `.h`

- If a `.c` file uses an identifier from a `.h` or C standard library not available by default, that file must `#include` the correct `.h` file or library.
- For example, the `mssError()` function is defined in `mtsession.h`, so that file must be `#include`d by every C file that calls `mssError()`.
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

### Preprocessor Directives
For: `.c`, `.h`

- A directive at the outermost level starts in the first column, even inside a function.
- Nested directives are indented one space per level of nesting, placing the spaces before the `#`.
	- A `.h` file's [include guard](#include-files) does not count as a level, since it wraps the whole file.
- For example:
	```c
	#ifdef HAVE_CONFIG_H
	 #ifndef PRE_MAX_ITEMS
	  #define PRE_MAX_ITEMS 256
	 #endif
	#endif
	```
- A `#define` that spans multiple lines ends every line but the last with a backslash, placed one space after that line's content.
- Do not align the backslashes in a column because a long line added later forces a reflow of every line, clobbering git blame history.
- Continuation lines are indented one level.  A comment may take a continuation line of its own.
- For example:
	```c
	#define PRE_ITEM_BUF_SIZE \
		/** One byte per item, plus a null terminator. **/ \
		(PRE_MAX_ITEMS + 1)
	```

### Naming Identifiers
All identifiers should be spelled correctly and avoid using non-obvious abbreviations.  The very common `i`, `j`, and `rval` local variables are exceptions to this general rule.
- Functions are named with camelCase.
	- The module prefix is prepended if the function is reachable from outside the module. (The prefix is optional for internal & unreachable functions.)
	- Internal functions prepend `_internal_` or `_i_`. (e.g. `xyz_i_hiddenStuff()`)
- Local variables and function parameters are named with snake_case.
- Every global variable in a module lives in a single module-wide global struct named with SCREAMING_SNAKE_CASE with the module name prepended (for easy identification).  The recommended name is `PRE_GLOBALS` (where `PRE` is the module prefix).
- `Typedef` names & structs:
	- `Typedef`ed names use PascalCase (except for the module prefix).
	- Both the struct type (`XxxxYyy`) and a pointer alias (`pXxxxYyy`) are declared in the same typedef.
	- Structs reachable from outside the module start their name with the module prefix.  (As with functions, the prefix is optional for internal structs.)
	- Members of a struct or union use PascalCase.
		- **Exception**:  A count member may take a lowercase `n` prefix, and a pointer member a lowercase `p` prefix, before the PascalCase name.  (e.g. `nData`, `pCluster`)
- In C, value macros are treated as globals, and use SCREAMING_SNAKE_CASE with a module prefix for any values used outside the module.
- In C, function macros are treated as functions, following those styles.
- In a structure file, the name of a group, such as a widget, uses snake_case.

### Types
For: `.c`, `.h`

- C code should work for C99, so it cannot assume the compiler uses any later standards.
	- Note: This means `<stdbool.h>` and variable declarations inside a `for` statement are always available.
- Use `NULL` as the value for null pointers, never `0`.
- Use `bool` with `true` and `false` for boolean values, never an `int` holding `0` or `1`.
- `int` and `unsigned int` are the default integer types.  Use the fixed-width types in `<stdint.h>` (e.g. `uint32_t`) only where the exact width matters, such as data written to a file, a database, or the network.
- A struct must be fully zeroed/initialized when allocated.  Memory a function receives may hold stale data that still looks valid, such as an old `Magic` value or a pointer into live memory.  In C, use `memset()` when the padding of the struct matters, such as when it is written to a file or the network, or compared with `memcmp()`.

For example:
```c
bool auto_seed = false;    /* Not an int holding 0. */
char* name = NULL;         /* Not 0. */
unsigned int n_items = 0;  /* Default integer type. */
uint32_t wire_value;       /* Width matters on the wire. */
```

### Struct & Union Declarations
For: `.c`, `.h`

- A struct or union is declared together with its `typedef` in a single statement (see [naming identifiers](#naming-identifiers) for the names used).
- C syntax requires the `*` of the pointer alias to attach to the name, so this is an exception to the [pointer spacing rule](#spacing).
- Each member is declared on its own line.
- Member names are aligned in a column, padded with spaces (never tabs, see [indentation](#indentation)).
	- Leave a few extra spaces of padding where a longer type may be added later, especially in a struct whose types are all short or which has many members.  This allows adding the type later without a reflow on every line that buries the real change in git-blame.
- If a structure can represent multiple kinds or types (not data types, this is a conceptual thing), it almost always holds a field using a [set type](#set-types) (typically an [enum](#enums)) to indicate the type/kind of the stored data.

For example:
```c
typedef struct _ClusterSource {
	Magic_t         Magic;
	unsigned int    nData;
	char*           Name;
	pVector*        Vectors;
	DateTime        DateCreated;
} ClusterSource, *pClusterSource;
```

#### Magic Structs
If a struct supports `magic.h` by beginning with a magic field of type `Magic_t`, the following rules should be followed:
- When creating the struct, call `SETMAGIC()` with the appropriate magic value (e.g. `MGK_FILE`).
- When a new scope first gains access to the struct (e.g. the first time a function reads and stores a pointer to it) and intends to read any field from it (rather than just passing the pointer to another scope), it must call `ASSERTMAGIC()` immediately (after verifying that the struct is not null, if needed).  No data should be read from the struct or used before `ASSERTMAGIC()` is called.

### Set Types
For: `.c`, `.h`

A set type is a named type that represents [one](#enums) or [multiple](#flags) of a collection of numerical available values, such as the algorithms a module supports or the flags a structure carries.  Every set follows these rules:
- The set type should have a `typedef` specifying a numerical type.  This makes declarations using this type more explicit and obvious.
	- The underlying numerical type should be the minimum size necessary for values defined.
	- Typically, unsigned numerical types like `unsigned char`, `unsigned short`, or `unsigned int` are used.
	- Signed types are less common (especially for flags), however, they have uses when defining some [enum](#enums) values as positive and others as negative communicates useful information, such as positive success values vs. negative error-code values.
	- The `typedef` should be preceded by a brief comment explaining what information is stored with this type.
- Each value is defined using a macro with a literal value cast to the set's type to improve type checking.
	- Values are aligned in a column (see [struct & union declarations](#struct--union-declarations) for the alignment rules, which apply here, too).
	- Macro names use the scheme `PRE_SET_XXX`, where `PRE` is the module prefix, `SET` is a short abbreviation for the set, and `XXX` is the name of the individual value.
	- When a structure could represent one of multiple types, the type being used is typically stored with a set type.
		- The names of value macros for such set types insert a `T` tag: `PRE_STRUCT_T_XXX`.
- Use these macros when interacting with the set.  DO NOT USE MAGIC VALUES.
- A [section comment](#section-comments) above the set says what it represents.

#### Enums
- An enum holds exactly one of its values at a time, so the values simply increment.
- The value `0` should be reserved to mean unset with a value name of `NULL`.
- For example:
	```c
	/** Enum type representing a clustering algorithm. **/
	typedef unsigned char ClusterAlgorithm;
	#define CA_ALG_NULL             ((ClusterAlgorithm)0u)
	#define CA_ALG_NONE             ((ClusterAlgorithm)1u)
	#define CA_ALG_SLIDING_WINDOW   ((ClusterAlgorithm)2u)
	#define CA_ALG_KMEANS           ((ClusterAlgorithm)3u)
	#define CA_ALG_KMEANS_PLUS_PLUS ((ClusterAlgorithm)4u)
	#define CA_ALG_KMEDOIDS         ((ClusterAlgorithm)5u)
	#define CA_ALG_DB_SCAN          ((ClusterAlgorithm)6u)
	```

#### Flags
- A flag set can hold any number of its values at once, so each value is a distinct power of two.
- Flags are always stored in full-length integers, and never as single-bit bitfields (i.e., `Flag:1;`).  This allows bulk editing and passing multiple flags as one parameter.
- Flag comparisons should use `flags & PRE_STRUCT_F_XXX`, which is considered a boolean for the purposes of the [Truthiness](#truthiness) rule.
- Values should be defined in binary (`0b0010`).
	- Leading 0s for all the bits available in the type should be explicitly included to prevent a reflow if new flag values are added later.
- The value `0` should be reserved to mean no flags with a value name of `NONE`.
- For example:
	```c
	/** Flags describing the state of a cluster. **/
	typedef unsigned char ClusterFlags;
	#define CA_CLU_NONE      ((ClusterFlags)0b00000000)
	#define CA_CLU_ALLOCATED ((ClusterFlags)0b00000001)
	#define CA_CLU_SEEDED    ((ClusterFlags)0b00000010)
	#define CA_CLU_CONVERGED ((ClusterFlags)0b00000100)
	#define CA_CLU_DIRTY     ((ClusterFlags)0b00001000)
	#define CA_CLU_READONLY  ((ClusterFlags)0b00010000)
	```

### Line Length
- There's no hard line length limit, but it is recommended to wrap lines at 80 characters.
- Code should rarely be indented more than 3-4 nested blocks within the enclosing function.  For example, if you write an `if` statement in an `if` statement in a `for` loop in an `if` statement, consider refactoring code into a helper function and double check that you're using [guard clauses](#error-checking-format) properly.
- **Exception**:  Markdown files should use longer lines, otherwise reflowing a line in a markdown file can quickly turn a small edit into a huge git-blame.
- **Exception**:  A line of prose in XML, such as the text of a `<p>` or a `<property>` element, is not wrapped for the same reason as Markdown.

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

#### Single Line Calls
Short function calls should also be placed on one line: `getTime(&timer)`.

#### Multiple Line Calls
Function calls with many or long parameters may span multiple lines.
- Literal parameters (especially string literals) may even be split across lines.
- Parameters usually start on the line after the function name.  However, "unimportant" parameters (such as a repeated context variable or output buffer) may be placed on the same line.
- Style rules for function calls are flexible because situations vary, so use discernment and consistency to write the clearest possible code.

The most common example of a complex multi-line function call is the C calls to write inline `HTML` and `CSS`:
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
In this example, multi-line strings are used to format the CSS block in the C code (even though the written CSS uses a single line to optimize HTML length).  Parameters are grouped by their use in the format: `t->id` and `detail_id` are passed on one line because they're both used on the first line of the format while `h` and `z + 1` are passed on separate lines because the format uses them on separate lines.

Note: `ht_flex_format` is a string-literal macro for a very commonly used format string and `ht_flex_w()` is a function macro that expands to satisfy it by passing multiple parameters to the function call.  Because this pattern may be unintuitive, it should be avoided unless strictly necessary.  In this case, it solves the need to duplicate flex format logic possibly 100 times, making modifying it later *much* easier.

Unsurprisingly, writing such calls can clearly be a real struggle!

### Local Variables
- Local variables should be declared as close to where they are used as possible, and especially within the narrowest scope.  This reduces the risk of side effects and the amount of scrolling a programmer must do when reading code.
- Local variables should not be reused for multiple purposes.  Each variable should have a single purpose so it can be precisely named to describe that purpose.
- In C, variables that store pointers to memory allocated during the scope should be declared at the start of the scope and initialized to `NULL`.  That way, if an error occurs, the error handler can check a variable's value to determine if it should be freed without worrying that it might not be declared/initialized at all yet.  See [error handling](#error-handling) for more information.
- Variables should be declared using `const` (or the equivalent keyword for the relevant language) when their value is not changed.  This serves as a note to the reader so they know that the value won't change.
- In `js`, declare with `const`, or with `let` if the value changes.
	- Never use `var`.  It is scoped to the entire function, so it cannot be declared in the narrowest scope.
	- Never assign to a name that was not declared.  This does not create a local variable, it creates a global one that outlives the function and is visible to every other file, possibly causing far-reaching side effects.
- **Exception**:  C dynamic arrays are their own flavor of fun that sometimes require exceptions to the above rules.

### Sensitive Data
For: `.c`, `.h`

- Potentially sensitive data, such as a password, a key, or any other credential, must be shredded before the memory holding it is freed or goes out of scope.
- Use `cxssShred()` in `centrallix` and `cxsecShred()` in `centrallix-lib`.  A plain `memset()` is not enough: the compiler is free to optimize it away if it thinks the memory won't be read.
- Every path out of the scope must shred, including error paths, so the [error handler](#error-checking-format) is usually responsible for shredding.

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

### Equality
For: `.js`

- Compare with `===` and `!==`.  Do not use `==` and `!=`, as loose operators convert their operands before comparing, which surprises readers and hides bugs.
- **Exception**:  `x == null` (or `x != null`) as an idiomatic way to test for `null` and `undefined` together is allowed.

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

### XML
For: `.xml`, `.xsl`

- Every XML file must be well-formed.  Tags must be closed and nested correctly, the file must have exactly one root element, and `&` and `<` must be escaped in text (or the text wrapped in a `<![CDATA[ ]]>` section).  (Note: Any XML-aware editor should easily detect mistakes.)
- Files with declared DTDs or schemas must follow them.  A broken declaration is worse than none: it misleads readers and breaks validating parsers.  Either maintain the grammar or remove the declaration.  (Note: Some editors also validate this, including the [XML VSCode Extension by RedHat](https://marketplace.visualstudio.com/items?itemName=redhat.vscode-xml).)
- XML files always begin with an XML declaration: `<?xml version="1.0" encoding="UTF-8"?>`.
- Attribute values always use double quotes, including those in the XML declaration.
- Element and attribute names use snake_case, like the names of groups in a [structure file](#naming-identifiers).
	- **Exception**:  A name the file does not own is written exactly as the vocabulary that defines it spells it.  This covers `xsl:` elements and their attributes, the HTML a template emits, and any name a DTD or schema requires (if it cannot be updated to allow/require the style specified here).  These names cannot be renamed, so matching them is the only valid choice.
- Content is indented one tab per level of element nesting (as noted in the [indentation](#indentation) section).
	- **Exception**:  Text inside a `<![CDATA[ ]]>` section is data, not markup, so it keeps whatever indentation it needs and does not follow the nesting of the surrounding elements.  See [widgets.xml](../centrallix-doc/Widgets/widgets.xml) for examples.
	- **Exception**:  Literal result-tree markup inside an `xsl:template` (the HTML the template emits) is indented as a block at one level, rather than one level per HTML tag because nesting the output tree inside the template tree adds a lot of indentation.
- Each element starts on its own line.
	- **Exception**:  An element with only text content may be written on one line, e.g. `<property name="align" type="string">Sets the alignment of the text.</property>`.
- An element with no content is self-closed, e.g. `<br/>`, not `<br></br>`.


## Error Handling

### Always Check Errors
When calling a function that could fail (including functions that return `int`), you must check for failure.  If a function doesn't promise that it won't error, it could be modified in the future to error, and your code should be ready for that.

### Promise Success When Possible
If a function cannot fail, specify that with its return type.  A `void` return cannot give an error, and an unsigned type (e.g. `unsigned int`) cannot carry `-1` or any other negative failure code.  If the return type cannot express this on its own, *explicitly* document it in the function comment.  This promise frees callers from the need to error-check your function.

### Error Checking Format
Error checks should use guard clauses.  These reduce indentation, and with it, they reduce the amount of context that a programmer has to keep in mind.  In addition, this style allows error checks to be added and removed without reindenting large amounts of code, leading to cleaner git-blame histories.

**Correct**
```c
void* data1 = NULL;
void* data2 = NULL;
void* data3 = NULL;

data1 = nmMalloc(sizeof(ModDataT));
if (data1 == NULL) goto error;
data2 = nmMalloc(sizeof(ModDataT));
if (data2 == NULL) goto error;
data3 = nmMalloc(sizeof(ModDataT));
if (data3 == NULL) goto error;
/** Some logic... **/
return 0;
```

**Incorrect** ("The Pyramid of Doom")
```c
void* data1;
void* data2;
void* data3;

data1 = nmMalloc(sizeof(ModDataT));
if (data1 != NULL) {
	data2 = nmMalloc(sizeof(ModDataT));
	if (data2 != NULL) {
		data3 = nmMalloc(sizeof(ModDataT));
		if (data3 != NULL) {
			/** Some logic... **/
		} else goto error;
	} else goto error;
} else goto error;
```

Typically, errors in C code will `goto` an error handler at the end of the current scope or function.  This code is responsible for clearing up memory and ensuring that at least one error message was printed, regardless of how the function failed.  Context available anywhere in the function (such as parameters) is typically included in this error message so that every individual error site doesn't need logic to print the same information.

In C, the error handler follows these rules:
- The handler is labeled `error:` or `end:` (if it also handles a success case).
- It is placed at the end of the scope or function.
- The label is not indented at all, but the handler is indented the normal amount for its scope.

### Full Error Handling Example
```c
int
ciInitSearch(char* path) {
	pCluster cluster = NULL;
	pSearch search = NULL;

	cluster = nmMalloc(sizeof(Cluster));
	if (cluster == NULL) goto error;

	/** Some logic... **/

	search = nmMalloc(sizeof(Search));
	if (search == NULL) goto error;

	/** More logic... **/

	/** Success. **/
	return 0;

error:
	mssError(0, "CI", "Failed to initialize search with path: '%s'", path);

	/** Clean up. **/
	if (cluster != NULL) nmFree(cluster, sizeof(Cluster));
	if (search != NULL) nmFree(search, sizeof(Search));

	return -1;
}
```

### If an error occurs...
- Always print an error message if your context can add information about the error.
- Do not print an error message if you are not able to add any new information about an already-detected error.
	- In most cases, you can add new information, but avoid cluttering the error log when you genuinely can't.
- Use `mssError()` to print errors and only to print errors.  (For warnings, use `fprintf(stderr, ...)`).
	- See the [function comment](#function-comments) above the `mssError_internal()` definition in [mtsession.c](../centrallix-lib/src/mtsession.c) for info about calling `mssError()`.

### Recovering From Errors
When you gracefully handle an error, if `mssError()` or similar functions are called (or likely to be called) during an error, call `mssClearError()` to mark the error as handled.  This prevents messages from this error from appearing at the start of the error stack for a later error.


## Comments & Documentation
Code should use a few types of documentation, where applicable.  See [comments](#comments) for commenting style.

Any English sentence should use two spaces (or a newline/similar whitespace) between a period and the following sentence.  This makes it easy to visually jump between sentences and skim text.  This is not required for string literals, such as error messages in code, although these rarely have more than a single sentence anyway.

### Warning Comments
- Warnings for unintuitive code and "foot-guns" should be called out in brief, concise comments.
- These should be very rare, only used if there isn't a better way to communicate the information or rewrite the unintuitive code.

### Section Comments
- Divide code into logical sections that start with brief comments for skimming code.
- Sections should be separated by blank lines.
- Section comments should start with a capital letter.
- Section comments should be concise, only a few words.  They are to make code more skimmable, not to describe every detail.
- Code that is quickly and intuitively obvious when skimming (such as variable declarations or simple error checks at the start of a scope) does not need a section comment.
- For example:
	```c
	/** Find the parameter. **/
	for (unsigned int i = 0; i < node_data->nParams; i++) {
		const pParam param = check_ptr(node_data->Params[i]);
		if (param == NULL) continue;
		if (strcmp(param->Name, attr_name) != 0) continue;

		/** Parameter found. **/
		return (param->Value == NULL) ? PRE_DATA_T_UNAVAILABLE : param->Value->DataType;
	}
	```
- Warning: Avoid overusing section comments when they do not actually make code easier to skim.  Avoid long or overly wordy section comments.  Never write a section comment that says what code *does not do*: section comments should exclusively say what code does do!

### Function Comments
- Functions should begin with a javadoc-style comment.
- These tell a developer how to call the function without reading its implementation.
- The comment opens with the function's name, then ` - `, then the description.
- For example:
	```c
	/*** ci_ParseNodeData - allocates a new pNodeData struct with data from `inf`.
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

**Exception**:  A copyright notice is optional for any file in `centrallix-os`.

Copyright notices should follow these rules:
- The copyright notice is placed as early in the file as possible (to the extent the language allows), even before `#include` or other import statements.  A few things may come first, though, as described in [file organization](#file-organization).
- Copyright notices are 74 characters wide, so a cursor on the right edge of one is in column 75.  This may need to vary slightly depending on the language's comment syntax.
- Use spaces when spacing out a copyright notice, not tabs (see [indentation](#indentation)).

This is the format in C:
```c
/************************************************************************/
/* Centrallix Application Server System                                 */
/* <subsystem>                                                          */
/*                                                                      */
/* Copyright (C) <created>-<changed> LightSys Technology Services, Inc. */
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
- `<subsystem>` names the part of the project the file belongs to, such as `Centrallix Core` for `centrallix/` or `Centrallix Base Library` for `centrallix-lib/`.  The line above it, `Centrallix Application Server System`, is the same in every file.
- `<created>` is the year the file was created and `<changed>` is the year it was last changed, so update the copyright notice when modifying a file with anything more than trivial changes.  When both are the same year, only write that year itself.

### Markdown Files
Markdown files use their own copyright notices which are always placed at the start of the file, not in a comment.  Comments are rarely used in markdown because their typical purpose (hiding inline documentation in code) is unnecessary in a file with only documentation.

```md
# <Document Title>

**Author**: <author name>

**Date**: <date created, e.g. June 24th, 2026>

**License**: Copyright (C) <created>-<changed> LightSys Technology Services.  See `LICENSE`.
```
- Replace the values in angle brackets (`<...>`).  For more info, see above.

### XML & XSL Files
XML files use a single-line notice rather than the full block because these files are considered documentation.  However, this line is still a comment to avoid breaking XML parsing.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- Copyright (C) <created>-<changed> LightSys Technology Services, Inc.  See `LICENSE`. -->
```
- Replace the values in angle brackets (`<...>`).  For more info, see above.
- XML does not allow anything before the XML declaration, so the notice goes on the line just after it.  This is the earliest point the language allows.
- A `<!DOCTYPE>` declaration, if the file has one, follows the notice.


## Examples
- `objdrv_cluster.c`: A good example of how to follow these styles, even in complex situations.


## AI Agents
For AI Agents reading this document for the first time, I recommend saving a memory saying where this doc is (so it can be used for reference), as well as brief notes on a couple frequently used styles like Indentation, Braces, Comments, Naming, and Error Handling so you don't have to reread it with every request.


## Todo - Israel
Styles that still need to be decided and documented:
- How Markdown files are styled?
- How Python files are styled.  The 26 files in `centrallix-ui-test/tests`.  They have no rules today, and several universal rules here do not fit them.
- How should long expressions or multi-line conditions be broken up? Does the line end with or start with the operator, and how far are continuation lines are indented?
- Add rules for `const` with pointers in C:
	- Where is `const` required, if anywhere?
	- What to do about the `pXxxxYyy` aliases, which hide the `const pXxxxYyy` trap.
- How should `js` functions be named?
	- [Naming identifiers](#naming-identifiers) calls for camelCase after the module prefix, but `centrallix-os/sys/js` is 1296 to 188 in favor of the prefix followed by snake_case (e.g. `ca_redraw_year()`).
- Which version of ECMAScript should `js` files assume?
	- Requiring `let` and `const` already sets the floor at ES6 (2015), but the tree also uses arrow functions, spread, `async`, template literals, and `class` without a stated target.
- Decide whether the C `enum` keyword should be used instead of the macro pattern in [constant sets](#constant-sets).  It is allowed for now.
	- `enum` Pros:  The compiler assigns the values, it can warn about an unhandled case in a `switch`, and debuggers show the value's name.
	- Macro Pros:  An `enum`'s underlying type is implementation-defined, so it cannot be sized for a struct member or a wire format, and in C it gives no type checking anyway.
