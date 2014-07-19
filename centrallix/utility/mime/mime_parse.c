/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	libmime							*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	August 12, 2002						*/
/* Description:	Provides MIME parsing facilities used mainly in the	*/
/*		MIME object system driver (objdrv_mime.c)		*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "mime.h"

char* TypeStrings[] =
    {
    "text",
    "multipart",
    "application",
    "message",
    "image",
    "audio",
    "video"
    };

char* EncodingStrings[] =
    {
    "7bit",
    "8bit",
    "base64",
    "quoted-printable",
    "binary"
    };

/***  libmime_ParseHeader
 ***
 ***  Parses a message (located at obj->Prev) starting at the "start" byte, and ending
 ***  at the "end" byte.  This creates the MimeHeader data structure and recursively
 ***  calls itself to fill it in.  Note that no data is actually stored.  This is just
 ***  the shell of the message and contains seek points denoting where to start
 ***  and end reading.
 ***/
int
libmime_ParseHeader(pLxSession lex, pMimeHeader msg, long start, long end)
    {
    int flag, toktype, alloc, len;
    XString xsbuf;
    char *hdrnme, *hdrbdy;
    long attrSeekStart = start, attrSeekEnd = start, nameOffset;

    /** Initialize the message structure **/
    libmime_CreateStringAttr(msg, "Content-Type", NULL, "text/plain", 0);
    libmime_CreateIntAttr(msg, "Content-Type", "ContentMainType", MIME_TYPE_TEXT);
    libmime_CreateStringAttr(msg, "Content-Type", "ContentSubType", "plain", 0);
    libmime_CreateIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_7BIT);
    msg->MsgSeekStart = 0;
    msg->MsgSeekEnd = 0;

    if (!lex)
	{
	return -1;
	}

    /** Seek to the beginning of the message and store the offset. **/
    mlxSetOffset(lex, start);
    msg->HdrSeekStart = start;

    flag = 1;
    while (flag)
	{
	mlxSetOptions(lex, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR || toktype == MLX_TOK_EOF)
	    {
	    return -1;
	    }
	/** get the next line **/
	alloc = 0;
	xsInit(&xsbuf);
	xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
	len = strlen(xsbuf.String);
	xsRTrim(&xsbuf);
	/** check if this is the end of the headers, if so, exit the loop (flag=0), **/
	/** otherwise parse the header elements **/
	if (!strlen(xsbuf.String))
	    {
	    flag = 0;
	    }
	else
	    {
	    if (libmime_LoadExtendedHeader(lex, msg, &xsbuf, &attrSeekEnd) < 0)
		{
		return -1;
		}

	    hdrnme = (char*)nmMalloc(64);
	    hdrbdy = (char*)nmMalloc(strlen(xsbuf.String)+1);
	    strncpy(hdrbdy, xsbuf.String, strlen(xsbuf.String)+1);
	    if (!libmime_ParseHeaderElement(hdrbdy, hdrnme, &attrSeekStart, &nameOffset))
		{
		/** Parse the attribute and store it in the Mime header. **/
		if (libmime_ParseAttr(msg, hdrnme, hdrbdy, attrSeekStart, attrSeekEnd, nameOffset))
		    {
		    mssError(0, "MIME", "ERROR PARSING \"%s\": \"%s\"\n", hdrnme, hdrbdy);
		    }
		}
	    else
		{
		mssError(1, "MIME", "ERROR PARSING: %s\n", xsbuf.String);
		}

	    /** Get the offset at the beginning of the next attribute. **/
	    attrSeekStart = attrSeekEnd;
	    }
	xsDeInit(&xsbuf);

	msg->HdrSeekEnd = attrSeekEnd;
	}

    /** Set the start and end offsets for the message **/
    msg->MsgSeekStart = mlxGetOffset(lex) + len;
    /** If an end offset was passed in, use it!  If not (end==0), then find the end **/
    if (end)
	{
	msg->MsgSeekEnd = end;
	}
    else
	{
	flag = 1;
	msg->MsgSeekEnd = msg->MsgSeekStart;
	mlxSetOffset(lex, msg->MsgSeekStart);
	while (flag)
	    {
	    mlxSetOptions(lex, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
	    toktype = mlxNextToken(lex);
	    if (toktype == MLX_TOK_ERROR || toktype == MLX_TOK_EOF)
		{
		flag = 0;
		}
	    else
		{
		xsInit(&xsbuf);
		xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
		msg->MsgSeekEnd += strlen(xsbuf.String);
		xsDeInit(&xsbuf);
		}
	    }
	}

    return 0;
    }


