#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "mtask.h"
#include "report.h"
#include "barcode.h"
#include "prtmgmt.h"
#include "newmalloc.h"

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
/* Module: 	text_prt.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 21, 1999						*/
/* Description: Print driver for plain-text output, without any format	*/
/*		codes of any sort.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: text_prt.c,v 1.1 2001/08/13 18:01:16 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/text_prt.c,v $

    $Log: text_prt.c,v $
    Revision 1.1  2001/08/13 18:01:16  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:17  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

typedef struct 
    {
    int			CurAttr;
    int			Flags;
    int			Lines;
    int			Cols;
    double		CurPageCol;
    int			CurLine;
    int			CurTableCol;
    int			ColRoomLeft;
    int			nColumns;
    char		Buffer[256];
    char		BCBuf[96];
    }
    Session, *pSession;


/*** txtOpen - open a new prpPrtSession ps, with the given FD as our 
 *** output stream.  Output can be to file, to pipe, etc.  We can
 *** even output to a pipe that is being read as input by another
 *** thread, because of MTASK's i/o scheduling.
 ***/
int
txtOpen(pPrtSession ps)
    {
    pSession s;

    	/** Allocate a session **/
	s = (pSession)nmMalloc(sizeof(Session));
	if (!s) return -1;

	/** Set up the data structure **/
	ps->PrivateData = (void*)s;
	s->CurAttr = 0;
	s->CurPageCol = 0;
	s->CurLine = 0;

    return 0;
    }


/*** txtClose - close an existing session.  This does NOT close the file
 *** descriptor -- the user must do that after txtClose()'ing.
 ***/
int
txtClose(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Send reset **/
	nmFree(s,sizeof(Session));
	ps->PrivateData = NULL;

    return 0;
    }


/*** txt_internal_Write - does an fdWrite unless it's a centered row, 
 *** in which case the data is buffered.
 ***/
int
txt_internal_Write(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;

	/** If we're in a centered row, cache the buffer **/
	if (s->CurAttr & RS_TX_CENTER)
	    {
	    n = strlen(s->Buffer);
	    memcpy(s->Buffer+n,text,cnt);
	    s->Buffer[n+cnt] = 0;
	    return cnt;
	    }

	/** If in a barcode, generate the graphics **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    /** Go through text, printing short or long bars **/
	    for(n=0;n<cnt;n++)
		{
		if (text[n]&1) ps->WriteFn(ps->WriteArg,"|",1,0,0);
		else ps->WriteFn(ps->WriteArg,".",1,0,0);
		}
	    return cnt;
	    }

    return ps->WriteFn(ps->WriteArg,text,cnt,0,0);
    }


/*** txtGetCharWidth - find the current character width relative
 *** to the 'standard' character width, usually 10cpi.
 ***/
double
txtGetCharWidth(pPrtSession ps, char* text, int a, int f)
    {
    /*pSession s = (pSession)(ps->PrivateData);*/
    return 1.0*(text?strlen(text):1);
    }


/*** txt_internal_BaseWidth - find the base width of the paper output,
 *** usually at 10cpi, likely 80 characters (.25 margin, 8" printable)
 ***/
int
txt_internal_BaseWidth(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int basewidth;

        /** Check base width **/
        basewidth = s->Cols;
        if (basewidth == 0) basewidth=80;

    return basewidth;
    }


/*** txt_internal_GetCols - find the number of column positions in the
 *** page based on page width and current font pitch.
 ***/
int
txt_internal_GetCols(pPrtSession ps)
    {
    /*pSession s = (pSession)(ps->PrivateData);*/
    int n;
    int basewidth;

	basewidth = txt_internal_BaseWidth(ps);

	/** Adjust based on attribs **/
	n = basewidth;

    return n;
    }


/*** txtPageGeom - set the estimated geometry of the printed page.
 ***/
int
txtPageGeom(pPrtSession ps, int lines, int cols)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Set the geometry **/
	s->Lines = lines;
	s->Cols = cols;

    return 0;
    }


/*** txtGetAlign - return the current x position on the page.
 ***/
double
txtGetAlign(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurPageCol;
    }


/*** txtAlign - horizontal tab over to a particular x position on the page.
 ***/
int
txtAlign(pPrtSession ps, double x)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;
    char hposbuf[161];

	if (x >= 160) x = 160;
	n = (x - s->CurPageCol + 0.001);
	if (n<0) n = 0;
	memset(hposbuf,' ', n);
	hposbuf[n] = 0;
        ps->WriteFn(ps->WriteArg, hposbuf, n,0,0);
	s->CurPageCol = (int)(x+0.001);

    return 0;
    }


/*** txtWriteText - write some text onto the output. 
 ***/
