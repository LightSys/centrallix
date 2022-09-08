#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>
#include <time.h>
#include <stdlib.h>

long long
test(char** tname)
    {
    int i, j, iter;
    int flags;
    char* strings [] = {":", "(", ")", "/", "$", "*", ",", "#", ";", "\0", "-", "-12", "+234", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    int tokenTypes[] = {MLX_TOK_COLON, MLX_TOK_OPENPAREN, MLX_TOK_CLOSEPAREN, 
                        MLX_TOK_SLASH, MLX_TOK_DOLLAR, MLX_TOK_ASTERISK, 
			MLX_TOK_COMMA, MLX_TOK_POUND, MLX_TOK_SEMICOLON,
			MLX_TOK_ERROR, MLX_TOK_MINUS, MLX_TOK_INTEGER, 
			MLX_TOK_INTEGER, MLX_TOK_INTEGER, MLX_TOK_INTEGER,
			MLX_TOK_INTEGER, MLX_TOK_INTEGER, MLX_TOK_INTEGER,
			MLX_TOK_INTEGER, MLX_TOK_INTEGER, MLX_TOK_INTEGER,
			MLX_TOK_INTEGER, MLX_TOK_INTEGER, MLX_TOK_INTEGER};
    int numTok = 23;
    pLxSession lxs;
	srand(time(NULL));

	*tname = "mtlexer-28 test interactions between strchr and NULL";

	iter = 5000;
	for(i=0;i<iter;i++)
	    {
	    for(j = 0 ; j < numTok ; j++)
	        {
		/** setup **/
		setlocale(0, "en_US.UTF-8");
		flags = MLX_F_ALLOWNUL;
		lxs = mlxStringSession(strings[j], flags);
		assert(lxs != NULL);

                int result = mlxNextToken(lxs);
		assert(result == tokenTypes[j]);
		assert(lxs->TokType == tokenTypes[j]);
		assert(strcmp(lxs->TokString, strings[j]) == 0); 
		}
	    }

    return iter;
    }