/***  libmime_LoadExtendedHeader
 ***
 ***  Header elements can span multiple lines.  We know that this occurs when there
 ***  is any white space at the beginning of the line.  This function will check if
 ***  there are any more lines that belong to the current header element.  If so,
 ***  they will be read into xsbuf replacing all white spaces with just normal spaces.
 ***/

int
libmime_LoadExtendedHeader(pLxSession lex, pMimeHeader msg, pXString xsbuf, int* attrSeekEnd)
    {
    int toktype, i;
    unsigned long offset;
    char *ptr;

    while (1)
	{
	offset = mlxGetCurOffset(lex);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR)
	    {
	    return -1;
	    }
	ptr = mlxStringVal(lex, NULL);
	if (!strchr(" \t", ptr[0])) break;
	xsConcatPrintf(xsbuf, " %s", ptr);
	}
    /** Be kind, rewind! (resetting the offset because we don't use the last string it fetched) **/
    mlxSetOffset(lex, offset);

    /** Store the offset at the end of the attribute string. **/
    *attrSeekEnd = offset;

    /** Set all tabs, NL's, CR's to spaces **/
    for(i=0;i<strlen(xsbuf->String);i++) if (strchr("\t\r\n",xsbuf->String[i])) xsbuf->String[i]=' ';

    return 0;
    }

/***  libmime_SetDate
 ***
 ***  Parses the "Date" header element and fills in the MimeHeader data structure
 ***  with the data accordingly.  If certain elements are not there, defaults are used.
 ***/
int
libmime_SetDate(pMimeHeader msg, char *buf)
    {
    /** Get the date **/

    /** FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME:
     **   objDataToDateTime does not currently behave properly.  When that function
     **   gets fixed, fix this!!
     **/
    // objDataToDateTime(DATA_T_STRING, xsptr->String, &msg->PartDate, NULL);

    return 0;
    }

/***  libmime_SetTransferEncoding
 ***
 ***  Parses the "Content-Transfer-Encoding" header element and fills in the MimeHeader data structure
 ***  with the data accordingly.  If certain elements are not there, defaults are used.
 ***/
int
libmime_SetTransferEncoding(pMimeHeader msg, char *buf)
    {
	if (!strlen(buf) || !strcasecmp(buf, "7bit"))
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_7BIT);
	else if (!strcasecmp(buf, "8bit"))
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_8BIT);
	else if (!strcasecmp(buf, "base64"))
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_BASE64);
	else if (!strcasecmp(buf, "quoted-printable"))
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_QP);
	else if (!strcasecmp(buf, "binary"))
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_BINARY);
	else
	    libmime_SetIntAttr(msg, "Transfer-Encoding", NULL, MIME_ENC_7BIT);

    return 0;
    }

/***  libmime_SetContentType
 ***
 ***  Parses the "Content-Type" header element and fills in the MimeHeader data structure
 ***  with the data accordingly.  If certain elements are not there, defaults are used.
 ***/
int
libmime_SetContentType(pMimeHeader msg, char *buf)
    {
    char *ptr, *cptr;
    char maintype[32];
    int i;
    ptrdiff_t len;

	/** Get the disp main type and subtype **/
	if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;

	/** Store the raw content type string. **/
	libmime_StringToLower(ptr);
	libmime_SetStringAttr(msg, "Content-Type", NULL, ptr, -1);

	/** Parse out the secondary content type. **/
	if ((cptr=strchr(ptr,'/')))
	    {
	    len = cptr - ptr;
	    if (len>31) len=31;
	    strncpy(maintype, ptr, len);
	    maintype[len] = 0;
	    libmime_StringToLower(cptr+1);
	    libmime_SetStringAttr(msg, "Content-Type", "ContentSubType", cptr+1, -1);
	    }
	else
	    {
	    strncpy(maintype, ptr, 31);
	    maintype[31] = 0;
	    }

	/** Determine the primary content type. **/
	for (i=0; i<7; i++)
	    {
	    if (!libmime_StringFirstCaseCmp(maintype, TypeStrings[i]))
		{
		libmime_SetIntAttr(msg, "Content-Type", "ContentMainType", i);
		break;
		}
	    }

    return 0;
    }

/*** libmime_SetFilename - Sets the Name attribute based on other relevant
 *** attributes. If no name is indicated by other attributes, the given default
 *** name is used.
 ***/
