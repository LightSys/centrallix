#ifndef _EXPRESSION_H
#define _EXPRESSION_H

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
/* Module: 	expression.h, expression.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: expression.h,v 1.20 2010/09/08 22:01:25 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/expression.h,v $

    $Log: expression.h,v $
    Revision 1.20  2010/09/08 22:01:25  gbeeley
    - (bugfix) allow /file/name:"attribute" to be quoted.
    - (bugfix) order by ... asc/desc keywords are now case insenstive
    - (bugfix) short-circuit eval was not resulting in aggregates properly
      evaluating
    - (change) new API function expModifyParamByID - use this for efficiency
    - (feature) multi-level aggregate functions now supported, for use when
      a sql query has a group by, e.g. select max(sum(...)) ... group by ...
    - (feature) added mathematical and trig functions radians, degrees, sin,
      cos, tan, asin, acos, atan, atan2, sqrt, square

    Revision 1.19  2010/01/10 07:33:23  gbeeley
    - (performance) reduce the number of times that subqueries are executed by
      only re-evaluating them if one of the ObjList entries has changed
      (instead of re-evaluating every time).  Ideally we should check for what
      objects are referenced by the subquery, but that is for a later fix...

    Revision 1.18  2009/06/24 17:33:19  gbeeley
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

    Revision 1.17  2008/09/14 05:17:26  gbeeley
    - (bugfix) subquery evaluator was leaking query handles if subquery did
      not return any rows.
    - (change) add ability to generate expression text based on the domain of
      evaluation (client, server, etc.)

    Revision 1.16  2008/03/29 02:26:15  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.15  2008/02/25 23:14:33  gbeeley
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

    Revision 1.14  2007/03/21 04:48:09  gbeeley
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

    Revision 1.13  2007/03/04 05:04:47  gbeeley
    - (change) This is a change to the way that expressions track which
      objects they were last evaluated against.  The old method was causing
      some trouble with stale data in some expressions.

    Revision 1.12  2005/09/30 04:37:10  gbeeley
    - (change) modified expExpressionToPod to take the type.
    - (feature) got eval() working
    - (addition) added expReplaceString() to search-and-replace in an
      expression tree.

    Revision 1.11  2005/02/26 06:42:38  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.10  2004/06/12 04:02:27  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.9  2004/02/24 20:28:09  gbeeley
    - OOPS!  my commit log message messed up the comment structure in
      this file!

    Revision 1.8  2004/02/24 20:23:00  gbeeley
    - external reference coverage-mask support to go along with changes to
      expression/{star}.c files

    Revision 1.7  2003/06/27 21:19:47  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

    Revision 1.6  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.5  2003/04/24 02:13:21  gbeeley
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

    Revision 1.2  2001/10/02 16:23:50  gbeeley
    Added expGenerateText().

    Revision 1.1.1.1  2001/08/13 18:00:52  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:19  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "obj.h"
#include "cxlib/mtlexer.h"
#include "cxlib/xhash.h"

#define EXPR_MAX_PARAMS		30


/** Global structure definition **/
typedef struct
    {
    int         PSeqID;
    int		ModSeqID;
    int         (*(EvalFunctions[64]))();
    int         Precedence[64];
    XHashTable  Functions;
    XHashTable  ReverseFunctions;
    }
    EXP_Globals;

extern EXP_Globals EXP;


/** expression tree control structure **/
typedef struct _EC
    {
    int			Remapped:1;
    int			PSeqID;
    int			ObjSeqID[EXPR_MAX_PARAMS];
    char		ObjMap[EXPR_MAX_PARAMS]; /* id or EXPR_CTL_xxx */
    }
    ExpControl, *pExpControl;

#define EXPR_CTL_CURRENT	(-2)
#define EXPR_CTL_PARENT		(-3)
#define EXPR_CTL_CONSTANT	(-4)


/** expression tree structure **/
typedef struct _ET
    {
    int			Magic;
    struct _ET*		Parent;
    XArray		Children;
    unsigned char	NodeType;		/* one-of EXPR_N_xxx */
    unsigned char	DataType;		/* one-of DATA_T_xxx */
    unsigned char	CompareType;		/* bitmask MLX_CMP_xxx */
    char		ObjID;
    int			Alloc;
    int			NameAlloc;
    pExpControl		Control;
    char*		Name;
    int			Flags;			/* bitmask EXPR_F_xxx */
    int			Integer;
    char*		String;
    union
        {
        char		StringBuf[64];
        double		Double;
        MoneyType	Money;
        DateTime	Date;
        StringVec	StrVec;
        IntVec		IntVec;
	}
	Types;
    int			ObjCoverageMask;
    int			ObjDelayChangeMask;
    int			ObjOuterMask;
    int			AggCount;
    int			AggValue;
    unsigned char	AggLevel;
    struct _ET*		AggExp;
    unsigned int	ListCount;
    unsigned int	ListTrackCnt;
    unsigned int	LinkCnt;
    unsigned int	LxFlags;
    unsigned int	CmpFlags;
    }
    Expression, *pExpression;

