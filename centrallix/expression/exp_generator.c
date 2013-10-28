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
/* Module:	expression.h, exp_generator.c                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 1, 2001                                         */
/*									*/
/* Description:	This is a new expression subsystem module which allows	*/
/*		an expression to be converted from an expression tree	*/
/*		back to its textual representation.			*/
/************************************************************************/



/*** Structure for handling the expression generation. ***/
typedef struct _EG
    {
    pParamObjects   Objlist;
    int		    (*WriteFn)();
    void*	    WriteArg;
    char	    EscChar;
    int		    Domain;
    char	    TmpBuf[256];
    }
    ExpGen, *pExpGen;


/*** exp_internal_WriteText - writes text through the write_fn, but checks
 *** for escaping of strings and such, as needed.
 ***/
int
exp_internal_WriteText(pExpGen eg, char* text)
    {
    int n = strlen(text);
    char* buf;
    char* ptr;

	/** No escaping?  Then just write it. **/
	if (!eg->EscChar)
	    {
	    eg->WriteFn(eg->WriteArg, text, n, 0, 0);
	    return 0;
	    }

	/** Allocate a xfer buffer large enough **/
	buf = (char*)nmSysMalloc(2*n + 1);
	ptr = buf;

	/** Move the contents of 'text' to 'buf', escaping as needed **/
	while(*text)
	    {
	    if (*text == eg->EscChar || *text == '\\')
	        {
		*(ptr++) = '\\';
		}
	    if (eg->EscChar == '\\' && (*text == '\r' || *text == '\n' || *text == '\t'))
		{
		*(ptr++) = '\\';
		switch(*text)
		    {
		    case '\r': *(ptr++) = 'r'; break;
		    case '\n': *(ptr++) = 'n'; break;
		    case '\t': *(ptr++) = 't'; break;
		    }
		text++;
		continue;
		}
	    *(ptr++) = *(text++);
	    }
	*ptr = '\0';

	/** Write it out. **/
	eg->WriteFn(eg->WriteArg, buf, ptr-buf, 0, 0);

	/** Free the tmp buffer **/
	nmSysFree(buf);

    return 0;
    }


/*** exp_internal_CheckConstants() - from the domain declaration and target
 *** for the expression generation, figure out if the node should be treated
 *** as a constant.
 ***/
int
exp_internal_CheckConstants(pExpression exp, pExpGen eg)
    {
    int nodetype = exp->NodeType;
    int n;

	if ((eg->Domain == EXPR_F_RUNCLIENT && ((exp->Flags & EXPR_F_RUNSERVER) || (exp->Flags & EXPR_F_RUNSTATIC) || (exp->Flags & EXPR_F_DOMAINMASK) == 0)) ||
	    (eg->Domain == EXPR_F_RUNSERVER && ((exp->Flags & EXPR_F_RUNSTATIC) || (exp->Flags & EXPR_F_DOMAINMASK) == 0)) ||
	    (exp->Flags & EXPR_F_FREEZEEVAL))
	    {
	    if (exp->Flags & EXPR_F_NULL)
		nodetype = 0;
	    else if ((n = expDataTypeToNodeType(exp->DataType)) >= 0)
		nodetype = n;
	    }

    return nodetype;
    }


/*** exp_internal_GenerateText_cxsql - internal recursive version to do the
 *** work needed by the below function.
 ***/
