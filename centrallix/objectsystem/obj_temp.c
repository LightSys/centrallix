#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xhashqueue.h"
#include "cxlib/xhandle.h"
#include "expression.h"
#include "cxlib/magic.h"
#include "cxlib/mtsession.h"
#include "stparse.h"
#include "cxss/cxss.h"
#include "cxlib/strtcpy.h"
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 20, 2018					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.	*/
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_temp.c: temporary object collections		*/
/************************************************************************/

#define	TMP_THRESHOLD	(32)	/* minimum collection size to index */
#define	TMP_MIN_ORDER	(513)	/* min buckets in index hash table */
#define TMP_MAX_ORDER	(3001)	/* max buckets in index hash table */
#define	TMP_TYPE_MIXED	(-1)	/* index field holds values of several types */

typedef struct
    {
    pObjTemp	TempObj;
    pStructInf	Data;
    int		CurAttr;
    IntVec	IVvalue;
    StringVec	SVvalue;
    void*	VecData;
    int		IsNew:1;
    }
    ObjTempData, *pObjTempData;

typedef struct _TIE
    {
    pStructInf	Tuple;
    struct _TIE* Prev;
    struct _TIE* Next;
    struct _TIN* Node;
    }
    ObjTempIdxEntry, *pObjTempIdxEntry;

typedef struct _TIN
    {
    char*	Key;
    pObjTempIdxEntry First;	/* Entries with this key, in the order added */
    pObjTempIdxEntry Last;
    }
    ObjTempIdxNode, *pObjTempIdxNode;

typedef struct
    {
    pStructInf	Data;
    int		ItemCnt;
    pStructInf	CurInf;
    pObjTemp	TempObj;
    pObjTempIndex UsingIndex;
    pXArray	Matches;	/* Rows found by the index, linked */
    pObjQuery	ObjQuery;
    }
    ObjTempQuery, *pObjTempQuery;


/*** tmp_internal_IndexHasField() - determine if an index has a particular
 *** field in it.  Returns the field ID if so, 0 if not, -1 on failure.
 ***/
int
tmp_internal_IndexHasField(pObjTempIndex idx, char* field)
    {
    int i;
    char* one_field;

	/** Loop through the fields in the index **/
	for(i=0; i<idx->Fields.nItems; i++)
	    {
	    one_field = (char*)idx->Fields.Items[i];
	    if (!strcmp(one_field, field))
		return i+1;
	    }

    return 0;
    }


/*** tmp_internal_GenerateKey() - determine lookup key for the index.
 *** Returns the key, which the caller frees with nmSysFree(), or NULL on
 *** failure.
 ***/
char*
tmp_internal_GenerateKey(pObjTempIndex idx, pExpression fields[])
    {
    XString key;
    char* key_str = NULL;

	if (xsInit(&key) < 0)
	    {
	    mssError(1, "TMP", "Failed to initialize the index key.");
	    return NULL;
	    }

	/** Generate the key value **/
	if (objBuildBinaryImageXString(&key, fields, idx->Fields.nItems, idx->OneObjList, 1) < 0)
	    {
	    mssError(0, "TMP", "Failed to build the index key from %d field values.", idx->Fields.nItems);
	    goto end;
	    }

	key_str = nmSysStrdup(key.String);
	if (key_str == NULL)
	    {
	    mssError(1, "TMP", "Failed to copy the %d-byte index key.", key.Length);
	    goto end;
	    }

    end:
	xsDeInit(&key);
	return key_str;
    }


/*** tmp_internal_GenerateKeyFromInf() - determine the lookup key for the index,
 *** using data in a StructInf structure.
 ***/
char*
tmp_internal_GenerateKeyFromInf(pObjTempIndex idx, pStructInf tuple)
    {
    pExpression fields[idx->Fields.nItems];
    pStructInf one_field;
    char* key;
    int i;

	/** Loop through the fields in the index **/
	for(i=0; i<idx->Fields.nItems; i++)
	    {
	    one_field = stLookup(tuple, (char*)idx->Fields.Items[i]);
	    fields[i] = stGetExpression(one_field, 0);
	    }

	key = tmp_internal_GenerateKey(idx, fields);

    return key;
    }


/*** tmp_internal_IndexLookup() - find an object via the index
 ***/
pObjTempIdxNode
tmp_internal_IndexLookup(pObjTempIndex idx, pExpression fields[])
    {
    char* key;
    pObjTempIdxNode node;

	/** Determine the lookup key **/
	key = tmp_internal_GenerateKey(idx, fields);
	if (!key)
	    return NULL;

	/** Look it up **/
	node = (pObjTempIdxNode)xhLookup(&idx->Index, key);
	nmSysFree(key);

    return node;
    }


/*** tmp_internal_IndexLookupFromInf() - find an object via the index
 ***/
pObjTempIdxNode
tmp_internal_IndexLookupFromInf(pObjTempIndex idx, pStructInf values)
    {
    char* key;
    pObjTempIdxNode node;

	/** Determine the lookup key **/
	key = tmp_internal_GenerateKeyFromInf(idx, values);
	if (!key)
	    return NULL;

	/** Look it up **/
	node = (pObjTempIdxNode)xhLookup(&idx->Index, key);
	nmSysFree(key);

    return node;
    }


/*** tmp_internal_FreeIndexNode() - release a node in the index
 ***/
int
tmp_internal_FreeIndexNode(pObjTempIdxNode node)
    {

	nmSysFree(node->Key);
	nmFree(node, sizeof(ObjTempIdxNode));

    return 0;
    }


/*** tmp_internal_FreeIndexEntry() - release an entry in the index
 ***/
int
tmp_internal_FreeIndexEntry(pObjTempIdxEntry entry)
    {

	nmFree(entry, sizeof(ObjTempIdxEntry));

    return 0;
    }


/*** tmp_internal_RemoveFromIndex() - remove an object from an index.
 *** Returns -1 if the object is not in the index.
 ***/
