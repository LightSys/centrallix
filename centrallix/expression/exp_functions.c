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

#define _GNU_SOURCE
#include <argon2.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cxlib/clusters.h"
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/util.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxss/cxss.h"
#include "expression.h"
#include "obj.h"


/** TODO: I think this should be moved to datatypes. **/
/** Should maybe replace duplocate functionality elsewhere. **/
static char* ci_TypeToStr(const int type)
    {
    switch (type)
	{
	case DATA_T_UNAVAILABLE: return "Unknown";
	case DATA_T_INTEGER:     return "Integer";
	case DATA_T_STRING:      return "String";
	case DATA_T_DOUBLE:      return "Double";
	case DATA_T_DATETIME:    return "DateTime";
	case DATA_T_INTVEC:      return "IntVecor";
	case DATA_T_STRINGVEC:   return "StringVector";
	case DATA_T_MONEY:       return "Money";
	case DATA_T_ARRAY:       return "Array";
	case DATA_T_CODE:        return "Code";
	case DATA_T_BINARY:      return "Binary";
	}
    
    /** Invalid type. **/
    mssErrorf(1, "Cluster", "Invalid type %d.\n", type);
    return "Invalid"; /* Shall not parse to a valid type in ci_TypeFromStr(). */
    }

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
    Binary b;

    if (!i0 || !i1 || i0->DataType != DATA_T_STRING || (i0->Flags & EXPR_F_NULL))
        {
	mssError(1,"EXP","convert() requires data type and value to be converted");
	return -1;
	}
    switch(i1->DataType)
        {
	case DATA_T_INTEGER: vptr = &(i1->Integer); break;
	case DATA_T_STRING: vptr = i1->String; break;
	case DATA_T_BINARY:
	    b.Size = i1->Size;
	    b.Data = (unsigned char*)i1->String;
	    vptr = &b;
	    break;
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
	if (i2->AggLevel > 0 || (i2->Flags & EXPR_F_HASWINDOWFN))
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
	if (i1->AggLevel > 0 || (i1->Flags & EXPR_F_HASWINDOWFN))
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


int exp_fn_mixed(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n,i,j,l;
    int is_boundary;
    char tmp;
    char* ptr;
    char* save;
    char* ast;
    XArray wordlist;

    tree->DataType = DATA_T_STRING;
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i0->DataType != DATA_T_STRING)
        {
	mssError(1,"EXP","One or two string parameters required for mixed()");
	return -1;
	}
    if (i1 && i1->Flags & EXPR_F_NULL)
	i1 = NULL;
    if (i1 && i1->DataType != DATA_T_STRING)
	{
	mssError(1,"EXP","Optional second parameter to mixed() must be a string");
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

    /** Identify words in the i1 wordlist **/
    if (i1)
	{
	xaInit(&wordlist, 16);
	ptr = strtok_r(i1->String, ",", &save);
	while (ptr)
	    {
	    xaAddItem(&wordlist, ptr);
	    ptr = strtok_r(NULL, ",", &save);
	    }
	}

    /** Convert the string. **/
    is_boundary = 1;
    for(i=0; i<n+1; i++) 
        {
	if (!is_boundary)
	    {
	    /** If not at a boundary, lower() things. **/
	    if (i0->String[i] >= 'A' && i0->String[i] <= 'Z')
		tree->String[i] = i0->String[i] + 32;
	    else
		tree->String[i] = i0->String[i];

	    /** Transition from word to boundary if the char is non-alpha, but
	     ** do not transition if the char is a single quote or a dash, since
	     ** those can occur inside of a "word".
	     **/
	    if (!isalpha(tree->String[i]) && tree->String[i] != '\'' && tree->String[i] != '-')
		is_boundary = 1;
	    }
	else
	    {
	    /** At a boundary.  Either upper() or replace with provided capitalization. **/
	    if (i1)
		{
		/** We're using a wordlist - check for a word. **/
		l = strspn(i0->String + i, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
		if (l > 0)
		    {
		    /** We have a "word".  Look it up. **/
		    tmp = i0->String[i+l];
		    i0->String[i+l] = '\0';

		    /** Look for a match in the wordlist **/
		    for(j=0; j<wordlist.nItems; j++)
			{
			ast = NULL;
			ptr = (char*)wordlist.Items[j];
			if (strcasecmp(ptr, i0->String + i) == 0)
			    {
			    i0->String[i+l] = tmp;
			    break;
			    }
			else
			    {
			    /** Wildcard match, implies following capital.  However, do
			     ** not apply this unless there are at least two characters
			     ** (one to be uppercase and one to be lower) matched by the
			     ** wildcard.  So, Mc* would not match McD, and Mac* would
			     ** not match Mack or Macy.
			     **/
			    ast = strchr(ptr, '*');
			    if (ast && strncasecmp(ptr, i0->String + i, ast - ptr) == 0 && l > ((ast - ptr) + 1))
				{
				/** Reset length to force upcase of next char **/
				i0->String[i+l] = tmp;
				l = ast - ptr;
				break;
				}
			    else
				{
				ptr = NULL;
				}
			    }
			}
		    if (ptr)
			{
			/** Replacement word specifies case **/
			for(j=0; j<l; j++)
			    tree->String[i+j] = ptr[j];
			if (!ast)
			    is_boundary = 0;
			}
		    else
			{
			/** No replacement word, upcase only first char **/
			i0->String[i+l] = tmp;
			for(j=0; j<l; j++)
			    {
			    if (j == 0 && i0->String[i+j] >= 'a' && i0->String[i+j] <= 'z')
				tree->String[i+j] = i0->String[i+j] - 32;
			    else if (j > 0 && i0->String[i+j] >= 'A' && i0->String[i+j] <= 'Z')
				tree->String[i+j] = i0->String[i+j] + 32;
			    else
				tree->String[i+j] = i0->String[i+j];
			    }
			is_boundary = 0;
			}

		    i += (l-1);
		    }
		else
		    {
		    tree->String[i] = i0->String[i];
		    }
		}
	    else
		{
		/** No wordlist, so just upper() **/
		if (i0->String[i] >= 'a' && i0->String[i] <= 'z')
		    tree->String[i] = i0->String[i] - 32;
		else
		    tree->String[i] = i0->String[i];
		if (isalpha(tree->String[i]))
		    is_boundary = 0;
		}
	    }
	}

    if (i1)
	{
	for(i=1; i<wordlist.nItems; i++)
	    {
	    /** Reset delimiters **/
	    ptr = wordlist.Items[i];
	    ptr[-1] = ',';
	    }
	xaDeInit(&wordlist);
	}

    return 0;
    }


int exp_fn_octet_length(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    tree->DataType = DATA_T_INTEGER;
    if (i0 && i0->Flags & EXPR_F_NULL)
        {
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || i1)
        {
	mssError(1,"EXP","One parameter required for octet_length()");
	return -1;
	}
    switch(i0->DataType)
	{
	case DATA_T_INTEGER:
	    tree->Integer = sizeof(tree->Integer);
	    break;
	case DATA_T_DOUBLE:
	    tree->Integer = sizeof(tree->Types.Double);
	    break;
	case DATA_T_STRING:
	    tree->Integer = strlen(i0->String);
	    break;
	case DATA_T_BINARY:
	    tree->Integer = i0->Size;
	    break;
	case DATA_T_DATETIME:
	    tree->Integer = sizeof(tree->Types.Date);
	    break;
	case DATA_T_MONEY:
	    tree->Integer = sizeof(tree->Types.Money);
	    break;
	default:
	    mssError(1,"EXP","Unsupported data type for octet_length()");
	    return -1;
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


int exp_fn_replace(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* repstr;
    long long newsize;
    char* srcptr;
    char* searchptr;
    char* dstptr;
    int searchlen, replen;
    if ((i0 && (i0->Flags & EXPR_F_NULL)) || (i1 && (i1->Flags & EXPR_F_NULL)))
	{
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
	newsize = (newsize * replen) / searchlen;
    newsize += 1;
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

/** Leading zero trim. */
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


int exp_fn_trim(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int ret;
    
    /** Invoke left trim. **/
    ret = exp_fn_ltrim(tree, objlist, i0, i1, i2);
    if (ret != 0)
	{
	mssErrorf(0, "EXP", "Failed to left trim (error code: %d).", ret);
	return ret;
	}
    
    /** Invoke right trim. **/
    ret = exp_fn_rtrim(tree, objlist, i0, i1, i2);
    if (ret != 0)
	{
	mssErrorf(0, "EXP", "Failed to right trim (error code: %d).", ret);
	return ret;
	}
    
    /** Success. **/
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


/* See centrallix-sysdoc/SubstituteFunction.md for more information. */
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
    pExpression child;
    int oldmainflags;
    int oldcurrent, oldparent;
    int newpermflags;
    int rval;
    int objid;

	/** NULL result because 1st param is null? **/
	if (!objlist || (i0 && i0->Flags & EXPR_F_NULL))
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Not allowed? **/
	if (objlist->MainFlags & EXPR_MO_NOEVAL)
	    {
	    mssError(1,"EXP","Cannot use eval() in this context");
	    return -1;
	    }

	/** Usage **/
	if (!i0 || i0->DataType != DATA_T_STRING)
	    {
	    mssError(1,"EXP","eval() first parameter must be a string");
	    return -1;
	    }
	for(parent=tree;parent->Parent;parent=parent->Parent);

	oldmainflags = objlist->MainFlags;
	oldcurrent = objlist->CurrentID;
	oldparent = objlist->ParentID;

	/** Permission flags **/
	if (i1 && !(i1->Flags & EXPR_F_NULL))
	    {
	    if (i1->DataType != DATA_T_STRING)
		{
		mssError(1,"EXP","eval() second parameter must be a string");
		return -1;
		}
	    newpermflags = 0;
	    if (strchr(i1->String, 'C') == NULL) newpermflags |= EXPR_MO_NOCURRENT;
	    if (strchr(i1->String, 'P') == NULL) newpermflags |= EXPR_MO_NOPARENT;
	    if (strchr(i1->String, 'O') == NULL) newpermflags |= EXPR_MO_NOOBJECT;
	    if (strchr(i1->String, 'D') == NULL) newpermflags |= EXPR_MO_NODIRECT;
	    if (strchr(i1->String, 'S') == NULL) newpermflags |= EXPR_MO_NOSUBQUERY;
	    if (strchr(i1->String, 'E') == NULL) newpermflags |= EXPR_MO_NOEVAL;
	    }
	else
	    {
	    newpermflags = EXPR_MO_DEFPERMMASK;
	    }
	objlist->MainFlags |= newpermflags;

	/** Current object name **/
	if (i2 && !(i2->Flags & EXPR_F_NULL))
	    {
	    if (i2->DataType != DATA_T_STRING)
		{
		mssError(1,"EXP","eval() third parameter must be a string");
		return -1;
		}
	    objid = expLookupParam(objlist, i2->String, 0);
	    if (objid >= 0)
		objlist->CurrentID = objid;
	    else
		{
		mssError(1,"EXP","eval() no such object %s", i2->String);
		return -1;
		}
	    }

	/** Parent object name **/
	if (tree->Children.nItems == 4)
	    {
	    child = tree->Children.Items[3];
	    if (child && !(child->Flags & EXPR_F_NULL))
		{
		if (child->DataType != DATA_T_STRING)
		    {
		    mssError(1,"EXP","eval() fourth parameter must be a string");
		    return -1;
		    }
		objid = expLookupParam(objlist, child->String, 0);
		if (objid >= 0)
		    objlist->ParentID = objid;
		else
		    {
		    mssError(1,"EXP","eval() no such object %s", child->String);
		    return -1;
		    }
		}
	    }

	/** Compile and evaluate **/
	eval_exp = expCompileExpression(i0->String, objlist, parent->LxFlags, parent->CmpFlags);
	if (!eval_exp) return -1;
	rval = expEvalTree(eval_exp, objlist);
	objlist->MainFlags = oldmainflags;
	objlist->CurrentID = oldcurrent;
	objlist->ParentID = oldparent;
	if (rval < 0)
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


int exp_fn_moneyformat(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;
    pMoneyType mp;
    MoneyType m;

    /** checks **/
    if (!i0 || !i1)
	{
	mssError(1, "EXP", "moneyformat() takes two parameters: (datetime, string)");
	return -1;
	}
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
	tree->DataType = DATA_T_STRING;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i0 || (i0->DataType != DATA_T_MONEY && i0->DataType != DATA_T_INTEGER))
	{
	mssError(1, "EXP", "moneyformat() first parameter must be an integer or money type");
	return -1;
	}
    if (!i1 || i1->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "moneyformat() second parameter must be a string");
	return -1;
	}

    if (i0->DataType == DATA_T_MONEY)
	{
	mp = &i0->Types.Money;
	}
    else
	{
	objDataToMoney(i0->DataType, &i0->Integer, &m);
	mp = &m;
	}

    ptr = objFormatMoneyTmp(mp, i1->String);
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


int exp_fn_dateformat(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    char* ptr;
    pDateTime dt;

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
    if (!i0 || (i0->DataType != DATA_T_DATETIME && i0->DataType != DATA_T_STRING))
	{
	mssError(1, "EXP", "dateformat() first parameter must be a date or string");
	return -1;
	}
    if (!i1 || i1->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "dateformat() second parameter must be a string");
	return -1;
	}

    dt = expPromoteDate(i0);
    ptr = objFormatDateTmp(dt, i1->String);
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
    DateTime dt1, dt2, tmpdt;
    pDateTime dt;

    /** checks **/
    if (!i0 || (i0->Flags & EXPR_F_NULL) || i0->DataType != DATA_T_STRING)
	{
	mssError(1, "EXP", "datediff() first parameter must be non-null string or keyword date part");
	return -1;
	}
    if ((i1 && (i1->Flags & EXPR_F_NULL)) || (i2 && (i2->Flags & EXPR_F_NULL)))
	{
	tree->DataType = DATA_T_INTEGER;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}
    if (!i1 || (i1->DataType != DATA_T_DATETIME && i1->DataType != DATA_T_STRING) || !i2 || (i2->DataType != DATA_T_DATETIME && i2->DataType != DATA_T_STRING))
	{
	mssError(1, "EXP", "datediff() second and third parameters must be datetime types");
	return -1;
	}
    tree->DataType = DATA_T_INTEGER;
    dt = expPromoteDate(i1);
    if (!dt)
	return -1;
    memcpy(&dt1, dt, sizeof(DateTime));
    dt = expPromoteDate(i2);
    if (!dt)
	return -1;
    memcpy(&dt2, dt, sizeof(DateTime));

    /** Swap operands if we're diffing backwards **/
    if (dt2.Value < dt1.Value)
	{
	sign = -1;
	tmpdt = dt2;
	dt2 = dt1;
	dt1 = tmpdt;
	}

    /** choose which date part.  Typecasts are to make sure we're working
     ** with signed values.
     **/
    if (strcmp(i0->String, "year") == 0)
	{
	tree->Integer = dt2.Part.Year - (int)dt1.Part.Year;
	}
    else if (strcmp(i0->String, "month") == 0)
	{
	tree->Integer = dt2.Part.Year - (int)dt1.Part.Year;
	tree->Integer = tree->Integer*12 + dt2.Part.Month - (int)dt1.Part.Month;
	}
    else
	{
	/** fun.  working with the day part of stuff gets tricky -- leap
	 ** years and all that stuff.  Count the days up manually.
	 **/
	tree->Integer = - dt1.Part.Day;
	yr = dt1.Part.Year;
	mo = dt1.Part.Month;
	while (yr < dt2.Part.Year || (yr == dt2.Part.Year &&  mo < dt2.Part.Month))
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
	tree->Integer += dt2.Part.Day;

	/** Hours, minutes, seconds? **/
	if (strcmp(i0->String, "day") != 0)
	    {
	    /** has to be H, M, or S **/
	    tree->Integer = tree->Integer*24 + dt2.Part.Hour - (int)dt1.Part.Hour;
	    if (strcmp(i0->String, "hour") != 0)
		{
		/** has to be M or S **/
		tree->Integer = tree->Integer*60 + dt2.Part.Minute - (int)dt1.Part.Minute;
		if (strcmp(i0->String, "minute") != 0)
		    {
		    /** has to be S **/
		    if (strcmp(i0->String, "second") != 0)
			{
			mssError(1,"EXP","Invalid date part '%s' for datediff()", i0->String);
			return -1;
			}
		    tree->Integer = tree->Integer*60 + dt2.Part.Second - (int)dt1.Part.Second;
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
    pDateTime dt;

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
    if (!i2 || (i2->DataType != DATA_T_DATETIME && i2->DataType != DATA_T_STRING))
	{
	mssError(1, "EXP", "dateadd() third parameter must be a datetime type");
	return -1;
	}
    dt = expPromoteDate(i2);

    /** ok, we're good.  set up for returning the value **/
    tree->DataType = DATA_T_DATETIME;
    memcpy(&tree->Types.Date, dt, sizeof(DateTime));
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
	tree->Types.Double = (double)((long double)val / ((long double)ULLONG_MAX + (long double)1.0));

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
	if (tree->Alloc && tree->String) nmSysFree(tree->String);
	tree->String = nmSysMalloc(hashlen * 2 + 1);
	tree->Alloc = 1;
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


int exp_fn_to_hex(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pXString dest = NULL;
    int len;

	if (!i0 || (!(i0->Flags & EXPR_F_NULL) && i0->DataType != DATA_T_STRING && i0->DataType != DATA_T_BINARY))
	    {
	    mssError(1, "EXP", "to_hex() expects one string or binary parameter");
	    goto error;
	    }

	tree->DataType = DATA_T_STRING;

	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	dest = xsNew();
	if (!dest)
	    goto error;
	
	if (i0->DataType == DATA_T_STRING)
	    len = strlen(i0->String);
	else
	    len = i0->Size;
	if (xsQPrintf(dest, "%*STR&HEX", len, i0->String) < 0)
	    goto error;
	expSetString(tree, dest->String);

	xsFree(dest);

	return 0;

    error:
	if (dest)
	    xsFree(dest);
	return -1;
    }


int exp_fn_from_hex(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pXString dest = NULL;

	if (!i0 || (!(i0->Flags & EXPR_F_NULL) && i0->DataType != DATA_T_STRING))
	    {
	    mssError(1, "EXP", "from_hex() expects one string parameter");
	    goto error;
	    }

	tree->DataType = DATA_T_BINARY;

	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	dest = xsNew();
	if (!dest)
	    goto error;

	if (xsQPrintf(dest, "%*STR&DHEX", strlen(i0->String), i0->String) < 0)
	    {
	    mssError(1, "EXP", "from_hex(): invalid hex-encoded data");
	    goto error;
	    }
	expSetBinary(tree, (unsigned char*)xsString(dest), xsLength(dest));

	xsFree(dest);

	return 0;

    error:
	if (dest)
	    xsFree(dest);
	return -1;
    }


int exp_fn_to_base64(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pXString dest = NULL;

	if (!i0 || (!(i0->Flags & EXPR_F_NULL) && i0->DataType != DATA_T_STRING))
	    {
	    mssError(1, "EXP", "to_base64() expects one string parameter");
	    goto error;
	    }

	tree->DataType = DATA_T_STRING;

	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	dest = xsNew();
	if (!dest)
	    goto error;

	if (xsQPrintf(dest, "%STR&B64", i0->String) < 0)
	    goto error;
	expSetString(tree, dest->String);

	xsFree(dest);

	return 0;

    error:
	if (dest)
	    xsFree(dest);
	return -1;
    }


int exp_fn_from_base64(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pXString dest = NULL;

	if (!i0 || (!(i0->Flags & EXPR_F_NULL) && i0->DataType != DATA_T_STRING))
	    {
	    mssError(1, "EXP", "from_base64() expects one string parameter");
	    goto error;
	    }

	tree->DataType = DATA_T_BINARY;

	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	dest = xsNew();
	if (!dest)
	    goto error;

	if (xsQPrintf(dest, "%STR&DB64", i0->String) < 0)
	    {
	    mssError(1, "EXP", "from_base64(): invalid base64-encoded data");
	    goto error;
	    }
	expSetBinary(tree, (unsigned char*)xsString(dest), xsLength(dest));

	xsFree(dest);

	return 0;

    error:
	if (dest)
	    xsFree(dest);
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


int exp_fn_log_natural(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double n;

	if (!i0)
	    {
	    mssError(1, "EXP", "ln() requires a number as its first parameter");
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
		mssError(1, "EXP", "ln() requires a number as its first parameter");
		goto error;
	    }
	if (n < 0)
	    {
	    mssError(1, "EXP", "ln(): cannot compute the logarithm of a negative number");
	    goto error;
	    }
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = log(n);
	return 0;

    error:
	return -1;
    }


int exp_fn_log_base_n(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    double n, p;

	if (!i0 || !i1)
	    {
	    mssError(1, "EXP", "logn() requires numbers as its first and second parameters");
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
		mssError(1, "EXP", "logn() requires a number as its first parameter");
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
		mssError(1, "EXP", "logn() requires an integer or double as its second parameter");
		goto error;
	    }
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = log(n) / log(p);
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

typedef struct
    {
    int		MaxLookback;
    int		CurLookback;
    pTObjData	Items[1];
    }
    ExpLagVector, *pExpLagVector;

int exp_fn_lag_finalize(void* lv_v)
    {
    pExpLagVector lv = (pExpLagVector)lv_v;
    int i;
    for (i=0; i<lv->CurLookback; i++)
	ptodFree(lv->Items[i]);
    nmSysFree(lv);
    return 0;
    }

int exp_fn_lag(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    int n;
    pExpLagVector lv;
    pTObjData ptod;

    if (!i0)
	{
	mssError(1,"EXP","lag() requires a parameter");
	return -1;
	}
    if (i1 && !(i1->Flags & EXPR_F_NULL) && (i1->DataType != DATA_T_INTEGER || i1->Integer <= 0))
	{
	mssError(1,"EXP","lag() second parameter, if supplied, must be a non-null positive integer");
	return -1;
	}

    /** Private data is where we store a chain of previous values **/
    if (!tree->PrivateData)
	{
	/** If the offset is constant, use it as the max lookback, otherwise set the max lookback (n) to
	 ** the greater of 100 or the current value.  Our hard ceiling is 4096.  If not specified, we
	 ** just look at the previous row (offset 1).
	 **/
	if (i1 && i1->NodeType == EXPR_N_INTEGER)
	    n = i1->Integer;
	else if (i1)
	    n = i1->Integer < 100 ? 100 : i1->Integer;
	else
	    n = 1;
	if (n > 4096)
	    n = 4096;

	lv = (pExpLagVector)nmSysMalloc(sizeof(ExpLagVector) + (sizeof(pTObjData) * (n - 1)));
	if (!lv)
	    return -ENOMEM;
	tree->PrivateData = lv;
	tree->PrivateDataFinalize = exp_fn_lag_finalize;
	memset(tree->PrivateData, 0, sizeof(ExpLagVector) + (sizeof(pTObjData) * (n - 1)));
	lv->MaxLookback = n;
	lv->CurLookback = 0;
	}
    else
	{
	lv = (pExpLagVector)tree->PrivateData;
	}

    /** No actual new value?  Exit now. **/
    if (tree->Flags & EXPR_F_AGGLOCKED)
	return 0;

    /** Set the right value from our lookback list **/
    if (i1)
	n = i1->Integer;
    else
	n = 1;
    if (n > lv->CurLookback || (i1->Flags & EXPR_F_NULL))
	{
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = i0->DataType;
	}
    else
	{
	if (expPtodToExpression(lv->Items[lv->CurLookback - n], tree) < 0)
	    return -1;
	}

    /** New value; add to our item list **/
    ptod = expExpressionToPtod(i0);
    if (lv->MaxLookback == lv->CurLookback)
	{
	ptodFree(lv->Items[0]);
	if (lv->MaxLookback > 1)
	    memmove(&(lv->Items[0]), &(lv->Items[1]), sizeof(pTObjData) * (lv->MaxLookback - 1));
	lv->CurLookback -= 1;
	}
    lv->Items[lv->CurLookback] = ptod;
    lv->CurLookback += 1;
    tree->AggCount += 1;

    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


int exp_fn_dense_rank(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    pExpression new_exp;
    char newbuf[512];

    /** Init the Aggregate computation expression? **/
    if (!tree->AggExp)
        {
	tree->AggCount = 0;
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
	expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 0);
	}

    if (tree->Flags & EXPR_F_AGGLOCKED) return 0;

    /** Changed? **/
    if (tree->Children.nItems > 0)
	{
	memset(newbuf, 0, sizeof(newbuf));
	if (objBuildBinaryImage(newbuf, sizeof(newbuf), tree->Children.Items, tree->Children.nItems, objlist, 0) < 0)
	    return -1;
	if (memcmp(newbuf, tree->PrivateData, sizeof(newbuf)) || tree->AggCount == 0)
	    {
	    /** Increment count **/
	    memcpy(tree->PrivateData, newbuf, sizeof(newbuf));
	    expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	    exp_internal_EvalTree(tree->AggExp, objlist);
	    expCopyValue(tree->AggExp, tree, 0);
	    tree->AggCount++;
	    }
	}
    else if (tree->AggCount == 0)
	{
	/** Single partition for entire result set **/
	expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
	exp_internal_EvalTree(tree->AggExp, objlist);
	expCopyValue(tree->AggExp, tree, 0);
	tree->AggCount++;
	}

    tree->Flags |= EXPR_F_AGGLOCKED;
    return 0;
    }


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
	if (memcmp(newbuf, tree->PrivateData, sizeof(newbuf)))
	    {
	    /** Reset count **/
	    memcpy(tree->PrivateData, newbuf, sizeof(newbuf));
	    tree->AggExp->Integer = 0;
	    tree->AggCount = 0;
	    tree->AggValue = 0;
	    }
	}

    expCopyValue(tree->AggExp, (pExpression)(tree->AggExp->Children.Items[1]), 0);
    exp_internal_EvalTree(tree->AggExp, objlist);
    expCopyValue(tree->AggExp, tree, 0);

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
    int rval;

    if (!i0)
	{
	mssError(1,"EXP","first() requires a parameter");
	return -1;
	}
    if (tree->AggCount == 0)
	rval = exp_internal_EvalTree(i0, objlist);
    if (rval < 0)
	return -1;
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


/*** Computes cosine or levenshtien similarity between two strings. These two
 *** tasks have a large amount of overlapping logic (mostly error checking),
 *** so doing them with one function greatly reduces code duplocation.
 *** 
 *** @param tree The tree resulting from this function.
 *** @param objlist The evaluation "scope", including available variables.
 *** @param maybe_str1 Possibly the first string.
 *** @param maybe_str2 Possibly the second string.
 *** @param u1 Unused parameter.
 *** @param is_cos Whether to compute cosine or levenshtien.
 *** @returns 0 for success, -1 for failure.
 ***/
static int exp_fn_cmp(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1, const char* fn_name)
    {
    /** Check number of arguments. **/
    const int num_params = tree->Children.nItems;
    if (num_params != 2)
	{
	mssErrorf(1, "EXP", "%s(?) expects 2 parameters, got %d parameters.", fn_name, num_params);
	return -1;
	}
    if (maybe_str1 == NULL || maybe_str2 == NULL || u1 != NULL)
	{
	mssErrorf(1, "EXP", "%s(?) expects 2 parameters.", fn_name);
	return -1;
	}
    
    /** Magic checks. **/
    ASSERTMAGIC(tree, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str1, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str2, MGK_EXPRESSION);
    
    /** Check object list. **/
    if (objlist == NULL)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?) no object list?", fn_name);
	return -1;
	}
    ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
    /** Extract str1. **/
    if (maybe_str1->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_DOUBLE;
	return 0;
	}
    if (maybe_str1->DataType != DATA_T_STRING)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?, ..) str1 should be a string.", fn_name);
	return -1;
	}
    char* str1 = check_ptr(maybe_str1->String);
	
    /** Extract str2. **/
    if (maybe_str2->Flags & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_DOUBLE;
	return 0;
	}
    if (maybe_str2->DataType != DATA_T_STRING)
	{
	mssErrorf(1, "EXP", "%s(\"%s\", \?\?\?) str2 should be a string.", fn_name, str1);
	return -1;
	}
    char* str2 = check_ptr(maybe_str2->String);
    
    /** Handle either cos_cmp or lev_cmp. **/
    if (fn_name[0] == 'c')
	{ /* cos_cmp */
	int ret;
	
	/** Build vectors. **/
	const pVector v1 = check_ptr(ca_build_vector(str1));
	const pVector v2 = check_ptr(ca_build_vector(str2));
	if (v1 == NULL || v2 == NULL)
	    {
	    mssErrorf(1, "EXP",
		"%s(\"%s\", \"%s\") - Failed to build vectors.",
		fn_name, str1, str2
	    );
	    ret = -1;
	    }
	else
	    {
	    tree->Types.Double = ca_cos_compare(v1, v2);
	    tree->DataType = DATA_T_DOUBLE;
	    ret = 0;
	    }
	
	if (v1 != NULL) ca_free_vector(v1);
	if (v2 != NULL) ca_free_vector(v2);
	return ret;
	}
    else
	{ /* lev_cmp */
	tree->Types.Double = ca_lev_compare(str1, str2);
	tree->DataType = DATA_T_DOUBLE;
	return 0;
	}
    return -1;
    }


int exp_fn_cos_cmp(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "cos_cmp"); }
int exp_fn_cos_compare(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "cos_compare"); }
int exp_fn_cosine_compare(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "cosine_compare"); }
int exp_fn_lev_cmp(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "lev_cmp"); }
int exp_fn_lev_compare(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "lev_compare"); }
int exp_fn_levenshtein_compare(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1)
    { return exp_fn_cmp(tree, objlist, maybe_str1, maybe_str2, u1, "levenshtein_compare"); }
    

/*** Comparse two strings to see if their sparse vectors are equal.
 *** 
 *** @param tree The tree resulting from this function.
 *** @param objlist The evaluation "scope", including available variables.
 *** @param maybe_str1 Possibly the first string.
 *** @param maybe_str2 Possibly the second string.
 *** @param u1 Unused parameter.
 *** @returns 0 for success, -1 for failure.
 ***/
static int exp_fn_sparse_eql(pExpression tree, pParamObjects objlist, pExpression maybe_str1, pExpression maybe_str2, pExpression u1, bool is_cos)
    {
    const char fn_name[] = "sparse_compare";
    
    /** Check number of arguments. **/
    const int num_params = tree->Children.nItems;
    if (num_params != 2)
	{
	mssErrorf(1, "EXP", "%s(?) expects 2 parameters, got %d parameters.", fn_name, num_params);
	return -1;
	}
    if (maybe_str1 == NULL || maybe_str2 == NULL || u1 != NULL)
	{
	mssErrorf(1, "EXP", "%s(?) expects 2 parameters.", fn_name);
	return -1;
	}
    
    /** Magic checks. **/
    ASSERTMAGIC(tree, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str1, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str2, MGK_EXPRESSION);
    
    /** Check object list. **/
    if (objlist == NULL)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?) no object list?", fn_name);
	return -1;
	}
    ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
    /** Extract str1. **/
    if (maybe_str1->Flags & EXPR_F_NULL)
	{
	mssErrorf(1, "EXP", "%s(NULL, ...) str1 cannot be NULL.", fn_name);
	return -1;
	}
    if (maybe_str1->DataType != DATA_T_STRING)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?, ..) str1 should be a string.", fn_name);
	return -1;
	}
    char* str1 = maybe_str1->String;
    if (str1 == NULL)
	{
	mssErrorf(1, "EXP",
	    "%s(nothing?, ...) expected string from str1 (of datatype DataType = "
	    "DATA_T_STRING), but the String was NULL or did not exist!",
	    fn_name
	);
	return -1;
	}
	
    /** Extract str2. **/
    if (maybe_str2->Flags & EXPR_F_NULL)
	{
	mssErrorf(1, "EXP", "%s(\"%s\", NULL) str2 cannot be NULL.", fn_name, str1);
	return -1;
	}
    if (maybe_str2->DataType != DATA_T_STRING)
	{
	mssErrorf(1, "EXP", "%s(\"%s\", \?\?\?) str2 should be a string.", fn_name, str1);
	return -1;
	}
    char* str2 = maybe_str2->String;
    if (str2 == NULL)
	{
	mssErrorf(1, "EXP",
	    "%s(\"%s\", nothing?) expected string from str2 (of datatype DataType = "
	    "DATA_T_STRING), but the String was NULL or did not exist!",
	    fn_name, str1
	);
	return -1;
	}
    
    /** Build vectors. **/
    int ret;
    const pVector v1 = check_ptr(ca_build_vector(str1));
    const pVector v2 = check_ptr(ca_build_vector(str2));
    if (v1 == NULL || v2 == NULL)
	{
	mssErrorf(1, "EXP",
	    "%s(\"%s\", \"%s\") - Failed to build vectors.",
	    fn_name, str1, str2
	);
	ret = -1;
	}
    else
	{
	tree->Integer = (ca_eql(v1, v2)) ? 1 : 0;
	tree->DataType = DATA_T_INTEGER;
	ret = 0;
	}
    
    if (v1 != NULL) ca_free_vector(v1);
    if (v2 != NULL) ca_free_vector(v2);
    return ret;
    }

