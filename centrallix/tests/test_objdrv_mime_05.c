#include <assert.h>
#include <string.h>

#include "centrallix.h"
#include "mime.h"

int libmime_PartRead(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags);

/** Decoded size of each part: several refills of the encoded buffer. **/
#define PART_LEN	3000

/** A message file with one read position, shared by every part read from it. **/
typedef struct
    {
    char	Data[2 * PART_LEN * 2];
    int		Len;
    int		Pos;
    }
    MsgFile;

static int
file_read(void* v, char* buf, int cnt, int offset, int flags)
    {
    MsgFile* f = (MsgFile*)v;
    int n;

	if (flags & FD_U_SEEK)
	    f->Pos = offset;
	n = f->Len - f->Pos;
	if (n > cnt)
	    n = cnt;
	memcpy(buf, f->Data + f->Pos, n);
	f->Pos += n;

    return n;
    }

/** Append the base64 encoding of src to the file, in 76-character lines. **/
static void
file_add_base64(MsgFile* f, unsigned char* src, int len)
    {
    int i, n;

	for(i=0;i<len;i+=57)
	    {
	    n = (len - i < 57) ? (len - i) : 57;
	    f->Len += libmime_EncodeBase64(f->Data + f->Len, src + i, sizeof(f->Data) - f->Len, n);
	    f->Data[f->Len++] = '\n';
	    }
    }

long long
test(char** tname)
    {
    static MsgFile file;
    static unsigned char plain[2][PART_LEN];
    static char out[2][PART_LEN + 1];
    MimeData mdat[2];
    pMimeHeader hdr[2];
    int got[2] = { 0, 0 };
    int i, p, rcnt, active;

	/*** This test verifies that two base64 parts of one message decode
	 *** correctly when read in alternating chunks through the same file,
	 *** so every buffer refill must seek to its own part's data.
	 ***/

	*tname = "objdrv_mime_05 alternating base64 part reads";

	/** Build the file: two base64 parts with different contents. **/
	for(p=0;p<2;p++)
	    {
	    for(i=0;i<PART_LEN;i++)
		plain[p][i] = (unsigned char)(p ? (255 - i % 251) : (i % 251));
	    hdr[p] = libmime_AllocateHeader();
	    assert(hdr[p] != NULL); /* header allocated */
	    assert(libmime_CreateIntAttr(hdr[p], "Content-Transfer-Encoding", NULL, MIME_ENC_BASE64) == 0); /* encoding set */
	    hdr[p]->MsgSeekStart = file.Len;
	    file_add_base64(&file, plain[p], PART_LEN);
	    hdr[p]->MsgSeekEnd = file.Len;
	    memset(&mdat[p], 0, sizeof(MimeData));
	    mdat[p].Parent = &file;
	    mdat[p].ReadFn = file_read;
	    }

	/** Read the parts in alternating chunks until both are done. **/
	do  {
	    active = 0;
	    for(p=0;p<2;p++)
		{
		rcnt = libmime_PartRead(&mdat[p], hdr[p], out[p] + got[p], 100, got[p], 0);
		assert(rcnt >= 0); /* no decode error */
		assert(got[p] + rcnt <= PART_LEN); /* no extra data */
		got[p] += rcnt;
		active |= (rcnt > 0);
		}
	    } while (active);

	/** Each part decodes to its own contents. **/
	for(p=0;p<2;p++)
	    {
	    assert(got[p] == PART_LEN); /* whole part read */
	    assert(memcmp(out[p], plain[p], PART_LEN) == 0); /* part decoded correctly */
	    libmime_DeallocateHeader(hdr[p]);
	    }

    return 0;
    }
