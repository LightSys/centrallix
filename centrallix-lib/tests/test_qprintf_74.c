#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qprintf.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	16
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Fmt;	/* format string, uses Arg */
    const char*	    Arg;	/* %STR argument, "" when unused */
    size_t	    Size;	/* size handed to qpfPrintf_g() */
    }
    Case;

/** Sizes range from far too small to more than enough. **/
static Case cases[] =
    {
    /**	  Fmt		Arg		Size	**/
	{ "plain",	"",		2 },
	{ "plain",	"",		AREA },
	{ "%STR",	"abcdefghijkl",	4 },
	{ "%STR",	"abcdefghijkl",	AREA },
	{ "%STR&QUOT",	"abc",		3 },
	{ "%STR&QUOT",	"abc",		AREA },
	{ "%STR&HTE",	"<tag>",	5 },
    };

long long
test(char** tname)
    {
    int i, c;
    int iter;
    int ncases = sizeof(cases) / sizeof(Case);
    unsigned char raw_null[RAW], raw_nogrow[RAW];
    char* dst_null;
    char* dst_nogrow;
    size_t size_null, size_nogrow;
    int rval_null, rval_nogrow;
    size_t n;

	/*** A NULL grow function means the buffer cannot grow, which is how
	 *** qpf_internal_Translate() and the base64 and hex helpers already
	 *** read it.  Passing NULL must therefore behave exactly like passing
	 *** qpfNoGrow(), not crash.
	 ***/

	*tname = "qprintf-74 qpfPrintf_g() with a NULL grow function";
	iter = 20000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		memset(raw_null, 0xAA, RAW);
		memset(raw_nogrow, 0xAA, RAW);
		dst_null = (char*)raw_null + GUARD;
		dst_nogrow = (char*)raw_nogrow + GUARD;
		size_null = cases[c].Size;
		size_nogrow = cases[c].Size;

		rval_null = qpfPrintf_g(NULL, &dst_null, &size_null, NULL, NULL,
			cases[c].Fmt, cases[c].Arg);
		rval_nogrow = qpfPrintf_g(NULL, &dst_nogrow, &size_nogrow, qpfNoGrow, NULL,
			cases[c].Fmt, cases[c].Arg);

		/** Neither call may move or resize the buffer. **/
		assert(dst_null == (char*)raw_null + GUARD);
		assert(size_null == cases[c].Size);
		assert(size_nogrow == cases[c].Size);

		/** Both calls must agree, byte for byte. **/
		assert(rval_null == rval_nogrow);
		assert(!memcmp(raw_null, raw_nogrow, RAW));

		/** Guard bytes and anything past the size are untouched. **/
		for(n=0;n<GUARD;n++)
		    assert(raw_null[n] == 0xAA);
		for(n=GUARD+cases[c].Size;n<RAW;n++)
		    assert(raw_null[n] == 0xAA);
		}
	    }

    return (long long)iter * ncases;
    }
