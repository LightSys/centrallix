#ifndef _STP_PARAM_H
#define _STP_PARAM_H

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
/* Module: 	st_param.h          					*/
/* Author:	Tim Young (TCY)  					*/
/* Creation:	February 18, 2000 					*/
/* Description:								*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: st_param.h,v 1.1 2001/08/13 18:00:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/st_param.h,v $

    $Log: st_param.h,v $
    Revision 1.1  2001/08/13 18:00:53  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "st_node.h"
#include "stparse.h"

/** Structure for storing protocol data. **/
typedef struct
    {
    pSnNode	Node;	      /* node file used when not in Override          */
    pObject	Obj;	      /* Pointer to parent Object		      */
    pStructInf	OverrideInf;  /* inf structure for Override                   */
    char*	AttrList[64]; /* list all attributes in both Node and Override*/
    int		CurrAttr;     /* index of Current attribute we are looking at */
    int		nSubInf;      /* Number of Infs, without duplication in both  */
    int		Version; 
    int		OpenCnt; /* track number of open connections                  */
    int		Status;  /* tag as dirty if you want to update original node  */
    }
    StpInf, *pStpInf;

pStpInf stpAllocInf(pSnNode node, pStructInf openctl, pObject obj, int DefaultVersion);
char* stpGetNextAttr(pStpInf inf);
char* stpGetFirstAttr(pStpInf inf);
pStructInf stpLookup(pStpInf inf, char* paramname);
pStructInf stpAddAttr(pStpInf inf, char* name);
int stpAttrValue(pStructInf attr,pObject obj,int* intval, char** strval, int nval);
int stpAttrType(pStpInf inf, char* name);
int stpSetAttrValue(pStpInf inf, char* attrname, void* val);
int stpUpdateNode(pStpInf inf);
int stpParsePathOverrides();
int stpMakeWhereTree();
int stpFreeInf(pStpInf);
pXString stpSubstParam(pStpInf inf, char* src);

#endif /* _STP_PARAM_H */
