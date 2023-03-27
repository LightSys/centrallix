#ifndef _MULTIQUERY_H
#define _MULTIQUERY_H

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


#include "obj.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "cxlib/xhandle.h"


#define MQ_MAX_ORDERBY		(25)

#define MQ_MAX_SOURCELEN	(OBJSYS_MAX_PATH+1+16384)


/*** Structure for a query driver.  A query driver basically manages a type
 *** of query operation, such as join, projection, union, etc.
 ***/
typedef struct
    {
    char		Name[64];		/* Name of driver */
    int			(*Analyze)();		/* Analyze and build qy tree */
    int			(*Start)();		/* Start the query operation */
    int			(*NextItem)();		/* Query advance to next item */
    int			(*Finish)();		/* Close the query operation */
    int			(*Release)();		/* Release private ds. alloc in analyze */
    int			Flags;			/* bitmask MQ_DF_xxx */
    int			Precedence;		/* call-order of this driver */
    }
    QueryDriver, *pQueryDriver;


/*** Structure for managing query elements (such as join, project, etc) ***/
typedef struct _QE
    {
    struct _QE*		Parent;
    XArray		Children;		/* child query items */
    XString		AttrNameBuf;		/* buffer for attr names */
    XArray		AttrNames;		/* ptrs to attribute names */
    XArray		AttrDeriv;		/* ptrs to qe where attr comes from */
    XArray		AttrExprPtr;		/* char* for each expression */
    XArray		AttrCompiledExpr;	/* pExpression ptrs for each attr */
    XArray		AttrAssignExpr;		/* pExpression ptrs for each assignment */
    pQueryDriver	Driver;			/* driver handling this qe */
    int 		Flags;			/* bitmask MQ_EF_xxx */
    int			PreCnt;
    int			IterCnt;
    int			SlaveIterCnt;
    int			SrcIndex;
    int			SrcIndexSlave;
    int			CoverageMask;		/* what objects the subtree contains (projections) */
    int			DependencyMask;		/* what objects the subtree constraints "depend" on */
    pObject		LLSource;
    pObjQuery		LLQuery;
    void*		QSLinkage;
    pExpression		Constraint;
    pExpression		OrderBy[MQ_MAX_ORDERBY];
    int			OrderPrio;		/* priority of ordering */
    void*		PrivateData;		/* q-driver specific data structure */
    }
    QueryElement, *pQueryElement;

#define MQ_EF_ADDTLEXP		1		/* if addl expression supplied to Start */
#define MQ_EF_OUTERJOIN		2		/* if join, this is an outerjoin. */
#define MQ_EF_CONSTEXP		4		/* constraint expression supplied in QE */
#define MQ_EF_SLAVESTART	8		/* slave side of join was started */
#define MQ_EF_FROMSUBTREE	16		/* SELECT ... FROM SUBTREE /path/name */
#define MQ_EF_INCLSUBTREE	32		/* SELECT ... FROM INCLUSIVE SUBTREE */
#define MQ_EF_FROMOBJECT	64		/* SELECT ... FROM OBJECT */
#define MQ_EF_WILDCARD		128		/* SELECT ... FROM WILDCARD /path/name*.txt */
#define MQ_EF_PRUNESUBTREE	256		/* SELECT ... FROM PRUNED SUBTREE /path */
#define MQ_EF_PAGED		512		/* SELECT ... FROM PAGED ... */


/*** Structure for the syntactical analysis of the query text. ***/
typedef struct _QS
    {
    struct _QS*		Parent;
    int			NodeType;		/* one-of MQ_T_xxx */
    XArray		Children;
    int			ObjID;
    char		Presentation[32];
    char		Source[MQ_MAX_SOURCELEN];
    char		Name[32];
    int			ObjFlags[EXPR_MAX_PARAMS];
    int			ObjCnt;
    XString		RawData;
    XString		AssignRawData;
    pExpression		Expr;
    pExpression		AssignExpr;
    pQueryElement	QELinkage;
    int			Specificity;		/* source specificity */
    int			Flags;
    int			IntVals[2];		/* used by LIMIT clause */
    }
    QueryStructure, *pQueryStructure;

