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

/** Integers in the data file, and so the ops performed by one pass. **/
#define N_INTS	60000

static bool
doTest(void)
    {
    int j;
    int t;
    int n;
    pLxSession lxs;
    pFile fd;

	fd = fdOpen("tests/test_mtlexer_06.txt", O_RDONLY, 0600);
	assert(fd != NULL);
	lxs = mlxOpenSession(fd, MLX_F_EOL | MLX_F_EOF);
	assert(lxs != NULL);
	for(j=1;j<=N_INTS;j++)
	    {
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_INTEGER);
	    n = mlxIntVal(lxs);
	    assert(n == j);
	    }
	t = mlxNextToken(lxs);
	assert(t == MLX_TOK_EOL);
	t = mlxNextToken(lxs);
	assert(t == MLX_TOK_EOF);
	mlxCloseSession(lxs);
	fdClose(fd, 0);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-06 integer data, all on one line with eol/eof";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest) * N_INTS;
    }
