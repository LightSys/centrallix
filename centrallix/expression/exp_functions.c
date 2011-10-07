#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"

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
/* Module: 	expression.h, exp_functions.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*		--> exp_functions.c: defines the functions that are	*/
/*		available for EXPR_N_FUNCTION nodes, and provides	*/
/*		evaluators for each function.  When adding a new one,	*/
/*		be sure to add the evaluator, and then add the function	*/
/*		to the function list initialization given below.  No	*/
/*		changes are normally needed to the compiler, unless you	*/
/*		are adding a new aggregate-type function, in which case	*/
/*		a place or two in the compiler need to be changed to	*/
/*		reflect that.						*/
/*									*/
/*		Careful when adding new functions!  See the note about	*/
/*		that issue in exp_evaluate.c				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: exp_functions.c,v 1.20 2010/09/13 23:30:29 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_functions.c,v $

    $Log: exp_functions.c,v $
    Revision 1.20  2010/09/13 23:30:29  gbeeley
    - (admin) prepping for 0.9.1 release, update text files, etc.
    - (change) removing some 'unused local variables'

    Revision 1.19  2010/09/08 21:55:09  gbeeley
    - (bugfix) allow /file/name:"attribute" to be quoted.
    - (bugfix) order by ... asc/desc keywords are now case insenstive
    - (bugfix) short-circuit eval was not resulting in aggregates properly
      evaluating
    - (change) new API function expModifyParamByID - use this for efficiency
    - (feature) multi-level aggregate functions now supported, for use when
      a sql query has a group by, e.g. select max(sum(...)) ... group by ...
    - (feature) added mathematical and trig functions radians, degrees, sin,
      cos, tan, asin, acos, atan, atan2, sqrt, square

    Revision 1.18  2009/10/20 23:07:20  gbeeley
    - (feature) adding dateadd() function

    Revision 1.17  2009/06/24 17:33:19  gbeeley
    - (change) adding domain param to expGenerateText, so it can be used to
      generate an expression string with lower domains converted to constants
    - (bugfix) better handling of runserver() embedded within runclient(), etc
    - (feature) allow subtracting strings, e.g., "abcde" - "de" == "abc"
    - (bugfix) after a property has been set using reverse evaluation, tag it
      as modified so it shows up as changed in other expressions using that
      same object param list
    - (change) condition() function now uses short-circuit evaluation
      semantics, so parameters are only evaluated as they are needed... e.g.
      condition(a,b,c) if a is true, b is returned and c is never evaluated,
      and vice versa.
    - (feature) add structure for reverse-evaluation of functions.  The
      isnull() function now supports this feature.
    - (bugfix) save/restore the coverage mask before/after evaluation, so that
      a nested subexpression (eval or subquery) using the same object list
      will not cause an inconsistency.  Basically a reentrancy bug.
    - (bugfix) some functions were erroneously depending on the data type of
      a NULL value to be correct.
    - (feature) adding truncate() function which is similar to round().
    - (feature) adding constrain() function which limits a value to be
      between a given minimum and maximum value.
    - (bugfix) first() and last() functions were not properly resetting the
      value to NULL between GROUP BY groups
    - (bugfix) some expression-to-JS fixes

    Revision 1.16  2008/04/06 20:36:16  gbeeley
    - (feature) adding support for SQL round() function
    - (change) adding support for division and multiplication with money
      data types.  It is still not permitted to multiply one money type by
      another money type, as 'money' is not a dimensionless value.

    Revision 1.15  2008/03/08 00:41:59  gbeeley
    - (bugfix) a double-free was being triggered on Subquery nodes as a
      result of an obscure glitch in expCopyNode.  The string value should
      be handled regardless of the DataType, as temporary data type changes
      are possible.  Also cleaned up a number of other expression string
      Alloc issues, many just for clarity.

    Revision 1.14  2008/03/06 01:18:59  gbeeley
    - (change) updates to centrallix.supp suppressions file for valgrind
    - (bugfix) several issues fixed as a result of a Valgrind scan, one of
      which has likely been causing a couple of recent crashes.

    Revision 1.13  2008/02/21 21:45:52  gbeeley
    - (bugfix) aggregate sum(), which can operate on strings, was not
      properly resetting string allocation

    Revision 1.12  2007/12/05 18:43:55  gbeeley
    - (bugfix) fix for min(string) crashing due to invalid free()

    Revision 1.11  2007/11/16 21:40:09  gbeeley
    - (bugfix) quote() should return NULL if its parameter is NULL.

    Revision 1.10  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.9  2005/09/30 04:37:10  gbeeley
    - (change) modified expExpressionToPod to take the type.
    - (feature) got eval() working
    - (addition) added expReplaceString() to search-and-replace in an
      expression tree.

    Revision 1.8  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.7  2004/06/30 19:26:09  gbeeley
    - Adding lztrim() function for suppressing leading zeroes.

    Revision 1.6  2004/02/24 20:02:26  gbeeley
    - adding proper support for external references in an expression, so
      that they get re-evaluated each time.  Example - getdate().
    - adding eval() function but no implementation at this time - it is
      however supported for runclient() expressions (in javascript).
    - fixing some quoting issues

    Revision 1.5  2003/07/09 18:07:55  gbeeley
    Added first() and last() aggregate functions.  Strictly speaking these
    are not truly relational functions, since they are row order dependent,
    but are useful for summary items in a sorted setting.

    Revision 1.4  2003/04/24 02:54:48  gbeeley
    Added quote() function as a special case of escape().

    Revision 1.3  2003/04/24 02:13:22  gbeeley
    Added functionality to handle "domain of execution" to the expression
    module, allowing the developer to specify the nature of an expression
    (run on client, server, or static on server).

    Revision 1.2  2001/09/25 18:02:34  gbeeley
    Added replicate() SQL function.

    Revision 1.1.1.1  2001/08/13 18:00:48  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/****** Evaluator functions follow for expEvalFunction ******/

int exp_fn_getdate(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    tree->DataType = DATA_T_DATETIME;
    objCurrentDate(&(tree->Types.Date));

    return 0;
    }

int exp_fn_user_name(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    tree->DataType = DATA_T_STRING;
    ptr = mssUserName();
    if (!ptr)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    tree->String = tree->Types.StringBuf;
    memccpy(tree->String, ptr, 0, 63);
    tree->String[63] = '\0';
    return 0;
    }


int exp_fn_convert(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    void* vptr;
    char* ptr;

    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || (i0->Flags & EXPR_F_NULL))
        {
	mssError(1,"EXP","convert() requires data type and value to be converted");
	return -1;
	}
    switch(i1->DataType)
        {
	case DATA_T_INTEGER: vptr = &(i1->Integer); break;
	case DATA_T_STRING: vptr = i1->String; break;
	case DATA_T_DOUBLE: vptr = &(i1->Types.Double); break;
	case DATA_T_DATETIME: vptr = &(i1->Types.Date); break;
	case DATA_T_MONEY: vptr = &(i1->Types.Money); break;
	default:
	    mssError(1,"EXP","convert(): unsupported arg 2 datatype");
	    return -1;
	}
    if (!strcmp(i0->String,"integer"))
        {
	tree->DataType = DATA_T_INTEGER;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	tree->Integer = objDataToInteger(i1->DataType, vptr, NULL);
	}
    else if (!strcmp(i0->String,"string"))
        {
	tree->DataType = DATA_T_STRING;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	ptr = objDataToStringTmp(i1->DataType, vptr, 0);
	if (tree->Alloc && tree->String)
	    {
	    nmSysFree(tree->String);
	    }
	if (strlen(ptr) > 63)
	    {
	    tree->Alloc = 1;
	    tree->String = nmSysStrdup(ptr);
	    }
	else
	    {
	    tree->Alloc = 0;
	    tree->String = tree->Types.StringBuf;
	    strcpy(tree->String, ptr);
	    }
	}
    else if (!strcmp(i0->String,"double"))
        {
	tree->DataType = DATA_T_DOUBLE;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	tree->Types.Double = objDataToDouble(i1->DataType, vptr);
	}
    else if (!strcmp(i0->String,"money"))
        {
	tree->DataType = DATA_T_MONEY;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	objDataToMoney(i1->DataType, vptr, &(tree->Types.Money));
	}
    else if (!strcmp(i0->String,"datetime"))
        {
	tree->DataType = DATA_T_DATETIME;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	objDataToDateTime(i1->DataType, vptr, &(tree->Types.Date), NULL);
	}
    else
        {
	mssError(1,"EXP","convert() datatype '%s' is invalid", i0->String);
	}
    return 0;
    }


