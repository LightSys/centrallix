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
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxss/cxss.h"
#include "expression.h"
#include "obj.h"

/** Duplocate detection settings. **/
// #define SEPARATOR "|"
// #define SEPARATOR_CHAR '|'
// #define DBL_BUF_SIZE 16u
// #define USE_PARALLEL_COMPLETE_SEARCH true
// #define MIN_PARALLEL_COMPLETE_SEARCH 1000
// #define MAX_COMPLETE_SEARCH 50 * 1000 // Default: 100 * 1000
// #define KMEANS_IMPROVEMENT_THRESHOLD 0.0002
#define EXP_NUM_DIMS 251 /* aka. The size of the vector table. */
const int EXP_VECTOR_TABLE_SIZE = EXP_NUM_DIMS; /* Should probably be removed. */

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
	/** Possible overflow? **/
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

/* See centrallix-sysdoc/string_comparison.md for more information. */
int exp_fn_levenshtein(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {

    if (!i0 || !i1)
	{
		mssError(1,"EXP","levenshtein() requires two parameters");
		return -1;
	}

    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
		tree->DataType = DATA_T_INTEGER;
		tree->Flags |= EXPR_F_NULL;
		return 0;
	}

    if ((i0->DataType != DATA_T_STRING) || (i1->DataType != DATA_T_STRING))
	{
		mssError(1,"EXP","levenshtein() requires two string parameters");
		return -1;
	}

	// for all i and j, d[i,j] will hold the Levenshtein distance between
	// the first i characters of s and the first j characters of t
	int length1 = strlen(i0->String);
	int length2 = strlen(i1->String);
	//int levMatrix[length1+1][length2+1];
	int (*levMatrix)[length1+1][length2+1] = nmSysMalloc(sizeof(*levMatrix));
	int i;
	int j;
    //set each element in d to zero
    for (i = 0; i < length1; i++)
    {
        for (j = 0; j < length2; j++)
        {
            (*levMatrix)[i][j] = 0;
        }        
    }
    
    // source prefixes can be transformed into empty string by
    // dropping all characters
    for (i = 0; i <= length1; i++)
    {
        (*levMatrix)[i][0] = i;
    }
     
    // target prefixes can be reached from empty source prefix
    // by inserting every character
    for (j = 0; j <= length2; j++)
    {
        (*levMatrix)[0][j] = j;
    }
    
	for (i = 1; i <= length1; i++)
    {
        for (j = 1; j <= length2; j++)
        {
            if (i0->String[i-1] == i1->String[j-1]) 
            {
                (*levMatrix)[i][j] = (*levMatrix)[i-1][j-1];
            }
            else 
            {
				int value1 = (*levMatrix)[i - 1][j] + 1;
				int value2 = (*levMatrix)[i][j-1] + 1;
				int value3 = (*levMatrix)[i-1][j-1] + 1;
                (*levMatrix)[i][j] = (value1 < value2) ? 
									  ((value1 < value3) ? value1 : value3) :
									  (value2 < value3) ? value2 : value3;
            }
        }
    }
    tree->DataType = DATA_T_INTEGER;
	tree->Integer = (*levMatrix)[length1][length2];
    nmSysFree(levMatrix);
    return 0;
    }

/* See centrallix-sysdoc/string_comparison.md for more information. */
int exp_fn_lev_compare(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {

    if (!i0 || !i1)
	{
		mssError(1,"EXP","lev_compare() requires two or three parameters");
		return -1;
	}

    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL) || (i2 && (i2->Flags & EXPR_F_NULL)))
	{
		tree->DataType = DATA_T_DOUBLE;
		tree->Flags |= EXPR_F_NULL;
		return 0;
	}

    if ((i0->DataType != DATA_T_STRING) || (i1->DataType != DATA_T_STRING) || (i2 && i2->DataType != DATA_T_INTEGER))
	{
		mssError(1,"EXP","lev_compare() requires two string and one optional integer parameters");
		return -1;
	}
	
	exp_fn_levenshtein(tree, objlist, i0, i1, i2);
	//!!! I am not checking for errors here, because IN THEORY we have two strings... if we don't, big uh-oh.
	int lev_dist = tree->Integer;
	
	int length1 = strlen(i0->String);
	int length2 = strlen(i1->String);

	double clamped_dist = 1.0;

	if (length1 == 0 || length2 == 0) //empty string
	{
		clamped_dist = 0.5;	
	} 
	else //normal case 
	{
		int max_len = (length1 > length2) ? length1 : length2;
		clamped_dist = ((double) lev_dist) / max_len;
	
		if (abs(length1-length2) == lev_dist)  //only inserts. Maybe substring.
		{
			clamped_dist /= 2;
		}
		
		//use max_field_width if it was provided as a sensible value. If not, don't use it.
		double max_field_width = i2?(i2->Integer):0;
		if (max_field_width && max_field_width >= max_len) {
			double mod = (lev_dist + max_field_width * 3/4) / max_field_width; 
			if (mod < 1) { //don't make clamped_dist bigger
				clamped_dist *= mod;
			}
		}
	}
	
	
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = 1.0 - clamped_dist;
	return 0;
}

/*
 * hash_char_pair
 * This method creates an vector table index based a given character pair. The characters are represented 
 * as their ASCII code points.
 *
 * Parameters:
 * 	num1 : first ASCII code point (double)
 * 	num2 : second ASCII code point (double)
 *
 * Returns:
 * 	vector table index (integer)
 */
int exp_fn_i_hash_char_pair(double num1, double num2)
    {
    int func_result = round(((num1 * num1 * num1) + (num2 * num2 * num2)) * ((num1+1)/(num2+1))) -1;
    return func_result % EXP_VECTOR_TABLE_SIZE;	
    }


/*
 * exp_fn_i_frequency_table
 * This method creates a vector frequency table based on a string of characters.
 *
 * Parameters:
 * 	table : integer pointer to vector frequency table (unsigned short)
 * 	term : the string of characters (char*)
 *
 * Returns:
 * 	0 	
 * 
 * LINK ../../centrallix-sysdoc/string_comparison.md#exp_fn_i_frequency_table
 */
int exp_fn_i_frequency_table(unsigned short *table, char *term)
    {
    int i;
    // Initialize hash table with 0 values
    for (i = 0; i < EXP_VECTOR_TABLE_SIZE; i++)
	{
	table[i] = 0;
	}

	int j = -1;
    for(i = 0; i < strlen(term) + 1; i++)
	{
	// If latter character is punctuation or whitespace, skip it
	if (ispunct(term[i]) || isspace(term[i]))
	    {
	    continue;
	    }

	double temp1 = 0.0;
	double temp2 = 0.0;

	// If previous character is null
	if (j == -1)
	    {
	    temp1 = 96;
	    }

	// Else character is not null
	else
	    {
	    temp1 = (int)tolower(term[j]);
	    }

	// If latter character is null
	if (i == strlen(term))
	    {
	    temp2 = 96;
	    }

	// Else character is not null
	else
	    {
	    temp2 = (int)tolower(term[i]);
	    }

	// Else character is not null	// If either character is a number, reassign the code point
	if (temp1 >= 48 && temp1 <= 57)
	    {
	    temp1 += 75;
	    }

	if (temp2 >= 48 && temp2 <= 57)
	    {
	    temp2 += 75;
	    }

	// Hash the character pair into an index
	int index = exp_fn_i_hash_char_pair(temp1, temp2);

	// Increment Frequency Table value by number from 0 to 13
	table[index] += ((unsigned short)temp1 + (unsigned short)temp2) % 13 + 1;

	// Move j up to latter character before incrementing i
	j = i;

	}

    return 0;	

    }

/*
 * exp_fn_i_dot_product
 * This method calculautes the dot product of two vectors.
 *
 * Parameters:
 * 	dot_product : the place where the result is stored (double)
 * 	r_freq_table1 : the first vector (unsigned short)
 * 	r_freq_table2 : the second vector (unsigned short)
 *
 * Returns:
 * 	0
 * 
 * LINK ../../centrallix-sysdoc/string_comparison.md#exp_fn_i_dot_product
 */
int exp_fn_i_dot_product(double *dot_product, unsigned short *r_freq_table1, unsigned short *r_freq_table2)
    {
    int i;
    for (i = 0; i < EXP_VECTOR_TABLE_SIZE; i++) 
        {
	*dot_product = *dot_product + ((double)r_freq_table1[i] * (double)r_freq_table2[i]);
	}
    return 0;
    }

/*
 * exp_fn_i_magnitude
 * This method calculates the magnitude of a vector
 *
 * Parameters:
 * 	magnitude : the place where the result is stored (double)
 * 	r_freq_table : the vector (unsigned short)
 * 
 * LINK ../../centrallix-sysdoc/string_comparison.md#exp_fn_i_magnitude
 */
int exp_fn_i_magnitude(double *magnitude, unsigned short *r_freq_table)
    {
    int i;
    for (i = 0; i < EXP_VECTOR_TABLE_SIZE; i++)
	{
	*magnitude = *magnitude + ((double)r_freq_table[i] * (double)r_freq_table[i]);
	}
    *magnitude = sqrt(*magnitude);
    return 0;
    }

/*
 * exp_fn_cos_compare
 * This method calculates the cosine similarity of two vector frequency tables
 * See centrallix-sysdoc/string_comparison.md for more information.
 *
 * Parameters:
 * 	tree : structure where output is stored
 *	objlist : unused
 *	i0 : first data entry (pExpression)
 *	i1 : second data entry (pExpression)
 *	i2 : unused
 *
 * Returns:
 * 	0 
 * 
 * LINK ../../centrallix-sysdoc/string_comparison.md#exp_fn_similarity
 */
