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
#define N_TOK		9

static char str[65536] = "select Select SELECT insert Insert INSERT what What WHAT";
static char* reswds[] = {"select", "insert", NULL};
static int flagtype[N_FLAGTYPE] = {MLX_F_ICASE, MLX_F_ICASER, MLX_F_ICASEK, 0};
static int toktype[N_FLAGTYPE][N_TOK] = {
			    {MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			};
static char* tokstr[N_FLAGTYPE][N_TOK] = {
			    {"select","select","select", "insert","insert","insert", "what","what","what"},
			    {"select","select","select", "insert","insert","insert", "what","What","WHAT"},
			    {"select","select","select", "insert","insert","insert", "what","what","what"},
			    {"select","Select","SELECT", "insert","Insert","INSERT", "what","What","WHAT"},
			};

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
	    lxs = mlxStringSession(str, flagtype[f]);
	    assert(lxs != NULL);
	    mlxSetReservedWords(lxs, reswds);
	    strcnt = 0;
	    for(j=0;j<N_TOK;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[f][j]);
		if (t == MLX_TOK_KEYWORD || t == MLX_TOK_RESERVEDWD)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 9);
		    assert(strcmp(strval,tokstr[f][strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);
	    }

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-11 case (in)sensitive keywords and reserved words";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest) * N_FLAGTYPE * N_TOK;
    }
