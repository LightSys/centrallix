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
    char str[65536] = "'하나님이' '세상을' '무척' '사랑하셔서' '하나밖에' '\xFF없는' '외아들마저' '보내'";  
    int n_tok = 8;
    char* tokstr[8] = {"하나님이", "세상을", "무척", "사랑하셔서", "하나밖에", "\xFF없는", "외아들마저", "보내"};
	int toktype[8] = {MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_STRING, MLX_TOK_ERROR, MLX_TOK_ERROR, MLX_TOK_ERROR};

	*tname = "mtlexer-22 test strings with invalid utf-8 characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
		/** setup **/
		setlocale(0, "en_US.UTF-8");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    strcnt = 0;

		/** check each token **/
	    for(j=0;j<n_tok;j++)
			{
			t = mlxNextToken(lxs);
			assert(t == toktype[j]);
			if (t == MLX_TOK_STRING)
				{
				strval = mlxStringVal(lxs, NULL);
				assert(strval != NULL);
				assert(strcmp(strval,tokstr[strcnt++]) == 0);
				}
			}
	    mlxCloseSession(lxs);

		/** test without utf-8 locale; should pass all junk **/ 
		setlocale(0, "C");
		lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    strcnt = 0;

		/** check each token **/
	    for(j=0;j<n_tok;j++)
			{
			t = mlxNextToken(lxs);
			assert(t ==  MLX_TOK_STRING);
			strval = mlxStringVal(lxs, NULL);
			assert(strval != NULL);
			assert(strcmp(strval,tokstr[strcnt++]) == 0);
			}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

