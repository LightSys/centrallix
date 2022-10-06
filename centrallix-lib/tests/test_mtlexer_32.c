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
    int alloc = 0;
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
	char* buf;

	*tname = "mtlexer-32 test String Val with invalid ascii characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 20000;
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
		buf = mlxStringVal(lxs, &alloc);
		assert(buf == NULL);
		assert(alloc == 0);
		assert(lxs->TokType == MLX_TOK_ERROR);
		mlxCloseSession(lxs);


		/** trying again without ascii flags results in a normal token **/
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
		buf = mlxStringVal(lxs, &alloc);
		assert(buf != NULL);
		assert(alloc);
		str[strlen(str)-1] = '\0';  /* change for test */
		assert(strcmp(buf, str+1) == 0);
		str[strlen(str)] = '\'';  /* revert back */
		assert(lxs->TokType == MLX_TOK_STRING);
	    }

    return iter;
    }

