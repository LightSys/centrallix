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
void prt_htmlfm_SetKeepSpaces(context);

const char * prt_htmlfm_GetFont(pPrtTextStyle style);
int prt_htmlfm_InitStyle(pPrtHTMLfmInf context, pPrtTextStyle initial_style);
int prt_htmlfm_SetStyle(pPrtHTMLfmInf context, pPrtTextStyle newstyle);
int prt_htmlfm_WriteStyle(pPrtHTMLfmInf context);
int prt_htmlfm_EndStyle(pPrtHTMLfmInf context);

int prt_htmlfm_Border(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj);
int prt_htmlfm_EndBorder(pPrtHTMLfmInf context, pPrtBorder border, pPrtObjStream obj);


/** Component generator entry points.  **/
int prt_htmlfm_GenerateArea(pPrtHTMLfmInf context, pPrtObjStream area);
int prt_htmlfm_GenerateTable(pPrtHTMLfmInf context, pPrtObjStream table);
int prt_htmlfm_GenerateMultiCol(pPrtHTMLfmInf context, pPrtObjStream section);


#endif /* not defined _PRTMGMT_V3_FM_HTML_H */