int
txtWriteString(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Check count **/
	if (cnt == -1) cnt = strlen(text);

	/** If in a barcode, do the conversion here. **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    if (barEncodeNumeric(BAR_T_POSTAL,text,cnt,s->BCBuf,69) >=0)
		{
		cnt=strlen(s->BCBuf);
		text = s->BCBuf;
		}
	    else
		{
		return -1;
		}
	    }

	/** Keep track of current column cnt **/
	s->CurPageCol += (cnt*txtGetCharWidth(ps,NULL,-1,-1));

    return txt_internal_Write(ps, text, cnt);
    }


/*** txt_internal_FinCenter - finish a centered line.
 ***/
int
txt_internal_FinCenter(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    double a;

	/** Only do centering logic if we printed real text. **/
	if (s->CurPageCol > 0)
	    {
	    a = (txt_internal_BaseWidth(ps)-s->CurPageCol)/2;
	    s->CurPageCol = 0;
	    txtAlign(ps, a);
	    }

	/** Now write the prewritten/buffered data **/
        ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer),0,0);

    return 0;
    }


/*** txtWriteNL - write out a line break (NL/CR). 
 ***/
int
txtWriteNL(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Centering?  Need to print the centered line? **/
	if (s->CurAttr & RS_TX_CENTER)
	    {
	    txt_internal_FinCenter(ps);
	    }

	/** Clear the buffer & col cnt **/
	s->CurPageCol = 0;
	s->Buffer[0] = 0;
	s->CurLine++;
	if (s->CurLine == s->Lines) s->CurLine = 0;

    return (ps->WriteFn(ps->WriteArg, "\n", 2,0,0)>0)?0:-1;
    }


/*** txtWriteLine - write a horizontal line across the page.  To do this,
 *** we just write a series of dashes, either a standard width if the
 *** Cols setting is 0, or a setting relative to Cols and the current text
 *** width if Cols is not 0.
 ***/
int
txtWriteLine(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int f;
    int cnt;
    char sbuf[256];

	f = s->CurAttr;

	/** Not at beginning of line?  Issue a NL if not. **/
	if (s->CurPageCol > 0) txtWriteNL(ps);

	/** Calculate the number of physical columns. **/
	cnt = txt_internal_GetCols(ps);
	cnt--;

	/** Create the print string. **/
	memset(sbuf,'-',cnt);
	sbuf[cnt++] = '\n';
	sbuf[cnt]=0;

	/** Write it. **/
	ps->WriteFn(ps->WriteArg, sbuf, cnt, 0,0);
	s->CurLine++;
	s->CurPageCol = 0;
	if (s->CurLine == s->Lines) s->CurLine = 0;

    return 0;
    }


/*** txtWriteFF - write out a form feed.  
 ***/
int
txtWriteFF(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    char sbuf[256];
    int cnt;

	/** Not at beginning of line?  Issue a NL if not. **/
	if (s->CurPageCol > 0) txtWriteNL(ps);

	/** Calculate the number of physical columns. **/
	cnt = txt_internal_GetCols(ps);
	cnt--;

	/** Create the print string. **/
	memset(sbuf,'=',cnt+1);
	sbuf[0] = '\n';
	cnt++;
	sbuf[cnt++] = '\n';
	sbuf[cnt++] = '\n';
	sbuf[cnt]=0;
	if (cnt > 16) memcpy(sbuf+4,"PAGE BREAK",10);

	/** Write it. **/
	ps->WriteFn(ps->WriteArg, sbuf, cnt, 0,0);

    return 0;
    }


/*** txtGetAttr - obtain the current attribute information for the
 *** printer, as previously set using txtSetAttr.
 ***/
int
txtGetAttr(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurAttr;
    }


/*** txtSetAttr - change the text attributes, including bold, italic,
 *** underline, expanded, compressed, etc.  We don't have to worry about
 *** properly nesting these attributes like we do in HTML.  So, we just
 *** figure out what needs to be turned off and on, and send those codes.
 ***/
int
txtSetAttr(pPrtSession ps, int attr)
    {
    pSession s = (pSession)(ps->PrivateData);
    int oldattr, onattr, offattr;

	/** Find attrs for on and off **/
	oldattr = s->CurAttr;
	onattr = (attr^oldattr)&attr;
	offattr = (attr^oldattr)&oldattr;

	/** Check for barcode turned off first. **/
	if (offattr & RS_TX_PBARCODE) s->CurAttr &= ~RS_TX_PBARCODE;

	/** Check for centering turned off. **/
	if (offattr & RS_TX_CENTER)
	    {
	    txt_internal_FinCenter(ps);
	    s->CurAttr &= ~RS_TX_CENTER;
	    }

	/** Set the new curattr **/
	s->CurAttr = attr;

    return 0;
    }

