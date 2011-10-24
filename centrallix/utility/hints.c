#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "stparse.h"
#include "hints.h"
#include "ptod.h"
#include "cxlib/util.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2004 LightSys Technology Services, Inc.		*/
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

    $Id: hints.c,v 1.13 2010/09/09 01:24:10 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/hints.c,v $

    $Log: hints.c,v $
    Revision 1.13  2010/09/09 01:24:10  gbeeley
    - (feature) stubbing out transaction log file functionality
    - (change) extending pathnames to 32 possible elements instead of 16
    - (change) stubbing out GetQueryCoverageMask functionality, to be used
      for correlated vs non-correlated subqueries
    - (change) adding hints flags for 'primary key' and 'apply hints on change'
      which allows the user to temporarily have an invalid value in the field
      that is only checked when the field loses focus

    Revision 1.12  2009/06/26 16:26:32  gbeeley
    - (change) switch to specifying the RUNCLIENT domain when generating hint
      expressions
    - (performance) reduce the encoded length of hint data

    Revision 1.11  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.10  2008/01/06 20:20:56  gbeeley
    - (bugfix) properly duplicate string values when copying a hints structure,
      to avoid double free problems.

    Revision 1.9  2007/04/19 21:26:50  gbeeley
    - (change/security) Big conversion.  HTML generator now uses qprintf
      semantics for building strings instead of sprintf.  See centrallix-lib
      for information on qprintf (quoting printf).  Now that apps can take
      parameters, we need to do this to help protect against "cross site
      scripting" issues, but it in any case improves the robustness of the
      application generation process.
    - (change) Changed many htrAddXxxYyyItem_va() to just htrAddXxxYyyItem()
      if just a constant string was used with no %s/%d/etc conversions.

    Revision 1.8  2007/03/01 21:54:05  gbeeley
    - (feature) stub out code for 'createonly' presentation hints style flag

    Revision 1.7  2005/02/26 06:42:41  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.6  2004/07/02 00:23:25  mmcgill
    Changes include, but are not necessarily limitted to:
        - fixed test_obj hints printing, added printing of hints to show command
        to make them easier to read.
        - added objDuplicateHints, for making deep copies of hints structures.
        - made sure GroupID and VisualLength2 were set to their proper defualts
          inf objPresentationHints() [obj_attr.c]
        - did a bit of restructuring in the sybase OS driver:
    	* moved the type conversion stuff in sybdGetAttrValue into a seperate
    	  function (sybd_internal_GetCxValue, sybd_internal_GetCxType). In
    	* Got rid of the Types union, made it an ObjData struct instead
    	* Stored column lengths in ColLengths
    	* Fixed a couple minor bugs
        - Roughed out a preliminary hints implementation for the sybase driver,
          in such a way that it shouldn't be *too* big a deal to add support for
          user-defined types.

    Revision 1.5  2004/05/07 01:18:23  gbeeley
    - support for StyleMask addition to the Style hint, which allows the
      determination of which hints have been set, not just whether they are on
      and off.  Also added 'negative' styles, e.g., 'allownull' vs 'notnull'.

    Revision 1.4  2004/05/04 18:21:05  gbeeley
    - moving location of PTOD definition

    Revision 1.3  2004/02/24 20:07:01  gbeeley
    - adding hntObjToHints() to retrieve hints from an object in the
      objectsystem
    - adding hntVerifyHints() to validate hints on a data value
    - adding hntEncodeHints() to prepare hints to be sent to the client
      (loosely based on some encoding that was in net_http.c)

    Revision 1.2  2003/04/24 03:10:01  gbeeley
    Adding AllowChars and BadChars to presentation hints base
    implementation.

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


/*** hnt_internal_GetExpr() - gets an expression from an object
 ***/
pExpression
hnt_internal_GetExpr(pObject obj, char* attrname)
    {
    pExpression exp;
    int t;
    ObjData pod;

	/** Check data type **/
	t = objGetAttrType(obj, attrname);
	if (t < 0) return NULL;
	if (t == DATA_T_CODE)
	    {
	    /** Get code directly **/
	    if (objGetAttrValue(obj, attrname, t, POD(&exp)) != 0)
		return NULL;
	    exp = expDuplicateExpression(exp);
	    }
	else
	    {
	    /** Build exp from pod **/
	    if (objGetAttrValue(obj, attrname, t, &pod) != 0)
		return NULL;
	    exp = expPodToExpression(&pod, t, NULL);
	    }

    return exp;
    }

