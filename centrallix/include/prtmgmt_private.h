#ifndef _PRTMGMT_PRIVATE_H
#define _PRTMGMT_PRIVATE_H

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
/*									*/
/*		File: prtmgmt_private.h - provides internal prtmgmt	*/
/*		definitions that aren't visible from the exterior.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: prtmgmt_private.h,v 1.2 2001/09/25 18:03:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt_private.h,v $

    $Log: prtmgmt_private.h,v $
    Revision 1.2  2001/09/25 18:03:44  gbeeley
    A few changes for the upcoming prtmgmt update.

    Revision 1.1.1.1  2001/08/13 18:00:53  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "report.h"
#include "barcode.h"
#include "prtmgmt_new.h"

#define PRT_MAX_LAYOUTDRV	32

#define PRT_LOC_UNSET		(-65536)
#define PRT_LOC_ISUNSET(x)	((x) < -65535)

/*** Global structure ***/
typedef struct
    {
    XArray		Drivers;
    pPrtLayoutDriver	LayoutDrivers[PRT_MAX_LAYOUTDRV];
    }
    PRT_INF_t;

extern PRT_INF_t PRT_INF;

/*** prtmgmt_main ***/
pPrtObjStream prt_internal_AllocOS(int type);
int prt_internal_FreeOS(pPrtObjStream element);
pPrtObjStream prt_internal_CopyOS(pPrtObjStream element);
pPrtObjStream prt_internal_CopyStream(pPrtObjStream stream, pPrtObjStream new_parent);
int prt_internal_CopyStyle(pPrtObjStream src, pPrtObjStream dst);
int prt_internal_FreeStream(pPrtObjStream element_stream);
int prt_internal_AddOS(pPrtObjStream parent, pPrtObjStream *streamhead, pPrtObjStream stream, pPrtObjStream new_element);
int prt_internal_RemoveOS(pPrtObjStream *streamhead, pPrtObjStream rm_element);
pPrtSession prt_internal_AllocSession();
int prt_internal_FreeSession(pPrtSession this);
int prt_internal_DumpStream_r(pPrtObjStream stream, int indent);
int prtDumpStream(pPrtObjStream stream, int indent);
int prtRegisterLayoutDriver(pPrtLayoutDriver ldrv);

/*** prtmgmt_layout ***/
int prt_internal_CheckPageBreak(pPrtSession this);
int prt_internal_PageBreak(pPrtSession this);
double prt_internal_DetermineTextWidth(pPrtSession this, char* text, int len);
int prt_internal_CheckTextFlow(pPrtSession this, pPrtObjStream element);
double prt_internal_DetermineYIncrement(pPrtSession this, pPrtObjStream element);

#endif /* ndef _PRTMGMT_PRIVATE_H */
