#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "mtask.h"
#include "mtsession.h"
#include "obj.h"
#include "stparse.h"
#include "hints.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	hints.c, hints.h					*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	March 10, 2003						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: hints.c,v 1.1 2003/03/10 15:41:47 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/hints.c,v $

    $Log: hints.c,v $
    Revision 1.1  2003/03/10 15:41:47  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.


 **END-CVSDATA***********************************************************/


/*** objInfToHints - converts a StructInfo structure tree to a presentation
 *** hints structure.  The StructInfo should be the format of a
 *** system/parameter.
 ***/
pObjPresentationHints
objInfToHints(pStructInf inf, int data_type)
    {
    pObjPresentationHints ph;
    pParamObjects tmplist;
    char* ptr;
    char* newptr;
    int n,cnt;

    	/** Allocate a new ph structure **/
	ph = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
	memset(ph,0,sizeof(ObjPresentationHints));
	xaInit(&(ph->EnumList),16);

	/** Check for constraint, default, min, and max expressions. **/
	stAttrValue(stLookup(inf,"constraint"),NULL,(char**)&(ph->Constraint),0);
	stAttrValue(stLookup(inf,"default"),NULL,(char**)&(ph->DefaultExpr),0);
	stAttrValue(stLookup(inf,"min"),NULL,(char**)&(ph->MinValue),0);
	stAttrValue(stLookup(inf,"max"),NULL,(char**)&(ph->MaxValue),0);

	/** Compile expressions, if any **/
	if (ph->Constraint || ph->DefaultExpr || ph->MinValue || ph->MaxValue)
	    {
	    tmplist = expCreateParamList();
	    expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
	    if (ph->Constraint)
	        {
		ph->Constraint = expCompileExpression(ph->Constraint, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->Constraint)
		    {
		    mssError(0,"OSML","Error in 'constraint' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->DefaultExpr = expCompileExpression(ph->DefaultExpr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->DefaultExpr)
		    {
		    mssError(0,"OSML","Error in 'default' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->MinValue = expCompileExpression(ph->MinValue, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->MinValue)
		    {
		    mssError(0,"OSML","Error in 'min' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->MaxValue = expCompileExpression(ph->MaxValue, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->MaxValue)
		    {
		    mssError(0,"OSML","Error in 'max' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		}
	    expFreeParamList(tmplist);
	    }

	/** Enumerated values list, given explicitly? **/
	ptr = NULL;
	cnt = 0;
	while(stAttrValue(stLookup(inf,"enumlist"),&n, &ptr, cnt) >= 0)
	    {
	    /** Check for string enum or integer enum. **/
	    if (ptr)
	        {
		newptr = nmSysStrdup(ptr);
		}
	    else
	        {
		newptr = (char*)nmSysMalloc(16);
		sprintf(newptr,"%d",n);
		}
	    xaAddItem(&(ph->EnumList), (void*)newptr);
	    cnt++;
	    ptr = NULL;
	    }

	/** Or, enumerated value list, given via query? **/
	if (cnt == 0 && stAttrValue(stLookup(inf,"enumquery"),NULL,&ptr,0) >= 0)
	    {
	    ph->EnumQuery = nmSysStrdup(ptr);
	    }

	/** Has a presentation format? **/
	if (stAttrValue(stLookup(inf,"format"),NULL,&ptr,0) >= 0)
	    {
	    ph->Format = nmSysStrdup(ptr);
	    }

	/** Set VisualLength attributes? **/
	if (stAttrValue(stLookup(inf,"length"),&n,NULL,0) >= 0)
	    {
	    ph->VisualLength = n;
	    }
	else
	    {
	    switch(data_type)
	        {
		case DATA_T_INTEGER: ph->VisualLength = 13; break;
		case DATA_T_STRING: ph->VisualLength = 32; break;
		case DATA_T_DATETIME: ph->VisualLength = 20; break;
		case DATA_T_MONEY: ph->VisualLength = 16; break;
		case DATA_T_DOUBLE: ph->VisualLength = 18; break;
		default: ph->VisualLength = 16; break;
		}
	    }
	if (stAttrValue(stLookup(inf,"height"),&n,NULL,0) >= 0)
	    ph->VisualLength2 = n;
	else
	    ph->VisualLength2 = 1;

	/** Check for read-only bits in a bitmask **/
	ph->BitmaskRO = 0;
	cnt = 0;
	while(stAttrValue(stLookup(inf,"readonlybits"), &n, NULL,cnt) >= 0)
	    {
	    ph->BitmaskRO |= (1<<n);
	    cnt++;
	    }

	/** Check for style information. **/
	cnt = 0;
	ph->Style = 0;
	while(stAttrValue(stLookup(inf,"style"),NULL,&ptr,cnt) >= 0)
	    {
	    if (!strcmp(ptr,"bitmask")) ph->Style |= OBJ_PH_STYLE_BITMASK;
	    else if (!strcmp(ptr,"list")) ph->Style |= OBJ_PH_STYLE_LIST;
	    else if (!strcmp(ptr,"buttons")) ph->Style |= OBJ_PH_STYLE_BUTTONS;
	    else if (!strcmp(ptr,"allownull")) ph->Style |= OBJ_PH_STYLE_ALLOWNULL;
	    else if (!strcmp(ptr,"strnull")) ph->Style |= OBJ_PH_STYLE_STRNULL;
	    else if (!strcmp(ptr,"grouped")) ph->Style |= OBJ_PH_STYLE_GROUPED;
	    else if (!strcmp(ptr,"readonly")) ph->Style |= OBJ_PH_STYLE_READONLY;
	    else if (!strcmp(ptr,"hidden")) ph->Style |= OBJ_PH_STYLE_HIDDEN;
	    else if (!strcmp(ptr,"password")) ph->Style |= OBJ_PH_STYLE_PASSWORD;
	    else if (!strcmp(ptr,"multiline")) ph->Style |= OBJ_PH_STYLE_MULTILINE;
	    }

	/** Check for group ID and Name **/
	ph->GroupID = -1;
	ph->GroupName = NULL;
	if (stAttrValue(stLookup(inf,"groupid"),&n,NULL,0) >= 0) ph->GroupID = n;
	if (stAttrValue(stLookup(inf,"groupname"),NULL,&ptr,0) >= 0)
	    {
	    ph->GroupName = nmSysStrdup(ptr);
	    }

	/** Description of field? **/
	if (stAttrValue(stLookup(inf,"description"),NULL,&ptr,0) >= 0)
	    {
	    ph->FriendlyName = nmSysStrdup(ptr);
	    }

    return ph;
    }
