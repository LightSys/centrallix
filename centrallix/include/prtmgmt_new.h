#ifndef _PRTMGMT_NEW_H
#define _PRTMGMT_NEW_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_new.c, prtmgmt_new.h				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 13th, 2000					*/
/* Description:	This module replaces the old prtmgmt module, and 	*/
/*		provides print management and layout services, mainly	*/
/*		for the reporting system.  This new module includes	*/
/*		additional features, including the ability to do 	*/
/*		shading, colors, and raster graphics in reports, and	*/
/*		to hpos/vpos anywhere on the page during page layout.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_new.h,v 1.2 2001/09/25 18:03:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt_new.h,v $

    $Log: prtmgmt_new.h,v $
    Revision 1.2  2001/09/25 18:03:44  gbeeley
    A few changes for the upcoming prtmgmt update.

    Revision 1.1.1.1  2001/08/13 18:00:53  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "report.h"
#include "barcode.h"


#define PRT_SECTION_MAX_COLS	8
#define PRT_TABLE_MAX_COLS	32
#define PRT_MAX_HEADERS		4
#define PRT_MAX_FOOTERS		4

#define XEND(p) ((p)->RelativeX + (p)->Width)
#define YEND(p) ((p)->RelativeY + (p)->Height)


/*** printing layout driver structure ***/
typedef struct _PLD
    {
    char		Name[64];
    int			ObjType;	/* PRT_OS_T_xxx */
    int			IsContainer;	/* 1 if container, 0 if not */
    int			(*Init)();
    int			(*Release)();
    int			(*Add)();
    }
    PrtLayoutDriver, *pPrtLayoutDriver;

/*** object-stream structure ***/
typedef struct _POS
    {
    unsigned int	Magic; 		/* MGK_PRTOBJSTRM  0x12340502 */
    pPrtLayoutDriver	Driver;
    struct _POS*	Prev;
    struct _POS*	Next;
    struct _POS**	Head;
    struct _POS*	Parent;
    struct _POS*	YPrev;
    struct _POS*	YNext;
    int			Type;
    int			Attributes;	/* RS_TX_xxx */
    int			FGColor;
    int			BGColor;
    unsigned char	Font;		/* RS_FONT_xxx */
    unsigned char	FontSize;
    unsigned char	Justification;	/* PRT_JST_xxx */
    unsigned short	Flags;		/* PRT_OS_F_xxx */
    double		LinesPerInch;
    double		SetRelX;
    double		SetRelY;
    double		RelativeX;
    double		RelativeY;
    double		AbsoluteX;
    double		AbsoluteY;
    double		Width;
    double		Height;
    double		AvailablePageHeight;
    double		AvailablePageWidth;
    double		ClipHeight;
    double		ClipWidth;
    union
        {
	struct
	    {
	    int		Length;
	    char*	Text;		/* can be NULL for empty entry */
	    }
	    String;
	struct
	    {
	    int		nColumns;
	    int		TableFlags;	/* PRT_T_F_xxx */
	    double	ColSep;
	    XArray	ColWidths;
	    struct _POS* Rows;		/* normally, sequence of rows */
	    struct _POS* HdrRow;		/* convenience reference to hdr row */
	    }
	    Table;
	struct
	    {
	    struct _POS* Columns;	/* sequence of columns */
	    int		IsHdrRow;	/* 1=header row, 0=data row */
	    }
	    TableRow;
	struct
	    {
	    int		ColSpan;	/* number of columns used by this */
	    int		ColID;		/* position of this column */
	    struct _POS* Content;	/* data in this table cell */
	    }
	    TableCell;
	struct
	    {
	    double	LMargin;
	    double	RMargin;
	    int		nColumns;
	    double	ColWidth;
	    double	ColSep;
	    struct _POS* InitialStream;
	    struct _POS* Streams[PRT_SECTION_MAX_COLS];
	    struct _POS* Header;
	    int		HdrEachCol;
	    }
	    Section;
	struct
	    {
	    struct _POS* Content;
	    int			(*(HdrFn[PRT_MAX_HEADERS]))();
	    void*		HdrArg[PRT_MAX_HEADERS];
	    unsigned char	HdrLines[PRT_MAX_HEADERS];
	    unsigned char	TotalHdrLines;
	    unsigned char	HdrCnt;
	    int			(*(FtrFn[PRT_MAX_FOOTERS]))();
	    void*		FtrArg[PRT_MAX_FOOTERS];
	    unsigned char	FtrLines[PRT_MAX_FOOTERS];
	    unsigned char	TotalFtrLines;
	    unsigned char	FtrCnt;
	    }
	    Page;
	struct
	    {
	    double	XPos;
	    double	YPos;
	    double	Width;
	    double	Height;
	    double	TextOffset;
	    struct _POS* Content;
	    }
	    Area;
	struct
	    {
	    /** Note about geometry settings - the VisualWidth is how wide
	     ** the picture appears on the page.  The PixelWidth is how many
	     ** pixels across the VisualWidth should be.  The DataPixelWidth
	     ** is how wide the actual image data is.  If PixelWidth is not
	     ** DataPixelWidth, the image is tiled PixelWidth/DataPixelWidth
	     ** number of times.  Scaling of PixelWidth into VisualWidth is
	     ** automatic.
	     **/
	    double	XPos;
	    double	YPos;
	    double	VisualWidth;		/* visual = 10ths of an inch */
	    double	VisualHeight;
	    int		PixelWidth;		/* pixel = total image pixels visually */
	    int		PixelHeight;
	    int		DataPixelWidth;		/* data = image pixels in the data */
	    int		DataPixelHeight;
	    double	CurHeightOffset;
	    double	TextOffset;
	    int		ColorMode;		/* PRT_IMG_xxx */
	    int		DataLength;
	    int		Flags;			/* PRT_IMG_F_xxx */
	    unsigned char* Data;
	    }
	    Picture;
	}
	Object;
    }
    PrtObjStream, *pPrtObjStream;