int exp_fn_cos_compare(pExpression tree, pParamObjects objlist, pExpression i0, pExpression i1, pExpression i2)
    {
    // Ensure function receives two non-null parameters
    if (!i0 || !i1)
	{
	mssError(1,"EXP","cos_compare() requires two parameter.");
	return -1;
	}

    // Ensure value passed in both parameters is not null
    if ((i0->Flags & EXPR_F_NULL) || (i1->Flags & EXPR_F_NULL))
	{
	tree->DataType = DATA_T_DOUBLE;
	tree->Flags |= EXPR_F_NULL;
	return 0;
	}

    // Ensure both parameters contain string values
    if ((i0->DataType != DATA_T_STRING) || (i1->DataType != DATA_T_STRING))
	{
	mssError(1,"EXP","cos_compare() requires two string parameters.");
	return -1;
	}

    //If the two strings are identical, don't bother running cosine compare	
    if (strcmp(i0->String, i1->String) == 0)
	{
	tree->DataType = DATA_T_DOUBLE;
	tree->Types.Double = 1.0;
	return 0;
	}

    // Allocate frequency tables (arrays of integers) for each term
    unsigned short *table1 = nmMalloc(EXP_VECTOR_TABLE_SIZE * sizeof(unsigned short));
    unsigned short *table2 = nmMalloc(EXP_VECTOR_TABLE_SIZE * sizeof(unsigned short));

    if (table1 == NULL || table2 == NULL)
	{
	mssError(1,"EXP","Memory allocation failed.");
	return -1;
	}

    // Calculate frequency tables for each term
    exp_fn_i_frequency_table(table1, i0->String);
    exp_fn_i_frequency_table(table2, i1->String);
	
    // Calculate dot product
    double dot_product = 0;
    exp_fn_i_dot_product(&dot_product, table1, table2);

    // Calculate magnitudes of each relative frequency vector
    double magnitude1 = 0;
    double magnitude2 = 0;
    exp_fn_i_magnitude(&magnitude1, table1);
    exp_fn_i_magnitude(&magnitude2, table2);
    
    tree->DataType = DATA_T_DOUBLE;
    tree->Types.Double = dot_product / (magnitude1 * magnitude2);
    nmFree(table1, EXP_VECTOR_TABLE_SIZE * sizeof(unsigned short));
    nmFree(table2, EXP_VECTOR_TABLE_SIZE * sizeof(unsigned short));

    return 0;
    }

// /*** =========================
//  *** DUPE SECTION
//  *** By: Israel Fuller
//  *** Last Updated: September, 2025
//  *** 
//  *** This section of the file deals with finding duplocates.
//  ***/

// /*** @brief Returns the smaller of two values.
//  *** 
//  *** @param a The first value.
//  *** @param b The second value.
//  *** @return The smaller of the two values.
//  *** 
//  *** @note This macro uses GNU C extensions and is type-safe.
//  ***/
// #define min(a, b) ({ \
//     __typeof__ (a) _a = (a); \
//     __typeof__ (b) _b = (b); \
//     (_a < _b) ? _a : _b; \
// })

// /*** @brief Returns the larger of two values.
//  *** 
//  *** @param a The first value.
//  *** @param b The second value.
//  *** @return The larger of the two values.
//  *** 
//  *** @note This macro uses GNU C extensions and is type-safe.
//  ***/
// #define max(a, b) ({ \
//     __typeof__ (a) _a = (a); \
//     __typeof__ (b) _b = (b); \
//     (_a > _b) ? _a : _b; \
// })

// /** The character used to create a pair with the first and last characters of a string. **/
// #define EXP_BOUNDARY_CHAR ('a' - 1)

// /*** Helpful error handling function. **/
// void mssErrorf(int clr, char* module, const char* format, ...);

// /*** Gets the hash, representing a pair of ASCII characters, represented by unsigned ints.
//  *** 
//  *** @param num1 The first character in the pair.
//  *** @param num1 The second character in the pair.
//  *** @returns The resulting hash.
//  ***/
// unsigned int exp_fn_get_char_pair_hash(const unsigned int num1, const unsigned int num2)
//     {
//     if (num1 == EXP_BOUNDARY_CHAR && num2 == EXP_BOUNDARY_CHAR)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "exp_fn_get_char_pair_hash(%u, %u) - Warning: Pair of boundary characters.",
// 	    num1, num2
// 	);
// 	}
//     const double sum = (num1 * num1 * num1) + (num2 * num2 * num2);
//     const double scale = ((double)num1 + 1.0) / ((double)num2 + 1.0);
//     const unsigned int hash = (unsigned int)round(sum * scale) - 1u;
//     return hash % EXP_NUM_DIMS;
//     }

// /*** Builds a vector using a string.
//  *** 
//  *** Vectors are based on the frequencies of character pairs in the string.
//  *** Space characters and punctuation characters (see code for list) are ignored,
//  *** and all characters are converted to lowercase. Character 96, which is just
//  *** before 'a' in the ASCII table (and maps to '`') is used to make pairs on the
//  *** start and end of strings. The only supported characters for the passed char*
//  *** are spaces, punctuation, uppercase and lowercase letters, and numbers.
//  *** 
//  *** This results in the following modified ASCII table:
//  *** ```csv
//  *** #,   char, #,   char, #,   char
//  *** 97,  a,    109, m,    121, y
//  *** 98,  b,    110, n,    122, z
//  *** 99,  c,    111, o,    123, 0
//  *** 100, d,    112, p,    124, 1
//  *** 101, e,    113, q,    125, 2
//  *** 102, f,    114, r,    126, 3
//  *** 103, g,    115, s,    127, 4
//  *** 104, h,    116, t,    128, 5
//  *** 105, i,    117, u,    129, 6
//  *** 106, j,    118, v,    130, 7
//  *** 107, k,    119, w,    131, 8
//  *** 108, l,    120, x,    132, 9
//  *** ```
//  *** Thus, any number from 96 (the start/end character) to 132 ('9') is a valid
//  *** input to get_char_pair_hash().
//  *** 
//  *** After hashing each character pair, we add some number from 1 to 13 to the
//  *** coresponding dimention. However, for most names, this results in a lot of
//  *** zeros and a FEW positive numbers. Thus, after creating the dense vector,
//  *** we convert it to a sparse vector in which a negative number replaces a run
//  *** of that many zeros. Consider the following example:
//  *** 
//  *** Dense Vector: `[1,0,0,0,3,0]`
//  *** 
//  *** Sparse Vector: `[1,-3,3,-1]`
//  *** 
//  *** Using these sparse vectors greatly reduces the required memory and gives
//  *** aproximately an x5 boost to performance when traversing vectors, at the
//  *** cost of more algorithmically complex code.
//  *** 
//  *** @param str The string to be divided into pairs and hashed to make the vector.
//  *** @returns The sparse vector built using the hashed character pairs.
//  ***/
// int* build_vector(char* str) {
//     /** Allocate space for a dense vector. **/
//     unsigned int dense_vector[EXP_NUM_DIMS] = {0u};
    
//     /** j is the former character, i is the latter. **/
//     const unsigned int num_chars = (unsigned int)strlen(str);
//     for (unsigned int j = 65535u, i = 0u; i <= num_chars; i++)
// 	{
// 	/** isspace: space, \n, \v, \f, \r **/
// 	if (isspace(str[i])) continue;
	
// 	/** ispunct: !"#$%&'()*+,-./:;<=>?@[\]^_{|}~ **/
// 	if (ispunct(str[i]) && str[i] != EXP_BOUNDARY_CHAR) continue;
	
// 	/*** iscntrl (0-8):   SOH, STX, ETX, EOT, ENQ, ACK, BEL, BS
// 	 ***         (14-31): SO, SI, DLE, DC1-4, NAK, SYN, ETB, CAN
// 	 ***                  EM, SUB, ESC, FS, GS, RS, US
// 	 ***/
// 	if (iscntrl(str[i]) && i != num_chars) {
// 	    mssErrorf(1, "EXP",
// 		"build_vector(%s) - Warning: Skipping unknown character #%u.\n",
// 		str, (unsigned int)str[i]
// 	    );
// 	    continue;
// 	}
	
// 	/** First and last character should fall one before 'a' in the ASCII table. **/
// 	unsigned int temp1 = (j == 65535u) ? EXP_BOUNDARY_CHAR : (unsigned int)tolower(str[j]);
// 	unsigned int temp2 = (i == num_chars) ? EXP_BOUNDARY_CHAR : (unsigned int)tolower(str[i]);
	
// 	/** Shift numbers to the end of the lowercase letters. **/
// 	if ('0' <= temp1 && temp1 <= '9') temp1 += 75u;
// 	if ('0' <= temp2 && temp2 <= '9') temp2 += 75u;
	
// 	/** Hash the character pair into an index (dimension).  **/
// 	/** Note that temp will be between 97 ('a') and 132 ('9'). **/
// 	unsigned int dim = exp_fn_get_char_pair_hash(temp1, temp2);
	
// 	/** Increment the dimension of the dense vector by a number from 1 to 13. **/
// 	dense_vector[dim] += (temp1 + temp2) % 13u + 1u;
	
// 	j = i;
// 	}
    
//     /** Count how much space is needed for a sparse vector. **/
//     bool zero_prev = false;
//     size_t size = 0u;
//     for (unsigned int dim = 0u; dim < EXP_NUM_DIMS; dim++)
// 	{
// 	if (dense_vector[dim] == 0u)
// 	    {
// 	    size += (zero_prev) ? 0u : 1u;
// 	    zero_prev = true;
// 	    }
// 	else
// 	    {
// 	    size++;
// 	    zero_prev = false;
// 	    }
// 	}
    
//     /*** Check compression size.
//      *** If this check fails, I doubt anything will break. However, the longest
//      *** word I know (supercalifragilisticexpialidocious) has only 35 character
//      *** pairs, so it shouldn't reach half this size (and it'd be even shorter
//      *** if the hash generates at least one collision).
//      *** 
//      *** Bad vector compression will result in degraded performace and increased
//      *** memory usage, and likely also indicates a bug or modified assumption
//      *** elsewhere in the code.
//      *** 
//      *** If this warning is ever generated, it's definitely worth investigating.
//      ***/
//     const size_t expected_max_size = 64u;
//     if (size > expected_max_size)
// 	{
// 	mssErrorf(1, "EXP"
// 	    "build_vector(%s) - Warning: Sparse vector larger than expected.\n"
// 	    "    > Size: %lu\n"
// 	    "    > #Dims: %u\n",
// 	    str,
// 	    size,
// 	    EXP_NUM_DIMS
// 	);
// 	}
    
