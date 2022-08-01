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
    int t;
    char* strval;
    int alloc;
    int j;
    int strcnt;
    char str[65536] = "";
    int n_tok = 3;
    int toktype[3] = {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF };
    char* tokstr[6];

	*tname = "mtlexer-09 strings spanning multiple lines";
	setlocale(0, "en_US.UTF-8");

	mssInitialize("system", "", "", 0, "test");

	iter = 6000;

	memset(str, 'a', iter+3);
	tokstr[0] = malloc(iter+2);
	memset(tokstr[0], 'a', iter+1);
	str[0] = '"';
	str[iter+2] = '"';

	for(i=0;i<iter-20;i++)
	    {
	    str[i+1] = '\r';
	    str[i+2] = '\n';
	    tokstr[0][i] = '\r';
	    tokstr[0][i+1] = '\n';
	    str[i+20] = '\n';
	    tokstr[0][i+19] = '\n';
	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		if (t != toktype[j]) printf("Error at iter=%d\n", i);
		assert(t == toktype[j]);
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    alloc = 0;
		    strval = mlxStringVal(lxs, &alloc);
		    assert(strval != NULL);
		    assert(strcnt < 1);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    if (alloc) nmSysFree(strval);
		    }
		}
	    mlxCloseSession(lxs);
	    str[i+1] = 'a';
	    str[i+2] = 'a';
	    tokstr[0][i] = 'a';
	    tokstr[0][i+1] = 'a';
	    str[i+20] = 'a';
	    tokstr[0][i+19] = 'a';
	    }

    return iter;
    }

