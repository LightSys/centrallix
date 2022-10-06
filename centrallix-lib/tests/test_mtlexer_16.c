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
    int flagtypes[5] = { MLX_F_CPPCOMM, MLX_F_POUNDCOMM, MLX_F_SEMICOMM, MLX_F_DASHCOMM, MLX_F_CCOMM };
    int n_flagtypes = 5;
    int flags;
    char* commnames[5] = { "cppcomm", "poundcomm", "semicomm", "dashcomm", "ccomm" };
    pLxSession lxs;
    pFile fd;

	*tname = "mtlexer-16 comments // # ; -- /**/ short only";

	mssInitialize("system", "", "none", 0, "test");

	iter = 6000;

	flags = 0;
	for(i=0;i<n_flagtypes;i++)
	    flags |= flagtypes[i];

	for(i=0;i<iter;i++)
	    {
	    /** normal **/
	    fd = fdOpen("tests/test_mtlexer_16.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, flags | MLX_F_EOF);
	    assert(lxs != NULL);
	    for(j=1;j<=12;j++)
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

	     /** utf-8 **/
	    fd = fdOpen("tests/test_mtlexer_16.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, flags | MLX_F_EOF | MLX_F_ENFORCEUTF8);
	    assert(lxs != NULL);
	    for(j=1;j<=12;j++)
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

	     /** ascii **/
	    fd = fdOpen("tests/test_mtlexer_16.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, flags | MLX_F_EOF | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    for(j=1;j<=12;j++)
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
	    }

    return iter * 10;
    }

