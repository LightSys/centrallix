#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>

long long
test(char** tname)
    {
    int i;
    int iv;
    int iter;
    pLxSession lxs;
    int n;
    char str[65536] = "";

	*tname = "mtlexer-13 normal/oversized integers";
	setlocale(0, "en_US.UTF-8");

	mssInitialize("system", "", "", 0, "test");

	iter = 20000;
	iv=0;
	for(i=0;i<iter;i++)
	    {
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
	    }

    return iter;
    }

