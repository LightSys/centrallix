#ifndef _PRTMGMT_V3_LM_TABLE_H
#define _PRTMGMT_V3_LM_TABLE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001-2003 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_lm_table.h                                   */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 11th, 2003                                        */
/*									*/
/* Description:	The lm_table module provides intelligence about the	*/
/*		layout procedures for tabular data (rows+cols).		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_lm_table.h,v 1.1 2003/04/21 21:00:51 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt_v3/prtmgmt_v3_lm_table.h,v $

    $Log: prtmgmt_v3_lm_table.h,v $
    Revision 1.1  2003/04/21 21:00:51  gbeeley
    HTML formatter additions including image, table, rectangle, multi-col,
    fonts and sizes, now supported.  Rearranged header files for the
    subsystem so that LMData (layout manager specific info) can be
    shared with HTML formatter subcomponents.

 **END-CVSDATA**********************************************************/


#define PRT_TABLM_MAXCOLS               256     /* maximum columns in a table */

#define PRT_TABLM_F_ISHEADER            1       /* row is a header that repeats */
#define PRT_TABLM_F_ISFOOTER            2       /* row is a repeating footer */
#define PRT_TABLM_F_INNEROUTER          4       /* user inner/outer bdr instead of l/r/t/b */

#define PRT_TABLM_DEFAULT_FLAGS         (0)
#define PRT_TABLM_DEFAULT_COLSEP        1.0     /* column separation */


/*** table specific data ***/
typedef struct _PTB
    {
    double              ColWidths[PRT_TABLM_MAXCOLS];
    double              ColX[PRT_TABLM_MAXCOLS];
    double              ColSep;
    int                 nColumns;       /* number of columns in table */
    int                 CurColID;       /* next cell inserted is this col. */
    pPrtObjStream       HeaderRow;      /* row that is the table header */
    pPrtObjStream       FooterRow;      /* table footer row */
    int                 Flags;
    PrtBorder           TopBorder;
    PrtBorder           BottomBorder;
    PrtBorder           LeftBorder;
    PrtBorder           RightBorder;
    PrtBorder           InnerBorder;    /* used only by cells in lieu of others */
    PrtBorder           OuterBorder;    /* used only by cells in lieu of others */
    PrtBorder           Shadow;         /* only valid on table as a whole */
    double              ShadowWidth;    /* overall width of the shadow */
    }
    PrtTabLMData, *pPrtTabLMData;



#endif /* not defined _PRTMGMT_V3_LM_TABLE_H */