#define	EXPR_OBJID_CURRENT	(-2)
#define EXPR_OBJID_PARENT	(-3)

/** Based on 30 parameters - 30 least significant bits **/
#define EXPR_OBJID_EXTREF	(31)
#define EXPR_MASK_EXTREF	(1<<31)
#define EXPR_OBJID_INDETERMINATE (30)
#define EXPR_MASK_INDETERMINATE	(1<<30)
#define EXPR_MASK_ALLOBJECTS	(0x3FFFFFFF)

#define EXPR(x) ((pExpression)(x))


/** parameter-object listing structure **/
typedef struct _PO
    {
    pObjSession		Session;
    pObject		Objects[EXPR_MAX_PARAMS];
    unsigned char	Flags[EXPR_MAX_PARAMS];		/* bitmask EXPR_O_xxx */
    char*		Names[EXPR_MAX_PARAMS];
    int			(*(GetTypeFn[EXPR_MAX_PARAMS]))();
    int			(*(GetAttrFn[EXPR_MAX_PARAMS]))();
    int			(*(SetAttrFn[EXPR_MAX_PARAMS]))();
    unsigned int 	SeqIDs[EXPR_MAX_PARAMS];
    unsigned char	nObjects;
    char		CurrentID;
    char		ParentID;
    unsigned char	MainFlags;		/* bitmask EXPR_MO_xxx */
    unsigned int 	PSeqID;
    int			ModCoverageMask;
    pExpControl		CurControl;
    }
    ParamObjects, *pParamObjects;


/*** Parameter-object flags ***/
#define EXPR_O_UPDATE		1	/* object is updateable */
#define EXPR_O_CHANGED		2	/* expModify called. */
#define EXPR_O_CURRENT		4	/* object has focus context */
#define EXPR_O_PARENT		8	/* object is parent of current */
#define EXPR_O_ALLOCNAME	16
#define EXPR_O_REFERENCED	32	/* object was referenced */

#define EXPR_MO_RECALC		1	/* ignore EXPR_F_STALE; recalc */

extern pParamObjects expNullObjlist;

/*** Types of expression nodes ***/
#define EXPR_N_FUNCTION		1
#define EXPR_N_MULTIPLY		2
#define EXPR_N_DIVIDE		3
#define EXPR_N_PLUS		4
#define EXPR_N_MINUS		5
#define EXPR_N_COMPARE		6
#define EXPR_N_IN		7
#define EXPR_N_LIKE		8
#define EXPR_N_CONTAINS		9
#define EXPR_N_ISNULL		10
#define EXPR_N_NOT		11
#define EXPR_N_AND		12
#define EXPR_N_OR		13
#define EXPR_N_TRUE		14
#define EXPR_N_FALSE		15
#define EXPR_N_INTEGER		16
#define EXPR_N_STRING		17
#define EXPR_N_DOUBLE		18
#define EXPR_N_DATETIME		19
#define EXPR_N_MONEY		20
#define EXPR_N_OBJECT		21
#define EXPR_N_PROPERTY		22
#define EXPR_N_LIST		23
#define EXPR_N_SUBQUERY		24	/* such as "i = (select ...)" */

/*** Flags for expression nodes ***/
#define EXPR_F_OPERATOR		1	/* node is an operator */
#define EXPR_F_NULL		2	/* node has a NULL value */
#define EXPR_F_NEW		4	/* node must be re-evaluated */
#define EXPR_F_CVNODE		8	/* used by coverage calculations */
#define EXPR_F_FREEZEEVAL	16	/* freeze values; do not do lookup */
#define EXPR_F_DESC		32	/* sort is descending */
#define EXPR_F_PERMNULL		64	/* node is permanently NULL. */
#define EXPR_F_AGGREGATEFN	128	/* node is an aggregate func. */
#define EXPR_F_LOUTERJOIN	256	/* left member is an outer member */
#define EXPR_F_ROUTERJOIN	512	/* right member is the outer member */
#define EXPR_F_AGGLOCKED	1024	/* Aggregate function already counted */
#define EXPR_F_DORESET		2048	/* Reset agg's on next Eval() */
#define EXPR_F_RUNCLIENT	4096	/* Run expression on client */
#define EXPR_F_RUNSERVER	8192	/* Run expression on server */
#define EXPR_F_RUNSTATIC	16384	/* Run expression as static on server */

#define EXPR_F_DOMAINMASK	(EXPR_F_RUNSTATIC | EXPR_F_RUNCLIENT | EXPR_F_RUNSERVER)
#define EXPR_F_RUNDEFAULT	(EXPR_F_RUNSTATIC)

#define EXPR_F_INDETERMINATE	32768	/* Value cannot yet be known */

#define EXPR_F_HASRUNSERVER	65536	/* Expression contains runserver() */