int
tmp_internal_RemoveFromIndex(pObjTempIndex idx, pStructInf tuple)
    {
    pObjTempIdxEntry entry;
    pObjTempIdxNode node;

	/** Look it up **/
	entry = (pObjTempIdxEntry)xhLookup(&idx->Entries, (char*)&tuple);
	if (!entry)
	    return -1;
	xhRemove(&idx->Entries, (char*)&tuple);

	/** Unlink it from its key's entries **/
	node = entry->Node;
	if (entry->Prev)
	    entry->Prev->Next = entry->Next;
	else
	    node->First = entry->Next;
	if (entry->Next)
	    entry->Next->Prev = entry->Prev;
	else
	    node->Last = entry->Prev;
	tmp_internal_FreeIndexEntry(entry);

	/** Free the node with its last entry **/
	if (!node->First)
	    {
	    xhRemove(&idx->Index, node->Key);
	    tmp_internal_FreeIndexNode(node);
	    }

    return 0;
    }


/*** tmp_internal_RecordTypes() - record the data type of each non-NULL
 *** index field in a tuple, marking a field mixed once its types differ.
 ***/
int
tmp_internal_RecordTypes(pObjTempIndex idx, pStructInf tuple)
    {
    pExpression exp;
    int i;

	for(i=0; i<idx->Fields.nItems; i++)
	    {
	    exp = stGetExpression(stLookup(tuple, (char*)idx->Fields.Items[i]), 0);
	    if (!exp)
		continue;
	    if (expEvalTree(exp, idx->OneObjList) < 0)
		{
		mssClearError();
		idx->Types[i] = TMP_TYPE_MIXED;
		}
	    else if (exp->Flags & EXPR_F_NULL)
		continue;
	    else if (idx->Types[i] == DATA_T_UNAVAILABLE)
		idx->Types[i] = exp->DataType;
	    else if (idx->Types[i] != exp->DataType)
		idx->Types[i] = TMP_TYPE_MIXED;
	    }

    return 0;
    }


/*** tmp_internal_AddToIndex() - add one object in the collection to the index,
 *** or update its key if it is already there.  On failure, the index no
 *** longer has every object, so queries stop using it, and this returns -1.
 *** -1 is not an error, and no error is left on the error stack.
 ***/
int
tmp_internal_AddToIndex(pObjTempIndex idx, pStructInf tuple)
    {
    pObjTempIdxNode node = NULL;
    pObjTempIdxEntry entry;
    char* key;

	/** Not complete? **/
	if (!idx->IsComplete)
	    return -1;

	/** Indexed before its values were set? **/
	tmp_internal_RemoveFromIndex(idx, tuple);

	/** Determine the lookup key **/
	key = tmp_internal_GenerateKeyFromInf(idx, tuple);
	if (!key)
	    goto error;

	/** Find the key's node, or add one **/
	node = (pObjTempIdxNode)xhLookup(&idx->Index, key);
	if (node)
	    {
	    nmSysFree(key);
	    }
	else
	    {
	    node = (pObjTempIdxNode)nmMalloc(sizeof(ObjTempIdxNode));
	    if (!node)
		{
		nmSysFree(key);
		goto error;
		}
	    node->Key = key;
	    node->First = NULL;
	    node->Last = NULL;
	    if (xhAdd(&idx->Index, node->Key, (void*)node) < 0)
		{
		tmp_internal_FreeIndexNode(node);
		node = NULL;
		goto error;
		}
	    }

	/** Add the entry after the key's other entries **/
	entry = (pObjTempIdxEntry)nmMalloc(sizeof(ObjTempIdxEntry));
	if (!entry)
	    goto error;
	entry->Tuple = tuple;
	entry->Node = node;
	if (xhAdd(&idx->Entries, (char*)&entry->Tuple, (void*)entry) < 0)
	    {
	    tmp_internal_FreeIndexEntry(entry);
	    goto error;
	    }
	entry->Next = NULL;
	entry->Prev = node->Last;
	if (node->Last)
	    node->Last->Next = entry;
	else
	    node->First = entry;
	node->Last = entry;
	tmp_internal_RecordTypes(idx, tuple);

	return 0;

    error:
	if (node && !node->First)
	    {
	    xhRemove(&idx->Index, node->Key);
	    tmp_internal_FreeIndexNode(node);
	    }
	mssClearError();
	idx->IsComplete = 0;
	return -1;
    }


/*** tmp_internal_AddToMatchingIndexes() - look at all indexes to see which
 *** ones contain a certain field, and add the tuple to those indexes
 ***/
int
tmp_internal_AddToMatchingIndexes(pObjTemp tmp, pStructInf tuple, char* field)
    {
    int i;
    pObjTempIndex idx;

	/** Scan the list of indexes **/
	for(i=0; i<tmp->Indexes.nItems; i++)
	    {
	    idx = (pObjTempIndex)tmp->Indexes.Items[i];
	    if (!strcmp(field, "*") || tmp_internal_IndexHasField(idx, field) >= 1)
		{
		tmp_internal_AddToIndex(idx, tuple);
		}
	    }

    return 0;
    }


/*** tmp_internal_RemoveFromMatchingIndexes() - look at all indexes to see which
 *** ones contain a certain field, and remove the tuple from those indexes
 ***/
int
tmp_internal_RemoveFromMatchingIndexes(pObjTemp tmp, pStructInf tuple, char* field)
    {
    int i;
    pObjTempIndex idx;

	/** Scan the list of indexes **/
	for(i=0; i<tmp->Indexes.nItems; i++)
	    {
	    idx = (pObjTempIndex)tmp->Indexes.Items[i];
	    if (!strcmp(field, "*") || tmp_internal_IndexHasField(idx, field) >= 1)
		{
		tmp_internal_RemoveFromIndex(idx, tuple);
		}
	    }

    return 0;
    }


/*** tmp_internal_AddAllToIndex() - adds all objects in the temp collection
 *** to the index.  This is used for initializing the index when it is first
 *** created.  Returns -1 if the index is incomplete, which is not an error.
 ***/
