#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "expression.h"
#include "mtsession.h"

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
/* Module: 	expression.h, exp_evaluate.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*									*/
/*		--> exp_evaluate.c: provides the functionality to 	*/
/*		    evaluate an expression against a list of param	*/
/*		    objects.						*/
/*		If you add new node types, add the evaluator function	*/
/*		here and optionally the reverse evaluator function, add */
/*		it to the list of evaluator functions (added in an	*/
/*		xarray in the EXP structure), and define a precedence	*/
/*		for the node type.  You will likely also have to modify	*/
/*		the compiler (exp_compiler.c) to properly interpret	*/
/*		your new node type when it occurs in a textual exp.	*/
/*									*/
/*		Be careful adding new node types!  If the objectsystem	*/
/*		driver being objQuery'd "eats" the expression tree and	*/
/*		passes the expression on to the remote DBMS or other	*/
/*		server, it also may need to be made aware of your new	*/
/*		node type.  If the new node type can't be handled in 	*/
/*		that DBMS/server in a clean manner, it should be noted	*/
/*		as such.  Otherwise, be sure to note and/or add the	*/
/*		functionality to handle such a node in that DBMS or	*/
/*		server's objectsystem driver.  One current example of	*/
/*		this is the Sybase objectsystem driver.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: exp_evaluate.c,v 1.9 2003/06/27 21:19:47 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_evaluate.c,v $

    $Log: exp_evaluate.c,v $
    Revision 1.9  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

    Revision 1.8  2002/11/22 19:29:36  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.7  2002/08/10 02:09:44  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.6  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.5  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.4  2001/10/16 23:53:01  gbeeley
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

    Revision 1.3  2001/10/02 16:23:09  gbeeley
    Added exp_generator expressiontree-to-text generation module.  Also fixed
    a precedence problem with EXPR_N_FUNCTION nodes; not sure why that wasn't
    causing trouble previously.

    Revision 1.2  2001/09/28 20:04:50  gbeeley
    Minor efficiency enhancement to expression trees.  Most PROPERTY nodes
    are now self-contained and require no redundant OBJECT nodes as parent
    nodes.  Substantial reduction in expression node allocation and
    evaluation.

    Revision 1.1.1.1  2001/08/13 18:00:47  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:52  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** expEvalContains - evaluate a CONTAINS node type.  This is for 
 *** determining whether an object's content CONTAINS the given string.
 *** Normally, this node should be sheared off by an Index object and
 *** thus a full search won't be required.
 ***/
int
expEvalContains(pExpression tree, pParamObjects objlist)
    {
    pExpression i0, i1;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The CONTAINS operator requires two operands");
	    return -1;
	    }

	/** Evaluate the children first **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = 0;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;
	
	/** Special case :object:content does objRead() **/
	/*if (i0->NodeType == EXPR_N_OBJECT && */
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;

	/** not quite done ;) **/

    return 0;
    }


/*** expEvalDivide - evaluate a DIVIDE node.  This is strictly for numeric
 *** type values.
 ***/
int
expEvalDivide(pExpression tree, pParamObjects objlist)
    {
    pExpression i0,i1;
    MoneyType m;
    void* dptr;
    int i;
    int is_negative = 0;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The / operator requires two operands");
	    return -1;
	    }

	/** Evaluate child expression trees **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = -1;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;

	/** If either is NULL, result is NULL. **/
	if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Get a data ptr to the 2nd data type value **/
	switch(i1->DataType)
	    {
	    case DATA_T_INTEGER: dptr = &(i1->Integer); break;
	    case DATA_T_DOUBLE: dptr = &(i1->Types.Double); break;
	    case DATA_T_MONEY: dptr = &(i1->Types.Money); break;
	    default:
	        mssError(1,"EXP","Only integer,double,money types allowed for '/'");
		return -1;
	    }

	/** Check for divide by zero **/
	if ((i1->DataType == DATA_T_INTEGER && i1->Integer == 0) ||
	    (i1->DataType == DATA_T_DOUBLE && i1->Types.Double == 0.0) ||
	    (i1->DataType == DATA_T_MONEY && i1->Types.Money.WholePart == 0 && i1->Types.Money.FractionPart == 0))
	    {
	    mssError(1,"EXP","Attempted divide by zero");
	    return -1;
	    }

	/** Perform the operation **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_INTEGER;
			tree->Integer = i0->Integer / i1->Integer;
			break;
		    case DATA_T_DOUBLE:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Integer / i1->Types.Double;
			break;
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_MONEY;
			mssError(1,"EXP","Unimplemented integer <divide> money operation");
			return -1;
		        break;
		    }
	        break;

	    case DATA_T_DOUBLE:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Types.Double / i1->Integer;
			break;
		    case DATA_T_DOUBLE:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Types.Double / i1->Types.Double;
			break;
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_MONEY;
			mssError(1,"EXP","Unimplemented double <divide> money operation");
			return -1;
		        break;
		    }
	        break;

	    case DATA_T_MONEY:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_MONEY;
			memcpy(&m, &(i0->Types.Money), sizeof(MoneyType));
			if (m.WholePart < 0)
		    	    {
			    is_negative = !is_negative;
			    if (m.FractionPart != 0)
		        	{
				m.WholePart = m.WholePart + 1;
				m.FractionPart = 10000 - m.FractionPart;
				}
			    m.WholePart = -m.WholePart;
		            }
			i = i1->Integer;
			if (i < 0) 
			    {
			    i = -i;
			    is_negative = !is_negative;
			    }
			tree->Types.Money.WholePart = m.WholePart / i;
			tree->Types.Money.FractionPart = (10000*(m.WholePart % i) + m.FractionPart)/i;
			if (is_negative)
			    {
			    if (tree->Types.Money.FractionPart != 0)
		        	{
				tree->Types.Money.WholePart = tree->Types.Money.WholePart + 1;
				tree->Types.Money.FractionPart = 10000 - tree->Types.Money.FractionPart;
				}
			    tree->Types.Money.WholePart = -tree->Types.Money.WholePart;
			    }
			break;
		    case DATA_T_DOUBLE:
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_MONEY;
			mssError(1,"EXP","Unimplemented money <divide> x operation");
			return -1;
		        break;
		    }
	        break;

	    default: 
	        mssError(1,"EXP","Only integer,double,money types allowed for '/'");
		return -1;
	    }

    return 0;
    }


/*** expEvalMultiply - evaluate a MULTIPLY node.  This is only for
 *** multiplying numeric values.
 ***/