/*** hnt_internal_GetString() - gets a string value 
 ***/
char*
hnt_internal_GetString(pObject obj, char* attrname)
    {
    char* str;

	if (objGetAttrValue(obj,attrname,DATA_T_STRING, POD(&str)) != 0)
	    return NULL;
	str = nmSysStrdup(str);

    return str;
    }


/*** hnt_internal_SetStyleItem() - sets a style item and updates the hints
 *** structure's Style and StyleMask accordingly.
 ***/
int
hnt_internal_SetStyleItem(pObjPresentationHints ph, char* style)
    {
    static char* style_names[] = {  "bitmask","list","buttons","notnull",
				    "strnull","grouped","readonly","hidden",
				    "password","multiline","highlight","uppercase",
				    "lowercase","tabpage","sepwindow","alwaysdef",
				    "createonly", "multiselect", "key", "applyonchange",
				    NULL };
    static int style_ids[] = {	OBJ_PH_STYLE_BITMASK, OBJ_PH_STYLE_LIST, OBJ_PH_STYLE_BUTTONS, OBJ_PH_STYLE_NOTNULL,
				OBJ_PH_STYLE_STRNULL, OBJ_PH_STYLE_GROUPED, OBJ_PH_STYLE_READONLY, OBJ_PH_STYLE_HIDDEN,
				OBJ_PH_STYLE_PASSWORD, OBJ_PH_STYLE_MULTILINE, OBJ_PH_STYLE_HIGHLIGHT, OBJ_PH_STYLE_UPPERCASE,
				OBJ_PH_STYLE_LOWERCASE, OBJ_PH_STYLE_TABPAGE, OBJ_PH_STYLE_SEPWINDOW, OBJ_PH_STYLE_ALWAYSDEF,
				OBJ_PH_STYLE_CREATEONLY, OBJ_PH_STYLE_MULTISEL, OBJ_PH_STYLE_KEY, OBJ_PH_STYLE_APPLYCHG };
    static char* nstyle_names[] = { "nobitmask","nolist","nobuttons","allownull",
				    "nostrnull","nogrouped","modifiable","visible",
				    "nopassword","singleline","nohighlight","mixedcase",
				    "mixedcase","notabpage","nosepwindow","noalwaysdef",
				    "nocreateonly","singleselect", "notkey", "applyonmodify",
				    NULL };
    int i;

	/** loop through styles, pick out the ones that apply and set em 
	 ** Don't break out of loop in second case since the style 
	 ** "mixedcase" negates two other styles - uppercase and lowercase.
	 **/
	for(i=0;i<sizeof(style_ids)/sizeof(int);i++)
	    {
	    if (!strcmp(style, style_names[i]))
		{
		ph->Style |= style_ids[i];
		ph->StyleMask |= style_ids[i];
		return 0;
		}
	    }
	for(i=0;i<sizeof(style_ids)/sizeof(int);i++)
	    {
	    if (!strcmp(style, nstyle_names[i]))
		{
		ph->Style &= ~style_ids[i];
		ph->StyleMask |= style_ids[i];
		}
	    }

    return 0;
    }


/*** hntObjToHints() - reads hints data from a pObject
 ***/