#if 0
/*** txtDoTable - begin a tabular structure in the output.  We'll need to
 *** keep track of set (or calculated) column widths and column count for
 *** making formatting decisions, since such formatting is not done for
 *** us.
 ***/
int
txtDoTable(pPrtSession ps)
    {
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** Set in table **/
	s->Flags |= FXP_F_INTABLE;
	s->nColumns = 0;
	s->ColTitleLen = 0;

    return 0;
    }


/*** txtEndTable - finish up a table we started with txtDoTable.  This also
 *** finishes up the last column, if need be.
 ***/
int
txtEndTable(pPrtSession ps)
    {
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** Still only in header? If so, trigger the thing. **/
	if (s->Flags & FXP_F_INTABHDR)
	    {
	    txtDoColumn(session, 1, 1);
	    }

	/** Write a newline **/
	txtWriteNL(pPrtSession ps);

	/** Ok, cancel the table flags **/
	s->Flags &= ~(FXP_F_INTABLE|FXP_F_INTABCOL|FXP_F_INTABHDR);

    return 0;
    }


/*** txt_internal_FindWrapPoint - finds the number of characters
 *** that can be included in a line of length n without breaking a
 *** word into two pieces.
 ***/
int
txt_internal_FindWrapPoint(char* txt, int n)
    {
    int i,last_space;

	/** If n is bigger than the text itself, we can use whole thing **/
	if (strlen(txt) <= n) return strlen(txt);

	/** Search for spaces backwards from 'n' or line len **/
	for(i=n;i;i--)
	    {
	    if (txt[i] == ' ') return i;
	    }

    return n;
    }


/*** txt_internal_WriteHeader - take the information already stored
 *** in the txt structure and write it as a header on the output.
 *** this is called when DoColumn ends the txtDoColHdr calls.
 ***/
