#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtlexer.h"
#include "expression.h"
#include "xstring.h"
#include "multiquery.h"
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
/* Module: 	multiquery.h, multiquery.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 19, 1999					*/
/* Description:	Provides support for multiple-object queries (such as	*/
/*		join and union operations, and subqueries).  Used by	*/
/*		the objMultiQuery routine (obj_query.c).		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: multiquery.c,v 1.2 2001/09/27 19:26:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiquery.c,v $

    $Log: multiquery.c,v $
    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:55  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:58  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Globals ***/
struct
    {
    XArray		Drivers;
    }
    MQINF;

int mqSetAttrValue(void* inf_v, char* attrname, void* value, pObjTrxTree* oxt);
int mqGetAttrValue(void* inf_v, char* attrname, void* value, pObjTrxTree* oxt);
int mqGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt);
int mqQueryClose(void* qy_v, pObjTrxTree* oxt);

/*** mq_internal_FreeQS - releases memory used by the query syntax tree.
 ***/
int 
mq_internal_FreeQS(pQueryStructure qstree)
    {
    int i;

    	/** First, release memory held by child items **/
	for(i=0;i<qstree->Children.nItems;i++)
	    {
	    mq_internal_FreeQS((pQueryStructure)(qstree->Children.Items[i]));
	    }
	
	/** Release some memory held by structure items. **/
	xaDeInit(&qstree->Children);
	xsDeInit(&qstree->RawData);
	if (qstree->Expr) expFreeExpression(qstree->Expr);
	if (qstree->ObjList) expFreeParamList(qstree->ObjList);

	/** Free this structure **/
	nmFree(qstree,sizeof(QueryStructure));

    return 0;
    }


/*** mq_internal_FreeQE - release a query exec tree structure...
 ***/
int
mq_internal_FreeQE(pQueryElement qetree)
    {
    int i;

    	/** Scan through child subtrees first... **/
	for(i=0;i<qetree->Children.nItems;i++)
	    {
	    mq_internal_FreeQE((pQueryElement)(qetree->Children.Items[i]));
	    }

	/** Tell the driver to release this **/
	qetree->Driver->Release(qetree, NULL);

	/** Release memory held by the arrays, etc **/
	xaDeInit(&qetree->Children);
	xaDeInit(&qetree->AttrNames);
	xaDeInit(&qetree->AttrDeriv);
	xaDeInit(&qetree->AttrExprPtr);
	xaDeInit(&qetree->AttrCompiledExpr);

	/** Release expression **/
	if (qetree->Constraint) expFreeExpression(qetree->Constraint);

	/** Release memory for the node itself **/
	nmFree(qetree, sizeof(QueryElement));

    return 0;
    }


/*** mq_internal_AllocQE - allocate a qe structure and initialize its fields
 ***/
pQueryElement
mq_internal_AllocQE()
    {
    pQueryElement qe;

    	/** Allocate and initialize the arrays, etc **/
	qe = (pQueryElement)nmMalloc(sizeof(QueryElement));
	if (!qe) return NULL;
	xaInit(&qe->Children,16);
	xaInit(&qe->AttrNames,16);
	xaInit(&qe->AttrDeriv,16);
	xaInit(&qe->AttrExprPtr,16);
	xaInit(&qe->AttrCompiledExpr,16);
	qe->Constraint = NULL;
	qe->Flags = 0;

    return qe;
    }


/*** Syntax Parser States ***/
typedef enum
    {
    /** when waiting for main clause keyword like SELECT, FROM, WHERE **/
    LookForClause,

    /** when looking at a clause item **/
    SelectItem,
    FromItem,
    WhereItem,
    OrderByItem,
    GroupByItem,
    HavingItem,

    /** Misc **/
    ParseDone,
    ParseError
    }
    ParserState;


/*** mq_internal_AllocQS - allocate a new qs with a given type.
 ***/
pQueryStructure
mq_internal_AllocQS(int type)
    {
    pQueryStructure this;

    	/** Allocate **/
	this = (pQueryStructure)nmMalloc(sizeof(QueryStructure));
	if (!this) return NULL;

	/** Fill in the structure **/
	this->NodeType = type;
	this->QELinkage = NULL;
	this->Expr = NULL;
	this->ObjList = NULL;
	this->Specificity = 0;
	this->Flags = 0;
	xaInit(&this->Children,16);
	xsInit(&this->RawData);

    return this;
    }


/*** mq_internal_PostProcess - performs some additional processing on the
 *** SELECT, FROM, ORDER-BY, and WHERE clauses after the initial parse has been
 *** completed.
 ***/
