#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "mtask.h"
#include "barcode.h"
#include "report.h"
#include "prtmgmt.h"
#include "ht_font_metrics.h"
#include "newmalloc.h"
#include "mtsession.h"

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
/* Module: 	html_prt.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	August 31, 1998						*/
/* Description: Print driver for HTML -- primarily for document preview	*/
/*		purposes.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: html_prt.c,v 1.1 2001/08/13 18:01:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/html_prt.c,v $

    $Log: html_prt.c,v $
    Revision 1.1  2001/08/13 18:01:13  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:15  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

typedef struct
    {
    int			CurAttr;
    int			CurFont;
    int			SavedFont;
    int			CurLine;
    double		CurCol;
    int			Flags;
    int			Lines;
    int			Cols;
    char		Buffer[512];
    char		BCBuf[96];
    }
    Session, *pSession;


/** Values for our flags **/
#define HTP_F_INTABLE		1
#define HTP_F_BEGINPAGE		2
#define HTP_F_INTABCOL		4
#define HTP_F_INTABHDR		8

/** Attribute on/off flags **/
static char* attr_on[] = { "<B>","<BIG>","<SMALL>","<CENTER>","<U>","<I>" };
static char* attr_off[] = { "</B>","</BIG>","</SMALL>","</CENTER>","</U>","</I>" };
static int attr_prec[] = { 3,1,2,0,4,5 };
#define HTP_NUM_ATTRS		6

/** Some text constants **/
#define HTP_TX_DOCHDR "<HTML><HEAD><TITLE>Centrallix Report</TITLE></HEAD><BODY background=/sys/images/slate2.gif text='#000080'>\n"
#define HTP_TX_DOCFTR "</BODY></HTML>\n"
#define HTP_TX_PGHDR "<TABLE bgcolor='#FFFFFF' border=1 cellspacing=0 width=550><TR><TD><TT>"
#define HTP_TX_PGFTR "</TT></TD></TR></TABLE>\n<BR>\n"

int htpSetFont(pPrtSession ps, int font_id);
int htpComment(pPrtSession ps, char* comment);

/*** htpOpen - open a new print session, with the given FD as our 
 *** output stream.  Output can be to file, to pipe, etc.  We can
 *** even output to a pipe that is being read as input by another
 *** thread, because of MTASK's i/o scheduling.
 ***/
int
htpOpen(pPrtSession ps)
    {
    pSession s;

    	/** Allocate a session. **/
	s = (pSession)nmMalloc(sizeof(Session));
	if (!s) return -1;

	/** Initialize it **/
	s->CurAttr = 0;
	s->CurLine = 0;
	s->Flags = HTP_F_BEGINPAGE;
	s->Lines = 60;
	s->Cols = 80;
	s->CurCol = 0;
	s->CurFont = 0;
	s->SavedFont = 0;
	ps->PrivateData = (void*)s;

	/** Write the document header **/
	ps->WriteFn(ps->WriteArg, HTP_TX_DOCHDR, strlen(HTP_TX_DOCHDR), 0, 0);

    return 0;
    }


/*** htpClose - close an existing session.  This does NOT close the file
 *** descriptor -- the user must do that after htpClose()'ing.
 ***/
int
htpClose(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Need to finish up a page? **/
	if (!(s->Flags & HTP_F_BEGINPAGE))
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGFTR, strlen(HTP_TX_PGFTR), 0, 0);
	    }

	/** Write the document footer **/
	ps->WriteFn(ps->WriteArg, HTP_TX_DOCFTR, strlen(HTP_TX_DOCFTR), 0, 0);

	/** Clear it. **/
	nmFree(s,sizeof(Session));
	ps->PrivateData = NULL;

    return 0;
    }


/*** htpPageGeom - set the estimated geometry of the printed page.
 ***/
int
htpPageGeom(pPrtSession ps, int lines, int cols)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Set the geometry **/
	s->Lines = lines;
	s->Cols = cols;

    return 0;
    }


/*** htpGetPitch - return the current character width of character in terms
 *** of the width of a 'normal' character.  For proportional fonts, such as
 *** times and helvetica, we use the hp-pcl font metrics table to estimate the
 *** font sizes.
 ***/
