#include <fcntl.h>
#include "headers.h"
#include "patchlevel.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "om_mime.h"
#include "mfs.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module:	mime.h,mime.c                                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	November 1996                                           */
/*									*/
/* Description:	This module parses MIME-encoded messages into their	*/
/*		subparts.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mime.c,v 1.1 2001/08/13 18:01:17 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/Attic/mime.c,v $

    $Log: mime.c,v $
    Revision 1.1  2001/08/13 18:01:17  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:18  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** For internal testing **/
#ifndef VERSION
#define VERSION "0.1"
#endif
#ifndef PATCHLEVEL
#define PATCHLEVEL "00"
#endif

#define CRLF_SIZE 1

/** External declarations **/
/*extern void SetupWindow();
extern int EncodingMethod;*/

char *TypeStrings[7] =
    {
    "Text",
    "Multipart",
    "Application",
    "Message",
    "Image",
    "Audio",
    "Video",
    };

#if 00
/** information structure for MIME msg **/
typedef struct _MM
    {
    int		ContentMainType;
    char	ContentSubType[80];
    char	CharSet[32];
    char	PartName[80];
    char	PartSubject[80];
    char	PartBoundary[80];
    char	PartDescription[80];
    char	TransferEncoding[32];
    char	PartContentID[80];
    char	MIMEversion[16];
    char	RefFile[256];
    int		MsgSeekStart;
    int		HdrSeekEnd;
    int		MsgSeekEnd;
    int		NumSubparts;
    struct _MM*	SubParts[16];
    }
    MimeMsg, *pMimeMsg;
#endif

/** Important external interfaces... **/
pMimeMsg MessageInformation = NULL;


int
StrFirstCaseCmp(s1,s2)
    char* s1;
    char* s2;
    {

	while(*s2)
	    {
	    if ((*s1 & 0xDF) >= 'A' && (*s1 & 0xDF) <= 'Z' &&
		(*s2 & 0xDF) >= 'A' && (*s2 & 0xDF) <= 'Z')
		{
		if ((*s1 & 0xDF) != (*s2 & 0xDF)) 
		    return ((*s1 & 0xDF) > (*s2 & 0xDF))*2-1;
		}
	    else
		{
		if (*s1 != *s2) return (*s1 > *s2)*2-1;
		}
	    s1++;
	    s2++;
	    }

    return 0;
    }


char* 
CreateBoundary(level)
    int level;
    {
    static char boundary[80];

	/** Create the boundary from version, PL, seconds, and pid **/
	sprintf(boundary,"OMELM-MULTIPART-BOUNDARY-V%s-PL%s-%8.8X-%05.5d-%d",
	    VERSION,
	    PATCHLEVEL,
	    time(NULL),
	    getpid(),
	    level);

    return boundary;
    }


void
LoadExtendedHdr(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    int curpos;
    int len;
    char abuf[2048];
    int i;

	/** Check the string length **/
	len = strlen(sbuf);

	/** Load the remainder into the sbuf **/
	while(!feof(fd) && !ferror(fd))
	    {
	    curpos = ftell(fd);
	    if (!fgets(abuf,2046,fd)) break;
	    abuf[2047] = 0;
	    if (!strchr(" \t",abuf[0])) break;
	    if (2046-len > strlen(abuf)) 
		{
		len += strlen(abuf);
		strcat(sbuf,abuf);
		}
	    sbuf[2047] = 0;
	    }

	/** Set all tabs, NL's, CR's to spaces **/
	for(i=0;i<len;i++) if (strchr("\t\r\n",sbuf[i])) sbuf[i]=' ';
	
	/** Rewind the file to the last unparsed line **/
	fseek(fd,curpos,SEEK_SET);

    return;
    }


char*
unquote(str)
    char* str;
    {

    	/** Strip trailing spaces... **/
	while(strlen(str) && str[strlen(str)-1] == ' ')
	    {
	    str[strlen(str)-1] = 0;
	    }

	/** If quoted, drop the quotes! **/
	if (*str == '"' && str[strlen(str)-1] == '"')
	    {
	    str[strlen(str)-1] = 0;
	    return str+1;
	    }
	if (*str == '\'' && str[strlen(str)-1] == '\'')
	    {
	    str[strlen(str)-1] = 0;
	    return str+1;
	    }

    return str;
    }


