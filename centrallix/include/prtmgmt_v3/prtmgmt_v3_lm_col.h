#ifndef _PRTMGMT_V3_LM_COL_H
#define _PRTMGMT_V3_LM_COL_H

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
/* Module:	prtmgmt.c,prtmgmt.h                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	December 12, 2001                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/************************************************************************/



#define PRT_COLLM_MAXCOLS	16	/* maximum allowed columns */
#define PRT_COLLM_DEFAULT_COLS	1	/* default is 1 column */
#define PRT_COLLM_DEFAULT_SEP	1.0	/* default column separation = 1.0 unit or 0.1 inch */

#define PRT_COLLM_F_BALANCED	1	/* content balances between cols */
#define PRT_COLLM_F_SOFTFLOW	2	/* content can flow from one col to next */

#define PRT_COLLM_DEFAULT_FLAGS	(PRT_COLLM_F_SOFTFLOW)


/*** multicolumn area specific data ***/
typedef struct _PMC
    {
    int		Flags;
    int		nColumns;
    double	ColSep;
    double	ColWidths[PRT_COLLM_MAXCOLS];
    PrtBorder	Separator;		/* line separator between columns */
    pPrtObjStream CurrentCol;
    }
    PrtColLMData, *pPrtColLMData;


#endif /* not defined _PRTMGMT_V3_LM_COL_H */


