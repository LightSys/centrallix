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

    $Id: multiquery.h,v 1.3 2002/04/05 04:42:42 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/multiquery.h,v $

    $Log: multiquery.h,v $
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
#include "mtlexer.h"
#include "expression.h"
#include "xstring.h"


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
    XArray		Children;		/* child query items */
    XString		AttrNameBuf;		/* buffer for attr names */
    XArray		AttrNames;		/* ptrs to attribute names */
    XArray		AttrDeriv;		/* ptrs to qe where attr comes from */
    XArray		AttrExprPtr;		/* char* for each expression */
    XArray		AttrCompiledExpr;	/* pExpression ptrs for each attr */
    pQueryDriver	Driver;			/* driver handling this qe */
    int 		Flags;			/* bitmask MQ_EF_xxx */
    int			PreCnt;
    int			IterCnt;
    int			SlaveIterCnt;
    int			SrcIndex;
    int			SrcIndexSlave;
    pObject		LLSource;
    pObjQuery		LLQuery;
    void*		QSLinkage;
    pExpression		Constraint;
    pExpression		OrderBy[17];
    void*		PrivateData;		/* q-driver specific data structure */
    }
    QueryElement, *pQueryElement;

#define MQ_EF_ADDTLEXP		1		/* if addl expression supplied to Start */
#define MQ_EF_OUTERJOIN		2		/* if join, this is an outerjoin. */
#define MQ_EF_CONSTEXP		4		/* constraint expression supplied in QE */
#define MQ_EF_SLAVESTART	8		/* slave side of join was started */


/*** Structure for the syntactical analysis of the query text. ***/
typedef struct _QS
    {
    struct _QS*		Parent;
    int			NodeType;		/* one-of MQ_T_xxx */
    XArray		Children;
    char		Presentation[32];
    char		Source[256];
    char		Name[32];
    int			ObjFlags[16];
    int			ObjCnt;
    XString		RawData;
    pExpression		Expr;
    pParamObjects	ObjList;
    pQueryElement	QELinkage;
    int			Specificity;		/* source specificity */
    int			Flags;
    int			IntVals[2];		/* used by LIMIT clause */
    }
    QueryStructure, *pQueryStructure;

#define MQ_SF_USED		1		/* QS has been used by another q-drv. */

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


/*** Structure for managing the multiquery. ***/
typedef struct
    {
    pExpression		WhereClause;		/* where clause expression */
    pExpression		HavingClause;		/* having-clause expression */
    pQueryElement	Trees[16];		/* list of tree and subtrees */
    pQueryElement	Tree;			/* query exec main tree ptr */
    pQueryStructure	QTree;			/* query syntax tree head ptr */
    int			Flags;			/* bitmask MQ_F_xxx */
    int			LinkCnt;		/* number of opens on this */
    int			CntSerial;		/* serial number counter. */
    int			CurSerial;		/* reflects what the objlist has in it. */
    pObjSession		SessionID;
    pParamObjects	OneObjList;
    ParamObjects	CurObjList;		/* objlist used for next fetch */
    }
    MultiQuery, *pMultiQuery;

#define MQ_F_ALLOWUPDATE	1
#define MQ_F_NOMOREREC		2


/*** Pseudo-object structure. ***/
typedef struct
    {
    pMultiQuery		Query;
    ParamObjects	ObjList;
    int			Serial;
    int			AttrID;
    }
    PseudoObject, *pPseudoObject;


/*** Administrative functions ***/
int mqInitialize();
int mqRegisterQueryDriver(pQueryDriver drv);

/*** Query set-up functions ***/

/*** INTERNAL functions ***/
int mq_internal_FreeQS(pQueryStructure qstree);
pQueryStructure mq_internal_AllocQS(int type);
pQueryStructure mq_internal_FindItem(pQueryStructure tree, int type, pQueryStructure next);
pQueryElement mq_internal_AllocQE();
int mq_internal_FreeQE(pQueryElement qe);


#endif  /* not defined _MULTIQUERY_H */
