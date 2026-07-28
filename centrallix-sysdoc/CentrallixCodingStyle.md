# Centrallix Coding Style

**Author**: Israel Fuller

**Date**: June 24th, 2026

**License**: Copyright (C) 2026 LightSys Technology Services.  See `LICENSE`.


## Introduction
The number one rule is to write readable code, but "readable code" is an inconsistent and subjective concept, so the rules in this document should help you do that effectively.  The following style rules should be followed in every applicable file in Centrallix, including `.c`, `.h`, `.md`, `.xml`, structure files, makefiles, etc.  Note that many files may break style rules, however, any new changes committed should still follow them.

Many of these style rules are enforced by the style linter, however, this file is the source of truth.  Thus, where the lint rules and this file disagree, update the lint rules.  Keep in mind that some styles documented here may not have lint rules due to linter limitations, but they should still be followed to the best of your ability.

<!-- TODO: Israel - Add info about the style linter after it's set up. -->

## Module Prefixes
All modules have an assigned prefix. This is usually (but not always!) a two-to-four-character abbreviation of the module name.  These prefixes are listed in [Prefixes.md](Prefixes.md).  This prefix is used frequently in identifiers for functions and globals when they are accessible from outside the module.


## Style Rules
All code should follow a consistent style when possible.  This helps developers to read it quickly and accurately without being tripped up by abrupt changes in style or unfamiliar coding practices.

These rules apply to every language in Centrallix, unless a section states otherwise.  A section that only covers certain file types is marked with a `For:` line listing them.  Examples use C, the most common language in the codebase.


### Indentation
- Code is always indented using 1 tab, not with spaces.
- You can set your editor tab length to any size you prefer.  However, code should look reasonable with any tab length up to 8 spaces.
- Prefer tabs, but always use spaces for aligning characters, such as in a copyright notice or a column of struct members.  Tabs break alignment when the viewer uses a different tab length.
- Lines should not have trailing whitespace.  Thus, blank lines are not indented.
- In a makefile, the tab that begins a recipe line is required by the syntax, so it must be a literal tab.  Replacing it with spaces breaks the build.

### Spacing
- Language constructs (e.g. `if ()`, `for ()`, `while ()`, `switch ()`, etc.) should always be separated from their parentheses with a space.
- Function calls (e.g. `mssError()`) and function declarations should *not* be separated from their parentheses by a space.
- Pointer types attach the `*` to the type, not to the identifier: `char* p`, not `char *p`.  A pointer is part of the type, so it belongs with the type.
- Do not put a space just inside parentheses or brackets.
- Commas and semicolons are followed by a space (or line break), never preceded by one.
- Unary operators, `->`, `.`, `[]`, and casts are written against their operand, e.g. `!found`, `&data`, `node->Name`, `list[i]`, `(char*)ptr`.
- In a makefile, an assignment is spaced like any other binary operator, e.g. `CC = @CC@`.
- The `return` statement should be followed by a space and the return value should only use parentheses when needed.  Do not treat `return` as a function call, it is not a function, and it does not give a return value... well, it kind of does, but I think you get the point :)

### Braces
- Braces use a style similar to the Linux kernel.
	- Braces are placed on the same line as the initiating structure.
	- Braces are terminated at the same indentation as the initiating structure.
- Any `if` statement, `for` loop, `switch` statement, or similar structure must use braces if it contains:
	- More than one line of code.
	- Another such structure.
- An `else` or `else if` starts on the line of the closing brace of the block before it, not on the line after that brace.
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
- **Exception**:  A file may only use the comment syntax its language accepts, such as `//` in an `.app` file, `#` in a makefile, or `<!-- -->` in XML.  Follow the rules above to the extent that syntax allows.  XML has no single-line comment syntax and cannot nest comments, so the asterisk tiers above do not apply to it at all.

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

### Preprocessor Directives
For: `.c`, `.h`

- A directive at the outermost level starts in the first column, even inside a function.
- Nested directives are indented one space per level of nesting, placing the spaces before the `#`.  A `.h` file's [include guard](#include-files) does not count as a level, since it wraps the whole file.
	```c
	#ifdef HAVE_CONFIG_H
	 #ifndef MOD_MAX_ITEMS
	  #define MOD_MAX_ITEMS 256
	 #endif
	#endif
	```
