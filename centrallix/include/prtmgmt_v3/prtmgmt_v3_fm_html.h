#ifndef _PRTMGMT_V3_FM_HTML_H
#define _PRTMGMT_V3_FM_HTML_H

#include "prtmgmt_v3/prtmgmt_v3.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_fm_html.h                                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 9th, 2003                                         */
/*									*/
/* Description:	This module provides the HTML formatter functionality	*/
/*		for the v3 print management subsystem.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_fm_html.h,v 1.1 2003/04/21 21:00:51 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt_v3/prtmgmt_v3_fm_html.h,v $

    $Log: prtmgmt_v3_fm_html.h,v $
    Revision 1.1  2003/04/21 21:00:51  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

 **END-CVSDATA***********************************************************/


#define PRT_HTMLFM_FONTSIZE_OFFSET      (+1)
#define PRT_HTMLFM_FONTSIZE_DEFAULT     (12)

#define PRT_HTMLFM_MAX_TABSTOP          (32)
#define PRT_HTMLFM_XPIXEL               (7)
#define PRT_HTMLFM_YPIXEL               (12)


/** incomplete struct def'n - don't need whole thing here **/
typedef struct _PSFI PrtHTMLfmInf, *pPrtHTMLfmInf;


/** Component generator support functions **/
int prt_htmlfm_Output(pPrtHTMLfmInf context, char* str, int len);
int prt_htmlfm_OutputPrintf(pPrtHTMLfmInf context, char* fmt, ...);
int prt_htmlfm_OutputEncoded(pPrtHTMLfmInf context, char* str, int len);

int prt_htmlfm_Generate_r(pPrtHTMLfmInf context, pPrtObjStream obj);

int prt_htmlfm_SaveStyle(pPrtHTMLfmInf context, pPrtTextStyle origstyle);
int prt_htmlfm_ResetStyle(pPrtHTMLfmInf context, pPrtTextStyle origstyle);

int prt_htmlfm_InitStyle(pPrtHTMLfmInf context, pPrtTextStyle initial_style);
int prt_htmlfm_SetStyle(pPrtHTMLfmInf context, pPrtTextStyle newstyle);
int prt_htmlfm_EndStyle(pPrtHTMLfmInf context);

int prt_htmlfm_Border(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj);
int prt_htmlfm_EndBorder(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj);


/** Component generator entry points.  **/
int prt_htmlfm_GenerateArea(pPrtHTMLfmInf context, pPrtObjStream area);
int prt_htmlfm_GenerateTable(pPrtHTMLfmInf context, pPrtObjStream table);
int prt_htmlfm_GenerateMultiCol(pPrtHTMLfmInf context, pPrtObjStream section);


#endif /* not defined _PRTMGMT_V3_FM_HTML_H */