#define MQ_SF_USED		1		/* QS has been used by another q-drv. */
#define MQ_SF_FORUPDATE		2		/* SELECT query results can be updated */
#define MQ_SF_FROMSUBTREE	4		/* SELECT ... FROM SUBTREE /path/name */
#define MQ_SF_ASTERISK		8		/* SELECT clause uses 'SELECT *' */
#define MQ_SF_IDENTITY		16		/* SELECT ... FROM IDENTITY /path/name */
#define MQ_SF_INCLSUBTREE	32		/* SELECT ... FROM INCLUSIVE SUBTREE */
#define MQ_SF_FROMOBJECT	64		/* SELECT ... FROM OBJECT */
#define MQ_SF_WILDCARD		128		/* SELECT ... FROM WILDCARD /path/name*.txt */
#define MQ_SF_PRUNESUBTREE	256		/* SELECT ... FROM PRUNED SUBTREE /path */
#define MQ_SF_ASSIGNMENT	512		/* SELECT :obj:attr = ... */
#define MQ_SF_EXPRESSION	1024		/* SELECT ... FROM EXPRESSION (exp) */
#define MQ_SF_IFMODIFIED	2048		/* UPDATE ... SET ... IF MODIFIED */
#define MQ_SF_APPSCOPE		4096		/* DECLARE ... SCOPE APPLICATION */
#define MQ_SF_COLLECTION	8192		/* DECLARE COLLECTION ... */
#define MQ_SF_NONEMPTY		16384		/* SELECT ... FROM NONEMPTY ... */
#define MQ_SF_PAGED		32768		/* SELECT ... FROM PAGED ... */

#define MQ_T_QUERY		0
#define MQ_T_SELECTCLAUSE	1
#define MQ_T_FROMCLAUSE		2
#define MQ_T_WHERECLAUSE	3
#define MQ_T_SELECTITEM		4
#define MQ_T_FROMSOURCE		5
#define MQ_T_WHEREITEM		6
#define MQ_T_UNION		7
#define MQ_T_ORDERBYCLAUSE	8
#define MQ_T_ORDERBYITEM	9
#define MQ_T_SETOPTION		10
#define MQ_T_GROUPBYCLAUSE	11
#define MQ_T_CROSSTABCLAUSE	12
#define MQ_T_GROUPBYITEM	13
#define MQ_T_HAVINGCLAUSE	14
#define MQ_T_INSERTCLAUSE	15
#define MQ_T_INSERTITEM		16
#define MQ_T_DELETECLAUSE	17
#define MQ_T_UPDATECLAUSE	18
#define MQ_T_UPDATEITEM		19
#define MQ_T_WITHCLAUSE		20
#define MQ_T_LIMITCLAUSE	21
#define MQ_T_ONDUPCLAUSE	22
#define MQ_T_ONDUPITEM		23
#define MQ_T_ONDUPUPDATEITEM	24
#define MQ_T_DECLARECLAUSE	25


/*** Structure for a declared object ***/
typedef struct _QDO
    {
    char		Name[32];		/* Name of the object */
    pStructInf		Data;			/* Object data */
    }
    QueryDeclaredObject, *pQueryDeclaredObject;


/*** Similarly, this is for a declared collection (i.e. table). ***/
typedef struct _QDC
    {
    char		Name[32];		/* Name of the collection */
    handle_t		Collection;		/* Collection data */
    }
    QueryDeclaredCollection, *pQueryDeclaredCollection;


/*** Data from end-user session/group/app.  This is for declared
 *** items with scope "application".
 ***/
typedef struct _QAD
    {
    XArray		DeclaredObjects;	/* objects created with DECLARE OBJECT ... */
    XArray		DeclaredCollections;	/* collections created with DECLARE COLLECTION ... */
    }
    QueryAppData, *pQueryAppData;


typedef struct _QST QueryStatement, *pQueryStatement;
typedef struct _MQ MultiQuery, *pMultiQuery;

/*** Structure for a multiquery Statement - one sql statement ***/
struct _QST /* QueryStatement */
    {
    int			LinkCnt;
    int			Flags;			/* bitmask MQ_TF_xxx */
    int			IterCnt;		/* rows processed so far, before LIMIT */
    int			UserIterCnt;		/* rows processed so far, after LIMIT */
    pMultiQuery		Query;			/* the main multiquery structure */
    pExpression		WhereClause;		/* where clause expression */
    pExpression		HavingClause;		/* having-clause expression */
    int			LimitStart;
    int			LimitCnt;
    XArray		Trees;			/* list of tree and subtrees */
    pQueryElement	Tree;			/* query exec main tree ptr */
    pQueryStructure	QTree;			/* query syntax tree head ptr */
    pParamObjects	OneObjList;		/* objlist used for query as a whole - e.g., HAVING clause */
    };

