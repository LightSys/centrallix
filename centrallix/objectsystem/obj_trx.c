#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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

/**CVSDATA***************************************************************

    $Id: obj_trx.c,v 1.8 2003/07/15 19:42:34 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_trx.c,v $

    $Log: obj_trx.c,v $
    Revision 1.8  2003/07/15 19:42:34  gbeeley
    Don't try calling driver PresentationHints function if the driver
    does not implement that function.

    Revision 1.7  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.6  2003/04/25 05:06:58  gbeeley
    Added insert support to OSML-over-HTTP, and very remedial Trx support
    with the objCommit API method and Commit osdriver method.  CSV datafile
    driver is the only driver supporting it at present.

    Revision 1.5  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.4  2002/08/10 02:09:45  gbeeley
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

    Revision 1.3  2002/06/19 23:29:34  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:00  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:01  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


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
    xaInit(&(this->Children),16);
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

    	/** Start at tree root **/
	if (strncmp(path,oxt->PathPtr,strlen(oxt->PathPtr))) return NULL;
	search = oxt;
	while(search)
	    {
	    /** Found the node? **/
	    if (!strcmp(path,search->PathPtr)) break;

	    /** Search node's children **/
	    find = NULL;
	    for(i=0;i<search->Children.nItems;i++)
	        {
		tmp = (pObjTrxTree)(search->Children.Items[i]);
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

    	/** If it has a parent, remove from that child list. **/
	if (oxt->Parent) 
	    {
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
	    xaDeInit(&(((pObject)(oxt->Object))->Attrs));
	    nmFree(oxt->Object,sizeof(Object));
	    }
	nmFree(oxt,sizeof(ObjTrxTree));
    
    return 0;
    }


/*** obj_internal_AddChildTree - adds a child oxt node to the parent oxt
 *** node and updates the data structures.
 ***/
int
obj_internal_AddChildTree(pObjTrxTree parent_oxt, pObjTrxTree child_oxt)
    {
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

    	/** Set type **/
	oxt->AttrType = type;

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

    return pass_oxt;
    }


/*** oxtOpen - open a new object.  Create the transaction tree entry and
 *** try to get the driver to process it.  If it doesn't, we just sit on
 *** the thing until we have a chance to send it with another request.
 ***/
void*
oxtOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pObjTrxPtr inf;
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt = NULL;
    int was_null=0;

    	/** Allocate the inf and the tree. **/
	inf = (pObjTrxPtr)nmMalloc(sizeof(ObjTrxPtr));
	if (!inf) return NULL;
	inf->Obj = obj;

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
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
	    exit(1);
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
    
    	/** Call the driver to make the close operation. **/
    	pass_oxt = &(this->Trx);
	rval = this->Obj->TLowLevelDriver->Close(this->LLParam, pass_oxt);

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

    	/** Call the driver to make the close operation. **/
	if (!this->Obj->TLowLevelDriver->Commit) return 0;
    	pass_oxt = &(this->Trx);
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
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt=NULL;
    int was_null = 0;
    int rval;

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
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
	    exit(1);
	    }

    return 0;
    }


/*** oxtDelete - delete an object.  Logic to find the object is similar to
 *** that found in oxtCreate and oxtOpen.
 ***/
int
oxtDelete(pObject obj, pObjTrxTree* oxt)
    {
    pObjTrxTree* pass_oxt;
    pObjTrxTree new_oxt=NULL;
    int was_null = 0;
    int rval;

	/** Transaction in progress?  If so, and prefix matches, make a new oxt. **/
	pass_oxt = oxt_internal_FindOxt(obj,oxt,&new_oxt);
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
	    exit(1);
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
    return this->Obj->TLowLevelDriver->GetAttrValue(this->LLParam, attrname, datatype, val, oxt);
    }


/*** oxtOpenQuery -- passthru for now.
 ***/