int
libmime_SetFilename(pMimeHeader msg, char *defaultName)
    {
    char *fileName = NULL;

	/** Get the name from the Content-Distribution attribute. **/
	if (!libmime_GetStringAttr(msg, "Content-Disposition", "Filename", &fileName))
	    {
	    libmime_GetStringAttr(msg, "Content-Disposition", "Name", &fileName);
	    }

	/** If found, store the name in the Name attribute. **/
	if (fileName)
	    {
	    if (libmime_SetStringAttr(msg, "Name", NULL, fileName, -1))
		{
		mssError(0, "MIME", "Failed to create the name attribute.");
		return -1;
		}
	    return 0;
	    }

	/** Get the name from the Content-Type attribute.
	 ** If found, store the name in the Name attribute.
	 **/
	if (!libmime_GetStringAttr(msg, "Content-Type", "Name", &fileName))
	    {
	    if (libmime_SetStringAttr(msg, "Name", NULL, fileName, -1))
		{
		mssError(0, "MIME", "Failed to create the name attribute.");
		return -1;
		}
	    return 0;
	    }

	/** If neither is found, use the default name. **/
	libmime_SetStringAttr(msg, "Name", NULL, defaultName, -1);

    return 0;
    }

/***
 ***  int
 ***  libmime_ParseHeaderElement(char* buf, char* hdr);
 ***     Parameters:
 ***         (char*) buf            A string of characters with no CRLF's in it.  This
 ***                                string should represent the whole header, including any
 ***                                folded header elements below itself.  This string will
 ***                                be modified to contain the main part of the header.
 ***         (char*) hdr            This string will be overwritten with a string that
 ***                                is the name of the header element (To, From, Sender...)
 ***         (int*)  attrSeekStart  Pointer to an integer indicating the seek offset
 ***                                to the beginning of the attribute value.
 ***     Returns:
 ***         This function returns 0 on success, and -1 on failure.  It modifies
 ***         the "buf" parameter and sends its work back in this way.  This
 ***         function will return a string of characters that is properly
 ***         formatted according to RFC822.  The header tag will be stripped away
 ***         from the beginning ("X-Header"), all extra whitespace will be
 ***         removed, and all comments will be removed as well.  This will be a
 ***         clean header line.
 ***
 ***     State Definitions:
 ***         0 == We have only seen non-space, non-tab, and non-colon characters
 ***              up to this point.  As soon as one of those characters is seen,
 ***              the state will change.
 ***         1 == We have seen a whitespace character, thus only a colon or more
 ***              whitespace should be visible.  If not, return an error.
 ***         2 == We have seen the colon!  The next character is the beginning of
 ***              the header content.  Trim and return that string.
 ***/

int
libmime_ParseHeaderElement(char *buf, char* hdr, int* attrSeekStart, int* nameOffset)
    {
    int count=0, state=0;
    char *ptr;
    char ch;
    while (count < strlen(buf))
	{
	ch = buf[count];
	/**  STATE 0 (no spaces or colons have been seen) **/
	if (state == 0)
	    {
	    if (ch == ':')
		{
		state = 2;
		}
	    else if (ch==' ' || ch=='\t')
		{
		state = 1;
		}
	    }
	/** STATE 1 (space has been seen, only spaces and a colon should follow) **/
	else if (state == 1)
	    {
	    if (ch == ':')
		{
		state = 2;
		}
	    else if (ch!=' ' && ch!='\t')
		{
		return -1;
		}
	    }
	/** STATE 2 (the colon has been spotted, left side is header, right is body **/
	else if (state == 2)
	    {
	    memcpy(hdr, buf, (count-1>79?79:count-1));
	    hdr[(count-1>79?79:count-1)] = 0;
	    memmove(buf, &buf[count+1], strlen(&buf[count+1])+1);
	    ptr = hdr; /* Shanghai'ed or rather, captured/destroyed/pillaged */
	    libmime_StringTrim(hdr);
	    libmime_StringTrim(buf);
	
	    /** Store the count of characters between the beginning of the name and the value. **/
	    *nameOffset = count + 1;

	    /** Store the count of characters between the beginning of the name and the value. **/
	    *nameOffset = count + 1;

	    /** Add the offset of the name to the start offset. **/
	    *attrSeekStart += hdr - ptr;
	    return 0;
	    }
	count++;
	}
    return -1;
    }

/***
 ***  int
 ***  libmime_ParseMultipartBody
 ***
 ***  Parses the body of a multipart message.  This fills in the Parts section of the
 ***  pMimeHeader data structure.  It will start parsing at the "start" location, and
 ***  will keep parsing until all the boundaries have been found or until the byte "end"
 ***  has been reached.
 ***/