pObjPresentationHints
hntObjToHints(pObject obj)
    {
    pObjPresentationHints ph;
    int t,i;
    char* ptr;
    int n;
    ObjData od;

    	/** Allocate a new ph structure **/
	ph = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
	if (!ph) return NULL;
	memset(ph,0,sizeof(ObjPresentationHints));
	xaInit(&(ph->EnumList),16);

	/** expressions **/
	ph->Constraint = hnt_internal_GetExpr(obj,"constraint");
	ph->DefaultExpr = hnt_internal_GetExpr(obj,"default");
	ph->MinValue = hnt_internal_GetExpr(obj,"min");
	ph->MaxValue = hnt_internal_GetExpr(obj,"max");

	/** enum list? **/
	t = objGetAttrType(obj,"enumlist");
	if (t >= 0) 
	    {
	    if (objGetAttrValue(obj,"enumlist",t,&od) != 0) t = -1;
	    }
	if (t>=0) switch(t)
	    {
	    case DATA_T_STRINGVEC:
		for(i=0;i<od.StringVec->nStrings;i++)
		    xaAddItem(&(ph->EnumList), nmSysStrdup(od.StringVec->Strings[i]));
		break;

	    case DATA_T_INTVEC:
		for(i=0;i<od.IntVec->nIntegers;i++)
		    {
		    ptr = nmSysMalloc(16);
		    sprintf(ptr,"%d",od.IntVec->Integers[i]);
		    xaAddItem(&(ph->EnumList), ptr);
		    }
		break;

	    case DATA_T_INTEGER:
		ptr = nmSysMalloc(16);
		sprintf(ptr,"%d",od.Integer);
		xaAddItem(&(ph->EnumList), ptr);
		break;

	    case DATA_T_STRING:
		xaAddItem(&(ph->EnumList), nmSysStrdup(od.String));
		break;

	    default:
		mssError(1,"HNT","Invalid type for enumlist!");
		objFreeHints(ph);
		return NULL;
	    }

	/** String type hint info **/
	ph->EnumQuery = hnt_internal_GetString(obj,"enumquery");
	ph->AllowChars = hnt_internal_GetString(obj,"allowchars");
	ph->BadChars = hnt_internal_GetString(obj,"badchars");
	ph->Format = hnt_internal_GetString(obj,"format");
	ph->GroupName = hnt_internal_GetString(obj,"groupname");
	ph->FriendlyName = hnt_internal_GetString(obj,"description");

	/** Int type hint info **/
	if (objGetAttrValue(obj,"length",DATA_T_INTEGER,POD(&n)) == 0)
	    ph->Length = n;
	if (objGetAttrValue(obj,"width",DATA_T_INTEGER,POD(&n)) == 0)
	    ph->VisualLength = n;
	if (objGetAttrValue(obj,"height",DATA_T_INTEGER,POD(&n)) == 0)
	    ph->VisualLength2 = n;
	else
	    ph->VisualLength2 = 1;
	if (objGetAttrValue(obj,"groupid",DATA_T_INTEGER,POD(&n)) == 0)
	    ph->GroupID = n;

	/** Read-only bits **/
	t = objGetAttrType(obj, "readonlybits");
	if (t >= 0) 
	    {
	    if (objGetAttrValue(obj, "readonlybits", t, &od) != 0) t = -1;
	    }
	if (t>=0) switch(t)
	    {
	    case DATA_T_INTEGER:
		ph->BitmaskRO = (1<<od.Integer);
		break;
	    case DATA_T_INTVEC:
		ph->BitmaskRO = 0;
		for(i=0;i<od.IntVec->nIntegers;i++)
		    ph->BitmaskRO |= (1<<od.IntVec->Integers[i]);
		break;
	    default:
		mssError(1,"HNT","Invalid type for readonlybits!");
		objFreeHints(ph);
		return NULL;
	    }

	/** Style **/
	t = objGetAttrType(obj, "style");
	if (t >= 0)
	    {
	    if (objGetAttrValue(obj, "style", t, &od) != 0) t = -1;
	    }
	i=0;
	if (t == DATA_T_STRING || t == DATA_T_STRINGVEC) 
	  while(1)
	    {
	    /** String or StringVec? **/
	    if (t == DATA_T_STRING)
		{
		ptr = od.String;
		}
	    else
		{
		if (od.StringVec->nStrings == 0) break;
		ptr = od.StringVec->Strings[i];
		}

	    /** Check style settings **/
	    hnt_internal_SetStyleItem(ph, ptr);

	    if (t == DATA_T_STRING || i >= od.StringVec->nStrings-1) break;
	    i++;
	    }

    return ph;
    }


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

	/** Permitted and not permitted characters in a string? **/
	if (stAttrValue(stLookup(inf,"allowchars"),NULL,&ptr,0) >= 0)
	    {
	    ph->AllowChars = nmSysStrdup(ptr);
	    }
	if (stAttrValue(stLookup(inf,"badchars"),NULL,&ptr,0) >= 0)
	    {
	    ph->BadChars = nmSysStrdup(ptr);
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
	    hnt_internal_SetStyleItem(ph, ptr);
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


/*** hnt_internal_GetAttrType() - return the type of the new value
 ***/
int
hnt_internal_GetAttrType(pObject obj, char* attrname)
    {
    pTObjData ptod = PTOD(obj);
    if (strcmp(attrname,"value")) return -1;
    return ptod->DataType;
    }
/*** hnt_internal_GetAttrValue() - return the value
 ***/
int
hnt_internal_GetAttrValue(pObject obj, char* attrname, int type, pObjData pod)
    {
    pTObjData ptod = PTOD(obj);
    if (strcmp(attrname,"value")) return -1;
    if (type != ptod->DataType) return -1;
    objCopyData(&(ptod->Data), pod, type);
    return 0;
    }
/*** hnt_internal_SetAttrValue() - set the value... not!
 ***/
int
hnt_internal_SetAttrValue(pObject obj, char* attrname, int type, pObjData pod)
    {
    return -1;
    }


/*** hntVerifyHints() - verify a given presentation hints structure against
 *** a data value and data type.  Returns 0 on success, -1 on failure to
 *** verify, and sets 'msg' equal to a static error message string as to
 *** the reason for any failure.  'msg' is unset on success.  If 'objlist'
 *** is provided (not NULL), it contains object(s) that can be referenced
 *** by the four hint expressions (constraint, default, min, max).  This
 *** should *only* be called if the value was modified.
 ***/
int
hntVerifyHints(pObjPresentationHints ph, pTObjData ptod, char** msg, pParamObjects objlist, pObjSession session_ctx)
    {
    pParamObjects our_objlist = NULL;
    int rval = 0;
    int cmp;
    int found;
    int was_empty_string;
    int i;

	/** Init our parameter object list for constraint eval; the proper
	 ** way for the new value to be referenced is :new:value
	 **/
	if (!objlist) 
	    our_objlist = expCreateParamList();
	else
	    our_objlist = objlist;
	if (!our_objlist) { *msg="ERR: out of memory"; return -1; }
	expAddParamToList(our_objlist, "new", (pObject)ptod, 0);
	expSetParamFunctions(our_objlist, "new", hnt_internal_GetAttrType,
		hnt_internal_GetAttrValue, hnt_internal_SetAttrValue);
	if (!objlist)
	    our_objlist->Session = session_ctx;

	/** If readonly, fail right away. **/
	if (ph->Style & OBJ_PH_STYLE_READONLY)
	    {
	    rval = -1;
	    *msg = "Value is readonly";
	    }

	/** Check default **/
	if (rval == 0 && (ptod->Flags & DATA_TF_NULL) && ph->DefaultExpr)
	    {
	    if (expEvalTree(ph->DefaultExpr, our_objlist) < 0)
		{
		rval = -1;
		*msg = "ERR: default eval failed";
		}
	    else
		{
		if (EXPR(ph->DefaultExpr)->DataType != ptod->DataType && ptod->DataType != DATA_T_UNAVAILABLE)
		    {
		    rval = -1;
		    *msg = "ERR: type mismatch on default value expression";
		    }
		else
		    {
		    ptod->DataType = EXPR(ph->DefaultExpr)->DataType;
		    ptod->Flags &= ~DATA_TF_NULL;
		    if (EXPR(ph->DefaultExpr)->Flags & EXPR_F_NULL) 
			{
			ptod->Flags |= DATA_TF_NULL;
			}
		    else
			{
			/** Copy expr data to ptod **/
			switch(ptod->DataType)
			    {
			    case DATA_T_INTEGER:
				ptod->Data.Integer = EXPR(ph->DefaultExpr)->Integer;
				break;
			    case DATA_T_DOUBLE:
				ptod->Data.Double = EXPR(ph->DefaultExpr)->Types.Double;
				break;
			    case DATA_T_STRING:
				/** FIXME: dangerous to set string ptr directly like this **/
				ptod->Data.String = EXPR(ph->DefaultExpr)->String;
				break;
			    default:
				rval = -1;
				*msg = "ERR: unsupported type for default value";
				break;
			    }
			}
		    }
		}
	    }

	/** Test null values / empty strings **/
	was_empty_string = 0;
	if (rval == 0 && !(ptod->Flags & DATA_TF_NULL) && ptod->DataType == DATA_T_STRING && ptod->Data.String[0] == '\0' && (ph->Style & OBJ_PH_STYLE_STRNULL))
	    {
	    was_empty_string = 1;
	    ptod->Flags |= DATA_TF_NULL;
	    }
	if (rval == 0 && (ptod->Flags & DATA_TF_NULL) && (ph->Style & OBJ_PH_STYLE_NOTNULL))
	    {
	    rval = -1;
	    if (was_empty_string)
		*msg = "Empty strings not permitted";
	    else
		*msg = "Null values not permitted";
	    }

	/** Test constraint **/
	if (ph->Constraint)
	    {
	    if (expEvalTree(ph->Constraint, our_objlist) < 0)
		{
		rval = -1;
		*msg = "ERR: constraint eval failed";
		}
	    else
		{
		if (EXPR(ph->Constraint)->DataType != DATA_T_INTEGER || (EXPR(ph->Constraint)->Flags & EXPR_F_NULL))
		    {
		    rval = -1;
		    *msg = "ERR: constraint result was NULL or not an integer";
		    }
		else
		    {
		    if (EXPR(ph->Constraint)->Integer == 0)
			{
			rval = -1;
			*msg = "Constraint test failed";
			}
		    }
		}
	    }

	/** Max/Min expression **/
	if (ph->MinValue)
	    {
	    if (expEvalTree(ph->MinValue, our_objlist) < 0)
		{
		rval = -1;
		*msg = "ERR: min value eval failed";
		}
	    else
		{
		if (EXPR(ph->MinValue)->DataType != ptod->DataType)
		    {
		    rval = -1;
		    *msg = "ERR: type mismatch checking min value";
		    }
		else if (EXPR(ph->MinValue)->Flags & EXPR_F_NULL)
		    {
		    rval = -1;
		    *msg = "ERR: min value is null";
		    }
		}
	    }
	if (ph->MaxValue)
	    {
	    if (expEvalTree(ph->MaxValue, our_objlist) < 0)
		{
		rval = -1;
		*msg = "ERR: max value eval failed";
		}
	    else
		{
		if (EXPR(ph->MaxValue)->DataType != ptod->DataType)
		    {
		    rval = -1;
		    *msg = "ERR: type mismatch checking max value";
		    }
		else if (EXPR(ph->MaxValue)->Flags & EXPR_F_NULL)
		    {
		    rval = -1;
		    *msg = "ERR: max value is null";
		    }
		}
	    }
	if ((ph->MinValue || ph->MaxValue) && rval == 0)
	    {
	    if (ptod->Flags & DATA_TF_NULL)
		{
		rval = -1;
		*msg = "Min/Max specified but new value is NULL";
		}
	    else
		{
		cmp = 1;
		switch(ptod->DataType)
		    {
		    case DATA_T_INTEGER:
			cmp &= (!ph->MinValue || EXPR(ph->MinValue)->Integer <= ptod->Data.Integer);
			cmp &= (!ph->MaxValue || EXPR(ph->MaxValue)->Integer >= ptod->Data.Integer);
			break;
		    case DATA_T_DOUBLE:
			cmp &= (!ph->MinValue || EXPR(ph->MinValue)->Types.Double <= ptod->Data.Double);
			cmp &= (!ph->MaxValue || EXPR(ph->MaxValue)->Types.Double >= ptod->Data.Double);
			break;
		    case DATA_T_STRING:
			cmp &= (!ph->MinValue || strcmp(EXPR(ph->MinValue)->String, ptod->Data.String) >= 0);
			cmp &= (!ph->MaxValue || strcmp(EXPR(ph->MaxValue)->String, ptod->Data.String) <= 0);
			break;
		    default:
			rval = -1;
			*msg = "ERR: unsupported type for min/max expression(s)";
			break;
		    }
		if (!cmp)
		    {
		    rval = -1;
		    *msg = "Min/Max test failed";
		    }
		}
	    }

	/** BadChars/AllowChars **/
	if ((ph->BadChars || ph->AllowChars) && ptod->DataType != DATA_T_STRING)
	    {
	    rval = -1;
	    *msg = "ERR: badchars/allowchars set but type is not a string";
	    }
	if (ph->BadChars)
	    {
	    if (!(ptod->Flags & DATA_TF_NULL) && strpbrk(ptod->Data.String, ph->BadChars))
		{
		rval = -1;
		*msg = "String value contains an excluded character";
		}
	    }
	if (ph->AllowChars)
	    {
	    if (!(ptod->Flags & DATA_TF_NULL) && strspn(ptod->Data.String, ph->AllowChars) < strlen(ptod->Data.String))
		{
		rval = -1;
		*msg = "String value contains a non-allowed character";
		}
	    }

	/** Enumlist? **/
	if (ph->EnumList.nItems)
	    {
	    if (ptod->Flags & DATA_TF_NULL)
		{
		rval = -1;
		*msg = "Enum list supplied but new value is NULL";
		}
	    else
		{
		if (ptod->DataType != DATA_T_INTEGER && ptod->DataType != DATA_T_STRING)
		    {
		    rval = -1;
		    *msg = "ERR: type mismatch with enum list";
		    }
		else
		    {
		    found = 0;
		    for(i=0;i<ph->EnumList.nItems;i++)
			{
			if (ptod->DataType == DATA_T_INTEGER)
			    found = (ptod->Data.Integer == strtoi(ph->EnumList.Items[i], NULL, 10));
			else
			    found = !strcmp(ptod->Data.String, ph->EnumList.Items[i]);
			if (found) break;
			}
		    if (!found)
			{
			rval = -1;
			*msg = "Value not found in the list of acceptable values";
			}
		    }
		}
	    }

	/** FIXME: Enumquery to be added later (MQ efficiency issue) **/

	/** Free the parameter object list **/
	expRemoveParamFromList(our_objlist, "new");
	if (!objlist) expFreeParamList(our_objlist);

    return rval;
    }



/*** hnt_internal_Escape() - escapes non-alphanumeric characters into %xx
 *** sequences.
 ***/
int
hnt_internal_Escape(pXString dst, char* src)
    {
    char* ok_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char* hex_code = "0123456789abcdef";
    int len;
    char buf[4] = "%20";

        /** go through and encode the chars. **/
        while(*src)
            {
            /** Find a "span" of ok chars **/
            len = strspn(src, ok_chars);
            if (len < 0) len = 0;
            if (len)
                {
                /** Concat the span onto the xstring **/
                xsConcatenate(dst, src, len);
                src += len;
                }
            else
                {
                /** Convert one char **/
                buf[1] = hex_code[(((unsigned char)(*src))>>4)&0x0F];
                buf[2] = hex_code[((unsigned char)(*src))&0x0F];
                xsConcatenate(dst, buf, 3);
                src++;
                }
            }

    return 0;
    }


/*** hntEncodeHints() - given the presentation hints structure, encode that into
 *** a string suitable for sending in a DHTML document or other HTTP 
 *** communication, in the form hint=value&hint=value ...
 ***
 *** Concatenates on to the end of the given XString.
 *** Returns the # of characters added to the XString, or negative on error.
 ***/
int
hntEncodeHints(pObjPresentationHints ph, pXString xs)
    {
    int initlen = xs->Length;
    int is_first = 1;
    int i;
    XString expstr;

	/** Add the expression-type hints **/
	xsInit(&expstr);
	if (ph->DefaultExpr)
	    {
	    xsConcatPrintf(xs, "%sd=", is_first?"":"&");
	    expGenerateText(ph->DefaultExpr, NULL, xsWrite, &expstr, '\0', "javascript", EXPR_F_RUNCLIENT);
	    hnt_internal_Escape(xs, expstr.String);
	    xsCopy(&expstr,"",0);
	    is_first=0;
	    }
	if (ph->MinValue)
	    {
	    xsConcatPrintf(xs, "%sm=", is_first?"":"&");
	    expGenerateText(ph->MinValue, NULL, xsWrite, &expstr, '\0', "javascript", EXPR_F_RUNCLIENT);
	    hnt_internal_Escape(xs, expstr.String);
	    xsCopy(&expstr,"",0);
	    is_first=0;
	    }
	if (ph->MaxValue)
	    {
	    xsConcatPrintf(xs, "%sM=", is_first?"":"&");
	    expGenerateText(ph->MaxValue, NULL, xsWrite, &expstr, '\0', "javascript", EXPR_F_RUNCLIENT);
	    hnt_internal_Escape(xs, expstr.String);
	    xsCopy(&expstr,"",0);
	    is_first=0;
	    }
	if (ph->Constraint)
	    {
	    xsConcatPrintf(xs, "%sc=", is_first?"":"&");
	    expGenerateText(ph->Constraint, NULL, xsWrite, &expstr, '\0', "javascript", EXPR_F_RUNCLIENT);
	    hnt_internal_Escape(xs, expstr.String);
	    xsCopy(&expstr,"",0);
	    is_first=0;
	    }
	xsDeInit(&expstr);

	/** Add enumlist **/
	if (ph->EnumList.nItems > 0)
	    {
	    xsConcatPrintf(xs, "%sel=", is_first?"":"&");
	    is_first=0;
	    for(i=0;i<ph->EnumList.nItems;i++)
		{
		if (i != 0) xsConcatenate(xs, ",", 1);
		hnt_internal_Escape(xs, ph->EnumList.Items[i]);
		}
	    }

	/** String type hints **/
	if (ph->EnumQuery != NULL)
	    {
	    xsConcatPrintf(xs, "%seq=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->EnumQuery);
	    is_first = 0;
	    }
	if (ph->Format != NULL)
	    {
	    xsConcatPrintf(xs, "%sfm=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->Format);
	    is_first = 0;
	    }
	if (ph->GroupName != NULL)
	    {
	    xsConcatPrintf(xs, "%sgn=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->GroupName);
	    is_first = 0;
	    }
	if (ph->FriendlyName != NULL)
	    {
	    xsConcatPrintf(xs, "%sfn=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->FriendlyName);
	    is_first = 0;
	    }
	if (ph->AllowChars != NULL)
	    {
	    xsConcatPrintf(xs, "%sac=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->AllowChars);
	    is_first = 0;
	    }
	if (ph->BadChars != NULL)
	    {
	    xsConcatPrintf(xs, "%sbc=", is_first?"":"&");
	    hnt_internal_Escape(xs, ph->BadChars);
	    is_first = 0;
	    }

	/** Integer type hints **/
	if (ph->Length > 0)
	    {
	    xsConcatPrintf(xs, "%sl=%d", is_first?"":"&", ph->Length);
	    is_first = 0;
	    }
	if (ph->VisualLength > 0)
	    {
	    xsConcatPrintf(xs, "%sv1=%d", is_first?"":"&", ph->VisualLength);
	    is_first = 0;
	    }
	if (ph->VisualLength2 > 1)
	    {
	    xsConcatPrintf(xs, "%sv2=%d", is_first?"":"&", ph->VisualLength2);
	    is_first = 0;
	    }
	if (ph->BitmaskRO > 0)
	    {
	    xsConcatPrintf(xs, "%sr=%d", is_first?"":"&", ph->BitmaskRO);
	    is_first = 0;
	    }
	if (ph->StyleMask > 0)
	    {
	    xsConcatPrintf(xs, "%ss=%d,%d", is_first?"":"&", ph->Style, ph->StyleMask);
	    is_first = 0;
	    }
	if (ph->GroupID > 0)
	    {
	    xsConcatPrintf(xs, "%sG=%d", is_first?"":"&", ph->GroupID);
	    is_first = 0;
	    }
	if (ph->OrderID > 0)
	    {
	    xsConcatPrintf(xs, "%sO=%d", is_first?"":"&", ph->OrderID);
	    is_first = 0;
	    }

    return xs->Length - initlen;
    }

pObjPresentationHints
objDuplicateHints(pObjPresentationHints hints)
    {
    pObjPresentationHints new_hints;
    int i;

	if (!hints) return NULL;
	if ( (new_hints=(pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
	
	/** shallow copy first **/
	memcpy(new_hints, hints, sizeof(ObjPresentationHints));

	/** Now the XArray **/
	xaInit(&(new_hints->EnumList), 8);
	for (i=0;i<xaCount(&(hints->EnumList));i++)
	    xaAddItem(&(new_hints->EnumList), xaGetItem(&(new_hints->EnumList), i));
	
	/** Duplicate the expressions **/
	if (hints->Constraint) new_hints->Constraint = expDuplicateExpression(hints->Constraint);
	if (hints->DefaultExpr) new_hints->DefaultExpr = expDuplicateExpression(hints->DefaultExpr);
	if (hints->MinValue) new_hints->MinValue = expDuplicateExpression(hints->MinValue);
	if (hints->MaxValue) new_hints->MaxValue = expDuplicateExpression(hints->MaxValue);

	/** Duplicate string values **/
	if (hints->EnumQuery) new_hints->EnumQuery = nmSysStrdup(hints->EnumQuery);
	if (hints->Format) new_hints->Format = nmSysStrdup(hints->Format);
	if (hints->GroupName) new_hints->GroupName = nmSysStrdup(hints->GroupName);
	if (hints->FriendlyName) new_hints->FriendlyName = nmSysStrdup(hints->FriendlyName);
	
	/** All Done **/
	return new_hints;
    }

    