void
SetContentType(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;
    char maintype[32];
    char tmpname[128];
    int len,i;

	/** Make sure we have the ENTIRE content-type line. **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Get the content-type main type **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,"; "))) return;
	if (cptr=strchr(ptr,'/'))
	    {
	    len = (int)cptr - (int)ptr;
	    if (len>31) len=31;
	    strncpy(maintype,ptr,len);
	    maintype[len] = 0;
	    strncpy(msgdata->ContentSubType,cptr+1,79);
	    msgdata->ContentSubType[79] = 0;
	    }
	else
	    {
	    strncpy(maintype,ptr,31);
	    maintype[31] = 0;
	    }
	for(i=0;i<7;i++)
	    {
	    if (!StrFirstCaseCmp(maintype,TypeStrings[i]))
		{
		msgdata->ContentMainType = i+1;
		}
	    }

	/** Look at any possible parameters. **/
	while(ptr = strtok(NULL,"= "))
	    {
	    if (!(cptr=strtok(NULL,";"))) break;
	    while(*ptr == ' ') ptr++;
	    if (!StrFirstCaseCmp(ptr,"boundary"))
		{
		strncpy(msgdata->PartBoundary,unquote(cptr),79);
		msgdata->PartBoundary[79] = 0;
		}
	    else if (!StrFirstCaseCmp(ptr,"name"))
		{
		strncpy(tmpname,unquote(cptr),127);
		tmpname[127]=0;
		if (strchr(tmpname,'/')) strncpy(msgdata->PartName,strrchr(tmpname,'/')+1,79);
		else strncpy(msgdata->PartName,tmpname,79);
		msgdata->PartName[79] = 0;
		if (strchr(msgdata->PartName,'\\')) strncpy(msgdata->PartName,strrchr(tmpname,'\\')+1,79);
		msgdata->PartName[79] = 0;
		}
	    else if (!StrFirstCaseCmp(ptr,"subject"))
		{
		strncpy(msgdata->PartSubject,unquote(cptr),79);
		msgdata->PartSubject[79] = 0;
		}
	    else if (!StrFirstCaseCmp(ptr,"charset"))
		{
		strncpy(msgdata->CharSet,unquote(cptr),31);
		msgdata->CharSet[31] = 0;
		}
	    }

    return;
    }


void
SetContentDisp(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;

	/** Make sure we have the ENTIRE content-disp line. **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Get the disp main type **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,"; "))) return;

	strncpy(msgdata->PartDispType,ptr,79);
	msgdata->PartDispType[79]=0;

	/** Check for the "filename=" content-disp token **/
	while(ptr = strtok(NULL,"= "))
	    {
	    if (!(cptr=strtok(NULL,";"))) break;
	    while(*ptr == ' ') ptr++;
	    if (!StrFirstCaseCmp(ptr,"filename"))
		{
		strncpy(msgdata->PartDispFilename,unquote(cptr),79);
		msgdata->PartDispFilename[79] = 0;
		}
	    }

    return;
    }


void
SetTfrEncoding(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;

	/** Get any extension lines to the header **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Examine to find the tfr-encoding **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,"; "))) return;
	strncpy(msgdata->TransferEncoding,ptr,31);
	msgdata->TransferEncoding[31]=0;

    return;
    }


void
SetMIMEversion(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;

	/** Get any extension lines to the header **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Set the MIME version... **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,"; ("))) return;
	strncpy(msgdata->MIMEversion,ptr,15);
	msgdata->MIMEversion[15]=0;

    return;
    }


void
SetContentID(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;

	/** Get any extension lines to the header **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Set the content-id... **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,"; ("))) return;
	strncpy(msgdata->PartContentID,ptr,79);
	msgdata->PartContentID[79]=0;

    return;
    }


void
SetContentDesc(msgdata,sbuf,fd)
    pMimeMsg msgdata;
    char* sbuf;
    FILE* fd;
    {
    char* ptr;
    char* cptr;

	/** Get any extension lines to the header **/
	LoadExtendedHdr(msgdata,sbuf,fd);

	/** Set the content-description... **/
	if (!(ptr=strtok(sbuf,": "))) return;
	if (!(ptr=strtok(NULL,";"))) return;
	strncpy(msgdata->PartDescription,unquote(ptr),79);
	msgdata->PartDescription[79]=0;

    return;
    }


