#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_xact.c: Implements simple transaction control	*/
/*		to make possible some grouped operations.		*/
/************************************************************************/



/** Data structures for handling the call indirection **/
typedef struct
    {
    pObject	Obj;
    pObjTrxTree	Trx;
    void*	LLParam;
    }
    ObjTrxPtr, *pObjTrxPtr;

typedef struct
    {
    pObject	Obj;
    pObjTrxPtr	Inf;
    pObjQuery	Query;
    void*	LLParam;
    }
    ObjTrxQuery, *pObjTrxQuery;


/*** obj_internal_AllocTree - allocate a tree element for the transaction
 *** tree structure.
 ***/
pObjTrxTree
obj_internal_AllocTree()
    {
    pObjTrxTree this;
    this = (pObjTrxTree)nmMalloc(sizeof(ObjTrxTree));
    if (!this) return NULL;
    memset(this,0,sizeof(ObjTrxTree));
    this->Status = OXT_S_PENDING;
    this->AllocObj = 0;
    this->AttrValue = NULL;
    xaInit(&(this->Children),16);
    SETMAGIC(this, MGK_OXT);
    return this;
    }


/*** obj_internal_FindTree - finds any tree item corresponding to the
 *** given object path.
 ***/
pObjTrxTree
obj_internal_FindTree(pObjTrxTree oxt, char* path)
    {
    pObjTrxTree search,tmp,find;
    int i;

	ASSERTMAGIC(oxt, MGK_OXT);

    	/** Start at tree root **/
	if (strncmp(path,oxt->PathPtr,strlen(oxt->PathPtr))) return NULL;
	search = oxt;
	while(search)
	    {
	    ASSERTMAGIC(search, MGK_OXT);

	    /** Found the node? **/
	    if (!strcmp(path,search->PathPtr)) break;

	    /** Search node's children **/
	    find = NULL;
	    for(i=0;i<search->Children.nItems;i++)
	        {
		tmp = (pObjTrxTree)(search->Children.Items[i]);
		ASSERTMAGIC(tmp, MGK_OXT);
		if (!strncmp(path,tmp->PathPtr,strlen(tmp->PathPtr)))
		    {
		    find = tmp;
		    break;
		    }
		}
	    search=find;
	    }

    return search;
    }


/*** obj_internal_FreeTree - frees a transaction tree or subtree.
 ***/
int
obj_internal_FreeTree(pObjTrxTree oxt)
    {
    int i;
    pObjTrxTree tmp;

	ASSERTMAGIC(oxt, MGK_OXT);

    	/** If it has a parent, remove from that child list. **/
	if (oxt->Parent) 
	    {
	    ASSERTMAGIC(oxt->Parent, MGK_OXT);
	    xaRemoveItem(&(oxt->Parent->Children),xaFindItem(&(oxt->Parent->Children),oxt));
	    oxt->Parent = NULL;
	    }

	/** If any parallel ones, remove from those. **/
	if (oxt->Parallel) obj_internal_FreeTree(oxt->Parallel);

	/** Remove children **/
	for(i=0;i<oxt->Children.nItems;i++)
	    {
	    tmp = (pObjTrxTree)(oxt->Children.Items[i]);
	    tmp->Parent = NULL;
	    obj_internal_FreeTree(tmp);
	    }

	/** Free this one. **/
	xaDeInit(&(oxt->Children));
	if (oxt->AttrValue) nmSysFree(oxt->AttrValue);
	if (oxt->AllocObj)
	    {
	    /*xaDeInit(&(((pObject)(oxt->Object))->Attrs));
	    nmFree(oxt->Object,sizeof(Object));*/
	    obj_internal_FreeObj(oxt->Object);
	    }
	ASSERTMAGIC(oxt, MGK_OXT);
	nmFree(oxt,sizeof(ObjTrxTree));
    
    return 0;
    }


/*** obj_internal_AddChildTree - adds a child oxt node to the parent oxt
 *** node and updates the data structures.
 ***/
int
obj_internal_AddChildTree(pObjTrxTree parent_oxt, pObjTrxTree child_oxt)
    {
    ASSERTMAGIC(parent_oxt, MGK_OXT);
    ASSERTMAGIC(child_oxt, MGK_OXT);
    child_oxt->Parent = parent_oxt;
    xaAddItem(&(parent_oxt->Children), (void*)child_oxt);
    return 0;
    }


/*** obj_internal_SetTreeAttr - set an attribute in the OXT transaction tree.
 *** This is mainly a convenience function for objectsystem drivers.
 ***/
