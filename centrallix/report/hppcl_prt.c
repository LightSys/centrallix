#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "mtask.h"
#include "report.h"
#include "barcode.h"
#include "prtmgmt.h"
#include "hp_font_metrics.h"
#include "newmalloc.h"

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
/* Module: 	hppcl_prt.c          					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 10, 1998					*/
/* Description: Print driver for HP printers using the PCL interface.	*/
/*		Specs from "HP PCL 5 Printer Language Technical 	*/
/*		Reference Manual".  Font size measurements obtained by	*/
/*		using a ruler :)					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: hppcl_prt.c,v 1.1 2001/08/13 18:01:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/hppcl_prt.c,v $

    $Log: hppcl_prt.c,v $
    Revision 1.1  2001/08/13 18:01:13  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:14  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

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
#define PCL_F_INTABLE		1
#define PCL_F_BEGINPAGE		2
#define PCL_F_INTABCOL		4
#define PCL_F_INTABHDR		8

/** Attribute on/off flags **/
#define PCL_NUM_ATTRS		6

/** Some character-printing defines **/
#define ESC			(0x1B)
#define CTRL(x)			((x) - 64)

int pcl_internal_FindWrapPoint(char* txt, int n);

/*** pclOpen - open a new print session, with the given FD as our 
 *** output stream.  Output can be to file, to pipe, etc.  We can
 *** even output to a pipe that is being read as input by another
 *** thread, because of MTASK's i/o scheduling.
 ***/
int
pclOpen(pPrtSession ps)
    {
    pSession s;
    static char reset_seq[] = { ESC, 'E', 0};
    /*char src_seq[] = "\33&l0H";*/

        /** Allocate a session **/
	s = (pSession)nmMalloc(sizeof(Session));
	if (!s) return -1;

	/** Send a reset **/
	ps->WriteFn(ps->WriteArg, reset_seq, 2, 0,0);

	/*src_seq[3] = '0' + *source;
	fdWrite(fd, src_seq, strlen(src_seq), 0,0);*/

	ps->PrivateData = (void*)s;
	s->CurAttr = 0;
	s->CurPageCol = 0;
	s->CurLine = 0;
	s->CurFont = 0;

    return 0;
    }


/*** pclClose - close an existing session.  This does NOT close the file
 *** descriptor -- the user must do that after pclClose()'ing.
 ***/
int
pclClose(pPrtSession ps)
    {
    static char reset_seq[] = { ESC, 'E', 0};
    pSession s = (pSession)(ps->PrivateData);

	/** Send a reset **/
	ps->WriteFn(ps->WriteArg, reset_seq, 2, 0,0);

	/** Free the memory **/
	nmFree(s, sizeof(Session));
	ps->PrivateData = NULL;

    return 0;
    }


/*** pcl_internal_Write - does an fdWrite unless it's a centered row, 
 *** in which case the data is buffered.
 ***/
