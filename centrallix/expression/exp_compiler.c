#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"

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
/* Module: 	expression.h, exp_compiler.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*									*/
/*		--> exp_compiler.c: compiles expressions from an MLX	*/
/*		    token stream into an expression tree.		*/
/************************************************************************/



/*** exp_internal_CompileExpression_r - builds an expression tree from a SQL
 *** where-clause expression.  This function is the recursive element for
 *** parenthesized groupings.
 ***/
pExpression
exp_internal_CompileExpression_r(pLxSession lxs, int level, pParamObjects objlist, int cmpflags)
    {
    pExpression etmp,etmp2,eptr,expr = NULL;
    int t,err=0,i;
    int was_unary = 0;
    int was_prefix_unary = 0;
    int new_cmpflags;
    char* sptr;
    int alloc;
    pXString subqy;
    int idx;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"EXP","Failed to compile expression: resource exhaustion occurred");
	    return NULL;
	    }

	/** This is to suppress a rather unintelligent compiler warning about
	 ** eptr being used uninitialized.
	 **/
	eptr = NULL;

	/** Loop through the tokens **/
	while (!err)
	    {
	    /** Get an immediate/identifier token **/
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_ERROR) break;

	    /** If last operator was postfix unary (i.e., "x IS NULL") **/
	    if (was_unary) goto SKIP_OPERAND;

	    /** If open paren, recursively handle this. **/
	    if (t == MLX_TOK_OPENPAREN)
		{
		etmp = exp_internal_CompileExpression_r(lxs,level+1,objlist, cmpflags | EXPR_CMP_WATCHLIST);
		if (!etmp)
		    {
                    if (expr) expFreeExpression(expr);
		    expr = NULL;
		    err=1;
		    break;
		    }
		}
	    else if (t == MLX_TOK_CLOSEPAREN)
	        {
		if (expr == NULL && (cmpflags & EXPR_CMP_WATCHLIST))
		    {
		    /** Empty list. () **/
		    expr = expAllocExpression();
		    expr->NodeType = EXPR_N_LIST;
		    if (cmpflags & EXPR_CMP_RUNSERVER) expr->Flags |= EXPR_F_RUNSERVER;
		    if (cmpflags & EXPR_CMP_RUNCLIENT) expr->Flags |= EXPR_F_RUNCLIENT;
		    break;
		    }
		else
		    {
		    /** Error. **/
		    if (expr) expFreeExpression(expr);
		    expr = NULL;
		    mssError(1,"EXP","Unexpected close-paren ')' in expression");
		    err = 1;
		    break;
		    }
		}
	    else
		{
	        etmp = expAllocExpression();
		if (cmpflags & EXPR_CMP_RUNSERVER) etmp->Flags |= EXPR_F_RUNSERVER;
		if (cmpflags & EXPR_CMP_RUNCLIENT) etmp->Flags |= EXPR_F_RUNCLIENT;
	        switch(t)
                    {
		    /*case MLX_TOK_MINUS:
			break;*/

                    case MLX_TOK_INTEGER:
                        etmp->NodeType = EXPR_N_INTEGER;
                        etmp->Integer = mlxIntVal(lxs);
			etmp->DataType = DATA_T_INTEGER;
                        break;

		    case MLX_TOK_DOLLAR:
		        t = mlxNextToken(lxs);
			if (t != MLX_TOK_INTEGER && t != MLX_TOK_DOUBLE)
			    {
			    expFreeExpression(etmp);
			    etmp = NULL;
			    if (expr) expFreeExpression(expr);
			    expr = NULL;
			    mssError(1,"EXP","Expected numeric value after '$', got '%s'", mlxStringVal(lxs, NULL));
			    err = 1;
			    break;
			    }
			etmp->NodeType = EXPR_N_MONEY;
			etmp->DataType = DATA_T_MONEY;
			if (t == MLX_TOK_INTEGER)
			    {
			    i = mlxIntVal(lxs);
			    etmp->Types.Money.WholePart = i;
			    etmp->Types.Money.FractionPart = 0;
			    }
			else
			    {
			    sptr = mlxStringVal(lxs,NULL);
			    objDataToMoney(DATA_T_STRING, sptr, &(etmp->Types.Money));
			    }
			break;

		    case MLX_TOK_DOUBLE:
		        etmp->NodeType = EXPR_N_DOUBLE;
			etmp->Types.Double = mlxDoubleVal(lxs);
			etmp->DataType = DATA_T_DOUBLE;
			break;
    		    
		    case MLX_TOK_KEYWORD:
                    case MLX_TOK_STRING:
                        etmp->NodeType = EXPR_N_STRING;
                        etmp->Alloc = 1;
                        etmp->String = mlxStringVal(lxs,&(etmp->Alloc));
			etmp->DataType = DATA_T_STRING;

			/** Check for a function() **/
			if (t == MLX_TOK_KEYWORD && !strcasecmp(etmp->String, "select") && !expr && (cmpflags & EXPR_CMP_WATCHLIST))
			    {
			    /** subquery **/
			    subqy = (pXString)nmMalloc(sizeof(XString));
			    if (!subqy)
				{ err = 1; break; }
			    xsInit(subqy);
			    xsCopy(subqy, "select ", 7);
			    i = 1;
			    etmp->NodeType = EXPR_N_SUBQUERY;
			    while(1)
				{
				t = mlxNextToken(lxs);
				if (t == MLX_TOK_ERROR)
				    { err=1; break; }
				if (t == MLX_TOK_EOF)
				    {
				    mssError(1, "EXP", "Unexpected end-of-expression");
				    err = 1; 
				    break;
				    }
				if (t == MLX_TOK_OPENPAREN) 
				    i++;
				if (t == MLX_TOK_CLOSEPAREN)
				    {
				    i--;
				    if (!i)
					{
					/** success - found end of subquery **/
					mlxHoldToken(lxs);
					break;
					}
				    }
				alloc=0;
				sptr = mlxStringVal(lxs, &alloc);
				if (!sptr)
				    { err=1; break; }
				if (t == MLX_TOK_STRING)
				    {
				    if (xsConcatQPrintf(subqy, " %STR&DQUOT", sptr) < 0)
					{ err=1; break; }
				    }
				else
				    {
				    if (xsConcatQPrintf(subqy, " %STR", sptr) < 0)
					{ err=1; break; }
				    }
				if (alloc) nmSysFree(sptr);
				}
			    if (!err)
				{
				etmp->Name = nmSysStrdup(subqy->String);
				etmp->NameAlloc = 1;
				/*etmp->ObjCoverageMask = EXPR_MASK_EXTREF;*/
				etmp->ObjCoverageMask = EXPR_MASK_INDETERMINATE;
				etmp->ObjID = -1;
				}
			    xsDeInit(subqy);
			    nmFree(subqy, sizeof(XString));
			    }
			else if (t==MLX_TOK_KEYWORD)
			    {
			    t = mlxNextToken(lxs);
			    if (!strcasecmp(etmp->String, "not"))
			        {
				mlxHoldToken(lxs);
				was_prefix_unary = 1;
				etmp->NameAlloc = etmp->Alloc;
				etmp->Name = etmp->String;
				etmp->Alloc = 0;
				etmp->String = NULL;
				etmp->NodeType = EXPR_N_NOT;
				}
			    else if (t == MLX_TOK_OPENPAREN)
			        {
				etmp->NameAlloc = etmp->Alloc;
				etmp->Name = etmp->String;
				etmp->Alloc = 0;
				etmp->String = NULL;
				etmp->NodeType = EXPR_N_FUNCTION;
				if (!strcasecmp(etmp->Name,"count") || !strcasecmp(etmp->Name,"avg") ||
				    !strcasecmp(etmp->Name,"max") || !strcasecmp(etmp->Name,"min") ||
				    !strcasecmp(etmp->Name,"sum") || !strcasecmp(etmp->Name,"first") ||
				    !strcasecmp(etmp->Name,"last") || !strcasecmp(etmp->Name,"nth"))
				    {
				    etmp->Flags |= (EXPR_F_AGGREGATEFN | EXPR_F_AGGLOCKED);
				    /*etmp->AggExp = expAllocExpression();*/
				    }

				/** Pseudo-functions declaring domain of exec of the expression **/
				if (!strcasecmp(etmp->Name,"runserver"))
				    etmp->Flags |= EXPR_F_RUNSERVER;
				else if (!strcasecmp(etmp->Name,"runclient"))
				    etmp->Flags |= EXPR_F_RUNCLIENT;
				else if (!strcasecmp(etmp->Name,"runstatic"))
				    etmp->Flags |= EXPR_F_RUNSTATIC;

				/** Ok, parse the elements in the param list until we get a close-paren **/
				while(lxs->TokType != MLX_TOK_CLOSEPAREN)
				    {
				    /** We preview the next token to see if the param list is empty. **/
				    mlxNextToken(lxs);
				    if (lxs->TokType == MLX_TOK_CLOSEPAREN) break;
				    mlxHoldToken(lxs);

				    /** Compile the param and add it as a child item **/
				    new_cmpflags = cmpflags & ~EXPR_CMP_WATCHLIST;
				    if (etmp->Flags & EXPR_F_RUNSERVER)
					new_cmpflags |= (EXPR_CMP_LATEBIND | EXPR_CMP_RUNSERVER);
				    if (etmp->Flags & EXPR_F_RUNCLIENT)
					new_cmpflags |= (EXPR_CMP_LATEBIND | EXPR_CMP_RUNCLIENT);
				    etmp2 = exp_internal_CompileExpression_r(lxs,level+1,objlist,new_cmpflags);
				    if (!etmp2)
				        {
					if (expr) expFreeExpression(expr);
					if (etmp) expFreeExpression(etmp);
					expr = NULL; 
					err=1;
					break;
					}
				    etmp2->Parent = etmp;
				    xaAddItem(&etmp->Children, (void*)etmp2);
				    
				    /** If comma, un-hold the token **/
				    if (lxs->TokType == MLX_TOK_COMMA) mlxNextToken(lxs);
				    }

				/** If this was a domain declaration, remove the function entirely
				 ** from the expression tree since it doesn't really exist
				 **/
				if (!strcasecmp(etmp->Name,"runserver") || !strcasecmp(etmp->Name,"runclient") || !strcasecmp(etmp->Name,"runstatic"))
				    {
				    if (etmp->Children.nItems != 1)
					{
					mssError(0,"EXP","%s() takes exactly one argument, %d supplied", etmp->Name, etmp->Children.nItems);
					err = 1;
					}
				    else
					{
					etmp2 = (pExpression)(etmp->Children.Items[0]);
					etmp2->Parent = etmp->Parent;
					xaRemoveItem(&(etmp->Children), 0);
					expFreeExpression(etmp);
					etmp = etmp2;
					}
				    }
				}
			    else
			        {
				if (!strcasecmp(etmp->String,"NULL")) etmp->Flags |= (EXPR_F_NULL | EXPR_F_PERMNULL);
				mlxHoldToken(lxs);
				}
			    }
                        break;


		    case MLX_TOK_FILENAME:
		        /** Referencing an attribute of a particular object? **/
			etmp->NodeType = EXPR_N_OBJECT;
			etmp->NameAlloc = 1;
			etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));
			etmp->ObjID = -1;
			etmp->ObjCoverageMask = EXPR_MASK_EXTREF;
			if (mlxNextToken(lxs) != MLX_TOK_COLON || ((t = mlxNextToken(lxs)) != MLX_TOK_KEYWORD && t != MLX_TOK_STRING))
			    {
                            expFreeExpression(etmp);
                            if (expr) expFreeExpression(expr);
                            expr = NULL;
			    mssError(1,"EXP","Expected :keyword reference after filename, got '%s'", mlxStringVal(lxs, NULL));
                            err=1;
                            break;
			    }
			etmp2 = expAllocExpression();
			etmp2->NodeType = EXPR_N_PROPERTY;
			etmp2->NameAlloc = 1;
			etmp2->Name = mlxStringVal(lxs,&(etmp2->NameAlloc));
			etmp2->Parent = etmp;
			etmp2->ObjID = -1;
			etmp2->ObjCoverageMask = EXPR_MASK_EXTREF;
			if (cmpflags & EXPR_CMP_RUNSERVER) etmp2->Flags |= EXPR_F_RUNSERVER;
			if (cmpflags & EXPR_CMP_RUNCLIENT) etmp2->Flags |= EXPR_F_RUNCLIENT;
			xaAddItem(&etmp->Children, (void*)etmp2);
			break;
    
                    case MLX_TOK_COLON:
		        /** Is this a cur reference (:) or parent reference (::)? **/
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_COLON)
			    {
			    /*etmp->ObjID = objlist->ParentID;*/
			    etmp->ObjID = EXPR_OBJID_PARENT;
			    t = mlxNextToken(lxs);
			    }
			else
			    {
			    /*etmp->ObjID = objlist->CurrentID;*/
			    etmp->ObjID = EXPR_OBJID_CURRENT;
			    }

			/** Ok, verify next element is the name of the attribute **/
			if (t != MLX_TOK_KEYWORD && t != MLX_TOK_FILENAME && t != MLX_TOK_STRING)
			    {
                            expFreeExpression(etmp);
                            if (expr) expFreeExpression(expr);
                            expr = NULL;
                            err=1;
			    mssError(1,"EXP","Expected source or explicit filename after :, got '%s'", mlxStringVal(lxs, NULL));
                            break;
			    }

			/** Setup the expression tree nodes. **/
			etmp->NodeType = EXPR_N_PROPERTY;
			etmp->NameAlloc = 1;
			etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));

			/** Is this an explicit named reference (:objname:attrname)? **/
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_COLON)
			    {
			    t = mlxNextToken(lxs);
			    if (objlist)
				{
				for(i=0;i<objlist->nObjects;i++) 
				    {
				    if (cmpflags & EXPR_CMP_REVERSE)
					idx = (objlist->nObjects-1) - i;
				    else
					idx = i;
				    if (objlist->Names[idx] && !strcmp(etmp->Name,objlist->Names[idx]))
					{
					if (etmp->NameAlloc)
					    {
					    nmSysFree(etmp->Name);
					    etmp->NameAlloc = 0;
					    }
					etmp->Name = NULL;
					etmp->ObjID = idx;
					break;
					}
				    }
				}
			    if (etmp->Name != NULL) /* didn't find?? */
			        {
				/** Late binding allowed? **/
				if (cmpflags & EXPR_CMP_LATEBIND)
				    {
				    etmp->ObjID = -1;
				    etmp2 = expAllocExpression();
				    xaAddItem(&(etmp->Children),(void*)etmp2);
				    etmp2->ObjID = etmp->ObjID;
				    etmp2->Parent = etmp;
				    etmp->NodeType = EXPR_N_OBJECT;
				    etmp2->NodeType = EXPR_N_PROPERTY;
				    etmp2->NameAlloc = 1;
				    etmp2->Name = mlxStringVal(lxs,&(etmp2->NameAlloc));
				    etmp2->Flags |= (etmp->Flags & EXPR_F_DOMAINMASK);
				    if (!(etmp2->Flags & EXPR_F_DOMAINMASK)) etmp2->Flags |= EXPR_F_RUNSERVER;
				    }
				else
				    {
				    /** No late binding... cause an error. **/
				    mssError(1,"EXP","Could not locate :objectname '%s' in object list", etmp->Name);
				    expFreeExpression(etmp);
				    if (expr) expFreeExpression(expr);
				    expr = NULL;
				    err=1;
				    break;
				    }
				}
			    else
				{
				etmp->NameAlloc = 1;
				etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));
				}
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    }

			i = -1;
			if (objlist) i = expObjID(etmp,objlist);
			if (i>=0)
			    {
			    objlist->Flags[i] |= EXPR_O_REFERENCED;
			    etmp->ObjCoverageMask |= (1<<(i));
			    }
			else
			    {
			    /** FIXME: we should have more intelligent handling of stuff not known at compile time **/
			    etmp->ObjCoverageMask = 0xFFFFFFFF;
			    }
			if (etmp->Children.nItems == 1)
			    {
			    etmp2 = (pExpression)(etmp->Children.Items[0]);
			    etmp2->ObjCoverageMask = etmp->ObjCoverageMask;
			    }
                        break;
    
                    default:
                        expFreeExpression(etmp);
                        if (expr) expFreeExpression(expr);
                        expr = NULL;
                        err=1;
			mssError(1,"EXP","Unexpected '%s' in expression", mlxStringVal(lxs, NULL));
                        break;
                    }
	        if (err) break;
	        etmp->Flags &= ~EXPR_F_OPERATOR;
		}

	    /** Now figure out what to do with the non-op token **/
	    if (expr == NULL)
		{
		expr = etmp;
		eptr = expr;
		}
	    else
		{
		xaAddItem(&(eptr->Children),(void*)etmp);
		etmp->Parent = eptr;
		eptr = etmp;
		}

	    /** Was this thing a prefix-unary operator? **/
	    if (was_prefix_unary) 
	        {
		was_prefix_unary = 0;
		continue;
		}

	    /** Now get the operator token or end-of-stream **/
	    t = mlxNextToken(lxs);

	SKIP_OPERAND:
	    was_unary = 0;

	    /** Got a comma and we're watching for a comma-separated list within these ()? **/
	    if (t == MLX_TOK_COMMA && (cmpflags & EXPR_CMP_WATCHLIST))
	        {
		/** Ok, add a LIST node for the top level if not already **/
		if (expr->NodeType != EXPR_N_LIST)
		    {
	    	    etmp = expAllocExpression();
		    if (cmpflags & EXPR_CMP_RUNSERVER) etmp->Flags |= EXPR_F_RUNSERVER;
		    if (cmpflags & EXPR_CMP_RUNCLIENT) etmp->Flags |= EXPR_F_RUNCLIENT;
		    etmp->NodeType = EXPR_N_LIST;
		    expr->Parent = etmp;
		    xaAddItem(&(etmp->Children),(void*)expr);
		    expr = etmp;
		    eptr = expr;
		    }
		else
		    {
		    eptr = expr;
		    }
		continue;
		}

	    /** Otherwise, comma or EOF specifies end-of-expression. **/
	    if (t == MLX_TOK_EOF || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) 
	        {
		if (t == MLX_TOK_EOF && level > 0)
		    {
		    mssError(1, "EXP", "Unexpected end-of-expression");
		    err = 1;
		    }
		mlxHoldToken(lxs);
		break;
		}

	    /** Close-paren -- exiting out of a subexpression? **/
	    if (level > 0 && t == MLX_TOK_CLOSEPAREN) break;

	    sptr = NULL;
	    if (t == MLX_TOK_KEYWORD && (cmpflags & EXPR_CMP_ASCDESC))
	        {
		sptr = mlxStringVal(lxs,NULL);
		if (!strcasecmp(sptr,"desc"))
		    {
		    expr->Flags |= EXPR_F_DESC;
		    break;
		    }
		else if (!strcasecmp(sptr,"asc"))
		    {
		    break;
		    }
		}
	    if (t==MLX_TOK_ERROR) 
		{
		/*if (etmp) expFreeExpression(etmp);*/
		if (expr) expFreeExpression(expr);
		expr = NULL;
		err = 1;
		mssError(0,"EXP","Error reading expression token stream");
		mlxNoteError(lxs);
		break;
		}
	    etmp = expAllocExpression();
	    if (cmpflags & EXPR_CMP_RUNSERVER) etmp->Flags |= EXPR_F_RUNSERVER;
	    if (cmpflags & EXPR_CMP_RUNCLIENT) etmp->Flags |= EXPR_F_RUNCLIENT;
	    switch(t)
		{
		case MLX_TOK_KEYWORD:
		    if (!sptr) sptr = mlxStringVal(lxs,NULL);
		    if (!strcasecmp(sptr,"and")) etmp->NodeType = EXPR_N_AND;
		    else if (!strcasecmp(sptr,"or")) etmp->NodeType = EXPR_N_OR;
		    else if (!strcasecmp(sptr,"in")) etmp->NodeType = EXPR_N_IN;
		    else if (!strcasecmp(sptr,"like")) etmp->NodeType = EXPR_N_LIKE;
		    else if (!strcasecmp(sptr,"contains")) etmp->NodeType = EXPR_N_CONTAINS;
		    else if (!strcasecmp(sptr,"is"))
		        {
			t = mlxNextToken(lxs);
			if (t != MLX_TOK_KEYWORD)
			    {
		            expFreeExpression(etmp);
		            if (expr) expFreeExpression(expr);
			    err=1;
			    mssError(1,"EXP","Expected NULL or NOT NULL after IS");
			    mlxNoteError(lxs);
			    expr = NULL;
			    }
			sptr = mlxStringVal(lxs,NULL);
			if (!strcasecmp(sptr,"null"))
			    {
			    etmp->NodeType = EXPR_N_ISNULL;
			    was_unary = 1;
			    }
			else if (!strcasecmp(sptr,"not"))
			    {
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_KEYWORD || (sptr = mlxStringVal(lxs,NULL)) == NULL || strcasecmp(sptr,"null") != 0)
				{
				mssError(1,"EXP","Expected NULL after IS NOT");
				mlxNoteError(lxs);
				expFreeExpression(etmp);
				if (expr) expFreeExpression(expr);
				err=1;
				expr = NULL;
				}
			    else
				{
				etmp->NodeType = EXPR_N_ISNOTNULL;
				was_unary = 1;
				}
			    }
			else
			    {
			    mssError(1,"EXP","Expected NULL or NOT NULL after IS");
			    mlxNoteError(lxs);
		            expFreeExpression(etmp);
		            if (expr) expFreeExpression(expr);
			    err=1;
			    expr = NULL;
			    }
			}
		    else 
			{
		        expFreeExpression(etmp);
		        if (expr) expFreeExpression(expr);
			mssError(1,"EXP","Valid keywords for operators: AND/OR/LIKE/CONTAINS/IS");
			mlxNoteError(lxs);
			err=1;
			expr = NULL;
			}
		    break;

		case MLX_TOK_ASTERISK: /* outer join? */
		    if (cmpflags & EXPR_CMP_OUTERJOIN)
		        {
			if ((t = mlxNextToken(lxs)) == MLX_TOK_EQUALS || t == MLX_TOK_COMPARE)
			    {
			    etmp->NodeType = EXPR_N_COMPARE;
			    etmp->Flags |= EXPR_F_LOUTERJOIN;
			    if (t == MLX_TOK_EQUALS)
				etmp->CompareType = MLX_CMP_EQUALS;
			    else
				etmp->CompareType = mlxIntVal(lxs);
			    break;
			    }
			else
			    {
			    mlxHoldToken(lxs);
		    	    etmp->NodeType = EXPR_N_MULTIPLY;
		    	    break;
			    }
			}
		    if (mlxNextToken(lxs) != MLX_TOK_EQUALS)
		        {
			mlxHoldToken(lxs);
			etmp->NodeType = EXPR_N_MULTIPLY;
			break;
			}

		    /** asterisk but no outer join or not allowed? **/
		    expFreeExpression(etmp);
		    if (expr) expFreeExpression(expr);
		    mssError(1,"EXP","*= encountered but outerjoins not allowed in expression");
		    err=1;
		    expr = NULL;
		    break;

		case MLX_TOK_EQUALS:
		    etmp->NodeType = EXPR_N_COMPARE;
		    etmp->CompareType = MLX_CMP_EQUALS;

		    /** Check outer join? **/
		    if (cmpflags & EXPR_CMP_OUTERJOIN)
		        {
			if (mlxNextToken(lxs) == MLX_TOK_ASTERISK)
			    {
			    etmp->Flags |= EXPR_F_ROUTERJOIN;
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    }
			}
		    break;

		case MLX_TOK_COMPARE:
		    etmp->NodeType = EXPR_N_COMPARE;
		    etmp->CompareType = mlxIntVal(lxs);

		    /** Check outer join? **/
		    if (cmpflags & EXPR_CMP_OUTERJOIN)
		        {
			if (mlxNextToken(lxs) == MLX_TOK_ASTERISK)
			    {
			    etmp->Flags |= EXPR_F_ROUTERJOIN;
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    }
			}
		    break;

		case MLX_TOK_PLUS:
		    etmp->NodeType = EXPR_N_PLUS;
		    break;

		case MLX_TOK_SLASH:
		    etmp->NodeType = EXPR_N_DIVIDE;
		    break;

		case MLX_TOK_FILENAME:
		    sptr = mlxStringVal(lxs,NULL);
		    if (sptr && !strcmp(sptr,"/"))
		        {
			etmp->NodeType = EXPR_N_DIVIDE;
			}
		    else
		        {
			mssError(1,"EXP","Unexpected filename in expression string");
			}
		    break;

		case MLX_TOK_MINUS:
		    etmp->NodeType = EXPR_N_MINUS;
		    break;

		default:
		    expFreeExpression(etmp);
		    if (expr) expFreeExpression(expr);
		    mssError(1,"EXP","Unexpected token in expression string");
		    mlxNoteError(lxs);
		    err=1;
		    expr = NULL;
		    break;
		}
	    if (err) break;
	    etmp->Flags |= EXPR_F_OPERATOR;

	    /** Now put the operator in the right place. **/
	    if (expr->Flags & EXPR_F_OPERATOR)
		{
		/** Scan until the precedence is correct **/
		while(eptr->Parent && EXP.Precedence[etmp->NodeType] >= EXP.Precedence[eptr->Parent->NodeType])
		    {
		    eptr = eptr->Parent;
		    }

		/** If at top, update expr, else substitute node **/
		if (!(eptr->Parent))
		    {
		    expr->Parent = etmp;
		    xaAddItem(&(etmp->Children),(void*)expr);
		    expr = etmp;
		    eptr = expr;
		    }
		else
		    {
		    xaRemoveItem(&(eptr->Parent->Children),xaFindItem(&(eptr->Parent->Children),(void*)eptr));
		    etmp->Parent = eptr->Parent;
		    eptr->Parent = etmp;
		    xaAddItem(&(etmp->Parent->Children),(void*)etmp);
		    xaAddItem(&(etmp->Children),(void*)eptr);
		    eptr = etmp;
		    }
		}
	    else
		{
		expr->Parent = etmp;
		xaAddItem(&(etmp->Children),(void*)expr);
		expr = etmp;
		eptr = expr;
		}
	    }

    return expr;
    }