//     /** Allocate space for sparse vector. **/
//     const size_t sparse_vector_size = size * sizeof(int);
//     int* sparse_vector = (int*)nmSysMalloc(sparse_vector_size);
//     if (sparse_vector == NULL) {
// 	mssErrorf(1, "EXP",
// 	    "build_vector(%s) - nmSysMalloc(%lu) failed.",
// 	    str, sparse_vector_size
// 	);
// 	return NULL;
//     }
    
//     /** Convert the dense vector above to a sparse vector. **/
//     unsigned int j = 0u, sparse_idx = 0u;
//     while (j < EXP_NUM_DIMS)
//         {
// 	if (dense_vector[j] == 0u)
// 	    {
// 	    /*** Count and store consecutive zeros, except the first one,
// 	     *** which we already know is zero.
// 	     ***/
// 	    unsigned int zero_count = 1u;
// 	    j++;
// 	    while (j < EXP_NUM_DIMS && dense_vector[j] == 0u)
// 	        {
// 		zero_count++;
// 		j++;
// 	        }
// 	    sparse_vector[sparse_idx++] = (int)-zero_count;
// 	    }
// 	else
// 	    {
// 	    /** Store the value. **/
// 	    sparse_vector[sparse_idx++] = (int)dense_vector[j++];
// 	    }
// 	}
    
//     return sparse_vector;
// }

// /*** Compute the magnitude of a sparsely allocated vector.
//  *** 
//  *** @param vector The vector.
//  *** @returns The computed magnitude.
//  ***/
// double exp_fn_magnitude_sparse(const int* vector)
//     {
//     unsigned int magnitude = 0u;
//     for (unsigned int i = 0u, dim = 0u; dim < EXP_NUM_DIMS;)
// 	{
// 	const int val = vector[i++];
	
// 	/** Negative val represents -val 0s in the array, so skip that many values. **/
// 	if (val < 0) dim += (unsigned)(-val);
	
// 	/** We have a param_value, so square it and add it to the magnitude. **/
// 	else { magnitude += (unsigned)(val * val); dim++; }
// 	}
//     return sqrt((double)magnitude);
//     }

// /*** Compute the magnitude of a densely allocated centroid.
//  *** 
//  *** @param centroid The centroid.
//  *** @returns The computed magnitude.
//  ***/
// double exp_fn_magnitude_dense(const double* centroid)
//     {
//     double magnitude = 0.0;
//     for (int i = 0; i < EXP_NUM_DIMS; i++)
// 	magnitude += centroid[i] * centroid[i];
//     return sqrt(magnitude);
//     }

// /*** Parse a token from a sparsely allocated vector and write the param_value and
//  *** number of remaining values to the passed locations.
//  *** 
//  *** @param token The sparse vector token being parsed.
//  *** @param remaining The location to save the remaining number of characters.
//  *** @param param_value The location to save the param_value of the token.
//  ***/
// void exp_fn_parse_token(const int token, unsigned int* remaining, unsigned int* param_value) {
//     if (token < 0)
// 	{
// 	/** This run contains -token zeros. **/
// 	*remaining = (unsigned)(-token);
// 	*param_value = 0u;
// 	}
//     else
// 	{
// 	/** This run contains one param_value. **/
// 	*remaining = 1u;
// 	*param_value = (unsigned)(token);
// 	}
// }

// /*** Calculate the similarity on sparcely allocated vectors. Comparing
//  *** any string to an empty string should always return 0.5 (untested).
//  *** 
//  *** @param v1 Sparse vector #1.
//  *** @param v2 Sparse vector #2.
//  *** @returns Similarity between 0 and 1 where
//  ***     1 indicates identical and
//  ***     0 indicates completely different.
//  ***/
// double exp_fn_sparse_similarity(const int* v1, const int* v2)
//     {
//     /** Calculate dot product. **/
//     unsigned int vec1_remaining = 0u, vec2_remaining = 0u;
//     unsigned int dim = 0u, i1 = 0u, i2 = 0u, dot_product = 0u;
//     while (dim < EXP_NUM_DIMS)
// 	{
// 	unsigned int val1 = 0u, val2 = 0u;
// 	if (vec1_remaining == 0u) exp_fn_parse_token(v1[i1++], &vec1_remaining, &val1);
// 	if (vec2_remaining == 0u) exp_fn_parse_token(v2[i2++], &vec2_remaining, &val2);
	
// 	/*** Accumulate the dot_product. If either vector is 0 here,
// 	 *** the total is 0 and this statement does nothing.
// 	 ***/
// 	dot_product += val1 * val2;
	
// 	/** Consume overlap from both runs. **/
// 	unsigned int overlap = min(vec1_remaining, vec2_remaining);
// 	vec1_remaining -= overlap;
// 	vec2_remaining -= overlap;
// 	dim += overlap;
// 	}
    
//     /** Optional optimization to speed up nonsimilar vectors. **/
//     if (dot_product == 0u) return 0.0;
    
//     /** Return the difference score. **/
//     return (double)dot_product / (exp_fn_magnitude_sparse(v1) * exp_fn_magnitude_sparse(v2));
//     }

// /*** Calculate the difference on sparcely allocated vectors. Comparing
//  *** any string to an empty string should always return 0.5 (untested).
//  *** 
//  *** @param v1 Sparse vector #1.
//  *** @param v2 Sparse vector #2.
//  *** @returns Similarity between 0 and 1 where
//  ***     1 indicates completely different and
//  ***     0 indicates identical.
//  ***/
// #define exp_fn_sparse_dif(v1, v2) (1.0 - exp_fn_sparse_similarity(v1, v2))

// /*** Calculate the similarity between a sparsely allocated vector
//  *** and a densely allocated centroid. Comparing any string to an
//  *** empty string should always return 0.5 (untested).
//  *** 
//  *** @param v1 Sparse vector #1.
//  *** @param c1 Dense centroid #2.
//  *** @returns Similarity between 0 and 1 where
//  ***     1 indicates identical and
//  ***     0 indicates completely different.
//  ***/
// double exp_fn_sparse_similarity_c(const int* v1, const double* c2)
//     {
//     /** Calculate dot product. **/
//     double dot_product = 0.0;
//     for (unsigned int i = 0u, dim = 0u; dim < EXP_NUM_DIMS;)
// 	{
// 	const int val = v1[i++];
	
// 	/** Negative val represents -val 0s in the array, so skip that many values. **/
// 	if (val < 0) dim += (unsigned)(-val);
	
// 	/** We have a param_value, so square it and add it to the magnitude. **/
// 	else dot_product += (double)val * c2[dim++];
// 	}
    
//     /** Return the difference score. **/
//     return dot_product / (exp_fn_magnitude_sparse(v1) * exp_fn_magnitude_dense(c2));
//     }

// /*** Calculate the difference between a sparsely allocated vector
//  *** and a densely allocated centroid. Comparing any string to an
//  *** empty string should always return 0.5 (untested).
//  *** 
//  *** @param v1 Sparse vector #1.
//  *** @param c1 Dense centroid #2.
//  *** @returns Difference between 0 and 1 where
//  ***     1 indicates completely different and
//  ***     0 indicates identical.
//  ***/
// #define exp_fn_sparse_dif_c(v1, c2) (1.0 - exp_fn_sparse_similarity_c(v1, c2))

// /*** Calculate the average size of all clusters in a set of vectors.
//  *** 
//  *** @param vectors The vectors of the dataset (allocated sparsely).
//  *** @param num_vectors The number of vectors in the dataset.
//  *** @param labels The clusters to which vectors are assigned.
//  *** @param centroids The locations of the centroids (allocated densely).
//  *** @param num_clusters The number of centroids (k).
//  *** @returns The average cluster size.
//  ***/
// double exp_fn_get_cluster_size(
//     int** vectors,
//     const unsigned int num_vectors,
//     unsigned int* labels,
//     double centroids[][EXP_NUM_DIMS],
//     const unsigned int num_clusters
// )
//     {
//     double cluster_sums[num_clusters];
//     unsigned int cluster_counts[num_clusters];
//     for (unsigned int i = 0u; i < num_clusters; i++)
// 	cluster_sums[i] = 0.0;
//     memset(cluster_counts, 0, sizeof(cluster_counts));
    
//     /** Sum the difference from each vector to its cluster centroid. **/
//     for (unsigned int i = 0u; i < num_vectors; i++)
// 	{
// 	const unsigned int label = labels[i];
// 	cluster_sums[label] += exp_fn_sparse_dif_c(vectors[i], centroids[label]);
// 	cluster_counts[label]++;
// 	}
    
//     /** Add up the average cluster size. **/
//     double cluster_total = 0.0;
//     unsigned int num_valid_clusters = 0u;
//     for (unsigned int label = 0u; label < num_clusters; label++)
// 	{
// 	const unsigned int cluster_count = cluster_counts[label];
// 	if (cluster_count == 0u) continue;
	
// 	cluster_total += cluster_sums[label] / cluster_count;
// 	num_valid_clusters++;
// 	}
    
//     /** Return average sizes. **/
//     return cluster_total / num_valid_clusters;
//     }

// /*** Compute the param_value for `k` (number of clusters), given a dataset of with
//  *** a size of `n`.
//  *** 
//  *** The following table shows data sizes vs.selected cluster size. In testing,
//  *** these numbers tended to givea good balance of accuracy and dulocates detected.
//  *** 
//  *** ```csv
//  *** Data Size, Actual
//  *** 10k,       12
//  *** 100k,      33
//  *** 1M,        67
//  *** 4M,        93
//  *** ```
//  *** 
//  *** This function is not intended for datasets smaller than (`n < ~2000`).
//  *** These should be handled using complete search.
//  *** 
//  *** LaTeX Notation: \log_{36}\left(n\right)^{3.1}-8
//  *** 
//  *** @param n The size of the dataset.
//  *** @returns k, the number of clusters to use.
//  *** 
//  *** Complexity: `O(1)`
//  ***/
// unsigned int exp_fn_compute_k(const unsigned int n)
//     {
//     return (unsigned)max(2, pow(log(n) / log(36), 3.2) - 8);
//     }

