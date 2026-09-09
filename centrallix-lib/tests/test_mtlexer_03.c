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

/** Longest line the sweep builds. **/
#define MAX_LEN	6000

#define N_TOK	7

static char str[65536] = "";
static int toktype[9] = {MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF };
static char* tokstr[6];

/*** Number of passes needed to cover every line length.  Each pass tests the
 *** lengths congruent to it, spread across the whole sweep, so even a single
 *** pass exercises the longest lines.
 ***/
#define N_PASS	250

/** Pass under test, advanced each time and wrapped once all passes are done. **/
static int sweep = 0;

static bool
doTest(void)
    {
    int i;
    int j;
    int t;
    int strcnt;
    int alloc;
    char* strval;
    pLxSession lxs;

	if (sweep >= N_PASS) sweep = 0;
	for(i=sweep++;i<MAX_LEN;i+=N_PASS)
	    {
	    /** Both the input and the expected first token end with one 'a'. **/
	    strcpy(str+i, "a\r\nnextline\r\nthirdline");
	    tokstr[0][i] = 'a';
	    tokstr[0][i+1] = '\0';

	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<N_TOK;j++)
		{
		t = mlxNextToken(lxs);
		if (t != toktype[j]) printf("Error at token length %d, line length %d\n", i+1, i+3);
		assert(t == toktype[j]);
		if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		    {
		    alloc = 0;
		    strval = mlxStringVal(lxs, &alloc);
		    assert(strval != NULL);
		    assert(strcnt < 3);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    if (alloc) nmSysFree(strval);
		    }
		}
	    mlxCloseSession(lxs);

	    /** Put the 'a' run back, including the terminator strcpy() wrote. **/
	    memset(str+i, 'a', 23);
	    tokstr[0][i+1] = 'a';
	    }

    return true;
    }

long long
test(char** tname)
    {
    long long rval;

	*tname = "mtlexer-03 BID#156 - line length based failure";

	mssInitialize("system", "", "", 0, "test");

	/** Seed the input and the expected first token at their full length. **/
	memset(str, 'a', MAX_LEN+1);
	tokstr[0] = nmSysMalloc(MAX_LEN+2);
	if (!tokstr[0]) return -1;
	memset(tokstr[0], 'a', MAX_LEN+1);
	tokstr[0][MAX_LEN+1] = '\0';
	tokstr[1] = "nextline";
	tokstr[2] = "thirdline";

	rval = loopTest(doTest);

	nmSysFree(tokstr[0]);
	tokstr[0] = NULL;

    return rval;
    }
