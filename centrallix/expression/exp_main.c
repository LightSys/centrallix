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
#include "magic.h"

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
/* Module: 	expression.h, exp_main.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: exp_main.c,v 1.4 2003/06/27 21:19:47 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_main.c,v $

    $Log: exp_main.c,v $
    Revision 1.4  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

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

    Revision 1.2  2001/09/28 20:03:13  gbeeley
    Updated magic number system syntax to remove the semicolons from within
    the macro expansions.  Semicolons now are (more naturally) placed after
    the macro calls.

    Revision 1.1.1.1  2001/08/13 18:00:48  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** EXP system globals ***/
EXP_Globals EXP;

pParamObjects expNullObjlist = NULL;


/*** expAllocExpression - allocate an expression structure.
 ***/
pExpression
expAllocExpression()
    {
    pExpression expr;

	/** Allocate and initialize **/
	expr = (pExpression)nmMalloc(sizeof(Expression));
	if (!expr) return NULL;
	xaInit(&(expr->Children),4);
	expr->Alloc = 0;
	expr->NameAlloc = 0;
	expr->Parent = NULL;
	expr->Flags = EXPR_F_NEW;
	expr->ObjCoverageMask = 0;
	expr->ObjID = -1;
	expr->AggExp = NULL;
	expr->AggCount = 0;
	expr->Control = NULL;
	expr->ListCount = 1;
	expr->ListTrackCnt = 1;
	expr->Magic = MGK_EXPRESSION;
	expr->LinkCnt = 1;

    return expr;
    }


/*** expLinkExpression - links to an expression.
 ***/
pExpression
expLinkExpression(pExpression this)
    {
    this->LinkCnt++;
    return this;
    }


/*** expFreeExpression - releases an expression's memory.
 ***/
int
expFreeExpression(pExpression this)
    {
    int i;

    	ASSERTMAGIC(this,MGK_EXPRESSION);

	/** Link count check **/
	if ((--this->LinkCnt) > 0) return 0;

	/** Free sub expressions first. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expFreeExpression((pExpression)(this->Children.Items[i]));
	    }

	/** Check to free control block **/
	if (!this->Parent && this->Control) nmFree(this->Control,sizeof(ExpControl));

	/** Free this itself. **/
	xaDeInit(&(this->Children));
	if (this->String && this->Alloc) nmSysFree(this->String);
	if (this->Name && this->NameAlloc) nmSysFree(this->Name);
	if (this->AggExp) expFreeExpression(this->AggExp);
	nmFree(this,sizeof(Expression));

    return 0;
    }


/*** expObjID - return the correct 'mapped' object id for a given objid
 ***/
int
expObjID(pExpression exp, pParamObjects objlist)
    {
    int id;
    if (exp->ObjID == EXPR_OBJID_CURRENT && objlist) id = objlist->CurrentID;
    else if (exp->ObjID == EXPR_OBJID_PARENT && objlist) id = objlist->ParentID;
    else id = exp->ObjID;
    if ((!objlist->CurControl && !exp->Control) || id < 0) return id;
    if (exp->Control) 
        id = exp->Control->ObjMap[id];
    else if (objlist)
        id = objlist->CurControl->ObjMap[id];
    if (id == EXPR_CTL_CURRENT && objlist) return objlist->CurrentID;
    else if (id == EXPR_CTL_PARENT && objlist) return objlist->ParentID;
    else return id;
    }


/*** exp_internal_CopyNode - makes a duplicate of a given node.
 ***/
int
exp_internal_CopyNode(pExpression src, pExpression dst)
    {
    pExpression new_tree = dst;

    	ASSERTMAGIC(src,MGK_EXPRESSION);
    	ASSERTMAGIC(dst,MGK_EXPRESSION);

	/** Copy the static fields **/
	new_tree->NodeType = src->NodeType;
	new_tree->DataType = src->DataType;
	new_tree->CompareType = src->CompareType;
	new_tree->Flags = src->Flags;
	new_tree->Alloc = src->Alloc;
	new_tree->NameAlloc = src->NameAlloc;
	new_tree->Name = src->Name;
	new_tree->String = src->String;
	new_tree->Integer = src->Integer;
	new_tree->ObjID = src->ObjID;
	new_tree->ObjCoverageMask = src->ObjCoverageMask;
	new_tree->ObjOuterMask = src->ObjOuterMask;
	memcpy(&(new_tree->Types), &(src->Types), sizeof(src->Types));

	/** String fields may need to be allocated.. **/
	if (new_tree->Alloc) 
	    {
	    new_tree->String = nmSysStrdup(src->String);
	    }
	if (new_tree->NameAlloc)
	    {
	    new_tree->Name = nmSysStrdup(src->Name);
	    }
	if (src->String == src->Types.StringBuf)
	    {
	    new_tree->String = new_tree->Types.StringBuf;
	    }

    return 0;
    }