int
pcl_internal_Write(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
    int n;
    static char short_bar[] = "\33&a-24V\33*c36v18h0P\33&a+24v+36H";
    static char long_bar[] = "\33&a-78V\33*c90v18h0P\33&a+78v+36H";

	/** If we're in a centered row, cache the buffer **/
	if (s->CurAttr & RS_TX_CENTER)
	    {
	    n = strlen(s->Buffer);
	    memcpy(s->Buffer+n,text,cnt);
	    s->Buffer[n+cnt] = 0;
	    return cnt;
	    }

	/** If we're in a barcode, output the graphics here **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    for(n=0;n<cnt;n++)
		{
		if (text[n]&1) ps->WriteFn(ps->WriteArg,long_bar,strlen(long_bar),0,0);
		else ps->WriteFn(ps->WriteArg,short_bar,strlen(short_bar),0,0);
		}
	    return cnt;
	    }

    return ps->WriteFn(ps->WriteArg,text,cnt,0,0);
    }


/*** pclGetCharWidth - find the current character width relative
 *** to the 'standard' character width, usually 10cpi.
 ***/
double
pclGetCharWidth(pPrtSession ps, char* text, int a, int f)
    {
    pSession s = (pSession)(ps->PrivateData);
    double n,v;
    int cnt,i,id;

	/** Adjust based on attribs **/
	if (a == -1) a = s->CurAttr;
	if (f == -1) f = s->CurFont;
	if (a & RS_TX_PBARCODE)
	    {
	    n = 1.0/2.0;
	    }
	else if ((a & RS_TX_EXPANDED) && (a & RS_TX_COMPRESSED))
	    {
	    n = 14.0/12.0;
	    }
	else if (a & RS_TX_EXPANDED)
	    {
	    n = 17.0/12.0;
	    }
	else if (a & RS_TX_COMPRESSED)
	    {
	    n = 7.0/12.0;
	    }
	else
	    {
	    n = 1.0;
	    }
	if (text == NULL) return n;

	/** Adjust based on the width of a char **/
	cnt = strlen(text);
	if (f == RS_FONT_COURIER)
	    {
	    n=cnt*n;
	    }
	else
	    {
	    id = ((a & RS_TX_ITALIC)?1:0) + ((a & RS_TX_BOLD)?2:0);
	    v = 0;
	    for(i=0;i<cnt;i++)
	        {
		if (((unsigned char*)text)[i] < 0x20 || ((unsigned char*)text)[i] > 0x7E)
		    v += n;
		else if (f == RS_FONT_TIMES)
		    v += n*hp_times_font_metrics[text[i]-0x20][id]/60.0;
		else if (f == RS_FONT_HELVETICA)
		    v += n*hp_helvetica_font_metrics[text[i]-0x20][id]/60.0;
		}
	    n = v;
	    }

    return n;
    }


/*** pcl_internal_BaseWidth - find the base width of the paper output,
 *** usually at 10cpi, likely 80 characters (.25 margin, 8" printable)
 ***/
int
pcl_internal_BaseWidth(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int basewidth;

	/** Check base width **/
	basewidth = s->Cols;
	if (basewidth == 0) basewidth=80;

    return basewidth;
    }


/*** pclPageGeom - set the estimated geometry of the printed page.
 ***/
int
pclPageGeom(pPrtSession ps, int lines, int cols)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Set the geometry **/
	s->Lines = lines;
	s->Cols = cols;

    return 0;
    }


/*** pclGetAlign - return the current x position on the page.
 ***/
double
pclGetAlign(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurPageCol;
    }


/*** pclAlign - horizontal tab over to a particular x position.
 ***/
int
pclAlign(pPrtSession ps, double x)
    {
    pSession s = (pSession)(ps->PrivateData);
    char hposbuf[32];

	/** Determine h pos to tab to. **/
        /*sprintf(hposbuf,"\33&a+%.1fH",(x - s->CurPageCol)*72 + 0.000001);*/
        sprintf(hposbuf,"\33&a%.1fH",(x)*72 + 0.000001);
        ps->WriteFn(ps->WriteArg, hposbuf, strlen(hposbuf),0,0);
	s->CurPageCol = x;

    return 0;
    }


/*** pclWriteString - write some text onto the output. 
 ***/
int
pclWriteString(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Check count **/
	if (cnt == -1) cnt = strlen(text);

	/** If in a barcode, do the conversion now. **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    if (barEncodeNumeric(BAR_T_POSTAL,text,cnt,s->BCBuf,69) >= 0)
		{
		text = s->BCBuf;
		cnt = strlen(text);
		}
	    else
		{
		return -1;
		}
	    }

	/** Update column counter **/
	s->CurPageCol += pclGetCharWidth(ps,text,-1,-1);

    return pcl_internal_Write(ps, text, cnt);
    }


/*** pcl_internal_FinCenter - finish a centered line.
 ***/
int
pcl_internal_FinCenter(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    double n;
    char hposbuf[32];

	/** Only do centering logic if we printed real text. **/
	if (s->CurPageCol > 0)
	    {
	    /** First figure out how far in we need to move on the line **/
            n = pcl_internal_BaseWidth(ps)-s->CurPageCol;
            sprintf(hposbuf,"\33&a+%.1fH",n*72/2 + 0.000001);
            ps->WriteFn(ps->WriteArg, hposbuf, strlen(hposbuf),0,0);
	    }

	/** Now write the prewritten/buffered data **/
        ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer),0,0);

    return 0;
    }


/*** pclWriteNL - write out a line break (NL/CR). 
 ***/
int
pclWriteNL(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Special processing if we just centered a line **/
    	if (s->CurAttr & RS_TX_CENTER)
	    {
	    pcl_internal_FinCenter(ps);
	    }

	/** Reset the page counters **/
    	s->Buffer[0] = 0;
    	s->CurPageCol = 0;
	s->CurLine += (6.0 / ps->LinesPerInch);
	if (s->CurLine >= s->Lines) s->CurLine = 0;

    return (ps->WriteFn(ps->WriteArg, "\r\n", 2,0,0)>0)?0:-1;
    }


