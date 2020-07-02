#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "charsets.h"
#include "cxss/cxss.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

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
	if (objDataToMoney(i1->DataType, vptr, &(tree->Types.Money)) < 0)
	    {
	    mssError(1,"EXP","convert(): invalid conversion to money value");
	    return -1;
	    }
	}
    else if (!strcmp(i0->String,"datetime"))
        {
	tree->DataType = DATA_T_DATETIME;
	if (i1->Flags & EXPR_F_NULL) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	if (objDataToDateTime(i1->DataType, vptr, &(tree->Types.Date), NULL) < 0)
	    {
	    mssError(1,"EXP","convert(): invalid conversion to datetime value");
	    return -1;
	    }
	}
    else
        {
	mssError(1,"EXP","convert(): datatype '%s' is invalid", i0->String);
	return -1;
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
    if ((i0->Flags & EXPR_F_NULL) || i0->String[0] == '\0')
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
	if (objlist)
	    {
	    i1->ObjDelayChangeMask |= (objlist->ModCoverageMask & i1->ObjCoverageMask);
	    i2->ObjDelayChangeMask |= (objlist->ModCoverageMask & i2->ObjCoverageMask);
	    }
	return 0;
	}
    if (i0->Integer != 0)
        {
	/** True, return 2nd argument i1 **/
	if (exp_internal_EvalTree(i1,objlist) < 0)
	    {
	    return -1;
	    }
	if (i0->Flags & EXPR_F_INDETERMINATE)
	    {
	    if (exp_internal_EvalTree(i2,objlist) < 0)
		{
		return -1;
		}
	    }
	if (i2->AggLevel > 0)
	    {
	    if (exp_internal_EvalAggregates(i2,objlist) < 0)
		return -1;
	    }
	else
	    {
	    if (objlist)
		i2->ObjDelayChangeMask |= (objlist->ModCoverageMask & i2->ObjCoverageMask);
	    }
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
	if (i0->Flags & EXPR_F_INDETERMINATE)
	    {
	    if (exp_internal_EvalTree(i1,objlist) < 0)
		{
		return -1;
		}
	    }
	if (i1->AggLevel > 0)
	    {
	    if (exp_internal_EvalAggregates(i1,objlist) < 0)
		return -1;
	    }
	else
	    {
	    if (objlist)
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
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One string parameter required for upper()");
	return -1;
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
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One string parameter required for lower()");
	return -1;
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
    struct tm tval;

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
	mssError(1,"EXP","param 1 to datepart() must be month, day, weekday, year, hour, minute, or second");
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
    else if (!strcasecmp(i0->String,"weekday"))
	{
	tval.tm_sec = dtptr->Part.Second;
	tval.tm_min = dtptr->Part.Minute;
	tval.tm_hour = dtptr->Part.Hour;
	tval.tm_mday = dtptr->Part.Day + 1;
	tval.tm_mon = dtptr->Part.Month; /* already base 0 */
	tval.tm_year = dtptr->Part.Year; /* already base 1900 */
	tval.tm_isdst = 0;
	mktime(&tval);
	tree->Integer = tval.tm_wday + 1;
	}
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
	{
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

int exp_fn_nullif(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0 || !i1)
        {
	mssError(1,"EXP","nullif() requires two parameters");
	return -1;
	}
    tree->DataType = i0->DataType;
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    tree->CompareType = MLX_CMP_EQUALS;
    if (expEvalCompare(tree, objlist) < 0)
	return -1;
    tree->DataType = i0->DataType;
    if (tree->Integer == 0)
	{
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: tree->Integer = i0->Integer; break;
	    case DATA_T_STRING: tree->String = i0->String; tree->Alloc = 0; break;
	    default: memcpy(&(tree->Types), &(i0->Types), sizeof(tree->Types));
	    }
	}
    else
	{
	tree->Flags |= EXPR_F_NULL;
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

//i0 is haystack, i1 is needle, i2 is replacement
int exp_fn_replace(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
	printf("Replacing\n");
    char* repstr;
    long long newsize;
    char* srcptr;
    char* searchptr;
    char* dstptr;
    int searchlen, replen;
    if ((i0 && (i0->Flags & EXPR_F_NULL)) || (i1 && (i1->Flags & EXPR_F_NULL)))
	{
	printf("UHOH\n");
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING || !i1 || i1->DataType != DATA_T_STRING || !i2 || (!(i2->Flags & EXPR_F_NULL) && i2->DataType != DATA_T_STRING))
	{
	mssError(1,"EXP","replace() expects three string parameters (str,search,replace)");
	return -1;
	}
    if (i2->Flags & EXPR_F_NULL)
	repstr = "";
    else
	repstr = i2->String;

    if (tree->Alloc && tree->String)
	nmSysFree(tree->String);
    tree->Alloc = 0;
    if (i1->String[0] == '\0')
	{
	tree->String = i0->String;
	return 0;
	}

    /** in determining new size, we assume the search string occurs throughout **/
    newsize = strlen(i0->String);
    replen = strlen(repstr);
    searchlen = strlen(i1->String);
    if (replen > searchlen)
	newsize = (newsize * replen) / searchlen + 1;//why * and / instead of + and -
    if (newsize >= 0x7FFFFFFFLL)
	{
	mssError(1,"EXP","replace(): out of memory");
	return -1;
	}
    if (newsize <= sizeof(tree->Types.StringBuf))
	{
	tree->String = tree->Types.StringBuf;
	tree->Alloc = 0;
	}
    else
	{
	tree->String = nmSysMalloc(newsize);
	tree->Alloc = 1;
	}

    /** Now do the string replace **/
    srcptr = i0->String;
    dstptr = tree->String;
    *dstptr = '\0';
    while (*srcptr)
	{
	searchptr = strstr(srcptr, i1->String);
	if (!searchptr)
	    {
	    /** copy remainder **/
	    strcpy(dstptr, srcptr);
	    break;
	    }
	if (searchptr > srcptr)
	    {
	    /** copy stuff in between matches **/
	    memcpy(dstptr, srcptr, searchptr - srcptr);
	    dstptr += (searchptr - srcptr);
	    *dstptr = '\0';
	    srcptr = searchptr;
	    }
	/** do the replace **/
	strcpy(dstptr, repstr);
	dstptr += replen;
	srcptr += searchlen;
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
    if (i0 && (i0->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i1 || (i1->Flags & EXPR_F_NULL))
	escchars = "";
    else
	escchars = i1->String;
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
    int i,j;
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
    int (*getfn)();
    int (*typefn)();

    xsInit(&dest);

    tree->DataType = DATA_T_STRING;

    /** Validate the params **/
    if (!objlist || (i0 && !i2 && i0->Flags & EXPR_F_NULL))
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
	    if (n_obj_names >= 0 && objname[0])
		{
		/** Using a remapping or scope-specification list, look up there. **/
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
			getfn = objlist->GetAttrFn[i];
			typefn = objlist->GetTypeFn[i];
			break;
			}
		    }
		if (!found)
		    {
		    /** no such object **/
		    if ((tree->Flags & EXPR_F_RUNSERVER) && (objlist->MainFlags & EXPR_MO_RUNSTATIC))
			{
			/** Not an error - just treat as a null. **/
			tree->Flags |= EXPR_F_NULL;
			fn_rval = 0;
			goto out;
			}
		    else
			{
			mssError(1,"EXP","substitute(): no such object '%s'",objname);
			goto out;
			}
		    }
		if (obj)
		    {
		    if (!typefn) typefn = objGetAttrType;
		    t = typefn(obj, fieldname);
		    if (t < 0)
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
		if (n_obj_names > 0)
		    {
		    /** Check the remapping list if there is one **/
		    for(i=0;i<n_obj_names;i++)
			{
			found = 0;
			for(j=0;j<objlist->nObjects;j++)
			    {
			    if (objlist->Objects[j] && objlist->Names[j] && !strcmp(objlist->Names[j], obj_remap_list[i]))
				{
				typefn = objlist->GetTypeFn[j];
				if (!typefn) typefn = objGetAttrType;
				t = typefn(objlist->Objects[j], fieldname);
				if (t > 0)
				    {
				    obj = objlist->Objects[j];
				    getfn = objlist->GetAttrFn[j];
				    found=1;
				    break;
				    }
				}
			    }
			if (found)
			    break;
			}
		    }
		else
		    {
		    /** If no remapping list, check the object list **/
		    for(i=0;i<objlist->nObjects;i++)
			{
			if (objlist->Objects[i])
			    {
			    typefn = objlist->GetTypeFn[i];
			    if (!typefn) typefn = objGetAttrType;
			    t = typefn(objlist->Objects[i], fieldname);
			    if (t > 0)
				{
				obj = objlist->Objects[i];
				getfn = objlist->GetAttrFn[i];
				break;
				}
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
		if (!getfn) getfn = objGetAttrValue;
		rval = getfn(obj, fieldname, t, &od);
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
    if (!objlist || (i0 && !i1 && i0->Flags & EXPR_F_NULL))
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
	    mt = ((long long)(i0->Types.Money.WholePart)) * 10000 + i0->Types.Money.FractionPart;
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


int exp_fn_dateformat(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;

    /** checks **/
    if (!i0 || !i1)
	{
	mssError(1, "EXP", "dateformat() takes two parameters: (datetime, string)");
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
	tree->DataType = DATA_T_STRING;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_DATETIME)
	{
	mssError(1, "EXP", "dateformat() first parameter must be a date");
	return -1;
	}
    if (!i1 || i1->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "dateformat() second parameter must be a string");
	return -1;
	}

    ptr = objFormatDateTmp(&i0->Types.Date, i1->String);
    if (!ptr)
	return -1;

    if (tree->Alloc && tree->String) nmSysFree(tree->String);
    tree->Alloc = 0;
    if (strlen(ptr) >= 64)
	{
	tree->Alloc = 1;
	tree->String = nmSysStrdup(ptr);
	}
    else
	{
	tree->String = tree->Types.StringBuf;
	strcpy(tree->Types.StringBuf, ptr);
	}

    return 0;
    }


int exp_fn_datediff(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int yr, mo;
    int sign = 1;
    pExpression tmp;

    /** checks **/
    if (!i0 || (i0->Flags & EXPR_F_NULL) || i0->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "datediff() first parameter must be non-null string or keyword date part");
	return -1;
	}
    if (!i1 || i1->DataType != DATA_T_DATETIME || !i2 || i2->DataType != DATA_T_DATETIME)
	{
	mssError(1, "EXP", "datediff() second and third parameters must be datetime types");
	return -1;
	}
    if ((i1 && (i1->Flags & EXPR_F_NULL)) || (i2 && (i2->Flags & EXPR_F_NULL)))
	{
	tree->DataType = DATA_T_INTEGER;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    tree->DataType = DATA_T_INTEGER;

    /** Swap operands if we're diffing backwards **/
    if (i2->Types.Date.Value < i1->Types.Date.Value)
	{
	sign = -1;
	tmp = i2;
	i2 = i1;
	i1 = tmp;
	}

    /** choose which date part.  Typecasts are to make sure we're working
     ** with signed values.
     **/
    if (strcmp(i0->String, "year") == 0)
	{
	tree->Integer = i2->Types.Date.Part.Year - (int)i1->Types.Date.Part.Year;
	}
    else if (strcmp(i0->String, "month") == 0)
	{
	tree->Integer = i2->Types.Date.Part.Year - (int)i1->Types.Date.Part.Year;
	tree->Integer = tree->Integer*12 + i2->Types.Date.Part.Month - (int)i1->Types.Date.Part.Month;
	}
    else
	{
	/** fun.  working with the day part of stuff gets tricky -- leap
	 ** years and all that stuff.  Count the days up manually.
	 **/
	tree->Integer = - i1->Types.Date.Part.Day;
	yr = i1->Types.Date.Part.Year;
	mo = i1->Types.Date.Part.Month;
	while (yr < i2->Types.Date.Part.Year || (yr == i2->Types.Date.Part.Year &&  mo < i2->Types.Date.Part.Month))
	    {
	    tree->Integer += obj_month_days[mo];
	    if (IS_LEAP_YEAR(yr+1900) && mo == 1) /* Feb of a leap year */
		tree->Integer += 1;
	    mo++;
	    if (mo == 12)
		{
		mo = 0;
		yr++;
		}
	    }
	tree->Integer += i2->Types.Date.Part.Day;

	/** Hours, minutes, seconds? **/
	if (strcmp(i0->String, "day") != 0)
	    {
	    /** has to be H, M, or S **/
	    tree->Integer = tree->Integer*24 + i2->Types.Date.Part.Hour - (int)i1->Types.Date.Part.Hour;
	    if (strcmp(i0->String, "hour") != 0)
		{
		/** has to be M or S **/
		tree->Integer = tree->Integer*60 + i2->Types.Date.Part.Minute - (int)i1->Types.Date.Part.Minute;
		if (strcmp(i0->String, "minute") != 0)
		    {
		    /** has to be S **/
		    if (strcmp(i0->String, "second") != 0)
			{
			mssError(1,"EXP","Invalid date part '%s' for datediff()", i0->String);
			return -1;
			}
		    tree->Integer = tree->Integer*60 + i2->Types.Date.Part.Second - (int)i1->Types.Date.Part.Second;
		    }
		}
	    }
	}

    /** Invert sign? **/
    tree->Integer *= sign;

    return 0;
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

    /** Adjust it **/
    objDateAdd(&tree->Types.Date, diff_sec, diff_min, diff_hr, diff_day, diff_mo, diff_yr);

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
	    mt = ((long long)(i0->Types.Money.WholePart)) * 10000 + i0->Types.Money.FractionPart;
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


int exp_fn_has_endorsement(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* context;
    if (!i0 || (i0->DataType != DATA_T_STRING) || (i1 && i1->DataType != DATA_T_STRING))
	{
	mssError(1,"EXP","has_endorsement() requires one or two string parameters");
	return -1;
	}
    tree->DataType = DATA_T_INTEGER;
    tree->Integer = 0;
    if (i0->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i1 || (i1->Flags & EXPR_F_NULL))
	context="";
    else
	context = i1->String;
    if (cxssHasEndorsement(i0->String, context) > 0)
	tree->Integer = 1;
    return 0;
    }


int exp_fn_rand(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    SHA256_CTX hashctx;
    unsigned char tmpseed[SHA256_DIGEST_LENGTH];
    unsigned long long val;
	
	if (!objlist)
	    {
	    return 0;
	    }

	/** Seed provided? **/
	if (i0 && !(i0->Flags & EXPR_F_NULL))
	    {
	    if (i0->DataType != DATA_T_INTEGER)
		{
		mssError(1,"EXP","rand() seed, if provided, must be an integer");
		return -1;
		}
	    memset(objlist->Random, 0, sizeof(objlist->Random));
	    memcpy(objlist->Random, &(i0->Integer), sizeof(i0->Integer));
	    objlist->RandomInit = 1;
	    }

	/** Seed initialized? **/
	if (!objlist->RandomInit)
	    {
	    /** Init seed by deriving from system master random data **/
	    SHA256_Init(&hashctx);
	    SHA256_Update(&hashctx, (unsigned char*)&(objlist->PSeqID), sizeof(objlist->PSeqID));
	    SHA256_Update(&hashctx, EXP.Random, sizeof(EXP.Random));
	    SHA256_Update(&hashctx, (unsigned char*)&(objlist->PSeqID), sizeof(objlist->PSeqID));
	    SHA256_Update(&hashctx, EXP.Random, sizeof(EXP.Random));
	    SHA256_Final(objlist->Random, &hashctx);
	    objlist->RandomInit = 1;

	    /** Update the master seed **/
	    memcpy(tmpseed, EXP.Random, sizeof(tmpseed));
	    SHA256(tmpseed, sizeof(tmpseed), EXP.Random);
	    }

	/** Chain our local seed **/
	memcpy(tmpseed, objlist->Random, sizeof(tmpseed));
	SHA256(tmpseed, sizeof(tmpseed), objlist->Random);

	/** Get the 64-bit value and convert to double **/
	memcpy(&val, objlist->Random, sizeof(val));
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = (double)val / (double)ULLONG_MAX;

    return 0;
    }


int exp_fn_hash(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    /*EVP_MD_CTX *hashctx = NULL;
    const EVP_MD *hashtype = NULL;
    unsigned char hashvalue[EVP_MAX_MD_SIZE];*/
    unsigned char hashvalue[SHA512_DIGEST_LENGTH];
    unsigned int hashlen;

	/** Init the hash function **/
	/*hashctx = EVP_MD_CTX_new();
	if (!hashctx)
	    {
	    mssError(1, "EXP", "hash(): could not allocate hash digest context");
	    goto error;
	    }*/
	if (!i0 || (i0->Flags & EXPR_F_NULL) || i0->DataType != DATA_T_STRING)
	    {
	    mssError(1, "EXP", "hash() requires hash type as its first parameter");
	    goto error;
	    }
	if (!i1)
	    {
	    mssError(1, "EXP", "hash() requires a string as its second parameter");
	    goto error;
	    }
	if (i1->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    tree->DataType = DATA_T_STRING;
	    return 0;
	    }
	if (i1->DataType != DATA_T_STRING)
	    {
	    mssError(1, "EXP", "hash() requires a string as its second parameter");
	    goto error;
	    }
	/*if (!strcmp(i0->String, "md5"))
	    hashtype = EVP_md5();
	else if (!strcmp(i0->String, "sha1"))
	    hashtype = EVP_sha1();
	else if (!strcmp(i0->String, "sha256"))
	    hashtype = EVP_sha256();
	else if (!strcmp(i0->String, "sha384"))
	    hashtype = EVP_sha384();
	else if (!strcmp(i0->String, "sha512"))
	    hashtype = EVP_sha512();
	if (!hashtype)
	    {
	    mssError(1, "EXP", "hash(): invalid or unsupported hash type %s", i0->String);
	    goto error;
	    }
	EVP_DigestInit_ex(hashctx, hashtype, NULL);
	EVP_DigestUpdate(hashctx, i1->String, strlen(i1->String));
	EVP_DigestFinal_ex(hashctx, hashvalue, &hashlen);*/
	if (!strcmp(i0->String, "md5"))
	    {
	    MD5((unsigned char*)i1->String, strlen(i1->String), hashvalue);
	    hashlen = 16;
	    }
	else if (!strcmp(i0->String, "sha1"))
	    {
	    SHA1((unsigned char*)i1->String, strlen(i1->String), hashvalue);
	    hashlen = 20;
	    }
	else if (!strcmp(i0->String, "sha256"))
	    {
	    SHA256((unsigned char*)i1->String, strlen(i1->String), hashvalue);
	    hashlen = 32;
	    }
	else if (!strcmp(i0->String, "sha384"))
	    {
	    SHA384((unsigned char*)i1->String, strlen(i1->String), hashvalue);
	    hashlen = 48;
	    }
	else if (!strcmp(i0->String, "sha512"))
	    {
	    SHA512((unsigned char*)i1->String, strlen(i1->String), hashvalue);
	    hashlen = 64;
	    }
	tree->String = nmSysMalloc(hashlen * 2 + 1);
	if (!tree->String)
	    {
	    mssError(1, "EXP", "hash(): out of memory");
	    goto error;
	    }
	qpfPrintf(NULL, tree->String, hashlen * 2 + 1, "%*STR&HEX", hashlen, hashvalue);

	/*EVP_MD_CTX_free(hashctx);*/
	return 0;

    error:
	/*if (hashctx)
	    EVP_MD_CTX_free(hashctx);*/
	return -1;
    }


int exp_fn_hmac(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    /*EVP_MD_CTX *hashctx = NULL;*/
    const EVP_MD *hashtype = NULL;
    unsigned char hashvalue[EVP_MAX_MD_SIZE];
    unsigned int hashlen;

	/** Init the hash function **/
	/*hashctx = EVP_MD_CTX_new();
	if (!hashctx)
	    {
	    mssError(1, "EXP", "hash(): could not allocate hash digest context");
	    goto error;
	    }*/
	if (!i0 || (i0->Flags & EXPR_F_NULL) || i0->DataType != DATA_T_STRING)
	    {
	    mssError(1, "EXP", "hash() requires hash type as its first parameter");
	    goto error;
	    }
	if (!i1)
	    {
	    mssError(1, "EXP", "hash() requires a string as its second parameter");
	    goto error;
	    }
	if (i1->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    tree->DataType = DATA_T_STRING;
	    return 0;
	    }
	if (i1->DataType != DATA_T_STRING)
	    {
	    mssError(1, "EXP", "hash() requires a string (input data) as its second parameter");
	    goto error;
	    }
	if (i2->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    tree->DataType = DATA_T_STRING;
	    return 0;
	    }
	if (i2->DataType != DATA_T_STRING)
	    {
	    mssError(1, "EXP", "hash() requires a string (key) as its third parameter");
	    goto error;
	    }
	if (!strcmp(i0->String, "md5"))
	    hashtype = EVP_md5();
	else if (!strcmp(i0->String, "sha1"))
	    hashtype = EVP_sha1();
	else if (!strcmp(i0->String, "sha256"))
	    hashtype = EVP_sha256();
	else if (!strcmp(i0->String, "sha384"))
	    hashtype = EVP_sha384();
	else if (!strcmp(i0->String, "sha512"))
	    hashtype = EVP_sha512();
	if (!hashtype)
	    {
	    mssError(1, "EXP", "hash(): invalid or unsupported hash type %s", i0->String);
	    goto error;
	    }
	/*EVP_DigestInit_ex(hashctx, hashtype, NULL);
	EVP_DigestUpdate(hashctx, i1->String, strlen(i1->String));
	EVP_DigestFinal_ex(hashctx, hashvalue, &hashlen);*/
	HMAC(hashtype, (unsigned char*)i2->String, strlen(i2->String), (unsigned char*)i1->String, strlen(i1->String), hashvalue, &hashlen);
	tree->String = nmSysMalloc(hashlen * 2 + 1);
	if (!tree->String)
	    {
	    mssError(1, "EXP", "hash(): out of memory");
	    goto error;
	    }
	qpfPrintf(NULL, tree->String, hashlen * 2 + 1, "%*STR&HEX", hashlen, hashvalue);

	/*EVP_MD_CTX_free(hashctx);*/
	return 0;

    error:
	/*if (hashctx)
	    EVP_MD_CTX_free(hashctx);*/
	return -1;
    }


/** pbkdf2('algo', passwd, salt, iterations) **/
int exp_fn_pbkdf2(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression i3 = (tree->Children.nItems >= 4)?(tree->Children.Items[3]):NULL;
    const EVP_MD *hashtype = NULL;
    unsigned char hashvalue[EVP_MAX_MD_SIZE];
    unsigned int hashlen;

	/** Validate parameters **/
	if (!i0 || !i1 || !i2 || !i3)
	    {
	    mssError(1, "EXP", "function usage: pbkdf2('algo', password, salt, iterations)");
	    goto error;
	    }
	tree->DataType = DATA_T_STRING;

	/** Nulls? **/
	if ((i0->Flags | i1->Flags | i2->Flags | i3->Flags) & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Wrong data types? **/
	if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_STRING || i2->DataType != DATA_T_STRING || i3->DataType != DATA_T_INTEGER)
	    {
	    mssError(1, "EXP", "function usage: pbkdf2('algo', password, salt, iterations)");
	    goto error;
	    }

	/** Select a hash algorithm **/
	if (!strcmp(i0->String, "md5"))
	    hashtype = EVP_md5();
	else if (!strcmp(i0->String, "sha1"))
	    hashtype = EVP_sha1();
	else if (!strcmp(i0->String, "sha256"))
	    hashtype = EVP_sha256();
	else if (!strcmp(i0->String, "sha384"))
	    hashtype = EVP_sha384();
	else if (!strcmp(i0->String, "sha512"))
	    hashtype = EVP_sha512();
	if (!hashtype)
	    {
	    mssError(1, "EXP", "pbkdf2(): invalid or unsupported hash type %s", i0->String);
	    goto error;
	    }
	hashlen = EVP_MD_size(hashtype);

	/** Compute it **/
	if (PKCS5_PBKDF2_HMAC(i1->String, strlen(i1->String), (unsigned char*)i2->String, strlen(i2->String), i3->Integer, hashtype, hashlen, hashvalue) == 0)
	    {
	    mssError(1, "EXP", "pbkdf2(): operation failed");
	    goto error;
	    }

	/** Generate output **/
	if (hashlen*2+1 > 63)
	    {
	    tree->Alloc = 1;
	    tree->String = nmSysMalloc(hashlen*2+1);
	    if (!tree->String)
		{
		mssError(1, "EXP", "pbkdf2(): out of memory");
		goto error;
		}
	    }
	else
	    {
	    tree->Alloc = 0;
	    tree->String = tree->Types.StringBuf;
	    }
	qpfPrintf(NULL, tree->String, hashlen * 2 + 1, "%*STR&HEX", hashlen, hashvalue);

	return 0;

    error:
	return -1;
    }


int exp_fn_log10(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double n;

	if (!i0)
	    {
	    mssError(1, "EXP", "log10() requires a number as its first parameter");
	    goto error;
	    }
	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->DataType = DATA_T_DOUBLE;
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
		n = i0->Integer;
		break;
	    case DATA_T_DOUBLE:
		n = i0->Types.Double;
		break;
	    case DATA_T_MONEY:
		n = objDataToDouble(DATA_T_MONEY, &(i0->Types.Money));
		break;
	    default:
		mssError(1, "EXP", "log10() requires a number as its first parameter");
		goto error;
	    }
	if (n < 0)
	    {
	    mssError(1, "EXP", "log10(): cannot compute the logarithm of a negative number");
	    goto error;
	    }
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = log10(n);
	return 0;

    error:
	return -1;
    }


int exp_fn_power(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double n, p;

	if (!i0 || !i1)
	    {
	    mssError(1, "EXP", "power() requires numbers as its first and second parameters");
	    goto error;
	    }
	if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	    {
	    tree->DataType = DATA_T_DOUBLE;
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
		n = i0->Integer;
		break;
	    case DATA_T_DOUBLE:
		n = i0->Types.Double;
		break;
	    case DATA_T_MONEY:
		n = objDataToDouble(DATA_T_MONEY, &(i0->Types.Money));
		break;
	    default:
		mssError(1, "EXP", "power() requires a number as its first parameter");
		goto error;
	    }
	switch(i1->DataType)
	    {
	    case DATA_T_INTEGER:
		p = i1->Integer;
		break;
	    case DATA_T_DOUBLE:
		p = i1->Types.Double;
		break;
	    default:
		mssError(1, "EXP", "power() requires an integer or double as its second parameter");
		goto error;
	    }
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = pow(n, p);
	return 0;
    
    error:
	return -1;
    }


/*** Windowing Functions ***/

int exp_fn_row_number(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp;
    char newbuf[512];

    /** Init the Aggregate computation expression? **/
    if (!tree->AggExp)
        {
	tree->PrivateData = nmSysMalloc(512);
	if (!tree->PrivateData)
	    return -ENOMEM;
	memset(tree->PrivateData, 0, 512);
	tree->AggExp = expAllocExpression();
	tree->AggExp->NodeType = EXPR_N_PLUS;
	tree->AggExp->DataType = DATA_T_INTEGER;
	tree->AggExp->Integer = -1;
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

    /** Changed? **/
    if (tree->Children.nItems > 0)
	{
	memset(newbuf, 0, sizeof(newbuf));
	if (objBuildBinaryImage(newbuf, sizeof(newbuf), tree->Children.Items, tree->Children.nItems, objlist, 0) < 0)
	    return -1;
	if (memcmp(newbuf, tree->PrivateData, 512))
	    {
	    /** Reset count **/
	    memcpy(tree->PrivateData, newbuf, 512);
	    tree->AggExp->Integer = 0;
	    tree->AggCount = 0;
	    tree->AggValue = 0;
	    }
	}

    /** Compute the possibly incremented value **/
    //if (!(i0->Flags & EXPR_F_NULL))
    //    {
	expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 0);
	//}

    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


/*** Aggregate Functions ***/

int exp_fn_count(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp;

    if (!i0)
	{
	mssError(1,"EXP","count() requires a parameter");
	return -1;
	}

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

    if (!i0)
	{
	mssError(1,"EXP","avg() requires a parameter");
	return -1;
	}

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

    if (!i0)
	{
	mssError(1,"EXP","sum() requires a parameter");
	return -1;
	}

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

    if (!i0)
	{
	mssError(1,"EXP","max() requires a parameter");
	return -1;
	}

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

    if (!i0)
	{
	mssError(1,"EXP","min() requires a parameter");
	return -1;
	}

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
    if (!i0)
	{
	mssError(1,"EXP","first() requires a parameter");
	return -1;
	}
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
    if (!i0)
	{
	mssError(1,"EXP","last() requires a parameter");
	return -1;
	}
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

int exp_fn_utf8_overlong(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
	{
	printf("EXP\nRecieved String: %s\n", i0->String);
	char* str = chrNoOverlong(i0->String);
	printf("A");
	fflush(stdout);
	printf("Final str: %s\n", str);
	printf("B");
	fflush(stdout);
	tree->String = str;
	printf("C");
        fflush(stdout);
	return 0;
	}

int exp_fn_utf8_ascii(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    size_t charValue;
    
    tree->DataType = DATA_T_INTEGER;
    if (!i0)
        {
        mssError(1, "EXP", "Parameter required for ascii() function.");
        return -1;
        }
    if (i0->DataType != DATA_T_STRING)
        {
        mssError(1, "EXP", "ascii() function takes a string parameter.");
        return -1;
        }
    if ((i0->Flags & EXPR_F_NULL) || i0->String[0] == '\0')
        tree->Flags |= EXPR_F_NULL;
    
    charValue = chrGetCharNumber(i0->String);
    if (charValue == CHR_INVALID_CHAR)
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character");
        return -1;
        }
    else
        tree->Integer = charValue;
    return 0;
    }

int exp_fn_utf8_charindex(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    size_t position;
    
    tree->DataType = DATA_T_INTEGER;
    if (!i0 || !i1)
        {
        mssError(1, "EXP", "Two string parameters required for charindex()");
        return -1;
        }
    if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
        {
        tree->Flags |= EXPR_F_NULL;
        return 0;
        }
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_STRING)
        {
        mssError(1, "EXP", "Two string parameters required for charindex()");
        return -1;
        }
    position = chrSubstringIndex(i1->String, i0->String);
    if(position == CHR_INVALID_CHAR)
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character");
        return -1;
        }
    else if(position == CHR_NOT_FOUND)
        tree->Integer = 0;
    else
        tree->Integer = position + 1;
    return 0;
    }

int exp_fn_utf8_upper(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
        char * result;
        size_t bufferLength = 64;
        
        tree->DataType = DATA_T_STRING;
        if (!i0 || i0->DataType != DATA_T_STRING)
        {
            mssError(1, "EXP", "One string parameter required for upper()");
            return -1;
        }
        if (i0->Flags & EXPR_F_NULL)
        {
            tree->Flags |= EXPR_F_NULL;
            return 0;
        }
        if (tree->Alloc && tree->String)
            nmSysFree(tree->String);
        
        /** Get the lower case string **/
        result = chrToUpper(i0->String, tree->Types.StringBuf, &bufferLength);
        
        if(result)
        {
            tree->String = result;
            tree->Alloc = bufferLength ? 1 : 0;
            return 0;
        }
        else
        {
            mssError(1, "EXP", "String contains invalid UTF-8 character"); /* Also assumed if invalid conversion */
            return -1;
        }
    }

int exp_fn_utf8_lower(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char * result;
    size_t bufferLength = 64;
    
    tree->DataType = DATA_T_STRING;
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
        mssError(1, "EXP", "One string parameter required for lower()");
        return -1;
        }
    if (i0->Flags & EXPR_F_NULL)
        {
        tree->Flags |= EXPR_F_NULL;
        return 0;
        }
    if (tree->Alloc && tree->String)
        nmSysFree(tree->String);
    
    /** Get the lower case string **/
    result = chrToLower(i0->String, tree->Types.StringBuf, &bufferLength);
        
    if(result)
        {
        tree->String = result;
        tree->Alloc = bufferLength ? 1 : 0;
        return 0;
        }
    else
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character"); /* Also assumed if invalid conversion */
        return -1;
        }
    }
    

int exp_fn_utf8_char_length(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    size_t wideLen;
    
    tree->DataType = DATA_T_INTEGER;
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
        tree->Flags |= EXPR_F_NULL;
        return 0;
        }
    if (!i0 || i0->DataType != DATA_T_STRING) {
        mssError(1, "EXP", "One string parameter required for char_length()");
        return -1;
        }
        
    wideLen = chrCharLength(i0->String);
        
    if(wideLen == CHR_INVALID_CHAR)
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character");
        return -1;
        }
    tree->Integer = wideLen;
    return 0;
    }

int exp_fn_utf8_right(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    size_t returnCode;
    
    if (!i0 || !i1 || i0->Flags & EXPR_F_NULL || i1->Flags & EXPR_F_NULL)
        {
        tree->Flags |= EXPR_F_NULL;
        tree->DataType = DATA_T_STRING;
        return 0;
        }
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER)
        {
        mssError(1, "EXP", "Invalid datatypes in right() function - takes (string,integer)");
        return -1;
        }
    if (tree->Alloc && tree->String)
        {
        nmSysFree(tree->String);
        }
        
    tree->DataType = DATA_T_STRING;
    tree->Alloc = 0;
    tree->String = chrRight(i0->String, i1->Integer < 0 ? 0 : i1->Integer, &returnCode);
    if(!tree->String)
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character"); /* Assumed invalid UTF-8 char */
        return -1;
        }
    return 0;
    }

/*int exp_fn_utf8_substring(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
{
	return 0;
}*/

int exp_fn_nth(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    if (!i0)
	{
	mssError(1,"EXP","nth() requires two parameters");
	return -1;
	}
    if (!i1 || (i1->Flags & EXPR_F_NULL) || i1->DataType != DATA_T_INTEGER)
	{
	mssError(1,"EXP","nth() function must have a non-null integer second parameter");
	return -1;
	}
    if (tree->AggCount == 0 || tree->AggCount < i1->Integer)
	tree->Flags |= EXPR_F_NULL;
    if (!(tree->Flags & EXPR_F_AGGLOCKED) && !(i0->Flags & EXPR_F_NULL))
	{
	tree->AggCount++;
	if (i1->Integer == tree->AggCount)
	    {
	    expCopyValue(i0, tree, 1);
	    }
	}
    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }

int exp_fn_utf8_substring(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    size_t bufferLength = 64;
    size_t initialPosition;
    char * output;
    
    if (!i0 || !i1 || i0->Flags & EXPR_F_NULL || i1->Flags & EXPR_F_NULL)
        {
        tree->Flags |= EXPR_F_NULL;
        tree->DataType = DATA_T_STRING;
        return 0;
        }
    if (i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER)
        {
        mssError(1, "EXP", "Invalid datatypes in substring() - takes (string,integer,[integer])");
        return -1;
        }
    if (i2 && i2->DataType != DATA_T_INTEGER)
        {
        mssError(1, "EXP", "Invalid datatypes in substring() - takes (string,integer,[integer])");
        return -1;
        }
    
    /** Free any previous string in preparation for these results **/
    if (tree->Alloc && tree->String)
        {
        nmSysFree(tree->String);
        tree->Alloc = 0;
        }
    tree->DataType = DATA_T_STRING;
    
    initialPosition = i1->Integer < 1 ? 0 : i1->Integer - 1;
    output = chrSubstring(i0->String, initialPosition, i2->Integer < 0 ? 0 : i2->Integer + initialPosition, tree->Types.StringBuf, &bufferLength);
        
    if(output)
        {
        tree->String = output;
        tree->Alloc = bufferLength ? 1 : 0;
        return 0;
        }
    else
        {
        if(bufferLength == CHR_MEMORY_OUT)
            {
            mssError(1, "EXP", "Memory exhausted!");
            return -1;
            }
        else
            {
            mssError(1, "EXP", "String contains invalid UTF-8 character");
            return -1;   
            }
        }
    }


/*** Pad string expression i0 with integer expression i1 number of spaces ***/
int exp_fn_utf8_ralign(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char * returned;
    size_t bufferLength = 64;
    
    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_INTEGER)
        {
        mssError(1, "EXP", "ralign() requires string parameter #1 and integer parameter #2");
        return -1;
        }
    tree->DataType = DATA_T_STRING;
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
        {
        tree->Flags |= EXPR_F_NULL;
        return 0;
        }
    
    if (tree->Alloc && tree->String)
        nmSysFree(tree->String);
    
    returned = chrRightAlign(i0->String, i1->Integer, tree->Types.StringBuf, &bufferLength);
    
    if(returned)
        {
        tree->String = returned;
        tree->Alloc = bufferLength ? 1 : 0;
        return 0;
        }
    else
        {
        mssError(1, "EXP", "String contains invalid UTF-8 character");
        return -1; 
        }    
    }
	
/** escape(string, escchars, badchars) **/
int exp_fn_utf8_escape(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* output, *esc, *bad;
    size_t bufferLength = 64;
        
    tree->DataType = DATA_T_STRING;
    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || i1->DataType != DATA_T_STRING) 
        {
        mssError(1, "EXP", "escape() requires two or three string parameters");
        return -1;
        }
    if (i2 && i2->DataType != DATA_T_STRING) {
        mssError(1, "EXP", "the optional third escape() parameter must be a string");
        return -1;
        }
    if ((i0->Flags & EXPR_F_NULL))
        {
        tree->Flags |= EXPR_F_NULL;
        return 0;
        }
    if (tree->Alloc && tree->String)
        {
        nmSysFree(tree->String);
        tree->Alloc = 0;
        }
        
    /** Set the bad and escape strings. */
    esc = i1->Flags & EXPR_F_NULL ? "" : i1->String;
    bad = (i2 && !(i2->Flags & EXPR_F_NULL)) ? i2->String : "";
    
    /** Get the escaped string **/
    output = chrEscape(i0->String, esc, bad, tree->Types.StringBuf, &bufferLength);
        
    if(output)
        {
        tree->String = output;
        tree->Alloc = bufferLength? 1: 0;
        return 0;
        }
    else
        {
        if(bufferLength == CHR_BAD_CHAR)
            {
            mssError(1, "EXP", "WARNING!! String contains invalid character!");
            return -1;
            }
        else /* Should be an invalid UTF-8 char */
            {
            mssError(1, "EXP", "String contains invalid UTF-8 character");
            return -1;
            }
        }
    }