int
exp_internal_GenerateText_cxsql(pExpression exp, pExpGen eg)
    {
    int i;
    int nodetype;
    int id;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"EXP","Failed to generate CXSQL expression: resource exhaustion occurred");
	    return -1;
	    }

	/** Do we have a domain declaration?  Add the pseudo-function for it if so **/
	if (exp->Flags & EXPR_F_DOMAINMASK)
	    {
	    if (exp->Flags & EXPR_F_RUNSTATIC)
		exp_internal_WriteText(eg, "runstatic(");
	    else if (exp->Flags & EXPR_F_RUNSERVER)
		exp_internal_WriteText(eg, "runserver(");
	    else
		exp_internal_WriteText(eg, "runclient(");
	    }

	/** Treat some expressions as constants **/
	nodetype = exp_internal_CheckConstants(exp, eg);

	/** Select an expression type **/
	switch(nodetype)
	    {
	    case EXPR_N_FUNCTION:
	        /** Function node - write function call, param list, end paren. **/
	        sprintf(eg->TmpBuf,"%.250s(",exp->Name);
		exp_internal_WriteText(eg, eg->TmpBuf);
		for(i=0;i<exp->Children.nItems;i++)
		    {
		    if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[i]), eg) < 0) return -1;
		    if (i != exp->Children.nItems-1)
		        {
		        exp_internal_WriteText(eg, ",");
			}
		    }
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_LIST:
	        /** List node - write paren, list items, end paren **/
		exp_internal_WriteText(eg, "(");
		for(i=0;i<exp->Children.nItems;i++)
		    {
		    if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[i]), eg) < 0) return -1;
		    if (i != exp->Children.nItems-1)
		        {
		        exp_internal_WriteText(eg, ",");
			}
		    }
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_MULTIPLY:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " * ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_DIVIDE:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " / ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_PLUS:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " + ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_MINUS:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " - ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_COMPARE:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		switch(exp->CompareType)
		    {
		    case (MLX_CMP_EQUALS): exp_internal_WriteText(eg, " == "); break;
		    case (MLX_CMP_GREATER): exp_internal_WriteText(eg, " > "); break;
		    case (MLX_CMP_LESS): exp_internal_WriteText(eg, " < "); break;
		    case (MLX_CMP_LESS | MLX_CMP_EQUALS): exp_internal_WriteText(eg, " <= "); break;
		    case (MLX_CMP_GREATER | MLX_CMP_EQUALS): exp_internal_WriteText(eg, " >= "); break;
		    case (MLX_CMP_GREATER | MLX_CMP_LESS): exp_internal_WriteText(eg, " != "); break;
		    default:
		        mssError(1,"EXP","Generator - invalid compare type in expression.");
			return -1;
		    }
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_IN:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " IN ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_CONTAINS:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " CONTAINS ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_LIKE:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " LIKE ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_ISNOTNULL:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " IS NOT NULL");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_ISNULL:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " IS NULL");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_NOT:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
		exp_internal_WriteText(eg, "NOT ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_AND:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " AND ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_OR:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " OR ");
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_TRUE:
		exp_internal_WriteText(eg, "1");
		break;

	    case EXPR_N_FALSE:
		exp_internal_WriteText(eg, "0");
		break;

	    case EXPR_N_INTEGER:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_INTEGER, &(exp->Integer), 0));
		break;

	    case EXPR_N_STRING:
	        if (eg->EscChar == '"')
		    exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_STRING, exp->String, DATA_F_QUOTED | DATA_F_SINGLE));
		else
		    exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_STRING, exp->String, DATA_F_QUOTED));
		break;

	    case EXPR_N_DOUBLE:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_DOUBLE, &(exp->Types.Double), 0));
		break;

	    case EXPR_N_DATETIME:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_DATETIME, &(exp->Types.Date), DATA_F_QUOTED));
		break;

	    case EXPR_N_MONEY:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_MONEY, &(exp->Types.Money), 0));
		break;
	    
	    case EXPR_N_OBJECT:
	        if (exp->ObjID == -1 && exp->Name) exp_internal_WriteText(eg, exp->Name);
		break;

	    case EXPR_N_PROPERTY:
		id = expObjID(exp, eg->Objlist);
	        switch(id)
		    {
		    case -1: break;
		    case EXPR_OBJID_CURRENT: break;
		    case EXPR_OBJID_PARENT: exp_internal_WriteText(eg, ":"); break;
		    default: 
		        if (id >= 0) 
			    {
			    exp_internal_WriteText(eg, ":");
		            exp_internal_WriteText(eg, eg->Objlist->Names[id]);
			    }
			break;
		    }
		exp_internal_WriteText(eg,":");
		exp_internal_WriteText(eg,exp->Name);
		break;

	    case 0:  /** NULL **/
		exp_internal_WriteText(eg, " NULL ");
		break;

	    default:
	        mssError(1,"EXP","Bark!  Generator - Unknown expression node type %d", exp->NodeType);
		return -1;
	    }

	/** If a domain declaration, add the closing parenthesis **/
	if (exp->Flags & EXPR_F_DOMAINMASK)
	    exp_internal_WriteText(eg,")");

    return 0;
    }