int
expEvalMultiply(pExpression tree, pParamObjects objlist)
    {
    pExpression i0,i1;
    MoneyType m,m2;
    void* dptr;
    int n;
    int is_negative = 0;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The * operator requires two operands");
	    return -1;
	    }

	/** Evaluate child expression trees **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = -1;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;

	/** If either is NULL, result is NULL. **/
	if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Get a data ptr to the 2nd data type value **/
	switch(i1->DataType)
	    {
	    case DATA_T_INTEGER: dptr = &(i1->Integer); break;
	    case DATA_T_STRING: dptr = i1->String; break;
	    case DATA_T_DOUBLE: dptr = &(i1->Types.Double); break;
	    case DATA_T_MONEY: dptr = &(i1->Types.Money); break;
	    case DATA_T_DATETIME: dptr = &(i1->Types.Date); break;
	    case DATA_T_INTVEC: dptr = &(i1->Types.IntVec); break;
	    case DATA_T_STRINGVEC: dptr = &(i1->Types.StrVec); break;
	    default:
		mssError(1,"EXP","Unexpected data type for operand #2 of '*': %d",i1->DataType);
		return -1;
	    }

	/** Do the computation based on the datatype of the first operand **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: 
	        tree->Integer = i0->Integer * objDataToInteger(i1->DataType, dptr, NULL);
		tree->DataType = DATA_T_INTEGER;
		break;

	    case DATA_T_DOUBLE:
	        tree->Types.Double = i0->Types.Double * objDataToDouble(i1->DataType, dptr);
		tree->DataType = DATA_T_DOUBLE;
		break;

	    case DATA_T_MONEY:
	        /** BUG BUG!! **/
	        mssError(1,"EXP","ERROR: UNIMPLEMENTED OPERATION moneytype * other_type");
		return -1;

	        objDataToMoney(i1->DataType, dptr, &m2);
		memcpy(&m, &(i0->Types.Money), sizeof(MoneyType));
		tree->DataType = DATA_T_MONEY;

		/** First step is convert both to positive amounts **/
		if (m.WholePart < 0)
		    {
		    is_negative = !is_negative;
		    if (m.FractionPart != 0)
		        {
			m.WholePart = m.WholePart + 1;
			m.FractionPart = 10000 - m.FractionPart;
			}
		    m.WholePart = -m.WholePart;
		    }
		if (m2.WholePart < 0)
		    {
		    is_negative = !is_negative;
		    if (m2.FractionPart != 0)
		        {
			m2.WholePart = m2.WholePart + 1;
			m2.FractionPart = 10000 - m2.FractionPart;
			}
		    m2.WholePart = -m2.WholePart;
		    }

		/** Second step is to compute the result value. **/
		tree->Types.Money.WholePart = m.WholePart * m2.WholePart;
		n = m.FractionPart * m2.FractionPart;
		tree->Types.Money.FractionPart = n%10000;
		tree->Types.Money.WholePart += n/10000;
		n = 0;

		/** Final step is to apply the negative sign, if necessary **/
		if (is_negative)
		    {
		    if (tree->Types.Money.FractionPart != 0)
		        {
			tree->Types.Money.WholePart = tree->Types.Money.WholePart - 1;
			tree->Types.Money.FractionPart = 10000 - tree->Types.Money.FractionPart;
			}
		    tree->Types.Money.WholePart = -tree->Types.Money.WholePart;
		    }
		break;

	    default:
	        mssError(1,"EXP","Only integer, double, and money types supported by *");
		return -1;
	    }

    return 0;
    }


/*** expEvalMinus - evaluate a MINUS node type.  This operates exclusively
 *** on numeric values.
 ***/
int
expEvalMinus(pExpression tree, pParamObjects objlist)
    {
    pExpression i0, i1;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The - operator requires two operands");
	    return -1;
	    }

	/** Evaluate the children first **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = -1;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;

	/** If either is NULL, result is NULL. **/
	if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Perform the operation depending on data type **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_INTEGER;
		        tree->Integer = i0->Integer - i1->Integer;
			break;
		    case DATA_T_DOUBLE:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = (double)(i0->Integer) - i1->Types.Double;
			break;
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_MONEY;
			tree->Types.Money.WholePart = i0->Integer - i1->Types.Money.WholePart;
			if (i1->Types.Money.FractionPart == 0)
			    {
			    tree->Types.Money.FractionPart = 0;
			    }
			else
			    {
			    tree->Types.Money.WholePart--;
			    tree->Types.Money.FractionPart = 10000 - i1->Types.Money.FractionPart;
			    }
			break;
		    }
		break;

	    case DATA_T_DOUBLE:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Types.Double - (double)(i1->Integer);
			break;
		    case DATA_T_DOUBLE:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Types.Double - i1->Types.Double;
			break;
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = i0->Types.Double - (i1->Types.Money.WholePart + i1->Types.Money.FractionPart/10000.0);
			break;
		    }
		break;

	    case DATA_T_MONEY:
	        switch(i1->DataType)
		    {
		    case DATA_T_INTEGER:
		        tree->DataType = DATA_T_MONEY;
			tree->Types.Money.WholePart = i0->Types.Money.WholePart - i1->Integer;
			tree->Types.Money.FractionPart = i0->Types.Money.FractionPart;
			break;
		    case DATA_T_DOUBLE:
		        tree->DataType = DATA_T_DOUBLE;
			tree->Types.Double = (i0->Types.Money.WholePart + i0->Types.Money.FractionPart/10000.0) - i1->Types.Double;
			break;
		    case DATA_T_MONEY:
		        tree->DataType = DATA_T_MONEY;
			tree->Types.Money.WholePart = i0->Types.Money.WholePart - i1->Types.Money.WholePart;
			tree->Types.Money.FractionPart = 10000 + i0->Types.Money.FractionPart - i1->Types.Money.FractionPart;
			if (tree->Types.Money.FractionPart >= 10000)
			    {
			    tree->Types.Money.FractionPart -= 10000;
			    }
			else
			    {
			    tree->Types.Money.WholePart--;
			    }
		        break;
		    }
		break;

	    default:
	        mssError(1,"EXP","Only integer, double, and money types valid for '-'");
		return -1;
	    }

    return 0;
    }