- A `#define` that spans multiple lines ends every line but the last with a backslash, placed one space after that line's content.  Do not align the backslashes in a column because one long line added later would force a reflow of every line.
- Continuation lines are indented one level.  A comment may take a continuation line of its own.
	```c
	#define PRT_HTMLFM_EMAIL_CONTENT_HEADER "\n" \
		"--"PRT_HTMLFM_EMAIL_BOUNDARY"\n" \
		/** Report data (e.g. donor names) may contain raw UTF-8 octets >127. **/ \
		"Content-Type: text/html; charset=utf-8\n" \
		"Content-Transfer-Encoding: 8bit\n" \
		"\n"
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
		- **Exception**:  A count member may take a lowercase `n` prefix, and a pointer member a lowercase `p` prefix, before the PascalCase name.  (e.g. `nData`, `pCluster`)
- In C, value macros are treated as globals and follow the naming style of the global struct.
- In C, function macros are treated as functions, following those styles.
- In a structure file, the name of a group, such as a widget, uses snake_case.

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
	unsigned int     nData;
	char*            Name;
	pVector*         Vectors;
	DateTime         DateCreated;
} ClusterSource, *pClusterSource;
```

#### Magic Structs
If a struct supports `magic.h` by beginning with a magic field of type `Magic_t`, the following rules should be followed:
- When creating the struct, call `SETMAGIC()` with the appropriate magic value (e.g. `MGK_FILE`).
- When a new scope first gains access to the struct (e.g. the first time a function reads and stores a pointer to it) and intends to read any field from it (rather than just passing the pointer to another scope), it must call `ASSERTMAGIC()` immediately (after verifying that the struct is not null, if needed).  No data should be read from the struct or used before `ASSERTMAGIC()` is called.

### File Organization
For: `.c`, `.h`, structure files

In a structure file, `$Version=2$` is always the first line, placed before the copyright notice (optional for files in `centrallix-os`).

In a `.c` or `.h` file, it is recommended to order the top level of the file as follows, for consistency with other files, unless there is a good reason to do otherwise (such as a forward reference):
1. `#include` guards (in a `.h` file).
2. Copyright notice.
3. `#include` groups.
4. Macros.
5. `Typedef`s, structs, and unions.
6. Prototypes for functions defined later in the same file.
7. Function definitions (`.c` files).

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
- In `js`, declare with `const`, or with `let` when the value changes.
	- Never use `var`.  It is scoped to the entire function, so it cannot be declared in the narrowest scope.
	- Never assign to a name that was not declared.  This does not create a local variable, it creates a global one that outlives the function and is visible to every other file, possibly causing far-reaching side-effects.
- A struct must be fully zeroed/initialized when allocated.  Memory a function receives may hold stale data that still looks valid, such as an old `Magic` value or a pointer into live memory.  In C, use `memset()` when the struct's padding matters, such as when it is written to a file or the network, or compared with `memcmp()`.
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
For: `js`

- Compare with `===` and `!==`, not `==` and `!=`.  The loose operators convert their operands before comparing, which surprises readers and hides bugs.
- **Exception**:  `x == null` (or `x != null`) as an idiomatic way to test for `null` and `undefined` together is allowed.

### XML
For: `.xml`, `.xsl`