int
mq_internal_PostProcess(pQueryStructure qs, pQueryStructure sel, pQueryStructure from, pQueryStructure where, 
		 	pQueryStructure ob, pQueryStructure gb, pQueryStructure ct)
    {
    int i,j,cnt;
    pQueryStructure subtree;
    char* ptr;

    	/** First, build the object list from the FROM clause **/
	qs->ObjList = expCreateParamList();
	if (from) for(i=0;i<from->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(from->Children.Items[i]);
	    if (subtree->Presentation[0])
	        ptr = subtree->Presentation;
	    else
	        ptr = subtree->Source;
	    expAddParamToList(qs->ObjList, ptr, NULL, 0);
	    }

	/** Ok, got object list.  Now compile the SELECT expressions **/
	if (sel) for(i=0;i<sel->Children.nItems;i++)
	    {
	    /** Compile the expression **/
	    subtree = (pQueryStructure)(sel->Children.Items[i]);
	    for(j=0;j<qs->ObjList->nObjects;j++) qs->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, qs->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->Expr) 
	        {
		mssError(0,"MQ","Error in SELECT list expression <%s>", subtree->RawData.String);
		return -1;
		}
	    for(cnt=j=0;j<qs->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = qs->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;

	    /** Determine if we need to assign it a generic name **/
	    if (subtree->Presentation[0] == '\0')
	        {
		if (subtree->Expr->NodeType == EXPR_N_OBJECT && strcmp(((pExpression)(subtree->Expr->Children.Items[0]))->Name,"objcontent"))
		    {
		    strcpy(subtree->Presentation, ((pExpression)(subtree->Expr->Children.Items[0]))->Name);
		    }
		else
		    {
		    sprintf(subtree->Presentation, "column_%3.3d", i);
		    }
		}
	    }

	/** Compile the order by expressions **/
	if (ob) for(i=0;i<ob->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(ob->Children.Items[i]);
	    for(j=0;j<qs->ObjList->nObjects;j++) qs->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, qs->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in ORDER BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    for(cnt=j=0;j<qs->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = qs->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the group-by expressions **/
	if (gb) for(i=0;i<gb->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(gb->Children.Items[i]);
	    for(j=0;j<qs->ObjList->nObjects;j++) qs->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, qs->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in GROUP BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    for(cnt=j=0;j<qs->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = qs->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

    return 0;
    }


/*** mq_internal_DetermineCoverage - determine which objects 'cover' which parts
 *** of the query's where clause, so the various drivers can intelligently apply
 *** filters...
 ***/
int
mq_internal_DetermineCoverage(pExpression where_clause, pQueryStructure qs_where, int level)
    {
    int i,v;
    int sum_objmask = 0;
    int sum_outermask = 0;
    int is_covered = 1;
    pExpression exp;
    pQueryStructure where_item = NULL;

    	/** IF this is an OBJECT or PROPERTY node, just grab the coverage mask. **/
	if (where_clause->NodeType == EXPR_N_OBJECT || where_clause->Children.nItems == 0)
	    {
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else
	    {
    	    /** First, call this routine on sub-expressions **/
	    for(i=0;i<where_clause->Children.nItems;i++)
	        {
		/** Call sub-expression **/
	        exp = (pExpression)(where_clause->Children.Items[i]);
	        mq_internal_DetermineCoverage(exp, qs_where, level+1);

		/** Sum up the object mask of objs involved, and determine outer members **/
	        sum_objmask |= exp->ObjCoverageMask;
		if (where_clause->NodeType == EXPR_N_COMPARE)
		    {
		    if (((where_clause->Flags & EXPR_F_LOUTERJOIN) && i==0) ||
		        ((where_clause->Flags & EXPR_F_ROUTERJOIN) && i==1))
			{
		        sum_outermask |= exp->ObjCoverageMask;
			}
		    }
		else
		    {
		    sum_outermask |= exp->ObjOuterMask;
		    }
	        }
	    where_clause->ObjCoverageMask = sum_objmask;
	    where_clause->ObjOuterMask = sum_outermask;

	    /** Now check to see if all objects can be covered by this. **/
	    for(i=0;i<where_clause->Children.nItems;i++)
	        {
	        exp = (pExpression)(where_clause->Children.Items[i]);
	        if (exp->ObjCoverageMask && (exp->ObjCoverageMask != sum_objmask))
	            {
		    is_covered = 0;
		    break;
	   	    }
	        }

	    /** If this isn't an AND node, it actually is covered... **/
	    if (where_clause->NodeType != EXPR_N_AND) is_covered = 1;
	    }

	/** IF NOT covered, take node's children and make where items from them. **/
	if (!is_covered)
	    {
	    /** Grab out the elements of the AND clause. **/
	    while(where_clause->Children.nItems)
	        {
	        exp = (pExpression)(where_clause->Children.Items[0]);
		where_item = mq_internal_AllocQS(MQ_T_WHEREITEM);
		where_item->Parent = qs_where;
		xaAddItem(&qs_where->Children, (void*)where_item);
		where_item->Expr = exp;
		where_item->Presentation[0] = 0;
		where_item->Name[0] = 0;
		where_item->Source[0] = 0;
		xaRemoveItem(&where_clause->Children,0);

		/** Set the object cnt **/
		if (where_item)
	    	    {
	    	    where_item->ObjCnt = 0;
	    	    v = where_item->Expr->ObjCoverageMask;
	    	    for(i=0;i<16;i++) 
	        	{
			if (v & 1) where_item->ObjCnt++;
			v >>= 1;
			}
	    	    }
		}

	    /** Convert the AND clause to an INTEGER node with value 1 **/
	    where_clause->NodeType = EXPR_N_INTEGER;
	    where_clause->ObjCoverageMask = 0;
	    where_clause->Integer = 1;
	    }

	/** If covered and at top-level, add this as one where-item. **/
	if (is_covered && level == 0)
	    {
	    where_clause->Flags |= EXPR_F_CVNODE;
	    where_item = mq_internal_AllocQS(MQ_T_WHEREITEM);
	    where_item->Parent = qs_where;
	    xaAddItem(&qs_where->Children, (void*)where_item);
	    where_item->Expr = where_clause;
	    where_item->Presentation[0] = 0;
	    where_item->Name[0] = 0;
	    where_item->Source[0] = 0;

	    /** Set the object cnt **/
	    if (where_item)
	    	{
	    	where_item->ObjCnt = 0;
	    	v = where_item->Expr->ObjCoverageMask;
	    	for(i=0;i<16;i++) 
	            {
		    if (v & 1) where_item->ObjCnt++;
		    v >>= 1;
		    }
	        }
	    }

    return 0;
    }


/*** mq_internal_OptimizeExpr - take the various WHERE expressions and perform
 *** little optimizations on them (like removing (x) AND TRUE types of things)
 ***/
int
mq_internal_OptimizeExpr(pExpression *expr)
    {
    pExpression expr_tmp;
    pExpression i0, i1;
    int i;

    	/** Do some 'shortcut' assignments **/
	i0 = (pExpression)((*expr)->Children.Items[0]);
	i1 = (pExpression)((*expr)->Children.Items[1]);

    	/** Perform AND TRUE optimization **/
	if (i1 && (*expr)->NodeType == EXPR_N_AND && i0->NodeType == EXPR_N_INTEGER && i0->Integer == 1)
	    {
	    expr_tmp = *expr;
	    *expr = i1;
	    xaRemoveItem(&(expr_tmp->Children),1);
	    expFreeExpression(expr_tmp);
	    }
	else if (i1 && (*expr)->NodeType == EXPR_N_AND && i1->NodeType == EXPR_N_INTEGER && i1->Integer == 1)
	    {
	    expr_tmp = *expr;
	    *expr = i0;
	    xaRemoveItem(&(expr_tmp->Children),0);
	    expFreeExpression(expr_tmp);
	    }

	/** Optimize sub-expressions **/
	for(i=0;i<(*expr)->Children.nItems;i++)
	    {
	    mq_internal_OptimizeExpr((pExpression*)&((*expr)->Children.Items[i]));
	    }

    return 0;
    }


/*** mq_internal_ProcessWhere - take the where expression, and remove all OR
 *** components from it by converting them to UNION DISTINCT types of operations,
 *** giving multiple queries, unless the OR is within a subtree that is covered
 *** completely by one directory source.  Then take the remaining elements of
 *** the WHERE expression, and add them as WHERE items in the query syntax
 *** structure.  Whew!!!
 ***/
int
mq_internal_ProcessWhere(pExpression where_clause, pExpression search_at, pQueryStructure *qs)
    {
    /*pQueryStructure new_qs;
    pQueryStructure sub_qs[16];*/

    	/** Is _this_ one an 'OR' expression node? **/
	if (search_at->NodeType == EXPR_N_OR)
	    {
	    }

    return 0;
    }


/*** mq_internal_SyntaxParse - parse the syntax of the SQL used for this
 *** query.
 ***/
pQueryStructure
mq_internal_SyntaxParse(pLxSession lxs)
    {
    pQueryStructure qs, new_qs, select_cls=NULL, from_cls=NULL, where_cls=NULL, orderby_cls=NULL, groupby_cls=NULL, crosstab_cls=NULL, having_cls=NULL;
    pQueryStructure insert_cls=NULL;
    /* pQueryStructure delete_cls=NULL, update_cls=NULL;*/
    ParserState state = LookForClause;
    ParserState next_state;
    int t,parenlevel;
    char* ptr;
    static char* reserved_wds[] = {"where","select","from","order","by","set","rowcount","group",
    				   "crosstab","as","having","into","update","delete","insert",
				   "values","with",NULL};

    	/** Setup reserved words list for lexical analyzer **/
	mlxSetReservedWords(lxs, reserved_wds);

    	/** Allocate a structure to work with **/
	qs = mq_internal_AllocQS(MQ_T_QUERY);

    	/** Enter finite state machine parser. **/
	while(state != ParseDone && state != ParseError)
	    {
	    switch(state)
	        {
		case ParseDone:
		case ParseError:
		    /** added these to shut up the -Wall gcc switch **/
		    /** never reached - see while() statement above **/
		    break;

	        case LookForClause:
		    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
		        {
			next_state = ParseError;
			mssError(1,"MQ","Query must begin with SELECT");
			mlxNoteError(lxs);
			}
		    else
		        {
			ptr = mlxStringVal(lxs,NULL);
			if (!strcmp("select",ptr))
			    {
			    select_cls = mq_internal_AllocQS(MQ_T_SELECTCLAUSE);
			    xaAddItem(&qs->Children, (void*)select_cls);
			    select_cls->Parent = qs;
		            next_state = SelectItem;
			    }
			else if (!strcmp("from",ptr))
			    {
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    }
			else if (!strcmp("where",ptr))
			    {
			    where_cls = mq_internal_AllocQS(MQ_T_WHERECLAUSE);
			    xaAddItem(&qs->Children, (void*)where_cls);
			    where_cls->Parent = qs;
		            next_state = WhereItem;
			    }
			else if (!strcmp("order",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after ORDER");
				mlxNoteError(lxs);
				}
			    else
			        {
				orderby_cls = mq_internal_AllocQS(MQ_T_ORDERBYCLAUSE);
				xaAddItem(&qs->Children, (void*)orderby_cls);
				orderby_cls->Parent = qs;
			        next_state = OrderByItem;
				}
			    }
			else if (!strcmp("group",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after GROUP");
				mlxNoteError(lxs);
				}
			    else
			        {
				groupby_cls = mq_internal_AllocQS(MQ_T_GROUPBYCLAUSE);
				xaAddItem(&qs->Children, (void*)groupby_cls);
				groupby_cls->Parent = qs;
			        next_state = GroupByItem;
				}
			    }
			else if (!strcmp("with",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_KEYWORD)
			        {
			        mssError(1,"MQ","Expected keyword after WITH");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else
			        {
				new_qs = mq_internal_AllocQS(MQ_T_WITHCLAUSE);
				xaAddItem(&qs->Children, (void*)new_qs);
				new_qs->Parent = qs;
				mlxCopyToken(lxs,new_qs->Name,31);
				next_state = LookForClause;
				}
			    }
			else if (!strcmp("crosstab",ptr))
			    {
			    /** Allocate the main node for the crosstab clause **/
			    crosstab_cls = mq_internal_AllocQS(MQ_T_CROSSTABCLAUSE);
			    xaAddItem(&qs->Children, (void*)crosstab_cls);
			    crosstab_cls->Parent = qs;

			    /** Determine column to be crosstab'd **/
			    if (mlxNextToken(lxs) != MLX_TOK_KEYWORD)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected column name after CROSSTAB");
				mlxNoteError(lxs);
				break;
				}
			    mlxCopyToken(lxs, crosstab_cls->Name, 32);
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD || strcmp(mlxStringVal(lxs,NULL),"by"))
			        {
				next_state = ParseError;
			        mssError(1,"MQ","Expected BY after CROSSTAB <%s>", crosstab_cls->Name);
				mlxNoteError(lxs);
				break;
				}

			    /** Determine crosstab criteria **/
			    while(1)
			        {
				t = mlxNextToken(lxs);
				if (t == MLX_TOK_ERROR || t == MLX_TOK_RESERVEDWD || t == MLX_TOK_EOF) break;
				if (crosstab_cls->RawData.String[0]) xsConcatenate(&crosstab_cls->RawData," ",1);
				if (t == MLX_TOK_STRING) xsConcatenate(&crosstab_cls->RawData,"\"",1);
				ptr = mlxStringVal(lxs,NULL);
				if (!ptr) break;
				xsConcatenate(&crosstab_cls->RawData,ptr,-1);
				if (t == MLX_TOK_STRING) xsConcatenate(&crosstab_cls->RawData,"\"",1);
				}
			    if (t == MLX_TOK_ERROR)
			        {
				mssError(1,"MQ","Expected End-of-SQL, reserved word, or AS after CROSSTAB ... BY ... ");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
			    if (t == MLX_TOK_EOF)
			        {
				next_state = ParseDone;
				break;
				}

			    /** See if we have an AS clause for the crosstab **/
			    if (strcmp(mlxStringVal(lxs,NULL),"as"))
			        {
				mlxHoldToken(lxs);
				break;
				}
			    mlxSetOptions(lxs,MLX_F_IFSONLY);
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_STRING)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected column header name after CROSSTAB ... AS");
				mlxNoteError(lxs);
				break;
				}
			    mlxCopyToken(lxs,crosstab_cls->Presentation,32);
			    mlxUnsetOptions(lxs,MLX_F_IFSONLY);
			    t = mlxNextToken(lxs);
			    if (t == MLX_TOK_RESERVEDWD) mlxHoldToken(lxs);
			    else if (t == MLX_TOK_EOF) next_state = ParseDone;
			    else if (t == MLX_TOK_ERROR) 
			        {
				mssError(1,"MQ","Expected End-of-SQL or reserved word after CROSSTAB ... BY ... AS");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    }
			else if (!strcmp("set",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
			        {
				next_state = ParseError;
				}
			    else
			        {
				ptr = mlxStringVal(lxs,NULL);
				if (!strcmp(ptr,"rowcount"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				        {
					next_state = ParseError;
			                mssError(1,"MQ","SET ROWCOUNT requires INTEGER parameter");
					mlxNoteError(lxs);
					}
				    else
				        {
					new_qs = mq_internal_AllocQS(MQ_T_SETOPTION);
					new_qs->Parent = qs;
					xaAddItem(&qs->Children, (void*)new_qs);
					mlxCopyToken(lxs,new_qs->Source,255);
					strcpy(new_qs->Name,"rowcount");
					next_state = LookForClause;
					}
				    }
				else
				    {
				    next_state = ParseError;
			            mssError(1,"MQ","Unknown SET parameter <%s>", ptr);
				    mlxNoteError(lxs);
				    }
				}
			    }
			else if (!strcmp("having",ptr))
			    {
			    having_cls = mq_internal_AllocQS(MQ_T_HAVINGCLAUSE);
			    xaAddItem(&qs->Children, (void*)having_cls);
			    having_cls->Parent = qs;
		            next_state = HavingItem;
			    }
			else if (!strcmp("delete",ptr))
			    {
			    }
			else if (!strcmp("insert",ptr))
			    {
			    /** Create the main insert clause **/
			    insert_cls = mq_internal_AllocQS(MQ_T_INSERTCLAUSE);
			    xaAddItem(&qs->Children, (void*)insert_cls);
			    insert_cls->Parent = qs;

			    /** Check for optional "INTO" and then table name **/
		    	    if ((t=mlxNextToken(lxs)) == MLX_TOK_RESERVEDWD)
			        {
				ptr = mlxStringVal(lxs,NULL);
				if (!strcasecmp(ptr,"into"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_KEYWORD)
				        {
				    	next_state = ParseError;
				    	mssError(1,"MQ","Expected table name after INSERT INTO");
				    	mlxNoteError(lxs);
				    	break;
					}
				    }
				else
				    {
				    next_state = ParseError;
				    mssError(1,"MQ","Expected table name or INTO after INSERT, got <%s>",ptr);
				    mlxNoteError(lxs);
				    break;
				    }
				}
			    else if (t != MLX_TOK_KEYWORD)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected table name or INTO after INSERT");
				mlxNoteError(lxs);
				break;
				}
		    	    mlxCopyToken(lxs,insert_cls->Source,256);

			    /** Ok, either a paren (for column list), VALUES, or SELECT is next. **/
			    t = mlxNextToken(lxs);
			    if (t == MLX_TOK_KEYWORD)
			        {
				/** VALUES or SELECT **/
				}
			    else if (t == MLX_TOK_OPENPAREN)
			        {
				/** Column list **/
				}
			    }
			else if (!strcmp("update",ptr))
			    {
			    }
			else
			    {
			    next_state = ParseError;
			    mssError(1,"MQ","Unknown reserved word or clause name <%s>", ptr);
			    mlxNoteError(lxs);
			    }
			}
		    break;

		case GroupByItem:
		    new_qs = mq_internal_AllocQS(MQ_T_GROUPBYITEM);
		    new_qs->Parent = groupby_cls;
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&groupby_cls->Children, (void*)new_qs);

		    /** Copy the entire item literally to the RawData for later compilation **/
		    parenlevel = 0;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD)
			    {
			    break;
			    }
			if (t == MLX_TOK_COMMA && parenlevel == 0) break;
			if (t == MLX_TOK_OPENPAREN) parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN) parenlevel--;
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    }
			else
			    {
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    }
			xsConcatenate(&new_qs->RawData," ",1);
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or group-by item after GROUP BY clause");
			mlxNoteError(lxs);
			break;
			}
		    break;


		case OrderByItem:
		    new_qs = mq_internal_AllocQS(MQ_T_ORDERBYITEM);
		    new_qs->Parent = orderby_cls;
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&orderby_cls->Children, (void*)new_qs);

		    /** Copy the entire item literally to the RawData for later compilation **/
		    parenlevel = 0;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD)
			    {
			    break;
			    }
			if (t == MLX_TOK_COMMA && parenlevel == 0) break;
			if (t == MLX_TOK_OPENPAREN) parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN) parenlevel--;
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    }
			else
			    {
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    }
			xsConcatenate(&new_qs->RawData," ",1);
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or order-by item after ORDER BY clause");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case SelectItem:
		    new_qs = mq_internal_AllocQS(MQ_T_SELECTITEM);
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&select_cls->Children, (void*)new_qs);
		    new_qs->Parent = select_cls;

		    /** Copy the entire item literally to the RawData for later compilation **/
		    parenlevel = 0;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD)
			    {
			    break;
			    }
			if (t == MLX_TOK_COMMA && parenlevel == 0) break;
			if (t == MLX_TOK_OPENPAREN) parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN) parenlevel--;
			if (t == MLX_TOK_EQUALS && new_qs->Presentation[0] == 0)
			    {
			    memccpy(new_qs->Presentation, new_qs->RawData.String,0,31);
			    new_qs->Presentation[31] = 0;
			    if (strrchr(new_qs->Presentation,' '))
			        *(strrchr(new_qs->Presentation,' ')) = '\0';
			    xsCopy(&new_qs->RawData,"",-1);
			    continue;
			    }
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    xsConcatenate(&new_qs->RawData,"\"",1);
			    }
			else
			    {
			    xsConcatenate(&new_qs->RawData,ptr,-1);
			    }
			xsConcatenate(&new_qs->RawData," ",1);
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected select item, FROM clause, or end-of-query after SELECT");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case FromItem:
		    t = mlxNextToken(lxs);
		    if (t != MLX_TOK_FILENAME && t != MLX_TOK_STRING)
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected data source filename in FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    new_qs = mq_internal_AllocQS(MQ_T_FROMSOURCE);
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&from_cls->Children, (void*)new_qs);
		    new_qs->Parent = from_cls;
		    mlxCopyToken(lxs,new_qs->Source,256);
		    t = mlxNextToken(lxs);
		    if (t == MLX_TOK_KEYWORD)
		        {
			mlxCopyToken(lxs,new_qs->Presentation,32);
			t = mlxNextToken(lxs);
			}
		    if (t == MLX_TOK_COMMA) 
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query, FROM source, or WHERE/ORDER BY after FROM clause");
			mlxNoteError(lxs);
			break;
			}
		    break;

		case WhereItem:
		    /** Copy the whole where clause first **/
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD)
			    {
			    break;
			    }
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatenate(&where_cls->RawData,"\"",1);
			    xsConcatenate(&where_cls->RawData,ptr,-1);
			    xsConcatenate(&where_cls->RawData,"\"",1);
			    }
			else
			    {
			    xsConcatenate(&where_cls->RawData,ptr,-1);
			    }
			xsConcatenate(&where_cls->RawData," ",1);
			}

		    /** We'll break the where clause out later.  Now should be end of SQL. **/
		    if (t == MLX_TOK_EOF)
		        {
		        next_state = ParseDone;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
		        next_state = LookForClause;
			mlxHoldToken(lxs);
			}
		    else
		    	{
		        next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query, GROUP BY, ORDER BY, or HAVING after WHERE clause");
			mlxNoteError(lxs);
			}
		    break;

		case HavingItem:
		    /** Copy the whole having clause first **/
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD)
			    {
			    break;
			    }
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatenate(&having_cls->RawData,"\"",1);
			    xsConcatenate(&having_cls->RawData,ptr,-1);
			    xsConcatenate(&having_cls->RawData,"\"",1);
			    }
			else
			    {
			    xsConcatenate(&having_cls->RawData,ptr,-1);
			    }
			xsConcatenate(&having_cls->RawData," ",1);
			}

		    /** We'll break the having clause out later.  Now should be end of SQL. **/
		    if (t == MLX_TOK_EOF)
		        {
		        next_state = ParseDone;
			}
		    else if (t == MLX_TOK_RESERVEDWD)
		        {
		        next_state = LookForClause;
			mlxHoldToken(lxs);
			}
		    else
		    	{
		        next_state = ParseError;
			mssError(1,"MQ","Expected end-of-query or ORDER BY after HAVING clause");
			mlxNoteError(lxs);
			}
		    break;
		}

	    /** Set the next state **/
	    state = next_state;
	    }

	/** Error? **/
	if (state == ParseError)
	    {
	    mq_internal_FreeQS(qs);
	    mssError(0,"MQ","Could not parse multiquery");
	    return NULL;
	    }

	/** Ok, postprocess the expression trees, etc. **/
	if (mq_internal_PostProcess(qs, select_cls, from_cls, where_cls, orderby_cls, groupby_cls, crosstab_cls) < 0)
	    {
	    mq_internal_FreeQS(qs);
	    mssError(0,"MQ","Could not postprocess multiquery");
	    return NULL;
	    }

    return qs;
    }