double
htpGetCharWidth(pPrtSession ps, char* text, int a, int f)
    {
    pSession s = (pSession)(ps->PrivateData);
    double v,n;
    int cnt,i,id;

	/** Get the attribute-width-multiplier. **/
	if (a == -1) a = s->CurAttr;
	if (f == -1) f = s->CurFont;
    	if ((a & RS_TX_EXPANDED) && (a & RS_TX_COMPRESSED))
	    v = 1.0;
	else if (a & RS_TX_COMPRESSED)
	    v = 1000.0/1666.0 /*0.85*/;
	else if (a & RS_TX_EXPANDED)
	    v = 2 /*1.3*/;
	else
	    v = 1.0;
	if (text == NULL) return v;

	/** Account for length and characters in the text to be measured **/
	cnt = strlen(text);
	if (f == RS_FONT_COURIER)
	    {
	    v = cnt*v;
	    }
	else
	    {
	    n = 0;
	    id = ((a & RS_TX_ITALIC)?1:0) + ((a & RS_TX_BOLD)?2:0);
	    for(i=0;i<cnt;i++)
	        {
		if (((unsigned char*)text)[i] < 0x20 || ((unsigned char*)text)[i] > 0x7E)
		    n += v;
		else if (f == RS_FONT_TIMES)
		    n += v * ht_times_font_metrics[text[i]-0x20][id]/45.0;
		else if (f == RS_FONT_HELVETICA)
		    n += v * ht_helvetica_font_metrics[text[i]-0x20][id]/50.0;
		}
	    v = n;
	    }

    return v;
    }


/*** htp_internal_Write - write text onto the output, at a lower level
 *** than htpWriteText.  This function is internal only.
 ***/
