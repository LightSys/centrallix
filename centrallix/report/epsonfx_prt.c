#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cxlib/mtask.h"
#include "report.h"
#include "barcode.h"
#include "prtmgmt.h"
#include "fx_font_metrics.h"
#include "cxlib/newmalloc.h"

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
/* Module: 	epsonfx_prt.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 2, 1998					*/
/* Description: Print driver for Epson FX-series 9-pin dot matrix 	*/
/*		printers.  This is for most final report hardcopies.	*/
/*		I took the FX specs from the "Users Guide FX-870/1170"	*/
/*		book.							*/
/************************************************************************/


typedef struct 
    {
    int			CurAttr;
    int			CurFont;
    int			Flags;
    int			Lines;
    int			Cols;
    double		CurPageCol;
    double		CurLine;
    int			CurTableCol;
    int			ColRoomLeft;
    int			nColumns;
    char		Buffer[256];
    char		BCBuf[96];
    }
    Session, *pSession;


/** Values for our flags **/
#define FXP_F_INTABLE		1
#define FXP_F_BEGINPAGE		2
#define FXP_F_INTABCOL		4
#define FXP_F_INTABHDR		8

/** Attribute on/off flags **/
#define FXP_NUM_ATTRS		6

/** Some character-printing defines **/
#define ESC			(0x1B)
#define CTRL(x)			((x) - 64)

int fxp_internal_FindWrapPoint(char* txt, int n);
int fxpGetAttr(pPrtSession ps);
int fxpSetAttr(pPrtSession ps, int attr);

/** Attribute on/off escape sequences **/
static char* attr_on[6] =
    {
    "\33E\33G",		/* bold */
    "\33W1",		/* bigger */
    "\17",		/* smaller */
    "\33a1",		/* center */
    "\33-1",		/* underline */
    "\334",		/* italic */
    };

static char* attr_off[6] =
    {
    "\33F\33H", 	/* bold-off */
    "\33W0",		/* bigger-off */
    "\22",		/* smaller-off */
    "\33a0",		/* centering-off */
    "\33-0",		/* underline-off */
    "\335",		/* italic-off */
    };


/*** fxpOpen - open a new prpPrtSession ps, with the given FD as our 
 *** output stream.  Output can be to file, to pipe, etc.  We can
 *** even output to a pipe that is being read as input by another
 *** thread, because of MTASK's i/o scheduling.
 ***/
int
fxpOpen(pPrtSession ps)
    {
    pSession s;

    	/** Allocate a session **/
	s = (pSession)nmMalloc(sizeof(Session));
	if (!s) return -1;

	/** Send reset seq to printer **/
	ps->WriteFn(ps->WriteArg, "\33@\r",3,0,0);

	/** Set up the data structure **/
	ps->PrivateData = (void*)s;
	s->CurAttr = 0;
	s->CurPageCol = 0;
	s->CurLine = 0;
	s->CurFont = 0;

    return 0;
    }


/*** fxpClose - close an existing session.  This does NOT close the file
 *** descriptor -- the user must do that after fxpClose()'ing.
 ***/
int
fxpClose(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Send reset **/
	ps->WriteFn(ps->WriteArg, "\33@\r",3,0,0);
	nmFree(s,sizeof(Session));
	ps->PrivateData = NULL;

    return 0;
    }


/*** fxp_internal_Write - does an fdWrite unless it's a centered row, 
 *** in which case the data is buffered.
 ***/
int
fxp_internal_Write(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;
    char sbuf[32];
    static char short_bar[] = {0,0,7,255,7,255,7,255,0,0,0,0};
    static char long_bar[] = {0,0,255,255,255,255,255,255,0,0,0,0};

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
	    /** Enter graphics mode **/
	    sprintf(sbuf,"\33^1%c%c",(cnt*6)%256,(cnt*6)/256);
	    ps->WriteFn(ps->WriteArg,sbuf,5,0,0);

	    /** Go through text, printing short or long bars **/
	    for(n=0;n<cnt;n++)
		{
		if (text[n]&1) ps->WriteFn(ps->WriteArg,long_bar,12,0,0);
		else ps->WriteFn(ps->WriteArg,short_bar,12,0,0);
		}
	    return cnt;
	    }

    return ps->WriteFn(ps->WriteArg,text,cnt,0,0);
    }


/*** fxpGetCharWidth - find the current character width relative
 *** to the 'standard' character width, usually 10cpi.
 ***/