/*** Printing session structure. ***/
typedef struct
    {
    unsigned int	Magic;			/* MGK_PRTOBJSSN - 0x1234058e */
    pPrintDriver	Driver;
    void*		PrivateData;
    pPrtObjStream	ObjStreamHead;		/* top of objstream tree */
    pPrtObjStream	ObjStreamPtr;		/* points to current element */
    int			(*WriteFn)();
    void*		WriteArg;
    int			PageNum;
    double		PageWidth;
    double		PageHeight;
    }
    PrtSession, *pPrtSession;


/*** objectstream types ***/
#define PRT_OS_T_STRING		1
#define PRT_OS_T_TABLE		2
#define PRT_OS_T_TABLEROW	3
#define PRT_OS_T_TABLECELL	4
#define PRT_OS_T_SECTION	5
#define PRT_OS_T_AREA		6
#define PRT_OS_T_PICTURE	7	/* graphics image */
#define PRT_OS_T_PAGE		8	/* top level page */

/*** Table flags ***/
#define PRT_T_F_NOTITLE         1       /* Do not emit the titlebar */
#define PRT_T_F_NORGTHDR        2       /* Do not right-align any col hdrs */
#define PRT_T_F_RELCOLWIDTH     4       /* Column widths are relative to text */
#define PRT_T_F_LOWERSEP        8       /* generate lower separator too. */

/*** Print object stream flags ***/
#define PRT_OS_F_POSITIONED	1	/* element was positioned manually */
#define PRT_OS_F_SETJUSTIFY	2	/* justification change with this cmd */
#define PRT_OS_F_ATTRCHANGED	4	/* attrs changed with this cmd */
#define PRT_OS_F_CLIPWIDTH	8	/* don't wrap; clip instead */
#define PRT_OS_F_CLIPHEIGHT	16	/* don't wrap; clip instead */
#define PRT_OS_F_BREAK		32	/* this is a paragraph break point. */
#define PRT_OS_F_INFLOW		64	/* in-flow rather than absolute. */
#define PRT_OS_F_FLOWAROUND	128	/* have text flow around this element */

