#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include "obj.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "multiquery.h"
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
/* Module: 	multiquery.h, multiquery.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 19, 1999					*/
/* Description:	Provides support for multiple-object queries (such as	*/
/*		join and union operations, and subqueries).  Used by	*/
/*		the objMultiQuery routine (obj_query.c).		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: multiquery.c,v 1.41 2011/02/18 03:47:46 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/multiquery/multiquery.c,v $

    $Log: multiquery.c,v $
    Revision 1.41  2011/02/18 03:47:46  gbeeley
    enhanced ORDER BY, IS NOT NULL, bug fix, and MQ/EXP code simplification

    - adding multiq_orderby which adds limited high-level order by support
    - adding IS NOT NULL support
    - bug fix for issue involving object lists (param lists) in query
      result items (pseudo objects) getting out of sorts
    - as a part of bug fix above, reworked some MQ/EXP code to be much
      cleaner

    Revision 1.40  2010/09/08 22:22:43  gbeeley
    - (bugfix) DELETE should only mark non-provided objects as null.
    - (bugfix) much more intelligent join dependency checking, as well as
      fix for queries containing mixed outer and non-outer joins
    - (feature) support for two-level aggregates, as in select max(sum(...))
    - (change) make use of expModifyParamByID()
    - (change) disable RequestNotify mechanism as it needs to be reworked.

    Revision 1.39  2010/01/10 07:51:06  gbeeley
    - (feature) SELECT ... FROM OBJECT /path/name selects a specific object
      rather than subobjects of the object.
    - (feature) SELECT ... FROM WILDCARD /path/name*.ext selects from a set of
      objects specified by the wildcard pattern.  WILDCARD and OBJECT can be
      combined.
    - (feature) multiple statements per SQL query now allowed, with the
      statements terminated by semicolons.

    Revision 1.38  2009/06/26 16:04:26  gbeeley
    - (feature) adding DELETE support
    - (change) HAVING clause now honored in INSERT ... SELECT
    - (bugfix) some join order issues resolved
    - (performance) cache 0 or 1 row result sets during a join
    - (feature) adding INCLUSIVE option to SUBTREE selects
    - (bugfix) switch to qprintf for building RawData sql data
    - (change) some minor refactoring

    Revision 1.37  2008/03/20 00:46:36  gbeeley
    - (bugfix) Fixing a bug introduced in revision 1.11 which caused
      reopen_sql to fail on record creation.

    Revision 1.36  2008/03/19 07:30:53  gbeeley
    - (feature) adding UPDATE statement capability to the multiquery module.
      Note that updating was of course done previously, but not via SQL
      statements - it was programmatic via objSetAttrValue.
    - (bugfix) fixes for two bugs in the expression module, one a memory leak
      and the other relating to null values when copying expression values.
    - (bugfix) the Trees array in the main multiquery structure could
      overflow; changed to an xarray.

    Revision 1.35  2008/03/14 18:25:44  gbeeley
    - (feature) adding INSERT INTO ... SELECT support, for creating new data
      using SQL as well as using SQL to copy rows around between different
      objects.

    Revision 1.34  2008/03/09 08:00:10  gbeeley
    - (bugfix) even though we shouldn't deallocate the pMultiQuery on query
      close (wait until all objects are closed too), we should shutdown the
      query execution, thus closing the underlying queries; this prevents
      some deadlocking issues at the remote RDBMS.

    Revision 1.33  2008/03/06 01:16:34  gbeeley
    - (bugfix) Use qprintf to fix the way that parsed FROM and WHERE items
      in a query are encoded.
    - (bugfix) Enforce having only one SELECT clause per query.

    Revision 1.32  2008/02/25 23:14:33  gbeeley
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

    Revision 1.31  2008/02/17 07:43:25  gbeeley
    - (change) handle system attrbutes (name, inner_type, etc.) automatically
      on queries, even if query has a join.

    Revision 1.30  2007/12/13 23:28:52  gbeeley
    - (bugfix) when determining to which FROM source to attach an unqualified
      WHERE item, and the item is mentioned twice in the SELECT clause since
      it appears from more than one FROM source, give precedence to earlier
      FROM sources, not earlier SELECT items.

    Revision 1.29  2007/12/06 01:02:02  gbeeley
    - (bugfix) partial correction for an issue where unqualified WHERE items
      were not being attached to the correct FROM source.  This can only be
      fully corrected once we have a way to determine subobject attributes
      without actually querying for those subobjects.  Was an issue when
      searching (via osrc) through a SQL join.

    Revision 1.28  2007/12/05 18:57:17  gbeeley
    - (bugfix) request-notify was causing trouble when a "select *" was being
      done, causing updates through a select * query to cause a crash.

    Revision 1.27  2007/09/18 17:59:07  gbeeley
    - (change) permit multiple WHERE clauses in the SQL.  They are automatically
      combined using AND.  This permits more flexible building of dynamic SQL
      (no need to do fancy text processing in order to add another WHERE
      constraint to the query).
    - (bugfix) fix for crash when using "SELECT *" with a join.
    - (change) permit the specification of one FROM source to be an "IDENTITY"
      data source for the query.  That data source will be the one affected by
      any inserting and deleting through the query.

    Revision 1.26  2007/07/31 17:39:59  gbeeley
    - (feature) adding "SELECT *" capability, rather than having to name each
      attribute in every query.  Note - "select *" does result in a query
      result set in which each row may differ in what attributes it has,
      depending on the data source(s) used.

    Revision 1.25  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.24  2007/03/15 17:39:59  gbeeley
    - (bugfix) reset the NULL flag on mqSetAttrValue

    Revision 1.23  2007/03/10 17:57:48  gbeeley
    - minor tweaks to INSERT function in multiquery; needed to commit in order
      to transfer work to a different location...

    Revision 1.22  2007/03/04 05:04:47  gbeeley
    - (change) This is a change to the way that expressions track which
      objects they were last evaluated against.  The old method was causing
      some trouble with stale data in some expressions.

    Revision 1.21  2007/01/31 22:32:19  gbeeley
    - (bugfix) Return error instead of data type when expression evaluation
      fails on error (not on NULL however).

    Revision 1.20  2006/07/07 22:09:04  gbeeley
    - patched up some 'unused variable' compile errors
    - (bugfix) double objClose() caught by ASSERTMAGIC when the last object
      in a query result set is modified after the query is closed, and then
      that last object is closed.

    Revision 1.19  2005/10/18 22:50:33  gbeeley
    - (bugfix) properly detect which object to go to for presentation hints,
      and if it is a composite property (computed), just return default hints
      for now since we'd have to do some fancy stuff with the hints values.

    Revision 1.18  2005/09/24 20:19:18  gbeeley
    - Adding "select ... from subtree /path" support to the SQL engine,
      allowing the retrieval of an entire subtree with one query.  Uses
      the new virtual attr support to supply the relative path of each
      retrieved object.  Much the reverse of what a querytree object can
      do.
    - Memory leak fixes in multiquery.c
    - Fix for objdrv_ux regarding fetched objects and the obj->Pathname.

    Revision 1.17  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.16  2004/06/12 04:02:28  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.15  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.14  2003/08/03 01:00:53  gbeeley
    Suppress attr-not-found warnings for certain attrs in mq.

    Revision 1.13  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.12  2003/03/12 03:19:08  lkehresman
    * Added basic presentation hint support to multiquery.  It only returns
      hints for the first result set, which is the wrong way to do it.  I went
      ahead and committed this so that peter and rupp can start working on the
      other stuff while I work on implementing this correctly.

    * Hints are now presented to the client in the form:
      <a target=XHANDLE HREF='http://ATTRIBUTE/?HINTS#TYPE'>
      where HINTS = hintname=value&hintname=value

    Revision 1.11  2002/11/14 03:46:39  gbeeley
    Updated some files that were depending on the old xaAddItemSorted() to
    use xaAddItemSortedInt32() because these uses depend on sorting on a
    binary integer field, which changes its physical byte ordering based
    on the architecture of the machine's CPU.

    Revision 1.10  2002/08/10 02:09:44  gbeeley
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

    Revision 1.9  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.8  2002/04/30 23:27:45  gbeeley
    Fixed a bug which caused a segfault on update of the last record if
    the query had already been closed.

    Revision 1.7  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.6  2002/04/05 04:42:43  gbeeley
    Fixed a bug involving inconsistent serial numbers and objlist states
    for a multiquery if the user skips back to a previous fetched object
    context and then continues on with the fetching.  Problem also did
    surface if user switched to last-fetched-object after switching to
    a previously fetched one.

    Revision 1.5  2002/03/23 01:30:44  gbeeley
    Added ls__startat option to the osml "queryfetch" mechanism, in the
    net_http.c driver.  Set ls__startat to the number of the first object
    you want returned, where 1 is the first object (in other words,
    ls__startat-1 objects will be skipped over).  Started to add a LIMIT
    clause to the multiquery module, but thought this would be easier and
    just as effective for now.

    Revision 1.4  2002/03/16 04:26:25  gbeeley
    Added functionality in net_http's object access routines so that it,
    when appropriate, sends the metadata attributes also, including the
    following:  "name", "inner_type", "outer_type", and "annotation".

    Revision 1.3  2001/09/28 20:04:50  gbeeley
    Minor efficiency enhancement to expression trees.  Most PROPERTY nodes
    are now self-contained and require no redundant OBJECT nodes as parent
    nodes.  Substantial reduction in expression node allocation and
    evaluation.

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

int mqSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData value, pObjTrxTree* oxt);
int mqGetAttrValue(void* inf_v, char* attrname, int datatype, void* value, pObjTrxTree* oxt);
int mqGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt);
int mq_internal_QueryClose(pMultiQuery qy, pObjTrxTree* oxt);

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
	xsDeInit(&qstree->AssignRawData);
	if (qstree->Expr) expFreeExpression(qstree->Expr);
	if (qstree->AssignExpr) expFreeExpression(qstree->AssignExpr);

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
	xaDeInit(&qetree->AttrAssignExpr);

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
	memset(qe, 0, sizeof(QueryElement));
	xaInit(&qe->Children,16);
	xaInit(&qe->AttrNames,16);
	xaInit(&qe->AttrDeriv,16);
	xaInit(&qe->AttrExprPtr,16);
	xaInit(&qe->AttrCompiledExpr,16);
	xaInit(&qe->AttrAssignExpr,16);
	qe->Parent = NULL;
	qe->Constraint = NULL;
	qe->Flags = 0;
	qe->PrivateData = NULL;
	qe->QSLinkage = NULL;

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
    UpdateItem,

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
	this->AssignExpr = NULL;
	this->Specificity = 0;
	this->Flags = 0;
	xaInit(&this->Children,16);
	xsInit(&this->RawData);
	xsInit(&this->AssignRawData);

    return this;
    }


/*** mq_internal_SetChainedReferences - for order by and group by clauses, the
 *** field lists might reference the result set column names rather than the
 *** source object attributes.  Here, we set those up so that the REFERENCED
 *** flag reflects the actual underlying objects.
 ***/