double
fxpGetCharWidth(pPrtSession ps, char* text, int a, int f)
    {
    pSession s = (pSession)(ps->PrivateData);
    double n,v;
    int cnt, id, i;

        /** Adjust based on attribs **/
	if (a == -1) a = s->CurAttr;
	if (f == -1) f = s->CurFont;
	if (a & RS_TX_PBARCODE)
	    {
	    n = 1.0/2.0;
	    }
        else if ((a & RS_TX_EXPANDED) && (a & RS_TX_COMPRESSED))
            {
            n = 100.0/85.0;
            }
        else if (a & RS_TX_EXPANDED)
            {
            n = 10.0/5.0;
            }
        else if (a & RS_TX_COMPRESSED)
            {
            n = 1000.0/1666.0;
            }
        else
            {
            n = 1.0;
            }
	if (text == NULL) return n;

	/** Adjust for proportional spaced font. **/
	cnt = strlen(text);
	if (f == RS_FONT_COURIER)
	    {
	    n = cnt*n;
	    }
	else
	    {
	    v = 0;
	    id = ((a & RS_TX_ITALIC)?1:0);
	    for(i=0;i<cnt;i++)
	        {
		if (((unsigned char*)text)[i] < 0x20 || ((unsigned char*)text)[i] > 0x7E)
		    v += n;
		else
		    v += n*fx_font_metrics[text[i]-0x20][id]/12.0;
		}
	    n = v;
	    }

    return n;
    }


/*** fxp_internal_BaseWidth - find the base width of the paper output,
 *** usually at 10cpi, likely 80 characters (.25 margin, 8" printable)
 ***/
int
fxp_internal_BaseWidth(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int basewidth;

        /** Check base width **/
        basewidth = s->Cols;
        if (basewidth == 0) basewidth=80;

    return basewidth;
    }


/*** fxpPageGeom - set the estimated geometry of the printed page.
 ***/
int
fxpPageGeom(pPrtSession ps, int lines, int cols)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Set the geometry **/
	s->Lines = lines;
	s->Cols = cols;

	/** Issue an escape seq to the printer to tell it the form length **/
	/*sprintf(sbuf,"\33C%c",lines);
        ps->WriteFn(ps->WriteArg, sbuf, 3,0,0);*/

    return 0;
    }


/*** fxpGetAlign - return the current x position on the page.
 ***/
double
fxpGetAlign(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurPageCol;
    }


/*** fxpAlign - horizontal tab over to a particular x position on the page.
 ***/
int
fxpAlign(pPrtSession ps, double x)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;
    char hposbuf[16];

	if (x <= s->CurPageCol) return 0;
	n = x*6 + 0.000001;
        sprintf(hposbuf,"\33$%c%c",n&255,n/256);
        ps->WriteFn(ps->WriteArg, hposbuf, 4,0,0);
	s->CurPageCol = x;

    return 0;
    }


/*** fxpWriteText - write some text onto the output. 
 ***/
int
fxpWriteString(pPrtSession ps, char* text, int cnt)
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
	s->CurPageCol += (fxpGetCharWidth(ps,text,-1,-1));

    return fxp_internal_Write(ps, text, cnt);
    }


/*** fxp_internal_FinCenter - finish a centered line.
 ***/
int
fxp_internal_FinCenter(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;
    char hposbuf[32];

	/** Only do centering logic if we printed real text. **/
	if (s->CurPageCol > 0)
	    {
	    /** First figure out how far in we need to move on the line **/
	    n = (fxp_internal_BaseWidth(ps)-s->CurPageCol)*6/2;
            sprintf(hposbuf,"\33$%c%c",n&255,n/256);
            ps->WriteFn(ps->WriteArg, hposbuf, 4,0,0);
	    }

	/** Now write the prewritten/buffered data **/
        ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer),0,0);

    return 0;
    }


/*** fxpWriteNL - write out a line break (NL/CR). 
 ***/
int
fxpWriteNL(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Centering?  Need to print the centered line? **/
	if (s->CurAttr & RS_TX_CENTER)
	    {
	    fxp_internal_FinCenter(ps);
	    }

	/** Clear the buffer & col cnt **/
	s->CurPageCol = 0;
	s->Buffer[0] = 0;
	s->CurLine += (6.0 / ps->LinesPerInch);
	if (s->CurLine >= s->Lines) s->CurLine = 0;

    return (ps->WriteFn(ps->WriteArg, "\r\n", 2,0,0)>0)?0:-1;
    }


/*** fxpWriteLine - write a horizontal line across the page.  To do this,
 *** we just write a series of dashes, either a standard width if the
 *** Cols setting is 0, or a setting relative to Cols and the current text
 *** width if Cols is not 0.
 ***/
