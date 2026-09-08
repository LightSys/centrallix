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
#define N_INTS		12

static int flagtypes[5] = { MLX_F_CPPCOMM, MLX_F_POUNDCOMM, MLX_F_SEMICOMM, MLX_F_DASHCOMM, MLX_F_CCOMM };

#define N_FLAGTYPES	((int)(sizeof(flagtypes)/sizeof(flagtypes[0])))

static bool
doTests(void)
    {
    int i;
    int j;
    int t;
    int n;
    int flags;
    pLxSession lxs;
    pFile fd;

	flags = 0;
	for(i=0;i<N_FLAGTYPES;i++)
	    flags |= flagtypes[i];

	fd = fdOpen("tests/test_mtlexer_15.txt", O_RDONLY, 0600);
	assert(fd != NULL);
	lxs = mlxOpenSession(fd, flags | MLX_F_EOF);
	assert(lxs != NULL);
	for(j=1;j<=N_INTS;j++)
	    {
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_INTEGER);
	    n = mlxIntVal(lxs);
	    assert(n == j);
	    }
	t = mlxNextToken(lxs);
	assert(t == MLX_TOK_EOF);
	mlxCloseSession(lxs);
	fdClose(fd, 0);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-15 comments // # ; -- /**/ long and short";
    mssInitialize("system", "", "", 0, "test");
    return loopTests(doTests) * 10;
    }