int
mq_internal_SetChainedReferences(pQueryStructure qs, pQueryStructure clause)
    {
    /*pQueryStructure clause_item;
    int i,cnt,j;*/

    return 0;
    }


int
mq_internal_ExprToPresentation(pExpression exp, char* pres, int maxlen)
    {

	if (exp->NodeType == EXPR_N_OBJECT && strcmp(((pExpression)(exp->Children.Items[0]))->Name,"objcontent"))
	    {
	    strtcpy(pres, ((pExpression)(exp->Children.Items[0]))->Name, maxlen);
	    return 0;
	    }
	else if (exp->NodeType == EXPR_N_PROPERTY && strcmp(exp->Name,"objcontent"))
	    {
	    strtcpy(pres, exp->Name, maxlen);
	    return 0;
	    }

    return -1;
    }


/*** mq_internal_SetCoverage - walk the QE tree and determine what parts
 *** of the tree contain various object references.
 ***/
int
mq_internal_SetCoverage(pQueryStatement stmt, pQueryElement tree)
    {
    int i;
    pQueryElement subtree;

	tree->CoverageMask = 0;

	/** Determine dependencies on all subtrees **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pQueryElement)(tree->Children.Items[i]);
	    mq_internal_SetCoverage(stmt, subtree);
	    tree->CoverageMask |= subtree->CoverageMask;
	    }

	/** No children?  projection module - grab the src index **/
	if (tree->Children.nItems == 0)
	    tree->CoverageMask |= (1<<tree->SrcIndex);

    return 0;
    }


/*** mq_internal_SetDependencies - walk the QE tree and determine what the
 *** dependencies are for various projections and joins, as far as the 
 *** constraints go.  This is mainly for outerjoin purposes, so we know
 *** when we don't have to run the slave side of a join.
 ***/
int
mq_internal_SetDependencies(pQueryStatement stmt, pQueryElement tree, pQueryElement parent)
    {
    int i;
    pQueryElement subtree;

	/** Parent's dependency mask applies to this too **/
	if (parent)
	    tree->DependencyMask = parent->DependencyMask;
	else
	    tree->DependencyMask = 0;

	/** Determine dependencies based on constraint expressions **/
	if (tree->Constraint)
	    tree->DependencyMask |= tree->Constraint->ObjCoverageMask;

	/** Determine dependencies on all subtrees **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    subtree = (pQueryElement)(tree->Children.Items[i]);
	    mq_internal_SetDependencies(stmt, subtree, tree);
	    }

	/** Don't include *ourselves* in the dependency mask **/
	tree->DependencyMask &= ~tree->CoverageMask;

    return 0;
    }


/*** mq_internal_PostProcess - performs some additional processing on the
 *** SELECT, FROM, ORDER-BY, and WHERE clauses after the initial parse has been
 *** completed.
 ***/