int exp_fn_wordify(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    tree->DataType = DATA_T_STRING;
    if (!i0)
        {
	mssError(1,"EXP","Parameter required for wordify() function.");
	return -1;
	}
    if (i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    switch(i0->DataType)
        {
	case DATA_T_INTEGER:
	    ptr = objDataToWords(DATA_T_INTEGER, &(i0->Integer));
	    break;

	case DATA_T_MONEY:
	    ptr = objDataToWords(DATA_T_MONEY, &(i0->Types.Money));
	    break;

	default:
	    mssError(1,"EXP","Can only convert integer and money types with wordify()");
	    return -1;
	}
    if (tree->Alloc && tree->String)
        {
	nmSysFree(tree->String);
	}
    if (strlen(ptr) > 63)
        {
	tree->String = nmSysStrdup(ptr);
	tree->Alloc = 1;
	}
    else
        {
	tree->Alloc = 0;
	tree->String = tree->Types.StringBuf;
        strcpy(tree->String, ptr);
	}
    return 0;
    }


int exp_fn_abs(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0)
        {
	mssError(1,"EXP","Parameter required for abs() function.");
	return -1;
	}
    tree->DataType = i0->DataType;
    if (i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	}
    else
        {
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
	        tree->Integer = abs(i0->Integer);
	        break;

	    case DATA_T_DOUBLE:
	        tree->Types.Double = fabs(i0->Types.Double);
	        break;

	    case DATA_T_MONEY:
	        if (i0->Types.Money.WholePart >= 0)
		    {
		    tree->Types.Money.WholePart = i0->Types.Money.WholePart;
		    tree->Types.Money.FractionPart = i0->Types.Money.FractionPart;
		    }
		else
		    {
		    if (i0->Types.Money.FractionPart != 0)
		        {
			tree->Types.Money.WholePart = -(i0->Types.Money.WholePart + 1);
			tree->Types.Money.FractionPart = 10000 - i0->Types.Money.FractionPart;
			}
		    else
		        {
			tree->Types.Money.WholePart = -i0->Types.Money.WholePart;
			tree->Types.Money.FractionPart = 0;
			}
		    }
	        break;

	    default:
	        mssError(1,"EXP","Invalid data type for abs() function");
		return -1;
	    }
	}
    return 0;
    }


int exp_fn_ascii(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    tree->DataType = DATA_T_INTEGER;
    if (!i0)
        {
	mssError(1,"EXP","Parameter required for ascii() function.");
	return -1;
	}
    if (i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","ascii() function takes a string parameter.");
	return -1;
	}
    if (i0->String[0] == '\0')
	tree->Flags |= EXPR_F_NULL;
    else
	tree->Integer = i0->String[0];
    return 0;
    }

int exp_fn_condition(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0 || !i1 || !i2)
        {
	mssError(1,"EXP","Three parameters required for condition()");
	return -1;
	}
    if (exp_internal_EvalTree(i0,objlist) < 0)
	{
	return -1;
	}
    if (i0->DataType != DATA_T_INTEGER)
        {
	mssError(1,"EXP","condition() first parameter must evaluate to boolean");
	return -1;
	}
    if (i0->Flags & EXPR_F_NULL) 
        {
	tree->DataType = DATA_T_INTEGER;
	tree->Flags |= EXPR_F_NULL;
	i1->ObjDelayChangeMask |= (objlist->ModCoverageMask & i1->ObjCoverageMask);
	i2->ObjDelayChangeMask |= (objlist->ModCoverageMask & i2->ObjCoverageMask);
	return 0;
	}
    if (i0->Integer != 0)
        {
	/** True, return 2nd argument i1 **/
	i2->ObjDelayChangeMask |= (objlist->ModCoverageMask & i2->ObjCoverageMask);
	if (exp_internal_EvalTree(i1,objlist) < 0)
	    {
	    return -1;
	    }
	if (i2->AggLevel > 0)
	    if (exp_internal_EvalTree(i2,objlist) < 0)
		return -1;
	tree->DataType = i1->DataType;
	if (i1->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	switch(i1->DataType)
    	    {
    	    case DATA_T_INTEGER: tree->Integer = i1->Integer; break;
    	    case DATA_T_STRING: tree->String = i1->String; tree->Alloc = 0; break;
    	    default: memcpy(&(tree->Types), &(i1->Types), sizeof(tree->Types)); break;
    	    }
	}
    else
        {
	/** False, return 3rd argument i2 **/
	if (i1->AggLevel > 0)
	    {
	    if (exp_internal_EvalTree(i1,objlist) < 0)
		return -1;
	    }
	else
	    {
	    i1->ObjDelayChangeMask |= (objlist->ModCoverageMask & i1->ObjCoverageMask);
	    }
	if (exp_internal_EvalTree(i2,objlist) < 0)
	    {
	    return -1;
	    }
	tree->DataType = i2->DataType;
	if (i2->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	switch(i2->DataType)
    	    {
    	    case DATA_T_INTEGER: tree->Integer = i2->Integer; break;
    	    case DATA_T_STRING: tree->String = i2->String; tree->Alloc = 0; break;
    	    default: memcpy(&(tree->Types), &(i2->Types), sizeof(tree->Types)); break;
    	    }
	}
    return 0;
    }


int exp_fn_charindex(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    tree->DataType = DATA_T_INTEGER;
    if (!i0 || !i1)
        {
	mssError(1,"EXP","Two string parameters required for charindex()");
	return -1;
	}
    if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","Two string parameters required for charindex()");
	return -1;
	}
    ptr = strstr(i1->String, i0->String);
    if (ptr == NULL)
	tree->Integer = 0;
    else
        tree->Integer = (ptr - i1->String) + 1;
    return 0;
    }


int exp_fn_upper(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n,i;

    tree->DataType = DATA_T_STRING;
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One string parameter required for upper()");
	return -1;
	}
    if (i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    n = strlen(i0->String);
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	tree->Alloc = 0;
	}
    if (n < 63)
	{
	tree->String = tree->Types.StringBuf;
	tree->Alloc = 0;
	}
    else
	{
	tree->String = (char*)nmSysMalloc(n+1);
	tree->Alloc = 1;
	}
    for(i=0;i<n+1;i++) 
        {
	if (i0->String[i] >= 'a' && i0->String[i] <= 'z') tree->String[i] = i0->String[i] - 32;
	else tree->String[i] = i0->String[i];
	}
    return 0;
    }


int exp_fn_lower(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n,i;

    tree->DataType = DATA_T_STRING;
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One string parameter required for lower()");
	return -1;
	}
    if (i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    n = strlen(i0->String);
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	tree->Alloc = 0;
	}
    if (n < 63)
	{
	tree->String = tree->Types.StringBuf;
	tree->Alloc = 0;
	}
    else
	{
	tree->String = (char*)nmSysMalloc(n+1);
	tree->Alloc = 1;
	}
    for(i=0;i<n+1;i++) 
        {
	if (i0->String[i] >= 'A' && i0->String[i] <= 'Z') tree->String[i] = i0->String[i] + 32;
	else tree->String[i] = i0->String[i];
	}
    return 0;
    }


int exp_fn_char_length(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    tree->DataType = DATA_T_INTEGER;
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One string parameter required for char_length()");
	return -1;
	}
    tree->Integer = strlen(i0->String);
    return 0;
    }


int exp_fn_datepart(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pDateTime dtptr;
    DateTime dt;

    tree->DataType = DATA_T_INTEGER;
    if (i0 && i1 && ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL)))
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || !i1) 
        {
	mssError(1,"EXP","datepart() requires two parameters");
	return -1;
	}
    if (i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","param 1 to datepart() must be month, day, year, hour, minute, or second");
	return -1;
	}
    if (i1->DataType == DATA_T_DATETIME)
        {
	dtptr = &(i1->Types.Date);
	}
    else if (i1->DataType == DATA_T_STRING)
        {
	if (objDataToDateTime(DATA_T_STRING, i1->String, &dt, NULL) < 0)
	    {
	    mssError(1,"EXP","in datepart(), failed to parse string date/time value");
	    return -1;
	    }
	dtptr = &dt;
	}
    else
        {
	mssError(1,"EXP","param 2 to datepart() must be a date/time value");
	return -1;
	}
    if (!strcasecmp(i0->String,"year"))
	tree->Integer = dtptr->Part.Year + 1900;
    else if (!strcasecmp(i0->String,"month"))
        tree->Integer = dtptr->Part.Month + 1;
    else if (!strcasecmp(i0->String,"day"))
        tree->Integer = dtptr->Part.Day + 1;
    else if (!strcasecmp(i0->String,"hour"))
        tree->Integer = dtptr->Part.Hour;
    else if (!strcasecmp(i0->String,"minute"))
        tree->Integer = dtptr->Part.Minute;
    else if (!strcasecmp(i0->String,"second"))
        tree->Integer = dtptr->Part.Second;
    else
        {
	mssError(1,"EXP","param 1 to datepart() must be month, day, year, hour, minute, or second");
	return -1;
	}
    return 0;
    }