/*** mq_internal_DumpQS - prints the QS tree out for debugging purposes.
 ***/
int
mq_internal_DumpQS(pQueryStructure tree, int level)
    {
    int i;

    	/** Print this item **/
	switch(tree->NodeType)
	    {
	    case MQ_T_QUERY: printf("%*.*sQUERY:\n",level*4,level*4,""); break;
	    case MQ_T_SELECTCLAUSE: printf("%*.*sSELECT:\n",level*4,level*4,""); break;
	    case MQ_T_FROMCLAUSE: printf("%*.*sFROM:\n",level*4,level*4,""); break;
	    case MQ_T_FROMSOURCE: printf("%*.*s%s (%s)\n",level*4,level*4,"",tree->Source,tree->Presentation); break;
	    case MQ_T_SELECTITEM: printf("%*.*s%s (%s)\n",level*4,level*4,"",tree->RawData.String,tree->Presentation); break;
	    case MQ_T_WHERECLAUSE: printf("%*.*sWHERE: %s\n",level*4,level*4,"",tree->RawData.String); break;
	    default: printf("%*.*sunknown\n",level*4,level*4,"");
	    }

	/** Print child items **/
	for(i=0;i<tree->Children.nItems;i++) 
	    mq_internal_DumpQS((pQueryStructure)(tree->Children.Items[i]),level+1);

    return 0;
    }