int
mq_internal_PostProcess(pQueryStatement stmt, pQueryStructure qs, pQueryStructure sel, pQueryStructure from, pQueryStructure where, 
		 	pQueryStructure ob, pQueryStructure gb, pQueryStructure ct, pQueryStructure up, pQueryStructure dc)
    {
    int i,j,cnt;
    pQueryStructure subtree;
    char* ptr;
    int has_identity;

    	/** First, build the object list from the FROM clause **/
	has_identity = 0;
	if (from)
	    {
	    for(i=0;i<from->Children.nItems;i++)
		{
		subtree = (pQueryStructure)(from->Children.Items[i]);
		if (subtree->Presentation[0])
		    ptr = subtree->Presentation;
		else
		    ptr = subtree->Source;
		if (subtree->Flags & MQ_SF_IDENTITY)
		    has_identity = 1;
		if (expLookupParam(stmt->Query->ObjList, ptr) >= 0)
		    {
		    mssError(1, "MQ", "Data source '%s' already exists in query or query parameter", ptr);
		    return -1;
		    }
		expAddParamToList(stmt->Query->ObjList, ptr, NULL, (i==0 || (subtree->Flags & MQ_SF_IDENTITY))?EXPR_O_CURRENT:0);
		}
	    if (from->Children.nItems > 1 && !has_identity && up)
		{
		mssError(1, "MQ", "UPDATE statement with multiple data sources must have one marked as the IDENTITY source.");
		return -1;
		}
	    if (from->Children.nItems > 0 && !has_identity && dc)
		{
		/** Mark the first one as the identity source **/
		subtree = (pQueryStructure)(from->Children.Items[0]);
		subtree->Flags |= MQ_SF_IDENTITY;
		}
	    if (from->Children.nItems == 1 && !has_identity)
		{
		/** If only one source, it is the identity source **/
		subtree = (pQueryStructure)(from->Children.Items[0]);
		subtree->Flags |= MQ_SF_IDENTITY;
		}
	    }

	/** Ok, got object list.  Now compile the SELECT expressions **/
	if (sel) for(i=0;i<sel->Children.nItems;i++)
	    {
	    /** Compile the expression **/
	    subtree = (pQueryStructure)(sel->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    if (!strcmp(subtree->RawData.String,"*") || !strcmp(subtree->RawData.String,"* "))
		{
		subtree->Expr = NULL;
		subtree->ObjCnt = stmt->Query->ObjList->nObjects - stmt->Query->nProvidedObjects;
		strtcpy(subtree->Presentation, "*", sizeof(subtree->Presentation));
		sel->Flags |= MQ_SF_ASTERISK;
		subtree->Flags |= MQ_SF_ASTERISK;
		if (subtree->ObjCnt == 0)
		    {
		    mssError(0,"MQ","Cannot use 'SELECT *' without at least one 'FROM' data source", subtree->RawData.String);
		    return -1;
		    }
		for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++)
		    {
		    subtree->ObjFlags[j] = EXPR_O_REFERENCED;
		    }
		}
	    else
		{
		subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
		if (!subtree->Expr) 
		    {
		    mssError(0,"MQ","Error in SELECT list expression <%s>", subtree->RawData.String);
		    return -1;
		    }
		cnt = 0;
		for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
		    {
		    subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		    if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		    }
		subtree->ObjCnt = cnt;

		/** Determine if we need to assign it a generic name **/
		if (subtree->Presentation[0] == '\0')
		    {
		    if (mq_internal_ExprToPresentation(subtree->Expr, subtree->Presentation, sizeof(subtree->Presentation)) < 0)
			{
			snprintf(subtree->Presentation, sizeof(subtree->Presentation), "column_%3.3d", i);
			}
		    }
		}
	    }

	/** Compile the order by expressions **/
	if (ob) for(i=0;i<ob->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(ob->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in ORDER BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    for(j=0;j<stmt->Query->nProvidedObjects;j++)
		{
		expFreezeOne(subtree->Expr, stmt->Query->ObjList, j);
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the group-by expressions **/
	if (gb) for(i=0;i<gb->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(gb->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_ASCDESC);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in GROUP BY expression <%s>", subtree->RawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Compile the update expressions **/
	if (up) for(i=0;i<up->Children.nItems;i++)
	    {
	    subtree = (pQueryStructure)(up->Children.Items[i]);
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) stmt->Query->ObjList->Flags[j] &= ~EXPR_O_REFERENCED;
	    subtree->Expr = expCompileExpression(subtree->RawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->Expr)
	        {
		mssError(0,"MQ","Error in UPDATE expression <%s>", subtree->RawData.String);
		return -1;
		}
	    subtree->AssignExpr = expCompileExpression(subtree->AssignRawData.String, stmt->Query->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!subtree->AssignExpr)
	        {
		mssError(0,"MQ","Error in UPDATE assignment expression <%s>", subtree->AssignRawData.String);
		return -1;
		}
	    cnt = 0;
	    for(j=stmt->Query->nProvidedObjects;j<stmt->Query->ObjList->nObjects;j++) 
	        {
		subtree->ObjFlags[j] = stmt->Query->ObjList->Flags[j];
		if (subtree->ObjFlags[j] & EXPR_O_REFERENCED) cnt++;
		}
	    subtree->ObjCnt = cnt;
	    }

	/** Merge WHERE clauses if there are more than one **/
	cnt = xaCount(&qs->Children);
	where = NULL;
	for(i=0;i<cnt;i++)
	    {
	    subtree = (pQueryStructure)xaGetItem(&qs->Children, i);
	    if (subtree->NodeType == MQ_T_WHERECLAUSE)
		{
		if (!where)
		    {
		    where = subtree;
		    }
		else
		    {
		    /** merge **/
		    ptr = nmSysStrdup(where->RawData.String);
		    xsQPrintf(&where->RawData, "( %STR ) and ( %STR )", ptr, subtree->RawData.String);

		    /** remove the extra where clause **/
		    xaRemoveItem(&qs->Children,i);
		    i--;
		    cnt--;
		    mq_internal_FreeQS(subtree);
		    }
		}
	    }

    return 0;
    }


/*** mq_internal_DetermineCoverage - determine which objects 'cover' which parts
 *** of the query's where clause, so the various drivers can intelligently apply
 *** filters...
 ***/
int
mq_internal_DetermineCoverage(pQueryStatement stmt, pExpression where_clause, pQueryStructure qs_where, pQueryStructure qs_select, pQueryStructure qs_update, int level)
    {
    int i,v;
    int sum_objmask = 0;
    int sum_outermask = 0;
    int is_covered = 1;
    int min_id;
    pExpression exp;
    pQueryStructure where_item = NULL;
    pQueryStructure select_item = NULL;
    char presentation[64];

	/** Check coverage mask for leaf nodes first **/
	if (qs_select && where_clause->NodeType == EXPR_N_PROPERTY && where_clause->ObjID == EXPR_OBJID_CURRENT)
	    {
	    /** If no object specified, make sure we have the obj id and thus coverage mask correct **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    min_id = 255;
	    for(i=0;i<qs_select->Children.nItems;i++)
	        {
		select_item = (pQueryStructure)(qs_select->Children.Items[i]);
		if (select_item->Expr && mq_internal_ExprToPresentation(select_item->Expr, presentation, sizeof(presentation)) == 0)
		    {
		    if (!strcmp(presentation, where_clause->Name) && select_item->Expr->ObjID < min_id)
			{
			sum_objmask = where_clause->ObjCoverageMask = select_item->Expr->ObjCoverageMask;
			min_id = where_clause->ObjID = select_item->Expr->ObjID;
			}
		    }
		}
	    }
	else if (qs_update && where_clause->NodeType == EXPR_N_PROPERTY && where_clause->ObjID == EXPR_OBJID_CURRENT)
	    {
	    /** Just leave it alone for now...  we may need to change
	     ** this to refer to the IDENTITY source later on.
	     **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else if (where_clause->Children.nItems == 0)
	    {
	    /** IF this is a non-property leaf node, just grab the coverage mask. **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else if (where_clause->NodeType == EXPR_N_OBJECT)
	    {
	    /** If an object node, just grab the coverage mask **/
	    sum_objmask = where_clause->ObjCoverageMask;
	    }
	else
	    {
    	    /** First, call this routine on sub-expressions **/
	    for(i=0;i<where_clause->Children.nItems;i++)
	        {
		/** Call sub-expression **/
	        exp = (pExpression)(where_clause->Children.Items[i]);
	        mq_internal_DetermineCoverage(stmt, exp, qs_where, qs_select, qs_update, level+1);

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
	        if (exp->ObjCoverageMask && ((exp->ObjCoverageMask & ~EXPR_MASK_EXTREF) != (sum_objmask & ~EXPR_MASK_EXTREF)))
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
		where_item->Expr->Parent = NULL;
		xaRemoveItem(&where_clause->Children,0);

		/** Set the object cnt **/
		if (where_item)
	    	    {
	    	    where_item->ObjCnt = 0;
	    	    v = where_item->Expr->ObjCoverageMask;
	    	    for(i=0;i<EXPR_MAX_PARAMS;i++) 
	        	{
			if ((v & 1) && i >= stmt->Query->nProvidedObjects) where_item->ObjCnt++;
			v >>= 1;
			}
	    	    }
		}

	    /** Convert the AND clause to an INTEGER node with value 1 **/
	    where_clause->NodeType = EXPR_N_INTEGER;
	    where_clause->ObjCoverageMask = 0;
	    where_clause->ObjOuterMask = 0;
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
	    	for(i=0;i<EXPR_MAX_PARAMS;i++) 
	            {
		    if ((v & 1) && i >= stmt->Query->nProvidedObjects) where_item->ObjCnt++;
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

	if ((*expr)->Children.nItems == 0) return 0;

    	/** Perform AND TRUE optimization **/
	if ((*expr)->Children.nItems == 2)
	    {
	    /** Do some 'shortcut' assignments **/
	    i0 = (pExpression)((*expr)->Children.Items[0]);
	    i1 = (pExpression)((*expr)->Children.Items[1]);

	    /** Weed out ___ AND TRUE or TRUE AND ___ **/
	    if (i1 && (*expr)->NodeType == EXPR_N_AND && i0->NodeType == EXPR_N_INTEGER && i0->Integer == 1)
		{
		expr_tmp = *expr;
		*expr = i1;
		(*expr)->Parent = expr_tmp->Parent;
		xaRemoveItem(&(expr_tmp->Children),1);
		expFreeExpression(expr_tmp);
		}
	    else if (i1 && (*expr)->NodeType == EXPR_N_AND && i1->NodeType == EXPR_N_INTEGER && i1->Integer == 1)
		{
		expr_tmp = *expr;
		*expr = i0;
		(*expr)->Parent = expr_tmp->Parent;
		xaRemoveItem(&(expr_tmp->Children),0);
		expFreeExpression(expr_tmp);
		}
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
mq_internal_SyntaxParse(pLxSession lxs, pQueryStatement stmt)
    {
    pQueryStructure qs, new_qs, select_cls=NULL, from_cls=NULL, where_cls=NULL, orderby_cls=NULL, groupby_cls=NULL, crosstab_cls=NULL, having_cls=NULL;
    pQueryStructure insert_cls=NULL, update_cls=NULL, delete_cls=NULL;
    /* pQueryStructure delete_cls=NULL, update_cls=NULL;*/
    pQueryStructure limit_cls = NULL;
    ParserState state = LookForClause;
    ParserState next_state = ParseError;
    int t,parenlevel,subtr,identity,inclsubtr,wildcard,fromobject;
    char* ptr;
    int in_assign;
    static char* reserved_wds[] = {"where","select","from","order","by","set","rowcount","group",
    				   "crosstab","as","having","into","update","delete","insert",
				   "values","with","limit","for", NULL};

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
		    if ((t=mlxNextToken(lxs)) != MLX_TOK_RESERVEDWD)
		        {
			if ((t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON) && (select_cls || update_cls || delete_cls))
			    {
			    mlxHoldToken(lxs);
			    next_state = ParseDone;
			    break;
			    }
			next_state = ParseError;
			mssError(1,"MQ","Query must begin with SELECT");
			mlxNoteError(lxs);
			}
		    else
		        {
			ptr = mlxStringVal(lxs,NULL);
			if (!strcmp("select",ptr))
			    {
			    if (update_cls || delete_cls)
				{
				mssError(1, "MQ", "Query cannot contain both a SELECT and an UPDATE or DELETE clause");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
			    if (select_cls)
				{
				mssError(1, "MQ", "Query must contain only one SELECT clause");
				mlxNoteError(lxs);
				next_state = ParseError;
				break;
				}
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
			else if (!strcmp("limit",ptr))
			    {
			    if (limit_cls)
				{
			        mssError(1,"MQ","Duplicate LIMIT clause found");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				{
			        mssError(1,"MQ","Expected number after LIMIT");
				mlxNoteError(lxs);
				next_state = ParseError;
				}
			    else
				{
				limit_cls = mq_internal_AllocQS(MQ_T_LIMITCLAUSE);
				xaAddItem(&qs->Children, (void*)limit_cls);
				limit_cls->Parent = qs;
				limit_cls->IntVals[0] = mlxIntVal(lxs);
				if (mlxNextToken(lxs) == MLX_TOK_COMMA)
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
					{
					mssError(1,"MQ","Expected numeric START,CNT after LIMIT");
					mlxNoteError(lxs);
					next_state = ParseError;
					}
				    limit_cls->IntVals[1] = mlxIntVal(lxs);
				    next_state = LookForClause;
				    }
				else
				    {
				    mlxHoldToken(lxs);
				    limit_cls->IntVals[1] = limit_cls->IntVals[0];
				    limit_cls->IntVals[0] = 0;
				    }
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
				if (t == MLX_TOK_ERROR || t == MLX_TOK_RESERVEDWD || t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON) break;
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
			    if (t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON)
			        {
				mlxHoldToken(lxs);
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
			    if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON) mlxHoldToken(lxs);
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
			    if (update_cls)
				{
				next_state = UpdateItem;
				break;
				}

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
				else if (!strcmp(ptr,"ms") || !strcmp(ptr,"multistatement"))
				    {
				    if (mlxNextToken(lxs) != MLX_TOK_INTEGER)
				        {
					next_state = ParseError;
			                mssError(1,"MQ","SET MULTISTATEMENT expects 0 or 1 following");
					mlxNoteError(lxs);
					}
				    else
				        {
					if (mlxIntVal(lxs))
					    {
					    stmt->Query->Flags |= MQ_F_MULTISTATEMENT;
					    }
					else
					    {
					    if (stmt->Query->Flags & MQ_F_ONESTATEMENT)
						{
						next_state = ParseError;
						mssError(1,"MQ","SET MULTISTATEMENT disabled in this context");
						mlxNoteError(lxs);
						}
					    else
						{
						stmt->Query->Flags &= ~MQ_F_MULTISTATEMENT;
						}
					    }
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
			    if (select_cls || insert_cls || update_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have DELETE after a SELECT, INSERT, or UPDATE clause");
				mlxNoteError(lxs);
				break;
				}

			    /** Create the main delete clause **/
			    delete_cls = mq_internal_AllocQS(MQ_T_DELETECLAUSE);
			    xaAddItem(&qs->Children, (void*)delete_cls);
			    delete_cls->Parent = qs;

			    /** Optional FROM keyword **/
			    t = mlxNextToken(lxs);
			    if (t != MLX_TOK_RESERVEDWD || ((ptr = mlxStringVal(lxs,NULL)) && strcasecmp("from", ptr)))
				{
				mlxHoldToken(lxs);
				}

			    /** Create a "from" clause too, the user didn't actually type it,
			     ** but this is sortof like DELETE FROM /data/source, and SUBTREE
			     ** is supported as are joins.
	 		     **/
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    break;
			    }
			else if (!strcmp("insert",ptr))
			    {
			    if (select_cls || update_cls || delete_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have INSERT after a SELECT, UPDATE, or DELETE clause");
				mlxNoteError(lxs);
				break;
				}

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
				    t = mlxNextToken(lxs);
				    if (t != MLX_TOK_FILENAME && t != MLX_TOK_STRING)
				        {
				    	next_state = ParseError;
				    	mssError(1,"MQ","Expected pathname after INSERT INTO");
				    	mlxNoteError(lxs);
				    	break;
					}
				    }
				else
				    {
				    next_state = ParseError;
				    mssError(1,"MQ","Expected pathname or INTO after INSERT, got <%s>",ptr);
				    mlxNoteError(lxs);
				    break;
				    }
				}
			    else if (t != MLX_TOK_FILENAME && t != MLX_TOK_STRING)
			        {
				next_state = ParseError;
				mssError(1,"MQ","Expected pathname or INTO after INSERT");
				mlxNoteError(lxs);
				break;
				}
		    	    mlxCopyToken(lxs,insert_cls->Source,256);

			    /** Ok, either a paren (for column list), VALUES, or SELECT is next. **/
			    t = mlxNextToken(lxs);
			    if (t == MLX_TOK_RESERVEDWD)
			        {
				/** VALUES or SELECT **/
				ptr = mlxStringVal(lxs, NULL);
				if (ptr && !strcmp(ptr, "select"))
				    {
				    if (select_cls)
					{
					mssError(1, "MQ", "INSERT INTO ... SELECT: Query must contain only one SELECT clause");
					mlxNoteError(lxs);
					next_state = ParseError;
					break;
					}
				    select_cls = mq_internal_AllocQS(MQ_T_SELECTCLAUSE);
				    xaAddItem(&qs->Children, (void*)select_cls);
				    select_cls->Parent = qs;
				    next_state = SelectItem;
				    }
				}
			    else if (t == MLX_TOK_OPENPAREN)
			        {
				/** Column list **/
				}
			    }
			else if (!strcmp("update",ptr))
			    {
			    if (select_cls || insert_cls || delete_cls)
				{
				next_state = ParseError;
				mssError(1,"MQ","Cannot have UPDATE after a SELECT, INSERT, or DELETE clause");
				mlxNoteError(lxs);
				break;
				}

			    /** Duh, this comes as no surprise. **/
			    stmt->Flags |= MQ_TF_ALLOWUPDATE;

			    /** Create the main update clause **/
			    update_cls = mq_internal_AllocQS(MQ_T_UPDATECLAUSE);
			    xaAddItem(&qs->Children, (void*)update_cls);
			    update_cls->Parent = qs;

			    /** Create a "from" clause too, the user didn't actually type it,
			     ** but this is sortof like UPDATE FROM /data/source, and SUBTREE
			     ** is supported as are joins.
	 		     **/
			    from_cls = mq_internal_AllocQS(MQ_T_FROMCLAUSE);
			    xaAddItem(&qs->Children, (void*)from_cls);
			    from_cls->Parent = qs;
		            next_state = FromItem;
			    break;
			    }
			else if (!strcmp("for",ptr))
			    {
			    if (mlxNextToken(lxs) != MLX_TOK_RESERVEDWD)
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected UPDATE after FOR");
				mlxNoteError(lxs);
				break;
				}
			    ptr = mlxStringVal(lxs,NULL);
			    if (ptr && !strcmp(ptr,"update"))
				{
				if (!select_cls)
				    {
				    next_state = ParseError;
				    mssError(1,"MQ","FOR UPDATE allowed only with SELECT");
				    mlxNoteError(lxs);
				    break;
				    }
				select_cls->Flags |= MQ_SF_FORUPDATE;
				next_state = LookForClause;
				}
			    else
				{
				next_state = ParseError;
				mssError(1,"MQ","Expected UPDATE after FOR");
				mlxNoteError(lxs);
				break;
				}
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
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
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
			    xsConcatQPrintf(&new_qs->RawData, "%STR&DQUOT", ptr);
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
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
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
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
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
			    xsConcatQPrintf(&new_qs->RawData, "%STR&DQUOT", ptr);
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
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
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
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
			    break;
			if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
			    break;
			if (t == MLX_TOK_OPENPAREN) 
			    parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN)
			    parenlevel--;
			if (t == MLX_TOK_EQUALS && new_qs->Presentation[0] == 0 && parenlevel == 0)
			    {
			    strtcpy(new_qs->Presentation, new_qs->RawData.String, sizeof(new_qs->Presentation));
			    if (strrchr(new_qs->Presentation,' '))
			        *(strrchr(new_qs->Presentation,' ')) = '\0';
			    xsCopy(&new_qs->RawData,"",-1);
			    continue;
			    }
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatQPrintf(&new_qs->RawData, "%STR&DQUOT", ptr);
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
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
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
		    subtr = 0;
		    identity = 0;
		    inclsubtr = 0;
		    wildcard = 0;
		    fromobject = 0;
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("identity", ptr))
			{
			t = mlxNextToken(lxs);
			identity = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("object", ptr))
			{
			t = mlxNextToken(lxs);
			fromobject = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("inclusive", ptr))
			{
			t = mlxNextToken(lxs);
			inclsubtr = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("subtree", ptr))
			{
			t = mlxNextToken(lxs);
			subtr = 1;
			}
		    if (t == MLX_TOK_KEYWORD && (ptr = mlxStringVal(lxs,NULL)) && !strcasecmp("wildcard", ptr))
			{
			t = mlxNextToken(lxs);
			wildcard = 1;
			}
		    if (inclsubtr && !subtr)
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: INCLUSIVE keyword is invalid without SUBTREE keyword");
			mlxNoteError(lxs);
			break;
			}
		    if (fromobject && subtr)
			{
			next_state = ParseError;
			mssError(1,"MQ","In FROM clause: OBJECT keyword conflicts with SUBTREE keyword");
			mlxNoteError(lxs);
			break;
			}
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
		    if (subtr) new_qs->Flags |= MQ_SF_FROMSUBTREE;
		    if (inclsubtr) new_qs->Flags |= MQ_SF_INCLSUBTREE;
		    if (identity) new_qs->Flags |= MQ_SF_IDENTITY;
		    if (wildcard) new_qs->Flags |= MQ_SF_WILDCARD;
		    if (fromobject) new_qs->Flags |= MQ_SF_FROMOBJECT;
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
			if (delete_cls && (ptr = mlxStringVal(lxs, NULL)) && !strcasecmp(ptr, "from"))
			    {
			    /** FROM keyword can be used to delimit sources in DELETE clause **/
			    next_state = state;
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    next_state = LookForClause;
			    }
			break;
			}
		    else if (t == MLX_TOK_EOF || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
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
		    parenlevel = 0;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
			    break;
			if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
			    break;
			if (t == MLX_TOK_OPENPAREN) 
			    parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN)
			    parenlevel--;
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatQPrintf(&where_cls->RawData, "%STR&DQUOT", ptr);
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
			if (parenlevel != 0)
			    {
			    next_state = ParseError;
			    mssError(1,"MQ","Unexpected end-of-query in WHERE clause");
			    mlxNoteError(lxs);
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    next_state = ParseDone;
			    }
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
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
		    parenlevel = 0;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF || ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON) && parenlevel <= 0))
			    {
			    break;
			    }
			if (t == MLX_TOK_OPENPAREN) 
			    parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN)
			    parenlevel--;
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    xsConcatQPrintf(&having_cls->RawData, "%STR&DQUOT", ptr);
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
			mlxHoldToken(lxs);
		        next_state = ParseDone;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
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

		case UpdateItem:
		    new_qs = mq_internal_AllocQS(MQ_T_UPDATEITEM);
		    new_qs->Presentation[0] = 0;
		    new_qs->Name[0] = 0;
		    xaAddItem(&update_cls->Children, (void*)new_qs);
		    new_qs->Parent = update_cls;

		    /** Copy the entire item literally to the RawData for later compilation **/
		    parenlevel = 0;
		    in_assign = 1;
		    while(1)
		        {
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_ERROR || t == MLX_TOK_EOF)
			    break;
			if ((t == MLX_TOK_RESERVEDWD || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) && parenlevel <= 0)
			    break;
			if (t == MLX_TOK_OPENPAREN) 
			    parenlevel++;
			if (t == MLX_TOK_CLOSEPAREN)
			    parenlevel--;
			if (t == MLX_TOK_EQUALS && parenlevel == 0 && in_assign)
			    {
			    in_assign = 0;
			    continue;
			    }
			ptr = mlxStringVal(lxs,NULL);
			if (!ptr) break;
			if (t == MLX_TOK_STRING)
			    {
			    if (in_assign)
				xsConcatQPrintf(&new_qs->AssignRawData, "%STR&DQUOT", ptr);
			    else
				xsConcatQPrintf(&new_qs->RawData, "%STR&DQUOT", ptr);
			    }
			else
			    {
			    if (in_assign)
				xsConcatenate(&new_qs->AssignRawData,ptr,-1);
			    else
				xsConcatenate(&new_qs->RawData,ptr,-1);
			    }
			xsConcatenate(&new_qs->RawData," ",1);
			}

		    if (!new_qs->RawData.String[0] || !new_qs->AssignRawData.String[0])
			{
			mssError(1, "MQ", "UPDATE assignment must have form '{assign-expr} = {value-expr}'");
			next_state = ParseError;
			mlxNotePosition(lxs);
			break;
			}

		    /** Where to from here? **/
		    if (t == MLX_TOK_COMMA)
		        {
			next_state = state;
			break;
			}
		    else if (t == MLX_TOK_RESERVEDWD || t == MLX_TOK_SEMICOLON)
		        {
			mlxHoldToken(lxs);
			next_state = LookForClause;
			break;
			}
		    else if (t == MLX_TOK_EOF)
		        {
			mlxHoldToken(lxs);
			next_state = ParseDone;
			break;
			}
		    else
		        {
			next_state = ParseError;
			mssError(1,"MQ","Expected update item, WHERE clause, or end-of-query after UPDATE");
			mlxNoteError(lxs);
			break;
			}
		    break;

		default:
		    /** This should not be reachable **/
		    next_state = ParseError;
		    mssError(1, "MQ", "Bark! Unhandled SQL parser state");
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
	if (mq_internal_PostProcess(stmt, qs, select_cls, from_cls, where_cls, orderby_cls, groupby_cls, crosstab_cls, update_cls, delete_cls) < 0)
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
	    case MQ_T_UPDATECLAUSE: printf("%*.*sUPDATE:\n",level*4,level*4,""); break;
	    case MQ_T_UPDATEITEM: printf("%*.*s%s <-- %s\n",level*4,level*4,"",tree->Name,tree->RawData.String); break;
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
	printf("%*.*sDRIVER=%s, CPTR=%8.8x, NC=%d, FLAG=%d, COV=0x%X, DEP=0x%X",level*4,level*4,"",tree->Driver->Name,
	    (unsigned int)(tree->Children.Items),tree->Children.nItems,tree->Flags, tree->CoverageMask, tree->DependencyMask);

	if (strncmp(tree->Driver->Name, "MQP", 3) == 0 && tree->QSLinkage)
	    printf(", IDX=%d, SRC=%s", tree->SrcIndex, ((pQueryStructure)tree->QSLinkage)->Source);

	printf("\n");

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
	if (mq->CurSerial == p->Serial) return 0;

	/** Ok, need to update... **/
	/*expSyncSeqID(&(p->ObjList), mq->QTree->ObjList);*/
	memcpy(mq->ObjList, &p->ObjList, sizeof(ParamObjects));
	/*mq->QTree->ObjList->MainFlags |= EXPR_MO_RECALC;*/
	mq->CurSerial = p->Serial;

    return 1;
    }



/*** mq_internal_UpdateNotify() - a callback function that is used by the
 *** OSML's request-notify (Rn) mechanism to let the multiquery layer know
 *** that an attribute of an underlying object has changed.  In response, we 
 *** flag the object on the objlist as having changed, and generate an event to
 *** the OSML to let it know that the composite object has been modified - which
 *** possibly will result in Notifications (Rn-style) to whatever has the
 *** multi-query open.
 ***/
int
mq_internal_UpdateNotify(void* v)
    {
    pObjNotification n = (pObjNotification)v;
    pPseudoObject p = (pPseudoObject)(n->Context);
    int objid;
    pExpression exp;
    int i;

	/** We're about to update... sync the seq ids **/
	/*expSyncSeqID(&(p->ObjList), p->Query->QTree->ObjList);*/

	/** Track down the entry in the objlist **/
	objid = expObjChanged(p->ObjList, n->Obj);
	/*objid = expObjChanged(&(p->Query->QTree->ObjList), n->Obj);*/
	if (objid < 0) return -1;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Find attributes affected by it **/
	for(i=0;i<p->Stmt->Tree->AttrCompiledExpr.nItems;i++)
	    {
	    if (!strcmp(p->Stmt->Tree->AttrNames.Items[i], "*"))
		{
		objDriverAttrEvent(p->Obj, n->Name, NULL, 1);
		}
	    else
		{
		exp = (pExpression)(p->Stmt->Tree->AttrCompiledExpr.Items[i]);
		if (expContainsAttr(exp, objid, n->Name))
		    {
		    /** Got one.  Trigger an event on this. **/
		    objDriverAttrEvent(p->Obj, p->Stmt->Tree->AttrNames.Items[i], NULL, 1);
		    }
		}
	    }

    return 0;
    }


/*** mq_internal_FinishStatement() - issue the Finish() call to the underlying
 *** query modules for one statement.
 ***/
int
mq_internal_FinishStatement(pQueryStatement stmt)
    {
    pMultiQuery qy = stmt->Query;
    int i, n;

	/** Only if finish not already called for this one **/
	if (!(stmt->Flags & MQ_TF_FINISHED))
	    {
	    stmt->Flags |= MQ_TF_FINISHED;

	    /** Make sure the cur objlist is correct, otherwise the Finish
	     ** routines in the drivers might close up incorrect objects.
	     **/
	    /*if (qy->CurSerial != qy->CntSerial)
		{
		qy->CurSerial = qy->CntSerial;
		memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
		}*/

	    /** Shutdown the mq drivers.  This can implicitly modify the object list,
	     ** so we update the serial# and save the new object list
	     **/
	    if (stmt->Tree)
		{
		stmt->Tree->Driver->Finish(stmt->Tree,stmt);

		/*memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));*/
		qy->CntSerial = ++qy->CurSerial;
		}

	    /** Trim the object list back to the # of provided objects **/
	    n = stmt->Query->ObjList->nObjects;
	    for(i=stmt->Query->nProvidedObjects; i<n; i++)
		{
		expRemoveParamFromList(stmt->Query->ObjList, stmt->Query->ObjList->Names[i]);
		}
	    }

    return 0;
    }


/*** mq_internal_CloseStatement() - clean up after running one statement.
 ***/
int
mq_internal_CloseStatement(pQueryStatement stmt)
    {

	/** Check link cnt - don't free the structure out from under someone... **/
	stmt->LinkCnt--;
	if (stmt->LinkCnt > 0) return 0;

	/** Issue Finish() if last unlink **/
	mq_internal_FinishStatement(stmt);

	/** Free the expressions **/
	if (stmt->WhereClause && !(stmt->WhereClause->Flags & EXPR_F_CVNODE)) 
	    expFreeExpression(stmt->WhereClause);
	stmt->WhereClause = NULL;
	if (stmt->HavingClause) 
	    expFreeExpression(stmt->HavingClause);
	stmt->HavingClause = NULL;

	/** Free the query-exec tree. **/
	if (stmt->Tree)
	    mq_internal_FreeQE(stmt->Tree);
	stmt->Tree = NULL;

	/** Free the query-structure tree. **/
	if (stmt->QTree)
	    mq_internal_FreeQS(stmt->QTree);
	stmt->QTree = NULL;

	/** Release the objlist for the having clause **/
	if (stmt->OneObjList)
	    expFreeParamList(stmt->OneObjList);
	stmt->OneObjList = NULL;

	/** List of query execution nodes **/
	xaDeInit(&stmt->Trees);

	/** Multistatement mode disabled? **/
	if (!(stmt->Query->Flags & MQ_F_MULTISTATEMENT))
	    stmt->Query->Flags |= MQ_F_ENDOFSQL;

	/** Free the statement itself **/
	nmFree(stmt, sizeof(QueryStatement));

    return 0;
    }



/*** mq_internal_NextStatement() - read the next SQL statement in from the
 *** query text, parse it, and start it.  Returns 1 on success, 0 if no
 *** more queries in the sql, and < 0 on an error condition.
 ***/
int
mq_internal_NextStatement(pMultiQuery this)
    {
    pQueryStructure qs, select_qs, sub_qs, update_qs;
    char* exp;
    int i;
    int t;
    pQueryDriver qdrv;
    pQueryStatement stmt;

	/** End of statement? **/
	if (this->Flags & MQ_F_ENDOFSQL)
	    return 0;

	/** Create new statement structure **/
	stmt = (pQueryStatement)nmMalloc(sizeof(QueryStatement));
	if (!stmt) goto error;
	memset(stmt, 0, sizeof(QueryStatement));
	stmt->LinkCnt = 1;
	stmt->Query = this;
	this->CurStmt = stmt;
	xaInit(&stmt->Trees, 16);
	stmt->IterCnt = -1;
	stmt->UserIterCnt = 0;
	stmt->Flags = MQ_TF_FINISHED;

	this->QueryCnt++;

	/** Parse the query **/
	stmt->QTree = mq_internal_SyntaxParse(this->LexerSession, stmt);
	if (!stmt->QTree)
	    {
	    mssError(0,"MQ","Could not analyze query text");
	    goto error;
	    }
	/*mq_internal_DumpQS(this->QTree,0);*/

	/** Got a semicolon or end-of-text?  Remember it for the next call. **/
	t = mlxNextToken(this->LexerSession);
	if (t == MLX_TOK_EOF || t == MLX_TOK_ERROR) 
	    this->Flags |= MQ_F_ENDOFSQL;
	else if (t != MLX_TOK_SEMICOLON)
	    {
	    mssError(1, "MQ", "Unexpected tokens after end of SQL statement");
	    goto error;
	    }

	/** Are we doing this "for update"? **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	if (qs && qs->Flags & MQ_SF_FORUPDATE)
	    stmt->Flags |= MQ_TF_ALLOWUPDATE;

	/** Ok, got syntax.  Find the where clause. **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_WHERECLAUSE, NULL);
	if (!qs)
	    exp = "1";
	else
	    exp = qs->RawData.String;
	stmt->WhereClause = expCompileExpression(exp,this->ObjList, MLX_F_ICASER | MLX_F_FILENAMES, EXPR_CMP_OUTERJOIN);
	if (!stmt->WhereClause)
	    {
	    mssError(0,"MQ","Error in query's WHERE clause");
	    goto error;
	    }
	for(i=0;i<this->nProvidedObjects;i++)
	    {
	    expFreezeOne(stmt->WhereClause, this->ObjList, i);
	    }

	if (qs)
	    {
	    /** Convert the where clause OR elements to UNION elements **/
	    mq_internal_ProcessWhere(stmt->WhereClause, stmt->WhereClause, &(stmt->QTree));

	    /** Break the where clause into tiny little chunks **/
	    select_qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, NULL);
	    update_qs = mq_internal_FindItem(stmt->QTree, MQ_T_UPDATECLAUSE, NULL);
	    mq_internal_DetermineCoverage(stmt, stmt->WhereClause, qs, select_qs, update_qs, 0);
	    if (stmt->WhereClause->Flags & EXPR_F_CVNODE) stmt->WhereClause = NULL;

	    /** Optimize the "little chunks" **/
	    for(i=0;i<qs->Children.nItems;i++)
	        {
		sub_qs = (pQueryStructure)(qs->Children.Items[i]);
		mq_internal_OptimizeExpr(&(sub_qs->Expr));
		}
	    }

	/** Build the one-object list for evaluating the Having clause, etc. **/
	stmt->OneObjList = expCreateParamList();
	stmt->OneObjList->Session = stmt->Query->SessionID;
	expAddParamToList(stmt->OneObjList, "this", NULL, 0);
	expSetParamFunctions(stmt->OneObjList, "this", mqGetAttrType, mqGetAttrValue, mqSetAttrValue);

	/** Compile the having expression, if one. **/
	stmt->HavingClause = NULL;
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_HAVINGCLAUSE, NULL);
	if (qs)
	    {
	    qs->Expr = expCompileExpression(qs->RawData.String, stmt->OneObjList, MLX_F_ICASER | MLX_F_FILENAMES, 0);
	    if (!qs->Expr)
	        {
		mssError(0,"MQ","Error in query's HAVING clause");
		goto error;
		}
	    stmt->HavingClause = qs->Expr;
	    qs->Expr = NULL;
	    }

	/** Limit clause? **/
	qs = mq_internal_FindItem(stmt->QTree, MQ_T_LIMITCLAUSE, NULL);
	if (qs)
	    {
	    stmt->LimitStart = qs->IntVals[0];
	    stmt->LimitCnt = qs->IntVals[1];
	    }
	else
	    {
	    stmt->LimitStart = 0;
	    stmt->LimitCnt = 0x7FFFFFFF;
	    }

	/** Ok, got the from, select, and where built.  Now call the mq-drivers **/
	for(i=0;i<MQINF.Drivers.nItems;i++)
	    {
	    qdrv = (pQueryDriver)(MQINF.Drivers.Items[i]);
	    if (qdrv->Analyze(stmt) < 0)
	        {
		goto error;
		}
	    }

	/** Build dependency mask **/
	mq_internal_SetCoverage(stmt, stmt->Tree);
	mq_internal_SetDependencies(stmt, stmt->Tree, NULL);

	/** Have the MQ drivers start the query. **/
	if (stmt->Tree->Driver->Start(stmt->Tree, stmt, NULL) < 0)
	    {
	    mssError(0,"MQ","Could not start the query");
	    goto error;
	    }

	/** Query is now "running" **/
	stmt->Flags &= ~MQ_TF_FINISHED;

	/** Issuing a Start can implicitly modify the object list, so
	 ** we save the new copy
	 **/
	/*memcpy(&this->CurObjList, this->ObjList, sizeof(ParamObjects));*/
	this->CntSerial = ++this->CurSerial;

	return 1;

    error:
	return -1;
    }


