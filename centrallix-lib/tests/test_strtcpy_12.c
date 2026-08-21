#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strtcpy.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	24
#define RAW	(GUARD + AREA + GUARD)
#define MAXAPP	3

/*** One append in a chain.  Every format consumes StrArg then IntArg, in
 *** that order, so that the table can drive them all through one call site.
 *** ExpRval and ExpPos pin each append down on its own, so that agreement
 *** between the two entry points is not the only thing being checked.
 ***/
typedef struct
    {
    const char*	    Fmt;
    const char*	    StrArg;
    int		    IntArg;
    int		    ExpRval;
    size_t	    ExpPos;
    }
    Append;

typedef struct
    {
    Append	    Appends[MAXAPP];
    const char*	    ExpFinal;	/* buffer contents once the chain is done */
    }
    Chain;

static Chain chains[] =
    {
    /** Each append is { Fmt, StrArg, IntArg, ExpRval, ExpPos }, then ExpFinal. **/

    /** Plain text accumulated one piece at a time. **/
	{ { {"%s","ab",0, 3,2}, {"%s","cd",0, 3,4}, {"%s","ef",0, 3,6} },
	  "abcdef" },

    /** Mixed conversions, including appends with no text of their own. **/
	{ { {"%s:","key",0, 5,4}, {"%s%d","",42, 3,6}, {"%s","!",0, 2,7} },
	  "key:42!" },
	{ { {"%s[%d]","",-7, 5,4}, {"%s=%d","n",99, 5,8}, {NULL,NULL,0, 0,0} },
	  "[-7]n=99" },

    /** An empty append returns 1 for the null and leaves *pos alone. **/
	{ { {"%s","x",0, 2,1}, {"%s","",0, 1,1}, {"%s","y",0, 2,2} },
	  "xy" },

    /** First append overruns; the rest must be no-ops. **/
	{ { {"%s","ABCDEFGHIJKLMNOPQRSTUVWXYZ01234",0, -24,23},
	    {"%s","tail",0, 0,23}, {"%s%d","",1, 0,23} },
	  "ABCDEFGHIJKLMNOPQRSTUVW" },

    /** Overrun happens partway through the chain instead. **/
	{ { {"%s","01234567890",0, 12,11},
	    {"%s","abcdefghijklm",0, -13,23}, {"%s","z",0, 0,23} },
	  "01234567890abcdefghijkl" },
    };

/** Caller-side varargs wrapper, the way strtcatf_va() is meant to be used. **/
static int
wrapper(char* dst, size_t dstlen, size_t* pos, const char* fmt, ...)
    {
    va_list ap;
    int rval;

	va_start(ap, fmt);
	rval = strtcatf_va(dst, dstlen, pos, fmt, ap);
	va_end(ap);

    return rval;
    }

long long
test(char** tname)
    {
    int i, c, a;
    int iter;
    int nchains = sizeof(chains) / sizeof(Chain);
    int nops;
    unsigned char raw_direct[RAW];
    unsigned char raw_va[RAW];
    char* direct = (char*)raw_direct + GUARD;
    char* through_va = (char*)raw_va + GUARD;
    size_t pos_direct, pos_va;
    int rval_direct, rval_va;
    size_t n;

	/*** This test verifies strtcatf_va() reached through a caller's own
	 *** varargs wrapper, which is the reason the _va entry point exists.
	 *** Chains of appends run through both entry points, and each append
	 *** is checked against its own expected return value and position as
	 *** well as against the other entry point, so that a fault common to
	 *** both cannot hide.  The chains also cover threading *pos across
	 *** calls, formats contributing no text, empty appends, and appending
	 *** onto a buffer that has already overrun.
	 ***/

	*tname = "strtcpy-12 strtcatf_va() parity and chained appends";
	iter = 40000;
	nops = 0;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<nchains;c++)
		{
		memset(raw_direct, 0xAA, RAW);
		memset(raw_va, 0xAA, RAW);
		direct[0] = '\0';
		through_va[0] = '\0';
		pos_direct = 0;
		pos_va = 0;

		for(a=0;a<MAXAPP && chains[c].Appends[a].Fmt;a++)
		    {
		    rval_direct = strtcatf(direct, AREA, &pos_direct,
			chains[c].Appends[a].Fmt, chains[c].Appends[a].StrArg,
			chains[c].Appends[a].IntArg);
		    rval_va = wrapper(through_va, AREA, &pos_va,
			chains[c].Appends[a].Fmt, chains[c].Appends[a].StrArg,
			chains[c].Appends[a].IntArg);

		    /** Each append matches its own expected result. **/
		    assert(rval_direct == chains[c].Appends[a].ExpRval);
		    assert(rval_va == chains[c].Appends[a].ExpRval);
		    assert(pos_direct == chains[c].Appends[a].ExpPos);
		    assert(pos_va == chains[c].Appends[a].ExpPos);

		    /** Both entry points produced the same bytes. **/
		    assert(!strcmp(direct, through_va));

		    /** Position stays on the terminating null. **/
		    assert(pos_va == strlen(through_va));

		    if (i == 0) nops++;
		    }

		/** The chain as a whole produced the expected string. **/
		assert(!strcmp(through_va, chains[c].ExpFinal));

		/** Neither call wrote outside the AREA it was given. **/
		for(n=0;n<GUARD;n++)
		    {
		    assert(raw_direct[n] == 0xAA);
		    assert(raw_va[n] == 0xAA);
		    }
		for(n=GUARD+AREA;n<RAW;n++)
		    {
		    assert(raw_direct[n] == 0xAA);
		    assert(raw_va[n] == 0xAA);
		    }
		}
	    }

    return (long long)iter * nops * 2;
    }