/*** pclWriteLine - write a horizontal line across the page.  To do this,
 *** we just write a series of dashes, either a standard width if the
 *** Cols setting is 0, or a setting relative to Cols and the current text
 *** width if Cols is not 0.
 ***/
int
pclWriteLine(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    int basewidth;
    int f;
    int cnt;
    char sbuf[256];

	/** Verify. **/
	f = s->Flags;

	/** Not at beginning of line?  Send a NL if not. **/
	if (s->CurPageCol > 0) pclWriteNL(ps);

	/** Determine 'decipoints' (1/720 inch) for line. **/
	/** Assume 10cpi for assigned baseline geometry **/
	basewidth=s->Cols;
	if (basewidth == 0) basewidth=80;
	cnt = basewidth * 72;

	sprintf(sbuf,"\33&a-50V\33*c6v%dh0P\33&a+50V\r\n", cnt);

	/** Write it. **/
	ps->WriteFn(ps->WriteArg, sbuf, strlen(sbuf), 0,0);
	s->CurLine += (6.0 / ps->LinesPerInch);
	s->CurPageCol = 0;
	if (s->CurLine >= s->Lines) s->CurLine = 0;

    return 0;
    }


/*** pclWriteFF - write out a form feed.  
 ***/
int
pclWriteFF(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    static char ff_seq[2] = {0x0C,0};

	/** Write the form feed **/
	if (s->CurLine != 0 || s->CurPageCol != 0) 
	    ps->WriteFn(ps->WriteArg, ff_seq, 1,0,0);
	s->CurLine = 0;

    return 0;
    }


/*** pclGetAttr - get current attributes, as previously set with the
 *** pclSetAttr call.
 ***/
int
pclGetAttr(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurAttr;
    }


/*** pclSetAttr - change the text attributes, including bold, italic,
 *** underline, expanded, compressed, etc.  We don't have to worry about
 *** properly nesting these attributes like we do in HTML.  So, we just
 *** figure out what needs to be turned off and on, and send those codes.
 ***/
int
pclSetAttr(pPrtSession ps, int attr)
    {
    pSession s = (pSession)(ps->PrivateData);
    int oldattr, onattr, offattr;
    double prtattr;
    char attrbuf[32];

	/** Find attrs for on and off **/
	oldattr = s->CurAttr;
	onattr = (attr^oldattr)&attr;
	offattr = (attr^oldattr)&oldattr;

	/** Check for barcode off first **/
	if (offattr & RS_TX_PBARCODE) s->CurAttr &= ~RS_TX_PBARCODE;

	/** Check for underline on/off **/
	if (onattr & RS_TX_UNDERLINE) pcl_internal_Write(ps, "\33&d0D", 5);
	else if (offattr & RS_TX_UNDERLINE) pcl_internal_Write(ps, "\33&d@", 4);

	/** Check for bold on/off **/
	if (onattr & RS_TX_BOLD) pcl_internal_Write(ps, "\33(s3B", 5);
	else if (offattr & RS_TX_BOLD) pcl_internal_Write(ps, "\33(s0B",5);

	/** Check for italic on/off **/
	if (onattr & RS_TX_ITALIC) pcl_internal_Write(ps, "\33(s1S", 5);
	else if (offattr & RS_TX_ITALIC) pcl_internal_Write(ps, "\33(s0S", 5);

	/** Check for centering on/off before font width setting change **/
	if (offattr & RS_TX_CENTER) 
	    {
	    pcl_internal_FinCenter(ps);
	    s->CurAttr &= ~RS_TX_CENTER;
	    }

	/** Font width.  Use 'pitch' setting for Courier font. **/
	if ((onattr|offattr) & (RS_TX_COMPRESSED|RS_TX_EXPANDED))
	    {
	    prtattr = 10.0/pclGetCharWidth(ps,NULL,attr,-1);
	    /*if ((attr & RS_TX_COMPRESSED) && (attr & RS_TX_EXPANDED))
		{
		prtattr = 8.5;
		}
	    else if (attr & RS_TX_COMPRESSED)
		{
		prtattr = 16.66;
		}
	    else if (attr & RS_TX_EXPANDED)
		{
		prtattr = 7;
		}
	    else
		{
		prtattr = 10;
		}*/
	    sprintf(attrbuf,"\33(s%.3fH\33(s%.3fV",prtattr+0.000001,(120/prtattr)+0.000001);
	    pcl_internal_Write(ps, attrbuf, strlen(attrbuf));
	    }

	/** Write the attr into the session structure **/
	s->CurAttr = attr;

    return 0;
    }