/*** mqStartQuery - starts a new MultiQuery.  The function of this routine
 *** is similar to the objOpenQuery; but this is called from objMultiQuery,
 *** and is not associated with any given object when it is opened.  Returns
 *** a pointer to a MultiQuery structure.
 ***/
void*
mqStartQuery(pObjSession session, char* query_text, pParamObjects objlist, int flags)
    {
    pMultiQuery this;

    	/** Allocate the multiquery structure itself. **/
	this = (pMultiQuery)nmMalloc(sizeof(MultiQuery));
	if (!this) return NULL;
	memset(this,0,sizeof(MultiQuery));
	this->CntSerial = 0;
	this->CurSerial = 0;
	this->SessionID = session;
	this->LinkCnt = 1;
	this->ObjList = NULL;
	this->nProvidedObjects = 0;
	this->QueryText = nmSysStrdup(query_text);
	this->RowCnt = 0;
	this->QueryCnt = 0;
	if (flags & OBJ_MQ_F_ONESTATEMENT)
	    {
	    this->Flags = MQ_F_ONESTATEMENT; /* multi statements disabled, cannot be enabled */
	    }
	else
	    {
	    this->Flags = MQ_F_MULTISTATEMENT; /* on by default, can be turned on/off */
	    }

	/** Parse the text of the query, building the syntax structure **/
	this->LexerSession = mlxStringSession(this->QueryText, 
		MLX_F_CCOMM | MLX_F_DASHCOMM | MLX_F_ICASER | MLX_F_FILENAMES | MLX_F_EOF);
	if (!this->LexerSession)
	    {
	    mssError(0,"MQ","Could not begin analysis of query text");
	    goto error;
	    }

	/** Import any externally provided data sources **/
	this->ObjList = expCreateParamList();
	if (objlist)
	    {
	    expCopyList(objlist, this->ObjList);
	    /*expLinkParams(this->ObjList, 0, -1);*/
	    this->nProvidedObjects = this->ObjList->nObjects;
	    this->ProvidedObjMask = (1<<(this->nProvidedObjects)) - 1;
	    }
	this->ObjList->Session = this->SessionID;

	/** Parse one SQL statement **/
	if (mq_internal_NextStatement(this) != 1)
	    goto error;

	return (void*)this;

    error:
	mq_internal_QueryClose(this, NULL);
	return NULL;
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
    int n;
    pMultiQuery mq;

    	/** Close the query **/
	n = inf->Stmt->Query->nProvidedObjects;
	mq = inf->Stmt->Query;
	mq_internal_CloseStatement(inf->Stmt);
	mq_internal_QueryClose(mq, oxt);

	/** Release all objects in our param list **/
	/*for(i=n;i<inf->ObjList.nObjects;i++) if (inf->ObjList.Objects[i])
	    {
	    objRequestNotify(obj, mq_internal_UpdateNotify, inf, 0);
	    }*/
	expUnlinkParams(inf->ObjList, n, -1);
	expFreeParamList(inf->ObjList);

	/** Release the pseudo-object **/
	nmFree(inf, sizeof(PseudoObject));

    return 0;
    }