void
r_ExamineMsg(fd, msgdata, start, end, nohdr)
    FILE* fd;
    pMimeMsg msgdata;
    int start;
    int end;
    int nohdr;
    {
    char sbuf[2048];
    int len;
    int prevpos;
    int i;

	/** Seek to the start of the part/subpart **/
	fseek(fd,start,SEEK_SET);
	msgdata->MsgSeekStart = start;
	msgdata->MsgSeekEnd = end;

	/** Scan the headers until we get a blank line or ftell==end **/
	while(ftell(fd) < end && !nohdr)
	    {
	    /** Get a line from the file... **/
	    if (ferror(fd) || feof(fd) || fgets(sbuf,2046,fd) == NULL) break;
	    sbuf[2047] = 0;

	    /** Examine the line... **/
	    if (strlen(sbuf) < 3) break;
	    if (!StrFirstCaseCmp(sbuf,"Content-Disposition"))
	        {
	        SetContentDisp(msgdata,sbuf,fd);
	        }
	    else if (!StrFirstCaseCmp(sbuf,"Content-Type"))
		{
		SetContentType(msgdata,sbuf,fd);
		}
	    else if (!StrFirstCaseCmp(sbuf,"Content-Transfer-Encoding"))
		{
		SetTfrEncoding(msgdata,sbuf,fd);
		}
	    else if (!StrFirstCaseCmp(sbuf,"MIME-Version"))
		{
		SetMIMEversion(msgdata,sbuf,fd);
		}
	    else if (!StrFirstCaseCmp(sbuf,"Content-id") ||
	        !StrFirstCaseCmp(sbuf,"Message-id"))
		{
		SetContentID(msgdata,sbuf,fd);
		}
	    else if (!StrFirstCaseCmp(sbuf,"Content-Description"))
		{
		SetContentDesc(msgdata,sbuf,fd);
		}
	    }

        /** Default to TEXT if no content type or no header **/
        if (msgdata->ContentMainType == 0) 
            {
            msgdata->ContentMainType = TEXT;
            strcpy(msgdata->ContentSubType,"Plain");
            }
	
	msgdata->HdrSeekEnd = ftell(fd);

	/** If multipart, now we need to locate the parts! **/
	if (msgdata->ContentMainType == MULTIPART && msgdata->PartBoundary[0])
	    {
	    msgdata->NumSubparts = 0;
	    len = strlen(msgdata->PartBoundary);

	    /** Look for the 'part boundary' starting the line... **/
	    while ((prevpos=ftell(fd)) < end)
		{
	        /** Get a line from the file... **/
	        if (ferror(fd) || feof(fd) || fgets(sbuf,2046,fd)==NULL) break;
	        sbuf[2047] = 0;
		
		/** Check for the boundary **/
		if (sbuf[0] == '-' && sbuf[1] == '-' &&
		    !strncmp(sbuf+2,msgdata->PartBoundary,len))
		    {
		    /** If previous part, set its END location. **/
		    if (msgdata->NumSubparts)
		        {
			msgdata->SubParts[msgdata->NumSubparts-1]->MsgSeekEnd =
			    prevpos-CRLF_SIZE;
			}

		    /** If not terminating boundary, init a new subpart **/
		    if (!(sbuf[len+2] == '-' && sbuf[len+3] == '-'))
			{
			msgdata->SubParts[msgdata->NumSubparts] = 
			    (pMimeMsg)nmSysMalloc(sizeof(MimeMsg));
			memset(msgdata->SubParts[msgdata->NumSubparts],0,
			    sizeof(MimeMsg));
			msgdata->SubParts[msgdata->NumSubparts]->MsgSeekStart =
			    ftell(fd);
			msgdata->NumSubparts++;
			}
		    }
		}

	    /** Now, last of all, fill in the structures for the subparts **/
	    for(i=0;i<msgdata->NumSubparts;i++)
	        {
		r_ExamineMsg(fd,msgdata->SubParts[i],
		    msgdata->SubParts[i]->MsgSeekStart,
		    msgdata->SubParts[i]->MsgSeekEnd, 0);
		}
	    }

    return;
    }


