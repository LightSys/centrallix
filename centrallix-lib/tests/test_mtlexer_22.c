#include <assert.h>
#include <string.h>

#include "mtsession.h"
#include "newmalloc.h"

#include "mtlexer.h"

/** Short names for the lexer tokens used in the cases below. **/
#define T_FILE		MLX_TOK_FILENAME
#define T_SEMI		MLX_TOK_SEMICOLON
#define T_KEYWORD	MLX_TOK_KEYWORD
#define T_STR		MLX_TOK_STRING
#define T_EOF		MLX_TOK_EOF

#define NTOK	6

typedef struct
    {
    const char*	    Input;	/* lexed with MLX_F_FILENAMES */
    int		    Tok[NTOK];	/* expected token types, in order */
    const char*	    Str[NTOK];	/* expected string value, or NULL to skip */
    }
    Case;

static Case cases[] =
    {
    /**	  Input			Tok						Str **/

    /** Semicolon after a path ends it. **/
	{ "/tmp/x.smtp;",	{ T_FILE, T_SEMI, T_EOF },			{ "/tmp/x.smtp" } },
	{ "./x/y;",		{ T_FILE, T_SEMI, T_EOF },			{ "./x/y" } },
	{ "/;",			{ T_FILE, T_SEMI, T_EOF },			{ "/" } },

    /** Semicolon inside a path splits it. **/
	{ "/a;b",		{ T_FILE, T_SEMI, T_KEYWORD, T_EOF },		{ "/a", NULL, "b" } },

    /** Quoted paths keep their semicolons. **/
	{ "'/a;b';",		{ T_STR, T_SEMI, T_EOF },			{ "/a;b" } },

    /** Each following semicolon is its own token. **/
	{ "./a/b;;",		{ T_FILE, T_SEMI, T_SEMI, T_EOF },		{ "./a/b" } },
	{ "/;;;",		{ T_FILE, T_SEMI, T_SEMI, T_SEMI, T_EOF },	{ "/" } },
    };

long long
test(char** tname)
    {
    int i, c, j;
    int iter;
    int ncases = sizeof(cases) / sizeof(Case);
    int alloc;
    pLxSession lxs;
    char* str;
    char longpath[MLX_STRVAL * 2 + 2];

	/*** This test verifies that a semicolon ends an unquoted path, both
	 *** when the path fits in the token buffer and when it is read in
	 *** pieces, and that a quoted path can still contain one.
	 ***/

	*tname = "mtlexer-22 semicolon ends a filename";

	mssInitialize("system", "", "", 0, "test");

	/** Path longer than the token buffer, followed by a semicolon. **/
	longpath[0] = '/';
	memset(longpath + 1, 'a', sizeof(longpath) - 3);
	longpath[sizeof(longpath) - 2] = ';';
	longpath[sizeof(longpath) - 1] = '\0';

	iter = 20000;
	for(i=0;i<iter;i++)
	    {
	    for(c=0;c<ncases;c++)
		{
		lxs = mlxStringSession((char*)cases[c].Input, MLX_F_EOF | MLX_F_FILENAMES);
		assert(lxs != NULL);
		for(j=0;j<NTOK;j++)
		    {
		    /** Token type matches, stopping at the end of input. **/
		    assert(mlxNextToken(lxs) == cases[c].Tok[j]);
		    if (cases[c].Tok[j] == T_EOF) break;

		    /** String value matches where one is expected. **/
		    if (cases[c].Str[j])
			assert(!strcmp(mlxStringVal(lxs, NULL), cases[c].Str[j]));
		    }
		mlxCloseSession(lxs);
		}

	    /** Long path stops before the semicolon. **/
	    lxs = mlxStringSession(longpath, MLX_F_EOF | MLX_F_FILENAMES);
	    assert(lxs != NULL);
	    assert(mlxNextToken(lxs) == T_FILE);
	    alloc = 0;
	    str = mlxStringVal(lxs, &alloc);
	    assert(str != NULL);
	    assert(strlen(str) == sizeof(longpath) - 2);  /* path without ';' */
	    assert(!strncmp(str, longpath, sizeof(longpath) - 2));
	    if (alloc) nmSysFree(str);
	    assert(mlxNextToken(lxs) == T_SEMI);
	    assert(mlxNextToken(lxs) == T_EOF);
	    mlxCloseSession(lxs);
	    }

    return (long long)iter * (ncases + 1);
    }
