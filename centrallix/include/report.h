#ifndef _REPORT_H
#define _REPORT_H

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
/* Module:      prtmgmt.c,prtmgmt.h                                     */
/* Author:      Greg Beeley (GRB)                                       */
/* Creation:    October 16, 1998                                        */
/* Description: Provides high-level management of the various printer   */
/*              drivers as well as performing operations such as page   */
/*              management, headers, etc. that are common functionality */
/*              across the print drivers.                               */
/************************************************************************/



/** Printer Driver Information **/
typedef struct _PDR
    {
    char                Name[40];       /* Name of print driver */
    char		ContentType[64]; /* Content type. */
    int                 (*Open)();      /* Open a print session */
    int                 (*Close)();     /* Close the print session */
    int                 (*PageGeom)();  /* Gets page geometry (size etc) */
    int                 (*WriteString)(); /* Function to write a text string */
    int                 (*WriteNL)();   /* Function to go to next line */
    int                 (*WriteFF)();   /* Function to do a form feed */
    int                 (*WriteLine)(); /* Writes a horiz. line across pg. */
    int                 (*SetAttr)();   /* Sets attributes RS_TX_xxx */
    int                 (*GetAttr)();   /* Gets current attributes */
    int                 (*DoTable)();   /* Function to start a table */
    int                 (*DoColHdr)();  /* Function to write a col. hdr */
    int                 (*DoColumn)();  /* Function to write a column */
    int                 (*EndTable)();  /* Finishes up a table */
    int                 (*Align)();     /* Set horizontal align to column. */
    double              (*GetCharWidth)();  /* Return current font's pitch */
    double		(*GetAlign)();	/* Get horizontal align */
    int			(*Comment)();	/* Insert a hidden comment into output */
    int			(*SetFont)();	/* Set font -- see RS_FONT_xxx below */
    int			(*SetLPI)();	/* Set lines-per-inch */
    }
    PrintDriver, *pPrintDriver;

int prtRegisterPrintDriver(pPrintDriver drv);

#define RS_MAX_CONCURRENT       16      /* Maximum concurrent queries */
#define RS_TX_BOLD              1       /* Bold text */
#define RS_TX_EXPANDED          2       /* Expanded text */
#define RS_TX_COMPRESSED        4       /* Compressed text */
#define RS_TX_CENTER            8       /* Centered (instead of left) */
#define RS_TX_UNDERLINE         16      /* Underlined */
#define RS_TX_ITALIC            32      /* Italic */
#define RS_TX_PBARCODE          64      /* Postal service bar code */
#define RS_TX_ALL               127     /* All above attributes */

#define RS_COL_RIGHTJUSTIFY	1	/* right-justify the column */
#define RS_COL_CENTER		2	/* center the column */

#define RS_FONT_COURIER		0
#define RS_FONT_TIMES		1
#define RS_FONT_HELVETICA	2

#endif /* not defined _REPORT_H */