#if 0
/*** pclDoTable - begin a tabular structure in the output.  We'll need to
 *** keep track of set (or calculated) column widths and column count for
 *** making formatting decisions, since such formatting is not done for
 *** us.
 ***/
int
pclDoTable(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** Set in table **/
	s->Flags |= PCL_F_INTABLE;
	s->nColumns = 0;
	s->ColTitleLen = 0;

    return 0;
    }


/*** pclEndTable - finish up a table we started with pclDoTable.  This also
 *** finishes up the last column, if need be.
 ***/
int
pclEndTable(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** Still only in header? If so, trigger the thing. **/
	if (s->Flags & PCL_F_INTABHDR)
	    {
	    pclDoColumn(session, 1, 1);
	    }

	/** Write a newline **/
	pclWriteNL(session);

	/** Ok, cancel the table flags **/
	s->Flags &= ~(PCL_F_INTABLE|PCL_F_INTABCOL|PCL_F_INTABHDR);

    return 0;
    }


/*** pcl_internal_FindWrapPoint - finds the number of characters
 *** that can be included in a line of length n without breaking a
 *** word into two pieces.
 ***/
int
pcl_internal_FindWrapPoint(char* txt, int n)
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


/*** pcl_internal_WriteHeader - take the information already stored
 *** in the pcl structure and write it as a header on the output.
 *** this is called when DoColumn ends the pclDoColHdr calls.
 ***/
int
pcl_internal_WriteHeader(session)
    {
    pFile fd;
    pSession s = &(PCL.Sessions[session]);
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
		n = pcl_internal_FindWrapPoint(
		    s->ColTitles+s->ColTitleOffset[i],s->TableColWidth[i]);
		if (n < strlen(s->ColTitles+s->ColTitleOffset[i])) used_all=0;
		sprintf(ptr,"%-*.*s ",s->TableColWidth[i],n,s->ColTitles+s->ColTitleOffset[i]);
		ptr += (s->TableColWidth[i]+1);
		s->ColTitleOffset[i] += n;
		if (s->ColTitles[s->ColTitleOffset[i]] == ' ') s->ColTitleOffset[i]++;
		}

	    /** Write the header line. **/
	    fdWrite(fd, s->ColHdr, strlen(s->ColHdr), 0,0);
	    pclWriteNL(session);
	    }

	/** Now write the line underscoring the header area **/
	memset(s->ColHdr, '-', strlen(s->ColHdr));
	for(n=0,i=0;i<s->nColumns-1;i++)
	    {
	    n+=s->TableColWidth[i];
	    s->ColHdr[n]=' ';
	    n++;
	    }
	fdWrite(fd, s->ColHdr, strlen(s->ColHdr), 0,0);
	pclWriteNL(session);

    return 0;
    }


/*** pclDoColHdr - start a column header entry for a table that has
 *** already been started with pclDoTable.  Although not particularly
 *** useful for column headers, this fn is the same as DoColumn in that
 *** it has a second parameter for start new row.
 ***/
int
pclDoColHdr(pPrtSession ps, int start_row, int set_width)
    {
    pSession s = &(PCL.Sessions[session]);
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** If in header, advance the column title information **/
	if (s->Flags & PCL_F_INTABHDR)
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

	/** Now in hdr **/
	s->Flags |= PCL_F_INTABHDR;

    return 0;
    }


/*** pclDoColumn - start one cell within a row in a table.  The
 *** start-row parameter indicates whether to start a new row (1) or
 *** continue on the current row (0).
 ***/
int
pclDoColumn(pPrtSession ps, int start_row, int col_span)
    {
    pSession s = &(PCL.Sessions[session]);
    pFile fd;

	/** Verify session. **/
	if ((fd=s->fd) == NULL) return -1;

	/** If in header, finish up last item and write the header out **/
	if (s->Flags & PCL_F_INTABHDR)
	    {
	    /** String-terminate the end of the titles. **/
	    s->ColTitles[s->ColTitleLen]=0;

	    /** Did user actually write any info?  If not, skip hdr **/
	    if (s->nColumns != (s->ColTitleLen + 1) && s->nColumns > 0)
		{
		pcl_internal_WriteHeader(session);
		}
	    s->Flags &= ~PCL_F_INTABHDR;
	    }

	/** Set the current column... **/
	if (s->Flags & PCL_F_INTABCOL)
	    {
	    if (start_row)
		{
		pclWriteNL(session);
	        s->CurTableCol = 0;
		}
	    else
		{
		/** First check for padding on previous col. **/
		s->ColRoomLeft++;
		memset(s->Buffer,' ',s->ColRoomLeft);
		s->Buffer[s->ColRoomLeft] = 0;
		fdWrite(fd, s->Buffer, s->ColRoomLeft, 0,0);

		/** Advance to next col. **/
	        s->CurTableCol++;
		}
	    }
	else
	    {
	    s->CurTableCol = 0;
	    s->Flags |= PCL_F_INTABCOL;
	    }

	/** Find the available space for this column **/
	s->ColRoomLeft = 0;
	while(col_span-- && s->CurTableCol < s->nColumns)
	    {
	    s->ColRoomLeft += (s->TableColWidth[s->CurTableCol++] + 1);
	    }
	s->ColRoomLeft--;
	s->CurTableCol--;

    return 0;
    }
