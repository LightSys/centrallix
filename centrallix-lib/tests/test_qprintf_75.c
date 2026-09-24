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
    const char*	    Fmt;	/* format string, uses Arg */
    const char*	    Arg;	/* %STR argument */
    size_t	    Size;	/* size handed to qpfPrintf() */
    int		    ExpRval;	/* expected return value */
    const char*	    ExpDst;	/* expected contents of dst afterwards */
    }
    Case;

static Case cases[] =
    {
    /**	  Fmt		Arg			Size	Rval		ExpDst		**/

    /** Far too small, so nothing is written but the terminator. **/
	{ "%STR&B64",	"Example",		 1,	-EINVAL,	"" },
	{ "%STR&B64",	"Example",		11,	-EINVAL,	"" },
	{ "%STR&DHEX",	"4578616d706c65",	 1,	-EINVAL,	"" },
	{ "%STR&DHEX",	"4578616d706c65",	 6,	-EINVAL,	"" },

    /** Exactly full, which leaves the terminator nowhere to go. **/
	{ "%STR&B64",	"Example",		12,	-EINVAL,	"" },
	{ "%STR&DHEX",	"4578616d706c65",	 7,	-EINVAL,	"" },

    /** Room for the result and its terminator. **/
	{ "%STR&B64",	"Example",		13,	12,		"RXhhbXBsZQ==" },
	{ "%STR&DHEX",	"4578616d706c65",	 8,	 7,		"Example" },

    /** Text ahead of the filter takes up room too. **/
	{ "abcde%STR&B64",  "Example",		17,	-EINVAL,	"abcde" },
	{ "abcde%STR&B64",  "Example",		18,	17,		"abcdeRXhhbXBsZQ==" },
	{ "abcde%STR&DHEX", "4578616d706c65",	12,	-EINVAL,	"abcde" },
	{ "abcde%STR&DHEX", "4578616d706c65",	13,	12,		"abcdeExample" },
	{ "abcde%STR&DB64", "RXhhbXBsZQ==",	12,	-EINVAL,	"abcde" },
	{ "abcde%STR&DB64", "RXhhbXBsZQ==",	16,	12,		"abcdeExample" },
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

	/*** These filters write their whole result or none of it, so a buffer
	 *** that is too small is an error rather than a truncation.  A buffer
	 *** the exact length of the result is still too small, since the null
	 *** terminator needs a byte of its own, and text already written ahead
	 *** of the filter takes up room as well.
	 ***/

	*tname = "qprintf-75 &B64, &DB64 and &DHEX buffer accounting";
	iter = 20000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		memset(raw, 0xAA, RAW);
		rval = qpfPrintf(NULL, dst, cases[c].Size, cases[c].Fmt, cases[c].Arg);

		/** Either the whole result or an error. **/
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