int
obj_internal_SetTreeAttr(pObjTrxTree oxt, int type, pObjData val)
    {
    void* valcpy;
    int len;

	ASSERTMAGIC(oxt, MGK_OXT);

    	/** Set type **/
	oxt->AttrType = type;

	/** Already set?  Unset it if so. **/
	if (oxt->AttrValue)
	    {
	    nmSysFree(oxt->AttrValue);
	    oxt->AttrValue = NULL;
	    }

	/** NULL?  Return and leave value unset. **/
	if (!val) return 0;

	/** Figure length of allocated value space **/
	switch(type)
	    {
	    case DATA_T_INTEGER: len = 4; break;
	    case DATA_T_STRING: len = strlen(val->String)+1; break;
	    case DATA_T_MONEY: len = sizeof(MoneyType); break;
	    case DATA_T_DATETIME: len = sizeof(DateTime); break;
	    case DATA_T_DOUBLE: len = sizeof(double); break;
	    default: return -1;
	    }
	valcpy = (void*)nmSysMalloc(len);

	/** Allocate and do the data copy. **/
	if (type == DATA_T_STRING || type == DATA_T_DATETIME || type == DATA_T_MONEY)
	    memcpy(valcpy,*(void**)val,len);
	else
	    memcpy(valcpy,(void*)val,len);
	oxt->AttrValue = valcpy;

    return 0;
    }


/*** obj_internal_FindAttrOxt - locate an attribute OXT with a given attribute
 *** name under the current Oxt tree structure.
 ***/
pObjTrxTree
oxt_internal_FindAttrOxt(pObjTrxTree oxt, char* attrname)
    {
    pObjTrxTree search_oxt;
    int i;

	ASSERTMAGIC(oxt, MGK_OXT);

    	/** Search the transaction tree for the oxt structure **/
	for(i=0;i<oxt->Children.nItems;i++)
	    {
	    search_oxt = (pObjTrxTree)(oxt->Children.Items[i]);
	    if (!strcmp(search_oxt->AttrName, attrname)) return search_oxt;
	    }

    return NULL;
    }


/*** oxt_internal_FindOxt - searches the given transaction tree for a given
 *** path, possibly allocating a new structure for the given pointer.
 ***/
pObjTrxTree*
oxt_internal_FindOxt(pObject obj, pObjTrxTree* oxt, pObjTrxTree* new_oxt)
    {
    pObjTrxTree* pass_oxt;
    pObjTrxTree tmp_oxt;
    int prefix_cnt, new_cnt, i;

	/** Another gcc warning suppression.  tmp_oxt is not used uninitialized,
	 ** but gcc can't seem to see that.
	 **/
	tmp_oxt = NULL;

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	if (*oxt)
	    {
	    ASSERTMAGIC(*oxt, MGK_OXT);

	    prefix_cnt = obj_internal_PathPrefixCnt(obj->Pathname, 
	        ((pObject)((*oxt)->Object))->Pathname);
	    if (prefix_cnt == 0)
	        {
		/** Opening object referenced in transaction.  Use same oxt. **/
		pass_oxt = oxt;
		}
	    else if (prefix_cnt > 0)
	        {
		/** Ok, prefixed.  Create a new oxt structure somewhere in here. **/
		while(prefix_cnt > 1)
		    {
		    /** Search for a closer prefix? **/
		    new_cnt = -1;
		    for(i=0;i<(*oxt)->Children.nItems;i++)
		        {
			tmp_oxt = (pObjTrxTree)((*oxt)->Children.Items[i]);
			if (tmp_oxt->Object == NULL) continue;
			new_cnt = obj_internal_PathPrefixCnt(obj->Pathname, 
				((pObject)(tmp_oxt->Object))->Pathname);
			if (new_cnt == prefix_cnt-1)
			    {
			    prefix_cnt = new_cnt;
			    break;
			    }
			}
		    if (new_cnt == -1) prefix_cnt = -1;
		    }

		/** Couldn't find it as prefixed? **/
		if (prefix_cnt != 1) 
		    {
		    /** We don't handle this one. **/
		    printf("Transaction not contiguous.\n");
		    exit(1);
		    }

		/** Ok, make the new transaction element **/
		*new_oxt = obj_internal_AllocTree();
		(*new_oxt)->Object = (void*)obj;
		obj_internal_AddChildTree(tmp_oxt, *new_oxt);
		pass_oxt = new_oxt;
		}
	    else
	        {
		pass_oxt = new_oxt;
		}
	    }
	else
	    {
	    pass_oxt = oxt;
	    }

	ASSERTMAGIC(*pass_oxt, MGK_OXT);

    return pass_oxt;
    }