/*** Computes double metaphone.
 *** 
 *** @param tree The tree resulting from this function.
 *** @param objlist The evaluation "scope", including available variables.
 *** @param maybe_str Possibly the string passed to double metaphone.
 *** @param u1 Unused parameter.
 *** @param u2 Unused parameter.
 ***/
int exp_fn_double_metaphone(pExpression tree, pParamObjects objlist, pExpression maybe_str, pExpression u1, pExpression u2)
    {
    const char fn_name[] = "double_metaphone";
    
    /** Check number of arguments. **/
    const int num_params = tree->Children.nItems;
    if (num_params != 1)
	{
	mssErrorf(1, "EXP", "%s(?) expects 1 parameter, got %d parameters.", fn_name, num_params);
	return -1;
	}
    if (maybe_str == NULL || u1 != NULL || u2 != NULL)
	{
	mssErrorf(1, "EXP", "%s(?) expects 1 parameter.", fn_name);
	return -1;
	}
    
    /** Magic checks. **/
    ASSERTMAGIC(tree, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str, MGK_EXPRESSION);
    
    /** Check object list. **/
    if (objlist == NULL)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?) no object list?", fn_name);
	return -1;
	}
    ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
    /** Extract str. **/
    if (maybe_str->Flags & EXPR_F_NULL)
	{
	mssErrorf(1, "EXP", "%s(NULL) str cannot be NULL.", fn_name);
	return -1;
	}
    if (maybe_str->DataType != DATA_T_STRING)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?) str should be a string.", fn_name);
	return -1;
	}
    const char* str = maybe_str->String;
    if (str == NULL)
	{
	mssErrorf(1, "EXP",
	    "%s(nothing?) expected string from str (of datatype DataType = "
	    "DATA_T_STRING), but the String was NULL or did not exist!",
	    fn_name
	);
	return -1;
	}
    const size_t str_len = strlen(str);
    if (str_len == 0u)
	{
	mssErrorf(1, "EXP", "%s(\"\") str cannot be an empty string.", fn_name);
	return -1;
	}
    
    /** Compute DoubleMetaphone. **/
    char* primary = NULL;
    char* secondary = NULL;
    meta_double_metaphone(str, &primary, &secondary);
    
    /** Process result. **/
    const size_t primary_length = strlen(primary);
    const size_t secondary_length = strlen(secondary);
    char* result = check_ptr(nmSysMalloc(primary_length + 1u + secondary_length + 1u));
    if (result == NULL) return -1;
    sprintf(result, "%s%c%s", primary, CA_BOUNDARY_CHAR, secondary);
    
    /** Return the result. **/
    tree->String = result;
    tree->DataType = DATA_T_STRING;
    return 0;
    }


