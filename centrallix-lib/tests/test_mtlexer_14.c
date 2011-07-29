#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    pLxSession lxs;
    double d;
    static char str[65536] = "";

	*tname = "mtlexer-14 normal/oversized double floating point";

	mssInitialize("system", "", "", 0, "test");

	iter = 20000;
	for(i=0;i<iter;i++)
	    {
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
	    }

    return iter;
    }

