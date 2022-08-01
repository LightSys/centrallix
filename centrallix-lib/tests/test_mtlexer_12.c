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
    int iter;
    pLxSession lxs;
    char* strval;
    int alloc;
    char str[65536] = "";

	*tname = "mtlexer-12 normal/oversized keywords";
	setlocale(0, "en_US.UTF-8");

	mssInitialize("system", "", "", 0, "test");

	iter = 20000;
	for(i=0;i<iter;i++)
	    {
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
	    }

    return iter;
    }

