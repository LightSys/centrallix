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

static char* teststr = "'string' 'test string' \"string\" \"test string\" 'string\\'s' \"\\\"string\\\"\" 'string\\\\string' 'string\"string' \"string'string\"";
static char* strs[] = {"string", "test string", "string", "test string", "string's", "\"string\"", "string\\string", "string\"string", "string'string", NULL};

static bool
doTests(void)
    {
    int cnt;
    int t;
    char* str;
    pLxSession lxs;

	lxs = mlxStringSession(teststr, MLX_F_EOF);
	assert(lxs != NULL);
	cnt = 0;
	while(1)
	    {
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_EOF) break;
	    assert(t == MLX_TOK_STRING);
	    str = mlxStringVal(lxs, NULL);
	    assert(strcmp(str, strs[cnt++]) == 0);
	    }
	assert(strs[cnt] == NULL);
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-08 string quoting";
    mssInitialize("system", "", "", 0, "test");
    return loopTests(doTests) * 9;
    }