/*** expEvalPlus - evaluate a PLUS node type.  This is for adding
 *** numbers and concatenating strings and string/number combinations.
 ***/
int
expEvalPlus(pExpression tree, pParamObjects objlist)
    {
    pExpression i0, i1;
    void* dptr;
    char* ptr;
    MoneyType m;
    int i;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The + operator requires two operands");
	    return -1;
	    }

	/** Evaluate the children first **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = -1;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;

	/** If either is NULL, result is NULL. **/
	if ((i0->Flags | i1->Flags) & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Get a data ptr to the 2nd data type value **/
	switch(i1->DataType)
	    {
	    case DATA_T_INTEGER: dptr = &(i1->Integer); break;
	    case DATA_T_STRING: dptr = i1->String; break;
	    case DATA_T_DOUBLE: dptr = &(i1->Types.Double); break;
	    case DATA_T_MONEY: dptr = &(i1->Types.Money); break;
	    case DATA_T_DATETIME: dptr = &(i1->Types.Date); break;
	    case DATA_T_INTVEC: dptr = &(i1->Types.IntVec); break;
	    case DATA_T_STRINGVEC: dptr = &(i1->Types.StrVec); break;
	    default:
		mssError(1,"EXP","Unexpected data type for operand #2 of '+': %d",i1->DataType);
		return -1;
	    }

	/** If first is an Integer, do integer addition. **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER:
	        tree->Integer = i0->Integer + objDataToInteger(i1->DataType, dptr, NULL);
		tree->DataType = DATA_T_INTEGER;
		break;

	    case DATA_T_STRING:
	        tree->DataType = DATA_T_STRING;
		ptr = objDataToStringTmp(i1->DataType, dptr, 0);
	        if (tree->Alloc && tree->String)
	            {
		    nmSysFree(tree->String);
		    tree->String = NULL;
		    }

		/** Auto-rtrim if the thing is an object/property item **/
		i = strlen(i0->String);
		if (i0->NodeType == EXPR_N_OBJECT || i0->NodeType == EXPR_N_PROPERTY)
		    {
		    while(i && i0->String[i-1] == ' ') i--;
		    }
		tree->String = (char*)nmSysMalloc(i + strlen(ptr) + 1);
		tree->Alloc = 1;
		sprintf(tree->String,"%*.*s%s",i,i, i0->String, ptr);
		break;

	    case DATA_T_DOUBLE:
	        tree->Types.Double = i0->Types.Double + objDataToDouble(i1->DataType, dptr);
		tree->DataType = DATA_T_DOUBLE;
		break;

	    case DATA_T_MONEY:
	        objDataToMoney(i1->DataType, dptr, &m);
		tree->DataType = DATA_T_MONEY;
		tree->Types.Money.WholePart = i0->Types.Money.WholePart + m.WholePart;
		tree->Types.Money.FractionPart = i0->Types.Money.FractionPart + m.FractionPart;
		if (tree->Types.Money.FractionPart >= 10000)
		    {
		    tree->Types.Money.FractionPart -= 10000;
		    tree->Types.Money.WholePart++;
		    }
		break;

	    case DATA_T_DATETIME:
	        mssError(1,"EXP","Cannot add DATETIME values together");
	        return -1;
	    }

    return 0;
    }


/*** expEvalIsNull - evaluate an IS NULL node type.
 ***/
int
expEvalIsNull(pExpression tree, pParamObjects objlist)
    {
    int t;

	/** Evaluate the child tree structure **/
    	t = exp_internal_EvalTree((pExpression)(tree->Children.Items[0]),objlist);
	if (t < 0) return -1;

	/** Check null of the evaluated item. **/
	tree->DataType = DATA_T_INTEGER;
	if (((pExpression)(tree->Children.Items[0]))->Flags & EXPR_F_NULL)
	    tree->Integer = 1;
	else
	    tree->Integer = 0;

    return 1;
    }


/*** expRevEvalIsNull - reverse evaluate an ISNULL node.  This
 *** doesn't do anything, since we can't nullify an object's
 *** existing property.
 ***/
int
expRevEvalIsNull(pExpression tree, pParamObjects objlist)
    {
    return 0;
    }


/*** expEvalAnd - evaluate an AND node type.
 ***/
int
expEvalAnd(pExpression tree, pParamObjects objlist)
    {
    int i,t;

	/** Loop through items... **/
	tree->DataType = DATA_T_INTEGER;
	for(i=0;i<tree->Children.nItems;i++) 
	    {
	    t=exp_internal_EvalTree((pExpression)(tree->Children.Items[i]),objlist);
	    if (t < 0 || ((pExpression)(tree->Children.Items[i]))->DataType != DATA_T_INTEGER) 
	        {
		mssError(1,"EXP","The AND operator only works on valid integer/boolean values");
		return -1;
		}
	    if (((pExpression)(tree->Children.Items[i]))->Integer == 0) 
		{
		tree->Integer = 0;
		return 0;
		}
	    }
	tree->Integer = 1;

    return 1;
    }


/*** expEvalOr - evaluate an OR node type.
 ***/
int
expEvalOr(pExpression tree, pParamObjects objlist)
    {
    int i,t;

    	/** Loop through items **/
	tree->DataType = DATA_T_INTEGER;
	for(i=0;i<tree->Children.nItems;i++) 
	    {
	    t=exp_internal_EvalTree((pExpression)(tree->Children.Items[i]),objlist);
	    if (t < 0 || ((pExpression)(tree->Children.Items[i]))->DataType != DATA_T_INTEGER) 
	        {
		mssError(1,"EXP","The OR operator only works on valid integer/boolean values");
		return -1;
		}
	    if (((pExpression)(tree->Children.Items[i]))->Integer == 1) 
		{
		tree->Integer = 1;
		return 1;
		}
	    }
	tree->Integer = 0;

    return 0;
    }


/*** expRevEvalAnd - reverse-evaluate an AND node.
 ***/