/*** oxtOpen - open a new object.  Create the transaction tree entry and
 *** try to get the driver to process it.  If it doesn't, we just sit on
 *** the thing until we have a chance to send it with another request.
 ***/
void*
oxtOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pObjTrxTree object_oxt = NULL;
    pObjTrxPtr inf;
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt = NULL;
    int was_null=0;

	oxt = &object_oxt;
	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Allocate the inf and the tree. **/
	inf = (pObjTrxPtr)nmMalloc(sizeof(ObjTrxPtr));
	if (!inf) return NULL;
	inf->Obj = obj;

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
	if (pass_oxt && *pass_oxt) ASSERTMAGIC(*oxt, MGK_OXT);
	if (new_oxt) ASSERTMAGIC(new_oxt, MGK_OXT);
	if (new_oxt == NULL) was_null = 1;
	if (new_oxt) new_oxt->OpType = OXT_OP_NONE;

	/** Make the driver call. **/
	inf->LLParam = obj->TLowLevelDriver->Open(obj, mask, systype, usrtype, pass_oxt);

	/** Was the pass_oxt completed or failed? **/
	if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

	/** Did it fail?  Remove any new oxt if so. **/
	if ((inf->LLParam) == NULL)
	    {
	    if (!was_null && new_oxt != NULL)
	        {
		obj_internal_FreeTree(new_oxt);
		new_oxt = NULL;
		}
	    nmFree(inf,sizeof(ObjTrxPtr));
	    inf = NULL;
	    }

	/** Did we just link two transactions together? **/
	if (*oxt && was_null && new_oxt != NULL)
	    {
	    /** Ha.  We don't handle this yet.  FIXME. **/
	    printf("Illegal transaction linkage.\n");
	    abort();
	    }

	/** Set the trx pointer for later use. **/
	if (inf) inf->Trx = (*pass_oxt);

    return (void*)inf;
    }


/*** oxtClose - close an open object of some sort.
 ***/
int
oxtClose(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxTree* pass_oxt;
    int rval;
    
	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Call the driver to make the close operation. **/
    	pass_oxt = &(this->Trx);
	rval = this->Obj->TLowLevelDriver->Close(this->LLParam, pass_oxt);

	if (*pass_oxt) ASSERTMAGIC(*pass_oxt, MGK_OXT);

	/** Completed or error? **/
	if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    rval = -1;
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

	/** Free the ptr structure. **/
	nmFree(this,sizeof(ObjTrxPtr));

    return rval;
    }


/*** oxtDeleteObj - delete an already open object, resulting in its closure
 *** This has similar logic to Close().
 ***/
int
oxtDeleteObj(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxTree* pass_oxt;
    int rval;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Call the driver to make the delete operation. **/
    	pass_oxt = &(this->Trx);
	if (*pass_oxt) ASSERTMAGIC(*pass_oxt, MGK_OXT);
	if (this->Obj->TLowLevelDriver->DeleteObj == NULL)
	    {
	    mssError(1,"OXT","oxtDeleteObj: [%s] objects do not support deletion",this->Obj->TLowLevelDriver->Name);
	    return -1;
	    }
	rval = this->Obj->TLowLevelDriver->DeleteObj(this->LLParam, pass_oxt);

	/** Completed or error? **/
	if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    rval = -1;
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

	/** Free the ptr structure. **/
	nmFree(this,sizeof(ObjTrxPtr));

    return rval;
    }


/*** oxtCommit - commit changes
 ***/
