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

/** Longest keyword the sweep builds. **/
#define MAX_LEN	20000

static char str[65536] = "";

/** Length under test, advanced each pass and wrapped at the longest case. **/
static int sweep = 0;

static bool
doTests(void)
    {
    int i;
    pLxSession lxs;
    char* strval;
    int alloc;

	if (sweep >= MAX_LEN) sweep = 0;
	i = sweep++;

	str[i] = 'a';
	str[i+1] = ' ';
	str[i+2] = 'b';
	str[i+3] = '\0';
	lxs = mlxStringSession(str, 0);
	assert(lxs != NULL);
	if ((i+1) <= 255)
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_KEYWORD);
	    alloc = 0;
	    strval = mlxStringVal(lxs, &alloc);
	    assert(strlen(strval) == i+1);
	    assert(memcmp(strval, str, i+1) == 0);
	    if (alloc) nmSysFree(strval);
	    assert(mlxNextToken(lxs) == MLX_TOK_KEYWORD);
	    strval = mlxStringVal(lxs, NULL);
	    assert(strval != NULL);
	    assert(strcmp(strval, "b") == 0);
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR);
	    }
	else
	    {
	    assert(mlxNextToken(lxs) == MLX_TOK_ERROR); /* keyword too long */
	    }
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-12 normal/oversized keywords";
    mssInitialize("system", "", "", 0, "test");
    return loopTests(doTests);
    }
