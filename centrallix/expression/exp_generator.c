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

/**CVSDATA***************************************************************

    $Id: exp_generator.c,v 1.2 2002/06/19 23:29:33 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_generator.c,v $

    $Log: exp_generator.c,v $
    Revision 1.2  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.1  2001/10/02 16:23:09  gbeeley
    Added exp_generator expressiontree-to-text generation module.  Also fixed
    a precedence problem with EXPR_N_FUNCTION nodes; not sure why that wasn't
    causing trouble previously.


 **END-CVSDATA***********************************************************/


/*** Structure for handling the expression generation. ***/
typedef struct _EG
    {
    pParamObjects   Objlist;
    int		    (*WriteFn)();
    void*	    WriteArg;
    char	    EscChar;
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
	    *(ptr++) = *(text++);
	    }
	*ptr = '\0';

	/** Write it out. **/
	eg->WriteFn(eg->WriteArg, buf, ptr-buf, 0, 0);

	/** Free the tmp buffer **/
	nmSysFree(buf);

    return 0;
    }


/*** exp_internal_GenerateText_cxsql - internal recursive version to do the
 *** work needed by the below function.
 ***/
int
exp_internal_GenerateText_cxsql(pExpression exp, pExpGen eg)
    {
    int i;

	/** Select an expression type **/
	switch(exp->NodeType)
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
	        if (exp_internal_GenerateText_cxsql((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
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
	        switch(exp->ObjID)
		    {
		    case -1: break;
		    case EXPR_OBJID_CURRENT: break;
		    case EXPR_OBJID_PARENT: exp_internal_WriteText(eg, ":"); break;
		    default: 
		        if (exp->ObjID >= 0) 
			    {
			    exp_internal_WriteText(eg, ":");
		            exp_internal_WriteText(eg, eg->Objlist->Names[expObjID(exp,eg->Objlist)]);
			    }
			break;
		    }
		exp_internal_WriteText(eg,":");
		exp_internal_WriteText(eg,exp->Name);
		break;

	    default:
	        mssError(1,"EXP","Bark!  Generator - Unknown expression node type %d", exp->NodeType);
		return -1;
	    }

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

	/** Select an expression type **/
	switch(exp->NodeType)
	    {
	    case EXPR_N_FUNCTION:
	        /** Function node - write function call, param list, end paren. **/
	        sprintf(eg->TmpBuf,"%.250s(",exp->Name);
		exp_internal_WriteText(eg, eg->TmpBuf);
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
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " + ");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
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

	    case EXPR_N_ISNULL:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[0]), eg) < 0) return -1;
		exp_internal_WriteText(eg, " == null");
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, ")");
		break;

	    case EXPR_N_NOT:
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
		    exp_internal_WriteText(eg, "(");
		exp_internal_WriteText(eg, "!");
	        if (exp_internal_GenerateText_js((pExpression)(exp->Children.Items[1]), eg) < 0) return -1;
		if (exp->Parent && EXP.Precedence[exp->Parent->NodeType] < EXP.Precedence[exp->NodeType])
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
	        switch(exp->ObjID)
		    {
		    case -1: break;
		    case EXPR_OBJID_CURRENT: break;
		    case EXPR_OBJID_PARENT: exp_internal_WriteText(eg, ":"); break;
		    default: 
		        if (exp->ObjID >= 0) 
			    {
			    exp_internal_WriteText(eg, ":");
		            exp_internal_WriteText(eg, eg->Objlist->Names[expObjID(exp,eg->Objlist)]);
			    }
			break;
		    }
		exp_internal_WriteText(eg,":");
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
 ***/
int
expGenerateText(pExpression exp, pParamObjects objlist, int (*write_fn)(), void* write_arg, char quote_char, char* language)
    {
    pExpGen eg;

	/** Allocate a structure for this **/
	eg = (pExpGen)nmMalloc(sizeof(ExpGen));
	if (!eg) return -1;
	eg->Objlist = objlist;
	eg->WriteFn = write_fn;
	eg->WriteArg = write_arg;
	eg->EscChar = quote_char;

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


