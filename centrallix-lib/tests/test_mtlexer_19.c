#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <stdbool.h>
#include "test_utils.h"

#define N_TOK	16

static char str[65536] = "Header: 'Val1 Val2 Val3'\r\nHeader-2: Val1 'Val2 Val3'\r\nHeader-3: 'Val1 Val2' Val3\r\n";
static int toktype[N_TOK] = {MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_KEYWORD, MLX_TOK_EOL, MLX_TOK_EOF};
static char* tokstr[10] = { "Header", "'Val1", "Val2", "Val3'", "Header-2", " Val1 'Val2 Val3'\r\n", "Header-3", "Val1 Val2", "Val3", NULL };
static int setflags[N_TOK] =   {0, 0, MLX_F_IFSONLY, 0, 0, 0,             0, 0, MLX_F_LINEONLY, 0,              0, 0, 0, 0, 0, 0 };
static int unsetflags[N_TOK] = {0, 0, 0,             0, 0, MLX_F_IFSONLY, 0, 0, 0,              MLX_F_LINEONLY, 0, 0, 0, 0, 0, 0 };

static bool
doTests(void)
    {
    int j;
    int t;
    int strcnt;
    char* strval;
    pLxSession lxs;

	lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_DASHKW);
	assert(lxs != NULL);
	strcnt = 0;
	for(j=0;j<N_TOK;j++)
	    {
	    if (setflags[j]) mlxSetOptions(lxs, setflags[j]);
	    if (unsetflags[j]) mlxUnsetOptions(lxs, unsetflags[j]);
	    t = mlxNextToken(lxs);
	    assert(t == toktype[j]);
	    if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		{
		strval = mlxStringVal(lxs, NULL);
		assert(strval != NULL);
		assert(strcnt < 9);
		assert(strcmp(strval,tokstr[strcnt++]) == 0);
		}
	    }
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-19 enabling/disabling LINEONLY/IFSONLY during parsing";
    mssInitialize("system", "", "", 0, "test");
    return loopTests(doTests) * 9;
    }