// Reverse isnull() just passes the value on to the first parameter, so the
// isnull() function can be used to set default values on SQL query properties.
int exp_fn_reverse_isnull(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0)
        {
	mssError(1,"EXP","isnull() requires two parameters");
	return -1;
	}
    if (tree->Flags & EXPR_F_NULL) 
	i0->Flags |= EXPR_F_NULL;
    else
	i0->Flags &= ~EXPR_F_NULL;
    switch(tree->DataType)
	{
	case DATA_T_INTEGER: i0->Integer = tree->Integer; break;
	case DATA_T_STRING:
	    if (i0->Alloc && i0->String)
		{
		nmSysFree(i0->String);
		}
	    i0->Alloc = 0;
	    i0->String = tree->String;
	    break;
	default: memcpy(&(i0->Types), &(tree->Types), sizeof(tree->Types)); break;
	}
    i0->DataType = tree->DataType;
    return expReverseEvalTree(i0, objlist);
    }

int exp_fn_isnull(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0 || !i1)
        {
	mssError(1,"EXP","isnull() requires two parameters");
	return -1;
	}
    if (i0->Flags & EXPR_F_NULL) i0 = i1;
    tree->DataType = i0->DataType;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    switch(i0->DataType)
        {
        case DATA_T_INTEGER: tree->Integer = i0->Integer; break;
        case DATA_T_STRING: tree->String = i0->String; tree->Alloc = 0; break;
        default: memcpy(&(tree->Types), &(i0->Types), sizeof(tree->Types));
	}
    return 0;
    }

int exp_fn_replicate(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int nl,n,l;
    char* ptr;
    int i;
    
    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || (i1->DataType != DATA_T_INTEGER && i1->DataType != DATA_T_DOUBLE))
        {
	mssError(1,"EXP","replicate() requires a string and a numeric parameter");
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL) || (i1->DataType == DATA_T_INTEGER && i1->Integer < 0) || (i1->DataType == DATA_T_DOUBLE && i1->Types.Double < 0))
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}

    tree->DataType = DATA_T_STRING;
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	}
    tree->Alloc = 0;
    if (i1->DataType == DATA_T_INTEGER)
        {
        n = (i1->Integer > 255)?255:i1->Integer;
	}
    else
        {
	n = i1->Types.Double + 0.0001;
	n = (n > 255)?255:n;
	}
    nl = strlen(i0->String)*n;
    if (nl <= 63)
        {
	tree->String = tree->Types.StringBuf;
	}
    else
        {
	tree->Alloc = 1;
	tree->String = nmSysMalloc(nl+1);
	}

    ptr = tree->String;
    l = strlen(i0->String);
    ptr[0] = '\0';
    if (l) for(i=0;i<n;i++)
        {
	strcpy(ptr,i0->String);
	ptr += l;
	}
    return 0;
    }


int exp_fn_reverse(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int len,i;
    char ch;
    if (i0 && (i0->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING)
	{
	mssError(1,"EXP","reverse() expects one string parameter");
	return -1;
	}
    if (tree->Alloc && tree->String)
	nmSysFree(tree->String);
    tree->DataType = DATA_T_STRING;
    len = strlen(i0->String);
    if (len >= 64)
	{
	tree->String = nmSysMalloc(len+1);
	tree->Alloc = 1;
	}
    else
	{
	tree->Alloc = 0;
	tree->String = tree->Types.StringBuf;
	}
    strcpy(tree->String, i0->String);
    for(i=0;i<len/2;i++)
	{
	ch = tree->String[i];
	tree->String[i] = tree->String[len-i-1];
	tree->String[len-i-1] = ch;
	}
    return 0;
    }


int exp_fn_lztrim(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    if (!i0 || i0->Flags & EXPR_F_NULL) 
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING) 
        {
	mssError(1,"EXP","lztrim() only works on STRING data types");
	return -1;
	}
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	}
    tree->DataType = DATA_T_STRING;
    ptr = i0->String;
    while(*ptr == '0' && (ptr[1] >= '0' && ptr[1] <= '9')) ptr++;
    tree->String = ptr;
    tree->Alloc = 0;
    return 0;
    }


int exp_fn_ltrim(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    if (!i0 || i0->Flags & EXPR_F_NULL) 
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING) 
        {
	mssError(1,"EXP","ltrim() only works on STRING data types");
	return -1;
	}
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	}
    tree->DataType = DATA_T_STRING;
    ptr = i0->String;
    while(*ptr == ' ') ptr++;
    tree->String = ptr;
    tree->Alloc = 0;
    return 0;
    }


int exp_fn_rtrim(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;
    int n;

    if (!i0 || i0->Flags & EXPR_F_NULL) 
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING) 
        {
	mssError(1,"EXP","rtrim() only works on STRING data types");
	return -1;
	}
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	}
    tree->Alloc = 0;
    tree->DataType = DATA_T_STRING;
    ptr = i0->String + strlen(i0->String);
    while(ptr > i0->String && ptr[-1] == ' ') ptr--;
    if (ptr == i0->String + strlen(i0->String))
        {
	/** optimization for strings are still the same **/
	tree->String = i0->String;
	}
    else
        {
	/** have to copy because we removed spaces **/
	n = ptr - i0->String;
	if (n < 63)
	    {
	    tree->String = tree->Types.StringBuf;
	    memcpy(tree->String, i0->String, n);
	    tree->String[n] = '\0';
	    tree->Alloc = 0;
	    }
	else
	    {
	    tree->String = (char*)nmSysMalloc(n+1);
	    memcpy(tree->String, i0->String, n);
	    tree->String[n] = '\0';
	    tree->Alloc = 1;
	    }
	}
    return 0;
    }


int exp_fn_right(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n,i;

    if (!i0 || !i1 || i0->Flags & EXPR_F_NULL || i1->Flags & EXPR_F_NULL) 
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER) 
        {
	mssError(1,"EXP","Invalid datatypes in right() function - takes (string,integer)");
	return -1;
	}
    if (tree->Alloc && tree->String)
	{
	nmSysFree(tree->String);
	}
    n = strlen(i0->String);
    i = i1->Integer;
    if (i>n) i = n;
    if (i < 0) i = 0;
    tree->DataType = DATA_T_STRING;
    tree->String = i0->String + (n - i);
    tree->Alloc = 0;
    return 0;
    }


int exp_fn_substring(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int i,n;
    char* ptr;

    if (!i0 || !i1 || i0->Flags & EXPR_F_NULL || i1->Flags & EXPR_F_NULL) 
        {
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER) 
        {
	mssError(1,"EXP","Invalid datatypes in substring() - takes (string,integer,[integer])");
	return -1;
	}
    if (i2 && i2->DataType != DATA_T_INTEGER) 
        {
	mssError(1,"EXP","Invalid datatypes in substring() - takes (string,integer,[integer])");
	return -1;
	}
    n = strlen(i0->String);
    i = i1->Integer-1;
    if (i<0) i = 0;
    if (i > n) i = n;
    ptr = i0->String + i;
    i = i2?(i2->Integer):(strlen(ptr));
    if (i < 0) i = 0;
    if (i > strlen(ptr)) i = strlen(ptr);

    /** Ok, got position and length.  Now make new string in tree-> **/
    if (tree->Alloc && tree->String)
        {
	nmSysFree(tree->String);
	tree->Alloc = 0;
	}
    if (i<64)
        {
	tree->String = tree->Types.StringBuf;
	tree->Alloc = 0;
	}
    else
        {
	tree->String = (char*)nmSysMalloc(i+1);
	tree->Alloc = 1;
	}
    memcpy(tree->String, ptr, i);
    tree->String[i] = '\0';
    tree->DataType = DATA_T_STRING;
    return 0;
    }