// /*** Executes the k-means clustering algorithm. Selects NUM_CLUSTERS random
//  *** vectors as initial centroids. Then points are assigned to the nearest
//  *** centroid, after which centroids are moved to the center of their points.
//  *** 
//  *** @param vectors The vectors to cluster.
//  *** @param num_vectors The number of vectors to cluster.
//  *** @param labels Stores the final cluster identities of the vectors after
//  ***     clustering is completed.
//  *** @param centroids Stores the locations of the centroids used for the clusters
//  ***     of the data.
//  *** @param iterations The number of iterations that actually executed is stored
//  ***     here. Leave this NULL if you don't care.
//  *** @param max_iter The max number of iterations.
//  *** @param num_clusters The number of clusters to generate.
//  ***
//  *** @attention - Assumes: num_vectors is the length of vectors.
//  *** @attention - Assumes: num_clusters is the length of labels.
//  ***
//  *** @attention - Issue: At larger numbers of clustering iterations, some
//  ***     clusters have a size of    negative infinity. In this implementation,
//  ***     the bug is mitigated by setting a small number of max iterations,
//  ***     such as 16 instead of 100.
//  *** @attention - Issue: Clusters do not apear to improve much after the first
//  ***     iteration, which puts the efficacy of the algorithm into question. This
//  ***     may be due to the uneven density of a typical dataset. However, the
//  ***     clusters still offer useful information.
//  *** 
//  *** Complexity:
//  *** 
//  *** - `O(kd + k + i*(k + n*(k+d) + kd))`
//  *** 
//  *** - `O(kd + k + ik + ink + ind + ikd)`
//  *** 
//  *** - `O(nk + nd)`
//  ***/
// void exp_fn_kmeans(
//     int** vectors,
//     const unsigned int num_vectors,
//     unsigned int* labels,
//     const unsigned int num_clusters,
//     const unsigned int max_iter
// )
//     {
//     // const size_t centroids_size = num_clusters * sizeof(double*);
//     // const size_t centroid_size = EXP_NUM_DIMS * sizeof(double);
//     // double** centroids = (double**)nmMalloc(centroids_size);
//     // if (centroids == NULL)
//     //     {
//     //     fprintf(stderr, "exp_fn_kmeans() - nmMalloc(%u) failed.\n", centroids_size);
//     //     return;
//     //     }
//     // for (int i = 0; i < num_clusters; i++)
//     //     {
//     //     double* centroid = centroids[i] = (double*)nmMalloc(centroid_size);
//     //     if (centroid == NULL)
//     //         {
//     //         fprintf(stderr, "exp_fn_kmeans() - nmMalloc(%u) failed.\n", centroid_size);
//     //         return;
//     //         }
//     //     memset(centroids[i], 0, centroid_size);
//     //     }
//     double centroids[num_clusters][EXP_NUM_DIMS];
//     memset(centroids, 0, sizeof(centroids));
    
//     /** Select random vectors to use as the initial centroids. **/
//     srand(time(NULL));
//     for (unsigned int i = 0u; i < num_clusters; i++)
// 	{
// 	// Pick a random vector.
// 	const unsigned int random_index = (unsigned int)rand() % num_vectors;
	
// 	// Sparse copy the vector into a densely allocated centroid.
// 	double* centroid = centroids[i];
// 	const int* vector = vectors[random_index];
// 	for (unsigned int i = 0u, dim = 0u; dim < EXP_NUM_DIMS;)
// 	    {
// 	    const int token = vector[i++];
// 	    if (token > 0) centroid[dim++] = (double)token;
// 	    else for (unsigned int j = 0u; j < -token; j++) centroid[dim++] = 0.0;
// 	    }
// 	}
    
//     /** Allocate memory for new centroids. **/
//     double new_centroids[num_clusters][EXP_NUM_DIMS];
    
//     /** Main exp_fn_kmeans loop. **/
//     double old_average_cluster_size = 1.0;
//     unsigned int cluster_counts[num_clusters];
//     for (unsigned int iter = 0u; iter < max_iter; iter++)
// 	{
// 	bool changed = false;
	
// 	/** Reset new centroids. **/
// 	for (unsigned int i = 0u; i < num_clusters; i++)
// 	    {
// 	    cluster_counts[i] = 0u;
// 	    for (unsigned int dim = 0; dim < EXP_NUM_DIMS; dim++)
// 		new_centroids[i][dim] = 0.0;
// 	    }
	
// 	/** Assign each point to the nearest centroid. **/
// 	for (unsigned int i = 0u; i < num_vectors; i++)
// 	    {
// 	    const int* vector = vectors[i];
// 	    double min_dist = DBL_MAX;
// 	    unsigned int best_centroid_label = 0u;
	    
// 	    // Find nearest centroid.
// 	    for (unsigned int j = 0u; j < num_clusters; j++)
// 		{
// 		const double dist = exp_fn_sparse_dif_c(vector, centroids[j]);
// 		if (dist < min_dist)
// 		    {
// 		    min_dist = dist;
// 		    best_centroid_label = j;
// 		    }
// 		}
		
// 	    /** Update label to new centroid, if necessary. **/
// 	    if (labels[i] != best_centroid_label)
// 		{
// 		labels[i] = best_centroid_label;
// 		changed = true;
// 		}
	    
// 	    /** Accumulate values for new centroid calculation. **/
// 	    double* best_centroid = new_centroids[best_centroid_label];
// 	    for (unsigned int i = 0u, dim = 0u; dim < EXP_NUM_DIMS;)
// 		{
// 		const int val = vector[i++];
// 		if (val < 0) dim += (unsigned)(-val);
// 		else best_centroid[dim++] += (double)val;
// 		}
// 	    cluster_counts[best_centroid_label]++;
// 	    }
	
// 	/** Stop if centroids didn't change. **/
// 	if (!changed) break;
	
// 	/** Update centroids. **/
// 	for (unsigned int i = 0u; i < num_clusters; i++)
// 	    {
// 	    if (cluster_counts[i] == 0u) continue;
// 	    double* centroid = centroids[i];
// 	    const double* new_centroid = new_centroids[i];
// 	    const unsigned int cluster_count = cluster_counts[i];
// 	    for (unsigned int dim = 0u; dim < EXP_NUM_DIMS; dim++)
// 		centroid[dim] = new_centroid[dim] / cluster_count;
// 	    }
	
// 	/** Print cluster size for debugging. **/
// 	const double average_cluster_size = exp_fn_get_cluster_size(vectors, num_vectors, labels, centroids, num_clusters);
	
// 	/** Is there enough improvement? **/
// 	const double improvement = old_average_cluster_size - average_cluster_size;
// 	if (improvement < KMEANS_IMPROVEMENT_THRESHOLD) break;
// 	old_average_cluster_size = average_cluster_size;
//     }
    
//     // Free unused memory.
// //     for (int i = 0; i < num_clusters; i++) {
// // 	nmFree(centroids[i], centroid_size);
// //     }
// //     nmFree(centroids, centroids_size);
// }

// /** Duplocate information. **/
// typedef struct
//     {
//     unsigned int id1;
//     unsigned int id2;
//     double similarity;
//     }
//     Dup, *pDup;

// /*** Runs complete search to find duplocates if `num_vectors <  MAX_COMPLETE_SEARCH`
//  *** and runs a search using k-means clustering on larger amounts of data.
//  *** 
//  *** @param vectors Array of precomputed frequency vectors for all dataset strings.
//  *** @param num_vectors The number of vectors to be scanned.
//  *** @param dupe_threshold The similarity threshold, below which dups are ignored.
//  *** @returns The duplicates in pDup structs.
//  ***/
// pXArray lightning_search(int** vectors, const unsigned int num_vectors, const double dupe_threshold)
//     {
//     /** Allocate space for dups. **/
//     const size_t guess_size = num_vectors * 2u;
//     pXArray dups = xaNew(guess_size);
//     if (dups == NULL)
// 	{
// 	mssErrorf(1, "EXP", "lightning_search() - xaNew(%lu) failed.", guess_size);
// 	return NULL;
// 	}
    
//     /** Descide which algorithm to use. **/
//     if (num_vectors <= MAX_COMPLETE_SEARCH)
// 	{ /** Do a complete search. **/
// 	for (unsigned int i = 0u; i < num_vectors; i++)
// 	    {
// 	    const int* v1 = vectors[i];
// 	    for (unsigned int j = i + 1u; j < num_vectors; j++)
// 		{
// 		const int* v2 = vectors[j];
// 		const double similarity = exp_fn_sparse_similarity(v1, v2);
// 		if (similarity > dupe_threshold) // Dup found!
// 		    {
// 		    Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
// 		    if (dup == NULL)
// 			{
// 			mssErrorf(1, "EXP", "lightning_search() - nmMalloc(%lu) failed.", sizeof(Dup));
// 			goto err_free_dups;
// 			}
		    
// 		    dup->id1 = i;
// 		    dup->id2 = j;
// 		    dup->similarity = similarity;
// 		    xaAddItem(dups, (void*)dup);
// 		    }
// 		}
// 	    }
// 	}
//     else
// 	{ /** Do a k-means search. **/
// 	/** Define constants for the algorithm. **/
// 	const unsigned int max_iter = 64u; /** Hardcode value because idk. **/
// 	const unsigned int num_clusters = exp_fn_compute_k(num_vectors);
	
// 	/** Allocate static memory for finding clusters. **/
// 	unsigned int labels[num_vectors];
// 	memset(labels, 0u, sizeof(labels));
	
