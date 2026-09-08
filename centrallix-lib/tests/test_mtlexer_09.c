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

/** Longest string the sweep builds. **/
#define MAX_LEN	6000

#define N_TOK	3

static char str[65536] = "";
static int toktype[N_TOK] = {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF };
static char* tokstr[6];

/** Line length under test, advanced each pass and wrapped at the longest case. **/
static int sweep = 0;

static bool
doTests(void)
    {
    int i;
    int j;
    int t;
    int strcnt;
    int alloc;
    char* strval;
    pLxSession lxs;

	if (sweep >= MAX_LEN-20) sweep = 0;
	i = sweep++;

	/** Put the line breaks in place for this pass. **/
	str[i+1] = '\r';
	str[i+2] = '\n';
	tokstr[0][i] = '\r';
	tokstr[0][i+1] = '\n';
	str[i+20] = '\n';
	tokstr[0][i+19] = '\n';

	lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF);
	assert(lxs != NULL);
	strcnt = 0;
	for(j=0;j<N_TOK;j++)
	    {
	    t = mlxNextToken(lxs);
	    if (t != toktype[j]) printf("Error at string length %d\n", i+1);
	    assert(t == toktype[j]);
	    if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		{
		alloc = 0;
		strval = mlxStringVal(lxs, &alloc);
		assert(strval != NULL);
		assert(strcnt < 1);
		assert(strcmp(strval,tokstr[strcnt++]) == 0);
		if (alloc) nmSysFree(strval);
		}
	    }
	mlxCloseSession(lxs);

	/** Put the line breaks back, ready for the next pass. **/
	str[i+1] = 'a';
	str[i+2] = 'a';
	tokstr[0][i] = 'a';
	tokstr[0][i+1] = 'a';
	str[i+20] = 'a';
	tokstr[0][i+19] = 'a';

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "mtlexer-09 strings spanning multiple lines";

	mssInitialize("system", "", "", 0, "test");

	/** A quoted run of 'a', and the expected token it should lex to. **/
	memset(str, 'a', MAX_LEN+3);
	str[0] = '"';
	str[MAX_LEN+2] = '"';
	tokstr[0] = nmSysMalloc(MAX_LEN+2);
	if (!tokstr[0]) return -1;
	memset(tokstr[0], 'a', MAX_LEN+1);
	tokstr[0][MAX_LEN+1] = '\0';

	rval = loopTests(doTests);

	nmSysFree(tokstr[0]);
	tokstr[0] = NULL;

    return rval;
    }