/*** Pad string expression i0 with integer expression i1 number of spaces ***/
int exp_fn_ralign(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n;
    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER)
        {
	mssError(1,"EXP","ralign() requires string parameter #1 and integer parameter #2");
	return -1;
	}
    tree->DataType = DATA_T_STRING;
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    n = strlen(i0->String);
    if (tree->Alloc && tree->String) nmSysFree(tree->String);
    if (n >= i1->Integer)
        {
	tree->Alloc = 0;
	tree->String = i0->String;
	}
    else
        {
	if (i1->Integer>=64)
	    {
	    tree->Alloc = 1;
	    tree->String = (char*)nmSysMalloc(i1->Integer+1);
	    }
	else
	    {
	    tree->Alloc = 0;
	    tree->String = tree->Types.StringBuf;
	    }
	sprintf(tree->String,"%*.*s",i1->Integer,i1->Integer,i0->String);
	}
    return 0;
    }


/** escape(string, escchars, badchars) **/
int exp_fn_escape(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;
    char* dst;
    char* escchars;
    int esccnt,len;
    tree->DataType = DATA_T_STRING;
    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","escape() requires two or three string parameters");
	return -1;
	}
    if (i2 && i2->DataType != DATA_T_STRING)
	{
	mssError(1,"EXP","the optional third escape() parameter must be a string");
	return -1;
	}
    if (i2 && !(i2->Flags & EXPR_F_NULL) && (ptr=strpbrk(i0->String, i2->String)) != NULL)
	{
	mssError(1,"EXP","WARNING!! String contains invalid character asc=%d", (int)(*ptr));
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i1->Flags & EXPR_F_NULL)
	escchars = "";
    else
	escchars = i1->String;
    if (tree->Alloc && tree->String) nmSysFree(tree->String);
    tree->Alloc = 0;
    ptr = strpbrk(i0->String, escchars);
    if (!ptr) ptr = strchr(i0->String, '\\');
    if (!ptr)
	{
	/** shortcut if no need to escape anything **/
	tree->Alloc = 0;
	tree->String = i0->String;
	return 0;
	}
    esccnt = 1;
    ptr++;
    while(*ptr)
	{
	if (strchr(escchars, *ptr) || *ptr == '\\') esccnt++;
	ptr++;
	}
    len = strlen(i0->String);
    if (len+esccnt < 64)
	{
	tree->Alloc = 0;
	tree->String = tree->Types.StringBuf;
	}
    else
	{
	tree->Alloc = 1;
	tree->String = nmSysMalloc(len+esccnt+1);
	}
    ptr = i0->String;
    dst = tree->String;
    while(*ptr)
	{
	if (strchr(escchars, *ptr) || *ptr == '\\')
	    *(dst++) = '\\';
	*(dst++) = *(ptr++);
	}
    *dst = '\0';
    return 0;
    }


