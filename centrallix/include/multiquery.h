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

/**CVSDATA***************************************************************

    $Id: multiquery.h,v 1.17 2011/02/18 03:47:46 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/multiquery.h,v $

    $Log: multiquery.h,v $
    Revision 1.17  2011/02/18 03:47:46  gbeeley
    enhanced ORDER BY, IS NOT NULL, bug fix, and MQ/EXP code simplification

    - adding multiq_orderby which adds limited high-level order by support
    - adding IS NOT NULL support
    - bug fix for issue involving object lists (param lists) in query
      result items (pseudo objects) getting out of sorts
    - as a part of bug fix above, reworked some MQ/EXP code to be much
      cleaner

    Revision 1.16  2010/09/08 22:22:43  gbeeley
    - (bugfix) DELETE should only mark non-provided objects as null.
    - (bugfix) much more intelligent join dependency checking, as well as
      fix for queries containing mixed outer and non-outer joins
    - (feature) support for two-level aggregates, as in select max(sum(...))
    - (change) make use of expModifyParamByID()
    - (change) disable RequestNotify mechanism as it needs to be reworked.

    Revision 1.15  2010/01/10 07:51:06  gbeeley
    - (feature) SELECT ... FROM OBJECT /path/name selects a specific object
      rather than subobjects of the object.
    - (feature) SELECT ... FROM WILDCARD /path/name*.ext selects from a set of
      objects specified by the wildcard pattern.  WILDCARD and OBJECT can be
      combined.
    - (feature) multiple statements per SQL query now allowed, with the
      statements terminated by semicolons.

    Revision 1.14  2009/06/26 16:04:26  gbeeley
    - (feature) adding DELETE support
    - (change) HAVING clause now honored in INSERT ... SELECT
    - (bugfix) some join order issues resolved
    - (performance) cache 0 or 1 row result sets during a join
    - (feature) adding INCLUSIVE option to SUBTREE selects
    - (bugfix) switch to qprintf for building RawData sql data
    - (change) some minor refactoring

    Revision 1.13  2008/03/19 07:30:53  gbeeley
    - (feature) adding UPDATE statement capability to the multiquery module.
      Note that updating was of course done previously, but not via SQL
      statements - it was programmatic via objSetAttrValue.
    - (bugfix) fixes for two bugs in the expression module, one a memory leak
      and the other relating to null values when copying expression values.
    - (bugfix) the Trees array in the main multiquery structure could
      overflow; changed to an xarray.

    Revision 1.12  2008/03/14 18:25:44  gbeeley
    - (feature) adding INSERT INTO ... SELECT support, for creating new data
      using SQL as well as using SQL to copy rows around between different
      objects.

    Revision 1.11  2008/03/09 08:00:10  gbeeley
    - (bugfix) even though we shouldn't deallocate the pMultiQuery on query
      close (wait until all objects are closed too), we should shutdown the
      query execution, thus closing the underlying queries; this prevents
      some deadlocking issues at the remote RDBMS.

    Revision 1.10  2008/02/25 23:14:33  gbeeley
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

    Revision 1.9  2007/09/18 17:59:07  gbeeley
    - (change) permit multiple WHERE clauses in the SQL.  They are automatically
      combined using AND.  This permits more flexible building of dynamic SQL
      (no need to do fancy text processing in order to add another WHERE
      constraint to the query).
    - (bugfix) fix for crash when using "SELECT *" with a join.
    - (change) permit the specification of one FROM source to be an "IDENTITY"
      data source for the query.  That data source will be the one affected by
      any inserting and deleting through the query.

    Revision 1.8  2007/07/31 17:39:59  gbeeley
    - (feature) adding "SELECT *" capability, rather than having to name each
      attribute in every query.  Note - "select *" does result in a query
      result set in which each row may differ in what attributes it has,
      depending on the data source(s) used.

    Revision 1.7  2005/09/24 20:19:18  gbeeley
    - Adding "select ... from subtree /path" support to the SQL engine,
      allowing the retrieval of an entire subtree with one query.  Uses
      the new virtual attr support to supply the relative path of each
      retrieved object.  Much the reverse of what a querytree object can
      do.
    - Memory leak fixes in multiquery.c
    - Fix for objdrv_ux regarding fetched objects and the obj->Pathname.

    Revision 1.6  2005/02/26 06:42:38  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.5  2004/06/12 04:02:27  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.4  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.3  2002/04/05 04:42:42  gbeeley
    Fixed a bug involving inconsistent serial numbers and objlist states
    for a multiquery if the user skips back to a previous fetched object
    context and then continues on with the fetching.  Problem also did
    surface if user switched to last-fetched-object after switching to
    a previously fetched one.

    Revision 1.2  2002/03/23 01:30:44  gbeeley
    Added ls__startat option to the osml "queryfetch" mechanism, in the
    net_http.c driver.  Set ls__startat to the number of the first object
    you want returned, where 1 is the first object (in other words,
    ls__startat-1 objects will be skipped over).  Started to add a LIMIT
    clause to the multiquery module, but thought this would be easier and
    just as effective for now.

    Revision 1.1.1.1  2001/08/13 18:00:53  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "obj.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/xstring.h"


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
    pExpression		OrderBy[25];
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


/*** Structure for the syntactical analysis of the query text. ***/
typedef struct _QS
    {
    struct _QS*		Parent;
    int			NodeType;		/* one-of MQ_T_xxx */
    XArray		Children;
    char		Presentation[32];
    char		Source[256];
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

typedef struct _QST QueryStatement, *pQueryStatement;
typedef struct _MQ MultiQuery, *pMultiQuery;

/*** Structure for a multiquery Statement - one sql statement ***/
struct _QST
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

/*** Structure for managing the multiquery. ***/
struct _MQ
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
    };

#define MQ_F_ENDOFSQL		1		/* reached end of list of sql queries */
#define MQ_F_MULTISTATEMENT	2		/* allow multiple statements separated by semicolons */
#define MQ_F_ONESTATEMENT	4		/* disable use of multiple statements (such as in subquery) */


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

#endif  /* not defined _MULTIQUERY_H */
