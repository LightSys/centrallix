# QPrintf centrallix utility routines documentation
Date:		January 31, 2006

Author:		Greg Beeley (GRB)

License:	Copyright (C) 2001-2026 LightSys Technology Services. See LICENSE.txt for more information.

## Overview
Centrallix is a modern application server environment that must work with a lot of untrusted and semi-trusted data, frequently building commands and structures which contain data encapsulated within control structures, such as SQL, HTML, JavaScript, and more.

The goal of this module is to provide an easy, seamless way to safely format such strings of data.  Its operation is similar, but not identical, to that of the more usual printf() style library calls.

## Functions
Two functions will initially be available:

- `qpfPrintf()` - quoted formatted printing into a fixed-length buffer.
- `qpfPrintf_va()` - quoted formatted printing into a fixed-length buffer, where the vararg pointer is explicitly provided in lieu of passing the arguments directly to the function.

The calling syntax of these functions is similar to that of the `snprintf()` and `vsnprintf()` respectively.  However, the formatting specifiers are different, and an initial "session" parameter can be provided to track any errors that occur (this can be left NULL if cumulative error handling is not needed).

The following functions for handling error sessions are also provided:

- `qpfOpenSession()` - Open a new error session.
- `qpfCloseSession()` - Close an error session, freeing all allocated resources.
- `qpfClearErrors()` - Clear errors on an error session so that it can be reused.
- `qpfErrors()` - Get the error mask for the errors that have occurred in an error session.
- `qpfLogErrors()` - Log a helpful message to the console that errors that have occurred in an error session.

## Formatting
Format specifiers begin with the percent character (`%`), similarly to the `printf()` functions. However, the `qprintf()` module uses a new set of format specifers.  The module includes several source format specifiers (e.g. `%STR`, `%INT`, etc.) which indicate what type of data the function should expect so that it can format that data into the output string.  Filter format specifiers (e.g. `%QUOT`, `%PATH`, etc.) can be appended to source format specifiers with the ampersand (`&`) to modify or process the string data, granting control over properties such as its length, position, padding, quoting, and more.  Multiple format specifiers may be added and their effects will be applied in order from left to right.  The module also provides control format specifiers which control the formatted data in other ways.

Some specifiers begin with an `n`, which should be replaced with a number or a wildcard (`*`), in which case the function will expect an int to be provided.  This number usually controlls the behavior of the format specifier in some way.  For example, when using `%nSTR`, the developer may use `%4STR` to indicate a string exactly 4 characters long.

**Warning**: Not all format specifiers below have been implemented yet!  Please check the end of this document for information on which specifiers are implemented so far.

### Control Format Specifiers

| Specifier | Description
| --------- | ------------
| `%%`      | A percent sign.
| `%&`      | An ampersand sign.
| `%[`      | Beginning of conditional printing (expects an integer: 0 = noprint, nonzero = print).
| `%]`      | End of conditional printing.

### Source Format Specifiers

| Specifier | Description
| --------- | ------------
| `%INT`    | An integer value, with range of the normal 'int' value in the C language.  Can be positive, negative, or zero.   
| `%LL`     | A 64-bit integer value, with range of 'long long' value in the C language.  Can be positive, negative, or zero.
| `%POS`    | A non-negative integer value (zero allowed).  Expects a positive int, and the function errors if the int is negative.
| `%DBL`    | A double-precision floating point value.
| `%STR`    | A normal null-terminated string.
| `%nSTR`   | A string of exactly `n` length (binary safe), where `n` is the integer supplied in the format string, or if `*`, supplied as an argument immediately preceding the string pointer. Warning:  This does *not* honor null-terminators.  Be careful.
| `%CHR`    | A single character.
| `%XSTR`   | An XString.
| `%EXP`    | An Expression tree node.
| `%POD`    | A Pointer-to-object-data.  The type of the POD is specified as an argument immediately preceding the string pointer.
| `%PTOD`   | A typed pointer-to-object-data.

### Filter Format Specifiers:

