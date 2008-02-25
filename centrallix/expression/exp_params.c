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

/**CVSDATA***************************************************************

    $Id: exp_params.c,v 1.10 2008/02/25 23:14:33 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_params.c,v $

    $Log: exp_params.c,v $
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

    Revision 1.9  2007/12/05 18:48:02  gbeeley
    - (bugfix) Problem was causing several aggregate functions to not properly
      reset to NULL on a new grouping, when used in a context where a "group"
      may not have any rows at all.

    Revision 1.8  2007/03/21 04:48:08  gbeeley
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

    Revision 1.7  2007/03/04 05:04:47  gbeeley
    - (change) This is a change to the way that expressions track which
      objects they were last evaluated against.  The old method was causing
      some trouble with stale data in some expressions.

    Revision 1.6  2005/09/30 04:37:10  gbeeley
    - (change) modified expExpressionToPod to take the type.
    - (feature) got eval() working
    - (addition) added expReplaceString() to search-and-replace in an
      expression tree.

    Revision 1.5  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.4  2004/06/12 04:02:27  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.3  2002/11/22 19:29:36  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.2  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.1.1.1  2001/08/13 18:00:48  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


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

    return objlist;
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


/*** expCopyList - make a copy of a param objects list
 ***/
int
expCopyList(pParamObjects src, pParamObjects dst)
    {
    int i;

	/** Might need to deallocate strings in dst **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    if (dst->Names[i] != NULL && dst->Flags[i] & EXPR_O_ALLOCNAME)
		nmSysFree(dst->Names[i]);

	/** For most things, just a straight memcpy will do **/
	memcpy(dst, src, sizeof(ParamObjects));

	/** Make copies of all names **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    if (dst->Names[i] != NULL)
		{
		dst->Names[i] = nmSysStrdup(dst->Names[i]);
		dst->Flags[i] |= EXPR_O_ALLOCNAME;
		}

	/** This is a transient property anyhow **/
	dst->CurControl = NULL;

    return 0;
    }


/*** expLookupParam - lookup a param in the object list, and return its ID if
 *** we find it, or -1 if not found.
 ***/
int
expLookupParam(pParamObjects this, char* name)
    {
    int i;

	/** Search for it **/
	for(i=0;i<EXPR_MAX_PARAMS;i++)
	    if (this->Names[i] != NULL && !strcmp(name, this->Names[i]))
		return i;

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
    int i;

    	/** Too many? **/
	if (this->nObjects >= EXPR_MAX_PARAMS) 
	    {
	    mssError(1,"EXP","Parameter Object list overflow -- too many objects in expression");
	    return -1;
	    }

	/** Ok, add parameter. **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Names[i] == NULL)
	    {
	    /** Setup the entry for this parameter. **/
	    this->SeqIDs[i] = EXP.ModSeqID++;
	    this->Objects[i] = obj;
	    this->Flags[i] = flags | EXPR_O_CHANGED;
	    this->GetTypeFn[i] = objGetAttrType;
	    this->GetAttrFn[i] = objGetAttrValue;
	    this->SetAttrFn[i] = objSetAttrValue;
	    this->nObjects++;
	    if (flags & EXPR_O_ALLOCNAME)
	        {
		this->Names[i] = nmSysStrdup(name);
		}
	    else
	        {
		this->Names[i] = name;
		}

	    /** Check for parent id and current id **/
	    if (flags & EXPR_O_PARENT) this->ParentID = i;
	    if (flags & EXPR_O_CURRENT) this->CurrentID = i;
	    if (this->nObjects == 1) this->CurrentID = i;

	    /** Set modified. **/
	    this->ModCoverageMask |= (1<<i);

	    break;
	    }

    return 0;
    }


/*** expRemoveParamFromList - removes an existing parameter from the param
 *** objects list, by name.
 ***/
int
expRemoveParamFromList(pParamObjects this, char* name)
    {
    int i;

    	/** Find the thing and delete it **/
	i = expLookupParam(this, name);
	if (i < 0) return -1;

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
    int i;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    slot_id = expLookupParam(this, name);
	    }
	if (slot_id < 0) return -1;

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
    int i;

    	/** Is this a PROPERTY object and does not match freeze_id?? **/
	if ((this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT) && this->ObjID == freeze_id)
	    {
	    this->Flags &= ~EXPR_F_FREEZEEVAL;
	    expEvalTree(this,objlist);
	    this->Flags |= EXPR_F_FREEZEEVAL;
	    return 0;
	    }

	/** Otherwise, check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expFreezeOne((pExpression)(this->Children.Items[i]), objlist, freeze_id);
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
    int i;

    	/** Is this a PROPERTY object and does not match freeze_id?? **/
	if ((this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT) && this->ObjID != -1 && this->ObjID != freeze_id)
	    {
	    this->Flags &= ~EXPR_F_FREEZEEVAL;
	    expEvalTree(this,objlist);
	    this->Flags |= EXPR_F_FREEZEEVAL;
	    return 0;
	    }

	/** Otherwise, check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expFreezeEval((pExpression)(this->Children.Items[i]), objlist, freeze_id);
	    }

    return 0;
    }


/*** expResetAggregates - resets the aggregate min/max/cnt/sum accumulators for
 *** either all objects or a given object id.  Set object id to -1 to reset for
 *** all objects.
 ***/
int
expResetAggregates(pExpression this, int reset_id)
    {
    /*this->Flags |= EXPR_F_DORESET;*/
    exp_internal_ResetAggregates(this,reset_id);
    return 0;
    }

int
exp_internal_ResetAggregates(pExpression this, int reset_id)
    {
    int i;
    int found_agg=0,rval;

    	/** Check reset on this. **/
	if (this->NodeType == EXPR_N_FUNCTION && (this->Flags & EXPR_F_AGGREGATEFN) &&
	    (this->ObjID == reset_id || reset_id < 0))
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
	    rval = exp_internal_ResetAggregates((pExpression)(this->Children.Items[i]), reset_id);
	    if (rval) found_agg = 1;
	    }

    return found_agg;
    }


/*** expUnlockAggregates - "unlock" the aggregate nodes so that they can be evaluated
 *** in the next expEvalTree operation.
 ***/
int
expUnlockAggregates(pExpression this)
    {
    int i;

    	/** Unlock on this node. **/
	this->Flags &= ~EXPR_F_AGGLOCKED;

	/** Unlock child nodes **/
	for(i=0;i<this->Children.nItems;i++) 
	    expUnlockAggregates((pExpression)(this->Children.Items[i]));

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
    int i;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    slot_id = expLookupParam(this, name);
	    }
	if (slot_id < 0) return -1;

	/** Set the functions. **/
	this->GetTypeFn[slot_id] = type_fn;
	this->GetAttrFn[slot_id] = get_fn;
	this->SetAttrFn[slot_id] = set_fn;

    return 0;
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
