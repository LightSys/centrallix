#ifndef _PRTMGMT_V3_PRIVATE_H
#define _PRTMGMT_V3_PRIVATE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001-2026 LightSys Technology Services, Inc.		*/
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
/* Module:	prtmgmt_v3_private.h, prtmgmt_v3_*.c			*/
/* Author:	Israel Fuller						*/
/* Date:	September 2nd, 2026					*/
/* 									*/
/* Description:	Internal declarations shared between the version-3	*/
/* 		print management sources.  These are not a part of	*/
/* 		the public print management interface.			*/
/************************************************************************/


#include "prtmgmt_v3/prtmgmt_v3.h"


/** Internal session functions **/
int prt_internal_NoZ(pPrtSession s);

/** Internal management functions **/
pPrtObjStream prt_internal_AllocObj(char* type);
pPrtObjStream prt_internal_AllocObjByID(int type_id);
int prt_internal_FreeObj(pPrtObjStream obj);
int prt_internal_Add(pPrtObjStream parent, pPrtObjStream new_child);
int prt_internal_Insert(pPrtObjStream sibling, pPrtObjStream new_obj);
int prt_internal_CopyAttrs(pPrtObjStream src, pPrtObjStream dst);
int prt_internal_CopyGeom(pPrtObjStream src, pPrtObjStream dst);
double prt_internal_GetFontHeight(pPrtObjStream obj);
double prt_internal_FontToLineHeight(pPrtObjStream obj);
double prt_internal_GetFontBaseline(pPrtObjStream obj);
double prt_internal_GetStringWidth(pPrtObjStream obj, char* str, int n);
pPrtObjStream prt_internal_YSort(pPrtObjStream obj);
int prt_internal_AddYSorted(pPrtObjStream obj, pPrtObjStream newobj);
int prt_internal_FreeTree(pPrtObjStream obj);
int prt_internal_GeneratePage(pPrtSession s, pPrtObjStream page);
pPrtObjStream prt_internal_GetPage(pPrtObjStream obj);
pPrtObjStream prt_internal_AddEmptyObj(pPrtObjStream container);
pPrtObjStream prt_internal_CreateEmptyObj(pPrtObjStream container);
int prt_internal_Dump(pPrtObjStream obj);
pPrtObjStream prt_internal_Duplicate(pPrtObjStream obj, int with_content);
int prt_internal_AdjustOpenCount(pPrtObjStream obj, int adjustment);
int prt_internal_Reflow(pPrtObjStream obj);
int prt_internal_ScheduleEvent(pPrtSession s, pPrtObjStream target, int type, void* param);
int prt_internal_DispatchEvents(pPrtSession s);
int prt_internal_MakeBorder(pPrtObjStream parent, double x, double y, double len, int flags, pPrtBorder b, pPrtBorder sb, pPrtBorder eb);
int prt_internal_GetPixel(pPrtImage img, double xoffset, double yoffset);
int prt_internal_GetPixelDirect(pPrtImage img, int x, int y);

/** Internal image and graphics functions **/
int prt_internal_WriteImageToPNG(int (*write_fn)(), void* write_arg, pPrtImage img, int w, int h);
int prt_internal_WriteSvgToFile(int (*write_fn)(), void* write_arg, pPrtSvg svg, int w, int h);

#endif /* defined _PRTMGMT_V3_PRIVATE_H */
