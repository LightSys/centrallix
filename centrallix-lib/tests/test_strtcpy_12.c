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
#define MAXAPP	8

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

/*** A chain of appends run end to end into one buffer.  DstLen is the size
 *** handed to strtcatf(), which is deliberately smaller than AREA in places
 *** so that a chain can overrun early rather than only at the far end.
 ***/
typedef struct
    {
    const char*	    Prefix;	/* seeded into dst before the chain runs */
    size_t	    DstLen;
    Append	    Appends[MAXAPP];
    const char*	    ExpFinal;
    }
    Chain;

static Chain chains[] =
    {
    /** Prefix, DstLen, then { Fmt, StrArg, IntArg, ExpRval, ExpPos } each. **/

    /** A single append, the shortest chain there is. **/
	{ "", AREA,
	  { {"%s","solo",0, 5,4} },
	  "solo" },

    /** Two appends, the second contributing no text of its own. **/
	{ "", AREA,
	  { {"%s:","key",0, 5,4}, {"%s%d","",42, 3,6} },
	  "key:42" },

    /** Three, mixing conversions that do and do not take a string. **/
	{ "", AREA,
	  { {"%s[%d]","",-7, 5,4}, {"%s=%d","n",99, 5,8}, {"%s","!",0, 2,9} },
	  "[-7]n=99!" },

    /** Five appends of growing length, threading *pos the whole way. **/
	{ "", AREA,
	  { {"%s","a",0, 2,1}, {"%s","bb",0, 3,3}, {"%s","ccc",0, 4,6},
	    {"%s","dddd",0, 5,10}, {"%s","eeeee",0, 6,15} },
	  "abbcccddddeeeee" },

    /** Eight appends: fills the buffer exactly, then the last one overruns. **/
	{ "", AREA,
	  { {"%s","abc",0, 4,3}, {"%s","abc",0, 4,6}, {"%s","abc",0, 4,9},
	    {"%s","abc",0, 4,12}, {"%s","abc",0, 4,15}, {"%s","abc",0, 4,18},
	    {"%s","abc",0, 4,21}, {"%s","abc",0, -3,23} },
	  "abcabcabcabcabcabcabcab" },

    /** A tiny buffer, so the very first append overruns and the rest are no-ops. **/
	{ "", 4,
	  { {"%s","hello",0, -4,3}, {"%s","x",0, 0,3}, {"%s","y",0, 0,3} },
	  "hel" },

    /** Resuming onto text already in dst, the way an error message is built. **/
	{ "start:", 16,
	  { {"%s","ab",0, 3,8}, {"%s%d","",1234, 5,12}, {"%s","xyz",0, 4,15},
	    {"%s","Q",0, 0,15} },
	  "start:ab1234xyz" },

    /** Empty appends at the front and the back return 1 and do not move *pos. **/
	{ "", AREA,
	  { {"%s","",0, 1,0}, {"%s","mid",0, 4,3}, {"%s","",0, 1,3} },
	  "mid" },
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
    size_t prefixlen;
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
	 *** both cannot hide.  Chain lengths run from one append to eight, and
	 *** DstLen varies per chain, so that a buffer can fill up early in a
	 *** chain, exactly at its end, or not at all.  The chains also cover
	 *** resuming onto text already in dst, formats contributing no text,
	 *** and empty appends at the front and back of a chain.
	 ***/

	*tname = "strtcpy-12 strtcatf_va() parity and chained appends";
	iter = 40000;
	nops = 0;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<nchains;c++)
		{
		/** Seed both buffers with the chain's starting text. **/
		prefixlen = strlen(chains[c].Prefix);
		memset(raw_direct, 0xAA, RAW);
		memset(raw_va, 0xAA, RAW);
		memcpy(direct, chains[c].Prefix, prefixlen + 1);
		memcpy(through_va, chains[c].Prefix, prefixlen + 1);
		pos_direct = prefixlen;
		pos_va = prefixlen;

		for(a=0;a<MAXAPP && chains[c].Appends[a].Fmt;a++)
		    {
		    rval_direct = strtcatf(direct, chains[c].DstLen, &pos_direct,
			chains[c].Appends[a].Fmt, chains[c].Appends[a].StrArg,
			chains[c].Appends[a].IntArg);
		    rval_va = wrapper(through_va, chains[c].DstLen, &pos_va,
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

		/** Neither call wrote outside the DstLen it was given. **/
		for(n=0;n<GUARD;n++)
		    {
		    assert(raw_direct[n] == 0xAA);
		    assert(raw_va[n] == 0xAA);
		    }
		for(n=GUARD+chains[c].DstLen;n<RAW;n++)
		    {
		    assert(raw_direct[n] == 0xAA);
		    assert(raw_va[n] == 0xAA);
		    }
		}
	    }

    return (long long)iter * nops * 2;
    }