// 	/** Execute kmeans clustering. **/
// 	exp_fn_kmeans(vectors, num_vectors, labels, num_clusters, max_iter);
	
// 	/** Find duplocates in clusters. **/
// 	for (unsigned int i = 0u; i < num_vectors; i++)
// 	    {
// 	    const int* v1 = vectors[i];
// 	    const unsigned int label = labels[i];
// 	    for (unsigned int j = i + 1u; j < num_vectors; j++)
// 		{
// 		if (labels[j] != label) continue;
// 		const int* v2 = vectors[j];
// 		const double similarity = exp_fn_sparse_similarity(v1, v2);
// 		if (similarity > dupe_threshold) /* Dup found! */
// 		    {
// 		    Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
// 		    if (dup == NULL)
// 			{
// 			mssErrorf(1, "EXP",
// 			    "lightning_search() - nmMalloc(%lu) failed.",
// 			    sizeof(Dup)
// 			);
// 			goto err_free_dups;
// 			}
		    
// 		    dup->id1 = i;
// 		    dup->id2 = j;
// 		    dup->similarity = similarity;
// 		    xaAddItem(dups, (void*)dup);
// 		    }
// 		}
// 	    }
// 	}
    
//     /** Done **/
//     return dups;
    
//     /** Free dups. **/
//     err_free_dups:;
//     const size_t num_dups = dups->nItems;
//     for (unsigned int i = 0u; i < num_dups; i++)
// 	{
// 	nmFree(dups->Items[i], sizeof(Dup));
// 	dups->Items[i] = NULL;
// 	}
//     xaDeInit(dups);
//     return NULL;
//     }

// /*** Computes Levenshtein distance between two strings.
//  *** 
//  *** @param str1 The first string.
//  *** @param str2 The second string.
//  *** @param length1 The length of the first string.
//  *** @param length1 The length of the first string.
//  *** 
//  *** @attention - Tip: Pass 0 for the length of either string to infer it
//  *** 	using the null terminating character. Thus, strings with no null
//  *** 	terminator are supported if you pass explicit lengths.
//  ***  
//  *** Complexity: O(length1 * length2).
//  *** 
//  *** @see centrallix-sysdoc/string_comparison.md
//  ***/
// unsigned int exp_fn_edit_dist(const char* str1, const char* str2, const size_t str1_length, const size_t str2_length)
//     {
//     /*** lev_matrix:
//      *** For all i and j, d[i][j] will hold the Levenshtein distance between
//      *** the first i characters of s and the first j characters of t.
//      *** 
//      *** As they say, no dynamic programming algorithm is complete without a
//      *** matrix that you fill out and it has the answer in the final location.
//      ***/
//     const size_t str1_len = (str1_length == 0u) ? strlen(str1) : str1_length;
//     const size_t str2_len = (str2_length == 0u) ? strlen(str2) : str2_length;
//     unsigned int lev_matrix[str1_len + 1][str2_len + 1];
    
//     /*** Base case #0:
//      *** Transforming an empty string into an empty string has 0 cost.
//      ***/
//     lev_matrix[0][0] = 0u;
    
//     /*** Base case #1:
//      *** Any source prefixe can be transformed into an empty string by
//      *** dropping each character.
//      ***/
//     for (unsigned int i = 1u; i <= str1_len; i++)
// 	lev_matrix[i][0] = i;
    
//     /*** Base case #2:
//      *** Any target prefixes can be transformed into an empty string by
//      *** inserting each character.
//      ***/
//     for (unsigned int j = 1u; j <= str2_len; j++)
// 	lev_matrix[0][j] = j;
    
//     /** General Case **/
//     for (unsigned int i = 1u; i <= str1_len; i++)
// 	{
// 	for (unsigned int j = 1u; j <= str2_len; j++)
// 	    {
// 	    /** Equal characters need no changes. **/
// 	    if (str1[i - 1] == str2[j - 1])
// 		lev_matrix[i][j] = lev_matrix[i - 1][j - 1];
	    
// 	    /*** We need to make a change, so use the opereration with the
// 	     *** lowest cost out of delete, insert, replace, or swap.
// 	     ***/
// 	    else 
// 		{
// 		unsigned int cost_delete  = lev_matrix[i - 1][j] + 1u;
// 		unsigned int cost_insert  = lev_matrix[i][j - 1] + 1u;
// 		unsigned int cost_replace = lev_matrix[i-1][j-1] + 1u;
		
// 		/** If a swap is possible, calculate the cost. **/
// 		bool can_swap = (
// 		    i > 1 && j > 1 &&
// 		    str1[i - 1] == str2[j - 2] &&
// 		    str1[i - 2] == str2[j - 1]
// 		);
// 		unsigned int cost_swap = (can_swap) ? lev_matrix[i - 2][j - 2] + 1 : UINT_MAX;
		
// 		// Find the best operation.
// 		lev_matrix[i][j] = min(min(min(cost_delete, cost_insert), cost_replace), cost_swap);
// 		}
// 	    }
// 	}
    
//     return lev_matrix[str1_len][str2_len];
//     }

// /*** Runs complete search to find duplocates in phone numbers using the
//  *** levenshtein min edit distance algorithm.
//  ***
//  *** @param dataset An array of characters for all dataset strings.
//  *** @param dataset_size The number of phone numbers to be scanned.
//  *** @param dupe_threshold The similarity threshold, below which dups are ignored.
//  *** @returns The duplicates in pDup structs.
//  ***/
// pXArray phone_search(char dataset[][10u], const unsigned int dataset_size, const double dupe_threshold)
//     {
//     /** Allocate space for dups. **/
//     const size_t guess_size = dataset_size * 2u;
//     pXArray dups = xaNew(guess_size);
//     if (dups == NULL)
// 	{
// 	mssErrorf(1, "EXP", "phone_search() - xaNew(%lu) failed.", guess_size);
// 	return NULL;
// 	}
    
//     /** Search for dups using edit distance. **/
//     for (unsigned int i = 0u; i < dataset_size; i++)
// 	{
// 	const char* v1 = dataset[i];
// 	for (unsigned int j = i + 1u; j < dataset_size; j++)
// 	    {
// 	    const char* v2 = dataset[j];
// 	    const unsigned int dist = exp_fn_edit_dist(v1, v2, 10u, 10u);
// 	    const double similarity = (double)dist / 10.0;
// 	    if (similarity > dupe_threshold) /* Dup found! */
// 		{
// 		Dup* dup = (Dup*)nmMalloc(sizeof(Dup));
// 		if (dup == NULL)
// 		    {
// 		    mssErrorf(1, "EXP", "phone_search() - nmMalloc(%lu) failed.", sizeof(Dup));
		    
// 		    /** Free data before returning. **/
// 		    const size_t num_dups = dups->nItems;
// 		    for (unsigned int i = 0u; i < num_dups; i++)
// 			{
// 			void* dup = dups->Items[i];
// 			nmFree(dup, sizeof(Dup));
// 			}
// 		    xaDeInit(dups);
// 		    return NULL;
// 		    }
		
// 		dup->id1 = i;
// 		dup->id2 = j;
// 		dup->similarity = similarity;
// 		xaAddItem(dups, (void*)dup);
// 		}
// 	    }
// 	}
    
//     return dups;
//     }

// /*** Usage: get_dups(<threshold : 0 - 1>, <out_file_path : path>, <data : string> )
//  *** data is assumed to contain only the following characters:
//  *** (Data containing ` or control characters is undefined.)
//  *** \n\v\f\r 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghij
//  *** klmnopqrstuvwxyz!"#$%&'()*+,-./:;<=>?@[\]^_{|}~
//  ***/
// int exp_fn_get_dups_general(pExpression tree, pParamObjects objlist, pExpression maybe_dup_threshold, pExpression maybe_out_file_path, pExpression maybe_data, const char* fn_name, bool is_phone_numbers)
//     {
//     /** Check number of arguments. **/
//     if (!maybe_dup_threshold || !maybe_out_file_path || !maybe_data)
// 	{
// 	mssErrorf(1, "EXP", "%s(?) expects 3 parameters.", fn_name);
// 	return -1;
// 	}
//     const int num_params = tree->Children.nItems;
//     if (num_params != 3)
// 	{
// 	mssErrorf(1, "EXP", "%s(?) expects 3 parameter, got %d.", fn_name, num_params);
// 	return -1;
// 	}
    
//     /** Magic checks. **/
//     ASSERTMAGIC(tree, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_dup_threshold, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_out_file_path, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_data, MGK_EXPRESSION);
    
//     /** Check object list. **/
//     if (!objlist)
// 	{
// 	mssErrorf(1, "EXP", "%s(\?\?\?) no object list?", fn_name);
// 	return -1;
// 	}
//     ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
//     /** Extract dup_threshold. **/
//     if (maybe_dup_threshold->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP", "%s(NULL, ...) dup_threshold cannot be NULL.", fn_name);
// 	return -1;
// 	}
//     if (maybe_dup_threshold->DataType != DATA_T_DOUBLE)
// 	{
// 	mssErrorf(1, "EXP", "%s(?, ...) dup_threshold must be a doube.", fn_name);
// 	return -1;
// 	}
//     double dup_threshold = maybe_dup_threshold->Types.Double;
//     if (isnan(dup_threshold))
// 	{
// 	mssErrorf(1, "EXP", "%s(NAN, ...) dup_threshold cannot be NAN.", fn_name);
// 	return -1;
// 	}
//     if (dup_threshold <= 0 || 1 <= dup_threshold)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, ...) dup_threshold must be between 0 and 1 (exclusive).",
// 	    fn_name, dup_threshold
// 	);
// 	return -1;
// 	}
    
