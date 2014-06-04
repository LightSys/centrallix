#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include "xarray.h"
#include <assert.h>

#define OFFSET_JUMPS 10

typedef struct
    {
    char*	Buffer;
    int		Length;
    int		Offset;
    int		CurOffset;
    }
    OffsetData, *pOffsetData;

long long
test(char** tname)
    {
    pLxSession lex = NULL;
    pFile fd = NULL;
    int alloc = 1;
    pOffsetData token;
    XArray actualtokens; /* list of pOffsetData */
    pXArray tokens = &actualtokens;
    char* currentToken;
    int jumps[OFFSET_JUMPS];
    long long iter;
    int j, i, k;

	*tname = "mtlexer-23 OFFSET test (IFSONLY)";

	mssInitialize("system", "", "", 0, "test");

	/** Open file for initial parsing. **/
	fd = fdOpen("tests/test_mtlexer_23.txt", O_RDONLY, 0600);
	assert(fd != NULL);
	lex = mlxOpenSession(fd, MLX_F_IFSONLY);
	assert(lex != NULL);

	xaInit(tokens, 4);

	/** Calculate the correct offset values for each token in the input file. **/
	while (mlxNextToken(lex) != MLX_TOK_ERROR)
	    {
	    /** Allocate a token data structure. **/
	    token = (pOffsetData)nmMalloc(sizeof(OffsetData));
	    assert(token); /* Note if fail to allocate data structure. */
	    memset(token, 0, sizeof(OffsetData));

	    /** Store the token string data in the token data structure. **/
	    token->Buffer = mlxStringVal(lex, &alloc);
	    assert(token->Buffer); /* Note if fail to get string value. */
	    token->Length = strlen(token->Buffer);

	    /** Calculate the theoretical offset values for the token. **/
	    if (xaCount(tokens) > 0)
		{
		/** Note: The + 1 at the end of this equation compensates for the spaces between words. **/
		token->Offset = ((pOffsetData)xaGetItem(tokens, xaCount(tokens)-1))->CurOffset + 1;
		}
	    else
		{
		token->Offset = 0;
		}
	    token->CurOffset = token->Offset + token->Length;

	    /** Add token data to the token array. **/
	    xaAddItem(tokens, token);
	    }

	mlxCloseSession(lex);
	fdClose(fd, 0);


	iter = 60000;

	/** Run tests. **/
	for(j=0;j<iter;j++)
	    {
	    /** Open a new lexer session on the file. **/
	    fd = fdOpen("tests/test_mtlexer_23.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lex = mlxOpenSession(fd, MLX_F_IFSONLY);
	    assert(lex != NULL);

	    /** Check that offset values are correct for regular iteration. **/
	    for (i = 0; i < xaCount(tokens); i++)
		{
		/** Get the next token information. **/
		token = xaGetItem(tokens, i);

		/** Get next token from lexer. **/
		mlxNextToken(lex);
		currentToken = mlxStringVal(lex, &alloc);

		/** Check that the current token and offsets are correct. **/
		assert(currentToken);
		assert(strlen(currentToken) == token->Length);
		assert(!strcmp(currentToken, token->Buffer));
		assert(mlxGetOffset(lex) == token->Offset);
		assert(mlxGetCurOffset(lex) == token->CurOffset);
		}

	    /** Generate a list of token aligned jumps. **/
	    /** The jumps[] array contains indices of tokens to which the test should jump. **/
	    for (i = 0; i < OFFSET_JUMPS; i++)
		{
		jumps[i] = (29*j+i) % xaCount(tokens);
		}

	    /** Test token aligned jumping. **/
	    for (i = OFFSET_JUMPS - 1; i >= 0; i--)
		{
		/** Offset to the beginning of the token. **/
		token = xaGetItem(tokens, jumps[i]);
		mlxSetOffset(lex, token->Offset);
		
		/** Check that offset values are still correct **/
		for (k = jumps[i]; k < jumps[i]+3; k++)
		    {
		     /** Get the next token information. **/
		    token = xaGetItem(tokens, k);

		    /** Get next token from lexer. **/
		    if (mlxNextToken(lex) == MLX_TOK_ERROR)
			{
			break;
			}
		    currentToken = mlxStringVal(lex, &alloc);

		    /** Check that the current token and offsets are correct. **/
		    assert(currentToken);
		    assert(strlen(currentToken) == token->Length);
		    assert(!strcmp(currentToken, token->Buffer));
		    assert(mlxGetOffset(lex) == token->Offset);
		    assert(mlxGetCurOffset(lex) == token->CurOffset);
		    }
		}

	    /** Generate a list of unaligned jumps. **/
	    /** The jumps[] array contains arbitrary offsets to which the test should jump. **/
	    for (i = 0; i < OFFSET_JUMPS; i++)
		{
		jumps[i] = (29*j + 5*i) % ((pOffsetData)xaGetItem(tokens, xaCount(tokens)-1))->CurOffset;
		}

	    /** Test unaligned jumps. **/
	    for (i = 0; i < OFFSET_JUMPS; i++)
		{
		/** Find the token that this jump falls inside. **/
		k = 0;
		token = (pOffsetData)xaGetItem(tokens, k++);
		while (jumps[i] >= token->CurOffset)
		    {
		    token = (pOffsetData)xaGetItem(tokens, k++);
		    }

		/** Jump to the indicated position. **/
		mlxSetOffset(lex, jumps[i]);

		/** Get next token from lexer. **/
		mlxNextToken(lex);
		currentToken = mlxStringVal(lex, &alloc);

		/** Check that the current token and offsets are correct. **/
		assert(currentToken);
		assert(strlen(currentToken) <= token->Length - (jumps[i] - token->Offset));
		assert(!strcmp(currentToken, token->Buffer + token->Length - strlen(currentToken)));
		assert(mlxGetOffset(lex) == jumps[i] || mlxGetOffset(lex) == jumps[i] + 1);
		assert(mlxGetCurOffset(lex) == token->CurOffset);
	    
		/** Get the next token information. **/
		/**(This line probably has too much magic in it) **/
		token = xaGetItem(tokens, k);

		/** Get next token from lexer. **/
		if (mlxNextToken(lex) == MLX_TOK_ERROR)
		    {
		    continue;
		    }
		currentToken = mlxStringVal(lex, &alloc);

		/** Check that the current token and offsets are correct. **/
		assert(currentToken);
		assert(strlen(currentToken) == token->Length);
		assert(!strcmp(currentToken, token->Buffer));
		assert(mlxGetOffset(lex) == token->Offset);
		assert(mlxGetCurOffset(lex) == token->CurOffset);
		}

	    /** Close the lexer session. **/
	    mlxCloseSession(lex);
	    fdClose(fd, 0);
	    }

	/** Deallocate all token data structures. **/
	for (i = 0; i < xaCount(tokens); i++)
	    {
	    token = (pOffsetData)xaGetItem(tokens, i);
	    nmFree(token, sizeof(OffsetData));
	    }

	xaDeInit(tokens);

    return iter;
    }