int exp_fn_quote(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int len,quotecnt;
    char* ptr;
    char* dst;
    tree->DataType = DATA_T_STRING;
    if (i0 && (i0->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING || i1)
        {
	mssError(1,"EXP","quote() requires one string parameter");
	return -1;
	}
    len = strlen(i0->String);
    ptr = i0->String;
    quotecnt = 0;
    while(*ptr)
	{
	if (*ptr == '\\' || *ptr == '"') quotecnt++;
	ptr++;
	}
    if (tree->Alloc && tree->String) nmSysFree(tree->String);
    tree->Alloc = 0;
    if (len + quotecnt + 2 < 64)
	{
	tree->Alloc = 0;
	tree->String = tree->Types.StringBuf;
	}
    else
	{
	tree->Alloc = 1;
	tree->String = nmSysMalloc(len + quotecnt + 2 + 1);
	}
    ptr = i0->String;
    dst = tree->String;
    *(dst++) = '"';
    while(*ptr)
	{
	if (*ptr == '\\' || *ptr == '"')
	    *(dst++) = '\\';
	*(dst++) = *(ptr++);
	}
    *(dst++) = '"';
    *dst = '\0';
    return 0;
    }


int exp_fn_substitute(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* subst_ptr;
    char* second_colon_ptr;
    char* close_tag_ptr;
    char* nl_ptr;
    char fieldname[64];
    char objname[64];
    XString dest;
    int subst_pos;
    int placeholder_len;
    int replace_len;
    pObject obj;
    int i;
    int t;
    int found;
    ObjData od;
    char* attr_string;
    int rval;
    int subst_len_this_line;
    int subst_cnt_this_line;
    int last_nl_pos;
    int ign_char_cnt;
    int len;
    char* obj_name_list[64];
    char* obj_remap_list[64];
    char* tmpstr;
    int n_obj_names = -1;
    char* tmpptr;
    char* eq_ptr;
    int fn_rval = -1;

    xsInit(&dest);

    /** Validate the params **/
    if (i0 && !i2 && i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	fn_rval = 0;
	goto out;
	}
    if (!i0 || i2 || i0->DataType != DATA_T_STRING || (i1 && !(i1->Flags & EXPR_F_NULL) && i1->DataType != DATA_T_STRING))
	{
	mssError(1,"EXP","substitute() requires one or two string parameters");
	goto out;
	}

    /** Object name/remapping list? **/
    if (i1 && !(i1->Flags & EXPR_F_NULL))
	{
	n_obj_names = 0;
	tmpstr = nmSysStrdup(i1->String);
	tmpptr = strtok(tmpstr, ",");
	while(tmpptr && n_obj_names < 64)
	    {
	    eq_ptr = strchr(tmpptr, '=');
	    if (!eq_ptr)
		{
		/** No equals sign, so no remapping **/
		obj_name_list[n_obj_names] = nmSysStrdup(tmpptr);
		obj_remap_list[n_obj_names] = nmSysStrdup(tmpptr);
		}
	    else
		{
		/** This one remaps the object name **/
		obj_name_list[n_obj_names] = nmSysStrdup(tmpptr);
		*strchr(obj_name_list[n_obj_names], '=') = '\0';
		obj_remap_list[n_obj_names] = nmSysStrdup(eq_ptr+1);
		}
	    n_obj_names++;
	    tmpptr = strtok(NULL, ",");
	    }
	nmSysFree(tmpstr);
	}

    xsCopy(&dest, i0->String, -1);
    xsConcatenate(&dest, "\n", 1);
    subst_pos = 0;
    subst_ptr = dest.String;
    subst_len_this_line = 0;
    subst_cnt_this_line = 0;
    last_nl_pos = -1;
    while(subst_ptr)
	{
	nl_ptr = strchr(subst_ptr, '\n');
	subst_ptr = strstr(subst_ptr, "[:");

	/** Passing a newline \n character? **/
	if (nl_ptr && ((subst_ptr && nl_ptr < subst_ptr) || !subst_ptr))
	    {
	    ign_char_cnt = strspn(dest.String + last_nl_pos + 1, " ,\t.%");
	    if (subst_len_this_line == 0 && ign_char_cnt == nl_ptr - (dest.String + last_nl_pos + 1) && subst_cnt_this_line > 0)
		{
		/** No substitution content this line, and nothing static either.  Delete the line. **/
		xsSubst(&dest, last_nl_pos+1, ign_char_cnt+1, "", 0);
		subst_ptr = dest.String + last_nl_pos + 1;
		subst_cnt_this_line = 0;
		continue;
		}
	    else
		{
		/** A new line.  Reset the counters. **/
		subst_len_this_line = 0;
		last_nl_pos = nl_ptr - dest.String;
		subst_cnt_this_line = 0;
		}
	    }

	/** Found a substitution?  Pull out the object name (optional) and field name (required) **/
	if (subst_ptr)
	    {
	    subst_pos = subst_ptr - dest.String;
	    second_colon_ptr = strchr(subst_ptr+2, ':');
	    close_tag_ptr = strchr(subst_ptr, ']');
	    if (!close_tag_ptr)
		break;
	    if (second_colon_ptr > close_tag_ptr)
		second_colon_ptr = NULL;
	    if (second_colon_ptr)
		{
		strtcpy(objname, subst_ptr+2, sizeof(objname));
		strtcpy(fieldname, second_colon_ptr+1, sizeof(fieldname));
		}
	    else
		{
		strtcpy(fieldname, subst_ptr+2, sizeof(fieldname));
		objname[0] = '\0';
		}
	    if (strchr(objname,':')) *strchr(objname, ':') = '\0';
	    if (strchr(fieldname,']')) *strchr(fieldname, ']') = '\0';
	    placeholder_len = close_tag_ptr - subst_ptr + 1;

	    /** Next, lookup the field/object. **/
	    obj = NULL;
	    t = DATA_T_UNAVAILABLE;
	    if (n_obj_names >= 0)
		{
		/** Using a remapping or scope-specification list, look up there. **/
		if (!objname[0])
		    strtcpy(objname, obj_name_list[0], sizeof(objname));
		found=0;
		for(i=0;i<n_obj_names;i++)
		    {
		    if (!strcmp(obj_name_list[i], objname))
			{
			found=1;
			strtcpy(objname, obj_remap_list[i], sizeof(objname));
			break;
			}
		    }
		if (!found)
		    {
		    mssError(1,"EXP","substitute(): object name '%s' not in list",objname);
		    goto out;
		    }
		}

	    if (objname[0])
		{
		/** We have an object name, look it up **/
		found = 0;
		for(i=0;i<objlist->nObjects;i++)
		    {
		    if (objlist->Names[i] && !strcmp(objlist->Names[i], objname))
			{
			found = 1;
			obj = objlist->Objects[i];
			break;
			}
		    }
		if (!found)
		    {
		    /** no such object **/
		    mssError(1,"EXP","substitute(): no such object '%s'",objname);
		    goto out;
		    }
		if (obj)
		    {
		    t = objGetAttrType(obj, fieldname);
		    if (t <= 0)
			{
			/** no such field **/
			mssError(1,"EXP","substitute(): no such attribute '%s' in object '%s'", fieldname, objname);
			goto out;
			}
		    }
		}
	    else
		{
		/** No object name, see if any obj has the requested attribute **/
		for(i=0;i<objlist->nObjects;i++)
		    {
		    if (objlist->Objects[i])
			{
			t = objGetAttrType(objlist->Objects[i], fieldname);
			if (t > 0)
			    {
			    obj = objlist->Objects[i];
			    break;
			    }
			}
		    }
		if (!obj)
		    {
		    /** no such field **/
		    /*mssError(1,"EXP","substitute(): no such attribute '%s'", fieldname);
		    return -1;*/
		    /** We can't error out here, in case we are evaluating before
		     ** any objects are set in the objlist.  So we return empty.
		     **/
		    attr_string = NULL;
		    }
		}

	    /** Ok, found an object containing the attribute.  Get the string value of the attribute. **/
	    if (obj && t > 0)
		{
		rval = objGetAttrValue(obj, fieldname, t, &od);
		if (rval == 1)
		    {
		    attr_string = NULL;
		    }
		else if (rval < 0)
		    {
		    mssError(0, "EXP", "substitute(): error with attribute '%s'", fieldname);
		    goto out;
		    }
		else
		    {
		    if (t == DATA_T_DATETIME || t == DATA_T_MONEY || t == DATA_T_STRING)
			attr_string = objDataToStringTmp(t, od.Generic, 0);
		    else
			attr_string = objDataToStringTmp(t, &od, 0);
		    }
		}
	    else
		{
		/** object not currently instantiated -- such as outer join type situation **/
		attr_string = NULL;
		}

	    /** Got the string - do the replacement - auto-trim all fields. **/
	    if (!attr_string) attr_string = "";
	    while(*attr_string == ' ') attr_string++;
	    attr_string = nmSysStrdup(attr_string);
	    replace_len = strlen(attr_string);
	    while (replace_len && attr_string[replace_len-1] == ' ')
		{
		attr_string[replace_len-1] = '\0';
		replace_len--;
		}
	    subst_len_this_line += replace_len;
	    subst_cnt_this_line++;
	    xsSubst(&dest, subst_pos, placeholder_len, attr_string, replace_len);
	    nmSysFree(attr_string);

	    subst_ptr = dest.String + subst_pos + replace_len;
	    }
	}

    /** Remove the trailing \n we added before. **/
    len = strlen(dest.String);
    if (len && dest.String[len-1] == '\n')
	xsSubst(&dest, len-1, 1, "", 0);

    /** Set replacement value **/
    if (tree->Alloc && tree->String) nmSysFree(tree->String);
    tree->Alloc = 0;
    if (strlen(dest.String) >= 64)
	{
	tree->Alloc = 1;
	tree->String = nmSysStrdup(dest.String);
	}
    else
	{
	tree->String = tree->Types.StringBuf;
	strcpy(tree->Types.StringBuf, dest.String);
	}

    /** Successful exit. **/
    fn_rval = 0;

  out:
    if (n_obj_names > 0)
	{
	for(i=0;i<n_obj_names;i++)
	    {
	    nmSysFree(obj_name_list[i]);
	    nmSysFree(obj_remap_list[i]);
	    }
	}
    xsDeInit(&dest);
    return fn_rval;
    }


int exp_fn_eval(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression eval_exp, parent;
    int rval;
    if (i0 && !i1 && i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING || i1)
        {
	mssError(1,"EXP","eval() requires one string parameter");
	return -1;
	}
    for(parent=tree;parent->Parent;parent=parent->Parent);
    eval_exp = expCompileExpression(i0->String, objlist, parent->LxFlags, parent->CmpFlags);
    if (!eval_exp) return -1;
    if ((rval=expEvalTree(eval_exp, objlist)) < 0)
	{
	expFreeExpression(eval_exp);
	return -1;
	}
    expCopyValue(eval_exp, tree, 1);
    expFreeExpression(eval_exp);
    return rval;
    }


int exp_fn_round(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int dec = 0;
    int i, v;
    double dv;
    long long mt, mv;
    if (!i0)
	{
	mssError(1,"EXP","round() requires a numeric parameter and an optional integer parameter");
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL) || (i1 && (i1->Flags & EXPR_F_NULL)))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if ((i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE && i0->DataType != DATA_T_MONEY) || (i1 && i1->DataType != DATA_T_INTEGER) || (i1 && i2))
	{
	mssError(1,"EXP","round() requires a numeric parameter and an optional integer parameter");
	return -1;
	}
    if (i1) dec = i1->Integer;
    tree->DataType = i0->DataType;
    switch(i0->DataType)
	{
	case DATA_T_INTEGER:
	    tree->Integer = i0->Integer;
	    if (dec < 0)
		{
		v = 1;
		for(i=dec;i<0;i++) v *= 10;
		if (tree->Integer > 0)
		    tree->Integer += (v/2);
		else
		    tree->Integer -= (v/2);
		tree->Integer /= v;
		tree->Integer *= v;
		}
	    break;

	case DATA_T_DOUBLE:
	    tree->Types.Double = i0->Types.Double;
	    dv = 1;
	    for(i=dec;i<0;i++) dv *= 10;
	    for(i=0;i<dec;i++) dv /= 10;
	    tree->Types.Double = tree->Types.Double/dv;
	    if (tree->Types.Double > 0)
		tree->Types.Double = floor(tree->Types.Double + 0.5);
	    else
		tree->Types.Double = ceil(tree->Types.Double - 0.5);
	    tree->Types.Double = tree->Types.Double*dv;
	    break;

	case DATA_T_MONEY:
	    mt = i0->Types.Money.WholePart * 10000 + i0->Types.Money.FractionPart;
	    if (dec < 4)
		{
		mv = 1;
		for(i=dec;i<4;i++) mv *= 10;
		if (mt > 0)
		    mt += (mv/2);
		else
		    mt -= (mv/2);
		mt /= mv;
		mt *= mv;
		}
	    tree->Types.Money.WholePart = mt/10000;
	    mt = mt % 10000;
	    if (mt < 0)
		{
		mt += 10000;
		tree->Types.Money.WholePart -= 1;
		}
	    tree->Types.Money.FractionPart = mt;
	    break;
	}
    return 0;
    }


int
exp_fn_dateadd_mod_add(int v1, int v2, int mod, int* overflow)
    {
    int rv;
    rv = (v1 + v2)%mod;
    *overflow = (v1 + v2)/mod;
    if (rv < 0)
	{
	*overflow -= 1;
	rv += mod;
	}
    return rv;
    }


