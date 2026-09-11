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

/** Longest number the sweep builds. **/
#define MAX_LEN	20000

static char str[65536] = "";

/** Length under test, advanced each pass and wrapped at the longest case. **/
static int sweep = 0;

static bool
doTest(void)
    {
    int i;
    double d;
    pLxSession lxs;

	if (sweep >= MAX_LEN) sweep = 0;
	i = sweep++;

	str[i] = '1';
	str[i+1] = '.';
	str[i+2] = '1';
	str[i+3] = ' ';
	str[i+4] = '2';
	str[i+5] = '.';
	str[i+6] = '2';
	str[i+7] = '\0';
	lxs = mlxStringSession(str, 0);
	assert(lxs != NULL);
	if ((i+1) <= 253)
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_DOUBLE);
	    d = mlxDoubleVal(lxs);
	    assert(mlxNextToken(lxs) == MLX_TOK_DOUBLE);
	    d = mlxDoubleVal(lxs);
	    assert(d == 2.2);
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR);
	    }
	else
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR); /* number too big */
	    }
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-14 normal/oversized double floating point";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest);
    }