int
expRevEvalAnd(pExpression tree, pParamObjects objlist)
    {
    int i, s = -1;
    pExpression subtree;
    	
	/** If the AND is FALSE, then we really can't do anything. **/
	if (tree->Integer == 0) return -1;

	/** If the answer was NULL, can't do much either **/
	if (tree->Flags & EXPR_F_NULL) return -1;

	/** If the AND is TRUE, then pass TRUE to each of its children. **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pExpression)(tree->Children.Items[i]);
	    subtree->Integer = 1;
	    s = expReverseEvalTree(subtree, objlist);
	    if (s < 0) break;
	    }
    
    return s;
    }


/*** expRevEvalOr - reverse-evaluate an OR node.
 ***/
int
expRevEvalOr(pExpression tree, pParamObjects objlist)
    {
    int i, s = -1;
    pExpression subtree;
    	
	/** If the OR is TRUE, then we really can't do anything. **/
	if (tree->Integer == 1) return -1;

	/** If the answer was NULL, can't do much either **/
	if (tree->Flags & EXPR_F_NULL) return -1;

	/** If the OR is FALSE, then pass FALSE to each of its children. **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pExpression)(tree->Children.Items[i]);
	    subtree->Integer = 0;
	    s = expReverseEvalTree(subtree, objlist);
	    if (s < 0) break;
	    }
    
    return s;
    }


/*** expEvalCompare - evaluate a comparison node type.
 ***/
int
expEvalCompare(pExpression tree, pParamObjects objlist)
    {
    int v;
    pExpression i0, i1;
    void* dptr0;
    void* dptr1;

	/** Verify item cnt **/
	if (tree->Children.nItems != 2) 
	    {
	    mssError(1,"EXP","The comparison operators take exactly two values to compare");
	    return -1;
	    }

	/** Evaluate the children first **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = -1;
	i0 = ((pExpression)(tree->Children.Items[0]));
	i1 = ((pExpression)(tree->Children.Items[1]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;
	if (exp_internal_EvalTree(i1,objlist) <0) return -1;

	/** Get a data ptr to the 2nd data type value **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: dptr0 = &(i0->Integer); break;
	    case DATA_T_STRING: dptr0 = i0->String; break;
	    case DATA_T_DOUBLE: dptr0 = &(i0->Types.Double); break;
	    case DATA_T_MONEY: dptr0 = &(i0->Types.Money); break;
	    case DATA_T_DATETIME: dptr0 = &(i0->Types.Date); break;
	    case DATA_T_INTVEC: dptr0 = &(i0->Types.IntVec); break;
	    case DATA_T_STRINGVEC: dptr0 = &(i0->Types.StrVec); break;
	    default:
		mssError(1,"EXP","Unexpected data type in LHS of comparison: %d", i0->DataType);
		return -1;
	    }

	/** Get a data ptr to the 2nd data type value **/
	switch(i1->DataType)
	    {
	    case DATA_T_INTEGER: dptr1 = &(i1->Integer); break;
	    case DATA_T_STRING: dptr1 = i1->String; break;
	    case DATA_T_DOUBLE: dptr1 = &(i1->Types.Double); break;
	    case DATA_T_MONEY: dptr1 = &(i1->Types.Money); break;
	    case DATA_T_DATETIME: dptr1 = &(i1->Types.Date); break;
	    case DATA_T_INTVEC: dptr1 = &(i1->Types.IntVec); break;
	    case DATA_T_STRINGVEC: dptr1 = &(i1->Types.StrVec); break;
	    default:
		mssError(1,"EXP","Unexpected data type in RHS of comparison: %d", i1->DataType);
		return -1;
	    }

	/** Compare as strings or integers **/
	if ((i0->Flags | i1->Flags) & EXPR_F_NULL) 
	    {
	    tree->Integer = 0;
	    }
	else
	    {
	    v = objDataCompare(i0->DataType, dptr0, i1->DataType, dptr1);
	    if (v == -2) 
		{
		mssError(1,"EXP","Invalid datatypes in comparison");
		return -1;
		}
	    tree->Integer = 
		((tree->CompareType & MLX_CMP_EQUALS) && v==0) ||
		((tree->CompareType & MLX_CMP_GREATER) && v>0) ||
		((tree->CompareType & MLX_CMP_LESS) && v<0);
	    }

    return tree->Integer;
    }


/*** expRevEvalCompare - reverse-evaluate a comparison node.
 ***/
int
expRevEvalCompare(pExpression tree, pParamObjects objlist)
    {
    pExpression const_node = NULL, obj_node = NULL;
    pExpression subtree;
    int i,id;

    	/** We have to have a constant node and object node. **/
	for(i=0;i<2;i++)
	    {
	    subtree = (pExpression)(tree->Children.Items[i]);
	    id = expObjID(subtree,objlist);
	    if (id != EXPR_CTL_CONSTANT && (subtree->NodeType == EXPR_N_OBJECT || subtree->NodeType == EXPR_N_PROPERTY) && (objlist->Flags[id] & EXPR_O_UPDATE))
	        {
		if (obj_node) return 0;
		obj_node = subtree;
		}
	    else
	        {
		if (const_node) return 0;
		const_node = subtree;
		}
	    }
	if (!const_node || !obj_node) return 0;

	/** Ok, if EQUAL and result was TRUE, pass value to obj **/
	/** OR if NOT-EQUAL and result was FALSE, pass the value. **/
	if ((tree->Integer != 0 && tree->CompareType == MLX_CMP_EQUALS) ||
	    (tree->Integer == 0 && tree->CompareType == (MLX_CMP_GREATER | MLX_CMP_LESS)))
	    {
	    if (exp_internal_EvalTree(const_node, objlist) < 0) return -1;
	    obj_node->Integer = const_node->Integer;
	    if (obj_node->Alloc && obj_node->String)
	        {
		obj_node->Alloc = 0;
		nmSysFree(obj_node->String);
		}
	    obj_node->String = const_node->String;
	    obj_node->DataType = const_node->DataType;
	    if (obj_node->DataType == DATA_T_MONEY || obj_node->DataType == DATA_T_DATETIME || obj_node->DataType == DATA_T_DOUBLE)
	        memcpy(&(obj_node->Types), &(const_node->Types), sizeof(obj_node->Types));
	    return expReverseEvalTree(obj_node, objlist);
	    }

    return 0;
    }


/*** expEvalObject - evaluates an object node, which right
 *** now does nothing except replicate its child.
 ***/
int
expEvalObject(pExpression tree, pParamObjects objlist)
    {
    pExpression i0;

	/** Verify child **/
	if (tree->Children.nItems != 1) return -1;

	/** Evaluate child **/
	i0 = ((pExpression)(tree->Children.Items[0]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;

	/** Copy child's type and data. **/
	tree->DataType = i0->DataType;
	if (i0->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: tree->Integer = i0->Integer; break;
	    case DATA_T_STRING: tree->String = i0->String; tree->Alloc = 0; break;
	    default: memcpy(&(tree->Types), &(i0->Types), sizeof(tree->Types)); break;
	    }

    return 0;
    }


/*** expRevEvalObject - reverse-evaluates an object node, which
 *** means we need to set a property value.
 ***/
int
expRevEvalObject(pExpression tree, pParamObjects objlist)
    {
    pExpression subtree;

	/** Copy data to the child, then eval it. **/
	subtree = (pExpression)(tree->Children.Items[0]);
	if (tree->Flags & EXPR_F_NULL) 
	    subtree->Flags |= EXPR_F_NULL;
	else
	    subtree->Flags &= ~EXPR_F_NULL;
	switch(tree->DataType)
	    {
	    case DATA_T_INTEGER: subtree->Integer = tree->Integer; break;
	    case DATA_T_STRING:
	        if (subtree->Alloc && subtree->String)
	            {
	            nmSysFree(subtree->String);
	            }
	        subtree->Alloc = 0;
	        subtree->String = tree->String;
		break;
	    default: memcpy(&(subtree->Types), &(tree->Types), sizeof(tree->Types)); break;
	    }
	subtree->DataType = tree->DataType;

    return expReverseEvalTree(subtree, objlist);
    }


/*** expEvalProperty - evaluates the property of the current
 *** object node.
 ***/
int
expEvalProperty(pExpression tree, pParamObjects objlist)
    {
    int t,v=-1,n, id = 0;
    pObject obj;
    char* ptr;
    void* vptr;
    int (*getfn)();

    	/** Which object are we getting at? **/
	if (tree->ObjID == -1)
	    {
	    /** If unset, but direct objsys reference using pathname, look it up **/
	    if (tree->Parent->Name[0] == '.' || tree->Parent->Name[0] == '/')
		{
		obj = objOpen(objlist->Session, tree->Parent->Name, O_RDONLY, 0600, "system/object");
		if (!obj) 
		    {
		    mssError(0,"EXP","Could not open object within expression");
		    return -1;
		    }
		getfn = objGetAttrValue;
		}
	    else
		{
		/** if unset because unbound to a real object, evaluate to NULL **/
		tree->Flags |= EXPR_F_NULL;
		tree->DataType = DATA_T_INTEGER;
		return 0;
		}
	    }
	else
	    {
	    id = expObjID(tree,objlist);
	    if (id == EXPR_CTL_CONSTANT) return 0;
	    obj = objlist->Objects[id];
	    getfn = objlist->GetAttrFn[id];
	    }

	/** If no object, set result to NULL. **/
	if (!obj)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    tree->DataType = DATA_T_INTEGER;
	    return 0;
	    }

	/** Need to release string? **/
	if (tree->Alloc == 1 && tree->String) nmSysFree(tree->String);
	tree->Alloc=0; 

	/** Figure out the property's type **/
	if (tree->ObjID == -1)
	    t = objGetAttrType(obj,tree->Name);
	else
	    t = objlist->GetTypeFn[id](obj,tree->Name);
	tree->DataType = t;
	switch(t)
	    {
	    case DATA_T_INTEGER: 
	        v = getfn(obj,tree->Name,DATA_T_INTEGER,&(tree->Integer));
		break;

	    case DATA_T_STRING: 
	        v = getfn(obj,tree->Name,DATA_T_STRING,&(tree->String));
		if (v != 0) break;
		n = strlen(tree->String);
		if (n < 64) 
		    {
		    strcpy(tree->Types.StringBuf, tree->String);
		    tree->String = tree->Types.StringBuf;
		    tree->Alloc=0; 
		    }
		else
		    {
		    tree->Alloc=1;
		    ptr = tree->String;
		    tree->String = (char*)nmSysMalloc(n+1);
		    strcpy(tree->String,ptr);
		    }
		break;

	    case DATA_T_DOUBLE:
	        v = getfn(obj,tree->Name,DATA_T_DOUBLE,&(tree->Types.Double));
		if (v != 0) break;
		break;

	    case DATA_T_DATETIME:
	        v = getfn(obj,tree->Name,DATA_T_DATETIME,&vptr);
		if (v != 0) break;
		memcpy(&(tree->Types.Date),vptr,sizeof(DateTime));
		break;

	    case DATA_T_MONEY:
	        v = getfn(obj,tree->Name,DATA_T_MONEY,&vptr);
		if (v != 0) break;
		memcpy(&(tree->Types.Money), vptr, sizeof(MoneyType));
		break;

	    case DATA_T_UNAVAILABLE:
	        tree->DataType = DATA_T_INTEGER;
		v = 1;
		break;

	    default: 
	        if (tree->ObjID == -1) objClose(obj);
	        return -1;
	    }

	/** Check null field **/
        if (tree->ObjID == -1) objClose(obj);
	if (v < 0) 
	    {
	    mssError(1,"EXP","Error accessing object attribute in expression");
	    return -1;
	    }
	if (v == 1) tree->Flags |= EXPR_F_NULL;

    return 0;
    }


/*** expRevEvalProperty - reverse evaluate a property node.  This means
 *** we, if the datatype matches, set the object's property value to
 *** whatever's in the tree node.
 ***/
int
expRevEvalProperty(pExpression tree, pParamObjects objlist)
    {
    pObject obj;
    int attr_type;
    pDateTime dtptr;
    DateTime dt;
    pMoneyType mptr;
    MoneyType m;
    int (*setfn)();
    int id;

    	/** Which object are we getting at? **/
	id = expObjID(tree,objlist);
	if (id == EXPR_CTL_CONSTANT) return 0;
	obj = objlist->Objects[id];
	setfn = objlist->SetAttrFn[id];

    	/** Verify data type match. **/
	dtptr = &(tree->Types.Date);
	mptr = &(tree->Types.Money);
	attr_type = objlist->GetTypeFn[id](obj,tree->Name);
	if (tree->DataType != attr_type)
	    {
	    if (tree->DataType == DATA_T_STRING && attr_type == DATA_T_DATETIME)
	        {
		objDataToDateTime(DATA_T_STRING, tree->String, &dt, NULL);
		dtptr = &dt;
		}
	    else if (tree->DataType == DATA_T_INTEGER && attr_type == DATA_T_DOUBLE)
	        {
		tree->Types.Double = tree->Integer;
		}
	    else if (tree->DataType == DATA_T_INTEGER && attr_type == DATA_T_MONEY)
	        {
		tree->Types.Money.WholePart = tree->Integer;
		tree->Types.Money.FractionPart = 0;
		}
	    else if (tree->DataType == DATA_T_DOUBLE && attr_type == DATA_T_MONEY)
	        {
		mptr = &m;
		objDataToMoney(DATA_T_DOUBLE, &(tree->Types.Double), &m);
		}
	    else
	        {
		mssError(1,"EXP","Bark!  Unhandled type conversion in expRevEvalProperty (%s->%s)", 
			obj_type_names[tree->DataType],obj_type_names[attr_type]);
		return -1;
		}
	    }

	/** Ok, set the value **/
	switch(attr_type)
	    {
	    case DATA_T_INTEGER:
	        setfn(obj,tree->Name,DATA_T_INTEGER,&(tree->Integer));
	        break;

	    case DATA_T_STRING:
	        setfn(obj,tree->Name,DATA_T_STRING,&(tree->String));
	        break;

	    case DATA_T_DATETIME:
	        setfn(obj,tree->Name,DATA_T_DATETIME,&dtptr);
		break;

	    case DATA_T_MONEY:
	        setfn(obj,tree->Name,DATA_T_MONEY,&mptr);
		break;

	    case DATA_T_DOUBLE:
	        setfn(obj,tree->Name, DATA_T_DOUBLE, &(tree->Types.Double));
		break;

	    default:
		mssError(1,"EXP","Bark!  Unhandled data type in expRevEvalProperty (%s)", obj_type_names[attr_type]);
		return -1;
	    }

    return 0;
    }


/*** expEvalFunction - evaluate a function node.  Determine the function
 *** type and evaluate it as appropriate.
 ***/
int
expEvalFunction(pExpression tree, pParamObjects objlist)
    {
    pExpression i0 = NULL, i1 = NULL, i2 = NULL;
    int i;
    int (*fn)();

    	/** Pick up the first two params... **/
	if (tree->Children.nItems > 0) i0 = (pExpression)(tree->Children.Items[0]);
	if (tree->Children.nItems > 1) i1 = (pExpression)(tree->Children.Items[1]);
	if (tree->Children.nItems > 2) i2 = (pExpression)(tree->Children.Items[2]);

	/** Evaluate all child items. **/
	for(i=0;i<tree->Children.nItems;i++) 
	    {
	    if (exp_internal_EvalTree((pExpression)(tree->Children.Items[i]),objlist) < 0)
	        {
		return -1;
		}
	    }

	/** Get and call the evaluator function. **/
	fn = (int(*)())xhLookup(&EXP.Functions,tree->Name);
	if (!fn)
	    {
	    mssError(1,"EXP","Unknown function name '%s' in expression",tree->Name);
	    return -1;
	    }

    return fn(tree,objlist,i0,i1,i2);
    }


/*** expRevEvalFunction - attempt to reverse-evaluate a function node,
 *** depending on the type of function.
 ***/
int
expRevEvalFunction(pExpression tree, pParamObjects objlist)
    {
    return 0;
    }


/*** expEvalNot - evaluate a NOT node -- this just negates the one 
 *** child item.
 ***/
int
expEvalNot(pExpression tree, pParamObjects objlist)
    {
    pExpression i0;

    	/** Get the child item **/
	i0 = (pExpression)(tree->Children.Items[0]);
	if (exp_internal_EvalTree(i0,objlist) < 0) return -1;

	/** If NULL or STRING, this is null too. **/
	if ((i0->Flags & EXPR_F_NULL) || i0->DataType == DATA_T_STRING) 
	    {
	    tree->Flags |= EXPR_F_NULL;
	    }

	/** Otherwise, pass on 1 if 0 and 0 if non-0 **/
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = !(i0->Integer);

    return 0;
    }


/*** expRevEvalNot - reverse-evaluate a NOT node -- this just passes
 *** the given value to the child, negated.
 ***/
int
expRevEvalNot(pExpression tree, pParamObjects objlist)
    {
    pExpression i0;

    	/** Get the child item **/
	i0 = (pExpression)(tree->Children.Items[0]);
	if (tree->Flags & EXPR_F_NULL) i0->Flags |= EXPR_F_NULL;
	i0->Integer = !(tree->Integer);
	expReverseEvalTree(i0,objlist);

    return 0;
    }


/*** expEvalIn - evaluate an IN node by checking to see if the first 
 *** argument is in the list specified as the second argument, or is
 *** equal to the second argument if arg2 isn't a list.
 ***/
int
expEvalIn(pExpression tree, pParamObjects objlist)
    {
    pExpression i0,i1,itmp;
    void* dptr0;
    void* dptr1;
    int i,v;

    	/** Is this just a straight compare with one item? **/
	i1 = (pExpression)(tree->Children.Items[1]);
	if (i1->NodeType != EXPR_N_LIST)
	    {
	    tree->CompareType = MLX_CMP_EQUALS;
	    return expEvalCompare(tree,objlist);
	    }

    	/** Get the main item. **/
	i0 = (pExpression)(tree->Children.Items[0]);
	if (exp_internal_EvalTree(i0,objlist) < 0) return -1;
	if (i0->Flags & EXPR_F_NULL)
	    {
	    tree->Flags |= EXPR_F_NULL;
	    tree->DataType = DATA_T_INTEGER;
	    tree->Integer = 0;
	    return 0;
	    }

	/** Get a data ptr to the data type value **/
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: dptr0 = &(i0->Integer); break;
	    case DATA_T_STRING: dptr0 = i0->String; break;
	    case DATA_T_DOUBLE: dptr0 = &(i0->Types.Double); break;
	    case DATA_T_MONEY: dptr0 = &(i0->Types.Money); break;
	    case DATA_T_DATETIME: dptr0 = &(i0->Types.Date); break;
	    case DATA_T_INTVEC: dptr0 = &(i0->Types.IntVec); break;
	    case DATA_T_STRINGVEC: dptr0 = &(i0->Types.StrVec); break;
	    default:
		mssError(1,"EXP","Unexpected data type in LHS of IN operator: %d", i0->DataType);
		return -1;
	    }

	/** Loop through the items in the list **/
	for(i=0;i<i1->Children.nItems;i++)
	    {
	    itmp = (pExpression)(i1->Children.Items[i]);
	    if (exp_internal_EvalTree(itmp,objlist) < 0) return -1;
	    switch(itmp->DataType)
	        {
	        case DATA_T_INTEGER: dptr1 = &(itmp->Integer); break;
	        case DATA_T_STRING: dptr1 = itmp->String; break;
	        case DATA_T_DOUBLE: dptr1 = &(itmp->Types.Double); break;
	        case DATA_T_MONEY: dptr1 = &(itmp->Types.Money); break;
	        case DATA_T_DATETIME: dptr1 = &(itmp->Types.Date); break;
	        case DATA_T_INTVEC: dptr1 = &(itmp->Types.IntVec); break;
	        case DATA_T_STRINGVEC: dptr1 = &(itmp->Types.StrVec); break;
		default:
		    mssError(1,"EXP","Unexpected data type in list item #%d of IN operator: %d", i, itmp->DataType);
		    return -1;
		}
	    v = objDataCompare(i0->DataType, dptr0, itmp->DataType, dptr1);
	    if (v == -2)
	        {
		mssError(1,"EXP","Invalid datatypes with IN (...) comparison");
		return -1;
		}
	    if (v == 0)
	        {
		tree->DataType = DATA_T_INTEGER;
		tree->Integer = 1;
		return 0;
		}
	    }
	tree->DataType = DATA_T_INTEGER;
	tree->Integer = 0;

    return 0;
    }


/*** expEvalList - evaluate a list.  This involves simply copying one of
 *** the list item values, whichever one is specified to be copied depending
 *** on the value of AggCount (we're overloading that value).
 ***/
int
expEvalList(pExpression tree, pParamObjects objlist)
    {
    pExpression i0;

	/** Verify non-empty list **/
	if (tree->Children.nItems == 0) return -1;

	/** Evaluate child **/
	i0 = ((pExpression)(tree->Children.Items[tree->AggCount]));
	if (exp_internal_EvalTree(i0,objlist) <0) return -1;

	/** Copy child's type and data. **/
	tree->DataType = i0->DataType;
	if (i0->Flags & EXPR_F_NULL) tree->Flags |= EXPR_F_NULL;
	switch(i0->DataType)
	    {
	    case DATA_T_INTEGER: tree->Integer = i0->Integer; break;
	    case DATA_T_STRING: tree->String = i0->String; tree->Alloc = 0; break;
	    default: memcpy(&(tree->Types), &(i0->Types), sizeof(tree->Types)); break;
	    }

    return 0;
    }


/*** expEvalTree - evalute an expression
 ***/
int
expEvalTree(pExpression tree, pParamObjects objlist)
    {
    int i,v,c;

    	/** Flag set to reset aggregate values? **/
	if (tree->Flags & EXPR_F_DORESET) 
	    {
	    exp_internal_ResetAggregates(tree,-1);
	    tree->Flags &= ~EXPR_F_DORESET;
	    /*expEvalTree(tree,objlist);*/
	    }

    	/** Determine modified-object coverage mask **/
	if (objlist)
	    {
	    if (objlist == expNullObjlist) objlist->MainFlags |= EXPR_MO_RECALC;
	    objlist->CurControl = tree->Control;
	    objlist->ModCoverageMask = 0;
	    if (tree->PSeqID != objlist->PSeqID) 
		{
		objlist->ModCoverageMask = 0xFFFFFFFF;
		}
	    else
		{
		for(i=0;i<objlist->nObjects;i++) if (objlist->SeqIDs[i] > tree->SeqID)
		    {
		    if (tree->Control)
			{
			for(c=0;c<EXPR_MAX_PARAMS;c++) if (tree->Control->ObjMap[c] == i)
			    {
			    objlist->ModCoverageMask |= (1<<c);
			    }
			}
		    else
			{
			objlist->ModCoverageMask |= (1<<i);
			}
		    }
		}
	    }

	/** Evaluate the thing. **/
	v = exp_internal_EvalTree(tree,objlist);

	/** Update sequence ids on the expression. **/
	if (objlist)
	    {
	    tree->PSeqID = objlist->PSeqID;
	    tree->SeqID = objlist->SeqID;
	    objlist->MainFlags &= ~EXPR_MO_RECALC;
	    }

    return v;
    }


/*** exp_internal_EvalTree - evaluate an expression tree against a
 *** given object, which will be used whenever an EXPR_N_OBJECT type
 *** node is encountered.
 ***/
int
exp_internal_EvalTree(pExpression tree, pParamObjects objlist)
    {
    int (*fn)();

    	/** If node is frozen, return OK **/
	if (tree->Flags & EXPR_F_FREEZEEVAL) return 0;

#if 00
    	/** If node is NOT stale, return OK immediately. **/
	if (!(objlist && (objlist->MainFlags & EXPR_MO_RECALC)) && !(tree->Flags & EXPR_F_NEW)) return 0;
	tree->Flags &= ~EXPR_F_NEW;
#endif
	
	/** If node is NOT stale and NOT in modified cov mask, return now. **/
	if (!(tree->Flags & (EXPR_F_NEW)) && !(tree->ObjCoverageMask & objlist->ModCoverageMask) && tree->AggLevel==0 && !(objlist->MainFlags & EXPR_MO_RECALC)) return 0;
	tree->Flags &= ~EXPR_F_NEW;

	/** Call the appropriate evaluator fn based on type **/
	if (!(tree->Flags & EXPR_F_PERMNULL)) tree->Flags &= ~EXPR_F_NULL;
	/*if (tree->NodeType == EXPR_N_LIST) return -1;*/
	fn = EXP.EvalFunctions[tree->NodeType];
	if (!fn) return 0;
	return fn(tree,objlist);

#if 00
	switch(tree->NodeType)
	    {
	    case EXPR_N_INTEGER: return 0;
	    case EXPR_N_STRING: return 0;
	    case EXPR_N_DOUBLE: return 0;
	    case EXPR_N_MONEY: return 0;
	    case EXPR_N_DATETIME: return 0;
	    case EXPR_N_PLUS: return expEvalPlus(tree,objlist);
	    case EXPR_N_MINUS: return expEvalMinus(tree,objlist);
	    case EXPR_N_DIVIDE: return expEvalDivide(tree,objlist);
	    case EXPR_N_MULTIPLY: return expEvalMultiply(tree,objlist);
	    case EXPR_N_NOT: return expEvalNot(tree,objlist);
	    case EXPR_N_AND: return expEvalAnd(tree,objlist);
	    case EXPR_N_OR: return expEvalOr(tree,objlist);
	    case EXPR_N_COMPARE: return expEvalCompare(tree,objlist);
	    case EXPR_N_OBJECT: return expEvalObject(tree,objlist);
	    case EXPR_N_PROPERTY: return expEvalProperty(tree,objlist);
	    case EXPR_N_ISNULL: return expEvalIsNull(tree,objlist);
	    case EXPR_N_FUNCTION: return expEvalFunction(tree,objlist);
	    case EXPR_N_CONTAINS: return expEvalContains(tree,objlist);
	    case EXPR_N_LIST: return -1;
	    case EXPR_N_IN: return expEvalIn(tree,objlist);
	    default: return -1;
	    }
#endif

    }


/*** expReverseEval - evaluates an expression tree in reverse.  This
 *** means that given an expression and a certain result that the
 *** expression should evaluate to, we set any object attributes
 *** that are forced.  For example if the expression is "where
 *** :id = 5", and the result is 'true', we can set the 'id' 
 *** attribute to 5.  If the expression were "where :id != 5", then
 *** we would do nothing, since id could be any number except 5, and
 *** we aren't going to try and guess it.  The answer that the
 *** expression should evaluate to should be set in the 'tree'
 *** structure's top level.
 ***/
int
expReverseEvalTree(pExpression tree, pParamObjects objlist)
    {

    	/** Call the evaluator based on the type of node **/
	switch(tree->NodeType)
	    {
	    case EXPR_N_INTEGER: return 0;
	    case EXPR_N_STRING: return 0;
	    case EXPR_N_DOUBLE: return 0;
	    case EXPR_N_MONEY: return 0;
	    case EXPR_N_DATETIME: return 0;
	    case EXPR_N_PLUS: return 0;
	    case EXPR_N_MULTIPLY: return 0;
	    case EXPR_N_NOT: return expRevEvalNot(tree,objlist);
	    case EXPR_N_AND: return expRevEvalAnd(tree,objlist);
	    case EXPR_N_OR: return expRevEvalOr(tree,objlist);
	    case EXPR_N_COMPARE: return expRevEvalCompare(tree,objlist);
	    case EXPR_N_OBJECT: return expRevEvalObject(tree,objlist);
	    case EXPR_N_PROPERTY: return expRevEvalProperty(tree,objlist);
	    case EXPR_N_ISNULL: return expRevEvalIsNull(tree,objlist);
	    case EXPR_N_FUNCTION: return expRevEvalFunction(tree,objlist);
	    case EXPR_N_CONTAINS: return 0;
	    case EXPR_N_LIST: return 0;
	    case EXPR_N_IN: return 0;
	    default: return -1;
	    }

    return 0;
    }


int
exp_internal_DefineNodeEvals()
    {

	/** Init function list **/
	EXP.EvalFunctions[EXPR_N_FUNCTION] = expEvalFunction;
	EXP.EvalFunctions[EXPR_N_MULTIPLY] = expEvalMultiply;
	EXP.EvalFunctions[EXPR_N_DIVIDE] = expEvalDivide;
	EXP.EvalFunctions[EXPR_N_PLUS] = expEvalPlus;
	EXP.EvalFunctions[EXPR_N_MINUS] = expEvalMinus;
	EXP.EvalFunctions[EXPR_N_COMPARE] = expEvalCompare;
	EXP.EvalFunctions[EXPR_N_IN] = expEvalIn;
	EXP.EvalFunctions[EXPR_N_LIKE] = NULL;
	EXP.EvalFunctions[EXPR_N_CONTAINS] = NULL;
	EXP.EvalFunctions[EXPR_N_ISNULL] = expEvalIsNull;
	EXP.EvalFunctions[EXPR_N_NOT] = expEvalNot;
	EXP.EvalFunctions[EXPR_N_AND] = expEvalAnd;
	EXP.EvalFunctions[EXPR_N_OR] = expEvalOr;
	EXP.EvalFunctions[EXPR_N_TRUE] = NULL;
	EXP.EvalFunctions[EXPR_N_FALSE] = NULL;
	EXP.EvalFunctions[EXPR_N_INTEGER] = NULL;
	EXP.EvalFunctions[EXPR_N_STRING] = NULL;
	EXP.EvalFunctions[EXPR_N_DOUBLE] = NULL;
	EXP.EvalFunctions[EXPR_N_DATETIME] = NULL;
	EXP.EvalFunctions[EXPR_N_MONEY] = NULL;
	EXP.EvalFunctions[EXPR_N_OBJECT] = expEvalObject;
	EXP.EvalFunctions[EXPR_N_PROPERTY] = expEvalProperty;
	EXP.EvalFunctions[EXPR_N_LIST] = expEvalList;

	/** Operator precedence list **/
	EXP.Precedence[EXPR_N_MULTIPLY] = 10;
	EXP.Precedence[EXPR_N_DIVIDE] = 10;
	EXP.Precedence[EXPR_N_PLUS] = 20;
	EXP.Precedence[EXPR_N_MINUS] = 20;
	EXP.Precedence[EXPR_N_COMPARE] = 30;
	EXP.Precedence[EXPR_N_IN] = 30;
	EXP.Precedence[EXPR_N_LIKE] = 30;
	EXP.Precedence[EXPR_N_CONTAINS] = 30;
	EXP.Precedence[EXPR_N_ISNULL] = 30;
	EXP.Precedence[EXPR_N_NOT] = 40;
	EXP.Precedence[EXPR_N_AND] = 50;
	EXP.Precedence[EXPR_N_OR] = 60;
	EXP.Precedence[EXPR_N_TRUE] = 100;
	EXP.Precedence[EXPR_N_FALSE] = 100;
	EXP.Precedence[EXPR_N_INTEGER] = 100;
	EXP.Precedence[EXPR_N_STRING] = 100;
	EXP.Precedence[EXPR_N_DOUBLE] = 100;
	EXP.Precedence[EXPR_N_DATETIME] = 100;
	EXP.Precedence[EXPR_N_MONEY] = 100;
	EXP.Precedence[EXPR_N_OBJECT] = 100;
	EXP.Precedence[EXPR_N_PROPERTY] = 100;
	EXP.Precedence[EXPR_N_FUNCTION] = 200;
	EXP.Precedence[EXPR_N_LIST] = 200;

    return 0;
    }