/*** mq_internal_DumpQE - print the QE tree out for debug purposes.
 ***/
int
mq_internal_DumpQE(pQueryElement tree, int level)
    {
    int i;

    	/** print the driver type handling this tree **/
	printf("%*.*sDRIVER=%s, CPTR=%8.8x, NC=%d\n",level*4,level*4,"",tree->Driver->Name,(unsigned int)(tree->Children.Items),tree->Children.nItems);

	/** print child items **/
	for(i=0;i<tree->Children.nItems;i++)
	    mq_internal_DumpQE((pQueryElement)(tree->Children.Items[i]), level+1);

    return 0;
    }


/*** mq_internal_FindItem_r - internal recursive version of the below.
 ***/
pQueryStructure
mq_internal_FindItem_r(pQueryStructure tree, int type, pQueryStructure next, int* found_next)
    {
    pQueryStructure qs;
    int i;

    	/** Is this structure itself it? **/
	if (tree->NodeType == type) 
	    {
	    if (tree == next) 
	        *found_next = 1;
	    else if (*found_next || !next)
	        return tree;
	    }

	/** Otherwise, try children. **/
	for(qs=NULL,i=0;i<tree->Children.nItems;i++)
	    {
	    qs = (pQueryStructure)(tree->Children.Items[i]);
	    qs = mq_internal_FindItem_r(qs,type,next, found_next);
	    if (qs) break;
	    }

    return qs;
    }