/*** expGenerateText_js - converts an expression tree into JavaScript format
 *** for eventual embedding in a DHTML page.
 ***
 *** Note that the following support functions are *required* for these expressions
 *** to work properly:
 ***
 ***         cxjs_indexof(array,element) - finds an element in an array
 ***         cxjs_likematch(string,patternstring) - do a "LIKE" match comparison
 ***/
int
exp_internal_GenerateText_js(pExpression exp, pExpGen eg)
    {
    int i;
    int prop_func;
    int nodetype;
    int id;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"EXP","Failed to generate JS expression: resource exhaustion occurred");
	    return -1;
	    }

	/** Treat some expressions as constants **/
	nodetype = exp_internal_CheckConstants(exp, eg);

	if ((exp->Flags & EXPR_F_PERMNULL) || nodetype == 0)
	    {
	    exp_internal_WriteText(eg, " null ");
	    return 0;
	    }

	/** Select an expression type **/
	switch(nodetype)
	    {
	    case EXPR_N_FUNCTION:
	        /** Function node - write function call, param list, end paren. **/
		if ((!strcmp(exp->Name,"runclient") || !strcmp(exp->Name,"runserver") || !strcmp(exp->Name,"runstatic")) && exp->Children.nItems == 1)
		    return exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg);
	        snprintf(eg->TmpBuf,sizeof(eg->TmpBuf),"cxjs_%.250s(",exp->Name);
		exp_internal_WriteText(eg, eg->TmpBuf);
		if (!strcmp(exp->Name, "substitute"))
		    {
		    /** This function requires awareness of its object/property scope **/
		    exp_internal_WriteText(eg, "_context,_this,");
		    }
		for(i=0;i<exp->Children.nItems;i++)
		    {
		    if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[i]), eg) < 0) return -1;
		    if (i != exp->Children.nItems-1)
		        {
		        exp_internal_WriteText(eg, ",");
			}
		    }
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_LIST:
	        /** List node - write paren, list items, end paren.  In javascript, this
		 ** amounts to an array, so write it as an array.
		 **/
		exp_internal_WriteText(eg, "(new Array(");
		for(i=0;i<exp->Children.nItems;i++)
		    {
		    if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[i]), eg) < 0) return -1;
		    if (i != exp->Children.nItems-1)
		        {
		        exp_internal_WriteText(eg, ",");
			}
		    }
		exp_internal_WriteText(eg, "))");
		break;

	    case EXPR_N_MULTIPLY:
		/** FIXME: precedence for standard cxsql expressions is not necessarily
		 ** the same as precedence of ops for JavaScript expressions.
		 **/
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " * ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_DIVIDE:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " / ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_PLUS:
		/** rules for string concat are different in javascript and cxsql **/
		exp_internal_WriteText(eg, "cxjs_plus(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ", ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_MINUS:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " - ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_COMPARE:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		switch(exp->CompareType)
		    {
		    case (MLX_CMP_EQUALS): exp_internal_WriteText(eg, " == "); break;
		    case (MLX_CMP_GREATER): exp_internal_WriteText(eg, " > "); break;
		    case (MLX_CMP_LESS): exp_internal_WriteText(eg, " < "); break;
		    case (MLX_CMP_LESS | MLX_CMP_EQUALS): exp_internal_WriteText(eg, " <= "); break;
		    case (MLX_CMP_GREATER | MLX_CMP_EQUALS): exp_internal_WriteText(eg, " >= "); break;
		    case (MLX_CMP_GREATER | MLX_CMP_LESS): exp_internal_WriteText(eg, " != "); break;
		    default:
		        mssError(1,"EXP","Generator - invalid compare type in expression.");
			return -1;
		    }
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_IN:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
		exp_internal_WriteText(eg, " cxjs_indexof(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ",");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ") >= 0");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_CONTAINS:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ".indexOf(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ") >= 0");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_LIKE:
		exp_internal_WriteText(eg, " cxjs_likematch(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ",");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_ISNOTNULL:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " != null");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_ISNULL:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " == null");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_NOT:
		exp_internal_WriteText(eg, "!");
		exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_AND:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " && ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_OR:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " || ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_TRUE:
		exp_internal_WriteText(eg, "true");
		break;

	    case EXPR_N_FALSE:
		exp_internal_WriteText(eg, "false");
		break;

	    case EXPR_N_INTEGER:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_INTEGER, &(exp->Integer), 0));
		break;

	    case EXPR_N_STRING:
	        if (eg->EscChar == '"')
		    exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_STRING, exp->String, DATA_F_QUOTED | DATA_F_SINGLE | DATA_F_CONVSPECIAL));
		else
		    exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_STRING, exp->String, DATA_F_QUOTED | DATA_F_CONVSPECIAL));
		break;

	    case EXPR_N_DOUBLE:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_DOUBLE, &(exp->Types.Double), 0));
		break;

	    case EXPR_N_DATETIME:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_DATETIME, &(exp->Types.Date), DATA_F_QUOTED));
		break;

	    case EXPR_N_MONEY:
	        exp_internal_WriteText(eg, objDataToStringTmp(DATA_T_MONEY, &(exp->Types.Money), 0));
		break;
	    
	    case EXPR_N_OBJECT:
		prop_func = 0;
	        if (exp->Children.nItems != 1) return -1;
	        if (exp->ObjID == -1 && exp->Name) 
		    {
		    if (((pExpression)(exp->Children.Items[0]))->NodeType == EXPR_N_PROPERTY)
			prop_func = 1;
		    if (prop_func)
			exp_internal_WriteText(eg, "wgtrGetProperty(");
		    exp_internal_WriteText(eg, "wgtrGetNode(_context,\"");
		    exp_internal_WriteText(eg, exp->Name);
		    exp_internal_WriteText(eg, "\")");
		    if (prop_func)
			exp_internal_WriteText(eg, ",\"");
		    else
			exp_internal_WriteText(eg, ".");
		    }
		if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		if (prop_func)
		    exp_internal_WriteText(eg, "\")");
		break;

	    case EXPR_N_PROPERTY:
		id = expObjID(exp,eg->Objlist);
	        switch(id)
		    {
		    case -1: break;
		    case EXPR_OBJID_CURRENT: break;
		    case EXPR_OBJID_PARENT: exp_internal_WriteText(eg, "this."); break;
		    default: 
		        if (id)
			    {
			    if (eg->Objlist) 
				exp_internal_WriteText(eg, eg->Objlist->Names[id]);
			    else
				exp_internal_WriteText(eg, exp->Name);
			    }
			break;
		    }
		if (!exp->Parent || exp->Parent->NodeType != EXPR_N_OBJECT)
		    {
		    exp_internal_WriteText(eg,"((typeof _this.");
		    exp_internal_WriteText(eg,exp->Name);
		    exp_internal_WriteText(eg," != 'undefined')?(_this.");
		    exp_internal_WriteText(eg,exp->Name);
		    exp_internal_WriteText(eg,"):null)");
		    }
		else
		    exp_internal_WriteText(eg,exp->Name);
		break;

	    default:
	        mssError(1,"EXP","Bark!  Generator - Unknown expression node type %d", exp->NodeType);
		return -1;
	    }

    return 0;
    }