int
oxtCommit(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxTree* pass_oxt;
    int rval;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Call the driver to make the close operation. **/
	if (!this->Obj->TLowLevelDriver->Commit) return 0;
    	pass_oxt = &(this->Trx);
	if (pass_oxt && *pass_oxt) ASSERTMAGIC(*pass_oxt, MGK_OXT);
	rval = this->Obj->TLowLevelDriver->Commit(this->LLParam, pass_oxt);

	/** Completed or error? **/
	if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if (pass_oxt && *pass_oxt && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    rval = -1;
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

    return rval;
    }


/*** oxtCreate - create a new object without opening it.  Similar
 *** logic to oxtOpen.
 ***/
int
oxtCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pObjTrxTree object_oxt = NULL;
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt=NULL;
    int was_null = 0;
    int rval;

	oxt = &object_oxt;
	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
	if (pass_oxt && *pass_oxt) ASSERTMAGIC(*pass_oxt, MGK_OXT);
	if (new_oxt) ASSERTMAGIC(new_oxt, MGK_OXT);
	if (new_oxt == NULL) was_null = 1;
	if (new_oxt) new_oxt->OpType = OXT_OP_CREATE;

	/** Make the driver call. **/
	rval = obj->TLowLevelDriver->Create(obj, mask, systype, usrtype, pass_oxt);

	/** Was the pass_oxt completed or failed? **/
	if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

	/** Did it fail?  Remove any new oxt if so. **/
	if (rval < 0)
	    {
	    if (!was_null && new_oxt != NULL)
	        {
		obj_internal_FreeTree(new_oxt);
		new_oxt = NULL;
		}
	    }

	/** Did we just link two transactions together? **/
	if (*oxt && was_null && new_oxt != NULL)
	    {
	    /** Ha.  We don't handle this yet.  FIXME. **/
	    printf("Illegal transaction linkage.\n");
	    abort();
	    }

    return 0;
    }


/*** oxtDelete - delete an object.  Logic to find the object is similar to
 *** that found in oxtCreate and oxtOpen.
 ***/
int
oxtDelete(pObject obj, pObjTrxTree* oxt)
    {
    pObjTrxTree object_oxt = NULL;
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt=NULL;
    int was_null = 0;
    int rval;

	oxt = &object_oxt;
	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
	if (pass_oxt && *pass_oxt) ASSERTMAGIC(*pass_oxt, MGK_OXT);
	if (new_oxt) ASSERTMAGIC(new_oxt, MGK_OXT);
	if (new_oxt == NULL) was_null = 1;
	if (new_oxt) new_oxt->OpType = OXT_OP_DELETE;

	/** Make the driver call. **/
	rval = obj->TLowLevelDriver->Delete(obj, pass_oxt);

	/** Was the pass_oxt completed or failed? **/
	if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if ((*pass_oxt) && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

	/** Did it fail?  Remove any new oxt if so. **/
	if (rval < 0)
	    {
	    if (!was_null && new_oxt != NULL)
	        {
		obj_internal_FreeTree(new_oxt);
		new_oxt = NULL;
		}
	    }

	/** Did we just link two transactions together? **/
	if (*oxt && was_null && new_oxt != NULL)
	    {
	    /** Ha.  We don't handle this yet.  FIXME. **/
	    printf("Illegal transaction linkage.\n");
	    abort();
	    }

    return 0;
    }


/*** oxtSetAttrValue - set the value of an attribute.  This is no passthru.
 *** We treat this in the oxt structure as if it were a child item.
 ***/
int
oxtSetAttrValue(void* this_v, char* attrname, int datatype, void* val, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxTree new_oxt = NULL;
    pObjTrxTree* pass_oxt;
    int rval;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Are we in a transaction? **/
	if (this->Trx)
	    {
	    /** In transaction, make a sub-oxt **/
	    new_oxt = obj_internal_AllocTree();
	    new_oxt->OpType = OXT_OP_SETATTR;
	    obj_internal_AddChildTree(this->Trx, new_oxt);
	    new_oxt->Status = OXT_S_PENDING;
	    pass_oxt = &new_oxt;
	    }
	else
	    {
	    pass_oxt = oxt;
	    }

	/** Call the driver. **/
	rval = this->Obj->TLowLevelDriver->SetAttrValue(this->LLParam, attrname, datatype, val, pass_oxt);

	/** Failed? **/
	if (rval < 0 && new_oxt)
	    {
	    obj_internal_FreeTree(new_oxt);
	    new_oxt = NULL;
	    }

	/** Completion? **/
	if (*pass_oxt && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }
	else if (*pass_oxt && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    rval = -1;
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    *pass_oxt = NULL;
	    }

    return rval;
    }


/*** oxtOpenAttr - open an attribute as if it were an object, for
 *** reading and/or writing to its content.
 ***/
void*
oxtOpenAttr(void* this_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)this_v;
    pObjTrxPtr new_this;
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt=NULL;
    void* rval;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

	/** GRB This interface is a kludge.  We'll probably be removing this
	 ** in the future.  for now, just error out.
	 **/
	return NULL;

    	/** Are we in a transaction? **/
	if (this->Trx)
	    {
	    /** In transaction, make a sub-oxt **/
	    new_oxt = obj_internal_AllocTree();
	    new_oxt->OpType = OXT_OP_SETATTR;
	    obj_internal_AddChildTree(this->Trx, new_oxt);
	    new_oxt->Status = OXT_S_PENDING;
	    pass_oxt = &new_oxt;
	    }
	else
	    {
	    pass_oxt = oxt;
	    }
    
	/** Call the low level driver **/
	rval = this->Obj->TLowLevelDriver->OpenAttr(this->Obj,attrname,pass_oxt);

	/** Completion? **/
	if (*pass_oxt && (*pass_oxt)->Status == OXT_S_COMPLETE)
	    {
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    new_oxt = NULL;
	    }
	else if (*pass_oxt && (*pass_oxt)->Status == OXT_S_FAILED)
	    {
	    rval = NULL;
	    if (*oxt == *pass_oxt) *oxt = NULL;
	    obj_internal_FreeTree(*pass_oxt);
	    new_oxt = NULL;
	    }

	/** Did the call succeed? **/
	if (rval)
	    {
	    new_this = (pObjTrxPtr)nmMalloc(sizeof(ObjTrxPtr));
	    if (!this) rval = NULL;
	    new_this->LLParam = rval;
	    new_this->Trx = *pass_oxt;
	    new_this->Obj = this->Obj;
	    }
	else if (new_oxt)
	    {
	    obj_internal_FreeTree(new_oxt);
	    }

    return this;
    }


