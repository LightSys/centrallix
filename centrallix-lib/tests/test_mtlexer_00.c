#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include "test_utils.h"

#define N_FLAGTYPE	4
#define N_TOK		3

static char str[65536] = "";
static int flagtype[N_FLAGTYPE] = {MLX_F_EOF, MLX_F_EOF | MLX_F_EOL, MLX_F_EOL, 0};
static int toktype[N_FLAGTYPE][N_TOK] = {
			    {MLX_TOK_EOF, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_EOL, MLX_TOK_EOF, MLX_TOK_ERROR},
			    {MLX_TOK_EOL, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR} };

static bool
doTest(void)
    {
    int f;
    int j;
    int t;
    pLxSession lxs;

	for(f=0;f<N_FLAGTYPE;f++)
	    {
	    lxs = mlxStringSession(str, flagtype[f]);
	    assert(lxs != NULL);
	    for(j=0;j<N_TOK;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[f][j]);
		}
	    mlxCloseSession(lxs);
	    }

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-00 empty string and eol/eof/error test";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest) * N_FLAGTYPE;
    }