/*** Font selection ***/
#define PRT_FONT_COURIER	(RS_FONT_COURIER)
#define PRT_FONT_TIMES		(RS_FONT_TIMES)
#define PRT_FONT_HELVETICA	(RS_FONT_HELVETICA)

/*** Style-attribute flags ***/
#define PRT_TX_BOLD		(RS_TX_BOLD)
#define PRT_TX_EXPANDED		(RS_TX_EXPANDED)
#define PRT_TX_COMPRESSED	(RS_TX_COMPRESSED)
#define PRT_TX_CENTER		(RS_TX_CENTER)		/* deprecated! */
#define PRT_TX_UNDERLINE	(RS_TX_UNDERLINE)
#define PRT_TX_ITALIC		(RS_TX_ITALIC)
#define PRT_TX_PBARCODE		(RS_TX_PBARCODE)
#define PRT_TX_ALL		(RS_TX_ALL)

/*** Justification modes ***/
#define PRT_JST_LEFT		0
#define PRT_JST_CENTER		1
#define PRT_JST_RIGHT		2
#define PRT_JST_FULL		3

/*** Picture color modes ***/
#define PRT_IMG_BINARY		0	/* black/white bitmap, 1bpp */
#define PRT_IMG_GREY		1	/* greyscale, 8bpp */
#define PRT_IMG_LOWCOLOR	2	/* xxRRGGBB, 8bpp */
#define PRT_IMG_HIGHCOLOR	3	/* RGB, 24bpp */

/*** Picture flags PRT_IMG_F_xxx ***/




/*** The following are the functions for the prtmgmt API.  Most of these work
 *** within a session context.  A printing session defines the destination
 *** for the printed output (whether pFile or pObject, using fdWrite or
 *** objWrite), as well as defining what the content-type of the output will
 *** be (such as text/html, text/plain, etc).
 ***/

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
int prtSetFontSize(pPrtSession this, int point_size);
int prtGetFontSize(pPrtSession this);
int prtSetColor(pPrtSession this, int fgcolor);
int prtGetColor(pPrtSession this);

/** Flow and layout commands **/
int prtSetJustification(pPrtSession this, int justification);
int prtGetJustification(pPrtSession this);
int prtSetLineSpacing(pPrtSession this, double lines_per_inch);
double prtGetLineSpacing(pPrtSession this);
int prtSetPosition(pPrtSession this, double x, double y);

/** Layout object commands **/
int prtBeginSection(pPrtSession this, int ncols, double lmargin, double rmargin, double colsep, int hdr_each_col);
int prtBeginSectionBody(pPrtSession this);
int prtEndSection(pPrtSession this);

int prtBeginArea(pPrtSession this, double x, double y, double width, double height, double offset, int flags);
int prtEndArea(pPrtSession this);

int prtBeginTable(pPrtSession this, int flags, int n_columns, double colsep);
int prtEndTable(pPrtSession this);
int prtBeginRow(pPrtSession this, int is_hdr_row);
int prtDoColumn(pPrtSession this, double width, double x, int colspan);
int prtEndRow(pPrtSession this);

int prtWritePicture(pPrtSession this, double x, double y, double width, double height, double offset,
			int pixel_width, int pixel_height, int data_pixel_width, int data_pixel_height,
			int colormode, int datalength, int flags, unsigned char* data);

/** Header/footer commands **/
int prtSetHeader(pPrtSession this, int (*hdr_fn)(), void* arg, int lines);
int prtRemoveHeader(pPrtSession this, void* arg);
int prtSetFooter(pPrtSession this, int (*ftr_fn)(), void* arg, int lines);
int prtRemoveFooter(pPrtSession this, void* arg);

/** Misc commands **/
int prtSetPageNumber(pPrtSession this, int new_page_num);
int prtGetPageNumber(pPrtSession this);
int prtSetPageGeom(pPrtSession this, int x, int y);
int prtGetPageGeom(pPrtSession this, int* x, int* y);

/** HTML-to-printdrv conversion functions **/
int prtConvertHTML(int (*read_fn)(), void* read_arg, pPrtSession sess);


#endif /* not defined _PRTMGMT_NEW_H */
