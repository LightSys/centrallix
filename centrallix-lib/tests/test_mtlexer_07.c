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
    int j;
    int k;
    int t;
    int n;
    int iter;
    int flags;
    pLxSession lxs;
    pFile fd;
    int tokens[256];
    int n_tokens;
    int n_iter;
    char* reswds[] = { "reserved", NULL };

	*tname = "mtlexer-07 all tokens, space/tab/noifs with eol";

	mssInitialize("system", "", "", 0, "test");

	iter = 10000;

	for(i=0;i<iter;i++)
	    {
	    fd = fdOpen("tests/test_mtlexer_07.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    flags = MLX_F_EOL | MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_SSTRING;
	    lxs = mlxOpenSession(fd, flags);
	    assert(lxs != NULL);
	    mlxSetReservedWords(lxs, reswds);
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_INTEGER);
	    n_iter = mlxIntVal(lxs);
	    assert(mlxNextToken(lxs) == MLX_TOK_EOL);
	    n_tokens = 0;
	    while((t = mlxNextToken(lxs)) == MLX_TOK_INTEGER)
		{
		assert(n_tokens < 256);
		tokens[n_tokens++] = mlxIntVal(lxs);
		assert(tokens[n_tokens-1] > 0 && tokens[n_tokens-1] <= MLX_TOK_MAX);
		}
	    assert(t == MLX_TOK_EOL);
	    for(j=0;j<n_iter;j++)
		{
		for(k=0;k<n_tokens;k++)
		    assert(mlxNextToken(lxs) == tokens[k]);
		assert(mlxNextToken(lxs) == MLX_TOK_EOL);
		}
	    mlxCloseSession(lxs);
	    fdClose(fd, 0);
	    }

    return iter * n_tokens * n_iter;
    }