- Every XML file must be well-formed.  Tags must be closed and nested correctly, the file must have exactly one root element, and `&` and `<` must be escaped in text (or the text wrapped in a `<![CDATA[ ]]>` section).  (Note: Any XML-aware editor should easily detect mistakes.)
- Files with declared DTDs or schemas must follow them.  A broken declaration is worse than none: it misleads readers and breaks validating parsers.  Either maintain the grammar or remove the declaration.  (Note: Some editors also validate this, including the [XML VSCode Extension by RedHat](https://marketplace.visualstudio.com/items?itemName=redhat.vscode-xml).)
- XML files always begin with an XML declaration: `<?xml version="1.0" encoding="UTF-8"?>`.
- Attribute values always use double quotes, including those in the XML declaration.
- Element and attribute names use snake_case, like the names of groups in a [structure file](#naming-identifiers).
- Content is indented one tab per level of element nesting (as noted in the [indentation](#indentation) section).
	- **Exception**:  Text inside a `<![CDATA[ ]]>` section is data, not markup, so it keeps whatever indentation it needs and does not follow the nesting of the elements around it.  See [widgets.xml](../centrallix-doc/Widgets/widgets.xml) for examples.
	- **Exception**:  Literal result-tree markup inside an `xsl:template` (the HTML the template emits) is indented as a block at one level, rather than one level per HTML tag because nesting the output tree inside the template tree adds a lot of indentation.
- Each element starts on its own line.
	- **Exception**:  An element with only text content may be written on one line, e.g. `<property name="align" type="string">Sets the alignment of the text.</property>`.
- An element with no content is self-closed, e.g. `<br/>`, not `<br></br>`.

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

### Constant Sets
For: `.c`, `.h`

A constant set is a named group of numerical values, such as the algorithms a module supports or the flags a structure carries.  Every set follows these rules:
- The set should have a `typedef` of an unsigned numerical type, so declarations using this type will show what the value means.
  - The underlying numerical type should be the minimum size necessary for the number of values defined.
	- Valid numerical types include at least: `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`, and possibly others.  Unsigned types are required to prevent signed issues.
- Each value is defined as a macro, cast to the set's type with an unsigned literal.  The cast carries the type into comparisons and `switch` statements, which a bare number would not.
- Values are aligned in a column (see [struct & union declarations](#struct--union-declarations) for the alignment rules, which apply here too).
- Macro names use the scheme `MOD_SET_XXX`, where "`MOD`" is the module prefix, "`SET`" is a short abbreviation for the set, and "`XXX`" is the name of the individual value.
- Use these macros when interacting with the set.  DO NOT USE MAGIC VALUES.
- A [section comment](#section-comments) above the set says what it represents.

#### Enums
- An enum holds exactly one of its values at a time, so the values simply increment.
- The value `0` is reserved to mean unset, and is named `MOD_SET_NULL`.
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
- Flag names insert an `F` tag: `MOD_STRUCT_F_XXX`, where "`STRUCT`" is the structure the flags serve.
- Flag comparisons should use `flags & MOD_STRUCT_F_XXX`, which is considered a boolean for the purposes of the [Truthiness](#truthiness) rule.

#### Multi-Type Structures
- If a structure can represent kinds or types (not data types, this is a conceptual thing), an enum lists those types.
- The struct almost always includes a field holding this enum to indicate the type that the struct holds. 
- These names insert a `T` tag: `MOD_STRUCT_T_XXX`.


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

**Exception**:  A copyright notice is optional for any file in `centrallix-os`.

Copyright notices should follow these rules:
- The copyright notice is placed as early in the file as possible (to the extent the language allows), even before `#include` or other import statements.  A few things may come first, though, as described in [file organization](#file-organization).
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

### XML & XSL Files
XML files use a single-line notice rather than the full block because these files are also considered documentation, like [Markdown files](#markdown-files).

```xml
<?xml version="1.0"?>
<!-- Copyright (C) <created>-<year> LightSys Technology Services, Inc.  See `LICENSE`. -->
```
- Replace the values in angle brackets (`<...>`).  For more info, see above.
- XML does not allow anything before the XML declaration, so the notice goes on the line just after it.  This is the earliest point the language allows.
- A `<!DOCTYPE>` declaration, if the file has one, follows the notice.


## Examples
- `objdrv_cluster.c`: A good example of how to follow these styles, even in complex situations.


## AI Agents
For AI Agents reading this document for the first time, I recommend saving a memory saying where this doc is (so it can be used for reference), as well as brief notes on a couple frequently used styles like Indentation, Braces, Comments, Naming, and Error Handling so you don't have to reread it with every request.


## Todo
Styles that still need to be decided and documented:
- How Python files are styled.  The 26 files in `centrallix-ui-test/tests` have no rules today, and several here do not fit them.
- How Markdown files are styled, beyond their [copyright notice](#markdown-files) and the two sections on the `centrallix-doc` and `centrallix-sysdoc` trees.
- Decide how to break up a long expression or condition across lines: whether the operator ends the broken line or starts the continuation line, and how far continuation lines are indented.
- Add rules for `const` with pointers in C:
	- Where `const` is required, if anywhere.  `const char* p` protects the data (visible to callers, propagates through the type system), while `char* const p` only protects the variable (invisible outside the function).  The two are independent.
	- What to do about the `pXxxxYyy` aliases.  They hide the `*`, so `const pClusterSource` is a const *pointer* with freely mutable members, which is the opposite of what most readers expect, and there is no way to spell const *data* through an alias (that needs `const ClusterSource*`).  Only 2 sites in the tree write `const pXxx` today, so this is still cheap to settle.
	- Note: spelling is not in question.  `const char*` and `char const*` are identical to the compiler, and the tree is 134 to 0 in favor of `const char*`, which also matches the [pointer spacing rule](#spacing).
- How `js` functions are named.
	- [Naming identifiers](#naming-identifiers) calls for camelCase after the module prefix, but `centrallix-os/sys/js` is 1296 to 188 in favor of the prefix followed by snake_case (e.g. `ca_redraw_year()`).
	- Which version of ECMAScript `js` files may assume?  Requiring `let` and `const` already sets the floor at ES6 (2015), but the tree also uses arrow functions, spread, `async`, template literals, and `class` without a stated target.
- Decide whether the C `enum` keyword should be used instead of the macro pattern in [constant sets](#constant-sets).  It is allowed for now.
	- `enum` Pros:  The compiler assigns the values, it can warn about an unhandled case in a `switch`, and debuggers show the value's name.
	- Macro Pattern Pros:  An `enum`'s underlying type is implementation-defined, so it cannot be sized for a struct member or a wire format, and in C it gives no type checking anyway.