/*** mqDeleteObj - delete an open query result item.  This only
 *** works on queries off of a single source.
 ***/
int
mqDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject inf = (pPseudoObject)inf_v;
    int rval;
    int objid = 0;
    pQueryStructure from_qs;
    int n;

	/** Do a 'delete obj' operation on the source object **/
	objid = -1;
	n = inf->ObjList->nObjects - inf->Stmt->Query->nProvidedObjects;
	if (n == 0)
	    {
	    mssError(1,"MQ","Could not delete - query must have at least one FROM source");
	    return -1;
	    }
	else if (n > 1)
	    {
	    from_qs = NULL;
	    while((from_qs = mq_internal_FindItem(inf->Stmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
		{
		if (from_qs->QELinkage && from_qs->QELinkage->SrcIndex >= 0 && (from_qs->Flags & MQ_SF_IDENTITY) && inf->ObjList->Objects[from_qs->QELinkage->SrcIndex])
		    {
		    objid = from_qs->QELinkage->SrcIndex;
		    break;
		    }
		}
	    }
	else if (n == 1)
	    {
	    objid = inf->Stmt->Query->nProvidedObjects;
	    }
	if (objid < 0)
	    {
	    mssError(1,"MQ","Could not delete - multi-source query must have one valid FROM source labled as the query IDENTITY source");
	    return -1;
	    }
	rval = objDeleteObj((pObject)(inf->ObjList->Objects[objid]));
	inf->ObjList->Objects[objid] = NULL;
	if (rval < 0) return rval;
	
    return mqClose(inf_v, oxt);
    }


pPseudoObject
mq_internal_CreatePseudoObject(pMultiQuery qy, pObject hl_obj)
    {
    pPseudoObject p;

	/** Allocate the pseudo-object. **/
	p = (pPseudoObject)nmMalloc(sizeof(PseudoObject));
	if (!p) return NULL;
	p->Stmt = qy->CurStmt;
	p->Obj = hl_obj;
	qy->LinkCnt++;
	qy->CurStmt->LinkCnt++;
	p->ObjList = expCreateParamList();

	/** Record the Counters **/
	p->QueryID = qy->QueryCnt - 1;
	p->RowIDAllQuery = qy->RowCnt;
	p->RowIDThisQuery = qy->CurStmt->UserIterCnt;
	p->RowIDBeforeLimit = qy->CurStmt->IterCnt + 1;

	/** Copy the object list and link to the objects.
	 ** We don't link to objects for the qy->CurObjList since that
	 ** objlist normally shadows QTree->ObjList, except when the
	 ** user skips back to previous objects temporarily and then
	 ** goes and does another fetch.
	 **/
	/*memcpy(&p->ObjList, qy->ObjList, sizeof(ParamObjects));
	memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));
	for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList.nObjects;i++) 
	    {
	    obj = (pObject)(p->ObjList.Objects[i]);
	    if (obj) objLinkTo(obj);
	    }*/
	expCopyList(qy->ObjList, p->ObjList);
	expLinkParams(p->ObjList, p->Stmt->Query->nProvidedObjects, -1);

	p->Serial = qy->CurSerial;

    return p;
    }


int
mq_internal_FreePseudoObject(pPseudoObject p)
    {
    pMultiQuery qy = p->Stmt->Query;

	mq_internal_CloseStatement(p->Stmt); /* unlink */
	/*qy->LinkCnt--;*/
	mq_internal_QueryClose(qy, NULL); /* unlink */

	expUnlinkParams(p->ObjList, qy->nProvidedObjects, -1);
	expFreeParamList(p->ObjList);

	nmFree(p, sizeof(PseudoObject));

    return 0;
    }


/*** return 1 if LIMIT matches, 0 if LIMIT does not match, or -1 if error.
 ***/
int
mq_internal_EvalLimitClause(pQueryStatement stmt, pPseudoObject p)
    {
    return stmt->IterCnt >= stmt->LimitStart && stmt->UserIterCnt < stmt->LimitCnt;
    }


/*** return 1 if HAVING matches, 0 if HAVING does not match, or -1 if error.
 ***/
int
mq_internal_EvalHavingClause(pQueryStatement stmt, pPseudoObject p)
    {

	if (!stmt->HavingClause) return 1;
	expModifyParam(stmt->OneObjList, "this", (void*)p);
	if (expEvalTree(stmt->HavingClause, stmt->OneObjList) < 0)
	    {
	    mssError(1,"MQ","Could not evaluate HAVING clause");
	    return -1;
	    }
	if (stmt->HavingClause->DataType != DATA_T_INTEGER)
	    {
	    mssError(1,"MQ","HAVING clause must evaluate to boolean or integer");
	    return -1;
	    }
	if (!(stmt->HavingClause->Flags & EXPR_F_NULL) && stmt->HavingClause->Integer != 0) 
	    {
	    return 1;
	    }

    return 0;
    }


/*** mqQueryFetch - retrieves the next item in the query result set.
 ***/
void*
mqQueryFetch(void* qy_v, pObject highlevel_obj, int mode, pObjTrxTree* oxt)
    {
    pPseudoObject p;
    pMultiQuery qy = (pMultiQuery)qy_v;
    int i, rval;
    pObject obj;

	/** Make sure the cur objlist is correct **/
	/*if (qy->CurSerial != qy->CntSerial)
	    {
	    qy->CurSerial = qy->CntSerial;
	    memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
	    }*/

	if (!qy->CurStmt) return NULL;

	/** Search for results matching the HAVING clause **/
	while(1)
	    {
	    /** We're about to modify the objlist implicitly - bump the
	     ** counter so it doesn't match any existing pseudo-objects.
	     **/
	    qy->CurSerial++;

    	    /** Try to fetch the next record. **/
	    if (qy->CurStmt->UserIterCnt < qy->CurStmt->LimitCnt)
		{
		rval = qy->CurStmt->Tree->Driver->NextItem(qy->CurStmt->Tree, qy->CurStmt);
		/*memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));*/
		}
	    else
		{
		rval = 0; /* limit-end of results, don't fetch */
		}
	    qy->CntSerial = qy->CurSerial;

	    if (rval != 1)
	        {
		if (rval == 0)
		    {
		    /** End of results from that statement - go to next statement **/
		    mq_internal_FinishStatement(qy->CurStmt);
		    mq_internal_CloseStatement(qy->CurStmt);
		    qy->CurStmt = NULL;
		    rval = mq_internal_NextStatement(qy);
		    if (rval != 1)
			{
			return NULL;
			}
		    continue;
		    /*rval = qy->CurStmt->Tree->Driver->NextItem(qy->CurStmt->Tree, qy->CurStmt);*/
		    }
		/*if (rval != 1)
		    {
		    memcpy(&qy->CurObjList, qy->ObjList, sizeof(ParamObjects));
		    qy->CntSerial = qy->CurSerial;
		    return NULL;
		    }*/
	        }

	    /** Create the pseudo object **/
	    p = mq_internal_CreatePseudoObject(qy, highlevel_obj);

	    /** Update row serial # **/
	    qy->CntSerial = qy->CurSerial;

	    /** Verify HAVING clause **/
	    rval = mq_internal_EvalHavingClause(qy->CurStmt, p);
	    if (rval == 1)
		{
		/** matched **/
		}
	    else if (rval == -1)
		{
		/** error **/
		mq_internal_FreePseudoObject(p);
		mssError(1,"MQ","Could not evaluate HAVING clause");
		return NULL;
		}
	    else
	        {
		/** no match, go get another row **/
		mq_internal_FreePseudoObject(p);
		p = NULL;
		continue;
		}

	    qy->CurStmt->IterCnt++;

	    /** Apply LIMIT clause **/
	    rval = mq_internal_EvalLimitClause(qy->CurStmt, p);
	    if (rval == 1)
		{
		/** matched **/
		break;
		}
	    else if (rval == -1)
		{
		/** error **/
		mq_internal_FreePseudoObject(p);
		mssError(1,"MQ","Could not evaluate LIMIT clause");
		return NULL;
		}
	    else
		{
		/** no match, go get another row **/
		mq_internal_FreePseudoObject(p);
		p = NULL;
		}
	    }

	/** If we have a pseudo-object, request update-notifies on its parts **/
	if (p)
	    {
	    for(i=qy->nProvidedObjects;i<p->ObjList->nObjects;i++) 
	        {
	        obj = (pObject)(p->ObjList->Objects[i]);
		/*if (obj) 
		    objRequestNotify(obj, mq_internal_UpdateNotify, p, OBJ_RN_F_ATTRIB);*/
		}
	    }

	qy->RowCnt++;
	qy->CurStmt->UserIterCnt++;

    return (void*)p;
    }


/*** mqQueryCreate - create a new object in the context of a running query;
 *** this requires object creation at one or more underlying levels depending
 *** on the nature of the query's joins and so forth.
 ***/
void*
mqQueryCreate(void* qy_v, pObject new_obj, char* name, int mode, int permission_mask, pObjTrxTree *oxt)
    {
    pPseudoObject p;
    pMultiQuery qy = (pMultiQuery)qy_v;

	/** For now, we just fail if this involves a join operation. **/
	if (qy->ObjList->nObjects - qy->nProvidedObjects > 1)
	    {
	    mssError(1,"MQ","Bark! QueryCreate() on a multi-source query is not yet supported.");
	    return NULL;
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

	/** Make sure the cur objlist is correct, otherwise the Finish
	 ** routines in the drivers might close up incorrect objects.
	 **/
	/*if (qy->CurSerial != qy->CntSerial)
	    {
	    qy->CurSerial = qy->CntSerial;
	    memcpy(qy->ObjList, &qy->CurObjList, sizeof(ParamObjects));
	    }*/
	qy->CurSerial = (++qy->CntSerial);

	/** No more results possible after QueryClose(), so we call
	 ** Finish() on the statement.
	 **/
	if (qy->CurStmt)
	    mq_internal_FinishStatement(qy->CurStmt);

	/** Shutdown the query structure **/
	mq_internal_QueryClose(qy, oxt);

    return 0;
    }


/*** mq_internal_QueryClose() - close the query; may be called by
 *** mqQueryClose or by mqClose.
 ***/
int
mq_internal_QueryClose(pMultiQuery qy, pObjTrxTree* oxt)
    {

    	/** Check the link cnt **/
	if ((--qy->LinkCnt) > 0) return 0;

	/** Release resources used by the one SQL statement **/
	if (qy->CurStmt)
	    mq_internal_CloseStatement(qy->CurStmt);

	/** Release the object list for the main query **/
	if (qy->ObjList)
	    {
	    /*expUnlinkParams(qy->ObjList, 0, -1);*/
	    expFreeParamList(qy->ObjList);
	    }

	if (qy->QueryText)
	    nmSysFree(qy->QueryText);

	if (qy->LexerSession)
	    mlxCloseSession(qy->LexerSession);

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
	if (!strcmp(attrname,"ls__rowid") || !strcmp(attrname,"cx__rowid") ||
	    !strcmp(attrname,"cx__queryid") || !strcmp(attrname,"cx__rowid_one_query") ||
	    !strcmp(attrname,"cx__rowid_before_limit")) return DATA_T_INTEGER;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    dt = objGetAttrType(p->ObjList->Objects[i], attrname);
		    if (dt > 0)
			return dt;
		    }
		}
	    }
	    
	if (id == -1) 
	    {
	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
	        return -1;
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expression to get the data type **/
	if (expEvalTree((pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id],p->ObjList) < 0)
	    return -1;
	dt = ((pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id])->DataType;

    return dt;
    }


/*** mqGetAttrValue - retrieves the value of an attribute.  See the object system
 *** documentation on the appropriate pointer types for *value.
 ***
 *** Returns: -1 on error, 0 on success, and 1 if successful but value was NULL.
 ***/
int
mqGetAttrValue(void* inf_v, char* attrname, int datatype, void* value, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int id=-1,i, rval;
    pExpression exp;
    pQueryStructure from_qs;

    	/** Request for row id? **/
	if (!strcmp(attrname,"ls__rowid") || !strcmp(attrname,"cx__rowid") ||
	    !strcmp(attrname,"cx__queryid") || !strcmp(attrname,"cx__rowid_one_query") ||
	    !strcmp(attrname,"cx__rowid_before_limit"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"MQ","Type mismatch getting attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    if (!strcmp(attrname,"ls__rowid"))
		*((int*)value) = p->Serial;
	    else if (!strcmp(attrname,"cx__rowid"))
		*((int*)value) = p->RowIDAllQuery;
	    else if (!strcmp(attrname,"cx__queryid"))
		*((int*)value) = p->QueryID;
	    else if (!strcmp(attrname,"cx__rowid_one_query"))
		*((int*)value) = p->RowIDThisQuery;
	    else if (!strcmp(attrname,"cx__rowid_before_limit"))
		*((int*)value) = p->RowIDBeforeLimit;
	    return 0;
	    }

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    rval = objGetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
		    if (rval >= 0)
			return rval;
		    }
		}
	    }
	    
	if (id == -1)
	    {
	    /** Suppress the error message on certain attrs **/
	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
		{
		/** for 'system' attrs, try to find 'identity' source **/
		from_qs = NULL;
		i = -1;
		while((from_qs = mq_internal_FindItem(p->Stmt->QTree, MQ_T_FROMSOURCE, from_qs)) != NULL)
		    {
		    if (from_qs->QELinkage && from_qs->QELinkage->SrcIndex >= 0 && (from_qs->Flags & MQ_SF_IDENTITY) && p->ObjList->Objects[from_qs->QELinkage->SrcIndex])
			{
			i = from_qs->QELinkage->SrcIndex;
			break;
			}
		    }
	        if (i == -1) return -1;
		rval = objGetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
		return rval;
		}
	    else
		{
		mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
		return -1;
		}
	    }

	/** Evaluate the expression to get the value **/
	exp = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (expEvalTree(exp,p->ObjList) < 0) return 1;
	if (exp->Flags & EXPR_F_NULL) return 1;
	if (exp->DataType != datatype)
	    {
	    mssError(1,"MQ","Type mismatch getting attribute '%s' [requested=%s, actual=%s]", 
		    attrname,obj_type_names[datatype],obj_type_names[exp->DataType]);
	    return -1;
	    }
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