#define MQ_TF_ALLOWUPDATE	1
#define MQ_TF_NOMOREREC		2
#define MQ_TF_ASTERISK		4		/* "select *" */
#define MQ_TF_FINISHED		8		/* Finish() called on this statement */
#define MQ_TF_ALLASSIGN		16		/* All select items are assignments */
#define MQ_TF_ONEASSIGN		32		/* At least one select item assigns */
#define MQ_TF_IMMEDIATE		64		/* command already executed */

/*** Structure for managing the multiquery. ***/
struct _MQ /* MultiQuery */
    {
    int			Flags;			/* bitmask MQ_F_xxx */
    int			LinkCnt;		/* number of opens on this */
    int			CntSerial;		/* serial number counter. */
    int			CurSerial;		/* reflects what the objlist has in it. */
    int			QueryCnt;		/* current query# that is running, starts at 0 */
    int			RowCnt;			/* last returned row# */
    pObjSession		SessionID;
    pParamObjects	ObjList;		/* master object list for query */
    /*ParamObjects	CurObjList;*/		/* objlist used for next fetch */
    int			nProvidedObjects;	/* number of objs in objlist provided to query externally */
    int			ProvidedObjMask;	/* mask of external object id's **/
    pLxSession		LexerSession;		/* tokenized query string */
    char*		QueryText;		/* saved copy of query string */
    pQueryStatement	CurStmt;		/* current SQL statement that is executing */
    int			ThisObj;		/* a self-reference to a select statement's items */
    unsigned int	StartMsec;		/* msec value at start of query */
    unsigned int	YieldMsec;		/* msec value at last thYield() */

    /*** the following are for declared objects and
     *** collections with scope "query", the default
     *** scope.
     ***/
    XArray		DeclaredObjects;	/* objects created with DECLARE OBJECT ... */
    XArray		DeclaredCollections;	/* collections created with DECLARE COLLECTION ... */
    };

#define MQ_F_ENDOFSQL		1		/* reached end of list of sql queries */
#define MQ_F_MULTISTATEMENT	2		/* allow multiple statements separated by semicolons */
#define MQ_F_ONESTATEMENT	4		/* disable use of multiple statements (such as in subquery) */
#define MQ_F_NOUPDATE		8		/* disallow changes to any data with this query. */
#define MQ_F_NOINSERTED		16		/* did not create __inserted object. **/
#define MQ_F_SHOWPLAN		32		/* print diagnostics for SQL statement **/


/*** Pseudo-object structure. ***/
typedef struct
    {
    pQueryStatement	Stmt;			/* statement that returned this row */
    pParamObjects	ObjList;		/* copy of objlist specific to this result set row */
    int			Serial;
    int			AttrID;
    int			AstObjID;
    pObject		Obj;
    int			QueryID;
    int			RowIDAllQuery;
    int			RowIDThisQuery;
    int			RowIDBeforeLimit;
    }
    PseudoObject, *pPseudoObject;


/*** Administrative functions ***/
int mqInitialize();
int mqRegisterQueryDriver(pQueryDriver drv);

/*** Query set-up functions ***/

/*** INTERNAL functions ***/
char* mq_internal_QEGetNextAttr(pMultiQuery mq, pQueryElement qe, pParamObjects objlist, int* attrid, int* astobjid);
int mq_internal_FreeQS(pQueryStructure qstree);
pQueryStructure mq_internal_AllocQS(int type);
pQueryStructure mq_internal_FindItem(pQueryStructure tree, int type, pQueryStructure next);
pQueryElement mq_internal_AllocQE();
int mq_internal_FreeQE(pQueryElement qe);
pPseudoObject mq_internal_CreatePseudoObject(pMultiQuery qy, pObject hl_obj);
int mq_internal_FreePseudoObject(pPseudoObject p);
int mq_internal_EvalHavingClause(pQueryStatement stmt, pPseudoObject p);
handle_t mq_internal_FindCollection(pMultiQuery mq, char* collection);
void mq_internal_CheckYield(pMultiQuery mq);

#endif  /* not defined _MULTIQUERY_H */
