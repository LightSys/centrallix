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
    char* fileNames = {"./実に神は、 /ひとり子/を/さ.え ./惜しまず与える /ほどに、 /この世界を愛して /くださいま/した.。 "
           "/それは、 /神の御子を信じる者が、 /だれ\xFF一人滅びず、 /永遠のいのち/を得るた/めで.す。"};
    char* valid_fileNames = {"./実に神は、 /ひとり子/を/さ.え ./惜しまず与える /ほどに、 /この世界を愛して /くださいま/した.。 "
           "/それは、 /神の御子を信じる者が、 /だれ一人滅びず、 "
           "/永遠のいのち/を得るた/めで.す。/永遠のいのち/を得るた/めで.す。/永遠のいのち/を得るた/めで.す。/永遠のいのち/を得る" /* extra long file test */
           "た/めで.す。/永遠のいのち/を得るた/めで.す。/永遠のいのち/を得るた/めで.す。/永遠のいのち/を得るた/めで.す。"};
	*tname = "mtlexer-23 Test valid and invalid UTF-8 file names";

	mssInitialize("system", "", "", 0, "test");
	iter = 20000;
	for(i=0;i<iter;i++)
	    { 
	    /** valid file name list: should pass all tokens **/
	    flags = MLX_F_FILENAMES; 
	    lxs = mlxStringSession(valid_fileNames, flags | MLX_F_ENFORCEUTF8);
	    assert(lxs != NULL);
	    int offset = 0;
	    for(j = 0 ; j < 10 ; j++)
	        {
		result = mlxNextToken(lxs);
		assert(result == MLX_TOK_FILENAME);
		assert(lxs->TokType == MLX_TOK_FILENAME);
		assert(memcmp(valid_fileNames+offset, lxs->TokString, strlen(lxs->TokString)) == 0);
		assert(verifyUTF8(lxs->TokString) == UTIL_VALID_CHAR);
		offset += strlen(lxs->TokString) + 1; /* move past the space as well */
		}
	    result = mlxNextToken(lxs);
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    mlxCloseSession(lxs);

	    /** invalid set should locate invalid **/
	    flags = MLX_F_FILENAMES;
	    lxs = mlxStringSession(fileNames, flags | MLX_F_ENFORCEUTF8);
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
	    assert(strcmp(lxs->TokString, "/だれ\xFF一人滅びず、") == 0);
	    mlxCloseSession(lxs);


	    /** test without utf-8 flag (and therefore without a validate); should pass all junk **/ 
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