int
fxpWriteLine(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int cnt;
    char sbuf[256];
    int oldattr;

	/** Not at beginning of line?  Issue a NL if not. **/
	if (s->CurPageCol > 0) fxpWriteNL(ps);

	/** Calculate the number of physical columns. **/
	oldattr = fxpGetAttr(ps);
	fxpSetAttr(ps, oldattr | RS_TX_BOLD);
	cnt = fxp_internal_BaseWidth(ps)/fxpGetCharWidth(ps,"-",-1,-1);
	cnt--;

	/** Create the print string. 0xc4 is the '-' graphic char. **/
	memset(sbuf,'-',cnt);
	sbuf[cnt++] = '\r';
	sbuf[cnt++] = '\n';
	sbuf[cnt]=0;

	/** Write it. **/
	ps->WriteFn(ps->WriteArg, sbuf, cnt, 0,0);
	fxpSetAttr(ps, oldattr);
	s->CurLine += (6.0 / ps->LinesPerInch);
	s->CurPageCol = 0;
	if (s->CurLine >= s->Lines) s->CurLine = 0;

    return 0;
    }


/*** fxpWriteFF - write out a form feed.  
 ***/
int
fxpWriteFF(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    static char ff_seq[3] = {0x0D,0x0C,0};

	/** Write the form feed **/
	ps->WriteFn(ps->WriteArg, ff_seq, 2,0,0);
	s->CurLine = 0;
	s->CurPageCol = 0;

    return 0;
    }


/*** fxpGetAttr - obtain the current attribute information for the
 *** printer, as previously set using fxpSetAttr.
 ***/
int
fxpGetAttr(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurAttr;
    }


/*** fxpSetAttr - change the text attributes, including bold, italic,
 *** underline, expanded, compressed, etc.  We don't have to worry about
 *** properly nesting these attributes like we do in HTML.  So, we just
 *** figure out what needs to be turned off and on, and send those codes.
 ***/
int
fxpSetAttr(pPrtSession ps, int attr)
    {
    pSession s = (pSession)(ps->PrivateData);
    int oldattr, onattr, offattr;
    int i;

	/** Find attrs for on and off **/
	oldattr = s->CurAttr;
	onattr = (attr^oldattr)&attr;
	offattr = (attr^oldattr)&oldattr;

	/** Check for barcode turned off first. **/
	if (offattr & RS_TX_PBARCODE) s->CurAttr &= ~RS_TX_PBARCODE;

	/** Check for centering turned off. **/
	if (offattr & RS_TX_CENTER)
	    {
	    fxp_internal_FinCenter(ps);
	    s->CurAttr &= ~RS_TX_CENTER;
	    }

	/** Turn off the ones that are no longer on **/
	for(i=0;i<FXP_NUM_ATTRS;i++) if (offattr & (1<<i))
	    {
	    if ((1<<i) != RS_TX_CENTER)
	        fxp_internal_Write(ps, attr_off[i], strlen(attr_off[i]));
	    }

	/** Turn on the ones that are newly enabled **/
	for(i=0;i<FXP_NUM_ATTRS;i++) if (onattr & (1<<i))
	    {
	    if ((1<<i) != RS_TX_CENTER)
	        fxp_internal_Write(ps, attr_on[i], strlen(attr_on[i]));
	    }

	/** Set the new curattr **/
	s->CurAttr = attr;

    return 0;
    }

#if 0
/*** fxpDoTable - begin a tabular structure in the output.  We'll need to
 *** keep track of set (or calculated) column widths and column count for
 *** making formatting decisions, since such formatting is not done for
 *** us.
 ***/
int
fxpDoTable(pPrtSession ps)
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


/*** fxpEndTable - finish up a table we started with fxpDoTable.  This also
 *** finishes up the last column, if need be.
 ***/
int
fxpEndTable(pPrtSession ps)
    {
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** Still only in header? If so, trigger the thing. **/
	if (s->Flags & FXP_F_INTABHDR)
	    {
	    fxpDoColumn(session, 1, 1);
	    }

	/** Write a newline **/
	fxpWriteNL(pPrtSession ps);

	/** Ok, cancel the table flags **/
	s->Flags &= ~(FXP_F_INTABLE|FXP_F_INTABCOL|FXP_F_INTABHDR);

    return 0;
    }


/*** fxp_internal_FindWrapPoint - finds the number of characters
 *** that can be included in a line of length n without breaking a
 *** word into two pieces.
 ***/
int
fxp_internal_FindWrapPoint(char* txt, int n)
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


/*** fxp_internal_WriteHeader - take the information already stored
 *** in the fxp structure and write it as a header on the output.
 *** this is called when DoColumn ends the fxpDoColHdr calls.
 ***/