/*** expGenerateText - converts an expression tree back to its textual
 *** representation.  Generates the text to a given write_fn with a given
 *** context parameter.  Can be used with fdWrite/objWrite, or with a 
 *** custom write function which could, for instance, write the text
 *** into a buffer.  Allows a quote character to be specified; if given,
 *** any generated text matching that character will be prefixed with a
 *** backslash (\); this is useful when the output text will be quoted as
 *** a string.  In this case, any otherwise normally-occurring backslashes
 *** in the output will also be escaped with a backslash.
 ***
 *** Normal values of quote_char are \0, ', and ".
 ***
 *** Currently supported languages: 
 ***
 ***         CXSQL (Centrallix SQL style expressions)
 ***         JavaScript (JavaScript style expressions for DHTML embedding)
 ***
 *** "domain" can be 0 to generate the full expression, or EXPR_F_RUNCLIENT
 *** to constant-ize any runserver() or runstatic() subexpressions.  If the
 *** domain is EXPR_F_RUNSERVER, only any runstatic() subexpressions are
 *** converted to constants.
 ***/
int
expGenerateText(pExpression exp, pParamObjects objlist, int (*write_fn)(), void* write_arg, char quote_char, char* language, int domain)
    {
    pExpGen eg;

	/** Allocate a structure for this **/
	eg = (pExpGen)nmMalloc(sizeof(ExpGen));
	if (!eg) return -1;
	eg->Objlist = objlist;
	eg->WriteFn = write_fn;
	eg->WriteArg = write_arg;
	eg->EscChar = quote_char;
	eg->Domain = domain;
	if (objlist && !objlist->CurControl && exp->Control)
	    objlist->CurControl = exp->Control;

	/** Call the internal recursive version of this function **/
	if (!strcasecmp(language,"cxsql"))
	    {
	    if (exp_internal_GenerateText_cxsql(exp, eg) < 0)
		{
		nmFree(eg,sizeof(ExpGen));
		return -1;
		}
	    }
	else if (!strcmp(language,"javascript"))
	    {
	    if (exp_internal_GenerateText_js(exp, eg) < 0)
		{
		nmFree(eg,sizeof(ExpGen));
		return -1;
		}
	    }
	else
	    {
	    mssError(1,"EXP","Unknown language '%s' requested for expression generation", language);
	    nmFree(eg,sizeof(ExpGen));
	    return -1;
	    }

	/** Free the structure **/
	nmFree(eg,sizeof(ExpGen));

    return 0;
    }