/*** exp_internal_SetCoverageMask - calculates the object coverage mask
 *** for upper-level nodes.
 ***/
int
exp_internal_SetCoverageMask(pExpression exp)
    {
    int i;
    pExpression subexp;

    	/** Shortcut quit **/
	if (exp->NodeType == EXPR_N_OBJECT || exp->NodeType == EXPR_N_PROPERTY) return 0;

	/** Compute coverage mask for this based on child objects **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    subexp = exp->Children.Items[i];
	    exp_internal_SetCoverageMask(subexp);
	    exp->ObjCoverageMask |= subexp->ObjCoverageMask;
	    }

	/** Coverage mask for direct references (incl getdate() and user_name()) **/
	if (exp->NodeType == EXPR_N_FUNCTION && (!strcmp(exp->Name,"user_name") || !strcmp(exp->Name,"getdate") || !strcmp(exp->Name,"row_number") || !strcmp(exp->Name, "eval")))
	    {
	    exp->ObjCoverageMask |= EXPR_MASK_EXTREF;
	    }
	if (exp->NodeType == EXPR_N_FUNCTION && (!strcmp(exp->Name,"substitute")))
	    {
	    exp->ObjCoverageMask |= EXPR_MASK_INDETERMINATE;
	    }
	if (exp->NodeType == EXPR_N_SUBQUERY)
	    {
	    exp->ObjCoverageMask |= EXPR_MASK_INDETERMINATE;
	    /*exp->ObjCoverageMask |= EXPR_MASK_EXTREF;*/
	    }

    return 0;
    }


