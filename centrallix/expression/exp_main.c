#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"
#include "cxlib/magic.h"
#include <openssl/sha.h>
#include <openssl/md5.h>

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
	expr->String = NULL;
	expr->Name = NULL;
	expr->NameAlloc = 0;
	expr->Parent = NULL;
	expr->Flags = EXPR_F_NEW;
	expr->ObjCoverageMask = 0;
	expr->ObjOuterMask = 0;
	expr->ObjDelayChangeMask = 0;
	expr->ObjID = -1;
	expr->AggExp = NULL;
	expr->AggCount = 0;
	expr->AggLevel = 0;
	expr->Control = NULL;
	expr->ListCount = 1;
	expr->ListTrackCnt = 1;
	expr->Magic = MGK_EXPRESSION;
	expr->LinkCnt = 1;
	expr->DataType = DATA_T_UNAVAILABLE;
	expr->PrivateData = NULL;

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
	exp_internal_UnlinkControl(this->Control);

	/** Free this itself. **/
	xaDeInit(&(this->Children));
	if (this->PrivateData) nmSysFree(this->PrivateData);
	if (this->Alloc && this->String) nmSysFree(this->String);
	if (this->NameAlloc && this->Name) nmSysFree(this->Name);
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
    if (((!objlist || !objlist->CurControl) && (!exp->Control || !exp->Control->Remapped)) || id < 0) return id;
    if (exp->Control && exp->Control->Remapped) 
        id = exp->Control->ObjMap[id];
    else if (objlist && objlist->CurControl && objlist->CurControl->Remapped)
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
	new_tree->ObjDelayChangeMask = src->ObjDelayChangeMask;
	new_tree->AggLevel = src->AggLevel;
	new_tree->CmpFlags = src->CmpFlags;
	new_tree->LxFlags = src->LxFlags;
	memcpy(&(new_tree->Types), &(src->Types), sizeof(src->Types));

	/** String fields may need to be allocated.. **/
	if (src->String == src->Types.StringBuf)
	    {
	    new_tree->String = new_tree->Types.StringBuf;
	    new_tree->Alloc = 0;
	    }
	else if (src->String)
	    {
	    /** force allocation if not using in-struct buf, since String
	     ** may have been pointing to a child node's string as well.
	     **/
	    new_tree->Alloc = 1;
	    new_tree->String = nmSysStrdup(src->String);
	    }
	if (new_tree->NameAlloc)
	    {
	    new_tree->Name = nmSysStrdup(src->Name);
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


/*** expIsConstant() - returns true (1) if the expression is a constant
 *** node.
 ***/
int
expIsConstant(pExpression this)
    {
    int t = this->NodeType;
    return (t == EXPR_N_INTEGER || t == EXPR_N_STRING || t == EXPR_N_DOUBLE || t == EXPR_N_MONEY || t == EXPR_N_DATETIME);
    }


/*** exp_internal_CopyTreeReduced() - same as below.
 ***/
pExpression
exp_internal_CopyTreeReduced(pExpression orig_exp)
    {
    pExpression new_exp;
    pExpression subexp;
    pExpression new_subexp;
    int i, t;

    	ASSERTMAGIC(orig_exp,MGK_EXPRESSION);

    	/** First, copy the current node. **/
	new_exp = expAllocExpression();
	exp_internal_CopyNode(orig_exp, new_exp);

	/** Is this node frozen? **/
	if (new_exp->Flags & EXPR_F_FREEZEEVAL)
	    {
	    /** convert to a constant **/
	    t = expDataTypeToNodeType(new_exp->DataType);
	    if (t > 0)
		{
		new_exp->NodeType = t;
		new_exp->ObjCoverageMask = 0;
		}
	    }
	else
	    {
	    /** Now, make copies of the subtrees **/
	    for(i=0;i<orig_exp->Children.nItems;i++)
		{
		subexp = (pExpression)(orig_exp->Children.Items[i]);
		new_subexp = exp_internal_CopyTreeReduced(subexp);
		xaAddItem(&(new_exp->Children), (void*)new_subexp);
		new_subexp->Parent = new_exp;
		}
	    }

	/** Optimize AND expressions **/
	if (new_exp->NodeType == EXPR_N_AND && new_exp->ObjCoverageMask != 0)
	    {
	    for(i=0; i<new_exp->Children.nItems; i++)
		{
		subexp = (pExpression)(new_exp->Children.Items[i]);
		if (subexp->ObjCoverageMask == 0 && subexp->DataType == DATA_T_INTEGER && !(subexp->Flags & EXPR_F_NULL) && subexp->Integer != 0)
		    {
		    /** True item in AND expression, remove it **/
		    expFreeExpression(subexp);
		    xaRemoveItem(&new_exp->Children, i);
		    i--;
		    continue;
		    }
		else if (subexp->ObjCoverageMask == 0 && subexp->DataType == DATA_T_INTEGER && !(subexp->Flags & EXPR_F_NULL) && subexp->Integer == 0)
		    {
		    /** False item in AND expression, force entire expression false **/
		    xaRemoveItem(&new_exp->Children, i);
		    expFreeExpression(new_exp);
		    new_exp = subexp;
		    break;
		    }
		}
	    
	    /** Anything left? **/
	    if (new_exp->Children.nItems == 0 && new_exp->NodeType == EXPR_N_AND)
		{
		/** Nothing left -- convert to true **/
		new_exp->NodeType = EXPR_N_INTEGER;
		new_exp->DataType = DATA_T_INTEGER;
		new_exp->Flags &= ~EXPR_F_NULL;
		new_exp->Integer = 1;
		new_exp->ObjCoverageMask = 0;
		}
	    else if (new_exp->Children.nItems == 1 && new_exp->NodeType == EXPR_N_AND)
		{
		/** One item left -- use it instead of AND clause **/
		subexp = (pExpression)(new_exp->Children.Items[0]);
		xaRemoveItem(&new_exp->Children, 0);
		expFreeExpression(new_exp);
		new_exp = subexp;
		}
	    }

	/** Optimize OR expressions **/
	if (new_exp->NodeType == EXPR_N_OR && new_exp->ObjCoverageMask != 0)
	    {
	    for(i=0; i<new_exp->Children.nItems; i++)
		{
		subexp = (pExpression)(new_exp->Children.Items[i]);
		if (subexp->ObjCoverageMask == 0 && subexp->DataType == DATA_T_INTEGER && !(subexp->Flags & EXPR_F_NULL) && subexp->Integer == 0)
		    {
		    /** False item in OR expression, remove it **/
		    expFreeExpression(subexp);
		    xaRemoveItem(&new_exp->Children, i);
		    i--;
		    continue;
		    }
		else if (subexp->ObjCoverageMask == 0 && subexp->DataType == DATA_T_INTEGER && !(subexp->Flags & EXPR_F_NULL) && subexp->Integer != 0)
		    {
		    /** True item in OR expression, force entire expression true **/
		    xaRemoveItem(&new_exp->Children, i);
		    expFreeExpression(new_exp);
		    new_exp = subexp;
		    break;
		    }
		}
	    
	    /** Anything left? **/
	    if (new_exp->Children.nItems == 0 && new_exp->NodeType == EXPR_N_OR)
		{
		/** Nothing left -- convert to false **/
		new_exp->NodeType = EXPR_N_INTEGER;
		new_exp->DataType = DATA_T_INTEGER;
		new_exp->Flags &= ~EXPR_F_NULL;
		new_exp->Integer = 0;
		new_exp->ObjCoverageMask = 0;
		}
	    else if (new_exp->Children.nItems == 1 && new_exp->NodeType == EXPR_N_OR)
		{
		/** One item left -- use it instead of OR clause **/
		subexp = (pExpression)(new_exp->Children.Items[0]);
		xaRemoveItem(&new_exp->Children, 0);
		expFreeExpression(new_exp);
		new_exp = subexp;
		}
	    }

	/** Optimize NULL IS (not) NULL and {constant} IS (not) NULL **/
	if (new_exp->NodeType == EXPR_N_ISNOTNULL)
	    {
	    subexp = (pExpression)(new_exp->Children.Items[0]);
	    if (subexp && expIsConstant(subexp))
		{
		expFreeExpression(subexp);
		xaRemoveItem(&new_exp->Children, 0);
		new_exp->NodeType = EXPR_N_INTEGER;
		new_exp->DataType = DATA_T_INTEGER;
		new_exp->Flags &= EXPR_F_NULL;
		new_exp->ObjCoverageMask = 0;
		new_exp->Integer = !(subexp->Flags & EXPR_F_NULL);
		}
	    }
	if (new_exp->NodeType == EXPR_N_ISNULL)
	    {
	    subexp = (pExpression)(new_exp->Children.Items[0]);
	    if (subexp && expIsConstant(subexp))
		{
		expFreeExpression(subexp);
		xaRemoveItem(&new_exp->Children, 0);
		new_exp->NodeType = EXPR_N_INTEGER;
		new_exp->DataType = DATA_T_INTEGER;
		new_exp->Flags &= EXPR_F_NULL;
		new_exp->ObjCoverageMask = 0;
		new_exp->Integer = (subexp->Flags & EXPR_F_NULL);
		}
	    }

    return new_exp;
    }



/*** expReducedDuplicate - duplicate an expression tree, but reduce it,
 *** converting frozen nodes to constants and optimizing out forced true
 *** or false parts of the expression
 ***/
pExpression
expReducedDuplicate(pExpression orig_exp)
    {
    pExpression new_exp;

	expEvalTree(orig_exp, NULL);
	new_exp = exp_internal_CopyTreeReduced(orig_exp);

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
    char* ptr;

    	ASSERTMAGIC(this,MGK_EXPRESSION);

    	/** Print the contents of this expression first **/
	printf("%*.*s",level,level,"");
	switch(this->NodeType)
	    {
	    case EXPR_N_INTEGER: printf("INTEGER = %d", this->Integer); break;
	    case EXPR_N_STRING: printf("STRING = <%s>", this->String); break;
	    case EXPR_N_DOUBLE: printf("DOUBLE = %.2f", this->Types.Double); break;
	    case EXPR_N_MONEY: printf("MONEY = %4.4lld", this->Types.Money.Value); break;
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
	    case EXPR_N_ISNOTNULL: printf("IS NOT NULL"); break;
	    case EXPR_N_FUNCTION: printf("FUNCTION = <%s>", this->Name); break;
	    case EXPR_N_LIST: printf("LIST OF VALUES"); break;
	    case EXPR_N_SUBQUERY: printf("SUBQUERY"); break;
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
		case DATA_T_MONEY: ptr = objDataToStringTmp(DATA_T_MONEY, &(this->Types.Money), 0); printf(", money=%s", ptr); break;
		case DATA_T_DATETIME: ptr = objDataToStringTmp(DATA_T_DATETIME, &(this->Types.Date), 0); printf(", datetime=%s", ptr); break;
		}
	    }
	if (this->Flags & EXPR_F_NEW) printf(", NEW");
	if (this->Flags & EXPR_F_AGGREGATEFN) printf(", AGGFN");
	if (this->Flags & EXPR_F_AGGLOCKED) printf(", AGGLK");
	if (this->Flags & EXPR_F_FREEZEEVAL) printf(", FRZ");
	if (this->AggLevel) printf(", AGGLVL=%d", this->AggLevel);
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
    if (this->Control) 
        {
	printf("Expression ParamList serial ID = %d\n", this->Control->PSeqID);
	printf("Expression Obj SeqIDs: ");
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    {
	    if (this->Control->ObjSeqID[i])
		printf("%d:%d ", i, this->Control->ObjSeqID[i]);
	    }
	printf("\n");
	if (this->Control->Remapped)
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
	dst->Flags &= ~EXPR_F_NULL;
	if (src->Flags & EXPR_F_NULL)
	    {
	    dst->Flags |= EXPR_F_NULL;
	    return 0;
	    }

	/** Release the string from dst if allocated. **/
	if (dst->Alloc && dst->String)
	    {
	    nmSysFree(dst->String);
	    }
	dst->String = NULL;
	dst->Alloc = 0;

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
	    default:
		return -1;
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
    parent->ObjCoverageMask |= child->ObjCoverageMask;
    return 0;
    }


/*** expDataTypeToNodeType - returns the EXPR_N_xxx for a DATA_T_xxx
 ***/
int
expDataTypeToNodeType(int data_type)
    {
    static int types[] = {0, EXPR_N_INTEGER, EXPR_N_STRING, EXPR_N_DOUBLE, EXPR_N_DATETIME, 0, 0, EXPR_N_MONEY };
    if (data_type < 0 || data_type >= sizeof(types)/sizeof(int)) return -1;
    return types[data_type];
    }


/*** expPodToExpression - takes a Pointer to Object Data (pod) and
 *** builds an expression node from it.
 ***/
pExpression
expPodToExpression(pObjData pod, int type, pExpression provided_exp)
    {
    int n;
    pExpression exp = provided_exp;

	/** Create expression node. **/
	if (!exp)
	    exp = expAllocExpression();
	exp->NodeType = expDataTypeToNodeType(type);

	/** Null value **/
	if (!pod)
	    {
	    exp->Flags |= (EXPR_F_NULL | EXPR_F_PERMNULL);
	    }
	else
	    {
	    exp->Flags &= ~(EXPR_F_NULL | EXPR_F_PERMNULL);

	    /** Based on type. **/
	    switch(type)
		{
		case DATA_T_INTEGER:
		    exp->Integer = pod->Integer;
		    break;
		case DATA_T_STRING:
		    if (exp->Alloc)
			nmSysFree(exp->String);
		    n = strlen(pod->String);
		    if (n < sizeof(exp->Types.StringBuf))
			{
			exp->String = exp->Types.StringBuf;
			exp->Alloc = 0;
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
		    if (!provided_exp)
			expFreeExpression(exp);
		    return NULL;
		}
	    }

	exp->DataType = type;
	/*expEvalTree(exp,expNullObjlist);*/

    return exp;
    }


/*** expExpressionToPod - takes an expression node and returns a
 *** pointer to object data (pod).
 ***/
int
expExpressionToPod(pExpression this, int type, pObjData pod)
    {

	/** Null? **/
	if (this->Flags & EXPR_F_NULL) return 1;

	/** Based on data type. **/
	switch(type)
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
	    case DATA_T_CODE:
		pod->Generic = this;
		break;
	    default:
		return -1;
	    }
    
    return 0;
    }


/*** expReplaceString - replace all occurrences of a given string in the
 *** expression tree.  This includes string values, function names, object
 *** names, and property names.  The match must be exact - this does not
 *** do substring replacement.  It also does not replace computed values
 *** or property values - just constants.
 ***/
int
expReplaceString(pExpression tree, char* oldstr, char* newstr)
    {
    int i;
    pExpression subtree;
   
	/** constant string values **/
	if (tree->NodeType == EXPR_N_STRING && !(tree->Flags & EXPR_F_NULL) && !strcmp(oldstr, tree->String))
	    {
	    if (tree->Alloc) nmSysFree(tree->String);
	    tree->Alloc = 1;
	    tree->String = nmSysStrdup(newstr);
	    }

	/** names of functions, objects, and properties **/
	if ((tree->NodeType == EXPR_N_FUNCTION || tree->NodeType == EXPR_N_PROPERTY || tree->NodeType == EXPR_N_OBJECT) && tree->Name && !strcmp(oldstr, tree->Name))
	    {
	    if (tree->NameAlloc) nmSysFree(tree->Name);
	    tree->NameAlloc = 1;
	    tree->Name = nmSysStrdup(newstr);
	    }

	/** handle subtrees **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pExpression)(tree->Children.Items[i]);
	    expReplaceString(subtree, oldstr, newstr);
	    }

    return 0;
    }


/*** expCompareExpressionValues -- see if two expressions have the same value
 *** Returns: 1 on true, 0 on false
 ***/
int
expCompareExpressionValues(pExpression exp1, pExpression exp2)
    {

	/** two nulls are equal even if types mismatch **/
	if ((exp1->Flags & EXPR_F_NULL) && (exp2->Flags & EXPR_F_NULL))
	    return 1;

	/** otherwise, data type must match **/
	if (exp1->DataType != exp2->DataType)
	    return 0;

	/** One is null and the other isn't **/
	if ((exp1->Flags & EXPR_F_NULL) != (exp2->Flags & EXPR_F_NULL))
	    return 0;

	/** Supported data types differ. **/
	if (!(exp1->Flags & EXPR_F_NULL) && exp1->DataType == DATA_T_STRING && exp1->String && exp2->String && strcmp(exp1->String, exp2->String) != 0)
	    return 0;
	if (!(exp1->Flags & EXPR_F_NULL) && exp1->DataType == DATA_T_INTEGER && exp1->Integer != exp2->Integer)
	    return 0;
	if (!(exp1->Flags & EXPR_F_NULL) && exp1->DataType == DATA_T_DOUBLE && exp1->Types.Double != exp2->Types.Double)
	    return 0;
	if (!(exp1->Flags & EXPR_F_NULL) && exp1->DataType == DATA_T_MONEY && exp1->Types.Money.Value != exp2->Types.Money.Value)
	    return 0;
	if (!(exp1->Flags & EXPR_F_NULL) && exp1->DataType == DATA_T_DATETIME && (exp1->Types.Date.Part.Second != exp2->Types.Date.Part.Second || exp1->Types.Date.Part.Minute != exp2->Types.Date.Part.Minute || exp1->Types.Date.Part.Hour != exp2->Types.Date.Part.Hour || exp1->Types.Date.Part.Day != exp2->Types.Date.Part.Day || exp1->Types.Date.Part.Month != exp2->Types.Date.Part.Month || exp1->Types.Date.Part.Year != exp2->Types.Date.Part.Year))
	    return 0;

	/** Unsupported data types **/
	if (exp1->DataType != DATA_T_STRING && exp1->DataType != DATA_T_INTEGER && exp1->DataType != DATA_T_DOUBLE && exp1->DataType != DATA_T_MONEY && exp1->DataType != DATA_T_DATETIME)
	    return -1;

    return 1;
    }


/*** expCompareExpressions - see if two expressions are equivalent, and
 *** if so, return 1.  Otherwise return 0 if they are different.
 ***/
int
expCompareExpressions(pExpression exp1, pExpression exp2)
    {
    int i;
    pExpression subexp1, subexp2;

	/** Compare current node first **/
	if (exp1->NodeType != exp2->NodeType)
	    return 0;
	if (exp1->NodeType == EXPR_N_STRING || exp1->NodeType == EXPR_N_INTEGER || exp1->NodeType == EXPR_N_DOUBLE || exp1->NodeType == EXPR_N_MONEY || exp1->NodeType == EXPR_N_DATETIME)
	    {
	    if (expCompareExpressionValues(exp1, exp2) <= 0)
		return 0;
	    }
	if (exp1->NodeType == EXPR_N_FUNCTION || exp1->NodeType == EXPR_N_SUBQUERY)
	    {
	    if (strcmp(exp1->Name, exp2->Name) != 0)
		return 0;
	    }
	if (exp1->NodeType == EXPR_N_OBJECT || exp1->NodeType == EXPR_N_PROPERTY)
	    {
	    if (exp1->ObjID != exp2->ObjID)
		return 0;
	    if (exp1->ObjID >= 0 && exp1->Name && exp2->Name && strcmp(exp1->Name, exp2->Name) != 0)
		return 0;
	    }
	
	/** Compare children **/
	if (exp1->Children.nItems != exp2->Children.nItems)
	    return 0;
	for(i=0;i<exp1->Children.nItems;i++)
	    {
	    subexp1 = (pExpression)(exp1->Children.Items[i]);
	    subexp2 = (pExpression)(exp2->Children.Items[i]);
	    if (!expCompareExpressions(subexp1, subexp2))
		return 0;
	    }

    return 1;
    }



/*** exp_internal_SetupControl() - setup the expression evaluation
 *** control structure that is normally present on the head node of
 *** an expression tree.
 ***/
int
exp_internal_SetupControl(pExpression exp)
    {
    pExpControl old_control;

	old_control = exp->Control;
	exp->Control = (pExpControl)nmMalloc(sizeof(ExpControl));
	if (!exp->Control) return -ENOMEM;
	memset(exp->Control, 0, sizeof(ExpControl));
	exp->Control->LinkCnt = 1;
	exp->Control->PSeqID = (EXP.PSeqID++);

	if (old_control)
	    exp_internal_UnlinkControl(old_control);

    return 0;
    }


/*** exp_internal_LinkControl() - link to a expression eval control
 *** structure.
 ***/
pExpControl
exp_internal_LinkControl(pExpControl ctl)
    {
    if (ctl)
	ctl->LinkCnt++;
    return ctl;
    }


/*** exp_internal_UnlinkContrl() - unlink from an exp eval ctl struct
 ***/
int
exp_internal_UnlinkControl(pExpControl ctl)
    {
    if (ctl)
	{
	ctl->LinkCnt--;
	if (ctl->LinkCnt <= 0)
	    {
	    nmFree(ctl,sizeof(ExpControl));
	    }
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
	EXP.ModSeqID = 1;

	/** Init function list **/
	exp_internal_DefineNodeEvals();

	/** Function list for EXPR_N_FUNCTION nodes **/
	xhInit(&EXP.Functions, 255, 0);
	xhInit(&EXP.ReverseFunctions, 255, 0);
	exp_internal_DefineFunctions();

	/** Define the null objectlist **/
	expNullObjlist = expCreateParamList();

	/** Initialize random number generator seed **/
	cxssGenerateKey(EXP.Random, sizeof(EXP.Random));

    return 0;
    }