int
libmime_ParseMultipartBody(pLxSession lex, pMimeHeader msg, int start, int end)
    {
    XString xsbuf;
    pMimeHeader l_msg;
    int flag=1, alloc, toktype, p_count=0, count=0, s=0, num=0;
    int l_pos=0;
    char bound[80], bound_end[82], ext[5], buf[80];
    char* sub_type = NULL;
    int main_type;

    if (!lex)
	{
	return -1;
	}
    mlxSetOffset(lex, msg->MsgSeekStart);
    count = msg->MsgSeekStart;

    libmime_GetStringAttr(msg, "Content-Type", "Boundary", &sub_type); /* Reusing variable :P */
    snprintf(bound, 79, "--%s", sub_type);
    snprintf(bound_end, 81, "--%s--", sub_type);
    bound[79] = 0;
    bound_end[81] = 0;

    while (flag)
	{
	mlxSetOptions(lex, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR || end <= count || toktype == MLX_TOK_EOF)
	    {
	    flag = 0;
	    }
	else
	    {
	    alloc = 0;
	    xsInit(&xsbuf);
	    xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
	    count = mlxGetOffset(lex);
	    /** Check if this is the start of a boundary **/
	    if (!strncmp(xsbuf.String, bound, strlen(bound)))
		{
		if (l_pos != 0)
		    {
		    l_msg = libmime_AllocateHeader();
		    if (!l_msg) return -1;

		    libmime_ParseHeader(lex, l_msg, l_pos+s, p_count);
		    xaAddItem(&msg->Parts, l_msg);
		    num++;

		    /** Check for an extension for the file in order to calculate the default filename. **/
		    if (!libmime_GetIntAttr(l_msg, "Content-Type", "ContentMainType", &main_type)  &&
			    !libmime_GetStringAttr(l_msg, "Content-Type", "ContentSubType", &sub_type) &&
			    libmime_ContentExtension(ext, main_type, sub_type))
			{
			sprintf(buf, "attachment%d.%s", num, ext);
			}
		    else
			{
			sprintf(buf, "attachment%d", num);
			}

		    /** Set the Name attribute. **/
		    libmime_SetFilename(l_msg, buf);

		    if (!libmime_GetIntAttr(l_msg, "Content-Type", "ContentMainType", &main_type) &&
			    main_type == MIME_TYPE_MULTIPART)
			{
			libmime_ParseMultipartBody(lex, l_msg, l_msg->MsgSeekStart, l_msg->MsgSeekEnd);
			}
		    }
		s=strlen(xsbuf.String);
		l_pos = count;
		/** Check if it is the boundary end **/
		if (!strncmp(xsbuf.String, bound_end, strlen(bound_end)))
		    {
		    flag = 0;
		    }
		mlxSetOffset(lex, l_pos+s);
		}
	    p_count = count + strlen(xsbuf.String);
	    xsDeInit(&xsbuf);
	    }
	}

    return 0;
    }

/***
 ***  libmime_PartRead
 ***
 ***  Using nearly the same interface as objRead (except for the first
 ***  parameter), this function will read an arbitrary number of bytes from a
 ***  MIME part, doing all the decoding of that part behind the scenes (as
 ***  specified by the Content-Transfer-Encoding header element).
 ***/
