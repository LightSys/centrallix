#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "expression.h"
#include "cxlib/mtsession.h"
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
/* Module: 	expression.h, exp_params.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*		--> exp_params.c: handles the object parameter list	*/
/*		mechanism.						*/
/************************************************************************/



/*** expCreateParamList - sets up and initializes the object parameter list
 *** that can be filled out and used later in the expression compilation and
 *** evaluation stages.
 ***/
pParamObjects
expCreateParamList()
    {
    pParamObjects objlist;

    	/** Allocate the structure **/
	objlist = (pParamObjects)nmMalloc(sizeof(ParamObjects));
	if (!objlist) return NULL;
	memset(objlist, 0, sizeof(ParamObjects));
	objlist->CurrentID = -1;
	objlist->ParentID = -1;
	objlist->MainFlags = 0;
	objlist->PSeqID = (EXP.PSeqID++);
	objlist->Session = NULL;
	objlist->CurControl = NULL;

	/** Initialize the per-objlist random seed **/
	objlist->RandomInit = 0;

    return objlist;
    }


/*** expSetEvalDomain - sets the evaluation domain for an object list -
 *** whether static or server (client doesn't make sense here).  The
 *** domain should be one of EXPR_MO_RUNxxxxxx.  If the domain of the
 *** objlist is lesser than the domain of the expression, then late
 *** binding of properties is allowed (nonexistent objects return NULL
 *** instead of an error).
 ***/
int
expSetEvalDomain(pParamObjects this, int domain)
    {

	this->MainFlags &= ~EXPR_MO_DOMAINMASK;
	this->MainFlags |= (domain & EXPR_MO_DOMAINMASK);

    return 0;
    }


/*** expFreeParamList - frees memory used by, and deinitializes, an object
 *** parameter list structure.
 ***/
int 
expFreeParamList(pParamObjects this)
    {
    int i;

    	/** Free the names, if allocated **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Flags[i] & EXPR_O_ALLOCNAME)
	    {
	    nmSysFree(this->Names[i]);
	    this->Names[i] = NULL;
	    }

	if (this->CurControl)
	    exp_internal_UnlinkControl(this->CurControl);

	/** Free the structure. **/
	nmFree(this, sizeof(ParamObjects));

    return 0;
    }


/*** expFreeParamListWithCB - free the parameter list, but also call a callback
 *** function on each param that is in the list.
 ***/
int
expFreeParamListWithCB(pParamObjects this, int (*free_fn)())
    {
    int i;

	/** Go through em **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Names[i])
	    {
	    free_fn(this->Objects[i]);
	    }

	/** Free it up **/
	expFreeParamList(this);

    return 0;
    }


/*** expUnlinkParams - do an objClose() on the objects in the param list
 ***/
void
expUnlinkParams(pParamObjects objlist, int start, int end)
    {
    int i;

	end = (end >= 0)?end:(objlist->nObjects-1);

	for(i=start; i<=end; i++)
	    if (objlist->Objects[i])
		{
		objClose(objlist->Objects[i]);
		expModifyParamByID(objlist, i, NULL);
		}

    return;
    }

/*** expLinkParams - do an objLinkTo() on the objects in the param list
 ***/
void
expLinkParams(pParamObjects objlist, int start, int end)
    {
    int i;

	end = (end >= 0)?end:(objlist->nObjects-1);

	for(i=start; i<=end; i++)
	    if (objlist->Objects[i])
		objLinkTo(objlist->Objects[i]);

    return;
    }


/*** expCopyParams - copy just certain objects from one param list
 *** to another.  This is different from expCopyList, since this function
 *** copies just certain objects, rather than copying the entire list's metadata
 *** like expCopyList does.
 ***
 *** Neither function *Links* to the copied objects.
 ***/
int
expCopyParams(pParamObjects src, pParamObjects dst, int start, int n_objects)
    {
    int i;

	/** Copy all? **/
	if (n_objects == -1)
	    n_objects = EXPR_MAX_PARAMS - start;

	/** Do the copy **/
	for(i=start; i<start+n_objects; i++)
	    {
	    dst->SeqIDs[i] = src->SeqIDs[i];
	    dst->GetTypeFn[i] = src->GetTypeFn[i];
	    dst->GetAttrFn[i] = src->GetAttrFn[i];
	    dst->SetAttrFn[i] = src->SetAttrFn[i];
	    if (dst->Names[i]) dst->nObjects--;
	    if (src->Names[i]) dst->nObjects++;
	    if (dst->Names[i] != NULL && (dst->Flags[i] & EXPR_O_ALLOCNAME))
		nmSysFree(dst->Names[i]);
	    dst->Flags[i] = src->Flags[i];
	    dst->Names[i] = NULL;
	    dst->Flags[i] &= ~EXPR_O_ALLOCNAME;
	    if (src->Names[i])
		{
		dst->Names[i] = nmSysStrdup(src->Names[i]);
		dst->Flags[i] |= EXPR_O_ALLOCNAME;
		}
	    dst->Objects[i] = src->Objects[i];
	    }

    return 0;
    }