/*** mq_internal_FindItem - finds a query structure subtree within the
 *** main tree, where the type of the structure is the given type.
 ***/
pQueryStructure
mq_internal_FindItem(pQueryStructure tree, int type, pQueryStructure next)
    {
    int found_next = 0;
    return mq_internal_FindItem_r(tree, type, next, &found_next);
    }



/*** mq_internal_CkSetObjList - checks to see whether the object list serial
 *** id number is the same as the query's serial id number, and if not, sets
 *** the given object list as the active one for determining the query's src
 *** object set (for expr evaluation).
 ***/
int
mq_internal_CkSetObjList(pMultiQuery mq, pPseudoObject p)
    {

    	/** Check serial id # **/
	if (mq->Serial == p->Serial) return 0;

	/** Ok, need to update... **/
	memcpy(mq->QTree->ObjList, &p->ObjList, sizeof(ParamObjects));
	mq->QTree->ObjList->MainFlags |= EXPR_MO_RECALC;

    return 1;
    }



/*** mqStartQuery - starts a new MultiQuery.  The function of this routine
 *** is similar to the objOpenQuery; but this is called from objMultiQuery,
 *** and is not associated with any given object when it is opened.  Returns
 *** a pointer to a MultiQuery structure.
 ***/
void*
mqStartQuery(pObjSession session, char* query_text)
    {
    pMultiQuery this;
    pLxSession lxs;
    pQueryStructure qs, sub_qs;
    char* exp;
    int i;
    pQueryDriver qdrv;

    	/** Allocate the multiquery structure itself. **/
	this = (pMultiQuery)nmMalloc(sizeof(MultiQuery));
	if (!this) return NULL;
	memset(this,0,sizeof(MultiQuery));
	this->Serial = 0;
	this->SessionID = session;
	this->LinkCnt = 1;

	/** Parse the text of the query, building the syntax structure **/
	lxs = mlxStringSession(query_text, 
		MLX_F_CCOMM | MLX_F_DASHCOMM | MLX_F_ICASER | MLX_F_FILENAMES | MLX_F_EOF);
	if (!lxs)
	    {
	    nmFree(this,sizeof(MultiQuery));
	    mssError(0,"MQ","Could not begin analysis of query text");
	    return NULL;
	    }
	this->QTree = mq_internal_SyntaxParse(lxs);
	mlxCloseSession(lxs);
	if (!this->QTree)
	    {
	    nmFree(this,sizeof(MultiQuery));
	    mssError(0,"MQ","Could not analyze query text");
	    return NULL;
	    }
	/*mq_internal_DumpQS(this->QTree,0);*/

	/** Ok, got syntax.  Find the where clause. **/
	qs = mq_internal_FindItem(this->QTree, MQ_T_WHERECLAUSE, NULL);
	if (!qs)
	    exp = "1";
	else
	    exp = qs->RawData.String;
	this->WhereClause = expCompileExpression(exp,this->QTree->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_OUTERJOIN);
	if (!this->WhereClause)
	    {
	    mq_internal_FreeQS(this->QTree);
	    nmFree(this,sizeof(MultiQuery));
	    mssError(0,"MQ","Error in query's WHERE clause");
	    return NULL;
	    }

	if (qs)
	    {
	    /** Convert the where clause OR elements to UNION elements **/
	    mq_internal_ProcessWhere(this->WhereClause, this->WhereClause, &(this->QTree));

	    /** Break the where clause into tiny little chunks **/
	    mq_internal_DetermineCoverage(this->WhereClause, qs, 0);
	    if (this->WhereClause->Flags & EXPR_F_CVNODE) this->WhereClause = NULL;

	    /** Optimize the "little chunks" **/
	    for(i=0;i<qs->Children.nItems;i++)
	        {
		sub_qs = (pQueryStructure)(qs->Children.Items[i]);
		mq_internal_OptimizeExpr(&(sub_qs->Expr));
		}
	    }

	/** Compile the having expression, if one. **/
	this->HavingClause = NULL;
	qs = mq_internal_FindItem(this->QTree, MQ_T_HAVINGCLAUSE, NULL);
	if (qs)
	    {
	    qs->Expr = expCompileExpression(qs->RawData.String,this->QTree->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!qs->Expr)
	        {
	        mq_internal_FreeQS(this->QTree);
	        nmFree(this,sizeof(MultiQuery));
		mssError(0,"MQ","Error in query's HAVING clause");
		return NULL;
		}
	    this->HavingClause = qs->Expr;
	    qs->Expr = NULL;
	    }

	/** Ok, got the from, select, and where built.  Now call the mq-drivers **/
	for(i=0;i<MQINF.Drivers.nItems;i++)
	    {
	    qdrv = (pQueryDriver)(MQINF.Drivers.Items[i]);
	    if (qdrv->Analyze(this) < 0)
	        {
	        mq_internal_FreeQS(this->QTree);
	        nmFree(this,sizeof(MultiQuery));
	        return NULL;
		}
	    }

	/** Have the MQ drivers start the query. **/
	if (this->Tree->Driver->Start(this->Tree, this, NULL) < 0)
	    {
	    mq_internal_FreeQS(this->QTree);
	    nmFree(this,sizeof(MultiQuery));
	    mssError(0,"MQ","Could not start the query");
	    return NULL;
	    }

	/** Build the one-object list for evaluating the Having clause, etc. **/
	this->OneObjList = expCreateParamList();
	expAddParamToList(this->OneObjList, "this", NULL, 0);
	expSetParamFunctions(this->OneObjList, "this", mqGetAttrType, mqGetAttrValue, mqSetAttrValue);

    return (void*)this;
    }