int
htp_internal_Write(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
    int i;
    static char* long_bar = "<IMG SRC=/sys/images/long_bar.gif border=0>";
    static char* short_bar = "<IMG SRC=/sys/images/short_bar.gif border=0>";
    char bcbuf[64];

	/** Check to see if we need to write page header **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    htpSetFont(ps,s->SavedFont);
	    s->Flags &= ~HTP_F_BEGINPAGE;
	    }

	/** If in barcode, issue string of images instead of literal text **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    for(i=0;i<cnt;i++)
		{
		if (text[i]&1)
		    ps->WriteFn(ps->WriteArg,long_bar,strlen(long_bar),0,0);
		else
		    ps->WriteFn(ps->WriteArg,short_bar,strlen(short_bar),0,0);
		}
	    strcpy(bcbuf, "BDATA ");
	    memccpy(bcbuf+6, text, 0, (cnt>57)?57:cnt);
	    bcbuf[6+((cnt>57)?57:cnt)] = '\0';
	    htpComment(ps, bcbuf);
	    return cnt;
	    }

    return ps->WriteFn(ps->WriteArg, text, cnt, 0, 0);
    }


/*** htpWriteText - write some text onto the output.  Since this is
 *** html, we don't have to worry about wrapping the text.
 ***/
int
htpWriteText(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
	
	/** Do count if lazy user called with -1. **/
	if (cnt == -1) cnt = strlen(text);

	/** In barcoding right now? **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    if (barEncodeNumeric(BAR_T_POSTAL,text,cnt,s->BCBuf,69) >=0)
		{
		text = s->BCBuf;
		cnt = strlen(text);
		}
	    else
		{
		return -1;
		}
	    }

    return htp_internal_Write(ps, text, cnt);
    }


/*** htpWriteString - write a string of text to the output.  Wrapping
 *** has already been done for us.
 ***/
int
htpWriteString(pPrtSession ps, char* text, int cnt)
    {
    pSession s = (pSession)(ps->PrivateData);
    char* ptr;
    char* eptr;
    int i;
    static char* long_bar = "<IMG SRC=/sys/images/long_bar.gif border=0>";
    static char* short_bar = "<IMG SRC=/sys/images/short_bar.gif border=0>";
    char bcbuf[64];

	/** Check to see if we need to write page header **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    htpSetFont(ps,s->SavedFont);
	    s->Flags &= ~HTP_F_BEGINPAGE;
	    }

    	/** Check string length cnt **/
	if (cnt == -1) cnt = strlen(text);

	/** Barcode? **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    /** Barcode.  Encode it and send that string off **/
	    if (barEncodeNumeric(BAR_T_POSTAL,text,cnt,s->BCBuf,69) >=0)
		{
	        strcpy(bcbuf, "BDATA ");
	        memccpy(bcbuf+6, text, 0, (cnt>57)?57:cnt);
	        bcbuf[6+((cnt>57)?57:cnt)] = '\0';
	        htpComment(ps, bcbuf);
		text = s->BCBuf;
		cnt = strlen(text);
	        s->CurCol += cnt*htpGetCharWidth(ps,NULL,-1,-1);
		}
	    else
		{
		return -1;
		}
	    }
	else
	    {
	    /** Non barcode.  Output the text with ' ', '<', '>' translated. **/
	    s->CurCol += htpGetCharWidth(ps,text,-1,-1);
	    ptr = text;
	    eptr = s->Buffer;
	    while(*ptr)
	        {
		if (*ptr == ' ')
		    {
		    strcpy(eptr,"&nbsp;");
		    eptr += 6;
		    ptr++;
		    }
		else if (*ptr == '<')
		    {
		    strcpy(eptr,"&lt;");
		    eptr += 4;
		    ptr++;
		    }
		else if (*ptr == '>')
		    {
		    strcpy(eptr,"&gt;");
		    eptr += 4;
		    ptr++;
		    }
		else if (*ptr == '&')
		    {
		    strcpy(eptr,"&amp;");
		    eptr += 5;
		    ptr++;
		    }
		else
		    {
		    *(eptr++) = *(ptr++);
		    }

		/** If buffer is full, dump it NOW! **/
		if (eptr > s->Buffer+500)
		    {
	    	    *eptr = '\0';
	            ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer), 0,0);
		    eptr = s->Buffer;
		    }
		}
	    *eptr = '\0';
	    text = s->Buffer;
	    cnt = strlen(text);
	    }

	/** If in barcode, issue string of images instead of literal text **/
	if (s->CurAttr & RS_TX_PBARCODE)
	    {
	    for(i=0;i<cnt;i++)
		{
		if (text[i]&1)
		    ps->WriteFn(ps->WriteArg,long_bar,strlen(long_bar),0,0);
		else
		    ps->WriteFn(ps->WriteArg,short_bar,strlen(short_bar),0,0);
		}
	    }
	else
	    {
	    /** Write it. **/
	    ps->WriteFn(ps->WriteArg, text, cnt, 0,0);
	    }

    return cnt;
    }


/*** htpWriteNL - write out a line break (NL/CR).  In HTML, this
 *** is just a <BR>.
 ***/
int
htpWriteNL(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    
	/** Check to see if we need to write page header **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    htpSetFont(ps,s->SavedFont);
	    s->Flags &= ~HTP_F_BEGINPAGE;
	    }

    	/** Increment line cnt **/
	s->CurLine++;
	s->CurCol = 0;

    return ps->WriteFn(ps->WriteArg, "<BR>\r\n", 6,0,0);
    }


/*** htpWriteLine - write a horizontal line across the page.  In HTML,
 *** this should be done with <HR>.
 ***/
int
htpWriteLine(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    
	/** Check to see if we need to write page header **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    htpSetFont(ps,s->SavedFont);
	    s->Flags &= ~HTP_F_BEGINPAGE;
	    }

    	/** Increment line cnt **/
	s->CurLine++;
	s->CurCol = 0;

    return ps->WriteFn(ps->WriteArg, "<HR>\r\n", 6,0,0);
    }


/*** htpWriteFF - write out a form feed.  In the HTML document, this
 *** ends the one-row one-col white table and starts another one when
 *** the caller does another text write.
 ***/
