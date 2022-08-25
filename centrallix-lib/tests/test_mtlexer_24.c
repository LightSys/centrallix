#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>

long long
test(char** tname)
    {
    int BUF_SIZE = 2000;
    int i;
    int iter;
    int flags;
    pLxSession lxs;
    int t;
    char* str = ""; 

	*tname = "mtlexer-24 test Copy Token with invalid utf-8 characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 50000;
	for(i=0;i<iter;i++)
	    {
	    /** setup **/
	    setlocale(0, "en_US.UTF-8");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    /* read in the first token. This should read the first 256 bytes */
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    strval = lxs->TokString;
	    assert(strval != NULL);
	    assert(strcmp(strval,tokstr) == 0);
	    /* now peform a copy into a buffer twice the size. Should enable invalid UTF8 to bypass the 
	     * checks in next token, but should be caught by the check at the end of copy
	     */
	    int result = mlxCopyToken(lxs, buf, BUF_SIZE);
	    assert(result == strlen(str)-2); /* -2 since ' is not included */
	    str[strlen(str)-1] = '\0';  /* change for test */
	    assert(strcmp(buf, str+1) == 0);
	    str[strlen(str)] = '\'';  /* revert back. Note string is 1 shorter now */
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    mlxCloseSession(lxs);


	    /** trying again without utf8 results in a normal token **/
	    setlocale(0, "C");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    /* read in the first token. This should read the first 256 bytes */
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    strval = lxs->TokString;
	    assert(strval != NULL);
	    assert(strcmp(strval,tokstr) == 0);
	    /* now test again */
	    result = mlxCopyToken(lxs, buf, BUF_SIZE);
	    str[strlen(str)-1] = '\0';  /* change for test */
	    assert(strcmp(buf, str+1) == 0);
	    str[strlen(str)] = '\'';  /* revert back */
	    assert(lxs->TokType == MLX_TOK_STRING);
	    }

    return iter;
    }