/*** oxtGetAttrValue -- even this is passthru for now.  Sigh.
 ***/
int
oxtGetAttrValue(void* this_v, char* attrname, int datatype, void* val, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetAttrValue(this->LLParam, attrname, datatype, val, oxt);
    }


/*** oxtOpenQuery -- passthru for now.
 ***/
void*
oxtOpenQuery(void* this_v, char* query, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxQuery qy;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Allocate the query **/
	qy = (pObjTrxQuery)nmMalloc(sizeof(ObjTrxQuery));
	if (!qy) return NULL;

	/** Call the low level driver. **/
	qy->Obj = this->Obj;
	qy->Inf = this;
	qy->LLParam = this->Obj->TLowLevelDriver->OpenQuery(this->LLParam,query,oxt);
	if (!(qy->LLParam))
	    {
	    nmFree(qy,sizeof(ObjTrxQuery));
	    return NULL;
	    }

    return (void*)qy;
    }


/*** oxtQueryDelete -- passthru for now.
 ***/
int
oxtQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    pObjTrxQuery qy = (pObjTrxQuery)(qy_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return qy->Obj->TLowLevelDriver->QueryDelete(qy->LLParam, oxt);
    }


/*** oxtQueryFetch -- passthru for now.
 ***/
void*
oxtQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pObjTrxPtr subobj;
    pObjTrxQuery qy = (pObjTrxQuery)qy_v;
    pObjTrxTree new_oxt = NULL;
    pObjTrxTree* pass_oxt = &new_oxt;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Allocate the subobject **/
	subobj = (pObjTrxPtr)nmMalloc(sizeof(ObjTrxPtr));
	if (!subobj) return NULL;

	/** In a transaction? **/
	if (qy->Inf->Trx)
	    {
	    new_oxt = obj_internal_AllocTree();
	    new_oxt->OpType = OXT_OP_NONE;
	    new_oxt->Status = OXT_S_COMPLETE;
	    obj_internal_AddChildTree(qy->Inf->Trx, new_oxt);
	    }
	
	/** Call the lowlevel driver **/
	subobj->LLParam = qy->Obj->TLowLevelDriver->QueryFetch(qy->LLParam, obj, mode, pass_oxt);
	if (!(subobj->LLParam)) 
	    {
	    nmFree(subobj,sizeof(ObjTrxPtr));
	    return NULL;
	    }
	subobj->Obj = obj;
	subobj->Trx = *pass_oxt;

    return subobj;
    }


/*** oxtQueryClose -- passthru for now.
 ***/
int
oxtQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pObjTrxQuery qy = (pObjTrxQuery)qy_v;

	if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);

    	/** Free the object and call the lowlevel. **/
	qy->Obj->TLowLevelDriver->QueryClose(qy->LLParam,oxt);
	nmFree(qy,sizeof(ObjTrxQuery));

    return 0;
    }


/*** oxtWrite -- passthru for now.  Later will be included as a part of
 *** the trans layer.
 ***/
int
oxtWrite(void* this_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->Write(this->LLParam,buffer,cnt,offset,flags,(this->Trx)?&(this->Trx):oxt);
    }


/*** oxtRead -- passthru to lowlevel for now.  Read and Write will
 *** not be passthru later when we implement the whole trans layer.
 ***/