pMimeMsg
ExamineMIMEmessage(msgfile, flags)
    char* msgfile;
    int flags;
    {
    pMimeMsg msgdata;
    FILE* msgfd;
    int size;

	/** Open the mail message to be examined **/
	msgfd = fopen(msgfile,"r");
	if (!msgfd)
	    {
	    return NULL;
	    }

	/** Create the msg data structure **/
	msgdata = (pMimeMsg)nmSysMalloc(sizeof(MimeMsg));
	if (!msgdata)
	    {
	    fclose(msgfd);
	    return NULL;
	    }
	memset(msgdata,0,sizeof(MimeMsg));

	/** Get the file size. **/
	fseek(msgfd,0,SEEK_END);
	size = ftell(msgfd);
	fseek(msgfd,0,SEEK_SET);

	/** Do a recursive examination of the msg **/
	r_ExamineMsg(msgfd,msgdata,0,size, (flags?1:0) );

    return msgdata;
    }


int
ExtractMIMEattachment(msgfile,msgdata,destfile)
    char* msgfile;
    pMimeMsg msgdata;
    char* destfile;
    {
    FILE* pipefd;
    int ispipe = 1;
    char cmd[300];
    char sbuf[2048];
    FILE* msgfd;
    int i;
    int ck_uudecode=0;
    char* ptr;

    	/** Open the source **/
	msgfd = fopen(msgfile,"r");

	/** Save the filename. **/
    	strncpy(msgdata->RefFile,destfile,255);
	msgdata->RefFile[255] = 0;

	/** Determine the encoding mechanism **/
	if (!StrFirstCaseCmp(msgdata->TransferEncoding,"base64"))
	    {
	    sprintf(cmd,"mmencode -u -b -o %s",msgdata->RefFile);
	    pipefd = popen(cmd,"w");
	    }
	else if (!StrFirstCaseCmp(msgdata->TransferEncoding,"quoted"))
	    {
	    sprintf(cmd,"mmencode -u -q -o %s",msgdata->RefFile);
	    pipefd = popen(cmd,"w");
	    }
	else if (!StrFirstCaseCmp(msgdata->ContentSubType,"x-uue") ||
		 !StrFirstCaseCmp(msgdata->ContentSubType,"uue"))
	    {
#ifndef _LINUX_
	    sprintf(cmd,"uudecode -s >%s",msgdata->RefFile);
#else
	    sprintf(cmd,"uudecode");
	    ck_uudecode = 1;
#endif
	    pipefd = popen(cmd,"w");
	    }
	else
	    {
	    pipefd = fopen(destfile,"w+");
	    ispipe = 0;
	    }

	/** Write this subpart out to the file **/
	fseek(msgfd,msgdata->HdrSeekEnd,0);
	while(ftell(msgfd) < msgdata->MsgSeekEnd && !feof(msgfd) && 
	      !ferror(msgfd))
	    {
	    i = msgdata->MsgSeekEnd - ftell(msgfd);
	    if (i == 0) break;
	    if (i>511) i = 511;
	    if (!fgets(sbuf,i+1,msgfd)) break;
	    sbuf[i] = 0;
	    if (ck_uudecode && !strncmp(sbuf,"begin ",6) && atoi(sbuf+6) > 0)
		{
		ck_uudecode = 0;
		ptr = strchr(sbuf+6,' ');
		if (ptr)
		    {
		    strcpy(ptr+1,msgdata->RefFile);
		    }
		else
		    {
		    sprintf(sbuf,"begin 0600 %s\n",msgdata->RefFile);
		    }
		}
	    fputs(sbuf,pipefd);
	    }
	
	/** Close the input and output pipe/file **/
	if (ispipe) 
	    {
	    fputs("\n",pipefd);
	    pclose(pipefd);
	    }
	else 
	    {
	    fclose(pipefd);
	    }
	fclose(msgfd);

    return 0;
    }
    


void
FreeMsgStructure(msgdata)
    pMimeMsg msgdata;
    {
    int i;

    	/** Go through the subparts and free each part. **/
	for(i=0;i<msgdata->NumSubparts;i++) 
	    {
	    FreeMsgStructure(msgdata->SubParts[i]);
	    }

	/** Free this structure itself. **/
	nmSysFree(msgdata);

    return;
    }