int
htpWriteFF(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Didn't start the old page? **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    /*ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    s->Flags &= ~HTP_F_BEGINPAGE;*/
	    htpWriteNL(ps);
	    }

	/** Need to fill out the page to the end? **/
	while (s->CurLine < s->Lines)
	    {
	    htpWriteNL(ps);
	    }

	/** End the previous page. **/
	s->SavedFont = s->CurFont;
	htpSetFont(ps,0);
	ps->WriteFn(ps->WriteArg, HTP_TX_PGFTR, strlen(HTP_TX_PGFTR), 0, 0);
	s->Flags |= HTP_F_BEGINPAGE;
	s->CurLine = 0;
	s->CurCol = 0;

    return 0;
    }


/*** htpGetAttr - gets the current attributes based on the attrib
 *** set previously using htpSetAttr.
 ***/
int
htpGetAttr(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurAttr;
    }


/*** htpSetAttr - change the text attributes, including bold, italic,
 *** underline, expanded, compressed, etc.  We have to be very careful
 *** in HTML so we get the nesting of the various tags correct.  We do
 *** this here by applying/unapplying the tags in order of the bits in
 *** the attributes mask.  So, if we have 1011 and we want 1101, we
 *** first turn off 1000, then turn off 0010, then turn on 0100, and then
 *** turn back on 1000.  So the higher bits have the innermost nest levels.
 ***/
int
htpSetAttr(pPrtSession ps, int attr)
    {
    pSession s = (pSession)(ps->PrivateData);
    int oldattr, tmpattr;
    int i;

	/** Check to see if we need to write page header **/
	if (s->Flags & HTP_F_BEGINPAGE)
	    {
	    ps->WriteFn(ps->WriteArg, HTP_TX_PGHDR, strlen(HTP_TX_PGHDR), 0, 0);
	    s->Flags &= ~HTP_F_BEGINPAGE;
	    htpSetFont(ps,s->SavedFont);
	    }

	/** Find out what styles we need to set and undo **/
	oldattr = s->CurAttr;
	tmpattr = attr^oldattr;
	tmpattr &= ~RS_TX_PBARCODE;

	/** If we're ending a barcode, we need to end it NOW. **/
	if ((oldattr & RS_TX_PBARCODE) && !(attr & RS_TX_PBARCODE)) 
	    {
	    s->CurAttr &= ~RS_TX_PBARCODE;
	    htpComment(ps,"BARCODE 0");
	    }
	else if ((attr & RS_TX_PBARCODE) && !(oldattr & RS_TX_PBARCODE))
	    {
	    htpComment(ps,"BARCODE 1");
	    }

	/** Undo everything that's set until nothing left to change **/
	for(i=HTP_NUM_ATTRS-1;tmpattr;i--)
	    {
	    if (oldattr & (1<<attr_prec[i])) 
		{
		ps->WriteFn(ps->WriteArg, attr_off[attr_prec[i]], strlen(attr_off[attr_prec[i]]),0,0);
		}
	    tmpattr &= ~(1<<attr_prec[i]);
	    }

	/** Ok, now set everything that's set in the new mask. **/
	for(i++;i<HTP_NUM_ATTRS;i++)
	    {
	    if (attr & (1<<attr_prec[i]))
		{
		ps->WriteFn(ps->WriteArg, attr_on[attr_prec[i]], strlen(attr_on[attr_prec[i]]),0,0);
		}
	    }

	/** Set the new current attributes **/
	s->CurAttr = attr;

    return 0;
    }


/*** htpAlign - set vertical alignment to a certain column relative to the
 *** 'normal' pitch units (as in s->Cols)
 ***/
int
htpAlign(pPrtSession ps, double x)
    {
    pSession s = (pSession)(ps->PrivateData);
    int cnt;
    char buf[256];

    	/** Estimate cnt needed **/
	x = x + 0.00001;
	cnt = (x - s->CurCol)/htpGetCharWidth(ps,NULL,-1,-1);
	if (cnt <= 0) return 0;
	if (cnt > 255) 
	    {
	    mssError(1,"HTP","Column count for horizontal alignment too large");
	    return -1;
	    }

	/** Write spaces to equal cnt **/
	memset(buf, ' ', cnt);
	buf[cnt] = 0;
	htpWriteString(ps, buf, cnt);

    return 0;
    }


