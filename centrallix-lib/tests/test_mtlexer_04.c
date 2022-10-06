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
    char str[65536] = "";

	*tname = "mtlexer-04 open/close session";

	mssInitialize("system", "", "", 0, "test");

	iter = 700000;

	for(i=0;i<iter;i++)
	    {
	    /** normal **/
	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY);
	    assert(lxs != NULL);
	    mlxCloseSession(lxs);

	    /** utf-8 **/
	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_ENFORCEUTF8);
	    assert(lxs != NULL);
	    mlxCloseSession(lxs);

	    /** normal **/
	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