int exp_fn_aggregate_similarities(pExpression tree, pParamObjects objlist)
    {
    const char fn_name[] = "aggregate_similarities";
	
    /** Check number of arguments. **/
    const int num_params = tree->Children.nItems;
    if (num_params != 6)
	{
	mssErrorf(1, "EXP", "%s(?) expects 6 parameters, got %d parameters.", fn_name, num_params);
	return -1;
	}
    
    /** Magic checks. **/
    ASSERTMAGIC(tree, MGK_EXPRESSION);
    ASSERTMAGIC(tree->Children.Items[0], MGK_EXPRESSION);
    ASSERTMAGIC(tree->Children.Items[1], MGK_EXPRESSION);
    ASSERTMAGIC(tree->Children.Items[2], MGK_EXPRESSION);
    ASSERTMAGIC(tree->Children.Items[3], MGK_EXPRESSION);
    
    /** Check object list. **/
    if (objlist == NULL)
	{
	mssErrorf(1, "EXP", "%s(\?\?\?) no object list?", fn_name);
	return -1;
	}
    ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
    /** Extract parameters. **/
    double params[4] = {NAN};
    const char names[4][8] = {"name", "email", "phone", "address"};
    for (unsigned int i = 0; i < 4u; i++)
	{
	pExpression param = (pExpression)tree->Children.Items[i];
	
	/** Ignore null values. **/
	if (param->Flags & EXPR_F_NULL) continue;
	
	/** Only accept doubles. **/
	if (param->DataType != DATA_T_DOUBLE)
	    {
	    mssErrorf(1, "EXP",
		"%s() param%u (%s) expected type %s but got %s.",
		fn_name, i, names[i], ci_TypeToStr(DATA_T_DOUBLE), ci_TypeToStr(param->DataType)
	    );
	    if (param->DataType == DATA_T_INTEGER) fprintf(stderr, "Value: %d\n", param->Integer);
	    return -1;
	    }
	
	/** Do not accept NaN. **/
	params[i] = param->Types.Double;
	if (isnan(params[i]))
	    {
	    mssErrorf(1, "EXP", "%s() param%u (%s) cannot be NaN", fn_name, names[i], i);
	    return -1;
	    }
	}
    
    char* dup_names[2] = {NULL};
    for (unsigned int i = 0; i < 2u; i++)
	{
	pExpression param = (pExpression)tree->Children.Items[i + 4u];
	
	/** Ignore null values. **/
	if (param->Flags & EXPR_F_NULL) continue;
	
	/** Only accept doubles. **/
	if (param->DataType != DATA_T_STRING)
	    {
	    mssErrorf(1, "EXP",
		"%s() param%u expected type %s but got %s.",
		fn_name, i, ci_TypeToStr(DATA_T_STRING), ci_TypeToStr(param->DataType)
	    );
	    return -1;
	    }
	
	dup_names[i] = param->String;
	}
    
    FILE *f = check_ptr(fopen("/home/israel/exp_log.swift", "a"));
    check_neg(fprintf(f, "aggregate_similarities(%g, %g, %g, %g, \"%s\", \"%s\")", params[0], params[1], params[2], params[3], dup_names[0], dup_names[1]));
    
    /** Compute aggregated similarity. **/
    double name_sim = params[0];
    double email_sim = params[1];
    double phone_sim = params[2];
    double address_sim = params[3];
    
    double mean = 0.0, n = 0.0;
    if (name_sim > 0.0) { mean += name_sim; n++; }
    if (email_sim > 0.0) { mean += email_sim; n++; }
    if (phone_sim > 0.0) { mean += phone_sim; n++; }
    if (address_sim > 0.0) { mean += address_sim; n++; }
    mean /= n;
    
    /** Success. **/
    tree->DataType = DATA_T_DOUBLE;
    tree->Types.Double = mean;
    fprintf(f, " = %g\n", tree->Types.Double);
    check(fclose(f));
    return 0;
    }


