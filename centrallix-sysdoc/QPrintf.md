Document:	QPrintf centrallix utility routines documentation
Date:		January 31, 2006
Author:		Greg Beeley (GRB)
-------------------------------------------------------------------------------

OVERVIEW...

    Centrallix is a modern application server environment that must work
    with a lot of untrusted and semi-trusted data, frequently building
    commands and structures which contain data encapsulated within control
    structures, such as SQL, HTML, JavaScript, and more.

    The goal of this module is to provide an easy, seamless way to safely
    format such strings of data.  Its operation is similar, but not identical,
    to that of the more usual printf() style library calls.


FUNCTIONS...

    Two functions will initially be available:

	qpfPrintf() - quoted formatted printing into a fixed-length buffer.

	qpfPrintf_va() - quoted formatted printing into a fixed-length buffer,
	    where the vararg pointer is explicitly provided in lieu of 
	    passing the arguments directly to the function.

    The calling syntax of these functions is similar to that of the 
    corresponding snprintf() and vsnprintf() calls, but the formatting
    specifiers are different, and an initial "session" parameter can be
    provided - but can be left NULL in most cases where cumulative error
    handling is not needed.


FORMATTING...

    Formatting specifiers will begin with a % sign, as in normal printf().
    However, the specifiers will be very different and can be combined
    together to obtain the desired result.  Formatting specifiers can
    affect a parameter's length, position, padding, content, quoting,
    and more.

    To combine specifiers, simply chain them with the ampersand &.  The
    first one will be applied first to the parameter, and then the second,
    and so forth.

    For non-string values, once the INT, POS, DBL, etc., conversion has been
    performed, further specifiers can be added to further process the string.

    Please check the end of this document for information on which of these
    specifiers are currently implemented.

    Simple specifiers:

	%%	A percent sign.

	%&	An ampersand sign.

	%[	Beginning of conditional printing (an integer argument is
		expected, 0 = noprint, nonzero = print)

	%]	End of conditional printing


    Source data specifiers:

	%INT	An integer value, with range of the normal 'int' value
		in the C language.  Can be positive, negative, or zero.
		
	%LL     A 64-bit integer value, with range of 'long long' value
        in the C language.  Can be positive, negative, or zero.

	%POS	A non-negative integer value (zero allowed).

	%DBL	Double-precision floating point value.

	%STR	A normal zero-terminated string.

	%nSTR	A string of exactly <n> length (binary safe), where <n> is
		an integer supplied in the format string, or if '*', supplied
		as an argument immediately preceding the string pointer.
		Warning:  this does *not* honor null terminators.  Be careful.

	%CHR	A single character.

	%XSTR	An XString.

	%EXP	An Expression tree node.

	%POD	A Pointer-to-object-data.  The type of the POD is specified
		as an argument immediately preceding the string pointer.

	%PTOD	A typed pointer-to-object-data.


    Specifiers used for filtering or processing the data:

	&QUOT	Adds single quotes around the string value if its source type
		was a string or date/time value (esp. useful for expressions
		and pods).

	&DQUOT	Adds double quotes around the string value if its source type
		was a string or date/time value.

	&ESCQ	Causes quotes in the string to be escaped, with single quotes,
		double quotes, and backslashes quoted with a leading backslash.

	&WS	Causes whitespace in the string to be processed into its
		native values (\n -> newline, \r -> carriage return, and \t ->
		horizontal tab).

	&ESCWS	Causes newlines, carriage returns, and tab characters to be
		processed back into escaped representations \n, \r, and \t.

	&ESCSP	Causes spaces to be escaped with a backslash.

	&UNESC	Causes string to be unescaped (backslashes removed and escaped
		values converted to their normal characters).

	&SSYB	Causes single quotes in the string to be doubled, sybase quote
		style ' -> ''

	&DSYB	Causes double quotes in the string to be doubled " -> ""

	&FILE	Presumes that the string is a filename, and thus results in an
		error if the string contains '/' or if the string is solely
		'.' or '..'.

	&PATH	Presumes that the string is a pathname, and so cannot contain
		'..' at the beginning, end, or between two '/' characters.

	&SYM	Treats the string as a 'symbol', beginning with [_a-zA-Z] and
		then containing [_a-zA-Z0-9].  Results in an error if the
		string does not match.

	&HEX	Hex-encodes the entire string.

	&DHEX	Hex-decodes the entire string.

	&B64	Base64-encodes the entire string.

	&DB64	Base64-decodes the entire string.

	&RF/reg/ Filters string value through regular expression, and if it 
		does not match in its entirety, causes an error.

	&RR/reg/rep/ Filters string value through regular expression and 
		replaces occurrences of 'reg' with 'rep'.  It is not an error
		if the regular expression does not match at all.

	&HTE	Converts special HTML characters to HTML entities (includes
		&, <, >, ', and ".

	&DHTE	Converts the above HTML entities back to normal characters.

	&URL	Converts any special characters other than [A-Za-z0-9] into
		%nn where nn is the hex value of the character.

	&DURL	Converts any %nn back to normal characters.

	&nLSET	Space-pads the string on the right (left-align) until there
		are at least n characters in the string.

	&nRSET	Space-pads the string on the left (right-align) until there
		are at least n characters in the string.

	&nZRSET	Zero-pads the string on the left (right-align) until there
		are at least n characters in the string.

	&nLEN	Truncates the string to at most n characters.

	&SQLARG	Makes the argument safe for inclusion in a SQL command as
		a data value.

	&SQLSYM	Makes the argument safe for inclusion in a SQL command as
		a symbol (for example, table or column name).

	&HTDATA	Makes the argument safe for inclusion in an HTML document
		as data, for example between tags or as an attribute of
		a tag.


IMPLEMENTED...

    Here are the currently implemented specifier chains:

	%INT
	%LL
	%POS
	%STR
	%nSTR
	%DBL
	%CHR
	%STR&nLEN
	%STR&SYM
	%STR&SYM&nLEN
	%STR&ESCQ
	%STR&ESCQ&nLEN
	%STR&HTE
	%STR&HTE&nLEN
	%STR&HEX
	%STR&HEX&nLEN
	%STR&QUOT
	%STR&DQUOT
	%STR&FILE
	%STR&FILE&nLEN
	%STR&PATH
	%STR&PATH&nLEN

    All others are unimplemented and will result in a return value of -ENOSYS.