int
tmp_internal_AddAllToIndex(pObjTemp tmp, pObjTempIndex idx)
    {
    int i;
    pStructInf tuple;

	/** Loop through the tuples **/
	for(i=0; i<((pStructInf)tmp->Data)->nSubInf; i++)
	    {
	    tuple = ((pStructInf)tmp->Data)->SubInf[i];

	    /** Add it **/
	    if (tmp_internal_AddToIndex(idx, tuple) < 0)
		return -1;
	    }

    return 0;
    }


/*** tmp_internal_FreeIndex() - release an index to a temp collection
 ***/
int
tmp_internal_FreeIndex(pObjTempIndex idx)
    {

	expFreeParamList(idx->OneObjList);
	xaClear(&idx->Fields, (void*)nmSysFree, NULL);
	xhClear(&idx->Entries, (void*)tmp_internal_FreeIndexEntry, NULL);
	xhClear(&idx->Index, (void*)tmp_internal_FreeIndexNode, NULL);
	xaDeInit(&idx->Fields);
	xhDeInit(&idx->Entries);
	xhDeInit(&idx->Index);
	nmSysFree(idx->Types);
	nmFree(idx, sizeof(ObjTempIndex));

    return 0;
    }


/*** tmp_internal_FindIndex() - given a list of fields, find a
 *** compatible index.  For now, we ONLY look to see if there is
 *** an exact match; later we can keep index statistics to allow
 *** inexact matches.
 ***/
pObjTempIndex
tmp_internal_FindIndex(pObjTemp tmp, pXArray fields)
    {
    int i, j, k;
    pObjTempIndex idx;
    int matched, found;

	/** Search the index list **/
	for(i=0; i<tmp->Indexes.nItems; i++)
	    {
	    idx = (pObjTempIndex)tmp->Indexes.Items[i];

	    /** Compare the fields **/
	    if (fields->nItems == idx->Fields.nItems)
		{
		matched = 1;
		for(j=0; j<fields->nItems; j++)
		    {
		    found = 0;
		    for(k=0; k<fields->nItems; k++)
			{
			if (!strcmp(fields->Items[j], idx->Fields.Items[k]))
			    {
			    found = 1;
			    break;
			    }
			}
		    if (!found)
			matched = 0;
		    }
		if (matched)
		    return idx;
		}
	    }

    return NULL;
    }


/*** tmp_internal_CreateIndex() - create an index on a collection.  Returns
 *** NULL if it can't, which is not an error: the caller scans instead.
 ***/
pObjTempIndex
tmp_internal_CreateIndex(pObjTemp tmp, pXArray fields)
    {
    pObjTempIndex idx;
    char* name;
    int order, i;

	idx = (pObjTempIndex)nmMalloc(sizeof(ObjTempIndex));
	if (!idx)
	    return NULL;
	memset(idx, 0, sizeof(ObjTempIndex));
	idx->Types = (int*)nmSysMalloc(sizeof(int) * fields->nItems);
	if (idx->Types == NULL)
	    goto error;
	for(i=0; i<fields->nItems; i++)
	    idx->Types[i] = DATA_T_UNAVAILABLE;
	if (xaInit(&idx->Fields, 16) < 0)
	    goto error;
	idx->IsComplete = 1;
	order = ((pStructInf)tmp->Data)->nSubInf;

	/** Tuneables **/
	if (order > TMP_MAX_ORDER) order = TMP_MAX_ORDER;
	if (order < TMP_MIN_ORDER) order = TMP_MIN_ORDER;

	/** Make the hash table that backs the index (we will switch
	 ** to a B+ tree for this in the future).  This index uses
	 ** variable length keys (asciz).
	 **/
	if (xhInit(&idx->Index, order, 0) < 0)
	    {
	    memset(&idx->Index, 0, sizeof(XHashTable));
	    goto error;
	    }
	if (xhInit(&idx->Entries, order, sizeof(pStructInf)) < 0)
	    {
	    memset(&idx->Entries, 0, sizeof(XHashTable));
	    goto error;
	    }

	/** Copy the field list **/
	for(i=0; i<fields->nItems; i++)
	    {
	    name = nmSysStrdup(fields->Items[i]);
	    if (name == NULL)
		goto error;
	    if (xaAddItem(&idx->Fields, name) < 0)
		{
		nmSysFree(name);
		goto error;
		}
	    }

	/** Create a param objects list **/
	idx->OneObjList = expCreateParamList();
	if (idx->OneObjList == NULL)
	    goto error;
	if (expAddParamToList(idx->OneObjList, NULL, NULL, EXPR_O_CURRENT) < 0)
	    goto error;

	/** Add the index to the temp collection **/
	if (xaAddItem(&tmp->Indexes, idx) < 0)
	    goto error;

	return idx;

    error:
	if (idx->OneObjList != NULL)
	    expFreeParamList(idx->OneObjList);
	if (idx->Index.nRows != 0)
	    xhDeInit(&idx->Index);
	if (idx->Entries.nRows != 0)
	    xhDeInit(&idx->Entries);
	if (idx->Fields.Items != NULL)
	    {
	    xaClear(&idx->Fields, (void*)nmSysFree, NULL);
	    xaDeInit(&idx->Fields);
	    }
	if (idx->Types != NULL)
	    nmSysFree(idx->Types);
	mssClearError();
	nmFree(idx, sizeof(ObjTempIndex));
	return NULL;
    }

/*** tmp_internal_AnalyzeCriteria_r() - see below.  This does the actual
 *** expression search work.  Right now, we only allow AND expressions with
 *** equality comparisons (FIXME - enhancement).
 ***/
