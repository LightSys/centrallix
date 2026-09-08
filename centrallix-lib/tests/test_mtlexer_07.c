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

static char* reswds[] = { "reserved", NULL };

/** Token count and repeat count read from the data file by the last pass. **/
static int n_tokens = 0;
static int n_iter = 0;

static bool
doTests(void)
    {
    int j;
    int k;
    int t;
    int flags;
    pLxSession lxs;
    pFile fd;
    int tokens[256];

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

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "mtlexer-07 all tokens, space/tab/noifs with eol";

	mssInitialize("system", "", "", 0, "test");

	rval = loopTests(doTests);
	if (rval > 0) rval *= (long long)n_tokens * n_iter;

    return rval;
    }
