#ifndef _STPARSE_NE_H
#define _STPARSE_NE_H

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
/* Module: 	stparse_new.c, stparse_new.h 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	Parser to handle the request data stream from the end-	*/
/*		user.  Uses the MTLEXER module.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: stparse_ne.h,v 1.1 2001/10/16 23:53:01 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/stparse_ne.h,v $

    $Log: stparse_ne.h,v $
    Revision 1.1  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#include "mtask.h"
#include "mtlexer.h"

/** Non-expression structured data storage **/
typedef struct _SO
    {
    char	    Name[64];
    char*	    StrVal;
    int		    StrAlloc;
    struct _SO*	    SubInf[64];
    struct _SO*	    Parent;
    int		    nSubInf;
    int		    Type;
    }
    Struct, *pStruct;

#ifndef ST_T_STRUCT
#define ST_T_STRUCT 	0
#define ST_T_ATTRIB	1
#define ST_T_SUBGROUP	2
#define ST_T_SCRIPT	3
#endif

/*** functions added for non-expression structured data ***/
pStruct stCreateStruct_ne(char* name);
pStruct stAddAttr_ne(pStruct inf, char* name);
pStruct stAddGroup_ne(pStruct inf, char* name);
int stAddValue_ne(pStruct inf, char* strval);
pStruct stLookup_ne(pStruct inf, char* name);
int stAttrValue_ne(pStruct inf, char** strval);
pStruct stAllocInf_ne();
int stFreeInf_ne(pStruct inf);
int stAddInf_ne(pStruct main_inf, pStruct sub_inf);

#endif /* _STPARSE_NE_H */

