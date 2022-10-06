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
    int j;
    char str[65536] = "";
    int n_flagtype = 4;
    int n_tok = 3;
    int flagtype[4] = {MLX_F_EOF, MLX_F_EOF | MLX_F_EOL, MLX_F_EOL, 0};
    int toktype[4][3] = {   {MLX_TOK_EOF, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_EOL, MLX_TOK_EOF, MLX_TOK_ERROR},
			    {MLX_TOK_EOL, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR} };

	*tname = "mtlexer-00 empty string and eol/eof/error test";

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
		}
	    mlxCloseSession(lxs);
		
	    /* with utf-8 flags */
	    flags = flagtype[i%n_flagtype] | MLX_F_ENFORCEUTF8;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		}
	    mlxCloseSession(lxs);

	    /* with ascii flags */
	    flags = flagtype[i%n_flagtype] | MLX_F_ENFORCEASCII;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