/*** mq_internal_QEGetNextAttr - get the next attribute available from the
 *** query exec tree element.  Uses two externals - attrid and astobjid - to
 *** track iteration state.  If astobjid == -1 on return, then the attribute
 *** is internal, otherwise it is via "select *" and iterated from the given
 *** astobjid object.
 ***/
char*
mq_internal_QEGetNextAttr(pMultiQuery mq, pQueryElement qe, pParamObjects objlist, int* attrid, int* astobjid)
    {
    char* attrname = NULL;

    	/** Check overflow... **/
	while(!attrname)
	    {
	    if (*attrid >= qe->AttrNames.nItems) return NULL;

	    /** Asterisk? **/
	    attrname = qe->AttrNames.Items[*attrid];
	    if (!strcmp(attrname,"*"))
		{
		attrname = NULL;
		while(!attrname)
		    {
		    if (*astobjid == -1)
			{
			/** First non-external object **/
			if (mq->nProvidedObjects < objlist->nObjects && objlist->Objects[mq->nProvidedObjects])
			    attrname = objGetFirstAttr(objlist->Objects[mq->nProvidedObjects]);
			*astobjid = mq->nProvidedObjects;
			}
		    else	
			{
			if (objlist->Objects[*astobjid])
			    attrname = objGetNextAttr(objlist->Objects[*astobjid]);
			}
		    if (attrname == NULL)
			{
			(*astobjid)++;
			if (*astobjid >= objlist->nObjects)
			    {
			    *astobjid = -1;
			    (*attrid)++;
			    break;
			    }
			if (objlist->Objects[*astobjid])
			    attrname = objGetFirstAttr(objlist->Objects[*astobjid]);
			}
		    }
		}
	    else
		{
		(*attrid)++;
		}
	    }

    return attrname;
    }


