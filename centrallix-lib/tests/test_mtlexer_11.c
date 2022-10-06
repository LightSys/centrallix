#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>

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
    char str[65536] = "select Select SELECT insert Insert INSERT what What WHAT";
    char* reswds[] = {"select", "insert", NULL};
    int n_flagtype = 4;
    int n_tok = 9;
    int flagtype[4] = {MLX_F_ICASE, MLX_F_ICASER, MLX_F_ICASEK, 0};
    int toktype[4][9] = {   
			    {MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			    {MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_RESERVEDWD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD, MLX_TOK_KEYWORD},
			};
    char* tokstr[4][9] ={
			    {"select","select","select", "insert","insert","insert", "what","what","what"},
			    {"select","select","select", "insert","insert","insert", "what","What","WHAT"},
			    {"select","select","select", "insert","insert","insert", "what","what","what"},
			    {"select","Select","SELECT", "insert","Insert","INSERT", "what","What","WHAT"},
			};

	*tname = "mtlexer-11 case (in)sensitive keywords and reserved words";

	mssInitialize("system", "", "", 0, "test");

	iter = 200000;
	for(i=0;i<iter;i++)
	    {

	    /** normal **/
	    flags = flagtype[i%n_flagtype];
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    mlxSetReservedWords(lxs, reswds);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_KEYWORD || t == MLX_TOK_RESERVEDWD)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 9);
		    assert(strcmp(strval,tokstr[i%n_flagtype][strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

	    /** utf-8 **/
	    flags = flagtype[i%n_flagtype] | MLX_F_ENFORCEUTF8;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    mlxSetReservedWords(lxs, reswds);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_KEYWORD || t == MLX_TOK_RESERVEDWD)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 9);
		    assert(strcmp(strval,tokstr[i%n_flagtype][strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);

	    /** ascii **/
	    flags = flagtype[i%n_flagtype] | MLX_F_ENFORCEASCII;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    mlxSetReservedWords(lxs, reswds);
	    strcnt = 0;
	    for(j=0;j<n_tok;j++)
		{
		t = mlxNextToken(lxs);
		assert(t == toktype[i%n_flagtype][j]);
		if (t == MLX_TOK_KEYWORD || t == MLX_TOK_RESERVEDWD)
		    {
		    strval = mlxStringVal(lxs, NULL);
		    assert(strval != NULL);
		    assert(strcnt < 9);
		    assert(strcmp(strval,tokstr[i%n_flagtype][strcnt++]) == 0);
		    }
		}
	    mlxCloseSession(lxs);
	    }

    return iter * n_tok;
    }