int exp_fn_dateadd(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int diff_sec, diff_min, diff_hr, diff_day, diff_mo, diff_yr;
    int carry;

    /** checks **/
    if (!i0 || (i0->Flags & EXPR_F_NULL) || i0->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "dateadd() first parameter must be non-null string or keyword date part");
	return -1;
	}
    if ((i1 && (i1->Flags & EXPR_F_NULL)) || (i2 && (i2->Flags & EXPR_F_NULL)))
	{
	tree->DataType = DATA_T_DATETIME;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i1 || i1->DataType != DATA_T_INTEGER)
	{
	mssError(1, "EXP", "dateadd() second parameter must be an integer (amount to add/subtract)");
	return -1;
	}
    if (!i2 || i2->DataType != DATA_T_DATETIME)
	{
	mssError(1, "EXP", "dateadd() third parameter must be a datetime type");
	return -1;
	}

    /** ok, we're good.  set up for returning the value **/
    tree->DataType = DATA_T_DATETIME;
    memcpy(&tree->Types.Date, &i2->Types.Date, sizeof(DateTime));
    diff_sec = diff_min = diff_hr = diff_day = diff_mo = diff_yr = 0;
    if (!strcmp(i0->String, "second"))
	diff_sec = i1->Integer;
    else if (!strcmp(i0->String, "minute"))
	diff_min = i1->Integer;
    else if (!strcmp(i0->String, "hour"))
	diff_hr = i1->Integer;
    else if (!strcmp(i0->String, "day"))
	diff_day = i1->Integer;
    else if (!strcmp(i0->String, "month"))
	diff_mo = i1->Integer;
    else if (!strcmp(i0->String, "year"))
	diff_yr = i1->Integer;
    else
	{
	mssError(1, "EXP", "dateadd() first parameter must be a valid date part (second/minute/hour/day/month/year)");
	return -1;
	}

    /** Do the add **/
    tree->Types.Date.Part.Second = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Second, diff_sec, 60, &carry);
    diff_min += carry;
    tree->Types.Date.Part.Minute = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Minute, diff_min, 60, &carry);
    diff_hr += carry;
    tree->Types.Date.Part.Hour = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Hour, diff_hr, 24, &carry);
    diff_day += carry;

    /** Now add months and years **/
    tree->Types.Date.Part.Month = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Month, diff_mo, 12, &carry);
    diff_yr += carry;
    tree->Types.Date.Part.Year += diff_yr;

    /** Adding days is more complicated **/
    while (diff_day > 0)
	{
	tree->Types.Date.Part.Day++;
	if (tree->Types.Date.Part.Day >= (obj_month_days[tree->Types.Date.Part.Month] + ((tree->Types.Date.Part.Month==1 && IS_LEAP_YEAR(tree->Types.Date.Part.Year+1900))?1:0)))
	    {
	    tree->Types.Date.Part.Day = 0;
	    tree->Types.Date.Part.Month = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Month, 1, 12, &carry);
	    tree->Types.Date.Part.Year += carry;
	    }
	diff_day--;
	}
    while (diff_day < 0)
	{
	if (tree->Types.Date.Part.Day == 0)
	    {
	    tree->Types.Date.Part.Day = (obj_month_days[exp_fn_dateadd_mod_add(tree->Types.Date.Part.Month, -1, 12, &carry)] + ((tree->Types.Date.Part.Month==2 && IS_LEAP_YEAR(tree->Types.Date.Part.Year+1900))?1:0)) - 1;
	    tree->Types.Date.Part.Month = exp_fn_dateadd_mod_add(tree->Types.Date.Part.Month, -1, 12, &carry);
	    tree->Types.Date.Part.Year += carry;
	    }
	else
	    {
	    tree->Types.Date.Part.Day--;
	    }
	diff_day++;
	}

    return 0;
    }


int exp_fn_truncate(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int dec = 0;
    int i, v;
    double dv;
    long long mt, mv;
    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE && i0->DataType != DATA_T_MONEY) || (i1 && i1->DataType != DATA_T_INTEGER) || (i1 && i2))
	{
	mssError(1,"EXP","truncate() requires a numeric parameter and an optional integer parameter");
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL) || (i1 && (i1->Flags & EXPR_F_NULL)))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i1) dec = i1->Integer;
    tree->DataType = i0->DataType;
    switch(i0->DataType)
	{
	case DATA_T_INTEGER:
	    tree->Integer = i0->Integer;
	    if (dec < 0)
		{
		v = 1;
		for(i=dec;i<0;i++) v *= 10;
		tree->Integer /= v;
		tree->Integer *= v;
		}
	    break;

	case DATA_T_DOUBLE:
	    tree->Types.Double = i0->Types.Double;
	    dv = 1;
	    for(i=dec;i<0;i++) dv *= 10;
	    for(i=0;i<dec;i++) dv /= 10;
	    tree->Types.Double = tree->Types.Double/dv;
	    if (tree->Types.Double > 0)
		tree->Types.Double = floor(tree->Types.Double + 0.000001);
	    else
		tree->Types.Double = ceil(tree->Types.Double - 0.000001);
	    tree->Types.Double = tree->Types.Double*dv;
	    break;

	case DATA_T_MONEY:
	    mt = i0->Types.Money.WholePart * 10000 + i0->Types.Money.FractionPart;
	    if (dec < 4)
		{
		mv = 1;
		for(i=dec;i<4;i++) mv *= 10;
		mt /= mv;
		mt *= mv;
		}
	    tree->Types.Money.WholePart = mt/10000;
	    mt = mt % 10000;
	    if (mt < 0)
		{
		mt += 10000;
		tree->Types.Money.WholePart -= 1;
		}
	    tree->Types.Money.FractionPart = mt;
	    break;
	}
    return 0;
    }

/*** constrain(value, min, max) ***/
int exp_fn_constrain(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0 || !i1 || !i2 || (i0->DataType != i1->DataType) || i0->DataType != i2->DataType || !(i0->DataType == DATA_T_INTEGER || i0->DataType == DATA_T_MONEY || i0->DataType == DATA_T_DOUBLE))
	{
	mssError(1,"EXP","constrain() requires three numeric parameters of the same data type");
	return -1;
	}
    tree->DataType = i0->DataType;
    if ((i0->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}

    /* check min */
    if (!(i1->Flags & EXPR_F_NULL))
	{
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
		if (objDataCompare(i0->DataType, &(i0->Integer), i1->DataType, &(i1->Integer)) < 0)
		    {
		    tree->Integer = i1->Integer;
		    return 0;
		    }
		break;
	    case DATA_T_DOUBLE: 
		if (objDataCompare(i0->DataType, &(i0->Types.Double), i1->DataType, &(i1->Types.Double)) < 0)
		    {
		    tree->Types.Double = i1->Types.Double;
		    return 0;
		    }
		break;
	    case DATA_T_MONEY: 
		if (objDataCompare(i0->DataType, &(i0->Types.Money), i1->DataType, &(i1->Types.Money)) < 0)
		    {
		    tree->Types.Money = i1->Types.Money;
		    return 0;
		    }
		break;
	    }
	}

    /* check max */
    if (!(i2->Flags & EXPR_F_NULL))
	{
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
		if (objDataCompare(i0->DataType, &(i0->Integer), i2->DataType, &(i2->Integer)) > 0)
		    {
		    tree->Integer = i2->Integer;
		    return 0;
		    }
		break;
	    case DATA_T_DOUBLE: 
		if (objDataCompare(i0->DataType, &(i0->Types.Double), i2->DataType, &(i2->Types.Double)) > 0)
		    {
		    tree->Types.Double = i2->Types.Double;
		    return 0;
		    }
		break;
	    case DATA_T_MONEY: 
		if (objDataCompare(i0->DataType, &(i0->Types.Money), i2->DataType, &(i2->Types.Money)) > 0)
		    {
		    tree->Types.Money = i2->Types.Money;
		    return 0;
		    }
		break;
	    }
	}

    /* go with actual value */
    switch(i0->DataType)
	{
	case DATA_T_INTEGER:
	    tree->Integer = i0->Integer;
	    break;
	case DATA_T_DOUBLE: 
	    tree->Types.Double = i0->Types.Double;
	    break;
	case DATA_T_MONEY: 
	    tree->Types.Money = i0->Types.Money;
	    break;
	}

    return 0;
    }


int exp_fn_radians(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;
    double pi = 3.14159265358979323846;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","radians() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    tree->Types.Double = d*pi/180.0;
    return 0;
    }

int exp_fn_degrees(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;
    double pi = 3.14159265358979323846;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","degrees() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    tree->Types.Double = d*180.0/pi;
    return 0;
    }


int exp_fn_sin(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","sin() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    tree->Types.Double = sin(d);
    return 0;
    }


int exp_fn_cos(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","cos() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    tree->Types.Double = cos(d);
    return 0;
    }


int exp_fn_tan(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","tan() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    tree->Types.Double = tan(d);
    return 0;
    }


int exp_fn_asin(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","asin() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    errno = 0;
    tree->Types.Double = asin(d);
    if (errno == EDOM)
	tree->Flags |= EXPR_F_NULL;
    return 0;
    }


