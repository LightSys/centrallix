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
    int j;
    int t;
    int n;
    int iter;
    pLxSession lxs;
    pFile fd;

	*tname = "mtlexer-05 integer data, one per line with eol/eof";

	mssInitialize("system", "", "", 0, "test");
	setlocale(0, "en_US.UTF-8");

	iter = 10;

	for(i=0;i<iter;i++)
	    {
	    fd = fdOpen("tests/test_mtlexer_05.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, MLX_F_EOL | MLX_F_EOF);
	    assert(lxs != NULL);
	    for(j=1;j<=60000;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_INTEGER);
		n = mlxIntVal(lxs);
		assert(n == j);
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_EOL);
		}
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_EOF);
	    mlxCloseSession(lxs);
	    fdClose(fd, 0);
	    }

    return iter * 60000;
    }