/*** The following are OSDriver functions.  The MQ Module implements a part
 *** of the ObjectSystem Driver interface.
 ***/

/*** mqClose - closes a pseudo-object that was open as a result of a fetch
 *** operation on the multiquery object or query.
 ***/
int
mqClose(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject inf = (pPseudoObject)inf_v;
    int i;

    	/** Close the query **/
	mqQueryClose(inf->Query, oxt);

	/** Release all objects in our param list **/
	for(i=0;i<inf->ObjList.nObjects;i++) if (inf->ObjList.Objects[i])
	    {
	    objClose((pObject)inf->ObjList.Objects[i]);
	    }

	/** Release the param list **/
	/*expFreeParamList(inf->ObjList);*/

	/** Release the pseudo-object **/
	nmFree(inf, sizeof(PseudoObject));

    return 0;
    }


/*** mqQueryFetch - retrieves the next item in the query result set.
 ***/
void*
mqQueryFetch(void* qy_v, int mode, pObjTrxTree* oxt)
    {
    pPseudoObject p;
    pMultiQuery qy = (pMultiQuery)qy_v;
    int i;
    pObject obj;

	/** Search for results matching the HAVING clause **/
	while(1)
	    {
    	    /** Try to fetch the next record. **/
	    if (qy->Tree->Driver->NextItem(qy->Tree, qy) != 1)
	        {
	        return NULL;
	        }

    	    /** Allocate the pseudo-object. **/
	    p = (pPseudoObject)nmMalloc(sizeof(PseudoObject));
	    if (!p) return NULL;
	    p->Query = qy;
	    qy->LinkCnt++;

	    /** Copy the object list and link to the objects **/
	    memcpy(&p->ObjList, qy->QTree->ObjList, sizeof(ParamObjects));
	    for(i=0;i<p->ObjList.nObjects;i++) 
	        {
	        obj = (pObject)(p->ObjList.Objects[i]);
	        if (obj) objLinkTo(obj);
	        }

	    /** Update row serial # **/
	    qy->Serial++;
	    p->Serial = qy->Serial;

	    /** Verify HAVING clause **/
	    if (!qy->HavingClause) break;
	    expModifyParam(qy->OneObjList, "this", (void*)p);
	    if (expEvalTree(qy->HavingClause, qy->OneObjList) < 0)
	        {
		for(i=0;i<p->ObjList.nObjects;i++) if (p->ObjList.Objects[i]) objClose((pObject)(p->ObjList.Objects[i]));
		qy->LinkCnt--;
		nmFree(p, sizeof(PseudoObject));
		mssError(1,"MQ","Could not evaluate HAVING clause");
		return NULL;
		}
	    if (qy->HavingClause->DataType != DATA_T_INTEGER)
	        {
		for(i=0;i<p->ObjList.nObjects;i++) if (p->ObjList.Objects[i]) objClose((pObject)(p->ObjList.Objects[i]));
		qy->LinkCnt--;
		nmFree(p, sizeof(PseudoObject));
		mssError(1,"MQ","HAVING clause must evaluate to boolean or integer");
		return NULL;
		}
	    if (!(qy->HavingClause->Flags & EXPR_F_NULL) && qy->HavingClause->Integer != 0) 
	        {
		break;
		}
	    else
	        {
		for(i=0;i<p->ObjList.nObjects;i++) if (p->ObjList.Objects[i]) objClose((pObject)(p->ObjList.Objects[i]));
		qy->LinkCnt--;
		nmFree(p, sizeof(PseudoObject));
		}
	    }

    return (void*)p;
    }