/*** exp_internal_CopyTree - make a copy of a whole tree
 ***/
pExpression
exp_internal_CopyTree(pExpression orig_exp)
    {
    pExpression new_exp;
    pExpression subexp;
    pExpression new_subexp;
    int i;

    	ASSERTMAGIC(orig_exp,MGK_EXPRESSION);

    	/** First, copy the current node. **/
	new_exp = expAllocExpression();
	exp_internal_CopyNode(orig_exp, new_exp);

	/** Now, make copies of the subtrees **/
	for(i=0;i<orig_exp->Children.nItems;i++)
	    {
	    subexp = (pExpression)(orig_exp->Children.Items[i]);
	    new_subexp = exp_internal_CopyTree(subexp);
	    xaAddItem(&(new_exp->Children), (void*)new_subexp);
	    new_subexp->Parent = new_exp;
	    }

    return new_exp;
    }


/*** expDuplicateExpression - duplicate an expression tree (as above)
 ***/
pExpression
expDuplicateExpression(pExpression orig_exp)
    {
    return exp_internal_CopyTree(orig_exp);
    }


/*** exp_internal_SplitTree_r - recursive 'heart' of the below function.
 ***/
int
exp_internal_SplitTree_r(pExpression src, pExpression split, pExpression* result[], int level, int cnt)
    {
    pExpression* subtree_result[16];
    pExpression new_trees[16];
    int i,j;

    	ASSERTMAGIC(src,MGK_EXPRESSION);
    	ASSERTMAGIC(split,MGK_EXPRESSION);

    	/** Is this the split point? **/
	if (src == split)
	    {
	    /** Split point -- copy subtrees one to each of the result[]s **/
	    for(i=0;i<cnt;i++)
	        {
		exp_internal_SplitTree_r((pExpression)(src->Children.Items[i]), 
			split, result+i, level+1, 1);
		}
	    }
	else
	    {
	    /** Not split point -- just copy the src tree n times. **/
	    for(i=0;i<cnt;i++)
	        {
		new_trees[i] = *(result[i]) = expAllocExpression();
		exp_internal_CopyNode(src, new_trees[i]);
		for(j=0;j<src->Children.nItems;j++) 
		    xaAddItem(&(new_trees[i])->Children, (void*)NULL);
		}
	    
	    /** Copy each child tree of the src tree n times. **/
	    for(j=0;j<src->Children.nItems;j++)
	        {
		for(i=0;i<cnt;i++) subtree_result[i] = (pExpression*)&(new_trees[i]->Children.Items[j]);
		exp_internal_SplitTree_r((pExpression)(src->Children.Items[i]),
			split, subtree_result, level+1, cnt);
		}
	    }

    return 0;
    }


/*** expSplitTree - makes one or more copies of a tree, splitting the tree at
 *** a given node, optionally.  The split operation copies all of the tree, 
 *** but with only one branch of the subtree substituted in place of the 
 *** original split point node.  For example, splitting (a AND b) OR (c AND d)
 *** at the OR node would give (a AND b) as one tree, and (c AND d) as the
 *** second tree.  Splitting (a AND (b OR c)) at the OR node would give
 *** (a AND (b)) as one tree, and (a AND (c)) as the other one.
 ***/
int
expSplitTree(pExpression src_tree, pExpression split_point, pExpression result_trees[])
    {
    pExpression* result_ptrs[16];
    int i;

    	ASSERTMAGIC(split_point,MGK_EXPRESSION);
    	ASSERTMAGIC(src_tree,MGK_EXPRESSION);

    	/** Setup the result tree ptrs **/
	for(i=0;i<split_point->Children.nItems;i++) result_ptrs[i] = &(result_trees[i]);

    	/** Call the recursive part of the routine. **/
	exp_internal_SplitTree_r(src_tree, split_point, result_ptrs, 0, split_point->Children.nItems);

    return split_point->Children.nItems;
    }


/*** exp_internal_DumpExpression_r - internal function which does the real hard
 *** work for the below.
 ***/