/*** mqGetNextAttr - returns the name of the _next_ attribute.  This function, and
 *** the previous one, both return NULL if there are no (more) attributes.
 ***/
char*
mqGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

    return mq_internal_QEGetNextAttr(p->Stmt->Query, p->Stmt->Tree, p->ObjList, &p->AttrID, &p->AstObjID);
    }


/*** mqGetFirstAttr - returns the name of the first attribute in the multiquery
 *** result set, and preparse mqGetNextAttr to return subsequent attributes.
 ***/
char*
mqGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;

    	/** Set attr id, and return next attr **/
	p->AstObjID = -1;
	p->AttrID = 0;

    return mqGetNextAttr(inf_v, oxt);
    }


/*** mqSetAttrValue - sets the value of an attribute.  In order for this to work,
 *** the query must have been opened FOR UPDATE.
 ***/
int
mqSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData value, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    int i, id, rval, dt;
    pExpression exp;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute needs updating... **/
	id = -1;
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    dt = objGetAttrType(p->ObjList->Objects[i], attrname);
		    if (dt > 0)
			{
			rval = objSetAttrValue(p->ObjList->Objects[i], attrname, datatype, value);
			return rval;
			}
		    }
		}
	    }
	    
	if (id == -1)
	    {
	    mssError(1,"MQ","setattr: unknown attribute '%s' for multiquery result set", attrname);
	    return -1;
	    }

	/** Evaluate the expr first to find the data type.  This isn't
	 ** ideal, but it'll work for now.
	 **/
	exp = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (expEvalTree(exp,p->ObjList) < 0) return -1;
	if (exp->DataType == DATA_T_UNAVAILABLE) return -1;

	/** Verify that the types match. **/
	if (exp->DataType != datatype)
	    {
	    mssError(1,"MQ","Type mismatch setting attribute '%s' [requested=%s, actual=%s]", 
		    attrname,obj_type_names[datatype],obj_type_names[exp->DataType]);
	    return -1;
	    }

	/** Set the expression result to the given value. **/
	exp->Flags &= ~(EXPR_F_NULL);
	switch(exp->DataType)
	    {
	    case DATA_T_INTEGER: 
		exp->Integer = value->Integer; 
		break;
	    case DATA_T_STRING: 
		if (exp->Alloc && exp->String)
		    {
		    nmSysFree(exp->String);
		    exp->Alloc = 0;
		    }
		if (strlen(value->String) > 63)
		    {
		    exp->Alloc = 1;
		    exp->String = nmSysMalloc(strlen(value->String)+1);
		    }
		else
		    {
		    exp->String = exp->Types.StringBuf;
		    }
		strcpy(exp->String, value->String);
		break;
	    case DATA_T_DOUBLE: 
		exp->Types.Double = value->Double; 
		break;
	    case DATA_T_MONEY: 
		memcpy(&(exp->Types.Money), value->Money, sizeof(MoneyType));
		break;
	    case DATA_T_DATETIME: 
		memcpy(&(exp->Types.Date), value->DateTime, sizeof(DateTime));
		break;
	    }

	/** Evaluate the expression in reverse to set the value!! **/
	if (expReverseEvalTree(exp, p->ObjList) < 0) return -1;

    return 0;
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


