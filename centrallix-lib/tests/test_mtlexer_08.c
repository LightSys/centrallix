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
    int cnt;
    int t;
    char* teststr = "'string' 'test string' \"string\" \"test string\" 'string\\'s' \"\\\"string\\\"\" 'string\\\\string' 'string\"string' \"string'string\"";
    char* strs[] = {"string", "test string", "string", "test string", "string's", "\"string\"", "string\\string", "string\"string", "string'string", NULL};
    char *str;
    int iter;
    pLxSession lxs;

	*tname = "mtlexer-08 string quoting";

	mssInitialize("system", "", "", 0, "test");

	iter = 150000;

	for(i=0;i<iter;i++)
	    {
	    
	    /** normal **/
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

	    /** utf-8 **/
	    lxs = mlxStringSession(teststr, MLX_F_EOF | MLX_F_ENFORCEUTF8);
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

	    /**  **/
	    lxs = mlxStringSession(teststr, MLX_F_EOF | MLX_F_ENFORCEASCII);
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
	    }

    return iter * 9;
    }