/*** exp_internal_SetDomain - set up domain (runserver/etc) flags
 ***/
int
exp_internal_SetDomain(pExpression exp)
    {
    int i;
    int has_runserver = 0;
    pExpression subexp;

	/** Handle children first **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    subexp = exp->Children.Items[i];
	    exp_internal_SetDomain(subexp);
	    if (subexp->Flags & (EXPR_F_RUNSERVER | EXPR_F_HASRUNSERVER))
		has_runserver = 1;
	    }

	/** Set flag **/
	if (has_runserver && !(exp->Flags & EXPR_F_RUNSERVER))
	    exp->Flags |= EXPR_F_HASRUNSERVER;

    return 0;
    }


/*** exp_internal_SetAggLevel - determine the level of aggregate nesting
 *** in an expression.
 ***/
int
exp_internal_SetAggLevel(pExpression exp)
    {
    int i;
    int max_level = 0,rval;
    pExpression child;

    	/** First, search for the max level of child expressions **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    child = (pExpression)(exp->Children.Items[i]);
	    rval = exp_internal_SetAggLevel(child);
	    if (rval > max_level) max_level = rval;
	    }

	/** Is this an aggregate function?  If so, set level one higher **/
	if (exp->NodeType == EXPR_N_FUNCTION && (exp->Flags & EXPR_F_AGGREGATEFN))
	    {
	    max_level++;
	    }

	/** Set the level and return it **/
	exp->AggLevel = max_level;

    return max_level;
    }


