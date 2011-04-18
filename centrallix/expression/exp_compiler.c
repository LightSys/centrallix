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

/**CVSDATA***************************************************************

    $Id: exp_compiler.c,v 1.22 2011/02/18 03:47:46 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_compiler.c,v $

    $Log: exp_compiler.c,v $
    Revision 1.22  2011/02/18 03:47:46  gbeeley
    enhanced ORDER BY, IS NOT NULL, bug fix, and MQ/EXP code simplification

    - adding multiq_orderby which adds limited high-level order by support
    - adding IS NOT NULL support
    - bug fix for issue involving object lists (param lists) in query
      result items (pseudo objects) getting out of sorts
    - as a part of bug fix above, reworked some MQ/EXP code to be much
      cleaner

    Revision 1.21  2010/09/08 21:55:09  gbeeley
    - (bugfix) allow /file/name:"attribute" to be quoted.
    - (bugfix) order by ... asc/desc keywords are now case insenstive
    - (bugfix) short-circuit eval was not resulting in aggregates properly
      evaluating
    - (change) new API function expModifyParamByID - use this for efficiency
    - (feature) multi-level aggregate functions now supported, for use when
      a sql query has a group by, e.g. select max(sum(...)) ... group by ...
    - (feature) added mathematical and trig functions radians, degrees, sin,
      cos, tan, asin, acos, atan, atan2, sqrt, square

    Revision 1.20  2010/01/10 07:33:23  gbeeley
    - (performance) reduce the number of times that subqueries are executed by
      only re-evaluating them if one of the ObjList entries has changed
      (instead of re-evaluating every time).  Ideally we should check for what
      objects are referenced by the subquery, but that is for a later fix...

    Revision 1.19  2009/06/24 17:33:19  gbeeley
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

    Revision 1.18  2008/03/09 07:58:50  gbeeley
    - (bugfix) sometimes the object name in objlist->Names[] is NULL.

    Revision 1.17  2008/02/25 23:14:33  gbeeley
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

    Revision 1.16  2007/03/21 04:48:08  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.15  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.14  2007/03/04 05:04:47  gbeeley
    - (change) This is a change to the way that expressions track which
      objects they were last evaluated against.  The old method was causing
      some trouble with stale data in some expressions.

    Revision 1.13  2006/09/15 20:40:27  gbeeley
    - (change) allow "strings" to be used as identifiers when specifying column
      names in queries.  This permits column/attribute names to contain special
      characters, including spaces.

    Revision 1.12  2005/09/30 04:37:10  gbeeley
    - (change) modified expExpressionToPod to take the type.
    - (feature) got eval() working
    - (addition) added expReplaceString() to search-and-replace in an
      expression tree.

    Revision 1.11  2005/09/17 01:28:19  gbeeley
    - Some fixes for handling of direct object attributes in expressions,
      such as /path/to/object:attributename.

    Revision 1.10  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.9  2004/02/24 20:02:26  gbeeley
    - adding proper support for external references in an expression, so
      that they get re-evaluated each time.  Example - getdate().
    - adding eval() function but no implementation at this time - it is
      however supported for runclient() expressions (in javascript).
    - fixing some quoting issues

    Revision 1.8  2003/07/09 18:07:55  gbeeley
    Added first() and last() aggregate functions.  Strictly speaking these
    are not truly relational functions, since they are row order dependent,
    but are useful for summary items in a sorted setting.

    Revision 1.7  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

    Revision 1.6  2003/05/30 17:39:48  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.5  2003/04/24 02:13:22  gbeeley
    Added functionality to handle "domain of execution" to the expression
    module, allowing the developer to specify the nature of an expression
    (run on client, server, or static on server).

    Revision 1.4  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.3  2001/10/16 23:53:01  gbeeley
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

    Revision 1.2  2001/09/28 20:04:50  gbeeley
    Minor efficiency enhancement to expression trees.  Most PROPERTY nodes
    are now self-contained and require no redundant OBJECT nodes as parent
    nodes.  Substantial reduction in expression node allocation and
    evaluation.

    Revision 1.1.1.1  2001/08/13 18:00:47  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


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
				    !strcasecmp(etmp->Name,"last"))
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
				    if (objlist->Names[i] && !strcmp(etmp->Name,objlist->Names[i]))
					{
					if (etmp->NameAlloc)
					    {
					    nmSysFree(etmp->Name);
					    etmp->NameAlloc = 0;
					    }
					etmp->Name = NULL;
					etmp->ObjID = i;
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
			if (mlxNextToken(lxs) == MLX_TOK_EQUALS)
			    {
			    etmp->NodeType = EXPR_N_COMPARE;
			    etmp->Flags |= EXPR_F_LOUTERJOIN;
			    etmp->CompareType = MLX_CMP_EQUALS;
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
	if (exp->NodeType == EXPR_N_FUNCTION && (!strcmp(exp->Name,"user_name") || !strcmp(exp->Name,"getdate")))
	    {
	    exp->ObjCoverageMask |= EXPR_MASK_EXTREF;
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

	/** Open the lexer on the input text. **/
	lxs = mlxStringSession(text, MLX_F_EOF | lxflags);
	if (!lxs) 
	    {
	    mssError(0,"EXP","Could not begin analysis of expression");
	    return NULL;
	    }

	e = expCompileExpressionFromLxs(lxs, objlist, cmpflags);

	/** Close the lexer session. **/
	mlxCloseSession(lxs);

    return e;
    }


/*** expBindExpression - do late binding of an expression tree to an
 *** object list.  'domain' specifies the requested bind domain, whether
 *** runstatic (EXP_F_RUNSTATIC), runserver (EXP_F_RUNSERVER), or runclient
 *** (EXP_F_RUNCLIENT).
 ***/
int
expBindExpression(pExpression exp, pParamObjects objlist, int domain)
    {
    int i,cm=0;

	/** For a property node, check if the object should be set. **/
	if (exp->NodeType == EXPR_N_PROPERTY && (exp->Flags & domain) && exp->ObjID == -1 && exp->Parent && exp->Parent->NodeType == EXPR_N_OBJECT)
	    {
	    for(i=0;i<objlist->nObjects;i++)
		{
		if (objlist->Names[i] && !strcmp(exp->Parent->Name, objlist->Names[i]))
		    {
		    cm |= (1<<i);
		    exp->ObjID = i;
		    break;
		    }
		}
	    if (exp->ObjID == -1)
		{
		cm |= EXPR_MASK_EXTREF;
		}
	    }

	/** Check for absolute references in functions **/
	if (exp->NodeType == EXPR_N_FUNCTION && (!strcmp(exp->Name,"getdate") || !strcmp(exp->Name,"user_name")))
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

