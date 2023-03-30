#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <limits.h>
#include "util.h"

long long
test(char** tname)
    {
    int i, j;
    int iter;
    int flags;
    int result;
    pLxSession lxs;
    char* fileNames = {"./file /folder/another/file.ext ./localFile /anotherOne /yetStillMore /short/path.c "
           "/test /aConsiderablyLongerFileName /invalid\xFFFileName /an/extra/path"};
    char* valid_fileNames = {"./file /folder/another/file.ext ./localFile /anotherOne /yetStillMore /short/path.c "
           "/test /aConsiderablyLongerFileName /validFileName /an/extra/path"};
	*tname = "mtlexer-30 Test valid and invalid ASCII file names";
	
	mssInitialize("system", "", "", 0, "test");
	iter = 20000;
	for(i=0;i<iter;i++)
	    { 
	    /** valid file name list: should pass all tokens **/
	    flags = MLX_F_FILENAMES; 
	    lxs = mlxStringSession(valid_fileNames, flags | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    int offset = 0;
	    for(j = 0 ; j < 10 ; j++)
	        {
		result = mlxNextToken(lxs);
		assert(result == MLX_TOK_FILENAME);
		assert(lxs->TokType == MLX_TOK_FILENAME);
		assert(memcmp(valid_fileNames+offset, lxs->TokString, strlen(lxs->TokString)) == 0);
		assert(verifyASCII(lxs->TokString) == UTIL_VALID_CHAR);
		offset += strlen(lxs->TokString) + 1; /* move past the space as well */
		}
	    result = mlxNextToken(lxs);
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    mlxCloseSession(lxs);

	    /** invalid set should locate invalid **/
	    flags = MLX_F_FILENAMES;
	    lxs = mlxStringSession(fileNames, flags | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    offset = 0;
	    for(j = 0 ; j < 8 ; j++)
	        {
		result = mlxNextToken(lxs);
		assert(result == MLX_TOK_FILENAME);
		assert(lxs->TokType == MLX_TOK_FILENAME);
		assert(memcmp(valid_fileNames+offset, lxs->TokString, strlen(lxs->TokString)) == 0);
		offset += strlen(lxs->TokString) + 1; /* move past the space as well */
		}

	    result = mlxNextToken(lxs);
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    assert(strcmp(lxs->TokString, "/invalid\xFFFileName") == 0);
	    mlxCloseSession(lxs);


	    /** test without ascii flag (and therefore without a validate); should pass all junk **/ 
	    flags = MLX_F_FILENAMES;
	    lxs = mlxStringSession(fileNames, flags);
	    assert(lxs != NULL);
	    offset = 0;
	    for(j = 0 ; j < 10 ; j++)
	        {
		result = mlxNextToken(lxs);
		assert(result == MLX_TOK_FILENAME);
		assert(lxs->TokType == MLX_TOK_FILENAME);
		}
	    result = mlxNextToken(lxs);
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    mlxCloseSession(lxs);
	    }
    return iter;
    }

