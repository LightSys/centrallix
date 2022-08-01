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
    int flags;
    pLxSession lxs;
    int t;
    char* strval;
    int j;
    int strcnt;
    char str[65536] = "'string one' 'string two'\n'string three' 'string four'\r\n'string five'";  
	char str2[65536] = "'आदि में' 'परमेश्वर ने'\n'आकाश और' 'पृथ्वी को'\r\n'बनाया।'";
    int n_flagtype = 4;
    int n_tok = 9;
    int flagtype[4] = {MLX_F_EOF, MLX_F_EOF | MLX_F_EOL, MLX_F_EOL, 0};
    int toktype[4][9] = {   
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOF, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR},
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_EOF },
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_STRING, MLX_TOK_EOL, MLX_TOK_ERROR },
			    {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR},
			};
    char* tokstr[6] = {	"string one", "string two", "string three", "string four", "string five", NULL };
	char* tokstr2[6] = {	"आदि में", "परमेश्वर ने", "आकाश और", "पृथ्वी को", "बनाया।", NULL };

	*tname = "mtlexer-02 three lines of strings and eol/eof/error test";

	mssInitialize("system", "", "", 0, "test");

	iter = 200000;
	for(i=0;i<iter;i++)
	    {
	    flags = flagtype[i%n_flagtype];
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 5);
		    assert(strcmp(strval,tokstr[strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

		/** Test with utf8 **/
		setlocale(0, "en_US.UTF-8");
		lxs = mlxStringSession(str2, flags);
	    assert(lxs != NULL);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_STRING)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 5);
		    assert(strcmp(strval,tokstr2[strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);
		setlocale(0, "C");
	    }

    return iter;
    }