/*
 * exp_fn_argon2id
 * This method hashes a given password using the Argon2 algorithm (ID variant)
 *
 * Parameters:
 * 	pExpression tree: 
 * 	pParamObjects: 
 * 	pExpression passowrd: The password, passed as a pExpression
 * 	pExpression salt: The salt, passed as a pExpression
 *
 * returns:
 *	 0 if successful
 *	 -1 if error
 */
int exp_fn_argon2id(pExpression tree, pParamObjects objlist, pExpression password, pExpression salt)
{

    //check password and salt
    if (!password || !salt)
	{
	mssError(1, "EXP", "Invalid Parameters: argon2id() requires two arguments");
	return -1;
	}
    else if ((password->Flags | salt->Flags) & EXPR_F_NULL)
	{
	tree->Flags |= EXPR_F_NULL;
	tree->DataType = DATA_T_STRING;
	return 0;
	}

    // The default values of the following four variables should be tuned for each specific system's needs
    // T_COST determines the number of passes the algorithm makes
    unsigned int T_COST = 2; 
    if ((tree->Children.nItems >= 3) && 
	((tree->Children.Items[2]) != NULL) && 
	((pExpression)tree->Children.Items[2])->DataType == DATA_T_INTEGER && 
	((pExpression)tree->Children.Items[2])->Integer < 24 && 
	((pExpression)tree->Children.Items[2])->Integer > 0)
	{
	T_COST = ((pExpression)tree->Children.Items[2])->Integer;
	}

    // M_COST determines the amount of memory cost in kilobytes
    unsigned int M_COST = (1<<16);
    if ((tree->Children.nItems >= 4) &&
	((tree->Children.Items[3]) != NULL) && 
	((pExpression)tree->Children.Items[3])->DataType == DATA_T_INTEGER && 
	((pExpression)tree->Children.Items[3])->Integer < (2<<16) && 
	((pExpression)tree->Children.Items[3])->Integer >= 64) 
	{
	M_COST = ((pExpression)tree->Children.Items[3])->Integer;
	}

    // PARALLELISM determines the number of threads or 'lanes' used by the algorithm
    unsigned int PARALLELISM = 1;
    if ((tree->Children.nItems) >= 5 && 
	((tree->Children.Items[4]) != NULL) && 
	((pExpression)tree->Children.Items[4])->DataType == DATA_T_INTEGER &&
	((pExpression)tree->Children.Items[4])->Integer <= 8 && 
	((pExpression)tree->Children.Items[4])->Integer >=1)
	{
	PARALLELISM = ((pExpression)tree->Children.Items[4])->Integer;
	}

    // HASHLEN is the size of the finished hash
    unsigned int HASHLEN = 32;
    if ((tree->Children.nItems >= 6) && 
	((tree->Children.Items[5]) != NULL) && 
	((pExpression)tree->Children.Items[5])->DataType == DATA_T_INTEGER && 
	((pExpression)tree->Children.Items[5])->Integer < 256 && 
	((pExpression)tree->Children.Items[5])->Integer >= 4)
	{
	HASHLEN = ((pExpression)tree->Children.Items[5])->Integer;
	}
 
    // check if required parameters exist
    if (!password || !salt)
    	{
	mssError(1, "EXP", "Invalid Parameters: function usage: exp_argon2id(password, salt)");
	return -1;
	}
    tree->DataType = DATA_T_STRING;
    
    // hashvalue is where the output is written 
    unsigned char hashvalue[HASHLEN];

    unsigned char *slt = (unsigned char *)strdup(salt->String);
    unsigned char *pwd = (unsigned char *)strdup(password->String);
    unsigned int pwdlen = strlen((char*)pwd);
    unsigned int sltlen = strlen((char*)slt);

    // salt length check
    if (sltlen < 8)
	{
	mssError(1, "EXP", "Salt is too short: Salt must be at least 8 bytes (16 recommended)");
	return -1;
	}
    // this call to the argon2id_hash_raw method is where the magic happens
    // after this call, the hashed password is written in hashvalue
    argon2id_hash_raw(T_COST, M_COST, PARALLELISM, pwd, pwdlen, slt, sltlen, hashvalue, HASHLEN);
 
    // this is where we write the contents of hashvalue to tree using qpfPrintf
    if (HASHLEN*2+1 > sizeof(tree->Types.StringBuf))
	{
	tree->Alloc = 1;
	tree->String = nmSysMalloc(HASHLEN*2+1);
	if (!tree->String)
	    {
	    mssError(1, "EXP", "argon2id(): out of memory");
	    return -1; }
	}
    else
	{
	tree->Alloc = 0;
	tree->String = tree->Types.StringBuf;
	}
    qpfPrintf(NULL, tree->String, HASHLEN*2+1, "%*STR&HEX", HASHLEN, hashvalue);
    return 0;
}