int
oxtRead(void* this_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->Read(this->LLParam,buffer,maxcnt,offset,flags,oxt);
    }


/*** oxtGetAttrType -- passthru to lowlevel.
 ***/
int
oxtGetAttrType(void* this_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetAttrType(this->LLParam,attrname,oxt);
    }


/*** oxtAddAttr -- passthru to lowlevel for now.  If we ever add more
 *** transaction functionality, this may have to change.
 ***/
int
oxtAddAttr(void* this_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->AddAttr(this->LLParam, oxt);
    }


/*** oxtGetFirstAttr - passthru to lowlevel.
 ***/
char*
oxtGetFirstAttr(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetFirstAttr(this->LLParam,oxt);
    }


/*** oxtGetNextAttr - passthru to lowlevel.
 ***/
char*
oxtGetNextAttr(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetNextAttr(this->LLParam,oxt);
    }


/*** oxtGetFirstMethod - passthru to lowlevel.
 ***/
char*
oxtGetFirstMethod(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetFirstMethod(this->LLParam,oxt);
    }


/*** oxtGetNextMethod - passthru to lowlevel.
 ***/
char*
oxtGetNextMethod(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->GetNextMethod(this->LLParam,oxt);
    }


/*** oxtExecuteMethod - passthru to lowlevel.
 ***/
int
oxtExecuteMethod(void* this_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    return this->Obj->TLowLevelDriver->ExecuteMethod(this->LLParam,methodname,param,oxt);
    }


/*** oxtPresentationHints - passthru.
 ***/
pObjPresentationHints
oxtPresentationHints(void* this_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (oxt && *oxt) ASSERTMAGIC(*oxt, MGK_OXT);
    if (!(this->Obj->TLowLevelDriver->PresentationHints)) return NULL;
    return this->Obj->TLowLevelDriver->PresentationHints(this->LLParam,attrname,oxt);
    }

/*** oxtInfo - Return the capabilities of the object. Passthrough.
 ***/
int
oxtInfo(void* this_v, pObjectInfo info)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (!(this->Obj->TLowLevelDriver->Info)) return -1;
    return this->Obj->TLowLevelDriver->Info(this->LLParam,info);
    }

/*** oxtInitialize - initialize the transaction layer and cause it to register
 *** itself with the ObjectSystem.  The objectsystem will locate this driver
 *** based on the capability OBJDRV_C_ISTRANS.
 ***/
int
oxtInitialize()
    {
    pObjDriver drv;

    	/** Allocate the object. **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv)
	    {
	    mssError(0,"OXT","Initialize - could not allocate driver structure");
	    return -1;
	    }
	memset(drv,0,sizeof(ObjDriver));

	/** Fill in the function members and capabilities, etc **/
	strcpy(drv->Name,"OXT - ObjectSystem Transaction Layer");
	drv->Capabilities = OBJDRV_C_LLQUERY | OBJDRV_C_ISTRANS;
	xaInit(&(drv->RootContentTypes),16);
	drv->Open = oxtOpen;
	drv->Close = oxtClose;
	drv->Delete = oxtDelete;
	drv->DeleteObj = oxtDeleteObj;
	drv->Create = oxtCreate;
	drv->Read = oxtRead;
	drv->Write = oxtWrite;
	drv->OpenQuery = oxtOpenQuery;
	drv->QueryDelete = oxtQueryDelete;
	drv->QueryFetch = oxtQueryFetch;
	drv->QueryClose = oxtQueryClose;
	drv->GetAttrType = oxtGetAttrType;
	drv->GetAttrValue = oxtGetAttrValue;
	drv->SetAttrValue = oxtSetAttrValue;
	drv->AddAttr = oxtAddAttr;
	drv->OpenAttr = oxtOpenAttr;
	drv->GetFirstAttr = oxtGetFirstAttr;
	drv->GetNextAttr = oxtGetNextAttr;
	drv->GetFirstMethod = oxtGetFirstMethod;
	drv->GetNextMethod = oxtGetNextMethod;
	drv->ExecuteMethod = oxtExecuteMethod;
	drv->PresentationHints = oxtPresentationHints;
	drv->Commit = oxtCommit;
	drv->Info = oxtInfo;

	nmRegister(sizeof(ObjTrxPtr),"ObjTrxPtr");
	nmRegister(sizeof(ObjTrxQuery),"ObjTrxQuery");

	/** Register it now. **/
	objRegisterDriver(drv);

    return 0;
    }