int
txt_internal_WriteHeader(pPrtSession ps)
    {
    pFile fd;
    pSession s = &(s->;
    int i,n;
    char* ptr;
    int used_all;

	/** Get the file descriptor to print on **/
	fd = s->fd;
	
	/** Build lines of header and write them until no mo' **/
	s->ColHdr[0] = 0;
	s->Buffer[0] = 0;
	used_all=0;
	while(!used_all)
	    {
	    /** Initialize our buffer and keep track of header usage **/
	    used_all=1;
	    ptr = s->ColHdr;

	    /** Nab as much as we can out of each col hdr **/
	    for(i=0;i<s->nColumns;i++)
		{
		n = txt_internal_FindWrapPoint(
		    s->ColTitles+s->ColTitleOffset[i],s->TableColWidth[i]);
		if (n < strlen(s->ColTitles+s->ColTitleOffset[i])) used_all=0;
		sprintf(ptr,"%-*.*s ",s->TableColWidth[i],n,s->ColTitles+s->ColTitleOffset[i]);
		ptr += (s->TableColWidth[i]+1);
		s->ColTitleOffset[i] += n;
		if (s->ColTitles[s->ColTitleOffset[i]] == ' ') s->ColTitleOffset[i]++;
		}

	    /** Write the header line. **/
	    ps->WriteFn(ps->WriteArg, s->ColHdr, strlen(s->ColHdr), 0,0);
	    txtWriteNL(pPrtSession ps);
	    }

	/** Now write the line underscoring the header area **/
	memset(s->ColHdr, '-', strlen(s->ColHdr));
	for(n=0,i=0;i<s->nColumns-1;i++)
	    {
	    n+=s->TableColWidth[i];
	    s->ColHdr[n]=' ';
	    n++;
	    }
	ps->WriteFn(ps->WriteArg, s->ColHdr, strlen(s->ColHdr), 0,0);
	txtWriteNL(pPrtSession ps);

    return 0;
    }


/*** txtDoColHdr - start a column header entry for a table that has
 *** already been started with txtDoTable.  Although not particularly
 *** useful for column headers, this fn is the same as DoColumn in that
 *** it has a second parameter for start new row.
 ***/
int
txtDoColHdr(pPrtSession ps, int start_row, int set_width)
    {
    pSession s = &(s->;
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** If in header, advance the column title information **/
	if (s->Flags & FXP_F_INTABHDR)
	    {
	    s->ColTitles[s->ColTitleLen++]=0;
	    s->ColTitleOffset[s->nColumns] = s->ColTitleLen;
	    s->TableColWidth[s->nColumns] = set_width;
	    s->nColumns++;
	    }
	else
	    {
	    s->ColTitleOffset[0] = 0;
	    s->TableColWidth[0] = set_width;
	    s->nColumns=1;
	    s->ColTitleLen = 0;
	    }

	/** We're in table hdr now **/
	s->Flags |= FXP_F_INTABHDR;

    return 0;
    }


/*** txtDoColumn - start one cell within a row in a table.  The
 *** start-row parameter indicates whether to start a new row (1) or
 *** continue on the current row (0).
 ***/
int
txtDoColumn(pPrtSession ps, int start_row, int col_span)
    {
    pSession s = &(s->;
    pFile fd;

	/** If in header, finish up last item and write the header out **/
	if (s->Flags & FXP_F_INTABHDR)
	    {
	    /** String-terminate the end of the titles. **/
	    s->ColTitles[s->ColTitleLen]=0;

	    /** Did user actually write any info?  If not, skip hdr **/
	    if (s->nColumns != (s->ColTitleLen + 1) && s->nColumns > 0)
		{
		txt_internal_WriteHeader(pPrtSession ps);
		}
	    s->Flags &= ~FXP_F_INTABHDR;
	    }

	/** Set the current column... **/
	if (s->Flags & FXP_F_INTABCOL)
	    {
	    if (start_row)
		{
		txtWriteNL(ps);
	        s->CurTableCol = 0;
		}
	    else
		{
		/** First check for padding on previous col. **/
		s->ColRoomLeft++;
		memset(s->Buffer,' ',s->ColRoomLeft);
		s->Buffer[s->ColRoomLeft] = 0;
		ps->WriteFn(ps->WriteArg, s->Buffer, s->ColRoomLeft, 0,0);

		/** Advance to next col. **/
	        s->CurTableCol++;
		}
	    }
	else
	    {
	    s->CurTableCol = 0;
	    s->Flags |= FXP_F_INTABCOL;
	    }

	/** Find the available space for this column **/
	s->ColRoomLeft = 0;
	while(col_span-- && s->CurTableCol < s->nColumns)
	    {
	    s->ColRoomLeft += (s->TableColWidth[s->CurTableCol++] + 1);
	    }

	/** Correction since padding is only between columns **/
	s->ColRoomLeft--;
	s->CurTableCol--;

    return 0;
    }
#endif 


/*** txtComment - write a comment into the output stream so the output stream
 *** can be re-parsed as a prtmgmt command stream.  This is a NOP for FXP.
 ***/
int
txtComment(pPrtSession ps, char* txt)
    {
    return 0;
    }


/*** txtSetFont - does nothing for plaintext output...
 ***/
int
txtSetFont(pPrtSession ps, int font_id)
    {
    return 0;
    }

/*** txtSetLPI - how can we make the lines closer together on the
 *** screen?  This is silly.
 ***/
int
txtSetLPI(pPrtSession ps, int lines_per_inch)
    {
    return 0;
    }


/*** txtInitialize -- initialize this module, which causes it to register
 *** itself with the report server.
 ***/
int
txtInitialize()
    {
    pPrintDriver drv;

	/** Zero our globals **/
	/*memset(&FXP,0,sizeof(FXP));*/

	/** Allocate the descriptor structure **/
	drv = (pPrintDriver)nmMalloc(sizeof(PrintDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(PrintDriver));

	/** Fill out the structure **/
	strcpy(drv->Name, "TXT - ASCII Textonly Print Driver");
	strcpy(drv->ContentType, "text/plain");
	drv->Open = txtOpen;		/* Open a session */
	drv->Close = txtClose;		/* Close a session */
	drv->PageGeom = txtPageGeom;	/* Set page geometry */
	/*drv->WriteText = txtWriteText;*/	/* Write a string of text */
	drv->WriteNL = txtWriteNL;	/* Output a new line (CR) */
	drv->WriteFF = txtWriteFF;	/* Output a "form feed" */
	drv->WriteLine = txtWriteLine;	/* Output a horizontal line */
	drv->SetAttr = txtSetAttr;	/* Set attributes */
	drv->GetAttr = txtGetAttr;	/* Get attributes */
	/*drv->DoTable = txtDoTable;*/	/* Start a table */
	/*drv->DoColHdr = txtDoColHdr;*/	/* Start a column header */
	/*drv->DoColumn = txtDoColumn;*/	/* Start a column */
	/*drv->EndTable = txtEndTable;*/	/* Finish up the table */
	drv->Align = txtAlign;		/* Set column alignment. */
	drv->GetAlign = txtGetAlign;	/* Get current horizontal pos */
	drv->GetCharWidth = txtGetCharWidth;
	drv->WriteString = txtWriteString;
	drv->Comment = txtComment;
	drv->SetFont = txtSetFont;
	drv->SetLPI = txtSetLPI;

	/** Now register with the report server **/
	prtRegisterPrintDriver(drv);

    return 0;
    }