/*** mqQueryClose - closes an open query and dismantles the projection and
 *** join structures in the multiquery query tree.
 ***/
int
mqQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pMultiQuery qy = (pMultiQuery)qy_v;

    	/** Check the link cnt **/
	if (--(qy->LinkCnt)) return 0;

	/** Shutdown the mq drivers **/
	qy->Tree->Driver->Finish(qy->Tree,qy);

	/** Free the expressions **/
	if (qy->WhereClause && !(qy->WhereClause->Flags & EXPR_F_CVNODE)) 
	    expFreeExpression(qy->WhereClause);
	if (qy->HavingClause) 
	    expFreeExpression(qy->HavingClause);

	/** Free the query-exec tree. **/
	mq_internal_FreeQE(qy->Tree);

	/** Free the query-structure tree. **/
	mq_internal_FreeQS(qy->QTree);

	/** Release the objlist for the having clause **/
	expFreeParamList(qy->OneObjList);

	/** Free the qy itself **/
	nmFree(qy,sizeof(MultiQuery));

    return 0;
    }


/*** mqRead - reads from the object.  We do this simply by trying the 
 *** objRead functionality in the objects comprising the query, in order of
 *** specificity (childmost object first; object on many side of a one-to
 *** many relationship first, as determined by the structure of the join
 *** clause).
 ***/
int
mqRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* nyi - not yet implemented */
    }


/*** mqWrite - writes to the pseudo-object.  Works like the above, except that
 *** the query must explicitly allow FOR UPDATE.
 ***/
int
mqWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* nyi - not yet implemented */
    }


/*** mqGetAttrType - retrieves the data type of an attribute.  Returns DATA_T_xxx
 *** values (from obj.h).
 ***/
int
mqGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int id=-1,i,dt;

    	/** Request for ls__rowid? **/
	if (!strcmp(attrname,"ls__rowid")) return DATA_T_INTEGER;

    	/** Check to see whether we're on current object. **/
	mq_internal_CkSetObjList(p->Query, p);

	/** Figure out which attribute... **/
	for(i=0;i<p->Query->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Query->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }
	if (id == -1) 
	    {
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expression to get the data type **/
	p->Query->QTree->ObjList->Session = p->Query->SessionID;
	expEvalTree((pExpression)p->Query->Tree->AttrCompiledExpr.Items[id],p->Query->QTree->ObjList);
	dt = ((pExpression)p->Query->Tree->AttrCompiledExpr.Items[id])->DataType;

    return dt;
    }


