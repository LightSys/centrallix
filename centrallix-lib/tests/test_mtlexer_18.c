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
#define N_TOK		15

static char str[65536] = "\n\n'string one' 'string two'\n'string three' 'string four'\r\n'string five'\r\n\r\nString Six\n";
static int flagtype[N_FLAGTYPE] = {MLX_F_EOF, MLX_F_EOF | MLX_F_EOL, MLX_F_EOL, 0};
static int toktype[N_FLAGTYPE][N_TOK] = {
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOF, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF },
			    {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_ERROR },
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR},
			};
static char* tokstr[8] = { "\n", "\n", "'string one' 'string two'\n", "'string three' 'string four'\r\n", "'string five'\r\n", "\r\n", "String Six\n", NULL };

static bool
doTest(void)
    {
    int f;
    int j;
    int t;
    int strcnt;
    char* strval;
    pLxSession lxs;

	for(f=0;f<N_FLAGTYPE;f++)
	    {
	    lxs = mlxStringSession(str, flagtype[f] | MLX_F_LINEONLY);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<N_TOK;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[f][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 7);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);
	    }

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-18 LINEONLY mode test - file ends in newline";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest) * N_FLAGTYPE * 7;
    }