/*** exp_internal_SetListCounts - set the list count levels for each node so
 *** the evaluation routines know how many times to iterate the evaluation.  A
 *** non-list subtree will have list count values of 1.  When lists are
 *** encountered, its list count will equal the sum of its items' list counts.
 *** For all other nodes, the list count is equal to the product of its child
 *** items list counts.  Thus, the top-level expression's list count will be
 *** equal to the number of distinct evaluations the expression can have due
 *** to various combinations of list items.
 ***
 *** If a list is encountered with a parent IN statement, then the list count
 *** of the IN statement must be set to 1, since the sub-items of the IN list
 *** can then be automatically iterated through for proper evaluation.  This
 *** is also true for any other comparison expressions as well.  The idea is
 *** that the expression substring('abcdefghi',(1,4,7),3) has three values,
 *** but the expression :a:strval IN ('abc','def','ghi') has only one value,
 *** that is, whether 'strval' is equal to any of the three items in the list.
 *** Other boolean operators behave in the same fashion.
 ***/
int
exp_internal_SetListCounts(pExpression exp)
    {
    int i,product,sum;

    	/** Node is an EXPR_N_LIST node? **/
	if (exp->NodeType == EXPR_N_LIST)
	    {
    	    /** Evaluate all sub-nodes, summing the list counts. **/
	    sum = 0;
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		sum += ((pExpression)(exp->Children.Items[i]))->ListCount;
	        }
	    exp->ListCount = sum;
	    }
	else if (exp->NodeType == EXPR_N_IN || exp->NodeType == EXPR_N_COMPARE)
	    {
	    /** Boolean compare node? **/
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		}
	    exp->ListCount = 1;
	    }
	else
	    {
	    /** Node is a normal node -- list count is product (1 if no child nodes). **/
	    product = 1;
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		product *= ((pExpression)(exp->Children.Items[i]))->ListCount;
	        }
	    exp->ListCount = product;
	    }

    return 0;
    }


