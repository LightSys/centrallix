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
    int i;
    int iter;
    pLxSession lxs;
    int t;
    char* strval;
    int j;
    int strcnt;
    char str[65536] = "Header: 'Val1 Val2 Val3'\r\nHeader-2: Val1 'Val2 Val3'\r\nHeader-3: 'Val1 Val2' Val3\r\n";
    int n_tok = 16;
    int toktype[16] = {MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_KEYWORD, MLX_TOK_COLON, MLX_TOK_STRING, MLX_TOK_KEYWORD, MLX_TOK_EOL, MLX_TOK_EOF};
    char* tokstr[10] = { "Header", "'Val1", "Val2", "Val3'", "Header-2", " Val1 'Val2 Val3'\r\n", "Header-3", "Val1 Val2", "Val3", NULL };
    int setflags[16] =   {0, 0, MLX_F_IFSONLY, 0, 0, 0,             0, 0, MLX_F_LINEONLY, 0,              0, 0, 0, 0, 0, 0 };
    int unsetflags[16] = {0, 0, 0,             0, 0, MLX_F_IFSONLY, 0, 0, 0,              MLX_F_LINEONLY, 0, 0, 0, 0, 0, 0 };

	*tname = "mtlexer-19 enabling/disabling LINEONLY/IFSONLY during parsing";

	mssInitialize("system", "", "", 0, "test");
	setlocale(0, "en_US.UTF-8");

	iter = 100000;
	for(i=0;i<iter;i++)
	    {
		if(i == iter/2) setlocale(0, "C"); /* switch half way */

	    lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_DASHKW);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
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
	    }

    return iter * 9;
    }