#endif

/*** pclComment - this is pretty much a NOP for PCL.
 ***/
int
pclComment(pPrtSession ps, char* txt)
    {
    return 0;
    }


/*** pclSetFont - set the active font for this session.
 ***/
int
pclSetFont(pPrtSession ps, int font_id)
    {
    pSession s = (pSession)(ps->PrivateData);
    static char* set_courier_font = "\33(s0P\33(s3T";
    static char* set_times_font = "\33(s1P\33(s4101T";
    static char* set_helvetica_font = "\33(s1P\33(s4148T";

    	/** Same font? **/
	if (font_id == s->CurFont) return 0;

	/** Issue the appropriate font-change commands. **/
	switch(font_id)
	    {
	    case RS_FONT_COURIER: pcl_internal_Write(ps, set_courier_font, strlen(set_courier_font)); break;
	    case RS_FONT_TIMES: pcl_internal_Write(ps, set_times_font, strlen(set_times_font)); break;
	    case RS_FONT_HELVETICA: pcl_internal_Write(ps, set_helvetica_font, strlen(set_helvetica_font)); break;
	    }

	/** Set the font. **/
    	s->CurFont = font_id;

    return 0;
    }


/*** pclSetLPI - set the number of lines per inch on the output.
 ***/
int
pclSetLPI(pPrtSession ps, int lines_per_inch)
    {
    char sbuf[24];
    double forty_eighths;

	/** Determine number of 48ths of an inch this equals **/
	forty_eighths = ((double)48.0)/lines_per_inch;

    	/** Build the command for the printer **/
	sprintf(sbuf,"\33&l%.4fC", forty_eighths + 0.000001);
	pcl_internal_Write(ps, sbuf, strlen(sbuf));

    return 0;
    }


/*** pclInitialize -- initialize this module, which causes it to register
 *** itself with the report server.
 ***/
int
pclInitialize()
    {
    pPrintDriver drv;

	/** Allocate the descriptor structure **/
	drv = (pPrintDriver)nmMalloc(sizeof(PrintDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(PrintDriver));

	/** Fill out the structure **/
	strcpy(drv->Name, "PCL - HP PCL Content Driver");
	strcpy(drv->ContentType, "text/x-hp-pcl");
	drv->Open = pclOpen;		/* Open a session */
	drv->Close = pclClose;		/* Close a session */
	drv->PageGeom = pclPageGeom;	/* Set page geometry */
	/*drv->WriteText = pclWriteText;*/	/* Write a string of text */
	drv->WriteNL = pclWriteNL;	/* Output a new line (CR) */
	drv->WriteFF = pclWriteFF;	/* Output a "form feed" */
	drv->WriteLine = pclWriteLine;	/* Output a horizontal line */
	drv->SetAttr = pclSetAttr;	/* Set attributes */
	drv->GetAttr = pclGetAttr;	/* Get attributes */
	/*drv->DoTable = pclDoTable;*/	/* Start a table */
	/*drv->DoColHdr = pclDoColHdr;*/	/* Start a column header */
	/*drv->DoColumn = pclDoColumn;*/	/* Start a column */
	/*drv->EndTable = pclEndTable;*/	/* Finish up the table */
	drv->Align = pclAlign;		/* Set horizontal pos alignment */
	drv->GetAlign = pclGetAlign;	/* get horizontal pos */
	drv->GetCharWidth = pclGetCharWidth;
	drv->WriteString = pclWriteString;
	drv->Comment = pclComment;
	drv->SetFont = pclSetFont;
	drv->SetLPI = pclSetLPI;	/* set lines per inch */

	/** Now register with the report server **/
	prtRegisterPrintDriver(drv);

    return 0;
    }