//     /** Extract output file path. **/
//     if (maybe_out_file_path->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, NULL, ...) out_file_path cannot be NULL.",
// 	    fn_name, dup_threshold
// 	);
// 	return -1;
// 	}
//     if (maybe_out_file_path->DataType != DATA_T_STRING)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \?\?\?, ...) out_file_path should be a string.",
// 	    fn_name, dup_threshold
// 	);
// 	return -1;
// 	}
//     char* out_file_path = maybe_out_file_path->String;
//     if (out_file_path == NULL)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, nothing?, ...) expected string from out_file_path "
// 	    "(of type DataType = DATA_T_STRING), but the String was NULL "
// 	    "or did not exist!",
// 	    fn_name, dup_threshold
// 	);
// 	return -1;
// 	}
//     size_t out_path_len = strlen(out_file_path);
//     if (out_path_len == 0u)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", ...) out_file_path cannot be an empty string.",
// 	    fn_name, dup_threshold, out_file_path
// 	);
// 	return -1;
// 	}
//     const size_t max_len = BUFSIZ - 48u;
//     if (out_path_len >= max_len)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", ...) out_file_path length (%lu) > max length (%lu).",
// 	    fn_name, dup_threshold, out_file_path, out_path_len, max_len
// 	);
// 	return -1;
// 	}
//     if (strncmp(out_file_path + (out_path_len - 4u), ".csv", 4u) != 0)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", ...) out_file_path must end in .csv, "
// 	    "because the output file is a csv.",
// 	    fn_name, dup_threshold, out_file_path
// 	);
// 	return -1;
// 	}
    
//     /** Extract dataset string. **/
//     if (maybe_data->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", NULL) data cannot be NULL.",
// 	    fn_name, dup_threshold, out_file_path
// 	);
// 	return -1;
// 	}
//     if (maybe_data->DataType != DATA_T_STRING)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", \?\?\?) data must be a string.",
// 	    fn_name, dup_threshold, out_file_path
// 	);
// 	return -1;
// 	}
//     char* data = maybe_data->String;
//     if (data == NULL)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", \?\?\?) expected string from data "
// 	    "(of type DataType = DATA_T_STRING), but the String "
// 	    "was NULL or did not exist!",
// 	    fn_name, dup_threshold, out_file_path
// 	);
// 	return -1;
// 	}
//     if (strlen(data) == 0u)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", \"%s\") data cannot be an empty string.",
// 	    fn_name, dup_threshold, out_file_path, data
// 	);
// 	return -1;
// 	}
    
//     /** Check number of entries in the dataset. **/
//     size_t dataset_size = 1;
//     for (char* buf = data; *buf != '\0'; buf++)
// 	if (*buf == SEPARATOR_CHAR) dataset_size++;
    
//     /** Verify dataset is reasonable size. **/
//     if (dataset_size == 1)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"%s\", \"\?\?\?\") Expected data to contain multiple "
// 	    "values separated by \""SEPARATOR"\", but data was: \"%s\"",
// 	    fn_name, dup_threshold, out_file_path, data
// 	);
// 	return -1;
// 	}
    
//     /** Parse strs out of the data into the dataset. **/
//     size_t count = 0u;
//     char* token = strtok(data, SEPARATOR);
//     char* dataset[dataset_size];
//     memset(dataset, 0, sizeof(dataset));
//     while (token && count < dataset_size)
// 	{
// 	char* new_token = strdup(token);
// 	if (new_token == NULL)
// 	    {
// 	    mssErrorf(1, "EXP",
// 		"%s(%lg, \"%s\", \"...\") Failed to copy token \"%s\" from data.",
// 		fn_name, dup_threshold, out_file_path, token
// 	    );
// 	    goto err_free_dataset;
// 	    }
// 	dataset[count++] = new_token;
// 	token = strtok(NULL, SEPARATOR);
// 	}
    
//     /** Allocate memory to store dups. **/
//     pXArray dups;
    
//     /** Handle phone numbers. **/
//     if (is_phone_numbers)
// 	{
// 	/*** Phone number strings are always 10 characters long. Thus, they
// 	 *** are NOT NULL TERMINATED because we can assume the length.
// 	 ***/
// 	unsigned int num_phone_numbers = 0u;
// 	char phone_numbers[dataset_size][10u];
	
// 	/** Parse the dataset. **/
// 	for (unsigned int i = 0u; i < dataset_size; i++)
// 	    {
// 	    char* maybe_phone_number = dataset[i];
	    
// 	    /** Verify length can be a valid phone number. **/
// 	    const size_t len = strlen(maybe_phone_number);
// 	    if (len < 10u)
// 		{
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") \"Phone number\" (\"%s\") is too short. (skipped)",
// 		    fn_name, dup_threshold, out_file_path, maybe_phone_number
// 		);
// 		continue;
// 		}
// 	    if (len > 18u)
// 		{
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") \"Phone number\" (\"%s\") is too long. (skipped)",
// 		    fn_name, dup_threshold, out_file_path, maybe_phone_number
// 		);
// 		continue;
// 		}
	    
// 	    /** Parse phone number. **/
// 	    char buf[11u], cur_char = maybe_phone_number[0];
// 	    unsigned int j = ((cur_char == '+') ? 2u :
// 	                     ((cur_char == '1') ? 1u : 0u));
// 	    unsigned int number_len = 0u;
// 	    while (cur_char != '\0' && number_len <= 10u)
// 		{
// 		cur_char = maybe_phone_number[j];
		
// 		if (
// 		    cur_char == '-' ||
// 		    cur_char == ' ' ||
// 		    cur_char == '(' ||
// 		    cur_char == ')'
// 		) continue;
// 		else if (!isdigit(cur_char))
// 		    {
// 		    /** Unknown character. **/
// 		    mssErrorf(1, "EXP",
// 			"%s(%lg, \"%s\", \"...\") \"Phone number\" (\"%s\") contains unexpected character '%c'. (skipped)",
// 			fn_name, dup_threshold, out_file_path, maybe_phone_number, cur_char
// 		    );
// 		    goto next_phone_number;
// 		    }
		
// 		/** Add the character to the phone number. */
// 		buf[number_len] = cur_char;
// 		number_len++;
		
// 		/** Advance to next number. **/
// 		j++;
// 		}
	    
// 	    /** Check number of digits. **/
// 	    if (number_len < 10u)
// 		{
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") \"Phone number\" (\"%s\") has less than 10 digits. (skipped)",
// 		    fn_name, dup_threshold, out_file_path, maybe_phone_number
// 		);
// 		continue;
// 		}
// 	    if (number_len > 10u)
// 		{
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") \"Phone number\" (\"%s\") has more than 10 digits. (skipped)",
// 		    fn_name, dup_threshold, out_file_path, maybe_phone_number
// 		);
// 		continue;
// 		}
	    
// 	    /** Copy valid phone number (with no null-terminator). **/
// 	    memcpy(phone_numbers[num_phone_numbers++], buf, 10u);
		    
// 	    next_phone_number:;
// 	    }
	
// 	/** Invoke phone number search to find dups in the processed data. **/
// 	dups = phone_search(phone_numbers, num_phone_numbers, dup_threshold);
// 	}
    
//     /** Handle text. **/
//     else
// 	{
// 	/** Build vectors from the strs in the dataset. **/
// 	const size_t vectors_size = dataset_size * sizeof(int*);
// 	int** vectors = (int**)nmMalloc(vectors_size);
// 	if (vectors == NULL)
// 	    {
// 	    mssErrorf(1, "EXP", 
// 		"%s(%lg, \"%s\", \"...\") - nmMalloc(%lu) failed.",
// 		fn_name, dup_threshold, out_file_path, vectors_size
// 	    );
// 	    goto err_free_dataset;
// 	    }
// 	for (size_t i = 0; i < dataset_size; i++)
// 	    {
// 	    const int* vector = vectors[i] = build_vector(dataset[i]);
// 	    if (vector == NULL)
// 		{
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") - build_vector(%s) failed.",
// 		    fn_name, dup_threshold, out_file_path, dataset[i]
// 		);
// 		goto err_free_vectors;
// 		}
// 	    if (vector[0] == -EXP_NUM_DIMS) {
// 		mssErrorf(1, "EXP",
// 		    "%s(%lg, \"%s\", \"...\") - build_vector(%s) produced no character pairs.",
// 		    fn_name, dup_threshold, out_file_path, dataset[i]
// 		);
// 		goto err_free_vectors;
// 	    }
// 	    }
	
// 	/** Invoke lightning search to find dups using the vectors. **/
// 	dups = lightning_search(vectors, dataset_size, dup_threshold);
// 	if (dups == NULL) {
// 	    mssErrorf(1, "EXP",
// 		"%s(%lg, \"%s\", \"...\") - lightning_search() failed.",
// 		fn_name, dup_threshold, out_file_path
// 	    );
// 	    goto err_free_vectors;
// 	}
	
// 	/** Free unused memory. **/
// 	for (size_t i = 0; i < dataset_size; i++)
// 	    {
// 	    nmSysFree(vectors[i]);
// 	    vectors[i] = NULL;
// 	    }
// 	nmFree(vectors, vectors_size);
// 	vectors = NULL;
// 	goto search_done;
	
// 	/** Free vectors, if needed. **/
// 	err_free_vectors:
// 	if (vectors != NULL)
// 	    {
// 	    for (size_t i = 0; i < dataset_size; i++)
// 		{
// 		if (vectors[i] == NULL) break;
// 		nmSysFree(vectors[i]);
// 		vectors[i] = NULL;
// 		}
// 	    nmFree(vectors, vectors_size);
// 	    vectors = NULL;
// 	    }
// 	goto err_free_dataset;
	
// 	search_done:;
// 	}
    
//     /** Check number of dups found. **/
//     const int num_dups = dups->nItems;
    
//     // Hack where we hardcode the path to the root directory because trying to
//     // track it down is way too hard.
//     const char root_path[] = "/usr/local/src/cx-git/centrallix-os";
    
//     /** Create output file path. **/
//     char out_path[BUFSIZ];
//     snprintf(memset(out_path, 0, sizeof(out_path)), sizeof(out_path), "%s/%s", root_path, out_file_path);
    
