#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>

#include "centrallix.h"
#include "mime.h"

int libmime_PartRead(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags);

/** Room for the largest part, with space left over to catch extra data. **/
#define MAX_PART	(128 * 1024)

typedef struct
    {
    char*	File;	/* message in centrallix-os/tests/mimetest */
    int		Part;	/* index of the part to read, or -1 for the message itself */
    int		Len;	/* decoded length */
    char*	Md5;	/* MD5 of the decoded data */
    }
    Case;

static Case cases[] =
    {
    /**	  File			Part	Len	Md5 **/

    /** Images. **/
	{ "EM1.1.2-GIF.eml",	-1,	2610,	"c405b40f30c3237dfe5b63e4cbb40e29" },
	{ "EM2.1.eml",		-1,	2177,	"71e9717c1af35da79b2fd7977172ee0c" },
	{ "EM2.2.eml",		-1,	2784,	"795762185a1aa9b9fa1338f7433ba872" },
	{ "EM2.3.eml",		-1,	11774,	"8374d78a682e0df69386f8b739322a56" },

    /** Audio, video, and applications. **/
	{ "EM3.1.eml",		-1,	39128,	"5a640a915b74c0ae7f600eaa45fb63a3" },
	{ "EM7.1.eml",		-1,	105507,	"1a0cf7817eae60c635c566b984e789eb" },
	{ "EM4.1.eml",		-1,	2048,	"ae5ab35b88a6f5273198814b2171aed1" },
	{ "EM5.1.eml",		0,	429,	"76d5b4c2c8401d4ecccb757f06ab4055" },
    };

long long
test(char** tname)
    {
    static char data[MAX_PART];
    char path[256];
    char md5[33];
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen;
    int ncases = sizeof(cases) / sizeof(Case);
    int c, i, got, rcnt;
    pFile fd;
    pLxSession lex;
    pMimeHeader msg, part;
    MimeData mdat;

	/*** This test verifies that the binary base64 parts of the IMC
	 *** MimeTest messages decode to their exact contents, including
	 *** bytes past the first null, which SQL queries cannot see.
	 ***/

	*tname = "objdrv_mimetest_10 MimeTest binary base64 parts";
	for(c=0;c<ncases;c++)
	    {
	    /** Parse the message. **/
	    snprintf(path, sizeof(path), "../centrallix-os/tests/mimetest/%s", cases[c].File);
	    fd = fdOpen(path, O_RDONLY, 0);
	    assert(fd != NULL); /* message opened */
	    lex = mlxGenericSession(fd, fdRead, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
	    assert(lex != NULL); /* lexer opened */
	    msg = libmime_AllocateHeader();
	    assert(msg != NULL); /* header allocated */
	    assert(libmime_ParseHeader(lex, msg, 0, 0) == 0); /* header parsed */
	    assert(libmime_ParseMultipartBody(lex, msg, msg->MsgSeekStart, msg->MsgSeekEnd) == 0); /* body parsed */
	    mlxCloseSession(lex);
	    part = (cases[c].Part < 0) ? msg : (pMimeHeader)xaGetItem(&msg->Parts, cases[c].Part);
	    assert(part != NULL); /* part found */

	    /** Read the whole part. **/
	    memset(&mdat, 0, sizeof(MimeData));
	    mdat.Parent = fd;
	    mdat.ReadFn = fdRead;
	    got = 0;
	    do  {
		rcnt = libmime_PartRead(&mdat, part, data + got, 1000, got, FD_U_SEEK);
		assert(rcnt >= 0); /* no decode error */
		got += rcnt;
		assert(got + 1000 <= MAX_PART); /* no extra data */
		} while (rcnt > 0);

	    /** The part decodes to its own contents. **/
	    assert(got == cases[c].Len); /* whole part read */
	    assert(EVP_Digest(data, got, digest, &dlen, EVP_md5(), NULL) == 1); /* digest computed */
	    for(i=0;i<16;i++)
		snprintf(md5 + 2*i, 3, "%02x", digest[i]);
	    assert(strcmp(md5, cases[c].Md5) == 0); /* part decoded correctly */

	    libmime_DeallocateHeader(msg);
	    fdClose(fd, 0);
	    }

    return 0;
    }