/*** expCompileExpressionFromLxs - builds an expression, given an already-open 
 *** lexer session.
 ***/
pExpression
expCompileExpressionFromLxs(pLxSession s, pParamObjects objlist, int cmpflags)
    {
    pExpression e;

	/** Parse it. **/
	e = exp_internal_CompileExpression_r(s, 0, objlist, cmpflags);
	if (!e) return NULL;
	/*if (!(e->Flags & EXPR_F_DOMAINMASK)) e->Flags |= EXPR_F_RUNDEFAULT;*/
	exp_internal_SetAggLevel(e);
	exp_internal_SetCoverageMask(e);
	exp_internal_SetDomain(e);

	/** Set SEQ ids. **/
	exp_internal_SetupControl(e);
	e->Control->PSeqID = 0;

	e->CmpFlags = cmpflags;
	e->LxFlags = s->Flags;

    return e;
    }
 


/*** expCompileExpression - uses the above function to parse
 *** the expression and build the expression tree.
 ***/
pExpression
expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags)
    {
    pExpression e = NULL;
    pLxSession lxs;
    pParamObjects my_objlist;
    int mlxFlags;

	/** Create a temporary objlist? **/
	if (objlist)
	    my_objlist = objlist;
	else
	    {
	    my_objlist = expCreateParamList();
	    expAddParamToList(my_objlist, "this", NULL, EXPR_O_CURRENT);
	    }

	/** Open the lexer on the input text. **/
	mlxFlags = MLX_F_EOF | lxflags;
	if(CxGlobals.CharacterMode == CharModeUTF8) mlxFlags |= MLX_F_ENFORCEUTF8;
	lxs = mlxStringSession(text, mlxFlags);
	if (!lxs) 
	    {
	    mssError(0,"EXP","Could not begin analysis of expression");
	    return NULL;
	    }

	e = expCompileExpressionFromLxs(lxs, objlist, cmpflags);

	/** Close the lexer session. **/
	mlxCloseSession(lxs);

	if (!objlist)
	    expFreeParamList(my_objlist);

    return e;
    }