/*** Compiler flags ***/
#define EXPR_CMP_ASCDESC	1	/* flag asc/desc for sort expr */
#define EXPR_CMP_OUTERJOIN	2	/* allow =* and *= for == */
#define EXPR_CMP_WATCHLIST	4	/* watch for a list within () */
#define EXPR_CMP_LATEBIND	8	/* allow late object name binding */
#define EXPR_CMP_RUNSERVER	16	/* compile as a 'runserver' expression */
#define EXPR_CMP_RUNCLIENT	32	/* compile as a 'runclient' expression */


/*** Functions ***/
pExpression expAllocExpression();
int expFreeExpression(pExpression this);
pExpression expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags);
pExpression expCompileExpressionFromLxs(pLxSession s, pParamObjects objlist, int cmpflags);
pExpression expLinkExpression(pExpression this);
pExpression expPodToExpression(pObjData pod, int type, pExpression exp);
int expExpressionToPod(pExpression this, int type, pObjData pod);
pExpression expDuplicateExpression(pExpression this);
int expReplaceString(pExpression this, char* oldstr, char* newstr);
int expIsConstant(pExpression this);
pExpression expReducedDuplicate(pExpression this);

/*** Generator functions ***/
int expGenerateText(pExpression exp, pParamObjects objlist, int (*write_fn)(), void* write_arg, char esc_char, char* language, int domain);

/*** Internal Functions ***/
pExpression exp_internal_CompileExpression_r(pLxSession lxs, int level, pParamObjects objlist, int cmpflags);
int expObjID(pExpression exp, pParamObjects objlist);
int exp_internal_CopyNode(pExpression src, pExpression dst);
pExpression exp_internal_CopyTree(pExpression orig_exp);
int expSplitTree(pExpression src_tree, pExpression split_point, pExpression result_trees[]);
int exp_internal_EvalTree(pExpression tree, pParamObjects objlist);
int exp_internal_DefineFunctions();
int exp_internal_DefineNodeEvals();
int expCopyValue(pExpression src, pExpression dst, int make_independent);
int expAddNode(pExpression parent, pExpression child);
int expDataTypeToNodeType(int data_type);
int exp_internal_SetupControl(pExpression exp);


/*** Evaluator functions ***/
int expEvalIsNull(pExpression tree, pParamObjects objlist);
int expEvalAnd(pExpression tree, pParamObjects objlist);
int expEvalOr(pExpression tree, pParamObjects objlist);
int expEvalNot(pExpression tree, pParamObjects objlist);
int expEvalIn(pExpression tree, pParamObjects objlist);
int expEvalList(pExpression tree, pParamObjects objlist);
int expEvalCompare(pExpression tree, pParamObjects objlist);
int expEvalObject(pExpression tree, pParamObjects objlist);
int expEvalProperty(pExpression tree, pParamObjects objlist);
int expEvalTree(pExpression tree, pParamObjects objlist);
int expEvalContains(pExpression tree, pParamObjects objlist);
int expEvalDivide(pExpression tree, pParamObjects objlist);
int expEvalMultiply(pExpression tree, pParamObjects objlist);
int expEvalMinus(pExpression tree, pParamObjects objlist);
int expEvalPlus(pExpression tree, pParamObjects objlist);
int expEvalFunction(pExpression tree, pParamObjects objlist);
int expReverseEvalTree(pExpression tree, pParamObjects objlist);


/*** Param-object functions ***/
pParamObjects expCreateParamList();
int expCopyList(pParamObjects src, pParamObjects dst);
int expBindExpression(pExpression exp, pParamObjects this, int domain);
int expFreeParamList(pParamObjects this);
int expFreeParamListWithCB(pParamObjects this, int (*free_fn)());
int expAddParamToList(pParamObjects this, char* name, pObject obj, int flags);
int expModifyParam(pParamObjects this, char* name, pObject replace_obj);
int expModifyParamByID(pParamObjects this, int id, pObject replace_obj);
int expLookupParam(pParamObjects this, char* name);
int expSyncModify(pExpression tree, pParamObjects objlist);
int expReplaceID(pExpression tree, int oldid, int newid);
int expFreezeEval(pExpression tree, pParamObjects objlist, int freeze_id);
int expFreezeOne(pExpression tree, pParamObjects objlist, int freeze_id);
int expReplaceVariableID(pExpression tree, int newid);
int expResetAggregates(pExpression tree, int reset_id, int level);
int exp_internal_ResetAggregates(pExpression tree, int reset_id, int level);
int expUnlockAggregates(pExpression tree, int level);
int expRemoveParamFromList(pParamObjects this, char* name);
int expSetParamFunctions(pParamObjects this, char* name, int (*type_fn)(), int (*get_fn)(), int (*set_fn)());
int expRemapID(pExpression tree, int exp_obj_id, int objlist_obj_id);
int expClearRemapping(pExpression tree);
int expObjChanged(pParamObjects this, pObject obj);
int expContainsAttr(pExpression exp, int objid, char* attrname);
int expAllObjChanged(pParamObjects this);
int expSetCurrentID(pParamObjects this, int current_id);

#endif /* not defined _EXPRESSION_H */