| Specifier         | Description
| ----------------- | ------------
| `&QUOT`           | Adds single quotes around the string value if its source type was a string or DateTime value (esp. useful for expressions and pods).
| `&DQUOT`          | Adds double quotes around the string value if its source type was a string or DateTime value.
| `&ESCQ`           | Escapes quotes (`'"`) and backslashes (`\`) with a leading backslash.
| `&WS`             | Processes whitespace notations into the actual characters (\n -> newline, \r -> carriage return, and \t -> horizontal tab).
| `&ESCWS`          | Escapes newlines, carriage returns, and tab characters into their notations: `\n`, `\r`, and `\t`.
| `&ESCSP`          | Escapes spaces with a leading backslash.
| `&UNESC`          | Unescapes whitespace (backslashes removed and escaped values converted to their normal characters).
| `&SSYB`           | Doubles single quotes, sybase quote style `'` -> `''`
| `&DSYB`           | Doubles double quotes, sybase quote style `"` -> `""`
| `&FILE`           | Ensures the string is a valid filename, giving an error if it contains `'/'` or is only `"."` or `".."`.
| `&PATH`           | Ensures the string is a pathname, giving an error if it has `'..'` at the start, end, or between two `'/'` characters.
| `&SYM`            | Ensures the string is a symbol (starts with `[_a-zA-Z]`, followed by `[_a-zA-Z0-9]`), giving an error if is does not.
| `&HEX`            | **Hex-Encode**s the string (e.g. `"Example"` -> `"4578616d706c65"`).
| `&DHEX`           | **Hex-Decode**s the string (e.g. `"4578616d706c65"` -> `"Example"`).
| `&B64`            | **Base64-Encode**s the entire string (e.g. `"Example"` -> `"RXhhbXBsZQ=="`).
| `&DB64`           | **Base64-Decode**s the entire string (e.g. `"RXhhbXBsZQ=="` -> `"Example"`).
| `&HTE`            | **HTML-Encode**: Escapes special HTML characters into HTML entities (including &, <, >, ', and ") to prevent script injection.
| `&DHTE`           | **HTML-Dencode**: Unescapes HTML entities back to normal characters.
| `&URL`            | **URL-Encode**: Escapes any special characters other than `[A-Za-z0-9]` into `%nn` where `nn` is the character's hex value.
| `&DURL`           | **URL-Decode**: Converts any `%nn` encodings back to normal characters.
| `&RF/reg/`        | Ensures the string matches a regular expression, giving an error if it does not.
| `&RR/reg/rep/`    | Replaces occurrences of the `reg` regular expression with `"rep"`.  Does not give an error if no matches occur.
| `&nLSET`          | Right-pads the string (left-align) with spaces until it has at least `n` characters.
| `&nRSET`          | Left-pads the string (right-align) with spaces until it has at least `n` characters.
| `&nZRSET`         | Left-pads the string (right-align) with zeros until it has at least `n` characters.
| `&nLEN`           | Truncates the string to at most `n` characters.
| `&SQLARG`         | Ensures the string is safe as an SQL data value.
| `&SQLSYM`         | Ensures the string is safe as an SQL symbol (for example, table or column name).
| `&HTDATA`         | Ensures the string is safe as an HTML document as data, for example between tags or as an attribute of a tag.

## Implemented
Below is a list of all implemented specifier chains:

- %INT
- %LL
- %POS
- %STR
- %nSTR
- %DBL
- %CHR
- %STR&nLEN
- %STR&SYM
- %STR&SYM&nLEN
- %STR&ESCQ
- %STR&ESCQ&nLEN
- %STR&HTE
- %STR&HTE&nLEN
- %STR&HEX
- %STR&HEX&nLEN
- %STR&QUOT
- %STR&DQUOT
- %STR&FILE
- %STR&FILE&nLEN
- %STR&PATH
- %STR&PATH&nLEN

All others are unimplemented and may result in a return value of `-ENOSYS` with the `QPF_ERR_T_NOTIMPL` error set.