int exp_fn_acos(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","acos() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    errno = 0;
    tree->Types.Double = acos(d);
    if (errno == EDOM)
	tree->Flags |= EXPR_F_NULL;
    return 0;
    }


int exp_fn_atan(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","atan() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    errno = 0;
    tree->Types.Double = atan(d);
    if (errno == EDOM)
	tree->Flags |= EXPR_F_NULL;
    return 0;
    }


int exp_fn_atan2(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d1, d2;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || !i1 || (i1->DataType != DATA_T_INTEGER && i1->DataType != DATA_T_DOUBLE))
	{
	mssError(1,"EXP","atan2() requires two numeric (integer or double) parameters");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d1 = i0->Integer;
    else
	d1 = i0->Types.Double;
    if (i1->DataType == DATA_T_INTEGER)
	d2 = i1->Integer;
    else
	d2 = i1->Types.Double;
    errno = 0;
    tree->Types.Double = atan2(d1, d2);
    if (errno == EDOM)
	tree->Flags |= EXPR_F_NULL;
    return 0;
    }


int exp_fn_sqrt(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double d;

    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","sqrt() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = DATA_T_DOUBLE;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	d = i0->Integer;
    else
	d = i0->Types.Double;
    errno = 0;
    tree->Types.Double = sqrt(d);
    if (errno == EDOM)
	tree->Flags |= EXPR_F_NULL;
    return 0;
    }


int exp_fn_square(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0 || (i0->DataType != DATA_T_INTEGER && i0->DataType != DATA_T_DOUBLE) || i1)
	{
	mssError(1,"EXP","square() requires a single numeric (integer or double) parameter");
	return -1;
	}
    tree->DataType = i0->DataType;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (i0->DataType == DATA_T_INTEGER)
	tree->Integer = i0->Integer * i0->Integer;
    else
	tree->Types.Double = i0->Types.Double * i0->Types.Double;
    return 0;
    }