int
fxp_internal_WriteHeader(pPrtSession ps)
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
		n = fxp_internal_FindWrapPoint(
		    s->ColTitles+s->ColTitleOffset[i],s->TableColWidth[i]);
		if (n < strlen(s->ColTitles+s->ColTitleOffset[i])) used_all=0;
		sprintf(ptr,"%-*.*s ",s->TableColWidth[i],n,s->ColTitles+s->ColTitleOffset[i]);
		ptr += (s->TableColWidth[i]+1);
		s->ColTitleOffset[i] += n;
		if (s->ColTitles[s->ColTitleOffset[i]] == ' ') s->ColTitleOffset[i]++;
		}

	    /** Write the header line. **/
	    ps->WriteFn(ps->WriteArg, s->ColHdr, strlen(s->ColHdr), 0,0);
	    fxpWriteNL(pPrtSession ps);
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
	fxpWriteNL(pPrtSession ps);

    return 0;
    }


/*** fxpDoColHdr - start a column header entry for a table that has
 *** already been started with fxpDoTable.  Although not particularly
 *** useful for column headers, this fn is the same as DoColumn in that
 *** it has a second parameter for start new row.
 ***/
int
fxpDoColHdr(pPrtSession ps, int start_row, int set_width)
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


/*** fxpDoColumn - start one cell within a row in a table.  The
 *** start-row parameter indicates whether to start a new row (1) or
 *** continue on the current row (0).
 ***/
int
fxpDoColumn(pPrtSession ps, int start_row, int col_span)
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
		fxp_internal_WriteHeader(pPrtSession ps);
		}
	    s->Flags &= ~FXP_F_INTABHDR;
	    }

	/** Set the current column... **/
	if (s->Flags & FXP_F_INTABCOL)
	    {
	    if (start_row)
		{
		fxpWriteNL(ps);
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


/*** fxpComment - write a comment into the output stream so the output stream
 *** can be re-parsed as a prtmgmt command stream.  This is a NOP for FXP.
 ***/
int
fxpComment(pPrtSession ps, char* txt)
    {
    return 0;
    }


/*** fxpSetFont - sets the current font for this session.
 ***/
int
fxpSetFont(pPrtSession ps, int font_id)
    {
    pSession s = (pSession)(ps->PrivateData);
    static char* proportional_on = "\33p1";
    static char* proportional_off = "\33p0";

    	/** Same font? **/
	if (font_id == s->CurFont) return 0;

	/** Issue the font change command **/
	if (font_id == RS_FONT_COURIER)
	    fxp_internal_Write(ps, proportional_off, 3);
	else
	    fxp_internal_Write(ps, proportional_on, 3);

	/** Set the font **/
    	s->CurFont = font_id;

    return 0;
    }


/*** fxpSetLPI - sets the current lines per inch (line spacing) on the
 *** page.
 ***/
int
fxpSetLPI(pPrtSession ps, int lines_per_inch)
    {
    char sbuf[24];

    	/** Build the print command to set the 216ths **/
	sprintf(sbuf, "\33" "3%c", 216/lines_per_inch);
	fxp_internal_Write(ps, sbuf, strlen(sbuf));

    return 0;
    }


/*** fxpInitialize -- initialize this module, which causes it to register
 *** itself with the report server.
 ***/
int
fxpInitialize()
    {
    pPrintDriver drv;

	/** Zero our globals **/
	/*memset(&FXP,0,sizeof(FXP));*/

	/** Allocate the descriptor structure **/
	drv = (pPrintDriver)nmMalloc(sizeof(PrintDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(PrintDriver));

	/** Fill out the structure **/
	strcpy(drv->Name, "FXP - Epson FX-Series Content Driver");
	strcpy(drv->ContentType, "text/x-epson-fx");
	drv->Open = fxpOpen;		/* Open a session */
	drv->Close = fxpClose;		/* Close a session */
	drv->PageGeom = fxpPageGeom;	/* Set page geometry */
	/*drv->WriteText = fxpWriteText;*/	/* Write a string of text */
	drv->WriteNL = fxpWriteNL;	/* Output a new line (CR) */
	drv->WriteFF = fxpWriteFF;	/* Output a "form feed" */
	drv->WriteLine = fxpWriteLine;	/* Output a horizontal line */
	drv->SetAttr = fxpSetAttr;	/* Set attributes */
	drv->GetAttr = fxpGetAttr;	/* Get attributes */
	/*drv->DoTable = fxpDoTable;*/	/* Start a table */
	/*drv->DoColHdr = fxpDoColHdr;*/	/* Start a column header */
	/*drv->DoColumn = fxpDoColumn;*/	/* Start a column */
	/*drv->EndTable = fxpEndTable;*/	/* Finish up the table */
	drv->Align = fxpAlign;		/* Set column alignment. */
	drv->GetAlign = fxpGetAlign;	/* Get current horizontal pos */
	drv->GetCharWidth = fxpGetCharWidth;
	drv->WriteString = fxpWriteString;
	drv->Comment = fxpComment;
	drv->SetFont = fxpSetFont;
	drv->SetLPI = fxpSetLPI;

	/** Now register with the report server **/
	prtRegisterPrintDriver(drv);

    return 0;
    }