void*
oxtOpenQuery(void* this_v, char* query, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    pObjTrxQuery qy;

    	/** Allocate the query **/
	qy = (pObjTrxQuery)nmMalloc(sizeof(ObjTrxQuery));
	if (!qy) return NULL;

	/** Call the low level driver. **/
	qy->Obj = this->Obj;
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
    return qy->Obj->TLowLevelDriver->QueryDelete(qy->LLParam, oxt);
    }


/*** oxtQueryFetch -- passthru for now.
 ***/
void*
oxtQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pObjTrxPtr subobj;
    pObjTrxQuery qy = (pObjTrxQuery)qy_v;

    	/** Allocate the subobject **/
	subobj = (pObjTrxPtr)nmMalloc(sizeof(ObjTrxPtr));
	if (!subobj) return NULL;
	
	/** Call the lowlevel driver **/
	subobj->LLParam = qy->Obj->TLowLevelDriver->QueryFetch(qy->LLParam, obj, mode, oxt);
	if (!(subobj->LLParam)) 
	    {
	    nmFree(subobj,sizeof(ObjTrxPtr));
	    return NULL;
	    }
	subobj->Obj = obj;
	subobj->Trx = *oxt;

    return subobj;
    }


/*** oxtQueryClose -- passthru for now.
 ***/
int
oxtQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pObjTrxQuery qy = (pObjTrxQuery)qy_v;

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
    return this->Obj->TLowLevelDriver->Write(this->LLParam,buffer,cnt,offset,flags,(this->Trx)?&(this->Trx):oxt);
    }


/*** oxtRead -- passthru to lowlevel for now.  Read and Write will
 *** not be passthru later when we implement the whole trans layer.
 ***/
int
oxtRead(void* this_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->Read(this->LLParam,buffer,maxcnt,offset,flags,oxt);
    }


/*** oxtGetAttrType -- passthru to lowlevel.
 ***/
int
oxtGetAttrType(void* this_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->GetAttrType(this->LLParam,attrname,oxt);
    }


/*** oxtAddAttr -- passthru to lowlevel for now.  If we ever add more
 *** transaction functionality, this may have to change.
 ***/
int
oxtAddAttr(void* this_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->AddAttr(this->LLParam, oxt);
    }


/*** oxtGetFirstAttr - passthru to lowlevel.
 ***/
char*
oxtGetFirstAttr(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->GetFirstAttr(this->LLParam,oxt);
    }


/*** oxtGetNextAttr - passthru to lowlevel.
 ***/
char*
oxtGetNextAttr(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->GetNextAttr(this->LLParam,oxt);
    }


/*** oxtGetFirstMethod - passthru to lowlevel.
 ***/
char*
oxtGetFirstMethod(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->GetFirstMethod(this->LLParam,oxt);
    }


/*** oxtGetNextMethod - passthru to lowlevel.
 ***/
char*
oxtGetNextMethod(void* this_v, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->GetNextMethod(this->LLParam,oxt);
    }


/*** oxtExecuteMethod - passthru to lowlevel.
 ***/
int
oxtExecuteMethod(void* this_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    return this->Obj->TLowLevelDriver->ExecuteMethod(this->LLParam,methodname,param,oxt);
    }


/*** oxtPresentationHints - passthru.
 ***/
pObjPresentationHints
oxtPresentationHints(void* this_v, char* attrname, pObjTrxTree* oxt)
    {
    pObjTrxPtr this = (pObjTrxPtr)(this_v);
    if (!(this->Obj->TLowLevelDriver->PresentationHints)) return NULL;
    return this->Obj->TLowLevelDriver->PresentationHints(this->LLParam,attrname,oxt);
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

	nmRegister(sizeof(ObjTrxPtr),"ObjTrxPtr");
	nmRegister(sizeof(ObjTrxQuery),"ObjTrxQuery");

	/** Register it now. **/
	objRegisterDriver(drv);

    return 0;
    }

