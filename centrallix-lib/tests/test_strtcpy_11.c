#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strtcpy.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	8
#define RAW	(GUARD + AREA + GUARD)

typedef struct
    {
    const char*	    Prefix;	/* seeded into dst before the call */
    const char*	    Src;	/* appended with a "%s" format */
    size_t	    DstLen;	/* size handed to strtcatf() */
    int		    ExpRval;	/* expected return value */
    const char*	    ExpDst;	/* expected contents of dst afterwards */
    size_t	    ExpPos;	/* expected *pos afterwards */
    }
    Case;

/** Every append is "%s" of Src onto Prefix, into an AREA-byte area. **/
static Case cases[] =
    {
    /**	  Prefix	Src		DstLen	Rval	ExpDst		ExpPos  **/

    /** Nothing to append. **/
	{ "",		"",		AREA,	 1,	"",		0 },
	{ "hi",		"",		AREA,	 1,	"hi",		2 },

    /** Ordinary appends with room to spare. **/
	{ "",		"abc",		AREA,	 4,	"abc",		3 },
	{ "hi",		"abc",		AREA,	 4,	"hiabc",	5 },

    /** Exact fit: last char lands in dst[DstLen-2], null in dst[DstLen-1]. **/
	{ "",		"1234567",	AREA,	 8,	"1234567",	7 },
	{ "hi",		"12345",	AREA,	 6,	"hi12345",	7 },

    /** One byte too long: truncates, returns -(bytes appended incl. null). **/
	{ "",		"12345678",	AREA,	-8,	"1234567",	7 },
	{ "hi",		"123456",	AREA,	-6,	"hi12345",	7 },

    /** Far too long: same result as one byte too long. **/
	{ "",		"123456789012",	AREA,	-8,	"1234567",	7 },
	{ "hi",		"123456789012",	AREA,	-6,	"hi12345",	7 },

    /** Exactly one byte of room left before the null. **/
	{ "123456",	"x",		AREA,	 2,	"123456x",	7 },
	{ "123456",	"xy",		AREA,	-2,	"123456x",	7 },

    /** Buffer already full: no room for anything, not even one char. **/
	{ "1234567",	"x",		AREA,	 0,	"1234567",	7 },

    /** Degenerate sizes must not write at all. **/
	{ "",		"abc",		0,	 0,	"",		0 },
	{ "",		"abc",		1,	 0,	"",		0 },

    /** Smallest size that can hold a character. **/
	{ "",		"a",		2,	 2,	"a",		1 },
	{ "",		"ab",		2,	-2,	"a",		1 },
    };

long long
test(char** tname)
    {
    int i, c, rval;
    int iter;
    int ncases = sizeof(cases) / sizeof(Case);
    unsigned char raw[RAW];
    unsigned char snapshot[RAW];
    char* dst = (char*)raw + GUARD;
    size_t pos;
    size_t n;

	/*** This test verifies strtcatf() over a table of buffer sizes and
	 *** append lengths, checking the four things the function promises:
	 *** the strtcat() return convention (bytes appended including the
	 *** null terminator, negated when truncated, zero when nothing fits),
	 *** that *pos is left on the terminating null, that dst is always
	 *** null-terminated, and that no byte outside the caller's declared
	 *** DstLen is ever touched.
	 ***/

	*tname = "strtcpy-11 strtcatf() return values and buffer bounds";
	iter = 40000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		/** Fill the whole area, then seed the prefix over the front. **/
		memset(raw, 0xAA, RAW);
		memcpy(dst, cases[c].Prefix, strlen(cases[c].Prefix) + 1);
		memcpy(snapshot, raw, RAW);

		/** Append onto the end of the seeded prefix. **/
		pos = strlen(cases[c].Prefix);
		rval = strtcatf(dst, cases[c].DstLen, &pos, "%s", cases[c].Src);

		/** Return value follows the strtcat() convention. **/
		assert(rval == cases[c].ExpRval);

		/** Contents and write position are both as expected. **/
		assert(!strcmp(dst, cases[c].ExpDst));
		assert(pos == cases[c].ExpPos);

		/** *pos always lands on the terminating null. **/
		assert(pos == strlen(dst));

		/** Leading guard bytes must not be clobbered. **/
		for(n=0;n<GUARD;n++)
		    assert(raw[n] == 0xAA);

		/** Nothing at or past DstLen may be touched either. **/
		for(n=GUARD+cases[c].DstLen;n<RAW;n++)
		    assert(raw[n] == snapshot[n]);
		}
	    }

    return (long long)iter * ncases;
    }
