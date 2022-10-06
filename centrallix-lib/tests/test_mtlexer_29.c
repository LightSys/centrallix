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
    int strcnt;
    char str[65536] = "'these' 'are' 'some' 'mostly' 'valid' '\xFFwords' 'for' 'testing'";  
    int n_tok = 8;
    char* tokstr[8] = {"these", "are", "some", "mostly", "valid", "\xFFwords", "for", "testing"};
    int toktype[8] = {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR};

	*tname = "mtlexer-29 test strings with invalid ascii characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    /** setup **/
	    flags = 0;
	    lxs = mlxStringSession(str, flags | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    strcnt = 0;

	    /** check each token **/
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[j]);
		strval = mlxStringVal(lxs, NULL);
		if (t == MLX_TOK_STRING)
		    {
		    assert(strval != NULL);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

	    /** test without **/
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    strcnt = 0;

	    /** check each token **/
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t ==  MLX_TOK_STRING);
		strval = mlxStringVal(lxs, NULL);
		assert(strval != NULL);
		assert(strcmp(strval,tokstr[strcnt++]) == 0);
		}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