int exp_fn_utf8_reverse(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
	{
	int charLen, byteLen, a, i, end;
	char* temp;
    	char ch1, ch2, ch3, ch4;
    	char mask1 = 0x80, mask2 = 0xC0, mask3 = 0xE0, mask4 = 0xF0;
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
    	byteLen = strlen(i0->String);
    	if (byteLen >= 64)
        	{
        	tree->String = nmSysMalloc(byteLen+1);
        	tree->Alloc = 1;
        	}
    	else
        	{
        	tree->Alloc = 0;
        	tree->String = tree->Types.StringBuf;
        	}
    	strcpy(tree->String, i0->String);
    		
	/** Reversing string **/
	char    *scanl, *scanr, *scanr2, c;

	/* first reverse the string */
   	for (scanl= tree->String, scanr= tree->String + strlen(tree->String); scanl < scanr;)
        	c= *scanl, *scanl++= *--scanr, *scanr= c;

	/* then scan all bytes and reverse each multibyte character */
    	for (scanl= scanr= tree->String; c= *scanr++;) 
		{
        	if ( (c & 0x80) == 0) // ASCII char
            		scanl= scanr;
        	else if ( (c & 0xc0) == 0xc0 ) // start of multibyte
			{
            		scanr2= scanr;
            		switch (scanr - scanl) 
				{
                		case 4: c= *scanl, *scanl++= *--scanr, *scanr= c; // fallthrough
                		case 3: // fallthrough
                		case 2: c= *scanl, *scanl++= *--scanr, *scanr= c;
            			}
            		scanr= scanl= scanr2;
        		}
    		}



/*	i = 0;
	end = byteLen-1;
	temp = (char*)nmSysMalloc(sizeof(char) * byteLen+1);
	temp[byteLen] = '\0';
	charLen = chrCharLength(i0->String);
	for(a = 0; a < charLen; a++)
        	{
		ch1 = tree->String[i];
		if (ch1 < 0xC0)//maybe add helper functions instead of masks and validate following continuation bytes
			{
			printf("1\n");
			temp[end--] = ch1;
			i++;
			}
		else if (ch1 < 0xE0)
			{
			printf("2\n");
			ch2 = tree->String[++i];
			temp[end--] = ch2;
			temp[end--] = ch1;
        		i++;
			}
		else if (ch1 < 0xF0)
			{
			ch2 = tree->String[++i];
			ch3 = tree->String[++i];
			temp[end--] = ch3;
			temp[end--] = ch2;
                        temp[end--] = ch1;
			i++;
			}
		else// if ((ch1 & mask4) == 0xF0)
        		{
                        ch2 = tree->String[++i];    
			ch3 = tree->String[++i];
			ch4 = tree->String[++i];
			temp[end--] = ch4;
			temp[end--] = ch3;
			temp[end--] = ch2;
                        temp[end--] = ch1;
                        i++;                                                                                                                                                                                                               }
	       	}
*/
	//strcpy(tree->String, temp);
	
    	return 0;
	}

int exp_fn_utf8_replace(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
	printf("Starting\n");
    char *haystack, *needle, *replace;
    long long newsize, diff;
    char* pos;
    char *dstptr, *oldptr;
    size_t len_replace, len_needle, len_haystack, num_shifts;

	if ((i0 && (i0->Flags & EXPR_F_NULL)) || (i1 && (i1->Flags & EXPR_F_NULL)))
        {                                                                                                                                                                                                                  tree->Flags |= EXPR_F_NULL;                                                                                                                                                                                        tree->DataType = DATA_T_STRING;
        return 0;
        }
    if (!i0 || i0->DataType != DATA_T_STRING || !i1 || i1->DataType != DATA_T_STRING || !i2 || (!(i2->Flags & EXPR_F_NULL) && i2->DataType != DATA_T_STRING))
        {                                                                                                                                                                                                                  mssError(1,"EXP","replace() expects three string parameters (str,search,replace)");
        return -1;
        }
    if (i2->Flags & EXPR_F_NULL)
        replace = "";
    else
        replace = i2->String;

    if (tree->Alloc && tree->String)
        nmSysFree(tree->String);
    tree->Alloc = 0;
    	if (i1->String[0] == '\0')   
		{ 
		tree->String = i0->String;
        	return 0;
        	}
	printf("Passed init stuff\n");
	fflush(stdout);
	haystack = i0->String;
	needle = i1->String;
	printf("strcpy\n");
	len_haystack = strlen(haystack);
	len_needle = strlen(needle);
	len_replace = strlen(replace);
	printf("lens\n");
	fflush(stdout);
	dstptr = nmSysMalloc((len_haystack + 1) * sizeof(char));
	strcpy(dstptr, haystack);
	printf("malloc\n");
	fflush(stdout);
	pos = strstr(haystack, needle);
	while(pos != NULL)
		{
		printf("Start while\n");
		fflush(stdout);
		oldptr = dstptr;
		len_haystack = strlen(dstptr);
		diff = (long long) (len_replace - len_needle);
		dstptr = nmSysMalloc((len_haystack + diff + 1) * sizeof(char));
		
		num_shifts = pos - oldptr;
		memcpy(dstptr, oldptr, num_shifts);
		memcpy(dstptr + num_shifts, replace, len_replace);
		memcpy(dstptr + num_shifts + len_replace, pos + len_needle, len_haystack + 1 - num_shifts - len_needle);
		
		//free(oldptr);
		pos = strstr(haystack, needle);
		}

	strcpy(tree->String, dstptr);
	return 0;
	}

int exp_fn_utf8_substitute(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
	{
	return 0;
	}

int
exp_internal_DefineFunctions()
    {


    /** Function list for EXPR_N_FUNCTION nodes **/
    xhAdd(&EXP.Functions, "getdate", (char*) exp_fn_getdate);
    xhAdd(&EXP.Functions, "user_name", (char*) exp_fn_user_name);
    xhAdd(&EXP.Functions, "convert", (char*) exp_fn_convert);
    xhAdd(&EXP.Functions, "wordify", (char*) exp_fn_wordify);
    xhAdd(&EXP.Functions, "abs", (char*) exp_fn_abs);
    xhAdd(&EXP.Functions, "condition", (char*) exp_fn_condition);
    xhAdd(&EXP.Functions, "datepart", (char*) exp_fn_datepart);
    xhAdd(&EXP.Functions, "isnull", (char*) exp_fn_isnull);
    xhAdd(&EXP.Functions, "ltrim", (char*) exp_fn_ltrim);
    xhAdd(&EXP.Functions, "lztrim", (char*) exp_fn_lztrim);
    xhAdd(&EXP.Functions, "rtrim", (char*) exp_fn_rtrim);
    xhAdd(&EXP.Functions, "replicate", (char*) exp_fn_replicate);
    xhAdd(&EXP.Functions, "quote", (char*) exp_fn_quote);
    xhAdd(&EXP.Functions, "eval", (char*) exp_fn_eval);
    xhAdd(&EXP.Functions, "round", (char*) exp_fn_round);
    xhAdd(&EXP.Functions, "dateadd", (char*) exp_fn_dateadd);
    xhAdd(&EXP.Functions, "datediff", (char*) exp_fn_datediff);
    xhAdd(&EXP.Functions, "truncate", (char*) exp_fn_truncate);
    xhAdd(&EXP.Functions, "constrain", (char*) exp_fn_constrain);
    xhAdd(&EXP.Functions, "sin", (char*) exp_fn_sin);
    xhAdd(&EXP.Functions, "cos", (char*) exp_fn_cos);
    xhAdd(&EXP.Functions, "tan", (char*) exp_fn_tan);
    xhAdd(&EXP.Functions, "asin", (char*) exp_fn_asin);
    xhAdd(&EXP.Functions, "acos", (char*) exp_fn_acos);
    xhAdd(&EXP.Functions, "atan", (char*) exp_fn_atan);
    xhAdd(&EXP.Functions, "atan2", (char*) exp_fn_atan2);
    xhAdd(&EXP.Functions, "sqrt", (char*) exp_fn_sqrt);
    xhAdd(&EXP.Functions, "square", (char*) exp_fn_square);
    xhAdd(&EXP.Functions, "degrees", (char*) exp_fn_degrees);
    xhAdd(&EXP.Functions, "radians", (char*) exp_fn_radians);
    xhAdd(&EXP.Functions, "has_endorsement", (char*)exp_fn_has_endorsement);
    xhAdd(&EXP.Functions, "rand", (char*)exp_fn_rand);
    xhAdd(&EXP.Functions, "nullif", (char*)exp_fn_nullif);
    xhAdd(&EXP.Functions, "dateformat", (char*)exp_fn_dateformat);
    xhAdd(&EXP.Functions, "hash", (char*)exp_fn_hash);
    xhAdd(&EXP.Functions, "hmac", (char*)exp_fn_hmac);
    xhAdd(&EXP.Functions, "log10", (char*)exp_fn_log10);
    xhAdd(&EXP.Functions, "power", (char*)exp_fn_power);
    xhAdd(&EXP.Functions, "pbkdf2", (char*)exp_fn_pbkdf2);

    /** Windowing **/
    xhAdd(&EXP.Functions, "row_number", (char*)exp_fn_row_number);

    /** Aggregate **/
    xhAdd(&EXP.Functions, "count", (char*) exp_fn_count);
    xhAdd(&EXP.Functions, "avg", (char*) exp_fn_avg);
    xhAdd(&EXP.Functions, "sum", (char*) exp_fn_sum);
    xhAdd(&EXP.Functions, "max", (char*) exp_fn_max);
    xhAdd(&EXP.Functions, "min", (char*) exp_fn_min);
    xhAdd(&EXP.Functions, "first", (char*) exp_fn_first);
    xhAdd(&EXP.Functions, "last", (char*) exp_fn_last);
    xhAdd(&EXP.Functions, "nth", (char*) exp_fn_nth);
 
    /** Reverse functions **/
    xhAdd(&EXP.ReverseFunctions, "isnull", (char*) exp_fn_reverse_isnull);
    
    /** UTF-8/ASCII dependent **/
	xhAdd(&EXP.Functions, "utf8_reverse", (char*) exp_fn_utf8_reverse);
	xhAdd(&EXP.Functions, "utf8_replace", (char*) exp_fn_utf8_replace);
	xhAdd(&EXP.Functions, "overlong", (char*) exp_fn_utf8_overlong);
    if (CxGlobals.CharacterMode == CharModeSingleByte)
        {
        xhAdd(&EXP.Functions, "substring", (char*) exp_fn_substring);
        xhAdd(&EXP.Functions, "ascii", (char*) exp_fn_ascii);
        xhAdd(&EXP.Functions, "charindex", (char*) exp_fn_charindex);
        xhAdd(&EXP.Functions, "upper", (char*) exp_fn_upper);
        xhAdd(&EXP.Functions, "lower", (char*) exp_fn_lower);
        xhAdd(&EXP.Functions, "char_length", (char*) exp_fn_char_length);
        xhAdd(&EXP.Functions, "right", (char*) exp_fn_right);
        xhAdd(&EXP.Functions, "ralign", (char*) exp_fn_ralign);
        xhAdd(&EXP.Functions, "escape", (char*) exp_fn_escape);
        xhAdd(&EXP.Functions, "reverse", (char*) exp_fn_reverse);
        xhAdd(&EXP.Functions, "replace", (char*) exp_fn_replace);
        xhAdd(&EXP.Functions, "substitute", (char*) exp_fn_substitute);
	}
    else
        {
        xhAdd(&EXP.Functions, "substring", (char*) exp_fn_utf8_substring);
        xhAdd(&EXP.Functions, "ascii", (char*) exp_fn_utf8_ascii);
        xhAdd(&EXP.Functions, "charindex", (char*) exp_fn_utf8_charindex);
        xhAdd(&EXP.Functions, "upper", (char*) exp_fn_utf8_upper);
        xhAdd(&EXP.Functions, "lower", (char*) exp_fn_utf8_lower);
        xhAdd(&EXP.Functions, "char_length", (char*) exp_fn_utf8_char_length);
        xhAdd(&EXP.Functions, "right", (char*) exp_fn_utf8_right);
        xhAdd(&EXP.Functions, "ralign", (char*) exp_fn_utf8_ralign);
        xhAdd(&EXP.Functions, "escape", (char*) exp_fn_utf8_escape);
        
	xhAdd(&EXP.Functions, "reverse", (char*) exp_fn_utf8_reverse);
	xhAdd(&EXP.Functions, "replace", (char*) exp_fn_utf8_replace);
	xhAdd(&EXP.Functions, "substitute", (char*) exp_fn_utf8_substitute);
	}
    
    return 0;
    }