int exp_fn_count(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp;

    /** Init the Aggregate computation expression? **/
    if (!tree->AggExp)
        {
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_PLUS;
	tree->AggExp->DataType = DATA_T_INTEGER;
	tree->AggExp->Integer = 0;
	tree->AggExp->AggLevel = 1;
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_INTEGER;
	new_exp->DataType = DATA_T_INTEGER;
	new_exp->Integer = 1;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_INTEGER;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	}
    if (tree->Flags & EXPR_F_AGGLOCKED) return 0;

    /** Compute the possibly incremented value **/
    if (!(i0->Flags & EXPR_F_NULL))
        {
	expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 0);
	}

    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_avg(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp, new_subexp;
    pExpression sumexp, cntexp, s_accumexp, c_accumexp, valueexp;

    /** Init the Aggregate computation expression? **/
    if (!tree->AggExp)
        {
	/** Overall expression is sum(x) / count(x) **/
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_DIVIDE;
	tree->AggExp->DataType = DATA_T_INTEGER;
	tree->AggExp->Integer = 0;
	tree->AggExp->AggLevel = 1;
	tree->AggExp->Flags |= EXPR_F_NULL;

	/** Now for the sum(x) part **/
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_PLUS;
	new_exp->DataType = DATA_T_INTEGER;
	new_exp->Integer = 0;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	new_subexp = expAllocExpression();
	new_subexp->NodeType = EXPR_N_INTEGER;
	new_subexp->AggLevel = 1;
	expAddNode(new_exp, new_subexp);
	new_subexp = expAllocExpression();
	new_subexp->NodeType = EXPR_N_INTEGER;
	new_subexp->AggLevel = 1;
	expAddNode(new_exp, new_subexp);

	/** Now for the count(x) part **/
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_PLUS;
	new_exp->DataType = DATA_T_INTEGER;
	new_exp->Integer = 0;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	new_subexp = expAllocExpression();
	new_subexp->NodeType = EXPR_N_INTEGER;
	new_subexp->AggLevel = 1;
	expAddNode(new_exp, new_subexp);
	new_subexp = expAllocExpression();
	new_subexp->NodeType = EXPR_N_INTEGER;
	new_subexp->DataType = DATA_T_INTEGER;
	new_subexp->Integer = 1;
	new_subexp->AggLevel = 1;
	expAddNode(new_exp,new_subexp);
	}

    if (!(i0->Flags & EXPR_F_NULL) && !(tree->Flags & EXPR_F_AGGLOCKED))
        {
	/** Just to make things easier to read... **/
	sumexp = (pExpression)(tree->AggExp->Children.Items[0]);
	cntexp = (pExpression)(tree->AggExp->Children.Items[1]);
	s_accumexp = (pExpression)(sumexp->Children.Items[0]);
	valueexp = (pExpression)(sumexp->Children.Items[1]);
	c_accumexp = (pExpression)(cntexp->Children.Items[0]);

	/** Init the sum() part? **/
	if (tree->AggCount == 0) 
	    {
	    tree->AggExp->Flags &= ~EXPR_F_NULL;
	    sumexp->DataType = i0->DataType;
	    sumexp->String = sumexp->Types.StringBuf;
	    sumexp->Alloc = 0;
	    sumexp->String[0] = '\0';
	    sumexp->Integer = 0;
	    sumexp->Types.Double = 0;
	    sumexp->Types.Money.FractionPart = 0;
	    sumexp->Types.Money.WholePart = 0;

	    cntexp->Integer = 0;
	    }

	/** Do the count() part. **/
	expCopyValue(cntexp, c_accumexp, 0);

	/** Do the sum() part. **/
	expCopyValue(sumexp, s_accumexp, 1);
	expCopyValue(i0, valueexp, 0);
	valueexp->NodeType = expDataTypeToNodeType(i0->DataType);
	s_accumexp->NodeType = expDataTypeToNodeType(sumexp->DataType);

	/** Eval the expression and copy the result. **/
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 1);
	tree->AggCount++;

	/**
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
	        tree->DataType = DATA_T_INTEGER;
	        tree->AggCount++;
	        tree->AggExp->Integer += i0->Integer;
	        tree->Integer = tree->AggExp->Integer / tree->AggCount;
		break;
	    case DATA_T_DOUBLE:
	        tree->DataType = DATA_T_DOUBLE;
		tree->AggCount++;
		tree->AggExp->Types.Double += i0->Types.Double;
		tree->Types.Double = tree->AggExp->Types.Double / tree->AggCount;
		break;
	    }
	**/
	}
    else
        {
	if (tree->AggExp->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_sum(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp;

    if (!tree->AggExp)
        {
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_PLUS;
	tree->AggExp->DataType = DATA_T_INTEGER;
	tree->AggExp->Integer = 0;
	tree->AggExp->AggLevel = 1;
	tree->AggExp->Flags |= EXPR_F_NULL;
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_INTEGER;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	new_exp = expAllocExpression();
	new_exp->NodeType = EXPR_N_INTEGER;
	new_exp->AggLevel = 1;
	expAddNode(tree->AggExp, new_exp);
	}
    /*if (tree->AggCount == 0) tree->Flags |= EXPR_F_NULL;*/

    if (!(i0->Flags & EXPR_F_NULL) && !(tree->Flags & EXPR_F_AGGLOCKED))
        {
	if (tree->AggCount == 0) 
	    {
	    tree->AggExp->Flags &= ~EXPR_F_NULL;
	    tree->AggExp->DataType = i0->DataType;
	    if (tree->AggExp->Alloc && tree->AggExp->String) nmSysFree(tree->AggExp->String);
	    tree->AggExp->Alloc = 0;
	    tree->AggExp->String = tree->AggExp->Types.StringBuf;
	    tree->AggExp->String[0] = '\0';
	    tree->AggExp->Integer = 0;
	    tree->AggExp->Types.Double = 0;
	    tree->AggExp->Types.Money.FractionPart = 0;
	    tree->AggExp->Types.Money.WholePart = 0;
	    }
	expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[0]), 1);
	expCopyValue(i0, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	((pExpression)(tree->AggExp->Children.Items[1]))->NodeType = expDataTypeToNodeType(i0->DataType);
	((pExpression)(tree->AggExp->Children.Items[0]))->NodeType = expDataTypeToNodeType(tree->AggExp->DataType);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 1);
	tree->AggCount++;
	}
    else
        {
	if (tree->AggExp->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_max(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression exp,subexp;

    /** Initialize the aggexp tree? **/
    if (!tree->AggExp)
        {
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_FUNCTION;
	tree->AggExp->AggLevel = 1;
	tree->AggExp->Name = "condition";
	tree->AggExp->NameAlloc = 0;
	tree->AggExp->Flags |= EXPR_F_NULL;
	exp = expAllocExpression();
	exp->NodeType = EXPR_N_COMPARE;
	exp->CompareType = MLX_CMP_GREATER;
	exp->AggLevel = 1;
	subexp = expAllocExpression();
	subexp->AggLevel = 1;
	expAddNode(tree->AggExp, exp);
	expAddNode(tree->AggExp, expLinkExpression(i0));
	expAddNode(tree->AggExp, subexp);
	expAddNode(exp, expLinkExpression(i0));
	expAddNode(exp, expLinkExpression(subexp));
	}

    if (!(i0->Flags & EXPR_F_NULL) && !(tree->Flags & EXPR_F_AGGLOCKED))
        {
	if (tree->AggCount == 0) 
	    {
	    tree->AggExp->Flags &= ~EXPR_F_NULL;
	    tree->DataType = i0->DataType;
	    tree->String = tree->Types.StringBuf;
	    tree->Alloc = 0;
	    tree->String[0] = '\0';
	    tree->Integer = 0;
	    tree->Types.Double = 0;
	    tree->Types.Money.FractionPart = 0;
	    tree->Types.Money.WholePart = 0;
	    expCopyValue(i0,tree,0);
	    }
	subexp = ((pExpression)(tree->AggExp->Children.Items[2]));
	subexp->NodeType = expDataTypeToNodeType(i0->DataType);
	expCopyValue(tree, subexp, 1);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 1);
	tree->AggCount++;
	}
    else
        {
	if (tree->AggExp->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }

int exp_fn_min(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression exp,subexp;

    /** Initialize the aggexp tree? **/
    if (!tree->AggExp)
        {
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_FUNCTION;
	tree->AggExp->AggLevel = 1;
	tree->AggExp->Name = "condition";
	tree->AggExp->NameAlloc = 0;
	tree->AggExp->Flags |= EXPR_F_NULL;
	exp = expAllocExpression();
	exp->NodeType = EXPR_N_COMPARE;
	exp->CompareType = MLX_CMP_LESS;
	exp->AggLevel = 1;
	subexp = expAllocExpression();
	subexp->AggLevel = 1;
	expAddNode(tree->AggExp, exp);
	expAddNode(tree->AggExp, expLinkExpression(i0));
	expAddNode(tree->AggExp, subexp);
	expAddNode(exp, expLinkExpression(i0));
	expAddNode(exp, expLinkExpression(subexp));
	}

    if (!(i0->Flags & EXPR_F_NULL) && !(tree->Flags & EXPR_F_AGGLOCKED))
        {
	if (tree->AggCount == 0) 
	    {
	    tree->AggExp->Flags &= ~EXPR_F_NULL;
	    tree->DataType = i0->DataType;
	    tree->String = tree->Types.StringBuf;
	    tree->String[0] = '\0';
	    tree->Alloc = 0;
	    tree->Integer = 0;
	    tree->Types.Double = 0;
	    tree->Types.Money.FractionPart = 0;
	    tree->Types.Money.WholePart = 0;
	    expCopyValue(i0,tree,0);
	    }
	subexp = ((pExpression)(tree->AggExp->Children.Items[2]));
	subexp->NodeType = expDataTypeToNodeType(i0->DataType);
	expCopyValue(tree, subexp, 1);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 1);
	tree->AggCount++;
	}
    else
        {
	if (tree->AggExp->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_first(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!(tree->Flags & EXPR_F_AGGLOCKED) && !(i0->Flags & EXPR_F_NULL))
	{
	if (tree->AggCount == 0) 
	    {
	    expCopyValue(i0, tree, 1);
	    }
	tree->AggCount++;
	}
    else
	{
	if (tree->AggCount == 0) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_last(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!(tree->Flags & EXPR_F_AGGLOCKED) && !(i0->Flags & EXPR_F_NULL))
	{
	expCopyValue(i0, tree, 1);
	tree->AggCount++;
	}
    else
	{
	if (tree->AggCount == 0) tree->Flags |= EXPR_F_NULL;
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int
exp_internal_DefineFunctions()
    {

	/** Function list for EXPR_N_FUNCTION nodes **/
	xhAdd(&EXP.Functions, "getdate", (char*)exp_fn_getdate);
	xhAdd(&EXP.Functions, "user_name", (char*)exp_fn_user_name);
	xhAdd(&EXP.Functions, "convert", (char*)exp_fn_convert);
	xhAdd(&EXP.Functions, "wordify", (char*)exp_fn_wordify);
	xhAdd(&EXP.Functions, "abs", (char*)exp_fn_abs);
	xhAdd(&EXP.Functions, "ascii", (char*)exp_fn_ascii);
	xhAdd(&EXP.Functions, "condition", (char*)exp_fn_condition);
	xhAdd(&EXP.Functions, "charindex", (char*)exp_fn_charindex);
	xhAdd(&EXP.Functions, "upper", (char*)exp_fn_upper);
	xhAdd(&EXP.Functions, "lower", (char*)exp_fn_lower);
	xhAdd(&EXP.Functions, "char_length", (char*)exp_fn_char_length);
	xhAdd(&EXP.Functions, "datepart", (char*)exp_fn_datepart);
	xhAdd(&EXP.Functions, "isnull", (char*)exp_fn_isnull);
	xhAdd(&EXP.Functions, "ltrim", (char*)exp_fn_ltrim);
	xhAdd(&EXP.Functions, "lztrim", (char*)exp_fn_lztrim);
	xhAdd(&EXP.Functions, "rtrim", (char*)exp_fn_rtrim);
	xhAdd(&EXP.Functions, "substring", (char*)exp_fn_substring);
	xhAdd(&EXP.Functions, "right", (char*)exp_fn_right);
	xhAdd(&EXP.Functions, "ralign", (char*)exp_fn_ralign);
	xhAdd(&EXP.Functions, "replicate", (char*)exp_fn_replicate);
	xhAdd(&EXP.Functions, "reverse", (char*)exp_fn_reverse);
	xhAdd(&EXP.Functions, "escape", (char*)exp_fn_escape);
	xhAdd(&EXP.Functions, "quote", (char*)exp_fn_quote);
	xhAdd(&EXP.Functions, "substitute", (char*)exp_fn_substitute);
	xhAdd(&EXP.Functions, "eval", (char*)exp_fn_eval);
	xhAdd(&EXP.Functions, "round", (char*)exp_fn_round);
	xhAdd(&EXP.Functions, "dateadd", (char*)exp_fn_dateadd);
	xhAdd(&EXP.Functions, "truncate", (char*)exp_fn_truncate);
	xhAdd(&EXP.Functions, "constrain", (char*)exp_fn_constrain);
	xhAdd(&EXP.Functions, "sin", (char*)exp_fn_sin);
	xhAdd(&EXP.Functions, "cos", (char*)exp_fn_cos);
	xhAdd(&EXP.Functions, "tan", (char*)exp_fn_tan);
	xhAdd(&EXP.Functions, "asin", (char*)exp_fn_asin);
	xhAdd(&EXP.Functions, "acos", (char*)exp_fn_acos);
	xhAdd(&EXP.Functions, "atan", (char*)exp_fn_atan);
	xhAdd(&EXP.Functions, "atan2", (char*)exp_fn_atan2);
	xhAdd(&EXP.Functions, "sqrt", (char*)exp_fn_sqrt);
	xhAdd(&EXP.Functions, "square", (char*)exp_fn_square);
	xhAdd(&EXP.Functions, "degrees", (char*)exp_fn_degrees);
	xhAdd(&EXP.Functions, "radians", (char*)exp_fn_radians);

	xhAdd(&EXP.Functions, "count", (char*)exp_fn_count);
	xhAdd(&EXP.Functions, "avg", (char*)exp_fn_avg);
	xhAdd(&EXP.Functions, "sum", (char*)exp_fn_sum);
	xhAdd(&EXP.Functions, "max", (char*)exp_fn_max);
	xhAdd(&EXP.Functions, "min", (char*)exp_fn_min);
	xhAdd(&EXP.Functions, "first", (char*)exp_fn_first);
	xhAdd(&EXP.Functions, "last", (char*)exp_fn_last);

	/** Reverse functions **/
	xhAdd(&EXP.ReverseFunctions, "isnull", (char*)exp_fn_reverse_isnull);

    return 0;
    }