void
AddContentTypeHdr(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {

    	/** If not defined, don't add content-type. **/
	if (msgdata->ContentMainType == 0) return;

	/** First line. **/
    	fprintf(outfd,"Content-Type: %s%s%s",
	    TypeStrings[msgdata->ContentMainType-1],
	    msgdata->ContentSubType[0]?"/":"",
	    msgdata->ContentSubType);
	
	/** Any parameters? **/
	if (msgdata->CharSet[0])
	    {
	    fprintf(outfd,";\n\tcharset=\"%s\"",msgdata->CharSet);
	    }
	if (msgdata->PartBoundary[0])
	    {
	    fprintf(outfd,";\n\tboundary=\"%s\"",msgdata->PartBoundary);
	    }
	if (msgdata->PartSubject[0])
	    {
	    fprintf(outfd,";\n\tsubject=\"%s\"",msgdata->PartSubject);
	    }
	if (msgdata->PartName[0])
	    {
	    fprintf(outfd,";\n\tname=\"%s\"",msgdata->PartName);
	    }

	/** Closing NL **/
	fprintf(outfd,"\n");

    return;
    }


void
AddTfrEncodingHdr(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {

    	/** If no transfer-encoding, don't put a hdr **/
	if (msgdata->TransferEncoding[0] == '\0') return;

	/** Put the tfr encoding line **/
	fprintf(outfd,"Content-Transfer-Encoding: %s\n",
	    msgdata->TransferEncoding);

    return;
    }


void
AddContentIDHdr(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {

    	/** If no content-id, don't put a hdr **/
	if (msgdata->PartContentID[0] == '\0') return;

	/** Print the line into the file... **/
	if (msgdata->PartContentID[0] == '<')
	    {
	    fprintf(outfd,"Content-ID: %s\n",msgdata->PartContentID);
	    }
	else
	    {
	    fprintf(outfd,"Content-ID: <%s>\n",msgdata->PartContentID);
	    }

    return;
    }


void
AddContentDescHdr(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {

    	/** If no content-desc, skip this hdr **/
	if (msgdata->PartDescription[0] == 0) return;

	/** Print the hdr **/
	fprintf(outfd,"Content-Description: \"%s\"\n",msgdata->PartDescription);

    return;
    }


void
AddPreludeInformation(msgdata,outfd,level)
    pMimeMsg msgdata;
    FILE* outfd;
    int level;
    {

    	/** Don't add a prelude if in a nested multipart. **/
	if (level) return;

	/** Place the text in the msg data file. **/
    	fprintf(outfd,"This message has been MIME-encoded by the OMelm v%s PL%s\n",VERSION,PATCHLEVEL);
	fprintf(outfd,"mail system.  It can be read normally by OMelm v3.1 PL00\n");
	fprintf(outfd,"and above as well as by any other MIME-compliant mail\n");
	fprintf(outfd,"program.\n\n");

    return;
    }


void
AddEpilogueInformation(msgdata,outfd,level)
    pMimeMsg msgdata;
    FILE* outfd;
    int level;
    {

    	/** Don't add an epilogue if in a nested multipart **/
	if (level) return;

	/** Place the text in the msg data file. **/
	fprintf(outfd,"\n");

    return;
    }


void
AddBinaryEncodedFile(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {
    FILE* pipefd;
    char sbuf[80];
    char cmd[80];

    	/** Create the cmd to execute; default encoding is base64. (-b) **/
	sprintf(cmd,"mmencode -%c <%56.56s",
	    (!StrFirstCaseCmp(msgdata->TransferEncoding,"quoted"))?'q':'b',
	    msgdata->RefFile);

	/** Run the pipe, reading mmencode's output. **/
	pipefd = popen(cmd,"r");
	if (!pipefd) return;
	while(!feof(pipefd) && !ferror(pipefd))
	    {
	    if (!fgets(sbuf,78,pipefd)) break;
	    sbuf[79] = 0;
	    fputs(sbuf,outfd);
	    }
	pclose(pipefd);

    return;
    }


void
AddTextualFile(msgdata,outfd)
    pMimeMsg msgdata;
    FILE* outfd;
    {
    FILE* curfd;
    char sbuf[2048];
    char cmd[80];
    int isbinary=0;
    int ch;


	/** Set the encoding mechanism if not specified. **/
	curfd = fopen(msgdata->RefFile,"r");
	if (!curfd) return;
	if (msgdata->TransferEncoding[0] == '\0')
	    {
	    strcpy(msgdata->TransferEncoding,"7Bit");
	    }

	/** Now encode according to the proper mechanism. **/
	if (!StrFirstCaseCmp(msgdata->TransferEncoding,"quoted"))
	    {
	    fclose(curfd);
	    AddBinaryEncodedFile(msgdata,outfd);
	    return;
	    }
	else if (!StrFirstCaseCmp(msgdata->TransferEncoding,"base64"))
	    {
	    fclose(curfd);
	    AddBinaryEncodedFile(msgdata,outfd);
	    return;
	    }
	else
	    {
	    while(!feof(curfd) && !ferror(curfd))
	    	{
	    	if (!fgets(sbuf,2046,curfd)) break;
	    	sbuf[2047] = 0;
	    	fputs(sbuf,outfd);
	    	}
	    fclose(curfd);
	    }

    return;
    }


void
r_CreateMsg(msgdata,outfd,level)
    pMimeMsg msgdata;
    FILE* outfd;
    int level;
    {
    FILE* curfd;
    int i,isbinary,ch;

    	/** Just in case... **/
	if (!msgdata) return;

    	/** Set the output start point. **/
	msgdata->MsgSeekStart = ftell(outfd);

	/** Determine NOW if the attached ref file is 7bit clean **/
	if (!msgdata->TransferEncoding[0] || 
	    !StrFirstCaseCmp(msgdata->TransferEncoding,"7bit"))
	    {
	    if (msgdata->ContentMainType != MULTIPART)
	        {
		isbinary = 0;
		curfd = fopen(msgdata->RefFile,"r");
		if (curfd)
		    {
		    while((ch=getc(curfd)) != EOF)
	    	        {
	    	        isbinary |= (ch & 0x80);
	    	        }
		    fclose(curfd);
		    }
		if (isbinary) 
		    {
		    strcpy(msgdata->TransferEncoding,"Quoted-Printable");
		    }
		}
	    }

    	/** If this is a nested-level, we need to add headers. **/
	if (level > 0 || !(msgdata->Flags & MFS_F_NOHEADER))
	    {
	    AddContentTypeHdr(msgdata,outfd);
	    AddTfrEncodingHdr(msgdata,outfd);
	    AddContentIDHdr(msgdata,outfd);
	    AddContentDescHdr(msgdata,outfd);
	    fprintf(outfd,"\n");
	    }

	msgdata->HdrSeekEnd = ftell(outfd);

	/** Ok, now for the main msg part.  If multipart, do the nesting **/
	if (msgdata->ContentMainType == MULTIPART && msgdata->NumSubparts)
	    {
	    AddPreludeInformation(msgdata,outfd,level);
	    for(i=0;i<msgdata->NumSubparts;i++)
	        {
		fprintf(outfd,"--%s\n",msgdata->PartBoundary);
		r_CreateMsg(msgdata->SubParts[i],outfd,level+1);
		fprintf(outfd,"\n");
		}
	    fprintf(outfd,"--%s--\n\n",msgdata->PartBoundary);
	    AddEpilogueInformation(msgdata,outfd,level);
	    }

	/** Otherwise, just place the data in the msg. **/
	if (msgdata->ContentMainType != MULTIPART)
	    {
	    switch(msgdata->ContentMainType)
	        {
		case TEXT:
		case MESSAGE:
		    AddTextualFile(msgdata,outfd);
		    break;

		case IMAGE:
		case AUDIO:
		case VIDEO:
		case APPLICATION:
		    if ((!StrFirstCaseCmp(msgdata->TransferEncoding,"7bit")) ||
		        (!StrFirstCaseCmp(msgdata->TransferEncoding,"8bit")))
		        {
			AddTextualFile(msgdata,outfd);
			}
		    else
		        {
			AddBinaryEncodedFile(msgdata,outfd);
			}
		    break;

		default:
		    break;
		}
	    }

	/** Set the seek end point. **/
	msgdata->MsgSeekEnd = ftell(outfd);

    return;
    }


void
CreateMimeMsg(msgdata,outfile)
    pMimeMsg msgdata;
    char* outfile;
    {
    FILE* outfd;

	/** Open the msg output (destination) file... **/
	outfd = fopen(outfile,"w+");
	if (!outfd) return;

	/** Call the recursive routine to create the message. **/
    	r_CreateMsg(msgdata,outfd,0);

	/** Close the output/destination file **/
	fclose(outfd);

    return;
    }


int
r_ReplaceUpdate(msgdata,partptr,diff,chg)
    pMimeMsg msgdata;
    pMimeMsg partptr;
    int diff;
    int chg;
    {
    int i;

    	/** Update start position and hdr start position **/
	if (chg)
	    {
	    msgdata->MsgSeekStart += diff;
	    msgdata->HdrSeekEnd += diff;
	    }

	/** If this is the new component, set chg **/
	if (msgdata == partptr) chg = 1;

	/** If multipart, go through each part accordingly. **/
	for(i=0;i<msgdata->NumSubparts;i++)
	    {
	    chg = r_ReplaceUpdate(msgdata->SubParts[i],partptr,diff,chg);
	    }

	/** Update the msg end position **/
	if (chg)
	    {
	    msgdata->MsgSeekEnd += diff;
	    }

    return chg;
    }


void
ReplaceSubpart(msgfile,maindata,partdata,newfile)
    char* msgfile;
    pMimeMsg maindata;
    pMimeMsg partdata;
    char* newfile;
    {
    int size,oldpos,newsize,i;
    int fd;
    char tmpname[256];
    char ch;
    FILE* msgfd;
    FILE* tmpfd;
    FILE* newfd;
    char sbuf[256];

    	/** Set the subpart ref file name **/
	strncpy(partdata->RefFile,newfile,255);
	partdata->RefFile[255]=0;

	/** Determine current size of the attachment. **/
	size = partdata->MsgSeekEnd - partdata->HdrSeekEnd;

	/** Open the tmp file **/
	ch = 'a';
	do  {
	    sprintf(tmpname,"%s.t0%c",msgfile,ch);
	    ch++;
	    }
	    while ((fd = open(tmpname,O_RDONLY)) >= 0 && close(fd) >= 0);
	
	/** Copy to the tmp file, doing the replace. **/
	msgfd = fopen(msgfile,"r");
	tmpfd = fopen(tmpname,"w+");
	newfd = fopen(newfile,"r");

	/** Copy the first part of the msg to the tmpfile **/
	while(!ferror(msgfd) && !feof(msgfd) && 
	      ftell(msgfd) < partdata->HdrSeekEnd)
	    {
	    i = partdata->HdrSeekEnd - ftell(msgfd);
	    if (i>255) i = 255;
	    if (fgets(sbuf,i+1,msgfd) == NULL) break;
	    sbuf[i] = 0;
	    fputs(sbuf,tmpfd);
	    }

	/** Skip over the replaced section **/
	fseek(msgfd,partdata->MsgSeekEnd,0);

	/** Add the part for the replace. **/
	oldpos = ftell(tmpfd);
	/*while(!ferror(newfd) && !feof(newfd))
	    {
	    if (fgets(sbuf,255,newfd) == NULL) break;
	    sbuf[255] = 0;
	    fputs(sbuf,tmpfd);
	    }*/
	AddTextualFile(partdata,tmpfd);
	newsize = ftell(tmpfd) - oldpos;

	/** Add the remaining part of the original file **/
	while(!ferror(msgfd) && !feof(msgfd))
	    {
	    if (fgets(sbuf,255,msgfd) == NULL) break;
	    sbuf[255] = 0;
	    fputs(sbuf,tmpfd);
	    }

	/** Update the seek counters in the msgdata structure **/
	printf("Updating size from %d to %d.\n",size,newsize);
	r_ReplaceUpdate(maindata,partdata,newsize-size,0);

	/** Unlink original file, move tmp to original **/
	fclose(newfd);
	fclose(tmpfd);
	fclose(msgfd);
	unlink(msgfile);
	link(tmpname,msgfile);
	unlink(tmpname);

    return;
    }


/** This routine right now only understands one charset, and converts it to
    it's "closest neighbor" in the 0-127 us-ascii charset.  The only charset
    understood now is iso-1, since it's the one that will usually turn up.
    **/
int
ParseEncodedHdr(unsigned char* src, unsigned char* dst, int maxdst)
    {
    int ew_state = 0;
    char charset[32];
    char encoding = 'q';
    char* ptr;
    int cnt,ix;
    char x[3];
    char b64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char iso8859_1[128] = "  ,f,.**^%S<     ''\"\".-_~ts>o  Y !c#oY|s\"ca<--r-o+23'up.,10>qht?AAAAAAACEEEEIIIIDNOOOOOx0UUUUYPBaaaaaaaceeeeiiiidnooooo/0uuuuypy";

	maxdst--;

	/** Step through the string, looking for =? **/
	while(*src && maxdst > 0)
	    {
	    switch(ew_state)
		{
		case 0:  /* not in any encoded word */
		    if (src[0] == '=' && src[1] == '?')
			{
			ew_state = 1;
			src+=2;
			}
		    else
			{
		        *(dst++) = *(src++);
			maxdst --;
			}
		    break;

		case 1:  /* in encoding, charset */
		    ptr = strchr((char*)src,'?');
		    if (!ptr) return -1;
		    cnt = (int)ptr - (int)src;
		    if (cnt > 31) cnt = 31;
		    strncpy(charset,(char*)src,cnt);
		    charset[cnt] = 0;
		    src = ptr + 1;
		    ew_state = 2;
		    break;

		case 2:  /* in encoding, tfr-encoding */
		    ptr = strchr((char*)src,'?');
		    if (!ptr) return -1;
		    encoding = *src;
		    if (encoding == 'B' || encoding == 'Q') encoding += 32;
		    if (encoding != 'b' && encoding != 'q') return -1;
		    src = ptr + 1;
		    ew_state = 3;
		    break;

		case 3:  /* in encoding, content */
		    if (src[0] == '?' && src[1] == '=')
			{
			ew_state = 0;
			src += 2;
			}
		    else
			{
			if (encoding == 'q')
			    {
			    if (*src == '=')
				{
				if (src[1] == 0 || src[2] == 0) return -1;
				x[0] = src[1];
				x[1] = src[2];
				x[2] = 0;
				*(dst++) = strtol(x,NULL,16);
				if (dst[-1] >= 128) dst[-1] = iso8859_1[dst[-1]-128];
			        if (dst[-1] < 32) dst[-1] = '.';
				maxdst --;
				src += 3;
				}
			    else if (*src == '_')
				{
				*(dst++) = ' ';
				maxdst --;
				src++;
				}
			    else
				{
				if (*src >= 128)
				    {
				    *(dst++) = iso8859_1[*(src++) - 128];
				    }
				else
				    {
				    *(dst++) = *(src++);
				    }
			        if (dst[-1] < 32) dst[-1] = '.';
				maxdst --;
				}
			    }
			else /* 'b' */
			    {
			    if (maxdst < 3) return -1;
			    ptr = strchr(b64,src[0]);
			    if (!ptr) return -1;
			    ix = (int)ptr - (int)b64;
			    dst[0] = ix<<2;
			    ptr = strchr(b64,src[1]);
			    if (!ptr) return -1;
			    ix = (int)ptr - (int)b64;
			    dst[0] |= ix>>4;
			    if (dst[0] >= 128) dst[0] = iso8859_1[dst[0]-128];
			    if (dst[0] < 32) dst[0] = '.';
			    dst[1] = (ix<<4)&0xF0;
			    if (src[2] == '=' && src[3] == '=')
				{
				maxdst -= 1;
				dst += 1;
				src += 4;
				break;
				}
			    ptr = strchr(b64,src[2]);
			    if (!ptr) return -1;
			    ix = (int)ptr - (int)b64;
			    dst[1] |= ix>>2;
			    if (dst[1] >= 128) dst[1] = iso8859_1[dst[1]-128];
			    if (dst[1] < 32) dst[1] = '.';
			    dst[2] = (ix<<6)&0xC0;
			    if (src[3] == '=')
				{
				maxdst -= 2;
				dst += 2;
				src += 4;
				break;
				}
			    ptr = strchr(b64,src[3]);
			    if (!ptr) return -1;
			    ix = (int)ptr - (int)b64;
			    dst[2] |= ix;
			    if (dst[2] >= 128) dst[2] = iso8859_1[dst[2]-128];
			    if (dst[2] < 32) dst[2] = '.';
			    maxdst -= 3;
			    src += 4;
			    }
			}
		    break;
		}
	    }
	*dst = 0;

    return 0;
    }