int
tmp_internal_AnalyzeCriteria_r(pObjTemp tmp, pObjTempIndex idx, pExpression root, pExpression query, pExpression* values)
    {
    int i, field;
    pExpression i0, i1, it;
    int i0_id = -10, i1_id = -10;

	/** AND? recurse if so. **/
	if (query->NodeType == EXPR_N_AND)
	    {
	    for(i=0; i<query->Children.nItems; i++)
		{
		if (tmp_internal_AnalyzeCriteria_r(tmp, idx, root, (pExpression)query->Children.Items[i], values) < 0)
		    return -1;
		}
	    return 0;
	    }

	/** Equality comparison? Analyze it. **/
	if (query->NodeType == EXPR_N_COMPARE && query->CompareType == MLX_CMP_EQUALS)
	    {
	    i0 = (pExpression)query->Children.Items[0];
	    if (i0->NodeType == EXPR_N_OBJECT) i0 = (pExpression)i0->Children.Items[0];
	    if (i0->NodeType == EXPR_N_PROPERTY)
		{
		i0_id = expObjID(i0,NULL);
		if (root->Control && root->Control->Remapped) 
		    i0_id = root->Control->ObjMap[i0_id];
		}
	    i1 = (pExpression)query->Children.Items[1];
	    if (i1->NodeType == EXPR_N_OBJECT) i1 = (pExpression)i1->Children.Items[0];
	    if (i1->NodeType == EXPR_N_PROPERTY)
		{
		i1_id = expObjID(i1,NULL);
		if (root->Control && root->Control->Remapped) 
		    i1_id = root->Control->ObjMap[i1_id];
		}

	    /** Must be an obj/prop that we handle, and a constant-like value **/
	    if (((expIsConstant(i0) || (i0->Flags & EXPR_F_FREEZEEVAL)) && (i1_id == 0 || i1_id == -1 || i1_id == -2)) ||
		((expIsConstant(i1) || (i1->Flags & EXPR_F_FREEZEEVAL)) && (i0_id == 0 || i0_id == -1 || i0_id == -2)))
		{
		if (expIsConstant(i0) || (i0->Flags & EXPR_F_FREEZEEVAL))
		    {
		    /** swap **/
		    it = i0; i0 = i1; i1 = it;
		    }

		/** Now i0 = property we handle, and i1 = constant **/
		field = tmp_internal_IndexHasField(idx, i0->Name);
		if (field <= 0)
		    return -1;
		values[field-1] = i1;
		return 0;
		}
	    }

    return -1; /* some other node type or construct that we don't handle */
    }


/*** tmp_internal_IsKeyUsable() - determine whether looking up a value in
 *** an index field finds exactly the rows whose field compares equal to it.
 *** Returns 1 if so, 0 if not.  0 is not an error: the caller scans instead,
 *** and no error is left on the error stack.
 ***/
int
tmp_internal_IsKeyUsable(pObjTempIndex idx, int field, pExpression value)
    {
    int type = idx->Types[field];

	if (expEvalTree(value, idx->OneObjList) < 0)
	    {
	    mssClearError();
	    return 0;
	    }

	/** NULL never compares equal **/
	if (value->Flags & EXPR_F_NULL)
	    return 0;

	/** Equal values of different types have different keys **/
	if (type == TMP_TYPE_MIXED || (type != DATA_T_UNAVAILABLE && type != value->DataType))
	    return 0;

	/** Equal doubles can have different keys (-0.0 and 0.0, NaN and anything) **/
	if (type == DATA_T_DOUBLE)
	    return 0;

    return 1;
    }


/*** tmp_internal_AnalyzeCriteria() - given an expression tree with search
 *** criteria in it, we determine whether we can handle the criteria, and if
 *** so, with what field values.  This adds to what expGetPropsForObject()
 *** already does (it lets us find the index).  Returns an array of expressions,
 *** which can then be passed to tmp_internal_IndexLookup(), or NULL if the
 *** index can't be used.  NULL is not an error: the caller scans instead, and
 *** no error is left on the error stack.
 ***/
pExpression*
tmp_internal_AnalyzeCriteria(pObjTemp tmp, pObjTempIndex idx, pExpression query)
    {
    pExpression* values = NULL;
    int i;

	/** Allocate our expression array **/
	values = (pExpression*)nmMalloc(sizeof(pExpression) * idx->Fields.nItems);
	if (!values)
	    return NULL;
	memset(values, 0, sizeof(pExpression) * idx->Fields.nItems);

	/** Recurse through the query expression, looking for criteria. **/
	if (tmp_internal_AnalyzeCriteria_r(tmp, idx, query, query, values) < 0)
	    {
	    nmFree(values, sizeof(pExpression) * idx->Fields.nItems);
	    return NULL;
	    }

	/** Did we get them all, with values the keys can find? **/
	for(i=0; i<idx->Fields.nItems; i++)
	    {
	    if (values[i] == NULL || !tmp_internal_IsKeyUsable(idx, i, values[i]))
		{
		nmFree(values, sizeof(pExpression) * idx->Fields.nItems);
		return NULL;
		}
	    }

    return values;
    }


/*** obj_internal_CloseTempObj() - release the connection to the
 *** temporary object
 ***/
int
obj_internal_CloseTempObj(pObjTemp tmp)
    {
    int i;
    pObjTempIndex idx;

	tmp->LinkCnt--;
	if (tmp->LinkCnt <= 0)
	    {
	    stFreeInf((pStructInf)tmp->Data);
	    xhnFreeHandle(&OSYS.TempObjects, tmp->Handle);
	    for(i=0; i<tmp->Indexes.nItems; i++)
		{
		/** free an index **/
		idx = (pObjTempIndex)tmp->Indexes.Items[i];
		tmp_internal_FreeIndex(idx);
		}
	    xaDeInit(&tmp->Indexes);
	    nmFree(tmp, sizeof(ObjTemp));
	    }

    return 0;
    }


/*** objCreateTempObject() - creates a temporary object and returns a handle
 *** to the newly created object, which can then be opened using the
 *** objOpenTempObject() function, and later scheduled for deletion using
 *** objDeleteTempObject().  These "temporary" objects exist outside of any
 *** OSML session, and so their lifetime and scope is not restricted to
 *** any one session but can be shared across sessions and across time.
 ***/
