#ifndef _PRTMGMT_H
#define _PRTMGMT_H

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
/* Module: 	prtmgmt.c,prtmgmt.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 9, 1999   					*/
/* Description:	Provides the general non-device-specific printing 	*/
/*		functionality for the reporting output mechanism.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt.h,v 1.1 2001/08/13 18:00:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt.h,v $

    $Log: prtmgmt.h,v $
    Revision 1.1  2001/08/13 18:00:53  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "report.h"
#include "barcode.h"


/** Structure for a print command **/
typedef struct _PC
    {
    struct _PC*		Next;
    struct _PC*		Prev;
    int			CmdType;
    double		StartLineX;
    double		XLength;
    int			CurRelLine;
    int			IntArgs[4];
    char*		StringArgs[3];
    unsigned char	StringAlloc[3];
    int			CurAttr;
    unsigned char	CurFont;
    unsigned char	Flags;
    unsigned char	LinkCnt;
    }
    PrtCommand, *pPrtCommand;

#define PRT_CMD_F_PROCESSED	1
#define PRT_CMD_F_INTABLE	2
#define PRT_CMD_F_NOOUTPUT	4


/** Cmd types -- bitmask for use in "PendingCmds" **/
#define	PRT_CMD_WRITESTRING	1
#define PRT_CMD_WRITENL		2
#define PRT_CMD_WRITEFF		4
#define PRT_CMD_WRITELINE	8
#define PRT_CMD_ATTRON		16
#define PRT_CMD_ATTROFF		32
#define PRT_CMD_DOTABLE		64
#define PRT_CMD_DOCOLHDR	128
#define PRT_CMD_DOCOLUMN	256
#define PRT_CMD_ENDTABLE	512
#define PRT_CMD_SETHPOS		1024
#define PRT_CMD_SETLINESPACING	2048
#define PRT_CMD_AUTOWRAP	4096
#define PRT_CMD_SETCOLS		8192
#define PRT_CMD_SETMARGINLR	16384
#define PRT_CMD_SETMARGINTB	32768
#define PRT_CMD_TABLESEP	65536

#define PRT_T_F_NOTITLE		1	/* Do not emit the titlebar */
#define PRT_T_F_NORGTHDR	2	/* Do not right-align any col hdrs */
#define PRT_T_F_RELCOLWIDTH	4	/* Column widths are relative to text */
#define PRT_T_F_LOWERSEP	8	/* generate lower separator too. */


/** Printing session **/
typedef struct _PS
    {
    pPrintDriver	Driver;
    pPrtCommand		CmdStream;
    pPrtCommand		CmdStreamTail;
    pPrtCommand		CmdStreamLinePtr;
    pPrtCommand		CmdStreamTablePtrs[16];
    int			nCmdStreamTablePtrs;
    int			CmdStreamActiveTables[16];
    int			(*WriteFn)();
    void*		WriteArg;
    int			ExecCnt;
    int			CurAttr;
    int			CurFont;
    int			PageX;
    int			PageY;
    double		CurPitch;
    double		CurWrapWidth;
    double		CurClipWidth;
    int 		CurMaxLines;
    double		CurLineX;
    double		CurPageY;
    int			CurRelY;
    int			LinesPerInch;
    int			CurTableCol;
    int			ColRoomLeft;
    int			nTableColumns;
    int			TableColWidth[64];
    int			ColTitleOffset[64];
    char		ColTitles[512];
    int			ColTitleLen;
    char		ColHdr[256];
    char		Buffer[256];
    char		BCBuf[96];
    int			LMargin;
    int			RMargin;
    int			TMargin;
    int			BMargin;
    int			Columns;
    int			ColSep;
    int			ColFirstWidth;
    int			PageNum;
    int			(*(HdrFn[16]))();
    void*		HdrArg[16];
    unsigned char	HdrLines[16];
    unsigned char	TotalHdrLines;
    unsigned char	HdrCnt;
    int			(*(FtrFn[16]))();
    void*		FtrArg[16];
    unsigned char	FtrLines[16];
    unsigned char	TotalFtrLines;
    unsigned char	FtrCnt;
    void*		PrivateData;
    }
    PrtSession, *pPrtSession;


/** Session Functions **/
pPrtSession prtOpenSession(char* content_type, int (*write_fn)(), void* write_arg, int flags);
int prtCloseSession(pPrtSession this);

/** Write-data functions **/
int prtWriteString(pPrtSession this, char* text, int len);
int prtWriteNL(pPrtSession this);
int prtWriteFF(pPrtSession this);
int prtWriteLine(pPrtSession this);

/** Attribute functions **/
int prtSetAttr(pPrtSession this, int new_attr);
int prtGetAttr(pPrtSession this);
int prtSetFont(pPrtSession this, char* font_name);
char* prtGetFont(pPrtSession this);

/** Table functions **/
int prtDoTable(pPrtSession this, int flags, int colsep);
int prtDoColHdr(pPrtSession this, int start_row, int set_width, int flags);
int prtDoColumn(pPrtSession this, int start_row, int set_width, int col_span, int flags);
int prtEndTable(pPrtSession this);
int prtDisengageTable(pPrtSession this);
int prtEngageTable(pPrtSession this);

/** Page Layout functions **/
int prtSetPageGeom(pPrtSession this, int x, int y);
int prtSetColumns(pPrtSession this, int n_columns, int col_sep, int first_width);
int prtSetLRMargins(pPrtSession this, int l_margin, int r_margin);
int prtSetTBMargins(pPrtSession this, int t_margin, int b_margin);
int prtSetLineSpacing(pPrtSession this, double lines_per_inch);
double prtGetLineSpacing(pPrtSession this);
int prtSetHPos(pPrtSession this, double x);
int prtSetVPos(pPrtSession this, double y);
int prtSetRelVPos(pPrtSession this, double y);
double prtGetRelVPos(pPrtSession this);
int prtGetLRMargins(pPrtSession this, int* l_margin, int* r_margin);
int prtGetTBMargins(pPrtSession this, int* t_margin, int* b_margin);
int prtGetColumns(pPrtSession this, int* n_columns, int* col_sep);
int prtSetHeader(pPrtSession this, int (*hdr_fn)(), void* arg, int lines);
int prtRemoveHeader(pPrtSession this, void* arg);
int prtSetFooter(pPrtSession this, int (*ftr_fn)(), void* arg, int lines);
int prtRemoveFooter(pPrtSession this, void* arg);
int prtSetPageNumber(pPrtSession this, int new_page_num);
int prtGetPageNumber(pPrtSession this);

/** HTML-to-printdrv conversion functions **/
int prtConvertHTML(int (*read_fn)(), void* read_arg, pPrtSession sess);


#endif /* not defined _PRTMGMT_H */