//     /** Write output file. **/
//     FILE* file = fopen(out_path, "w");
//     if (file == NULL)
// 	{
// 	perror("Failed to open file.");
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"...\", ...) failed to open file: %s",
// 	    fn_name, dup_threshold, out_path
// 	);
// 	goto err_free_dups;
// 	}
//     const int setvbuf_ret = setvbuf(file, NULL, _IOFBF, (1000 * 1000));
//     if (setvbuf_ret != 0)
// 	{
// 	perror("Failed to set buffering on file.");
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"...\", ...) failed to set buffering on file: %d, %s",
// 	    fn_name, dup_threshold, setvbuf_ret, out_path
// 	);
// 	goto err_close_file;
// 	}
    
//     /** Write CSV header. **/
//     fprintf(file, "id1,id2,sim\n");
    
//     /*** If no data was written, make sure there is at least one row in the
//      *** output file since assuming this file has data makes the sql faster.
//      ***/
//     if (num_dups == 0u)
// 	fprintf(file, "error,undefined,0.0\n");
    
//     /** Write CSV data rows. **/
//     else
// 	{
// 	for (unsigned int i = 0u; i < num_dups; i++)
// 	    {
// 	    Dup* data = (Dup*)dups->Items[i];
// 	    fprintf(file, "%s,%s,%.8lf\n", dataset[data->id1], dataset[data->id2], data->similarity);
// 	    nmFree(data, sizeof(Dup)); /* Free unused data. */
// 	    dups->Items[i] = NULL;
// 	    }
// 	}
    
//     /** Free unused data. **/
//     for (unsigned int i = 0u; i < dataset_size; i++)
// 	{
// 	free(dataset[i]);
// 	dataset[i] = NULL;
// 	}
//     xaDeInit(dups);
//     dups = NULL;
    
//     /** Close file. **/
//     const int fclose_ret = fclose(file);
//     if (fclose_ret != 0)
// 	{
// 	perror("Failed to close file.");
// 	mssErrorf(1, "EXP",
// 	    "%s(%lg, \"...\") failed to close file: %d, %s",
// 	    fn_name, dup_threshold, fclose_ret, out_path
// 	);
// 	goto err_free_dataset;
// 	}
//     file = NULL;
    
//     /** Success. **/
//     tree->DataType = DATA_T_INTEGER;
//     tree->Integer = (int)num_dups;
//     return 0;
    
//     /** Error cases. **/
    
//     /** Close file, if needed. **/
//     err_close_file:
//     if (file != NULL)
// 	{
// 	const int fclose_ret = fclose(file);
// 	if (fclose_ret != 0)
// 	    {
// 	    char dbl_buf[DBL_BUF_SIZE];
// 	    snprintf(dbl_buf, sizeof(dbl_buf), "%lg", dup_threshold);
// 	    perror("Failed to close file.");
// 	    mssErrorf(1, "EXP",
// 		"%s(%s, \"...\") failed to close file: %d, %s",
// 		fn_name, dbl_buf, fclose_ret, out_path
// 	    );
// 	    }
// 	}
    
//     /** Free dups, if needed. **/
//     err_free_dups:
//     if (dups != NULL)
// 	{
// 	for (unsigned int i = 0u; i < num_dups; i++)
// 	    {
// 	    nmFree(dups->Items[i], sizeof(Dup));
// 	    dups->Items[i] = NULL;
// 	    }
// 	xaDeInit(dups);
// 	dups = NULL;
// 	}
    
//     /** Free dataset, if needed. **/
//     err_free_dataset:
//     for (unsigned int i = 0u; i < dataset_size; i++)
// 	{
// 	if (dataset[i] == NULL) break;
// 	free(dataset[i]);
// 	dataset[i] = NULL;
// 	}
    
//     return -1;
//     }

// int exp_fn_get_dups(pExpression tree, pParamObjects objlist, pExpression p1, pExpression p2, pExpression p3)
//     {
//     return exp_fn_get_dups_general(tree, objlist, p1, p2, p3, "get_dups", false);
//     }

// int exp_fn_get_dups_phone(pExpression tree, pParamObjects objlist, pExpression p1, pExpression p2, pExpression p3)
//     {
//     return exp_fn_get_dups_general(tree, objlist, p1, p2, p3, "get_dups_phone", true);
//     }

// /** Magic values. **/
// #define EXP_NUM_FIELDS 7
// #define EXP_INDEX_FIRST_NAME 0
// #define EXP_INDEX_FIRST_NAME_METAPHONE 1
// #define EXP_INDEX_LAST_NAME 2
// #define EXP_INDEX_LAST_NAME_METAPHONE 3
// #define EXP_INDEX_EMAIL 4
// #define EXP_INDEX_PHONE 5
// #define EXP_INDEX_ADDRESS 6

// /** No-op function. **/
// int exp_fn_do_nothing() { return 0; }

// /*** Function to add parameters to private storage so that more than 3 parameters can be passed. 
//  *** Currently, doubles are the only supported param type.
//  *** 
//  *** Usage: param(<array : NULL | R>, <param_name : string>, <param_value : V> ) : R,
//  *** where: V : Double
//  *** 
//  *** @param tree Return param_value.
//  *** @param objlist Function scope.
//  *** @param maybe_array The 1st param, should be NULL or another call to param().
//  *** @param maybe_param_name The 2nd param, should be a string for the name of the param.
//  *** @param maybe_param_value The 3rd param, should be the param_value of the param being set.
//  ***/
// int exp_fn_param(pExpression tree, pParamObjects objlist, pExpression maybe_param_name, pExpression maybe_param_value, pExpression maybe_array) {
//     // Verify arg number.
//     if (!maybe_param_name || !maybe_param_value)
// 	{
// 	mssErrorf(1, "EXP", "param(?) expects two or three parameters.");
// 	return -1;
// 	}
    
//     // Magic checks.
//     ASSERTMAGIC(tree, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_param_name, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_param_value, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_array, MGK_EXPRESSION);
    
//     // Check object list.
//     if (!objlist)
// 	{
// 	mssErrorf(1, "EXP", "param(\?\?\?) no object list?");
// 	return -1;
// 	}
//     ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
	
//      // Extract param name.
//     if (maybe_param_name->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP", "param(NULL, ...) param_name cannot be null.");
// 	return -1;
// 	}
//     if (maybe_param_name->DataType != DATA_T_STRING)
// 	{
// 	mssErrorf(1, "EXP", "param(?, ...) param_name must be a string.");
// 	return -1;
// 	}
//     const char* param_name = maybe_param_name->String;
    
//     // Extract param value.
//     if (maybe_param_value->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP", "param(\"%s\", NULL, ...) param_value cannot be null.", param_name);
// 	return -1;
// 	}
//     if (maybe_param_value->DataType != DATA_T_DOUBLE)
// 	{
// 	mssErrorf(1, "EXP", "param(\"%s\", ?, ...) param_value must be a doube.", param_name);
// 	return -1;
// 	}
//     double param_value = maybe_param_value->Types.Double;
    
//     // Verify the value being set.
//     // TODO: Replace with hashmap.
//     signed int index = -1;
//     if (strcmp(param_name, "first_name") == 0) index = EXP_INDEX_FIRST_NAME;
//     else if (strcmp(param_name, "first_name_metaphone") == 0) index = EXP_INDEX_FIRST_NAME_METAPHONE;
//     else if (strcmp(param_name, "last_name") == 0) index = EXP_INDEX_LAST_NAME;
//     else if (strcmp(param_name, "last_name_metaphone") == 0) index = EXP_INDEX_LAST_NAME_METAPHONE;
//     else if (strcmp(param_name, "email") == 0) index = EXP_INDEX_EMAIL;
//     else if (strcmp(param_name, "phone") == 0) index = EXP_INDEX_PHONE;
//     else if (strcmp(param_name, "address") == 0) index = EXP_INDEX_ADDRESS;
//     if (index == -1)
// 	{
// 	mssErrorf(1, "EXP",
// 	    "param(\"%s\", %lf, ...) invalid field name %s.",
// 	    param_name, param_value, param_name
// 	);
// 	return -1;
// 	}
    
//     // Extract array.
//     double* array;
//     if (!maybe_array || maybe_array->Flags & EXPR_F_NULL)
// 	{
// 	const size_t size = EXP_NUM_FIELDS * sizeof(double);
// 	void* PrivateData = tree->PrivateData = memset(nmSysMalloc(size), 0, size);
// 	tree->PrivateDataFinalize = exp_fn_do_nothing; // DON'T FREE MY DATA UNTIL I'M READY.
	
// 	array = (double*)PrivateData;
// 	for (unsigned int i = 0u; i < EXP_NUM_FIELDS; i++) array[i] = NAN;
// 	}
//     else if (
// 	maybe_array->DataType == DATA_T_ARRAY &&
// 	maybe_array->PrivateData != NULL &&
// 	!strcmp(maybe_array->Name, "param")
//     )
// 	{
// 	tree->PrivateData = maybe_array->PrivateData;
// 	tree->PrivateDataFinalize = exp_fn_do_nothing; // DON'T FREE MY DATA UNTIL I'M READY.
// 	array = (double*)maybe_array->PrivateData;
// 	}
//     else
// 	{
// 	mssErrorf(1, "EXP", "param(\"%s\", %lf, ...) if provided, array must be from a call to param().", param_name, param_value);
// 	return -1;
// 	}
    
//     // Warn on previous data.
//     double old_value = array[index];
//     if (!isnan(old_value))
// 	{
// 	fprintf(stderr,
// 	    "Warning: Overwriting field '%s'(@ index %d) with %lf (was %lf).\n",
// 	    param_name, index, param_value, old_value
// 	);
//         }
    
//     // Set param_value.
//     array[index] = param_value;
    
//     // Done
//     tree->DataType = DATA_T_ARRAY;
//     tree->Integer = 0;
//     tree->Types.Double = 0.0;
//     return 0;
//     }

// int exp_fn_get_sim(pExpression tree, pParamObjects objlist, pExpression maybe_fields, pExpression unused1, pExpression unused2)
//     {
//     if (!maybe_fields || unused1 || unused2)
// 	{
// 	mssErrorf(1, "EXP", "get_sim(param(...)) expects one parameter, from param().");
// 	return -1;
// 	}
	