handle_t
objCreateTempObject()
    {
    pObjTemp tmp;

	/** Create the temporary object **/
	tmp = (pObjTemp)nmMalloc(sizeof(ObjTemp));
	if (!tmp)
	    return XHN_INVALID_HANDLE;
	tmp->LinkCnt = 1;
	tmp->CreateCnt = 0;
	xaInit(&tmp->Indexes, 16);
	tmp->Data = stCreateStruct("temp", "system/object");
	if (!tmp->Data)
	    {
	    nmFree(tmp, sizeof(ObjTemp));
	    return XHN_INVALID_HANDLE;
	    }
	tmp->Handle = xhnAllocHandle(&OSYS.TempObjects, (void*)tmp);

    return tmp->Handle;
    }


/*** objDeleteTempObject() - deletes a temporary object.  The object will
 *** not actually be permanently deleted until all other opens and child
 *** opens and queries on the object have been closed.
 ***/
int
objDeleteTempObject(handle_t tempobj)
    {
    pObjTemp tmp = NULL;

	/** Lookup the temporary object **/
	tmp = xhnHandlePtr(&OSYS.TempObjects, tempobj);
	if (!tmp)
	    goto error;

	/** Schedule it for deletion (or delete it, if no one else has
	 ** it open).
	 **/
	if (obj_internal_CloseTempObj(tmp) < 0)
	    goto error;

	return 0;

    error:
	return -1;
    }


/*** objOpenTempObject() - opens a temp object.  The open object pointer
 *** is only valid for the session in which it is opened.
 ***/
pObject
objOpenTempObject(pObjSession session, handle_t tempobj, int mode)
    {
    pObject obj = NULL;
    pObjTemp tmp = NULL;
    pObjTempData inf = NULL;

	/** Lookup the temporary object **/
	tmp = xhnHandlePtr(&OSYS.TempObjects, tempobj);
	if (!tmp)
	    goto error;
	tmp->LinkCnt++;

	/** Allocate the linkage to the node in the temp obj **/
	inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	if (!inf)
	    goto error;
	memset(inf, 0, sizeof(ObjTempData));
	inf->TempObj = tmp;
	inf->Data = stLinkInf((pStructInf)tmp->Data);
	inf->IsNew = 0;

	/** Allocate an open object **/
	obj = obj_internal_AllocObj();
	if (!obj)
	    goto error;
	obj->Data = (void*)inf;
	obj->Driver = OSYS.TempDriver;
	obj->Session = session;
	obj->Pathname = obj_internal_NormalizePath("","");

	/** Add to open objects this session. **/
	xaAddItem(&session->OpenObjects, (void*)obj);

	return obj;

    error:
	if (inf)
	    nmFree(inf, sizeof(ObjTempData));
	if (obj)
	    obj_internal_FreeObj(obj);
	if (tmp)
	    tmp->LinkCnt--;
	return NULL;
    }


/*** tmpOpen - temporary objects cannot be opened via objOpen because they are not
 *** anchored at any point in the actual ObjectSystem.
 ***/
void*
tmpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpOpenChild - open a child object of this one
 ***/
void*
tmpOpenChild(void* inf_v, pObject obj, char* childname, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;
    pObjTempData child_inf;
    pStructInf find_inf;
    int is_new = 0;

	/** Find it **/
	if (!strcmp(childname, "*") && (obj->Mode & OBJ_O_AUTONAME) && (obj->Mode & OBJ_O_CREAT))
	    find_inf = NULL;
	else
	    find_inf = stLookup(inf->Data, childname);

	/** Exists, but it is not a child object? **/
	if (find_inf && stStructType(find_inf) != ST_T_SUBGROUP)
	    return NULL;

	/** Exclusive create, but object already exists? **/
	if ((obj->Mode & OBJ_O_EXCL) && (obj->Mode & OBJ_O_EXCL) && find_inf)
	    return NULL;

	/** Create a new child object? **/
	if ((obj->Mode & OBJ_O_CREAT) && !find_inf)
	    {
	    find_inf = stAllocInf();
	    if (!find_inf)
		return NULL;
	    inf->TempObj->CreateCnt++;
	    if (!strcmp(childname, "*") && (obj->Mode & OBJ_O_AUTONAME))
		snprintf(find_inf->Name, ST_NAME_STRLEN, "%lld", inf->TempObj->CreateCnt);
	    else
		strtcpy(find_inf->Name, childname, ST_NAME_STRLEN);
	    find_inf->UsrType = nmMalloc(ST_USRTYPE_STRLEN);
	    if (!find_inf->UsrType)
		return NULL;
	    strtcpy(find_inf->UsrType, "", ST_USRTYPE_STRLEN);
	    find_inf->Flags |= ST_F_GROUP;
	    stAddInf(inf->Data, find_inf);
	    is_new = 1;
	    }

	/** Exists and is a child object **/
	if (find_inf)
	    {
	    child_inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	    if (!child_inf) return NULL;
	    memset(child_inf, 0, sizeof(ObjTempData));
	    child_inf->Data = stLinkInf(find_inf);
	    child_inf->TempObj = inf->TempObj;
	    child_inf->TempObj->LinkCnt++;
	    child_inf->IsNew = is_new;
	    return child_inf;
	    }

    return NULL;
    }


/*** tmpClose - close an open temp object.
 ***/
int
tmpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;

	/** Release the memory **/
	obj_internal_CloseTempObj(inf->TempObj);
	stFreeInf(inf->Data);
	nmFree(inf, sizeof(ObjTempData));

    return 0;
    }


/*** tmpCreate - create a new object, via pathname.  Since pathnames are not used
 *** to reference temp objects, this just returns failure.
 ***/
int
tmpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** tmpDeleteObj - delete an existing object.  This removes the object from the
 *** struct inf tree.
 ***/
int
tmpDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)inf_v;

	/** Trying to delete root? **/
	if (inf->Data == (pStructInf)inf->TempObj->Data)
	    {
	    mssError(1, "OSML", "Cannot delete the root of temporary object tree");
	    return -1;
	    }

	/** Delete it. **/
	tmp_internal_RemoveFromMatchingIndexes(inf->TempObj, inf->Data, "*");
	stRemoveInf(inf->Data);
	 
	/** Release the inf structure **/
	tmpClose(inf_v, oxt);

    return 0;
    }


/*** tmpRead - Structure elements have no content.  Fails.
 ***/
int
tmpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //pObjTempData inf = (pObjTempData)(inf_v);
    return -1;
    }


/*** tmpWrite - Again, no content.  This fails.
 ***/
int
tmpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //pObjTempData inf = (pObjTempData)(inf_v);
    return -1;
    }


/*** tmp_i_FreeString - frees a string unless it is NULL
 ***/
int
tmp_i_FreeString(char* ptr)
    {
    if (ptr) nmSysFree(ptr);
    return 0;
    }


/*** tmpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
tmpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pObjTempQuery qy;
    pXArray props = NULL;
    pXArray propnames = NULL;
    int i;
    pExpression* values;
    pObjTempIdxNode node;
    pObjTempIdxEntry entry;

	/** By default, no query criteria support **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;

	/** Allocate the query structure **/
	qy = (pObjTempQuery)nmMalloc(sizeof(ObjTempQuery));
	if (!qy)
	    goto error;
	memset(qy, 0, sizeof(ObjTempQuery));
	qy->Data = stLinkInf(inf->Data);
	qy->ItemCnt = 0;
	qy->TempObj = inf->TempObj;
	qy->UsingIndex = NULL;
	qy->ObjQuery = query;
	inf->TempObj->LinkCnt++;
	qy->Matches = xaNew(16);
	if (!qy->Matches)
	    {
	    mssError(1, "TMP", "Failed to allocate the query's index matches.");
	    goto error;
	    }

	/** Determine properties used by the query criteria **/
	if (query->Tree && inf->Data->nSubInf >= TMP_THRESHOLD)
	    {
	    /** Get both the explicit ID 0 and any "current" properties (-2) **/
	    props = expGetPropsForObject(query->Tree, 0, NULL);
	    expGetPropsForObject(query->Tree, -2, props);
	    if (props)
		{
		/** Do we have an index yet? **/
		propnames = xaNew(16);
		if (!propnames)
		    goto error;
		for(i=0; i<props->nItems; i++)
		    {
		    if (!(((pExpProperty)props->Items[i])->Flags & EXPR_F_FREEZEEVAL))
			xaAddItem(propnames, nmSysStrdup(((pExpProperty)props->Items[i])->PropName));
		    }

		/** Find a usable index **/
		qy->UsingIndex = tmp_internal_FindIndex(qy->TempObj, propnames);

		/** No matching index found?  Create one instead.  If the create
		 ** fails, the query will fall back to a collection scan.
		 **/
		if (!qy->UsingIndex)
		    {
		    qy->UsingIndex = tmp_internal_CreateIndex(qy->TempObj, propnames);

		    /** Add our items to the index **/
		    if (qy->UsingIndex)
			{
			if (tmp_internal_AddAllToIndex(qy->TempObj, qy->UsingIndex) < 0)
			    qy->UsingIndex = NULL;
			}
		    }

		/** Require a complete index **/
		if (qy->UsingIndex && !qy->UsingIndex->IsComplete)
		    qy->UsingIndex = NULL;

		/** If we have an index, see if our criteria allow us to use it **/
		if (qy->UsingIndex)
		    {
		    //printf("Using Index:");
		    //for(i=0; i<qy->UsingIndex->Fields.nItems; i++)
			//{
			//printf(" %s", qy->UsingIndex->Fields.Items[i]);
			//}
		    //printf(" - collection size is %d", ((pStructInf)qy->TempObj->Data)->nSubInf);
		    //printf(" - index size is %d", qy->UsingIndex->Index.nItems);
		    values = tmp_internal_AnalyzeCriteria(qy->TempObj, qy->UsingIndex, query->Tree);
		    if (values)
			{
			//printf(" - got values");
			/** Ok, it is usable.  Do the lookup. **/
			node = tmp_internal_IndexLookup(qy->UsingIndex, values);
			nmFree(values, sizeof(pExpression) * qy->UsingIndex->Fields.nItems);
			values = NULL;
			if (node)
			    {
			    for(entry=node->First; entry; entry=entry->Next)
				{
				if (xaAddItem(qy->Matches, entry->Tuple) < 0)
				    {
				    mssError(1, "TMP", "Failed to add index match %d to the query.", qy->Matches->nItems + 1);
				    goto error;
				    }
				stLinkInf(entry->Tuple);
				}
			    }
			}
		    else
			{
			qy->UsingIndex = NULL;
			}
		    //printf("\n");
		    }

		expFreeProps(props);
		props = NULL;
		xaClear(propnames, (void*)nmSysFree, NULL);
		xaFree(propnames);
		}
	    }

	if (qy->UsingIndex)
	    {
	    query->Flags |= OBJ_QY_F_FULLQUERY;
	    cxTestOutput(3, "TMP: Using index.\n");
	    }
	else
	    {
	    cxTestOutput(4, "TMP: Scanning collection.\n");
	    }

	return (void*)qy;

    error:
	if (qy)
	    {
	    if (qy->TempObj)
		qy->TempObj->LinkCnt--;
	    if (qy->Matches)
		{
		xaClear(qy->Matches, (void*)stFreeInf, NULL);
		xaFree(qy->Matches);
		}
	    if (qy->Data)
		stFreeInf(qy->Data);
	    nmFree(qy, sizeof(ObjTempQuery));
	    }
	if (props)
	    {
	    expFreeProps(props);
	    }
	if (propnames)
	    {
	    xaClear(propnames, (void*)nmSysFree, NULL);
	    xaFree(propnames);
	    }
	return NULL;
    }


/*** tmpQueryFetch - get the next directory entry as an open object.
 ***/
void*
tmpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pObjTempQuery qy = ((pObjTempQuery)(qy_v));
    pObjTempData inf;

	/** Using an index? **/
	if (qy->UsingIndex)
	    {
	    /** Yes, using an index.  Skip rows deleted since the lookup. **/
	    while(qy->ItemCnt < qy->Matches->nItems && !((pStructInf)qy->Matches->Items[qy->ItemCnt])->Parent)
		qy->ItemCnt++;
	    if (qy->ItemCnt >= qy->Matches->nItems)
		return NULL;
	    qy->CurInf = (pStructInf)qy->Matches->Items[qy->ItemCnt];
	    }
	else
	    {
	    /** Find a subgroup item **/
	    while(qy->ItemCnt < qy->Data->nSubInf && 
		  stStructType(qy->Data->SubInf[qy->ItemCnt]) != ST_T_SUBGROUP) qy->ItemCnt++;

	    /** No more left? **/
	    if (qy->ItemCnt >= qy->Data->nSubInf)
		return NULL;

	    qy->CurInf = qy->Data->SubInf[qy->ItemCnt];
	    }

	/** Alloc the structure **/
	inf = (pObjTempData)nmMalloc(sizeof(ObjTempData));
	if (!inf) return NULL;
	memset(inf, 0, sizeof(ObjTempData));
	inf->Data = stLinkInf(qy->CurInf);
	inf->TempObj = qy->TempObj;
	inf->TempObj->LinkCnt++;
	inf->IsNew = 0;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** tmpQueryClose - close the query.
 ***/
int
tmpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pObjTempQuery qy = ((pObjTempQuery)(qy_v));

	/** Free the structure **/
	xaClear(qy->Matches, (void*)stFreeInf, NULL);
	xaFree(qy->Matches);
	obj_internal_CloseTempObj(qy->TempObj);
	stFreeInf(qy->Data);
	nmFree(qy, sizeof(ObjTempQuery));

    return 0;
    }


/*** tmpGetAttrType - get the type (DATA_T_tmp) of an attribute by name.
 ***/
int
tmpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Lookup the subgroup inf **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf || stStructType(find_inf) != ST_T_ATTRIB) 
	    {
	    return (find_inf)?(-1):DATA_T_UNAVAILABLE;
	    }

	/** Examine the expr to determine the type **/
	t = stGetAttrType(find_inf, 0);
	if (find_inf->Value && find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (t == DATA_T_INTEGER) return DATA_T_INTVEC;
	    else return DATA_T_STRINGVEC;
	    }
	else if (find_inf->Value)
	    {
	    return t;
	    }

    return -1;
    }


/*** tmpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
tmpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int rval;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (stLookup(inf->Data, "content"))
	        val->String = "application/octet-stream";
	    else
	        val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->UsrType;
	    return 0;
	    }

	/** Look through the attribs in the subinf **/
	find_inf = stLookup(inf->Data, attrname);

	/** If annotation, and not found, return "" **/
	if (!find_inf && !strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

	/** Not found, or not an attribute? **/
	if (!find_inf) return 1;
	if (stStructType(find_inf) != ST_T_ATTRIB) return -1;

	/** Vector or scalar? **/
	if (find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (inf->VecData)
		{
		nmSysFree(inf->VecData);
		inf->VecData = NULL;
		}
	    if (stGetAttrType(find_inf, 0) == DATA_T_INTEGER)
		{
		if (datatype != DATA_T_INTVEC)
		    {
		    mssError(1,"OSML","Type mismatch getting attribute '%s' (should be intvec)", attrname);
		    return -1;
		    }
		inf->VecData = stGetValueList(find_inf, DATA_T_INTEGER, &(inf->IVvalue.nIntegers));
		val->IntVec = &(inf->IVvalue);
		val->IntVec->Integers = (int*)(inf->VecData);
		}
	    else
		{
		if (datatype != DATA_T_STRINGVEC)
		    {
		    mssError(1,"OSML","Type mismatch getting attribute '%s' (should be stringvec)", attrname);
		    return -1;
		    }
		/** FIXME - the below StringVec->Strings never gets freed **/
		inf->VecData = stGetValueList(find_inf, DATA_T_STRING, &(inf->SVvalue.nStrings));
		val->StringVec = &(inf->SVvalue);
		val->StringVec->Strings = (char**)(inf->VecData);
		}
	    return 0;
	    }
	else
	    {
	    rval = stGetAttrValue(find_inf, datatype, val, 0);
	    if (rval < 0)
		{
		mssError(1,"OSML","Type mismatch or non-existent attribute '%s'", attrname);
		}
	    return rval;
	    }

    return -1;
    }


/*** tmpGetNextAttr - get the next attribute name for this object.
 ***/
char*
tmpGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);

	/** Get the next attr from the list unless last one already **/
	while(inf->CurAttr < inf->Data->nSubInf && 
	      (stStructType(inf->Data->SubInf[inf->CurAttr]) != ST_T_ATTRIB || 
	       !strcmp(inf->Data->SubInf[inf->CurAttr]->Name, "annotation"))) inf->CurAttr++;
	if (inf->CurAttr >= inf->Data->nSubInf) return NULL;

    return inf->Data->SubInf[inf->CurAttr++]->Name;
    }