int exp_internal_DefineFunctions()
    {
	/** Initialize library. **/
	ca_init();
	
	/** Function list for EXPR_N_FUNCTION nodes. **/
	
	/** General. **/
	xhAdd(&EXP.Functions, "user_name", (char*)exp_fn_user_name);
	xhAdd(&EXP.Functions, "convert", (char*)exp_fn_convert);
	xhAdd(&EXP.Functions, "wordify", (char*)exp_fn_wordify);
	xhAdd(&EXP.Functions, "abs", (char*)exp_fn_abs);
	xhAdd(&EXP.Functions, "ascii", (char*)exp_fn_ascii);
	xhAdd(&EXP.Functions, "condition", (char*)exp_fn_condition);
	xhAdd(&EXP.Functions, "isnull", (char*)exp_fn_isnull);
	xhAdd(&EXP.Functions, "eval", (char*)exp_fn_eval);
	xhAdd(&EXP.Functions, "truncate", (char*)exp_fn_truncate);
	xhAdd(&EXP.Functions, "constrain", (char*)exp_fn_constrain);
	xhAdd(&EXP.Functions, "has_endorsement", (char*)exp_fn_has_endorsement);
	xhAdd(&EXP.Functions, "rand", (char*)exp_fn_rand);
	xhAdd(&EXP.Functions, "nullif", (char*)exp_fn_nullif);
	xhAdd(&EXP.Functions, "hash", (char*)exp_fn_hash);
	xhAdd(&EXP.Functions, "hmac", (char*)exp_fn_hmac);
	xhAdd(&EXP.Functions, "pbkdf2", (char*)exp_fn_pbkdf2);
	xhAdd(&EXP.Functions, "octet_length", (char*)exp_fn_octet_length);
	xhAdd(&EXP.Functions, "argon2id",(char*)exp_fn_argon2id);
	
	/** Dates. **/
	xhAdd(&EXP.Functions, "getdate", (char*)exp_fn_getdate);
	xhAdd(&EXP.Functions, "datepart", (char*)exp_fn_datepart);
	xhAdd(&EXP.Functions, "dateadd", (char*)exp_fn_dateadd);
	xhAdd(&EXP.Functions, "datediff", (char*)exp_fn_datediff);
	
	/** Strings. **/
	xhAdd(&EXP.Functions, "ltrim", (char*)exp_fn_ltrim);
	xhAdd(&EXP.Functions, "lztrim", (char*)exp_fn_lztrim);
	xhAdd(&EXP.Functions, "rtrim", (char*)exp_fn_rtrim);
	xhAdd(&EXP.Functions, "trim", (char*)exp_fn_trim);
	xhAdd(&EXP.Functions, "substring", (char*)exp_fn_substring);
	xhAdd(&EXP.Functions, "right", (char*)exp_fn_right);
	xhAdd(&EXP.Functions, "ralign", (char*)exp_fn_ralign);
	xhAdd(&EXP.Functions, "replicate", (char*)exp_fn_replicate);
	xhAdd(&EXP.Functions, "reverse", (char*)exp_fn_reverse);
	xhAdd(&EXP.Functions, "replace", (char*)exp_fn_replace);
	xhAdd(&EXP.Functions, "escape", (char*)exp_fn_escape);
	xhAdd(&EXP.Functions, "quote", (char*)exp_fn_quote);
	xhAdd(&EXP.Functions, "substitute", (char*)exp_fn_substitute);
	xhAdd(&EXP.Functions, "upper", (char*)exp_fn_upper);
	xhAdd(&EXP.Functions, "lower", (char*)exp_fn_lower);
	xhAdd(&EXP.Functions, "mixed", (char*)exp_fn_mixed);
	xhAdd(&EXP.Functions, "char_length", (char*)exp_fn_char_length);
	xhAdd(&EXP.Functions, "charindex", (char*)exp_fn_charindex);
	xhAdd(&EXP.Functions, "dateformat", (char*)exp_fn_dateformat);
	xhAdd(&EXP.Functions, "moneyformat", (char*)exp_fn_moneyformat);
	
	/** Numbering systems (e.g. base 16 aka. hex, base 8 aka. octal, etc.). **/
	xhAdd(&EXP.Functions, "to_base64", (char*)exp_fn_to_base64);
	xhAdd(&EXP.Functions, "from_base64", (char*)exp_fn_from_base64);
	xhAdd(&EXP.Functions, "to_hex", (char*)exp_fn_to_hex);
	xhAdd(&EXP.Functions, "from_hex", (char*)exp_fn_from_hex);
	
	/** Math. **/
	xhAdd(&EXP.Functions, "round", (char*)exp_fn_round);
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
	xhAdd(&EXP.Functions, "log10", (char*)exp_fn_log10);
	xhAdd(&EXP.Functions, "ln", (char*)exp_fn_log_natural);
	xhAdd(&EXP.Functions, "logn", (char*)exp_fn_log_base_n);
	xhAdd(&EXP.Functions, "power", (char*)exp_fn_power);
	
	/** Duplicate detection. **/
	xhAdd(&EXP.Functions, "cos_cmp", (char*)exp_fn_cos_cmp);
	xhAdd(&EXP.Functions, "cos_compare", (char*)exp_fn_cos_compare);
	xhAdd(&EXP.Functions, "cosine_compare", (char*)exp_fn_cosine_compare);
	xhAdd(&EXP.Functions, "lev_cmp", (char*)exp_fn_lev_cmp);
	xhAdd(&EXP.Functions, "lev_compare", (char*)exp_fn_lev_compare);
	xhAdd(&EXP.Functions, "levenshtein_compare", (char*)exp_fn_levenshtein_compare);
	xhAdd(&EXP.Functions, "sparse_eql", (char*)exp_fn_sparse_eql);
	xhAdd(&EXP.Functions, "aggregate_similarities", (char*)exp_fn_aggregate_similarities);
	xhAdd(&EXP.Functions, "double_metaphone", (char*)exp_fn_double_metaphone);
	
	/** Windowing. **/
	xhAdd(&EXP.Functions, "row_number", (char*)exp_fn_row_number);
	xhAdd(&EXP.Functions, "dense_rank", (char*)exp_fn_dense_rank);
	xhAdd(&EXP.Functions, "lag", (char*)exp_fn_lag);
	
	/** Aggregate. **/
	xhAdd(&EXP.Functions, "count", (char*)exp_fn_count);
	xhAdd(&EXP.Functions, "avg", (char*)exp_fn_avg);
	xhAdd(&EXP.Functions, "sum", (char*)exp_fn_sum);
	xhAdd(&EXP.Functions, "max", (char*)exp_fn_max);
	xhAdd(&EXP.Functions, "min", (char*)exp_fn_min);
	xhAdd(&EXP.Functions, "first", (char*)exp_fn_first);
	xhAdd(&EXP.Functions, "last", (char*)exp_fn_last);
	xhAdd(&EXP.Functions, "nth", (char*)exp_fn_nth);
	
	
	/** Reverse functions. **/
	xhAdd(&EXP.ReverseFunctions, "isnull", (char*)exp_fn_reverse_isnull);
	
    return 0;
    }