/*** mqGetAttrValue - retrieves the value of an attribute.  See the object system
 *** documentation on the appropriate pointer types for *value.
 ***/
int
mqGetAttrValue(void* inf_v, char* attrname, void* value, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int id=-1,i;
    pExpression exp;

    	/** Request for row id? **/
	if (!strcmp(attrname,"ls__rowid"))
	    {
	    *((int*)value) = p->Serial;
	    return 0;
	    }

    	/** Check to see whether we're on current object. **/
	mq_internal_CkSetObjList(p->Query, p);

	/** Figure out which attribute... **/
	for(i=0;i<p->Query->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Query->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }
	if (id == -1)
	    {
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expression to get the value **/
	exp = (pExpression)p->Query->Tree->AttrCompiledExpr.Items[id];
	p->Query->QTree->ObjList->Session = p->Query->SessionID;
	if (expEvalTree(exp,p->Query->QTree->ObjList) < 0) return 1;
	if (exp->Flags & EXPR_F_NULL) return 1;
	switch(exp->DataType)
	    {
	    case DATA_T_INTEGER: *(int*)value = exp->Integer; break;
	    case DATA_T_STRING: *(char**)value = exp->String; break;
	    case DATA_T_DOUBLE: *(double*)value = exp->Types.Double; break;
	    case DATA_T_MONEY: *(pMoneyType*)value = &(exp->Types.Money); break;
	    case DATA_T_DATETIME: *(pDateTime*)value = &(exp->Types.Date); break;
	    }

    return 0;
    }


/*** mqGetNextAttr - returns the name of the _next_ attribute.  This function, and
 *** the previous one, both return NULL if there are no (more) attributes.
 ***/
char*
mqGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Check overflow... **/
	if (p->AttrID >= p->Query->Tree->AttrNames.nItems) return NULL;

    return p->Query->Tree->AttrNames.Items[p->AttrID++];
    }


/*** mqGetFirstAttr - returns the name of the first attribute in the multiquery
 *** result set, and preparse mqGetNextAttr to return subsequent attributes.
 ***/
char*
mqGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Set attr id, and return next attr **/
	p->AttrID = 0;

    return mqGetNextAttr(inf_v, oxt);
    }


/*** mqSetAttrValue - sets the value of an attribute.  In order for this to work,
 *** the query must have been opened FOR UPDATE.
 ***/
int
mqSetAttrValue(void* inf_v, char* attrname, void* value, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* not yet impl. */
    }


/*** mqAddAttr - does NOT work for multiqueries.  Return 'bad user'.  Sigh.
 ***/
int
mqAddAttr(void* inf_v, char* attrname, int type, void* value, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* not yet impl. */
    }


/*** mqOpenAttr - does NOT work for multiqueries.  Maybe someday.  Return failed.
 ***/
void*
mqOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqGetFirstMethod - returns the first method associated with this object.
 *** This function will attempt to try to access methods available on the underlying
 *** objects, and will return the first one it finds.
 ***/
char*
mqGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqGetNextMethod - returns the _next_ method associated with this object.
 *** This function will do like the above one, and look at underlying objects.
 ***/
char*
mqGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return NULL; /* not yet impl. */
    }


/*** mqExecuteMethod - executes a given method.  This function will attempt to
 *** locate the method in the underlying objects, and execute it there.  The
 *** query must be opened FOR UPDATE for this to work.
 ***/
int
mqExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    /*pPseudoObject p = (pPseudoObject)inf_v;*/
    return -1; /* not yet impl. */
    }



/*** The following are administrative functions -- that is, they are used to
 *** setup/initialize/maintain the multiquery system.
 ***/


/*** mqRegisterQueryDriver - registers a query driver with this system.  A
 *** query driver may handle such things as a join, union, or projection, and
 *** has a precedence to control the order in which the drivers get chances to
 *** evaluate the SELECT query and arrange themselves.
 ***/
int
mqRegisterQueryDriver(pQueryDriver drv)
    {

    	/** Add the thing to the global XArray. **/
	xaAddItemSorted(&MQINF.Drivers, (void*)drv,((char*)&(drv->Precedence))-((char*)(drv)),4);

    return 0;
    }


/*** mqInitialize - initialize the multiquery module and link in with the
 *** objectsystem management layer, registering as the multiquery module.
 ***/
int
mqInitialize()
    {
    pObjDriver drv;

    	/** Initialize globals **/
	xaInit(&MQINF.Drivers,16);

    	/** Allocate the driver structure **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(ObjDriver));

	/** Fill in the driver structure, as best we can. **/
	drv->Capabilities = OBJDRV_C_ISMULTIQUERY;
	xaInit(&drv->RootContentTypes,16);
	strcpy(drv->Name, "MQ - ObjectSystem MultiQuery Module");
	drv->Open = NULL;
	drv->Close = mqClose;
	drv->Create = NULL;
	drv->Delete = NULL;
	drv->OpenQuery = mqStartQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = mqQueryFetch;
	drv->QueryClose = mqQueryClose;
	drv->Read = mqRead;
	drv->Write = mqWrite;
	drv->GetAttrType = mqGetAttrType;
	drv->GetAttrValue = mqGetAttrValue;
	drv->GetFirstAttr = mqGetFirstAttr;
	drv->GetNextAttr = mqGetNextAttr;
	drv->SetAttrValue = mqSetAttrValue;
	drv->AddAttr = mqAddAttr;
	drv->OpenAttr = mqOpenAttr;
	drv->GetFirstMethod = mqGetFirstMethod;
	drv->GetNextMethod = mqGetNextMethod;
	drv->ExecuteMethod = mqExecuteMethod;

	nmRegister(sizeof(QueryElement),"QueryElement");
	nmRegister(sizeof(QueryStructure),"QueryStructure");
	nmRegister(sizeof(MultiQuery),"MultiQuery");
	nmRegister(sizeof(QueryDriver),"QueryDriver");
	nmRegister(sizeof(PseudoObject),"PseudoObject");
	nmRegister(sizeof(Expression),"Expression");
	nmRegister(sizeof(ParamObjects),"ParamObjects");

	/** Register the module with the OSML. **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }


