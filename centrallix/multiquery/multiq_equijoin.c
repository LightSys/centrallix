#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
/* Module: 	multiq_equijoin.c	 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 8, 1999    					*/
/* Description:	Provides support for joins, where the join is		*/
/*		fully resolved on a given data field, and is not a	*/
/*		cartesian-product type of join.				*/
/*		April 5, 2000 - added <n>-orquery optimization		*/
/************************************************************************/



/*** This tuning parameter affects how a join is performed.  The first of
 *** the parameters, MQJ_ENABLE_PREFETCH, turns on a mechanism by which the
 *** join mechanism will read <n> rows from the "master" query at a time and
 *** then build a long "thing=valueA or thing=valueB or thing=valueC" etc.
 *** type of additional-constraint to send to the slave query.  This process
 *** will repeat for each group of <n> or fewer rows until the master query
 *** is exhausted.  Note that this is typically only valid for equijoins, not
 *** for joins on greater-than or less-than.
 ***
 *** The idea is to improve performance with intelligent datasources (like
 *** Sybase) so that only 1/nth of the number of queries actually need to be
 *** sent to the intelligent data source.  Performance should be about the 
 *** same with non-intelligent data sources (like CSV files).
 ***/
#define MQJ_ENABLE_PREFETCH	0
#define MQJ_MAX_PREFETCH	16

#define MQJ_MAX_JOIN		(EXPR_MAX_PARAMS)
#define MQJ_MAX_SOURCE		(EXPR_MAX_PARAMS)


/*** Globals ***/
struct
    {
    pQueryDriver        ThisDriver;
    }
    MQJINF;


/*** Data on one join operation ***/
typedef struct
    {
    unsigned int	Mask;			/* objects in this join */
    unsigned int	OuterMask;		/* if an outer join, these are the outer objects */
    unsigned int	InnerMask;		/* inner joined objects */
    unsigned int	GlobalInnerMask;	/* if an outer join, these are all query objects forced to be inner */
    unsigned int	DependencyMask;
    unsigned int	GlobalDependencyMask;
#if 00
    int			Specificity;
    int			Score;
    int			Used;
    int			PrimarySource;
#endif
    }
    MqjJoin, *pMqjJoin;


/*** Data on one source ***/
typedef struct _MQJS
    {
    pQueryStructure	FromItem;
    int			Score;
    unsigned int	DependencyMask;
    }
    MqjSource, *pMqjSource;


/*** "private data" structure for mqj-specific operations, including the
 *** n-row-prefetch algorithm.
 ***/
typedef struct
    {
    pObject 		PrefetchObjs[MQJ_MAX_PREFETCH];
    int			PrefetchMatchCnt[MQJ_MAX_PREFETCH];
    int 		nPrefetch;
    int			PrefetchUsesSlave;	/* boolean */
    int			PrefetchReachedEnd;	/* boolean */
    int			CurItem;
    pExpression		SlaveExpr;
    }
    MqjJoinData, *pMqjJoinData;


/*** mqj_internal_DetermineJoinOrder - take the list of sources and joins, and
 *** via recursive iteration, determine a viable ordering which satisfies the
 *** outer/inner relationships, specificities, and the complexities of 2-way
 *** and N-way joins.
 ***/
int
mqj_internal_DetermineJoinOrder(int n_joins, pMqjJoin joins[], int n_sources, pMqjSource sources[], unsigned int used_sourcemask, unsigned int constant_sources, pMqjSource ordered_sources[])
    {
    int i,j,rval;
    int source_objid;
    unsigned int to_try;
    int highest, highest_score;
    int dont_use, tied, covered;
    int our_score;

	/** Obtain a mask of source ID's to try **/
	for(i=to_try=0; i<n_sources; i++)
	    if (!(used_sourcemask & (1<<i)) && sources[i])
		to_try |= (1<<sources[i]->FromItem->ObjID);

	/** We're done? **/
	if (!to_try)
	    return 0;

	/** Loop through our sources, trying to find one that should go first **/
	while(to_try)
	    {
	    highest = -1;
	    highest_score = -1;
	    for(i=0; i<n_sources; i++)
		{
		if (!sources[i]) continue;
		source_objid = sources[i]->FromItem->ObjID;

		/** The source must not have been tried yet **/
		if (to_try & (1<<source_objid))
		    {
		    /** Our rules for source selection, in order of precedence:
		     **
		     ** 1.  It must not be an inner member of an outer join whose
		     **     outer members have not yet been used.
		     ** 2.  If there are joins that tie this source to used
		     **     sources, at least one must covered entirely by this
		     **     source and already-used sources.
		     ** 3.  Sources tied by joins to used sources must be tried
		     **     before sources that have no joins to used sources.
		     ** 4.  Higher-scoring sources should be tried before lower-
		     **     scoring ones.
		     **/

		    /** First, we start with the existing score, for criteria 4 **/
		    our_score = sources[i]->Score;

		    /** Scan the list of joins to take care of criteria 1, 2, & 3 **/
		    dont_use = 0;
		    tied = 0;
		    covered = 0;
		    for(j=0; j<n_joins; j++)
			{
			/** Criteria #1 **/
			if (((1<<source_objid) & joins[j]->GlobalInnerMask) && ((joins[j]->OuterMask) & used_sourcemask) != joins[j]->OuterMask)
			    {
			    dont_use = 1;
			    break;
			    }

			/** Criteria #2 and #3 **/
			if (((1<<source_objid) & joins[j]->Mask) && (joins[j]->Mask & (used_sourcemask & ~constant_sources)))
			    {
			    tied = 1;
			    if ((((1<<source_objid) | (used_sourcemask & ~constant_sources)) & (joins[j]->Mask & ~constant_sources)) == (joins[j]->Mask & ~constant_sources))
				covered = 1;
			    }
			}

		    /** Criteira #1 and #2 **/
		    if (dont_use || (tied && !covered))
			continue;

		    /** Criteria #3 **/
		    if (tied)
			our_score += 0x20000000;

		    /** Compare the scores: criteria #3 and #4 **/
		    if (highest == -1 || our_score > highest_score)
			{
			highest = i;
			highest_score = our_score;
			}
		    }
		}

	    /** None found? **/
	    if (highest == -1)
		return -1;

	    /** Try this one **/
	    rval = mqj_internal_DetermineJoinOrder(n_joins, joins, n_sources, sources, used_sourcemask | (1<<sources[highest]->FromItem->ObjID), constant_sources, ordered_sources+1);
	    if (rval >= 0)
		{
		/** A sequence beginning with this highest source worked. **/
		ordered_sources[0] = sources[highest];
		return rval + 1;
		}

	    /** Try another **/
	    to_try &= ~(1<<sources[highest]->FromItem->ObjID);
	    }

    return -1;
    }


int
mqj_internal_PrintMask(int mask, pMqjSource sources[], int n_sources)
    {
    int first;
    int i;

	first = 1;
	for(i=0; i<n_sources; i++)
	    {
	    if (mask & 1<<sources[i]->FromItem->ObjID)
		{
		fprintf(stderr, "%s%s", first?"":" ", sources[i]->FromItem->Presentation);
		first = 0;
		}
	    }

    return 0;
    }


/*** mqjAnalyze - take a given query syntax structure (qs) and scans it for
 *** join operations, by first scanning the Where clause for the various
 *** join statements, and then piecing them together to form a set of joins
 *** done in the 'proper' order :)
 ***/
int
mqjAnalyze(pQueryStatement stmt)
    {
    pQueryElement qe;
    pQueryElement master=NULL,slave=NULL;
    pQueryStructure from_qs = NULL;
    pQueryStructure select_qs = NULL;
    pQueryStructure where_qs = NULL;
    pQueryStructure select_item;
    pQueryStructure where_item;
    pQueryStructure from_item=NULL;
    int i,n=0,j,found,first;
    pExpression new_exp;
    pMqjJoin joins[MQJ_MAX_JOIN];
    pMqjJoin found_join;
    pMqjSource sources[MQJ_MAX_SOURCE];
    pMqjSource ordered_sources[MQJ_MAX_SOURCE];
    int joined_objects;
    int n_joins = 0;
    int n_sources = 0;
    unsigned int used_sourcemask, provided_mask;
    int min_objlist = stmt->Query->nProvidedObjects;
    unsigned int our_mask, our_outer_mask;
    int mask_objcnt;

	n_sources = stmt->Query->ObjList->nObjects;
	for(i=0; i<n_sources; i++)
	    sources[i] = NULL;

    	/** Search for WHERE clauses with join operations... **/
	while((from_qs = mq_internal_FindItem(stmt->QTree, MQ_T_FROMCLAUSE, from_qs)) != NULL)
	    {
	    /** Get the WHERE clause corresponding to this FROM, if any **/
	    where_qs = mq_internal_FindItem(from_qs->Parent, MQ_T_WHERECLAUSE, NULL);
	    if (where_qs)
		{
		/** Build a list of join expressions available in the where clause. **/
		for(i=0;i<where_qs->Children.nItems;i++)
		    {
		    where_item = (pQueryStructure)(where_qs->Children.Items[i]);
		    if (where_item->ObjCnt == 2)
			{
			/** Already seen this join combination? **/
			found_join = NULL;
			for(j=0;j<n_joins;j++)
			    {
			    if (joins[j]->Mask == where_item->Expr->ObjCoverageMask) 
				{
				found_join = joins[j];
				if (found_join->OuterMask != where_item->Expr->ObjOuterMask)
				    {
				    mssError(1,"MQJ","An entity cannot be both an inner and outer member of an outer join");
				    goto error;
				    }
				break;
				}
			    }

			/** Join has not been seen yet -- add it. **/
			if (!found_join)
			    {
			    /** Too many? **/
			    if (n_joins >= MQJ_MAX_JOIN)
				{
				mssError(1, "MQJ", "Too many join operations (max = %d)", MQJ_MAX_JOIN);
				goto error;
				}

			    /** Allocate the join **/
			    joins[n_joins] = nmMalloc(sizeof(MqjJoin));
			    if (!joins[n_joins])
				goto error;

			    /** Add to our list **/
			    joins[n_joins]->Mask = where_item->Expr->ObjCoverageMask;
			    joins[n_joins]->OuterMask = where_item->Expr->ObjOuterMask;
			    joins[n_joins]->InnerMask = joins[n_joins]->Mask & ~(joins[n_joins]->OuterMask);
			    joins[n_joins]->GlobalInnerMask = joins[n_joins]->InnerMask;
			    n_joins++;
			    }
			}
		    }
		}

	    /** Also look for joins declared by FROM clause item expressions **/
	    for(i=0; i<from_qs->Children.nItems; i++)
		{
		from_item = (pQueryStructure)(from_qs->Children.Items[i]);
		if (from_item->Flags & MQ_SF_EXPRESSION)
		    {
		    our_mask = from_item->Expr->ObjCoverageMask | 1<<(from_item->ObjID);
		    our_outer_mask = from_item->Expr->ObjCoverageMask;
		    mask_objcnt = 0;
		    n = our_mask & EXPR_MASK_ALLOBJECTS;
		    n >>= min_objlist;
		    while (n)
			{
			mask_objcnt += (n&1);
			n >>= 1;
			}
		   
		    if (mask_objcnt > 1)
			{
			/** Already seen this join combination? **/
			found_join = NULL;
			for(j=0;j<n_joins;j++)
			    {
			    if (joins[j]->Mask == our_mask) 
				{
				found_join = joins[j];
				if (found_join->OuterMask != our_outer_mask)
				    {
				    mssError(1,"MQJ","An entity cannot be both an inner and outer member of an outer join");
				    goto error;
				    }
				break;
				}
			    }

			/** Join has not been seen yet -- add it. **/
			if (!found_join)
			    {
			    /** Too many? **/
			    if (n_joins >= MQJ_MAX_JOIN)
				{
				mssError(1, "MQJ", "Too many join operations (max = %d)", MQJ_MAX_JOIN);
				goto error;
				}

			    /** Allocate the join **/
			    joins[n_joins] = nmMalloc(sizeof(MqjJoin));
			    if (!joins[n_joins])
				goto error;

			    /** Add to our list **/
			    joins[n_joins]->Mask = our_mask;
			    joins[n_joins]->OuterMask = our_outer_mask;
			    joins[n_joins]->InnerMask = joins[n_joins]->Mask & ~(joins[n_joins]->OuterMask);
			    joins[n_joins]->GlobalInnerMask = joins[n_joins]->InnerMask;
			    n_joins++;
			    }
			}
		    }
		}

	    /** Build a list of the FROM sources **/
	    for(i=min_objlist; i<n_sources; i++)
	        {
		/** Search for the object in the FROM clause **/
		found = -1;
		for(j=0;j<from_qs->Children.nItems;j++)
		    {
		    from_item = (pQueryStructure)(from_qs->Children.Items[j]);
		    if (!strcmp(stmt->Query->ObjList->Names[i], 
		        from_item->Presentation[0]?(from_item->Presentation):(from_item->Source)))
			{
			found = j;
			break;
			}
		    }
		if (found < 0) 
		    {
		    mssError(1,"MQE","Join: referenced object not found in FROM clause");
		    goto error;
		    }

		/** Setup our source data structure **/
		sources[i] = nmMalloc(sizeof(MqjSource));
		if (!sources[i])
		    goto error;
		sources[i]->DependencyMask = 0;
		sources[i]->FromItem = from_item;
		sources[i]->Score = from_item->Specificity + (1000 - from_item->QELinkage->OrderPrio) * 0x10000;
		}

	    /** Only one source? **/
	    if (n_sources <= min_objlist + 1)
		goto cleanup;

	    /** Determine inner/outer relationships query-wide **/
	    for(i=n_joins-1; i>=0; i--)
		{
		for(j=0; j<n_joins; j++)
		    {
		    if (i != j)
			{
			if (joins[i]->OuterMask && (joins[j]->Mask & joins[i]->GlobalInnerMask))
			    joins[i]->GlobalInnerMask |= joins[j]->GlobalInnerMask;
			}
		    }
		}

	    /** Sequence our sources based on joins and specificity.  This is
	     ** where the hard work of query optimization is really done.
	     **/
	    provided_mask = EXPR_MASK_EXTREF | EXPR_MASK_INDETERMINATE;
	    for(i=0; i<min_objlist; i++)
		provided_mask |= (1<<i);
	    joined_objects = mqj_internal_DetermineJoinOrder(n_joins, joins, n_sources, sources, provided_mask, provided_mask, ordered_sources);
	    if (joined_objects < 0)
		{
		mssError(1, "MQJ", "Invalid join structure in query");
		goto error;
		}

	    /** Determine direct dependencies for a given join **/
	    for(i=0; i<n_joins; i++)
		{
		joins[i]->DependencyMask = 0;
		first = 1;
		for(j=joined_objects-1; j>=0; j--)
		    {
		    if (joins[i]->Mask & (1<<ordered_sources[j]->FromItem->ObjID))
			{
			if (!first)
			    joins[i]->DependencyMask |= (1<<ordered_sources[j]->FromItem->ObjID);
			first = 0;
			}
		    }
		joins[i]->GlobalDependencyMask = joins[i]->DependencyMask;
		}

	    /** Determine all dependencies for a given join **/
	    for(i=0; i<n_joins; i++)
		{
		for(j=0; j<n_joins; j++)
		    {
		    if (i != j && (joins[j]->Mask & joins[i]->GlobalDependencyMask) && !(joins[j]->DependencyMask & joins[i]->GlobalDependencyMask))
			joins[i]->GlobalDependencyMask |= joins[j]->DependencyMask;
		    }
		}

	    /** Set dependencies for sources **/
	    for(i=0; i<joined_objects; i++)
		{
		for(j=0; j<n_joins; j++)
		    {
		    if ((joins[j]->Mask & (1<<ordered_sources[i]->FromItem->ObjID)) && !(joins[j]->GlobalDependencyMask & (1<<ordered_sources[i]->FromItem->ObjID)))
			ordered_sources[i]->DependencyMask |= joins[j]->GlobalDependencyMask;
		    }
		}

#if 00
	    for(i=0;i<joined_objects;i++)
		{
		fprintf(stderr, "Src%d: %s %s score 0x%1.1x, dep %2.2x (",
			i,
			ordered_sources[i]->FromItem->Presentation,
			ordered_sources[i]->FromItem->Source,
			ordered_sources[i]->Score,
			ordered_sources[i]->DependencyMask
			);
		mqj_internal_PrintMask(ordered_sources[i]->DependencyMask, ordered_sources, joined_objects);
		fprintf(stderr, ")\n");
		}
	    for(i=0;i<n_joins;i++)
		{
		fprintf(stderr, "Join%d: mask %2.2x (", i, joins[i]->Mask);
		mqj_internal_PrintMask(joins[i]->Mask, ordered_sources, joined_objects);
		fprintf(stderr, "), outer %2.2x (", joins[i]->OuterMask);
		mqj_internal_PrintMask(joins[i]->OuterMask, ordered_sources, joined_objects);
		fprintf(stderr, "), inner %2.2x (", joins[i]->InnerMask);
		mqj_internal_PrintMask(joins[i]->InnerMask, ordered_sources, joined_objects);
		fprintf(stderr, "), globalinner %2.2x (", joins[i]->GlobalInnerMask);
		mqj_internal_PrintMask(joins[i]->GlobalInnerMask, ordered_sources, joined_objects);
		fprintf(stderr, "), dep %2.2x (", joins[i]->DependencyMask);
		mqj_internal_PrintMask(joins[i]->DependencyMask, ordered_sources, joined_objects);
		fprintf(stderr, "), globaldep %2.2x (", joins[i]->GlobalDependencyMask);
		mqj_internal_PrintMask(joins[i]->GlobalDependencyMask, ordered_sources, joined_objects);
		fprintf(stderr, ")\n");
		}
#endif

	    /** Get the select clause **/
	    select_qs = mq_internal_FindItem(from_qs->Parent, MQ_T_SELECTCLAUSE, NULL);

	    /** Ok, got list of join expressions.  Now create a JOIN from each one. **/
	    used_sourcemask = 0;
	    for(i=0; i<joined_objects-1; i++)
		{
		/** Create the new QueryElement and link into the exec tree... **/
		qe = mq_internal_AllocQE();
		if (!qe)
		    goto error;
		qe->Driver = MQJINF.ThisDriver;

		/** Find the master QE from the MQ->Trees array if 1st join, otherwise use MQ->Tree. **/
		if (i == 0)
		    {
		    for(j=0; j<xaCount(&stmt->Trees); j++)
		        {
			master = (pQueryElement)xaGetItem(&stmt->Trees, j);
			if (((pQueryStructure)(master->QSLinkage)) == ordered_sources[i]->FromItem) break;
			}
		    used_sourcemask |= 1<<ordered_sources[i]->FromItem->ObjID;
		    }
		else
		    {
		    master = stmt->Tree;
		    }
		qe->SrcIndex = ordered_sources[i]->FromItem->ObjID;

		/** Find the slave QE, from the MQ->Trees array, whether 1st or nth join. **/
		for(j=0; j<xaCount(&stmt->Trees); j++)
		    {
		    slave = (pQueryElement)xaGetItem(&stmt->Trees, j);
		    if (((pQueryStructure)(slave->QSLinkage)) == ordered_sources[i+1]->FromItem) break;
		    }
		qe->SrcIndexSlave = ordered_sources[i+1]->FromItem->ObjID;

		/** Handle joins **/
		for(j=0; j<n_joins; j++)
		    {
		    /** Is it an outer join? Yes if:
		     **   1) The join is an outer join,
		     **   2) All outer members are now covered,
		     **   3) This current object is an inner member.
		     **/
		    if ((joins[j]->OuterMask & ~provided_mask) != 0 && ((joins[j]->OuterMask & ~provided_mask) & used_sourcemask) == (joins[j]->OuterMask & ~provided_mask) && (joins[j]->InnerMask & (1<<(qe->SrcIndexSlave))))
			{
			qe->Flags |= MQ_EF_OUTERJOIN;
			}
		    qe->DependencyMask = ordered_sources[i+1]->DependencyMask;
		    //qe->DependencyMask |= joins[j]->GlobalDependencyMask;
		    }

		/** We're now "using" this object ID **/
		used_sourcemask |= 1<<qe->SrcIndexSlave;

		/** Handle SELECT clause items **/
		if (select_qs)
		    {
		    /** Inherit the qe-linkages from the select items for any sub-objects. **/
		    for(j=0; j<select_qs->Children.nItems; j++)
			{
			select_item = (pQueryStructure)(select_qs->Children.Items[j]);
			if (select_item->QELinkage == slave || select_item->QELinkage == master)
			    select_item->QELinkage = qe;
			}

		    /** Setup the attribute list in the queryelement structure **/
		    for(j=0; j<select_qs->Children.nItems; j++)
			{
			select_item = (pQueryStructure)(select_qs->Children.Items[j]);
			if ((select_item->Expr && (select_item->Expr->ObjCoverageMask & ~(stmt->Query->ProvidedObjMask | used_sourcemask | EXPR_MASK_EXTREF)) == 0) || ((select_item->Flags & MQ_SF_ASTERISK) && i == joined_objects-2))
			    {
			    if (select_item->Flags & MQ_SF_ASTERISK)
				stmt->Flags |= MQ_TF_ASTERISK;
			    xaAddItem(&qe->AttrNames, (void*)select_item->Presentation);
			    xaAddItem(&qe->AttrExprPtr, (void*)select_item->RawData.String);
			    xaAddItem(&qe->AttrCompiledExpr, (void*)select_item->Expr);
			    xaAddItem(&qe->AttrDeriv, (void*)(select_item->QELinkage));
			    select_item->QELinkage = qe;
			    }
			}
		    }

		/** Grab up the WHERE expressions for this join... **/
		qe->Constraint = NULL;
		if (where_qs)
		    {
		    for(j=0; j<where_qs->Children.nItems; j++)
			{
			where_item = (pQueryStructure)(where_qs->Children.Items[j]);
			if (where_item->Expr && (where_item->Expr->ObjCoverageMask & used_sourcemask) != 0 && (where_item->Expr->ObjCoverageMask & (stmt->Query->ProvidedObjMask | used_sourcemask /*| provided_mask*/)) == where_item->Expr->ObjCoverageMask)
			    {
			    if (qe->Constraint)
				{
				new_exp = expAllocExpression();
				if (!new_exp)
				    goto error;
				new_exp->NodeType = EXPR_N_AND;
				expAddNode(new_exp, qe->Constraint);
				expAddNode(new_exp, where_item->Expr);
				qe->Constraint = new_exp;
				}
			    else
				{
				qe->Constraint = where_item->Expr;
				}
			    xaRemoveItem(&where_qs->Children, j);
			    where_item->Expr = NULL;
			    mq_internal_FreeQS(where_item);
			    j--;
			    continue;
			    }
			}
		    }

		/** Add the master and slave below this qe **/
		if (!master || !slave)
		    {
		    mssError(1,"MQJ","Bark!  Could not locate master/slave query component(s)!");
		    mq_internal_FreeQE(qe);
		    goto error;
		    }
		xaAddItem(&qe->Children,(void*)master);
		master->Parent = qe;
		xaAddItem(&qe->Children,(void*)slave);
		slave->Parent = qe;
		stmt->Tree = qe;
		qe->QSLinkage = NULL;
		qe->PrivateData = (void*)nmMalloc(sizeof(MqjJoinData));
		memset(qe->PrivateData, 0, sizeof(MqjJoinData));
		qe->OrderPrio = (slave->OrderPrio < master->OrderPrio)?slave->OrderPrio:master->OrderPrio;
		}
	    }

    cleanup:
	/** Clean up **/
	for(i=0;i<n_joins;i++)
	    nmFree(joins[i], sizeof(MqjJoin));
	for(i=min_objlist; i<n_sources; i++)
	    if (sources[i])
		nmFree(sources[i], sizeof(MqjSource));

	return 0;

    error:
	/** Error condition **/
	for(i=0;i<n_joins;i++)
	    nmFree(joins[i], sizeof(MqjJoin));
	for(i=min_objlist; i<n_sources; i++)
	    if (sources[i])
		nmFree(sources[i], sizeof(MqjSource));

	return -1;
    }


int
mqj_internal_ClosePrefetch(pQueryElement qe)
    {
    int i;
    pMqjJoinData md = (pMqjJoinData)(qe->PrivateData);

	for(i=0;i<md->nPrefetch;i++)
	    {
	    if (md->PrefetchObjs[i])
		{
		objClose(md->PrefetchObjs[i]);
		md->PrefetchObjs[i] = NULL;
		}
	    }
	md->nPrefetch = 0;

    return 0;
    }


/*** mqj_internal_DoPrefetch() - grab up to N rows from the given child
 *** source, and put them in the prefetch array in the PrivateData.
 ***/
int
mqj_internal_DoPrefetch(pQueryElement qe, pQueryStatement stmt)
    {
    pMqjJoinData md = (pMqjJoinData)(qe->PrivateData);
    pQueryElement prefetch_cld;
    int rval;

	/** are we prefetching on master or slave side of the join? **/
	if (md->PrefetchUsesSlave)
	    prefetch_cld = (pQueryElement)(qe->Children.Items[1]);
	else
	    prefetch_cld = (pQueryElement)(qe->Children.Items[0]);

	/** close any existing open prefetch objects **/
	mqj_internal_ClosePrefetch(qe);

	md->PrefetchReachedEnd = 0;

	/** Get at most MQJ_MAX_PREFETCH objects **/
	while(md->nPrefetch < MQJ_MAX_PREFETCH)
	    {
	    rval = prefetch_cld->Driver->NextItem(prefetch_cld, stmt);
	    if (rval < 0)
		{
		/** error from prefetch_cld **/
		return rval;
		}
	    if (rval == 0)
		{
		/** no more rows from prefetch_cld **/
		md->PrefetchReachedEnd = 1;
		break;
		}
	    md->PrefetchObjs[md->nPrefetch] = stmt->Query->ObjList->Objects[prefetch_cld->SrcIndex];
	    if (md->PrefetchObjs[md->nPrefetch])
		objLinkTo(md->PrefetchObjs[md->nPrefetch]);
	    md->nPrefetch++;
	    }

    return 0;
    }


/*** mqjStart - starts the join operation.  This function mostly does 
 *** some initialization, but does not fetch any rows.
 ***/
int
mqjStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    pQueryElement master;
    pMqjJoinData md;

    	/** Initialize the iteration counts. **/
	qe->IterCnt = 0;
	qe->SlaveIterCnt = 0;
	qe->Flags &= ~MQ_EF_SLAVESTART;
	md = (pMqjJoinData)(qe->PrivateData);
	md->nPrefetch = 0;
	md->CurItem = 0;

	/** Start the query on the master side. **/
	master = (pQueryElement)(qe->Children.Items[0]);
	if (master->Driver->Start(master, stmt, NULL) < 0) return -1;

    return 0;
    }


/*** mqj_internal_NextItemPrefetch - retrieves the first/next item in the
 *** joined result set, using the prefetch-n-rows method detailed at the
 *** top of this source code.  Only enabled if MQJ_ENABLE_PREFETCH is set
 *** to 1 (instead of 0).
 ***/
int
mqj_internal_NextItemPrefetch(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement master,slave;
    pMqjJoinData md;

	/** Determine master and slave sides of query **/
	master = (pQueryElement)(qe->Children.Items[0]);
	slave = (pQueryElement)(qe->Children.Items[1]);
	md = (pMqjJoinData)(qe->PrivateData);

    return 1;
    }


/*** mqjNextItem - retrieves the first/next item in the joined result set.
 *** This function must iterate through each item of the master's result
 *** set, and for each one, call a start, nextitem{}, and finish for the
 *** slave's result set.  Return 1 if valid row, 0 if no more rows, -1 if
 *** error.
 ***/
int
mqjNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement master,slave;
    int rval;
    int i;
    int nullouter;

    	/** Do query in multiple-OR-fetch mode? **/
	if (MQJ_ENABLE_PREFETCH) return mqj_internal_NextItemPrefetch(qe,stmt);

    	/** Get master, slave ptrs **/
	master = (pQueryElement)(qe->Children.Items[0]);
	slave = (pQueryElement)(qe->Children.Items[1]);

	/** Loop until we get a row. **/
	while(1)
	    {
	    /** IF slave cnt is 0, get a new master row and start the slave... **/
	    if (qe->SlaveIterCnt == 0)
	        {
	        rval = master->Driver->NextItem(master, stmt);
	        if (rval == 0 || rval < 0) return rval;
	        qe->IterCnt++;
		/*expFreezeEval(qe->Constraint, stmt->Query->ObjList, slave->SrcIndex);*/
		/*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL)
		    {*/
		    /** If slave only depends on NULL outer elements, then don't run it **/
		    nullouter = 0;
		    for(i=stmt->Query->nProvidedObjects;i<stmt->Query->ObjList->nObjects;i++)
			if (stmt->Query->ObjList->Objects[i] == NULL && ((1<<i) & master->CoverageMask))
			    nullouter |= (1<<i);
		    //if (slave->DependencyMask && (slave->DependencyMask & ~nullouter) == 0)
		    if (qe->DependencyMask & nullouter)
			{
			rval = 1;
			break;
			}

		    /** Start the slave **/
		    if (slave->Driver->Start(slave, stmt, qe->Constraint) < 0)
			return -1;
		    qe->Flags |= MQ_EF_SLAVESTART;
		    /*}*/
	        }

	    /** Ok, retrieve a slave row. **/
	    /*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL) */
	        rval = slave->Driver->NextItem(slave, stmt);
	    /*else
	        rval = 0;*/
	    if ((rval == 0 || rval < 0) && (qe->SlaveIterCnt > 0 || !(qe->Flags & MQ_EF_OUTERJOIN)))
	        {
		/*if (stmt->Query->ObjList->Objects[qe->SrcIndex] != NULL) 
		    {*/
		    slave->Driver->Finish(slave, stmt);
		    qe->Flags &= ~MQ_EF_SLAVESTART;
		    /*}*/
		qe->SlaveIterCnt = 0;
		continue;
		}
	    else if (rval == 0 && (qe->Flags & MQ_EF_OUTERJOIN))
		{
		/** outer join with NULL row from slave **/
		/*expModifyParam(stmt->Query->ObjList, stmt->Query->ObjList->Names[slave->SrcIndex], NULL);*/
		rval = 1;
		}
	    qe->SlaveIterCnt++;
	    break;
	    }

    return rval;
    }