/*** expCopyList - make a copy of a param objects list, in its entirety,
 *** possibly only including the first N objects (set n_objects to -1 to
 *** include all objects)
 ***/
int
expCopyList(pParamObjects src, pParamObjects dst, int n_objects)
    {
    int i;

	/** Copy all? **/
	if (n_objects == -1)
	    n_objects = EXPR_MAX_PARAMS;

	/** Might need to deallocate strings in dst **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    if (dst->Names[i] != NULL && (dst->Flags[i] & EXPR_O_ALLOCNAME))
		nmSysFree(dst->Names[i]);

	/** For most things, just a straight memcpy will do **/
	memcpy(dst, src, sizeof(ParamObjects));

	/** Make copies of all names **/
	for(i=0;i<n_objects;i++)
	    if (dst->Names[i] != NULL)
		{
		dst->Names[i] = nmSysStrdup(dst->Names[i]);
		dst->Flags[i] |= EXPR_O_ALLOCNAME;
		}

	/** If not copying all, clear the ones we don't want **/
	for(i=n_objects; i<EXPR_MAX_PARAMS; i++)
	    {
	    if (dst->Names[i] != NULL)
		{
		dst->Names[i] = NULL;
		dst->Objects[i] = NULL;
		dst->Flags[i] = 0;
		dst->nObjects--;
		}
	    }

	/** This is a transient property anyhow **/
	dst->CurControl = NULL;

    return 0;
    }


/*** expLookupParam - lookup a param in the object list, and return its ID if
 *** we find it, or -1 if not found.
 ***/
int
expLookupParam(pParamObjects this, char* name, int flags)
    {
    int i, idx;

	/** Search for it **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    {
	    if (flags & EXPR_F_REVERSE)
		idx = (EXPR_MAX_PARAMS-1) - i;
	    else
		idx = i;
	    if (this->Names[idx] != NULL && !strcmp(name, this->Names[idx]))
		return idx;
	    }

    return -1;
    }


/*** expAddParamToList - adds a new parameter slot to the parameter listing,
 *** possibly with or without a direct object reference (obj can be NULL).
 *** The name, slot number, and flags (EXPR_O_xxx) are used when the 
 *** compiler builds the expression tree.
 ***/
int 
expAddParamToList(pParamObjects this, char* name, pObject obj, int flags)
    {
    int i, exist;

    	/** Too many? **/
	if (this->nObjects >= EXPR_MAX_PARAMS) 
	    {
	    mssError(1,"EXP","Parameter Object list overflow -- too many objects in expression");
	    return -1;
	    }

	/** Already exists? **/
	exist = expLookupParam(this, name, flags);
	if (exist >= 0 && !(flags & (EXPR_O_ALLOWDUPS | EXPR_O_REPLACE)))
	    {
	    mssError(1,"EXP","Parameter Object name %s already exists", name);
	    return -1;
	    }
	if (!(flags & EXPR_O_REPLACE))
	    exist = -1;

	/** Ok, add parameter. **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    {
	    if ((this->Names[i] == NULL && exist == -1) || i == exist)
		{
		/** Setup the entry for this parameter. **/
		this->SeqIDs[i] = EXP.ModSeqID++;
		this->Objects[i] = obj;
		this->GetTypeFn[i] = objGetAttrType;
		this->GetAttrFn[i] = objGetAttrValue;
		this->SetAttrFn[i] = objSetAttrValue;
		if (i != exist) this->nObjects++;
		if (this->Names[i] && (this->Flags[i] & EXPR_O_ALLOCNAME))
		    nmSysFree(this->Names[i]);
		this->Flags[i] = flags | EXPR_O_CHANGED;
		if (flags & EXPR_O_ALLOCNAME)
		    {
		    this->Names[i] = nmSysStrdup(name);
		    }
		else
		    {
		    this->Names[i] = name;
		    }

		/** Check for parent id and current id **/
		if (flags & EXPR_O_PARENT)
		    this->ParentID = i;
		else if ((flags & EXPR_O_CURRENT) && this->CurrentID >= 0 && !(flags & EXPR_O_PRESERVEPARENT))
		    this->ParentID = this->CurrentID;
		if (flags & EXPR_O_CURRENT) this->CurrentID = i;
		if (this->nObjects == 1) this->CurrentID = i;

		/** Set modified. **/
		this->ModCoverageMask |= (1<<i);

		return i;
		}
	    }

    return -1;
    }


/*** expRemoveParamFromList - removes an existing parameter from the param
 *** objects list, by name.
 ***/
int
expRemoveParamFromList(pParamObjects this, char* name)
    {
    int i;

    	/** Find the thing and delete it **/
	i = expLookupParam(this, name, 0);
	if (i < 0) return -1;

    return expRemoveParamFromListById(this, i);
    }

int
expRemoveParamFromListById(pParamObjects this, int i)
    {

	/** Remove it **/
	if (this->Flags[i] & EXPR_O_ALLOCNAME) nmSysFree(this->Names[i]);
	this->Flags[i] = 0;
	this->Objects[i] = NULL;
	this->nObjects--;
	this->Names[i] = NULL;
	if (this->CurrentID == i)
	    {
	    if (i==0) this->CurrentID = -1;
	    else this->CurrentID--;
	    }
	if (this->ParentID == i)
	    {
	    if (i==0) this->ParentID = -1;
	    else this->ParentID--;
	    }

    return 0;
    }


/*** expModifyParam - sets a new object for a given parameter slot (by 
 *** name).  This is used to update the object list so that the next time
 *** the evaluator runs it, it will reference attributes in a new or
 *** different object.
 ***/
int 
expModifyParam(pParamObjects this, char* name, pObject replace_obj)
    {
    int slot_id = -1;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    slot_id = expLookupParam(this, name, 0);
	    }
	if (slot_id < 0) return -1;

    return expModifyParamByID(this, slot_id, replace_obj);
    }

int
expModifyParamByID(pParamObjects this, int slot_id, pObject replace_obj)
    {

	/** Replace the object. **/
	this->Objects[slot_id] = replace_obj;
	this->Flags[slot_id] |= EXPR_O_CHANGED;
	this->SeqIDs[slot_id] = EXP.ModSeqID++;

    return slot_id;
    }


#if 00
/*** expSyncModified - synchronizes the status indicators in a tree
 *** to a possibly modified object list structure.  Sets the flags
 *** field to EXPR_F_NEW if the object has a status of EXPR_O_CHANGED,
 *** and propogates the stale flags up the tree to parent items.  This
 *** way, a minimum of the tree is recalculated when only one of several
 *** objects are modified.
 ***/
int
expSyncModify(pExpression tree, pParamObjects objlist)
    {
    int s=0,i;

    	/** Check sub objects, return stale if one does. **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    if (expSyncModify((pExpression)(tree->Children.Items[i]),objlist)) s = 1;
	    }

    	/** Mark this one stale? **/
	if (s || (tree->NodeType == EXPR_N_PROPERTY && (objlist->Flags[expObjID(tree,objlist)] & EXPR_O_CHANGED)))
	    {
	    tree->Flags |= EXPR_F_NEW;
	    s = 1;
	    }

	/** Update objlist (clear CHANGED flags?) **/
	if (tree->Parent == NULL)
	    for(i=0;i<objlist->nObjects;i++) objlist->Flags[i] &= ~EXPR_O_CHANGED;

    return s;
    }
#endif


/*** expReplaceID - walks through the expression tree, changing all object
 *** id's that match oldid to newid.
 ***/
