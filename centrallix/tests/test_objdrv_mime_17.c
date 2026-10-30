#include <assert.h>
#include <string.h>

#include "centrallix.h"
#include "mime.h"

int libmime_PartRead(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags);

/** Decoded size of the part: several refills of the decoded buffer. **/
#define PART_LEN	3000

/** A message file holding one base64 part. **/
typedef struct
    {
    char	Data[PART_LEN * 2];
    int		Len;
    }
    MsgFile;

typedef struct
    {
    int		Offset;	/* where the read starts */
    int		Count;	/* bytes asked for */
    int		ExpLen;	/* bytes expected back */
    }
    Case;

/** Reads run in table order, so each one seeks from where the last one stopped. **/
static Case cases[] =
    {
    /**	  Offset	Count	ExpLen **/

    /** Forward from the start, then far past several refills. **/
	{ 0,		100,	100 },
	{ 2500,		200,	200 },

    /** Backward to data that is no longer buffered. **/
	{ 100,		50,	50 },

    /** Across a refill of the decoded buffer. **/
	{ 700,		200,	200 },

    /** Past the end of the part. **/
	{ 2950,		100,	50 },
	{ PART_LEN,	10,	0 },
    };

static int
file_read(void* v, char* buf, int cnt, int offset, int flags)
    {
    MsgFile* f = (MsgFile*)v;
    int n;

	n = f->Len - offset;
	if (n > cnt)
	    n = cnt;
	if (n < 0)
	    n = 0;
	memcpy(buf, f->Data + offset, n);

    return n;
    }

long long
test(char** tname)
    {
    static MsgFile file;
    static unsigned char plain[PART_LEN];
    char out[PART_LEN];
    MimeData mdat;
    pMimeHeader hdr;
    int ncases = sizeof(cases) / sizeof(Case);
    int c, i, n, got, rcnt;

	/*** This test verifies that seeking reads of a base64 part return
	 *** the right bytes, whether the offset is ahead of, behind, or
	 *** across the buffered chunk, or past the end of the part.
	 ***/

	*tname = "objdrv_mime_17 seeking base64 part reads";

	/** Build the file: one base64 part in 76-character lines. **/
	for(i=0;i<PART_LEN;i++)
	    plain[i] = (unsigned char)(i % 251);
	for(i=0;i<PART_LEN;i+=57)
	    {
	    n = (PART_LEN - i < 57) ? (PART_LEN - i) : 57;
	    file.Len += libmime_EncodeBase64((unsigned char*)file.Data + file.Len, plain + i, sizeof(file.Data) - file.Len, n);
	    file.Data[file.Len++] = '\n';
	    }
	hdr = libmime_AllocateHeader();
	assert(hdr != NULL); /* header allocated */
	assert(libmime_CreateIntAttr(hdr, "Content-Transfer-Encoding", NULL, MIME_ENC_BASE64) == 0); /* encoding set */
	hdr->MsgSeekStart = 0;
	hdr->MsgSeekEnd = file.Len;
	memset(&mdat, 0, sizeof(MimeData));
	mdat.Parent = &file;
	mdat.ReadFn = file_read;

	/** Each read returns the part's bytes at its offset. **/
	for(c=0;c<ncases;c++)
	    {
	    got = 0;
	    do  {
		rcnt = libmime_PartRead(&mdat, hdr, out + got, cases[c].Count - got, cases[c].Offset + got, FD_U_SEEK);
		assert(rcnt >= 0); /* no decode error */
		got += rcnt;
		} while (rcnt > 0 && got < cases[c].Count);
	    assert(got == cases[c].ExpLen); /* expected length read */
	    assert(memcmp(out, plain + cases[c].Offset, got) == 0); /* bytes from the offset */
	    }

	libmime_DeallocateHeader(hdr);

    return 0;
    }
