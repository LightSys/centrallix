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
    int t;
    int n;
    int iter;
    pLxSession lxs;
    pFile fd;
    char buf[256];

	*tname = "mtlexer-21 NODISCARD test (LINEONLY + ~EOL)";

	mssInitialize("system", "", "", 0, "test");

	iter = 60000;

	for(i=0;i<iter;i++)
	    {
	    fd = fdOpen("tests/test_mtlexer_21.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, MLX_F_EOF | MLX_F_LINEONLY | MLX_F_NODISCARD);
	    assert(lxs != NULL);
	    for(j=1;j<=3;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		}
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    mlxCloseSession(lxs);
	    n = fdRead(fd, buf, sizeof(buf) - 1, 0, 0);
	    assert(n >= 0);
	    buf[n] = '\0';
	    assert(strcmp(buf, "This is some text.\n") == 0);
	    fdClose(fd, 0);
	    }

    return iter;
    }

