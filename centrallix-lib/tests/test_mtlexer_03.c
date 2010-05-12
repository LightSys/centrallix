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
    int t;
    char* strval;
    int alloc;
    int j;
    int strcnt;
    char str[65536] = "";
    int n_tok = 7;
    int toktype[9] = {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF };
    char* tokstr[6];

	*tname = "mtlexer-03 BID#156 - line length based failure";

	mssInitialize("system", "", "", 0, "test");

	iter = 6000;

	memset(str, 'a', iter+1);
	tokstr[0] = malloc(iter+2);
	memset(tokstr[0], 'a', iter+1);
	tokstr[1] = "nextline";
	tokstr[2] = "thirdline";
	/*tokstr[0] = malloc(2046);
	memset(tokstr[0], 'a', 2045);
	tokstr[0][2045] = '\0';
	tokstr[1] = malloc(2045);
	memset(tokstr[1], 'b', 2044);
	tokstr[1][2044] = '\0';
	tokstr[2] = malloc(2047);
	memset(tokstr[2], 'c', 2046);
	tokstr[2][2046] = '\0';
	sprintf(str, "%s\r\n%s\r\n%s\r\n", tokstr[0], tokstr[1], tokstr[2]);*/

	for(i=0;i<iter;i++)
	    {
	    strcpy(str+i, "a\r\nnextline\r\nthirdline");
	    tokstr[0][i] = 'a';
	    tokstr[0][i+1] = '\0';
	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		if (t != toktype[j]) printf("Error at token length %d, line length %d\n", i+1, i+3);
		assert(t == toktype[j]);
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    alloc = 0;
		    strval = mlxStringVal(lxs, &alloc);
		    assert(strval != NULL);
		    assert(strcnt < 3);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    if (alloc) nmSysFree(strval);
		    }
		}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

