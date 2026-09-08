#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <stdbool.h>
#include "test_utils.h"

/** Longest integer the sweep builds. **/
#define MAX_LEN	20000

static char str[65536] = "";

/*** Length under test, advanced by one on each pass and wrapped once the
 *** longest case has been reached.  A native run walks the whole sweep many
 *** times over; a slower run, such as one under Valgrind, covers a prefix of
 *** it rather than taking proportionally longer.
 ***/
static int sweep = 0;

/** The repunit str currently holds, grown alongside the sweep. **/
static int iv = 0;

static bool
doTests(void)
    {
    int i;
    int n;
    pLxSession lxs;

	if (sweep >= MAX_LEN)
	    {
	    sweep = 0;
	    iv = 0;
	    }
	i = sweep++;

	str[i] = '1';
	str[i+1] = ' ';
	str[i+2] = '2';
	str[i+3] = '\0';
	iv = iv*10 + 1;
	lxs = mlxStringSession(str, 0);
	assert(lxs != NULL);
	if ((i+1) <= 10)
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_INTEGER);
	    n = mlxIntVal(lxs);
	    assert(n == iv);
	    assert(mlxNextToken(lxs) == MLX_TOK_INTEGER);
	    n = mlxIntVal(lxs);
	    assert(n == 2);
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR);
	    }
	else
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR); /* integer too big */
	    }
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-13 normal/oversized integers";
    mssInitialize("system", "", "", 0, "test");
    return loopTests(doTests);
    }