int
exp_internal_DumpExpression_r(pExpression this, int level)
    {
    int i;

    	ASSERTMAGIC(this,MGK_EXPRESSION);

    	/** Print the contents of this expression first **/
	printf("%*.*s",level,level,"");
	switch(this->NodeType)
	    {
	    case EXPR_N_INTEGER: printf("INTEGER = %d", this->Integer); break;
	    case EXPR_N_STRING: printf("STRING = <%s>", this->String); break;
	    case EXPR_N_DOUBLE: printf("DOUBLE = %.2f", this->Types.Double); break;
	    case EXPR_N_MONEY: printf("MONEY = %d.%4.4d", this->Types.Money.WholePart, this->Types.Money.FractionPart); break;
	    case EXPR_N_DATETIME: printf("DATETIME = %s", objDataToStringTmp(DATA_T_DATETIME,&(this->Types.Date), 0)); break;
	    case EXPR_N_PLUS: printf("PLUS"); break;
	    case EXPR_N_MULTIPLY: printf("MULTIPLY"); break;
	    case EXPR_N_NOT: printf("NOT"); break;
	    case EXPR_N_AND: printf("AND"); break;
	    case EXPR_N_OR: printf("OR"); break;
	    case EXPR_N_COMPARE: printf("COMPARE %s%s%s", (this->CompareType & MLX_CMP_LESS)?"<":"",
	    						  (this->CompareType & MLX_CMP_GREATER)?">":"",
							  (this->CompareType & MLX_CMP_EQUALS)?"=":""); break;
	    case EXPR_N_OBJECT: printf("OBJECT ID = %d (%s)", this->ObjID, (this->ObjID == -1)?(this->Name):"-"); break;
	    case EXPR_N_PROPERTY: printf("PROPERTY(%d) = <%s>", this->ObjID, this->Name); break;
	    case EXPR_N_ISNULL: printf("IS NULL"); break;
	    case EXPR_N_FUNCTION: printf("FUNCTION = <%s>", this->Name); break;
	    case EXPR_N_LIST: printf("LIST OF VALUES"); break;
	    }
	printf(", %d child(ren)", this->Children.nItems);
	if (this->Flags & EXPR_F_NULL) 
	    {
	    printf(", NULL");
	    }
	else
	    {
	    switch(this->DataType)
	        {
		case DATA_T_INTEGER: printf(", integer=%d",this->Integer); break;
		case DATA_T_STRING: printf(", string='%s'",this->String); break;
		case DATA_T_DOUBLE: printf(", double=%f",this->Types.Double); break;
		}
	    }
	if (this->Flags & EXPR_F_NEW) printf(", NEW");
	if (this->Flags & EXPR_F_AGGREGATEFN) printf(", AGGFN");
	if (this->Flags & EXPR_F_AGGLOCKED) printf(", AGGLK");
	if (this->Flags & EXPR_F_FREEZEEVAL) printf(", FRZ");
	if (this->ObjCoverageMask != 0) printf(", OCM=%d", this->ObjCoverageMask);
	printf("\n");

	/** Now do sub-expressions **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    exp_internal_DumpExpression_r((pExpression)(this->Children.Items[i]), level+4);
	    }

    return 0;
    }


/*** expDumpExpression - prints an expression tree out for debugging purposes
 ***/
int
expDumpExpression(pExpression this)
    {
    int i;
    printf("Expression serial ID = %d\nExpression ParamList serial ID = %d\n",this->SeqID, this->PSeqID);
    if (this->Control) 
        {
	printf("Remapping: ");
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    {
	    if (this->Control->ObjMap[i] != i)
	        {
		printf("E%d->O%d  ",i, this->Control->ObjMap[i]);
		}
	    }
	printf("\n");
	}
    exp_internal_DumpExpression_r(this, 0);
    return 0;
    }


/*** expCopyValue - copy the value of one expression tree node to
 *** another tree node.
 ***/
int
expCopyValue(pExpression src, pExpression dst, int make_independent)
    {

    	/** First, copy data type and NULL flag. **/
	dst->DataType = src->DataType;
	if (src->Flags & EXPR_F_NULL)
	    {
	    dst->Flags |= EXPR_F_NULL;
	    return 0;
	    }
	dst->Flags &= ~EXPR_F_NULL;

	/** Release the string from dst if allocated. **/
	if (dst->String && dst->Alloc)
	    {
	    nmSysFree(dst->String);
	    dst->String = NULL;
	    dst->Alloc = 0;
	    }

	/** Copy the value **/
	switch(dst->DataType)
	    {
	    case DATA_T_INTEGER: dst->Integer = src->Integer; break;
	    case DATA_T_DOUBLE: dst->Types.Double = src->Types.Double; break;
	    case DATA_T_MONEY: memcpy(&(dst->Types.Money), &(src->Types.Money), sizeof(MoneyType)); break;
	    case DATA_T_DATETIME: memcpy(&(dst->Types.Date), &(src->Types.Date), sizeof(DateTime)); break;
	    case DATA_T_STRING: 
	        if (make_independent)
		    {
		    if (src->String == src->Types.StringBuf)
		        {
			dst->String = dst->Types.StringBuf;
			strcpy(dst->String, src->String);
			}
		    else
		        {
			dst->String = nmSysStrdup(src->String);
			dst->Alloc = 1;
			}
		    }
		else
		    {
		    dst->String = src->String;
		    dst->Alloc = 0;
		    }
		break;
	    }

    return 0;
    }


/*** expAddNode - adds a sub-expression to a parent expression.
 ***/
int
expAddNode(pExpression parent, pExpression child)
    {
    xaAddItem(&(parent->Children), (void*)child);
    child->Parent = parent;
    return 0;
    }


/*** expDataTypeToNodeType - returns the EXPR_N_xxx for a DATA_T_xxx
 ***/
int
expDataTypeToNodeType(int data_type)
    {
    static int types[] = {0, EXPR_N_INTEGER, EXPR_N_STRING, EXPR_N_DOUBLE, EXPR_N_DATETIME, 0, 0, EXPR_N_MONEY };
    return types[data_type];
    }


/*** expPodToExpression - takes a Pointer to Object Data (pod) and
 *** builds an expression node from it.
 ***/
pExpression
expPodToExpression(pObjData pod, int type)
    {
    pExpression exp;
    int n;

	/** Create expression node. **/
	exp = expAllocExpression();

	/** Based on type. **/
	switch(type)
	    {
	    case DATA_T_INTEGER:
		exp->Integer = pod->Integer;
		break;
	    case DATA_T_STRING:
		n = strlen(pod->String);
		if (n <= 63)
		    {
		    exp->String = exp->Types.StringBuf;
		    }
		else
		    {
		    exp->String = nmSysMalloc(n+1);
		    exp->Alloc = 1;
		    }
		strcpy(exp->String, pod->String);
		break;
	    case DATA_T_DOUBLE:
		exp->Types.Double = pod->Double;
		break;
	    case DATA_T_MONEY:
		memcpy(&(exp->Types.Money), pod->Money, sizeof(MoneyType));
		break;
	    case DATA_T_DATETIME:
		memcpy(&(exp->Types.Date), pod->DateTime, sizeof(DateTime));
		break;
	    default:
		expFreeExpression(exp);
		return NULL;
	    }
	exp->NodeType = expDataTypeToNodeType(type);
	exp->DataType = type;
	/*expEvalTree(exp,expNullObjlist);*/

    return exp;
    }


/*** expExpressionToPod - takes an expression node and returns a
 *** pointer to object data (pod).
 ***/
int
expExpressionToPod(pExpression this, pObjData pod)
    {

	/** Null? **/
	if (this->Flags & EXPR_F_NULL) return 1;

	/** Based on data type. **/
	switch(this->DataType)
	    {
	    case DATA_T_INTEGER:
		pod->Integer = this->Integer;
		break;
	    case DATA_T_STRING:
		pod->String = this->String;
		break;
	    case DATA_T_MONEY:
		pod->Money = &(this->Types.Money);
		break;
	    case DATA_T_DATETIME:
		pod->DateTime = &(this->Types.Date);
		break;
	    case DATA_T_DOUBLE:
		pod->Double = this->Types.Double;
		break;
	    default:
		return -1;
	    }
    
    return 0;
    }


/*** expInitialize - initialize the expression evaluator subsystem.
 ***/
int
expInitialize()
    {

    	/** Init globals **/
	memset(&EXP,0,sizeof(EXP));
	EXP.PSeqID = 1;

	/** Init function list **/
	exp_internal_DefineNodeEvals();

	/** Function list for EXPR_N_FUNCTION nodes **/
	xhInit(&EXP.Functions, 255, 0);
	exp_internal_DefineFunctions();

	/** Define the null objectlist **/
	expNullObjlist = expCreateParamList();

    return 0;
    }
