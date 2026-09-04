#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	24
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Fmt;	/* format string, uses Arg1 and maybe Arg2 */
    const char*	    Arg1;	/* first %STR argument */
    const char*	    Arg2;	/* second %STR argument, "" when unused */
    size_t	    Size;	/* size handed to qpfPrintf() */
    int		    ExpRval;	/* expected return value */
    const char*	    ExpDst;	/* expected contents of dst afterwards */
    }
    Case;

/** Every case puts a quoted string after text that it must not disturb. **/
static Case cases[] =
    {
    /**	  Fmt			Arg1	Arg2	Size	Rval	ExpDst		**/

    /** Room for the terminator only. **/
	{ "x%STR&QUOT",		"abc",	"",	 1,	 6,	"" },

    /** No room for the opening quote: the text ahead of it survives. **/
	{ "x%STR&QUOT",		"abc",	"",	 2,	 6,	"x" },
	{ "[%STR&DQUOT]",	"ab",	"",	 2,	 6,	"[" },
	{ "pre %STR&QUOT post",	"hi",	"",	 5,	13,	"pre " },
	{ "%STR&QUOT %STR&QUOT","one",	"two",	 7,	11,	"'one' " },

    /** Part of the quoted string fits, and the closing quote trims it. **/
	{ "x%STR&QUOT",		"abc",	"",	 5,	 6,	"x'a'" },
	{ "[%STR&DQUOT]",	"ab",	"",	 5,	 6,	"[\"a\"" },
	{ "pre %STR&QUOT post",	"hi",	"",	 8,	13,	"pre 'h'" },
	{ "%STR&QUOT %STR&QUOT","one",	"two",	 9,	11,	"'one' ''" },

    /** Room for everything. **/
	{ "x%STR&QUOT",		"abc",	"",	 7,	 6,	"x'abc'" },
	{ "[%STR&DQUOT]",	"ab",	"",	 7,	 6,	"[\"ab\"]" },
	{ "pre %STR&QUOT post",	"hi",	"",	14,	13,	"pre 'hi' post" },
	{ "%STR&QUOT %STR&QUOT","one",	"two",	12,	11,	"'one' 'two'" },
    };

long long
test(char** tname)
    {
    int i, c, rval;
    int iter;
    int ncases = sizeof(cases) / sizeof(Case);
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;
    size_t n;

	/*** When the buffer runs out, the closing quote is written over the
	 *** end of the quoted string itself.  It must not reach back further
	 *** than that and overwrite output from earlier in the format.  The
	 *** return value is the full length no matter how much fit.
	 ***/

	*tname = "qprintf-72 %STR&QUOT truncation keeps earlier output";
	iter = 10000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		memset(raw, 0xAA, RAW);
		rval = qpfPrintf(NULL, dst, cases[c].Size, cases[c].Fmt, cases[c].Arg1, cases[c].Arg2);

		/** The full length is reported whether or not it all fit. **/
		assert(rval == cases[c].ExpRval);

		/** Contents are as expected, and null-terminated. **/
		assert(!strcmp(dst, cases[c].ExpDst));

		/** Guard bytes and anything past the size are untouched. **/
		for(n=0;n<GUARD;n++)
		    assert(raw[n] == 0xAA);
		for(n=GUARD+cases[c].Size;n<RAW;n++)
		    assert(raw[n] == 0xAA);
		}
	    }

    return (long long)iter * ncases;
    }