/*** mqjFinish - ends the join operation and cleans up after itself...
 ***/
int
mqjFinish(pQueryElement qe, pQueryStatement stmt)
    {
    pQueryElement master;
    pQueryElement slave;
    pMqjJoinData md;

    	/** Need to complete the slave-side query? (early query cancel) **/
	md = (pMqjJoinData)(qe->PrivateData);
	if (qe->Flags & MQ_EF_SLAVESTART)
	    {
	    qe->Flags &= ~MQ_EF_SLAVESTART;
	    slave = (pQueryElement)(qe->Children.Items[1]);
	    slave->Driver->Finish(slave, stmt);
	    }

    	/** Complete the query on the master side **/
	master = (pQueryElement)(qe->Children.Items[0]);
	master->Driver->Finish(master, stmt);

    return 0;
    }


/*** mqjRelease - deallocate any private data structures that were used
 *** in the Analyze phase.  Just returns without doing anything for this
 *** function.
 ***/
int
mqjRelease(pQueryElement qe, pQueryStatement stmt)
    {

    	if (qe->PrivateData) nmFree(qe->PrivateData, sizeof(MqjJoinData));

    return 0;
    }


/*** mqjInitialize - initialize this module and register with the multi-
 *** query system.
 ***/
int
mqjInitialize()
    {
    pQueryDriver drv;

    	/** Allocate the driver descriptor structure **/
	drv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!drv) return -1;
	memset(drv,0,sizeof(QueryDriver));

	nmRegister(sizeof(MqjJoin), "MqjJoin");
	nmRegister(sizeof(MqjSource), "MqjSource");
	nmRegister(sizeof(MqjJoinData), "MqjJoinData");

	/** Fill in the structure elements **/
	strcpy(drv->Name, "MQJ - MultiQuery Join Module");
	drv->Precedence = 3000;
	drv->Flags = 0;
	drv->Analyze = mqjAnalyze;
	drv->Start = mqjStart;
	drv->NextItem = mqjNextItem;
	drv->Finish = mqjFinish;
	drv->Release = mqjRelease;

	/** Register with the multiquery system. **/
	if (mqRegisterQueryDriver(drv) < 0) return -1;
	MQJINF.ThisDriver = drv;

    return 0;
    }