//     // Magic checks.
//     ASSERTMAGIC(tree, MGK_EXPRESSION);
//     ASSERTMAGIC(maybe_fields, MGK_EXPRESSION);
    
//     // Check object list.
//     if (!objlist)
// 	{
// 	mssErrorf(1, "EXP", "get_sim(\?\?\?) no object list?");
// 	return -1;
// 	}
//     ASSERTMAGIC(objlist->Session, MGK_OBJSESSION);
    
//     // Verify arg.
//     if (maybe_fields->Flags & EXPR_F_NULL)
// 	{
// 	mssErrorf(1, "EXP", "get_sim(NULL) fields from param() cannot be NULL.");
// 	return -1;
// 	}
//     if (maybe_fields->DataType != DATA_T_ARRAY || maybe_fields->PrivateData == NULL)
// 	{
// 	mssErrorf(1, "EXP", "get_sim(\?\?\?) expects arg 0 to be fields from a call to param().");
// 	return -1;
// 	}
    
//     // Extract arg(s?).
//     double* fields = (double*)maybe_fields->PrivateData;
    
//     const double first_name = fields[EXP_INDEX_FIRST_NAME];
//     if (isnan(first_name))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) first_name similarity not set.");
// 	return -1;
// 	}
    
//     const double first_name_metaphone = fields[EXP_INDEX_FIRST_NAME_METAPHONE];
//     if (isnan(first_name_metaphone))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) first_name_metaphone similarity not set.");
// 	return -1;
// 	}
    
//     const double last_name = fields[EXP_INDEX_LAST_NAME];
//     if (isnan(last_name))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) last_name similarity not set.");
// 	return -1;
// 	}
    
//     const double last_name_metaphone = fields[EXP_INDEX_LAST_NAME_METAPHONE];
//     if (isnan(last_name_metaphone))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) last_name_metaphone similarity not set.");
// 	return -1;
// 	}
    
//     const double email = fields[EXP_INDEX_EMAIL];
//     if (isnan(email))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) email similarity not set.");
// 	return -1;
// 	}
    
//     const double phone = fields[EXP_INDEX_PHONE];
//     if (isnan(phone))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) phone similarity not set.");
// 	return -1;
// 	}
    
//     const double address = fields[EXP_INDEX_ADDRESS];
//     if (isnan(address))
// 	{
// 	mssErrorf(1, "EXP", "get_sim(...) address similarity not set.");
// 	return -1;
// 	}
	
//     char* primary;
//     char* secondary;
//     meta_double_metaphone("text", &primary, &secondary);
//     printf("Primary: %s, secondary: %s\n", primary, secondary);
    
//     // Print args.
//     printf(
// 	"Sims:\n"
// 	    "\tfirst_name: %lf\n"
// 	    "\tfirst_name_metaphone: %lf\n"
// 	    "\tlast_name: %lf\n"
// 	    "\tlast_name_metaphone: %lf\n"
// 	    "\temail: %lf\n"
// 	    "\tphone: %lf\n"
// 	    "\taddress: %lf\n",
// 	first_name,
// 	first_name_metaphone,
// 	last_name,
// 	last_name_metaphone,
// 	email,
// 	phone,
// 	address
// 	);
    
//     // Compute total.
//     const double first_name_total = max(first_name * 1.0, first_name_metaphone * 0.9);
//     const double last_name_total = max(last_name * 1.0, last_name_metaphone * 0.9);
//     double total = (first_name_total * last_name_total) * 0.6 + email * 0.2 + address * 0.2;
    
//     // Clean up.
//     nmSysFree(fields);
    
//     // Return total.
//     tree->DataType = DATA_T_DOUBLE;
//     tree->Types.Double = total;
//     return 0;
//     }


int exp_fn_double_metaphone(pExpression tree, pParamObjects objlist, pExpression maybe_str, pExpression u1, pExpression u2)
    {
    const char fn_name[] = "double_metaphone";
    
    /** Check number of arguments. **/
    if (!maybe_str || u1 || u2)
	{
	mssErrorf(1, "EXP", "%s(?) expects 1 parameter.", fn_name);
	return -1;
	}
    const int num_params = tree->Children.nItems;
    if (num_params != 1)
	{
	mssErrorf(1, "EXP", "%s(?) expects 1 parameter, got %d.", fn_name, num_params);
	return -1;
	}
    
    /** Magic checks. **/
    ASSERTMAGIC(tree, MGK_EXPRESSION);
    ASSERTMAGIC(maybe_str, MGK_EXPRESSION);
    
    /** Check object list. **/
    if (!objlist)
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
	    "%s(nothing?) expected string from str "
	    "(of type DataType = DATA_T_STRING), but the String "
	    "was NULL or did not exist!",
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
    char* primary;
    char* secondary;
    meta_double_metaphone(
	str,
	memset(&primary, 0, sizeof(primary)),
	memset(&secondary, 0, sizeof(secondary))
    );
    
    /** Process result. **/
    const size_t primary_length = strlen(primary);
    const size_t secondary_length = strlen(secondary);
    char* result = nmSysMalloc(primary_length + 1u + secondary_length + 1u);
    sprintf(result, "%s%c%s", primary, CA_BOUNDARY_CHAR, secondary);
    
    /** Return the result. **/
    tree->String = result;
    tree->DataType = DATA_T_STRING;
    return 0;
    }

// // Clean up.
// #undef min
// #undef max

// // END OF DUPE SECTION
// // ===================

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
	xhAdd(&EXP.Functions, "mixed", (char*)exp_fn_mixed);
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
	xhAdd(&EXP.Functions, "replace", (char*)exp_fn_replace);
	xhAdd(&EXP.Functions, "escape", (char*)exp_fn_escape);
	xhAdd(&EXP.Functions, "quote", (char*)exp_fn_quote);
	xhAdd(&EXP.Functions, "substitute", (char*)exp_fn_substitute);
	xhAdd(&EXP.Functions, "eval", (char*)exp_fn_eval);
	xhAdd(&EXP.Functions, "round", (char*)exp_fn_round);
	xhAdd(&EXP.Functions, "dateadd", (char*)exp_fn_dateadd);
	xhAdd(&EXP.Functions, "datediff", (char*)exp_fn_datediff);
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
	xhAdd(&EXP.Functions, "has_endorsement", (char*)exp_fn_has_endorsement);
	xhAdd(&EXP.Functions, "rand", (char*)exp_fn_rand);
	xhAdd(&EXP.Functions, "nullif", (char*)exp_fn_nullif);
	xhAdd(&EXP.Functions, "dateformat", (char*)exp_fn_dateformat);
	xhAdd(&EXP.Functions, "moneyformat", (char*)exp_fn_moneyformat);
	xhAdd(&EXP.Functions, "hash", (char*)exp_fn_hash);
	xhAdd(&EXP.Functions, "hmac", (char*)exp_fn_hmac);
	xhAdd(&EXP.Functions, "log10", (char*)exp_fn_log10);
	xhAdd(&EXP.Functions, "power", (char*)exp_fn_power);
	xhAdd(&EXP.Functions, "pbkdf2", (char*)exp_fn_pbkdf2);
	xhAdd(&EXP.Functions, "levenshtein", (char*)exp_fn_levenshtein); /* Only used in its own tests. */
	xhAdd(&EXP.Functions, "lev_compare", (char*)exp_fn_lev_compare); /* Only used in its own tests. */
	xhAdd(&EXP.Functions, "cos_compare", (char*)exp_fn_cos_compare);
	xhAdd(&EXP.Functions, "to_base64", (char*)exp_fn_to_base64);
	xhAdd(&EXP.Functions, "from_base64", (char*)exp_fn_from_base64);
	xhAdd(&EXP.Functions, "to_hex", (char*)exp_fn_to_hex);
	xhAdd(&EXP.Functions, "from_hex", (char*)exp_fn_from_hex);
	xhAdd(&EXP.Functions, "octet_length", (char*)exp_fn_octet_length);
	xhAdd(&EXP.Functions, "argon2id",(char*)exp_fn_argon2id);
	
	/** Duplicate Detection **/
	// xhAdd(&EXP.Functions, "get_dups", (char*)exp_fn_get_dups);
	// xhAdd(&EXP.Functions, "get_dups_phone", (char*)exp_fn_get_dups_phone);
	// xhAdd(&EXP.Functions, "no_op", (char*)exp_fn_do_nothing);
	// xhAdd(&EXP.Functions, "do_nothing", (char*)exp_fn_do_nothing);
	// xhAdd(&EXP.Functions, "param", (char*)exp_fn_param);
	// xhAdd(&EXP.Functions, "total_sim", (char*)exp_fn_get_sim);
	xhAdd(&EXP.Functions, "double_metaphone", (char*)exp_fn_double_metaphone);
	
	/** Windowing **/
	xhAdd(&EXP.Functions, "row_number", (char*)exp_fn_row_number);
	xhAdd(&EXP.Functions, "dense_rank", (char*)exp_fn_dense_rank);
	xhAdd(&EXP.Functions, "lag", (char*)exp_fn_lag);

	/** Aggregate **/
	xhAdd(&EXP.Functions, "count", (char*)exp_fn_count);
	xhAdd(&EXP.Functions, "avg", (char*)exp_fn_avg);
	xhAdd(&EXP.Functions, "sum", (char*)exp_fn_sum);
	xhAdd(&EXP.Functions, "max", (char*)exp_fn_max);
	xhAdd(&EXP.Functions, "min", (char*)exp_fn_min);
	xhAdd(&EXP.Functions, "first", (char*)exp_fn_first);
	xhAdd(&EXP.Functions, "last", (char*)exp_fn_last);
	xhAdd(&EXP.Functions, "nth", (char*)exp_fn_nth);

	/** Reverse functions **/
	xhAdd(&EXP.ReverseFunctions, "isnull", (char*)exp_fn_reverse_isnull);

    return 0;
    }
