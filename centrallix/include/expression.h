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


#include "obj.h"
#include "cxlib/mtlexer.h"
#include "cxlib/xhash.h"
#include <openssl/sha.h>
#include <openssl/md5.h>

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
    unsigned char Random[SHA256_DIGEST_LENGTH];
    }
    EXP_Globals;

extern EXP_Globals EXP;


/** expression tree control structure **/
typedef struct _EC
    {
    int			LinkCnt;
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
    void*		PrivateData;		/* allocated with nmSysMalloc() */
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
    unsigned int	MainFlags;		/* bitmask EXPR_MO_xxx */
    unsigned int 	PSeqID;
    int			ModCoverageMask;
    pExpControl		CurControl;
    int			RandomInit;
    unsigned char	Random[SHA256_DIGEST_LENGTH];		/* current seed for rand() */
    }
    ParamObjects, *pParamObjects;


/*** Parameter-object flags ***/
#define EXPR_O_UPDATE		1	/* object is updateable */
#define EXPR_O_CHANGED		2	/* expModify called. */
#define EXPR_O_CURRENT		4	/* object has focus context */
#define EXPR_O_PARENT		8	/* object is parent of current */
#define EXPR_O_ALLOCNAME	16
#define EXPR_O_REFERENCED	32	/* object was referenced */
#define EXPR_O_ALLOWDUPS	64	/* allow duplicate object names */
#define EXPR_O_REPLACE		128	/* replace entry on duplicate */

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
#define EXPR_N_ISNOTNULL	25

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

/*** Expression objlist MainFlags ***/
#define EXPR_MO_RECALC		1	/* ignore EXPR_F_STALE; recalc */
#define EXPR_MO_RUNSTATIC	EXPR_F_RUNSTATIC
#define EXPR_MO_RUNSERVER	EXPR_F_RUNSERVER
#define EXPR_MO_RUNCLIENT	EXPR_F_RUNCLIENT
#define EXPR_MO_DOMAINMASK	EXPR_F_DOMAINMASK

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
int expCompareExpressions(pExpression exp1, pExpression exp2);
int expCompareExpressionValues(pExpression exp1, pExpression exp2);

/*** Generator functions ***/
int expGenerateText(pExpression exp, pParamObjects objlist, int (*write_fn)(), void* write_arg, char esc_char, char* language, int domain);

/*** Internal Functions ***/
pExpression exp_internal_CompileExpression_r(pLxSession lxs, int level, pParamObjects objlist, int cmpflags);
int expObjID(pExpression exp, pParamObjects objlist);
int exp_internal_CopyNode(pExpression src, pExpression dst);
pExpression exp_internal_CopyTree(pExpression orig_exp);
int expSplitTree(pExpression src_tree, pExpression split_point, pExpression result_trees[]);
int exp_internal_EvalTree(pExpression tree, pParamObjects objlist);
int exp_internal_EvalAggregates(pExpression tree, pParamObjects objlist);
int exp_internal_DefineFunctions();
int exp_internal_DefineNodeEvals();
int expCopyValue(pExpression src, pExpression dst, int make_independent);
int expAddNode(pExpression parent, pExpression child);
int expDataTypeToNodeType(int data_type);
int exp_internal_SetupControl(pExpression exp);
pExpControl exp_internal_LinkControl(pExpControl ctl);
int exp_internal_UnlinkControl(pExpControl ctl);


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
int expSetEvalDomain(pParamObjects this, int domain);
int expCopyList(pParamObjects src, pParamObjects dst, int n_objects);
int expCopyParams(pParamObjects src, pParamObjects dst, int start, int n_objects);
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
void expLinkParams(pParamObjects this, int start, int end);
void expUninkParams(pParamObjects this, int start, int end);

#endif /* not defined _EXPRESSION_H */