/*** mqCommit - commit changes.  Basically, run 'commit' on all underlying
 *** objects for the query.
 ***/
int
mqCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    /*int i;*/

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Commit each underlying object **/
	objCommit(p->Stmt->Query->SessionID);

    return 0;
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
	xaAddItemSortedInt32(&MQINF.Drivers, (void*)drv, offsetof(QueryDriver, Precedence));

    return 0;
    }

/*** mqPresentationHints
 ***/
pObjPresentationHints
mqPresentationHints(void* inf_v, char* attrname, pObjTrxTree* otx)
    {
    pPseudoObject p = (pPseudoObject)inf_v;
    pObjPresentationHints ph;
    pObject obj;
    int i, id;
    pExpression e;

    	/** Request for ls__rowid? **/
	if (!strcmp(attrname,"ls__rowid")) return NULL;

    	/** Check to see whether we're on current object. **/
	/*mq_internal_CkSetObjList(p->Stmt->Query, p);*/

	/** Figure out which attribute... **/
	id = -1;
	for(i=0;i<p->Stmt->Tree->AttrNames.nItems;i++)
	    {
	    if (!strcmp(attrname,p->Stmt->Tree->AttrNames.Items[i]))
	        {
		id = i;
		break;
		}
	    }

	/** If select *, then loop through FROM objects **/
	if (id == -1 && (p->Stmt->Flags & MQ_TF_ASTERISK))
	    {
	    for(i=p->Stmt->Query->nProvidedObjects;i<p->ObjList->nObjects;i++)
		{
		if (p->ObjList->Objects[i])
		    {
		    ph = objPresentationHints(p->ObjList->Objects[i], attrname);
		    if (ph)
			return ph;
		    }
		}
	    }
	    
	if (id == -1) 
	    {
	    if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname, "outer_type") || !strcmp(attrname, "annotation"))
	        return NULL;
	    mssError(1,"MQ","Unknown attribute '%s' for multiquery result set", attrname);
	    return NULL;
	    }

	/** Evaluate the expression to get the data type **/
	p->ObjList->Session = p->Stmt->Query->SessionID;
	e = (pExpression)p->Stmt->Tree->AttrCompiledExpr.Items[id];
	if (e->NodeType == EXPR_N_PROPERTY || e->NodeType == EXPR_N_OBJECT)
	    {
	    if (e->ObjID >= 0)
		{
		obj = p->ObjList->Objects[(int)e->ObjID];
		if (obj && p->ObjList->GetTypeFn[(int)e->ObjID] == objGetAttrType)
		    {
		    ph = objPresentationHints(obj, attrname);
		    return ph;
		    }
		}
	    }

    return NULL;
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
	drv->DeleteObj = mqDeleteObj;
	drv->OpenQuery = mqStartQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = mqQueryFetch;
	drv->QueryCreate = mqQueryCreate;
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
	drv->PresentationHints = mqPresentationHints;
	drv->Commit = mqCommit;

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


