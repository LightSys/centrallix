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
    const char*	    Arg1;	/* first %STR argument, "" when unused */
    const char*	    Arg2;	/* second %STR argument, "" when unused */
    size_t	    Size;	/* size handed to qpfPrintf() */
    int		    ExpRval;	/* expected return value */
    const char*	    ExpDst;	/* expected contents of dst afterwards */
    }
    Case;

/** Every case escapes a literal '%' or '&' somewhere in the output. **/
static Case cases[] =
    {
    /**	  Fmt		Arg1	Arg2	Size	Rval	ExpDst		**/

    /** No room for the escaped character. **/
	{ "a%%b%&c",	"",	"",	 2,	5,	"a" },
	{ "%%%%",	"",	"",	 1,	2,	"" },
	{ "%STR%&%STR",	"a",	"b",	 2,	3,	"a" },
	{ "100%% %STR",	"done",	"",	 4,	9,	"100" },

    /** Room for the first escaped character only. **/
	{ "a%%b%&c",	"",	"",	 4,	5,	"a%b" },
	{ "%%%%",	"",	"",	 2,	2,	"%" },

    /** Room for everything. **/
	{ "a%%b%&c",	"",	"",	 6,	5,	"a%b&c" },
	{ "%%%%",	"",	"",	 3,	2,	"%%" },
	{ "%STR%&%STR",	"a",	"b",	 4,	3,	"a&b" },
	{ "100%% %STR",	"done",	"",	10,	9,	"100% done" },
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

	/*** An escaped '%' or '&' that does not fit still counts toward the
	 *** return value, like every other truncated part of the output does.
	 *** Callers such as htr_internal_QPAddText() compare the return value
	 *** against the buffer size to detect truncation.
	 ***/

	*tname = "qprintf-73 %% and %& count toward the return value";
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
