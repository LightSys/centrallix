#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>

long long
test(char** tname)
    {
    const int NUM_TOK = 4;
    int i;
    int j;
    int t;
    int n;
    int iter;
    pLxSession lxs;
    pFile fd;
    char buf[256];
    char* aBuf;
    int alloc;

	*tname = "mtlexer-33 Read long utf-8 tokens from a file";

	mssInitialize("system", "", "", 0, "test");

	iter = 60000;

	for(i=0;i<iter;i++)
	    {
	    /** normal **/
	    fd = fdOpen("tests/test_mtlexer_33.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd,  MLX_F_EOF | MLX_F_LINEONLY | MLX_F_NODISCARD);
	    assert(lxs != NULL);

	    for(j=0 ; j < NUM_TOK ; j++)
		{
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		}
	    assert(strcmp(lxs->TokString, "𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀭𓀮𓀯𓀰𓀱𓀲𓀳𓀴𓀵𓀶𓀷𓀸𓀹𓀺𓀻𓀼𓀽𓀾\xF0\x93\x80") == 0);
	    aBuf = mlxStringVal(lxs, &alloc);
	    assert(strcmp(aBuf, "𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀭𓀮𓀯𓀰𓀱𓀲𓀳𓀴𓀵𓀶𓀷𓀸𓀹𓀺𓀻𓀼𓀽𓀾𓀿"
	        "𓁀𓁁𓁂𓁃𓁄𓁅𓁆𓁇𓁉𓁊𓁋𓁌𓁍𓁎𓁏𓁐𓁑𓁒𓁓𓁔𓁕𓁖𓁗𓁘𓁙𓁚𓁛𓁜𓁝𓁞𓁟𓁠𓁡𓁢𓁣𓁤𓁥𓁦𓁧𓁨𓁩𓁪𓁫𓁬𓁭𓁮𓁯𓁰𓁱𓁲𓁳𓁴𓁵𓁶𓁷𓁸𓁹𓁺𓁻𓁼𓁽𓁾𓁿") == 0);
	    nmSysFree(aBuf);

	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    mlxCloseSession(lxs);
	    assert(n >= 0);
	    buf[n] = '\0';
	    fdClose(fd, 0);

	    /** utf-8 **/
	    fd = fdOpen("tests/test_mtlexer_33.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, MLX_F_EOF | MLX_F_LINEONLY | MLX_F_NODISCARD | MLX_F_ENFORCEUTF8);
	    assert(lxs != NULL);
	    for(j=0 ; j < NUM_TOK ; j++)
		{
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		}
	    assert(strcmp(lxs->TokString, "𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀭𓀮𓀯𓀰𓀱𓀲𓀳𓀴𓀵𓀶𓀷𓀸𓀹𓀺𓀻𓀼𓀽𓀾") == 0);
	    aBuf = mlxStringVal(lxs, &alloc);
	    assert(strcmp(aBuf, "𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀭𓀮𓀯𓀰𓀱𓀲𓀳𓀴𓀵𓀶𓀷𓀸𓀹𓀺𓀻𓀼𓀽𓀾𓀿"
	        "𓁀𓁁𓁂𓁃𓁄𓁅𓁆𓁇𓁉𓁊𓁋𓁌𓁍𓁎𓁏𓁐𓁑𓁒𓁓𓁔𓁕𓁖𓁗𓁘𓁙𓁚𓁛𓁜𓁝𓁞𓁟𓁠𓁡𓁢𓁣𓁤𓁥𓁦𓁧𓁨𓁩𓁪𓁫𓁬𓁭𓁮𓁯𓁰𓁱𓁲𓁳𓁴𓁵𓁶𓁷𓁸𓁹𓁺𓁻𓁼𓁽𓁾𓁿") == 0);
	    nmSysFree(aBuf);
	    
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    mlxCloseSession(lxs);
	    n = fdRead(fd, buf, sizeof(buf) - 1, 0, 0);
	    assert(n >= 0);
	    buf[n] = '\0';
	    fdClose(fd, 0);

	    /** ascii **/
	    fd = fdOpen("tests/test_mtlexer_33_ASCII.txt", O_RDONLY, 0600);
	    assert(fd != NULL);
	    lxs = mlxOpenSession(fd, MLX_F_EOF | MLX_F_LINEONLY | MLX_F_NODISCARD | MLX_F_ENFORCEASCII);
	    assert(lxs != NULL);
	    for(j=0 ; j < NUM_TOK ; j++)
		{
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		}

	    assert(strcmp(lxs->TokString, "hid in mine heart, that I might not sin against thee. 119:12 Blessed art thou,"
		" O LORD: teach me thy statutes. 119:13 With my lips have I declared all the judgments of thy mouth. 119:"
		"14 I have rejoiced in the way of thy testimonies, as much as in all riche") == 0);
	    aBuf = mlxStringVal(lxs, &alloc);
	    assert(strcmp(aBuf, "hid in mine heart, that I might not sin against thee. 119:12 Blessed art thou, O LORD: "
		"teach me thy statutes. 119:13 With my lips have I declared all the judgments of thy mouth. 119:14 I "
		"have rejoiced in the way of thy testimonies, as much as in all riches. 119:15 I will meditate in thy "
		"precepts, and have respect unto thy ways. 119:16 I will delight myself in thy statutes: I will not forget"
		" thy word. 119:17 Deal bountifully with thy servant, that I may live, and keep thy word. 119:18 Open thou"
		" mine eyes, that I may behold wondrous things out of thy law.") == 0);
	    nmSysFree(aBuf);
	    
	    t = mlxNextToken(lxs);
	    assert(t == MLX_TOK_STRING);
	    mlxCloseSession(lxs);
	    n = fdRead(fd, buf, sizeof(buf) - 1, 0, 0);
	    assert(n >= 0);
	    buf[n] = '\0';
	    fdClose(fd, 0);
	    }

    return iter;
    }

