#include <assert.h>
#include <errno.h>
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
    const char*	    Fmt;	/* format string, uses Arg1 and Arg2 */
    int		    Arg1;	/* first argument */
    int		    Arg2;	/* second argument */
    size_t	    Size;	/* size handed to qpfPrintf() */
    int		    ExpRval;	/* expected return value */
    const char*	    ExpDst;	/* expected contents of dst afterwards */
    }
    Case;

static Case cases[] =
    {
    /**	  Fmt		    Arg1    Arg2    Size    Rval    ExpDst		**/

    /** Zero is false and any nonzero value is true. **/
	{ "%BOOL",	     0,	     0,	     8,	     5,	    "false" },
	{ "%BOOL",	     1,	     0,	     8,	     4,	    "true" },
	{ "%BOOL",	    -1,	     0,	     8,	     4,	    "true" },
	{ "%BOOL",	    12345,   0,	     8,	     4,	    "true" },

    /** Mixed with literal text and with other specifiers. **/
	{ "a=%BOOL;",	     1,	     0,	    16,	     7,	    "a=true;" },
	{ "%BOOL,%BOOL",     0,	     1,	    16,	    10,	    "false,true" },
	{ "%INT:%BOOL",	     7,	     0,	    16,	     7,	    "7:false" },

    /** Truncation still reports the length that the whole output needed. **/
	{ "%BOOL",	     0,	     0,	     5,	     5,	    "fals" },
	{ "%BOOL",	     1,	     0,	     3,	     4,	    "tr" },
	{ "%BOOL",	     1,	     0,	     1,	     4,	    "" },

    /** Conditional printing consumes the argument without printing it. **/
	{ "%[%BOOL%]",	     1,	     0,	     8,	     5,	    "false" },
	{ "%[%BOOL%]",	     0,	     1,	     8,	     0,	    "" },
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

	/*** %BOOL prints "true" for a nonzero int and "false" for zero.  It
	 *** behaves like the other source specifiers with respect to buffer
	 *** truncation, return values, and conditional printing.
	 ***/

	*tname = "qprintf-77 %BOOL conversion";
	iter = 20000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		memset(raw, 0xAA, RAW);
		rval = qpfPrintf(NULL, dst, cases[c].Size, cases[c].Fmt, cases[c].Arg1, cases[c].Arg2);

		/** Return value counts the whole output, even when truncated. **/
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