/*** tmpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
tmpGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = tmpGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** tmpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
tmpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf find_inf;
    int t;
    int removed_indexes = 0;
    int added_attr = 0;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (strlen(val->String) >= ST_NAME_STRLEN)
		{
		mssError(1,"OSML","Name too long (%lu chars) for temp object, max %d", (unsigned long)strlen(val->String), ST_NAME_STRLEN);
		return -1;
		}
	    strtcpy(inf->Data->Name, val->String, ST_NAME_STRLEN);
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (strlen(val->String) >= ST_USRTYPE_STRLEN)
		{
		mssError(1,"OSML","Content type too long (%lu chars) for temp object, max %d", (unsigned long)strlen(val->String), ST_USRTYPE_STRLEN);
		return -1;
		}
	    strtcpy(inf->Data->UsrType, val->String, ST_USRTYPE_STRLEN);
	    return 0;
	    }

	/** Index update - part 1 **/
	if (!inf->IsNew)
	    {
	    tmp_internal_RemoveFromMatchingIndexes(inf->TempObj, inf->Data, attrname);
	    removed_indexes = 1;
	    }

	/** Otherwise, set the integer or string value **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf) 
	    {
	    find_inf = stAddAttr(inf->Data, attrname);
	    if (!find_inf)
		{
		mssError(1,"OSML","Could not add attribute '%s' to object", attrname);
		goto error;
		}
	    added_attr = 1;
	    }
	if (stStructType(find_inf) != ST_T_ATTRIB)
	    goto error;

	/** Set value of attribute **/
	t = stGetAttrType(find_inf, 0);
	if (!val && t > 0)
	    datatype = t;
	if (t > 0 && datatype != t && !stAttrIsNull(find_inf))
	    {
	    mssError(1,"OSML","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
		    attrname, obj_type_names[datatype], obj_type_names[t]);
	    goto error;
	    }
	if (stSetAttrValue(find_inf, datatype, val, 0) < 0)
	    {
	    mssError(1,"OSML","Could not set attribute '%s'", attrname);
	    goto error;
	    }

	/** Index update - part 2 **/
	if (!inf->IsNew)
	    tmp_internal_AddToMatchingIndexes(inf->TempObj, inf->Data, attrname);

	return 0;

    error:
	if (added_attr)
	    stRemoveInf(find_inf);
	if (removed_indexes)
	    tmp_internal_AddToMatchingIndexes(inf->TempObj, inf->Data, attrname);
	return -1;
    }


/*** tmpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
tmpAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    pStructInf new_inf;
    char* ptr;

    	/** Add the attribute **/
	new_inf = stAddAttr(inf->Data, attrname);
	if (type == DATA_T_STRING)
	    {
	    ptr = (char*)nmSysStrdup(val->String);
	    stAddValue(new_inf, ptr, 0);
	    }
	else if (type == DATA_T_INTEGER)
	    {
	    stAddValue(new_inf, NULL, val->Integer);
	    }
	else
	    {
	    stAddValue(new_inf, NULL, 0);
	    }

    return 0;
    }


/*** tmpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
tmpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpGetFirstMethod -- return name of First method available on the object.
 ***/
char*
tmpGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
tmpGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpExecuteMethod - Execute a method, by name.
 ***/
int
tmpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** tmpPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
tmpPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** tmpInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
tmpInfo(void* inf_v, pObjectInfo info)
    {
    pObjTempData inf = (pObjTempData)(inf_v);
    int i;

	/** Setup the flags, and we know the subobject count btw **/
	memset(info, 0, sizeof(ObjectInfo));
	info->Flags = (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_SUBOBJ_CNT_KNOWN |
		OBJ_INFO_F_CAN_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
		OBJ_INFO_F_NO_CONTENT | OBJ_INFO_F_SUPPORTS_INHERITANCE | OBJ_INFO_F_TEMPORARY);
	info->nSubobjects = 0;
	for(i=0;i<inf->Data->nSubInf;i++)
	    {
	    if (stStructType(inf->Data->SubInf[i]) == ST_T_SUBGROUP) info->nSubobjects++;
	    }
	if (info->nSubobjects)
	    info->Flags |= OBJ_INFO_F_HAS_SUBOBJ;
	else
	    info->Flags |= OBJ_INFO_F_NO_SUBOBJ;

    return 0;
    }


/*** tmpCommit - commit any changes made to the underlying data source.
 ***/
int
tmpCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pObjTempData inf = (pObjTempData)(inf_v);

	/** update indexes **/
	if (inf->IsNew)
	    {
	    tmp_internal_AddToMatchingIndexes(inf->TempObj, inf->Data, "*");
	    inf->IsNew = 0;
	    }

    return 0;
    }


/*** temp object handler initialization ***/
int
tmpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Setup the structure **/
	strcpy(drv->Name,"TMP - Temporary objects driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/tempobj");

	nmRegister(sizeof(ObjTempData), "ObjTempData");
	nmRegister(sizeof(ObjTempIdxNode), "ObjTempIdxNode");
	nmRegister(sizeof(ObjTempQuery), "ObjTempQuery");
	nmRegister(sizeof(ObjTempIndex), "ObjTempIndex");
	nmRegister(sizeof(ObjTemp), "ObjTemp");

	/** Setup the function references. **/
	drv->Open = tmpOpen;
	drv->OpenChild = tmpOpenChild;
	drv->Close = tmpClose;
	drv->Create = tmpCreate;
	drv->Delete = NULL;
	drv->DeleteObj = tmpDeleteObj;
	drv->OpenQuery = tmpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = tmpQueryFetch;
	drv->QueryClose = tmpQueryClose;
	drv->Read = tmpRead;
	drv->Write = tmpWrite;
	drv->GetAttrType = tmpGetAttrType;
	drv->GetAttrValue = tmpGetAttrValue;
	drv->GetFirstAttr = tmpGetFirstAttr;
	drv->GetNextAttr = tmpGetNextAttr;
	drv->SetAttrValue = tmpSetAttrValue;
	drv->AddAttr = tmpAddAttr;
	drv->OpenAttr = tmpOpenAttr;
	drv->GetFirstMethod = tmpGetFirstMethod;
	drv->GetNextMethod = tmpGetNextMethod;
	drv->ExecuteMethod = tmpExecuteMethod;
	drv->Info = tmpInfo;
	drv->Commit = tmpCommit;

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	OSYS.TempDriver = drv;

    return 0;
    }
