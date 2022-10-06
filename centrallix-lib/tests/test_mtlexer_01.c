#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    int flags;
    pLxSession lxs;
    int t;
    char* strval;
    int j;
    char str[65536] = "'hello world'";
	char str2[65536] = "'Привет мир'";
    int n_flagtype = 4;
    int n_tok = 4;
    int flagtype[4] = {MLX_F_EOF, MLX_F_EOF | MLX_F_EOL, MLX_F_EOL, 0};
    int toktype[4][4] = {   {MLX_TOK_STRING, MLX_TOK_EOF, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF, MLX_TOK_ERROR},
			    {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_STRING, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR} };
    char* tokstr[4] = {	"hello world", NULL, NULL, NULL };
	char* tokstr2[4] = { "Привет мир", NULL, NULL, NULL };


	*tname = "mtlexer-01 string token and eol/eof/error test";

	mssInitialize("system", "", "", 0, "test");

	iter = 400000;
	for(i=0;i<iter;i++)
	    {
	    flags = flagtype[i%n_flagtype];
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcmp(strval,tokstr[j]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

		/** now test utf-8 **/
		flags = flagtype[i%n_flagtype];
		flags |= MLX_F_ENFORCEUTF8;
		lxs = mlxStringSession(str2, flags);
	    assert(lxs != NULL);
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcmp(strval,tokstr2[j]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

	    /** now test ascii **/
	    flags = flagtype[i%n_flagtype];
	    flags |= MLX_F_ENFORCEASCII;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcmp(strval,tokstr[j]) == 0);
		    }
		}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