int
expReplaceID(pExpression this, int oldid, int newid)
    {
    int i;

    	/** Check this id. **/
	if (this->ObjID == oldid) this->ObjID = newid;

	/** Check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expReplaceID((pExpression)(this->Children.Items[i]), oldid, newid);
	    }

    return 0;
    }


/*** expReplaceVariableID - walks through the expression tree, changing all object
 *** id's that are not frozen to newid.
 ***/
int
expReplaceVariableID(pExpression this, int newid)
    {
    int i;

    	/** Check this id. **/
	if (this->Flags & EXPR_F_FREEZEEVAL) return 0;
	if (this->ObjID >= 0 && 
	    (this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT)) 
	         this->ObjID = newid;

	/** Check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expReplaceVariableID((pExpression)(this->Children.Items[i]), newid);
	    }

    return 0;
    }


/*** expFreezeOne - evaluates all id's in a tree with object id freeze_id
 *** and sets those object's values as such.  It then marks them as
 *** EXPR_F_FREEZEEVAL so that the next eval of the tree views such nodes
 *** as constants and does not evaluate them.
 ***/
int
expFreezeOne(pExpression this, pParamObjects objlist, int freeze_id)
    {
    int i, oldflags;

    	/** Is this a PROPERTY object and does not match freeze_id?? **/
	if ((this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT) && (this->ObjID == freeze_id || (this->ObjID == EXPR_OBJID_PARENT && objlist->ParentID == freeze_id)))
	    {
	    oldflags = this->Flags;
	    this->Flags &= ~EXPR_F_FREEZEEVAL;
	    if (expEvalTree(this,objlist) < 0)
		{
		this->Flags = oldflags;
		return -1;
		}
	    this->Flags |= EXPR_F_FREEZEEVAL;
	    return 0;
	    }

	/** Otherwise, check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    if (expFreezeOne((pExpression)(this->Children.Items[i]), objlist, freeze_id) < 0)
		return -1;
	    }

    return 0;
    }


/*** expFreezeEval - evaluates all id's in a tree with the exception of 
 *** freeze_id and sets those object's values as such.  It then marks them
 *** as EXPR_F_FREEZEEVAL so that the next eval of the tree views such nodes
 *** as constants and does not evaluate them.
 ***/
int
expFreezeEval(pExpression this, pParamObjects objlist, int freeze_id)
    {
    int i, oldflags;

    	/** Is this a PROPERTY object and does not match freeze_id?? **/
	if ((this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT) && this->ObjID != -1 && this->ObjID != freeze_id)
	    {
	    oldflags = this->Flags;
	    this->Flags &= ~EXPR_F_FREEZEEVAL;
	    if (expEvalTree(this,objlist) < 0)
		{
		this->Flags = oldflags;
		return -1;
		}
	    this->Flags |= EXPR_F_FREEZEEVAL;
	    return 0;
	    }

	/** Otherwise, check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    if (expFreezeEval((pExpression)(this->Children.Items[i]), objlist, freeze_id) < 0)
		return -1;
	    }

    return 0;
    }


/*** expResetAggregates - resets the aggregate min/max/cnt/sum accumulators for
 *** either all objects or a given object id.  Set object id to -1 to reset for
 *** all objects.
 ***/
int
expResetAggregates(pExpression this, int reset_id, int level)
    {
    /*this->Flags |= EXPR_F_DORESET;*/
    exp_internal_ResetAggregates(this,reset_id, level);
    return 0;
    }

int
exp_internal_ResetAggregates(pExpression this, int reset_id, int level)
    {
    int i;
    int found_agg=0,rval;

    	/** Check reset on this. **/
	if (this->NodeType == EXPR_N_FUNCTION && (this->Flags & EXPR_F_AGGREGATEFN) &&
	    this->AggLevel == level && (this->ObjID == reset_id || reset_id < 0))
	    {
	    this->AggCount = 0;
	    this->AggValue = 0;
	    /*this->Integer = 0;*/
	    if (this->AggExp)
	        {
	        this->AggExp->Integer = 0;
	        this->AggExp->Types.Double = 0;
	        this->AggExp->Types.Money.WholePart = 0;
	        this->AggExp->Types.Money.FractionPart = 0;
	        this->Flags |= EXPR_F_AGGLOCKED;
		}
	    this->Integer = 0;
	    this->Types.Double = 0;
	    this->Types.Money.WholePart = 0;
	    this->Types.Money.FractionPart = 0;
	    if (!strcmp(this->Name,"count")) 
	        {
		this->Flags &= ~EXPR_F_NULL;
		this->DataType = DATA_T_INTEGER;
		this->Integer = 0;
		}
	    else
	        {
		if (this->AggExp) this->AggExp->Flags |= EXPR_F_NULL;
		this->Flags |= EXPR_F_NULL;
		}
	    found_agg = 1;
	    }

	/** Check all sub-nodes **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    rval = exp_internal_ResetAggregates((pExpression)(this->Children.Items[i]), reset_id, level);
	    if (rval) found_agg = 1;
	    }

    return found_agg;
    }


/*** expUnlockAggregates - "unlock" the aggregate nodes so that they can be evaluated
 *** in the next expEvalTree operation.
 ***/
int
expUnlockAggregates(pExpression this, int level)
    {
    int i;

    	/** Unlock on this node. **/
	if (this->AggLevel == level)
	    this->Flags &= ~EXPR_F_AGGLOCKED;

	/** Unlock child nodes **/
	for(i=0;i<this->Children.nItems;i++) 
	    expUnlockAggregates((pExpression)(this->Children.Items[i]), level);

    return 0;
    }


/*** expSetParamFunctionsByID - set the functions that will be used to get/set
 *** paramobjects attribute values and types.  Used for "custom" objects and
 *** such.
 ***/
int
expSetParamFunctionsByID(pParamObjects this, int id, int (*type_fn)(), int (*get_fn)(), int (*set_fn)())
    {

	/** Set the functions. **/
	if (type_fn == NULL && get_fn == NULL && set_fn == NULL)
	    {
	    this->GetTypeFn[id] = objGetAttrType;
	    this->GetAttrFn[id] = objGetAttrValue;
	    this->SetAttrFn[id] = objSetAttrValue;
	    }
	else
	    {
	    this->GetTypeFn[id] = type_fn;
	    this->GetAttrFn[id] = get_fn;
	    this->SetAttrFn[id] = set_fn;
	    }

    return 0;
    }


/*** expSetParamFunctions - set the functions that will be used to get/set
 *** paramobjects attribute values and types.  Used for "custom" objects and
 *** such.
 ***/
int
expSetParamFunctions(pParamObjects this, char* name, int (*type_fn)(), int (*get_fn)(), int (*set_fn)())
    {
    int slot_id = -1;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    slot_id = expLookupParam(this, name, 0);
	    }
	if (slot_id < 0) return -1;


    return expSetParamFunctionsByID(this, slot_id, type_fn, get_fn, set_fn);
    }


/*** expRemapID - remaps an ID so that an object ID referenced in the expression
 *** tree actually references a _different_ object ID in the object list.  Also
 *** used to mark an object within an expression CONSTANT so it does NOT re-
 *** evaluate.
 ***/
int
expRemapID(pExpression tree, int exp_obj_id, int objlist_obj_id)
    {
    int i;

	/** No control block? **/
    	if (!tree->Control) exp_internal_SetupControl(tree);

	/** Setup remapping **/
	if (!tree->Control->Remapped)
	    {
	    tree->Control->Remapped = 1;
	    for(i=0;i<EXPR_MAX_PARAMS;i++) tree->Control->ObjMap[i] = i;
	    }
	tree->Control->ObjMap[exp_obj_id] = objlist_obj_id;

    return 0;
    }


/*** expClearRemapping - Removes all remapping that has been done on an expression's
 *** parameter objects.
 ***/
int
expClearRemapping(pExpression tree)
    {
    if (tree->Control) tree->Control->Remapped = 0;
    return 0;
    }


/*** expObjChanged - indicates that the value of an object has changed, so it
 *** should be re-evaluated on the next pass.
 ***/
int
expObjChanged(pParamObjects objlist, pObject obj)
    {
    int i, found_obj = -1;

	/** Find it **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    {
	    if (objlist->Objects[i] == obj)
		{
		/** Got it.  Give it a new serial number. **/
		objlist->SeqIDs[i] = EXP.ModSeqID++;
		found_obj = i;
		break;
		}
	    }

    return found_obj;
    }


/*** expContainsAttr - determines whether an expression tree references a given
 *** attribute, by name and object id (id from param objects)
 ***/
int
expContainsAttr(pExpression exp, int objid, char* attrname)
    {
    int i;

	/** Expression not dependent on this object? **/
	if (!(exp->ObjCoverageMask & (1<<objid)))
	    return 0;

	/** Is this the attribute? **/
	if (exp->NodeType == EXPR_N_PROPERTY && !strcmp(exp->Name, attrname))
	    return 1;

	/** Search subexpressions **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    if (expContainsAttr((pExpression)exp->Children.Items[i], objid, attrname))
		return 1;
	    }

    return 0;
    }


/*** expAllObjChanged() - indicate that all objects in the param objects list
 *** have potentially changed.
 ***/
int
expAllObjChanged(pParamObjects objlist)
    {

	/** stamp a new objlist serial id **/
	objlist->PSeqID = EXP.PSeqID++;

    return 0;
    }


/*** expSetCurrentID() - set the object that is considered the 'current' object,
 *** that is, the one referred to by :attr without a :obj:attr.
 ***/
int
expSetCurrentID(pParamObjects objlist, int current_id)
    {
    if (current_id >= 0 && current_id < objlist->nObjects)
	objlist->CurrentID = current_id;
    return 0;
    }