/*** expBindExpression - do late binding of an expression tree to an
 *** object list.  'domain' specifies the requested bind domain, whether
 *** runstatic (EXP_F_RUNSTATIC), runserver (EXP_F_RUNSERVER), or runclient
 *** (EXP_F_RUNCLIENT).  'domain' can also be -0-, in which case we rebind
 *** a domainless expression.
 ***/
int
expBindExpression(pExpression exp, pParamObjects objlist, int flags)
    {
    int i,cm=0;
    int idx;
    int domain = flags & EXPR_F_DOMAINMASK;

	/** For a property node, check if the object should be set. **/
	if (exp->NodeType == EXPR_N_PROPERTY && ((exp->Flags & domain) || !domain))
	    {
	    if (exp->ObjID == -1 && exp->Parent && exp->Parent->NodeType == EXPR_N_OBJECT)
		{
		for(i=0;i<objlist->nObjects;i++)
		    {
		    if (flags & EXPR_F_REVERSE)
			idx = (objlist->nObjects-1) - i;
		    else
			idx = i;
		    if (objlist->Names[idx] && !strcmp(exp->Parent->Name, objlist->Names[idx]))
			{
			cm |= (1<<idx);
			exp->ObjID = idx;
			break;
			}
		    }
		if (exp->ObjID == -1)
		    {
		    cm |= EXPR_MASK_EXTREF;
		    }
		}
	    else if (exp->ObjID == -2 || exp->ObjID == -3)
		{
		if (exp->ObjID == -2) cm |= (1<<(objlist->CurrentID));
		if (exp->ObjID == -3) cm |= (1<<(objlist->ParentID));
		}
	    else if (exp->ObjID >= 0)
		{
		cm |= (1<<(exp->ObjID));
		}
	    }

	/** Check for absolute references in functions **/
	if (exp->NodeType == EXPR_N_FUNCTION && (!strcmp(exp->Name,"getdate") || !strcmp(exp->Name,"user_name") || !strcmp(exp->Name,"row_number")))
	    {
	    cm |= EXPR_MASK_EXTREF;
	    }
	if (exp->NodeType == EXPR_N_SUBQUERY)
	    {
	    cm |= EXPR_MASK_INDETERMINATE;
	    /*cm |= EXPR_MASK_EXTREF;*/
	    }

	/** Loop through subnodes in the tree to process them as well. **/
	for(i=0;i<exp->Children.nItems;i++) cm |= expBindExpression((pExpression)(exp->Children.Items[i]), objlist, domain);
	exp->ObjCoverageMask = cm;

    return cm;
    }

