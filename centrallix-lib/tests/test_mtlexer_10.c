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
    int i,j;
    char* teststr = "0 -1 1 -255 -256 255 256 -127 -128 127 128 32767 32768 -32767 -32768 65535 65536 -65535 -65536 16777216 -16777216 2147483647 -2147483648\n"
		    ".0 .1 .10 .01 0.0 0.1 0.10 0.01 1.0 1.1 1.10 1.01 10.0 10.1 10.10 10.01 -.0 -.1 -.10 -.01 -0.0 -0.1 -0.10 -0.01 -1.0 -1.1 -1.10 -1.01 -10.0 -10.1 -10.10 -10.01";
    int integers[] = {0, -1, 1, -255, -256, 255, 256, -127, -128, 127, 128, 32767, 32768, -32767, -32768, 65535, 65536, -65535, -65536, 16777216, -16777216, 2147483647, -2147483647-1 };
    double doubles[] = {.0, .1, .10, .01, 0.0, 0.1, 0.10, 0.01, 1.0, 1.1, 1.10, 1.01, 10.0, 10.1, 10.10, 10.01, -.0, -.1, -.10, -.01, -0.0, -0.1, -0.10, -0.01, -1.0, -1.1, -1.10, -1.01, -10.0, -10.1, -10.10, -10.01};
    int n;
    double d;
    int iter;
    pLxSession lxs;

	*tname = "mtlexer-10 integer and double parsing";

	mssInitialize("system", "", "", 0, "test");

	iter = 15000;

	for(i=0;i<iter;i++)
	    {
	    lxs = mlxStringSession(teststr, MLX_F_EOF);
	    assert(lxs != NULL);
	    for(j=0;j<sizeof(integers)/sizeof(integers[0]);j++)
		{
		assert(mlxNextToken(lxs) == MLX_TOK_INTEGER);
		n = mlxIntVal(lxs);
		assert(n == integers[j]);
		}
	    for(j=0;j<sizeof(doubles)/sizeof(doubles[0]);j++)
		{
		assert(mlxNextToken(lxs) == MLX_TOK_DOUBLE);
		d = mlxDoubleVal(lxs);
		assert(d == doubles[j]);
		}
	    assert(mlxNextToken(lxs) == MLX_TOK_EOF);
	    mlxCloseSession(lxs);
	    }

    return iter * (sizeof(integers)/sizeof(integers[0]) + sizeof(doubles)/sizeof(doubles[0]));
    }