int
libmime_PartRead(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags)
    {
    int size=0, bytes_left, len, rem=0, end;
    int tlen, tsize, tremoved, trem_total, toffset, tleft;  /* these are used for getting a purified b64 chunk */
    char *ptr, *bptr, *tptr;
    int transfer_encoding;

    libmime_GetIntAttr(msg, "Transfer-Encoding", NULL, &transfer_encoding);
    switch (transfer_encoding)
	{
	/** 7BIT AND 8BIT ENCODING **/
	/** BINARY ENCODING **/
	/** QUOTED-PRINTABLE ENCODING **/
	case MIME_ENC_7BIT:
	case MIME_ENC_8BIT:
	case MIME_ENC_BINARY:  /*  Split this off to its own if needed at some point  */
	case MIME_ENC_QP:  /*  Not currently supported, just print the text  */
	    if (msg->MsgSeekStart+offset > msg->MsgSeekEnd)
		return 0;
	    if (msg->MsgSeekStart+offset+maxcnt > msg->MsgSeekEnd)
		maxcnt = msg->MsgSeekEnd - (msg->MsgSeekStart + offset);
	    size = mdat->ReadFn(mdat->Parent, buffer, maxcnt, msg->MsgSeekStart+offset, FD_U_SEEK);
	    break;
	/** BASE64 ENCODING **/
	case MIME_ENC_BASE64:
	    ptr = buffer;
	    if (flags & FD_U_SEEK)
		{
		mdat->InternalSeek = offset;
		}
	    bytes_left = maxcnt;

	    end = 0;
	    while (bytes_left > 0 && !end && mdat->ExternalChunkSeek <= msg->MsgSeekEnd)
		{
		/**  Figure out what chunk we're inside  **/
		mdat->InternalChunkSeek = (int)(mdat->InternalSeek/MIME_BUF_SIZE)*MIME_BUF_SIZE;
		mdat->ExternalChunkSeek = (int)(mdat->InternalSeek/MIME_BUF_SIZE)*MIME_ENCBUF_SIZE+rem;
		/**
		 **  If the InternalSeek is not inside the chunk that is already buffered
		 **  then we need to rebuffer.  We also need to rebuffer if there is nothing
		 **  currently buffered.
		 **/
		if (!mdat->InternalChunkSize || (mdat->InternalSeek <= mdat->InternalChunkSeek || mdat->InternalSeek > (mdat->InternalChunkSeek + mdat->InternalChunkSize)))
		    {
		    /**
		     **  Figure out the ExternalChunkSize (number of b64 characters to get).
		     **  This checks if we're trying to read past the end or not.  Note that
		     **  this number includes characters that are not in the b64 alphabet.
		     **  The next code chunk goes through and purifies the stream, refilling
		     **  the buffer as necessary.  This is just the initial chunk.
		     **/
		    mdat->ExternalChunkSize = MIME_ENCBUF_SIZE;
		    if (msg->MsgSeekEnd < (msg->MsgSeekStart + mdat->ExternalChunkSeek + MIME_ENCBUF_SIZE))
			mdat->ExternalChunkSize = msg->MsgSeekEnd - msg->MsgSeekStart - mdat->ExternalChunkSeek;

		    /**
		     **  Now we need to fetch a chunk of base64 encoded data.  The only
		     **  problems is that we need to ignore anything that is not in the
		     **  base64 alphabet.  Here, I'm looping through and filling up the
		     **  buffer with base64 encoded data until the buffer is full or
		     **  until I've reached the end of the stream, ignoring all characters
		     **  that are not part of the base64 alphabet.
		     **/
		    tptr = mdat->EncBuffer;
		    tlen = tsize = toffset = tremoved = trem_total = 0;
		    tleft = mdat->ExternalChunkSize;
		    while (tleft > 0)
			{
			if (msg->MsgSeekEnd < msg->MsgSeekStart + mdat->ExternalChunkSeek + tlen + tleft)
			    {
			    toffset = msg->MsgSeekEnd - (msg->MsgSeekStart + mdat->ExternalChunkSeek + tlen);
			    tleft = 0;
			    }
			else
			    {
			    toffset = msg->MsgSeekStart + mdat->ExternalChunkSeek + tlen;
			    }
			tsize = mdat->ReadFn(mdat->Parent, tptr, tleft, toffset, FD_U_SEEK);
			tlen += tsize;
			tremoved = libmime_B64Purify(mdat->EncBuffer); /* tremoved is the number of chars removed this iteration */
			trem_total += tremoved; /* trem_total is the total number of chars removed for this chunk */
			tptr += (tsize - tremoved);
			tleft -= (tsize - tremoved);
			}
		    mdat->EncBuffer[tlen-trem_total] = 0;
		    tsize = libmime_DecodeBase64(mdat->Buffer, mdat->EncBuffer, tlen-trem_total);
		    rem += trem_total; /* rem is the total characters removed (non b64 chars) */
		    }

		/**  Now lets figure out the number of characters that we want out of
		 **  this chunk.  It could be a few characters on the left side of the
		 **  buffer, on the right side of the buffer, or the whole buffer.  We
		 **  gotta check.
		 **/
		bptr = mdat->Buffer + (mdat->InternalSeek - mdat->InternalChunkSeek);
		len = MIME_BUF_SIZE - (bptr - mdat->Buffer);
		if (len > bytes_left) len = bytes_left;
		if (len >= MIME_BUF_SIZE || len >= maxcnt)
		    {
		    if ((tsize-(bptr-mdat->Buffer)) < MIME_BUF_SIZE)
			{
			mdat->InternalChunkSize = (tsize-(bptr-mdat->Buffer));
			end = 1;
			}
		    else
			{
			mdat->InternalChunkSize = MIME_BUF_SIZE;
			}
		    }
		else
		    {
		    mdat->InternalChunkSize = len;
		    }

		/**  Now copy the chunk of bytes into the buffer  **/
		memcpy(ptr, bptr, mdat->InternalChunkSize);
		/**  Update counters and pointers **/
		ptr += mdat->InternalChunkSize;
		bytes_left -= mdat->InternalChunkSize;
		mdat->InternalSeek += mdat->InternalChunkSize;
		size += mdat->InternalChunkSize;
		}
	    break;
	}

    return size;
    }