int
exp_internal_AddPropToList(pXArray objs_xa, pXArray props_xa, char* objname, char* propname)
    {
    int i;
    char* objn = NULL;
    char* propn = NULL;

	/** Check dups **/
	for(i=0;i<objs_xa->nItems;i++)
	    {
	    objn = (char*)(objs_xa->Items[i]);
	    propn = (char*)(props_xa->Items[i]);
	    if (((!objn && !objname) || (objn && objname && !strcmp(objn, objname))) &&
		((!propn && !propname) || (propn && propname && !strcmp(propn, propname))))
		return -1;
	    }

	/** add it **/
	if (objname)
	    objn = nmSysStrdup(objname);
	else
	    objn = NULL;
	if (propname)
	    propn = nmSysStrdup(propname);
	else
	    propn = NULL;
	xaAddItem(props_xa, propn);
	xaAddItem(objs_xa, objn);

    return 0;
    }


/*** expGetPropList - get a list of object/property names referenced in a given
 *** expression tree.  Returns the list in a pair of xarrays - one for the
 *** object names, the second with the property names.  The caller must init
 *** the xarrays, and must deinit them afterwards.  The strings referenced in
 *** the list MUST be freed with nmSysFree() by the caller.
 ***/
int
expGetPropList(pExpression exp, pXArray objs_xa, pXArray props_xa)
    {
    pExpression subexp;
    int i;
    char* objn = NULL;
    char* propn = NULL;

	/** Is node an object/property node? **/
	if (exp->NodeType == EXPR_N_OBJECT)
	    {
	    subexp = (pExpression)(exp->Children.Items[0]);
	    objn = exp->Name;
	    if (subexp && subexp->NodeType == EXPR_N_PROPERTY)
		propn = subexp->Name;
	    else
		propn = NULL;
	    exp_internal_AddPropToList(objs_xa, props_xa, objn, propn);
	    }
	else if (exp->NodeType == EXPR_N_FUNCTION && !strcmp(exp->Name, "substitute"))
	    {
	    /** This one could reference almost anything in the namespace **/
	    exp_internal_AddPropToList(objs_xa, props_xa, "*", "*");
	    }
	else if (exp->NodeType == EXPR_N_PROPERTY)
	    {
	    exp_internal_AddPropToList(objs_xa, props_xa, NULL, exp->Name);
	    }
	else
	    {
	    for(i=0;i<exp->Children.nItems;i++) expGetPropList((pExpression)(exp->Children.Items[i]), objs_xa, props_xa);
	    }

    return objs_xa->nItems;
    }

