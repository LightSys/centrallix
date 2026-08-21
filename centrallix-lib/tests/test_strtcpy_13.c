#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "strtcpy.h"

/** Working area is raw[GUARD] through raw[GUARD+AREA-1], guarded on both sides. **/
#define GUARD	4
#define AREA	8
#define RAW	(GUARD + AREA + GUARD)

/*** A wide character with no representation in the C locale, so that any
 *** attempt to convert it makes vsnprintf() fail with EILSEQ.
 ***/
static wchar_t unconvertible[] = { (wchar_t)0x4E2D, (wchar_t)0 };

/*** Formats that fail partway through, some only after emitting text.  Held
 *** in a volatile pointer so the compiler cannot fold the probe call below.
 ***/
static const char* volatile failing_fmts[] =
    {
    "%ls",
    "ab%ls",
    "%ls%ls",
    "0123456%ls",
    };

/*** Positions at or past the end of dst, as a caller with corrupt state
 *** would supply.  SIZE_MAX also catches a start+1 overflow in the guard.
 ***/
static size_t bad_positions[] =
    {
    AREA - 1,
    AREA,
    AREA + 1,
    AREA + 12,
    (size_t)-1,
    };

long long
test(char** tname)
    {
    int i, c, rval;
    int iter;
    int nfmts = sizeof(failing_fmts) / sizeof(failing_fmts[0]);
    int nbad = sizeof(bad_positions) / sizeof(size_t);
    int ncases;
    int can_fail;
    unsigned char raw[RAW];
    char* dst = (char*)raw + GUARD;
    char probe[32];
    size_t pos;
    size_t n;

	/*** This test verifies that strtcatf() refuses bad input safely,
	 *** covering the two paths ordinary appends never reach.  When a
	 *** conversion fails, the append must be abandoned and the string
	 *** already in dst left intact, even though vsnprintf() may have
	 *** written part of its output first.  A *pos at or past the end of
	 *** dst, including one large enough to overflow the guard's own
	 *** arithmetic, must append nothing.  Both return 0, leave *pos
	 *** alone, and write nothing outside the caller's dstlen.
	 ***/

	*tname = "strtcpy-13 strtcatf() failed conversions and bad positions";

	/** Only run the conversion cases where the platform really fails. **/
	setlocale(LC_ALL, "C");
	can_fail = (snprintf(probe, sizeof(probe), failing_fmts[0], unconvertible) < 0);
	if (!can_fail)
	    printf("(vsnprintf() converts %%ls here, skipping those cases) ");
	ncases = nbad + (can_fail ? nfmts * 2 : 0);

	iter = 40000;
	for(i=0;i<iter;i++)
	    {
	    /** A failed conversion appends nothing and keeps dst intact. **/
	    for(c=0;can_fail && c<nfmts;c++)
		{
		/** Onto an empty dst. **/
		memset(raw, 0xAA, RAW);
		dst[0] = '\0';
		pos = 0;
		rval = strtcatf(dst, AREA, &pos, failing_fmts[c],
		    unconvertible, unconvertible);
		assert(rval == 0);
		assert(pos == 0);
		assert(!strcmp(dst, ""));

		/** Onto existing text, which must survive untouched. **/
		memset(raw, 0xAA, RAW);
		memcpy(dst, "hi", 3);
		pos = 2;
		rval = strtcatf(dst, AREA, &pos, failing_fmts[c],
		    unconvertible, unconvertible);
		assert(rval == 0);
		assert(pos == 2);
		assert(!strcmp(dst, "hi"));

		/** Partial output may remain, but never outside dstlen. **/
		for(n=0;n<GUARD;n++)
		    assert(raw[n] == 0xAA);
		for(n=GUARD+AREA;n<RAW;n++)
		    assert(raw[n] == 0xAA);
		}

	    /** A *pos at or past the end appends nothing at all. **/
	    for(c=0;c<nbad;c++)
		{
		memset(raw, 0xAA, RAW);
		memcpy(dst, "abc", 4);

		pos = bad_positions[c];
		rval = strtcatf(dst, AREA, &pos, "%s", "XYZ");

		/** Nothing appended, and *pos left exactly as it was. **/
		assert(rval == 0);
		assert(pos == bad_positions[c]);
		assert(!strcmp(dst, "abc"));

		/** Not one byte of the buffer may have changed. **/
		for(n=0;n<RAW;n++)
		    assert(raw[n] == (n < GUARD || n > GUARD + 3 ? 0xAA : "abc"[n-GUARD]));
		}
	    }

    return (long long)iter * ncases;
    }