/*** htpGetAlign - return the current vertical alignment column.
 ***/
double
htpGetAlign(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);
    return s->CurCol;
    }


/*** htpComment - write a hidden comment into the html document.  This is
 *** used primarily for when the parser which converts html to fx or pcl
 *** needs additional information to drive the print management layer.
 ***/
int
htpComment(pPrtSession ps, char* comment)
    {
    pSession s = (pSession)(ps->PrivateData);

    	/** Build the comment string. **/
	sprintf(s->Buffer, "<!--HTP %s-->", comment);
	ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer), 0,0);

    return 0;
    }


/*** htpDoTable - begin a tabular structure in the output.  In HTML, this
 *** requires little intelligence because of the tables functionality
 *** in the browser.
 ***/
int
htpDoTable(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Are we already in a table? **/
	if (s->Flags & HTP_F_INTABLE) return -1;

	/** Write the table header **/
	ps->WriteFn(ps->WriteArg, "<TABLE border=0><TR>", 20,0,0);
	s->Flags |= HTP_F_INTABLE;
	s->Flags &= ~(HTP_F_INTABCOL | HTP_F_INTABHDR);

    return 0;
    }


/*** htpEndTable - finish up a table we started with htpDoTable.  This also
 *** finishes up the last column, if need be.
 ***/
int
htpEndTable(pPrtSession ps)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Are we not in a table? **/
	if (!(s->Flags & HTP_F_INTABLE)) return -1;

	/** Need to finish up a column or header cell? **/
	if (s->Flags & HTP_F_INTABCOL)
	    {
	    ps->WriteFn(ps->WriteArg, "</TD>", 5,0,0);
	    }
	if (s->Flags & HTP_F_INTABHDR)
	    {
	    ps->WriteFn(ps->WriteArg, "</TH>", 5,0,0);
	    }
	
	/** Write the table ending **/
	ps->WriteFn(ps->WriteArg, "</TR></TABLE>\n", 14,0,0);

	/** Reset some flags **/
	s->Flags &= 
	    ~(HTP_F_INTABLE|HTP_F_INTABCOL|HTP_F_INTABHDR);

    return 0;
    }


/*** htpDoColHdr - start a column header entry for a table that has
 *** already been started with htpDoTable.  Although not particularly
 *** useful for column headers, this fn is the same as DoColumn in that
 *** it has a second parameter for start new row.  Because HTML auto-
 *** formats its tables, we ignore the set_width setting.
 ***/
int
htpDoColHdr(pPrtSession ps, int start_row, int set_width)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Are we not in a table? **/
	if (!(s->Flags & HTP_F_INTABLE)) return -1;

	/** Are we in the body of the table already? **/
	if (s->Flags & HTP_F_INTABCOL) return -1;

	/** Need to end a previous header? **/
	if (s->Flags & HTP_F_INTABHDR)
	    {
	    ps->WriteFn(ps->WriteArg, "</TH>", 5,0,0);
	    }

	/** Write the header **/
	ps->WriteFn(ps->WriteArg, "<TH><TT>", 4,0,0);
	s->Flags |= HTP_F_INTABHDR;

    return 0;
    }


/*** htpDoColumn - start one cell within a row in a table.  The
 *** start-row parameter indicates whether to start a new row (1) or
 *** continue on the current row (0).
 ***/
