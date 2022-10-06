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
    int BUF_SIZE = 2000;
    int i;
    int iter;
    int flags;
    pLxSession lxs;
    int t;
    char* strval;
    /* one really long line (1165 btyes). */
    char str[65536] = "'orem ipsum dolor sit amet, consectetur adipiscing elit. "
                      "Morbi ac vestibulum lectus. Quisque at felis sit amet augue "
		      "luctus dictum eget eu justo. Nulla facilisi. In gravida, "
		      "mauris quis ullamcorper pulvinar, leo turpis gravida diam, "
		      "quis dapibus nisl dui ac libero. Cras a accumsan odi\xFF. "
		      "Maecenas pharetra turpis egestas lectus efficitur fringilla. "
		      "Nunc quis interdum odio, non rutrum magna. Maecenas nisi "
		      "odio, rutrum vel sem non, blandit ultrices dolor. Proin et "
		      "pharetra justo. Vestibulum risus nunc, fermentum a ex eu, "
		      "tristique posuere erat. Phasellus semper tempus lobortis. "
		      "Suspendisse rhoncus, turpis in cursus finibus, lectus quam "
		      "ornare ante, sed scelerisque elit nisl eget elit. Integer id "
		      "molestie leo. Ut in tortor risus. Suspendisse nec elit erat. "
		      "Praesent sapien dui, venenatis at dignissim et, finibus vel "
		      "enim. Quisque aliquam ultricies metus, id consectetur lorem. "
		      "Pellentesque habitant morbi tristique senectus et netus et "
		      "malesuada fames ac turpis egestas. Nam commodo faucibus massa, "
		      "id mollis nisl feugiat fringilla. Maecenas pretium tristique "
		      "ligula, aliquet dignissim orci laoreet ac. Quisque turpis tortor,"
		      "pretium in sem eu, sagittis morbi. '"; 
    char tokstr[MLX_STRVAL];
    memcpy(tokstr, str+1, MLX_STRVAL-1);
    tokstr[MLX_STRVAL-1] = '\0';
    char buf[BUF_SIZE];  

	*tname = "mtlexer-31 test Copy Token with invalid ascii characters";
/************************** FIXME: make ascii ********/
	mssInitialize("system", "", "", 0, "test");
	iter = 50000;
	for(i=0;i<iter;i++)
	    {
	    /** setup **/
	    flags = 0;
	    lxs = mlxStringSession(str, flags | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    /* read in the first token. This should read the first 256 bytes */
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    strval = lxs->TokString;
	    assert(strval != NULL);
	    assert(strcmp(strval,tokstr) == 0);
	    /* now peform a copy into a buffer twice the size. Should enable invalid ascii to bypass the 
	     * checks in next token, but should be caught by the check at the end of copy
	     */
	    int result = mlxCopyToken(lxs, buf, BUF_SIZE);
	    assert(result == strlen(str)-2); /* -2 since ' is not included */
	    str[strlen(str)-1] = '\0';  /* change for test */
	    assert(strcmp(buf, str+1) == 0);
	    str[strlen(str)] = '\'';  /* revert back. Note string is 1 shorter now */
	    assert(lxs->TokType == MLX_TOK_ERROR);
	    mlxCloseSession(lxs);


	    /** trying again without ascii flag results in a normal token **/
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
	    /* read in the first token. This should read the first 256 bytes */
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    strval = lxs->TokString;
	    assert(strval != NULL);
	    assert(strcmp(strval,tokstr) == 0);
	    /* now test again */
	    result = mlxCopyToken(lxs, buf, BUF_SIZE);
	    str[strlen(str)-1] = '\0';  /* change for test */
	    assert(strcmp(buf, str+1) == 0);
	    str[strlen(str)] = '\'';  /* revert back */
	    assert(lxs->TokType == MLX_TOK_STRING);
	    }

    return iter;
    }

