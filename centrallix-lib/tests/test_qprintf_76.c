#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	32
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*		Fmt;		/* format string, uses Arg */
    unsigned long long	Arg;		/* %ULL argument */
    size_t		Size;		/* size handed to qpfPrintf() */
    int			ExpRval;	/* expected return value */
    const char*		ExpDst;		/* expected contents of dst afterwards */
    }
    Case;

static Case cases[] =
    {
    /**	  Fmt		Arg			Size	Rval	ExpDst			**/

    /** The whole unsigned range, including values that overflow a long long. **/
	{ "%ULL",	0ull,			32,	 1,	"0" },
	{ "%ULL",	1ull,			32,	 1,	"1" },
	{ "%ULL",	4294967296ull,		32,	10,	"4294967296" },
	{ "%ULL",	9223372036854775807ull,	32,	19,	"9223372036854775807" },
	{ "%ULL",	9223372036854775808ull,	32,	19,	"9223372036854775808" },
	{ "%ULL",	18446744073709551615ull, 32,	20,	"18446744073709551615" },

    /** Mixed with literal text. **/
	{ "n=%ULL;",	42ull,			32,	 5,	"n=42;" },

    /** Truncation still reports the length that the whole output needed. **/
	{ "%ULL",	18446744073709551615ull, 20,	20,	"1844674407370955161" },
	{ "%ULL",	4294967296ull,		 5,	10,	"4294" },
	{ "%ULL",	4294967296ull,		 1,	10,	"" },
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

	/*** %ULL prints an unsigned long long, so it covers the whole 64-bit
	 *** unsigned range rather than wrapping to a negative number the way
	 *** %LL would.  Truncation, return values, and buffer accounting match
	 *** the other source specifiers.
	 ***/

	*tname = "qprintf-76 %ULL conversion";
	iter = 20000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		memset(raw, 0xAA, RAW);
		rval = qpfPrintf(NULL, dst, cases[c].Size, cases[c].Fmt, cases[c].Arg);

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