int
htpDoColumn(pPrtSession ps, int start_row, int col_span)
    {
    pSession s = (pSession)(ps->PrivateData);

	/** Are we not in a table? **/
	if (!(s->Flags & HTP_F_INTABLE)) return -1;

	/** Time to end a header row? **/
	if (s->Flags & HTP_F_INTABHDR)
	    {
	    s->Flags &= ~HTP_F_INTABHDR;
	    sprintf(s->Buffer,"</TH></TR>\n<TR><TD COLSPAN=%d><TT>",col_span);
	    ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer), 0,0);
	    }
	else
	    {
	    if (s->Flags & HTP_F_INTABCOL)
		{
		if (start_row)
		    {
	            sprintf(s->Buffer,
			"</TD></TR>\n<TR><TD COLSPAN=%d><TT>",col_span);
		    ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer), 0,0);
		    }
		else
		    {
		    sprintf(s->Buffer, "</TD><TD COLSPAN=%d><TT>", col_span);
		    ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer), 0,0);
		    }
		}
	    else
		{
		sprintf(s->Buffer, "<TD COLSPAN=%d><TT>", col_span);
		ps->WriteFn(ps->WriteArg, s->Buffer, strlen(s->Buffer) ,0,0);
		}
	    }

	/** Set column status. **/
	s->Flags |= HTP_F_INTABCOL;

    return 0;
    }


/*** htpSetFont - change the current font in use.
 ***/
int
htpSetFont(pPrtSession ps, int font_id)
    {
    pSession s = (pSession)(ps->PrivateData);
    int oldattr;

    	/** Same font? **/
	if (font_id == s->CurFont) return 0;

    	/** Undo the current font settings **/
	oldattr = htpGetAttr(ps);
	htpSetAttr(ps,oldattr & RS_TX_CENTER);
	if (s->CurFont == RS_FONT_COURIER) ps->WriteFn(ps->WriteArg,"</TT>",5,0,0);
	else ps->WriteFn(ps->WriteArg,"</FONT>",7,0,0);

	/** Set the new font setting **/
	if (font_id == RS_FONT_COURIER) ps->WriteFn(ps->WriteArg,"<TT>",4,0,0);
	else if (font_id == RS_FONT_TIMES) ps->WriteFn(ps->WriteArg,"<FONT FACE=Times>",17,0,0);
	else if (font_id == RS_FONT_HELVETICA) ps->WriteFn(ps->WriteArg,"<FONT FACE=Helvetica>",21,0,0);

	/** Restore the attributes **/
	htpSetAttr(ps, oldattr);
	s->CurFont = font_id;

    return 0;
    }


/*** htpSetLPI - for HTML, this doesn't actually do anything.  Sigh.  Ignore.
 ***/
int
htpSetLPI(pPrtSession ps, int lines_per_inch)
    {
    return 0;
    }


/*** htpInitialize -- initialize this module, which causes it to register
 *** itself with the report server.
 ***/
int
htpInitialize()
    {
    pPrintDriver drv;

	/** Zero our globals **/
	/*memset(&HTP,0,sizeof(HTP));*/

	/** Allocate the descriptor structure **/
	drv = (pPrintDriver)nmMalloc(sizeof(PrintDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(PrintDriver));

	/** Fill out the structure **/
	strcpy(drv->Name, "HTP - HTML Content Driver");
	strcpy(drv->ContentType, "text/html");
	drv->Open = htpOpen;		/* Open a session */
	drv->Close = htpClose;		/* Close a session */
	drv->PageGeom = htpPageGeom;	/* Set page geometry */
	drv->WriteString = htpWriteString; /* Write a string of text */
	drv->WriteNL = htpWriteNL;	/* Output a new line (CR) */
	drv->WriteFF = htpWriteFF;	/* Output a "form feed" */
	drv->WriteLine = htpWriteLine;	/* Output a horizontal rule */
	drv->SetAttr = htpSetAttr;	/* Set attributes */
	drv->GetAttr = htpGetAttr;	/* Get current attr */
	drv->DoTable = htpDoTable;	/* Start a table */
	drv->DoColHdr = htpDoColHdr;	/* Start a column header */
	drv->DoColumn = htpDoColumn;	/* Start a column */
	drv->EndTable = htpEndTable;	/* Finish up the table */
	drv->Align = htpAlign;		/* set column alignment/hpos */
	drv->GetAlign = htpGetAlign;	/* get horizontal pos alignment */
	drv->Comment = htpComment;	/* write a hidden comment */
	drv->GetCharWidth = htpGetCharWidth;
	drv->SetFont = htpSetFont;
	drv->SetLPI = htpSetLPI;

	/** Now register with the report server **/
	prtRegisterPrintDriver(drv);

    return 0;
    }

