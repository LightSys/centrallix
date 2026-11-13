#include <stdio.h>
#include <string.h>
#include "obj.h"
#include "application.h"
#include "stparse.h"
#include "cxss/cxss.h"
#include "cxlib/xarray.h"
#include "cxlib/mtask.h"
#include "cxlib/xhandle.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2026 LightSys Technology Services, Inc.		*/
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
/* Creation:	November 13, 2026					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.	*/
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_datascope.c: data scopes, which hold the named	*/
/*		collections and declared objects that are visible to	*/
/*		queries, and the tokens used to pass them through	*/
/*		open-ctl parameters on object paths.			*/
/************************************************************************/

#define OBJ_DS_TOKEN_LEN	(32)	/* hex characters in a token */

struct
    {
    pCxssKeystreamState	Keystream;
    }
    OBJDS;


/*** obj_internal_FreeDataItem() - release a data item, and the temp
 *** collection or object data it holds if the item owns them.
 ***/
static int
obj_internal_FreeDataItem(pObjDataItem item)
    {

	if (item->Flags & OBJ_DI_F_OWNED)
	    {
	    if (item->TempObj != XHN_INVALID_HANDLE)
		objDeleteTempObject(item->TempObj);
	    if (item->Data)
		stFreeInf((pStructInf)item->Data);
	    }
	if (item->Path)
	    nmSysFree(item->Path);
	nmFree(item, sizeof(ObjDataItem));

    return 0;
    }


/*** obj_internal_AllocDataItem() - allocate and fill in a data item.
 ***/
static pObjDataItem
obj_internal_AllocDataItem(char* name, int type, int flags, handle_t tempobj, char* path, void* data)
    {
    pObjDataItem item;

	if (strlen(name) >= sizeof(item->Name))
	    {
	    mssError(1, "OSML", "Data item name '%s' is too long", name);
	    return NULL;
	    }
	item = (pObjDataItem)nmMalloc(sizeof(ObjDataItem));
	if (!item)
	    return NULL;
	memset(item, 0, sizeof(ObjDataItem));
	strtcpy(item->Name, name, sizeof(item->Name));
	item->Type = type;
	item->Flags = flags;
	item->TempObj = tempobj;
	item->Data = data;
	if (path)
	    {
	    item->Path = nmSysStrdup(path);
	    if (!item->Path)
		{
		nmFree(item, sizeof(ObjDataItem));
		return NULL;
		}
	    }

    return item;
    }


/*** objScopeCreate() - create a new data scope.  Lookups that follow
 *** parents search the given parent scope (if any) after this one, and
 *** then the application scope.  The new scope has a link count of 1.
 ***/
pObjDataScope
objScopeCreate(pObjSession session, pObjDataScope parent)
    {
    pObjDataScope scope;

	scope = (pObjDataScope)nmMalloc(sizeof(ObjDataScope));
	if (!scope)
	    return NULL;
	memset(scope, 0, sizeof(ObjDataScope));
	scope->LinkCnt = 1;
	scope->Session = session;
	if (xaInit(&scope->Items, 8) < 0)
	    {
	    nmFree(scope, sizeof(ObjDataScope));
	    return NULL;
	    }
	if (xaInit(&scope->Tokens, 8) < 0)
	    {
	    xaDeInit(&scope->Items);
	    nmFree(scope, sizeof(ObjDataScope));
	    return NULL;
	    }
	scope->Parent = parent?objScopeLink(parent):NULL;

    return scope;
    }


/*** objScopeLink() - add a link to a data scope.
 ***/
pObjDataScope
objScopeLink(pObjDataScope scope)
    {

	scope->LinkCnt++;

    return scope;
    }


/*** objScopeRelease() - release a link to a data scope.  The last release
 *** frees the scope's items and tokens and releases its parent.
 ***/
int
objScopeRelease(pObjDataScope scope)
    {
    int i;
    pObjDataToken tok;

	/** Releasing the parent recurses up the scope chain **/
	if (thExcessiveRecursion())
	    {
	    mssError(1, "OSML", "Could not release data scope: resource exhaustion occurred");
	    return -1;
	    }

	if (--scope->LinkCnt > 0)
	    return 0;

	for(i=0; i<scope->Tokens.nItems; i++)
	    {
	    tok = (pObjDataToken)scope->Tokens.Items[i];
	    obj_internal_FreeDataItem(tok->Item);
	    nmFree(tok, sizeof(ObjDataToken));
	    }
	xaDeInit(&scope->Tokens);
	for(i=0; i<scope->Items.nItems; i++)
	    obj_internal_FreeDataItem((pObjDataItem)scope->Items.Items[i]);
	xaDeInit(&scope->Items);
	if (scope->Parent)
	    objScopeRelease(scope->Parent);
	nmFree(scope, sizeof(ObjDataScope));

    return 0;
    }


/*** objScopeGetAppScope() - get the root data scope for the current
 *** application, creating it if needed.  Returns NULL when there is no
 *** application context.
 ***/
pObjDataScope
objScopeGetAppScope()
    {
    pObjDataScope scope;

	scope = (pObjDataScope)appLookupAppData("OSML:appscope");
	if (!scope)
	    {
	    scope = objScopeCreate(NULL, NULL);
	    if (!scope)
		return NULL;
	    scope->Flags |= OBJ_DS_F_APPSCOPE;
	    if (appRegisterAppData("OSML:appscope", (void*)scope, objScopeRelease) < 0)
		{
		objScopeRelease(scope);
		return NULL;
		}
	    }

    return scope;
    }


/*** objScopeLookupLocal() - find an item of the given type in this scope
 *** only, without searching parents or the application scope.
 ***/
pObjDataItem
objScopeLookupLocal(pObjDataScope scope, char* name, int type)
    {
    int i;
    pObjDataItem item;

	if (!scope)
	    return NULL;
	for(i=0; i<scope->Items.nItems; i++)
	    {
	    item = (pObjDataItem)scope->Items.Items[i];
	    if (item->Type == type && !strcmp(item->Name, name))
		return item;
	    }

    return NULL;
    }


/*** objScopeLookup() - find an item of the given type.  With follow_parents
 *** set, searches this scope, then each parent, then the application scope.
 *** A NULL scope with follow_parents set searches only the application scope.
 ***/
pObjDataItem
objScopeLookup(pObjDataScope scope, char* name, int type, int follow_parents)
    {
    pObjDataItem item;
    pObjDataScope app_scope;

	for(; scope; scope = follow_parents?scope->Parent:NULL)
	    {
	    item = objScopeLookupLocal(scope, name, type);
	    if (item)
		return item;
	    }

	if (follow_parents)
	    {
	    app_scope = objScopeGetAppScope();
	    if (app_scope)
		return objScopeLookupLocal(app_scope, name, type);
	    }

    return NULL;
    }


/*** objScopeAdd() - add an item to a scope.  If flags includes
 *** OBJ_DI_F_OWNED, the scope takes ownership of tempobj and data, but only
 *** on success; on failure the caller still owns them.  The path, if given,
 *** is copied.  Fails if the scope already has an item of that name and type.
 ***/
pObjDataItem
objScopeAdd(pObjDataScope scope, char* name, int type, int flags, handle_t tempobj, char* path, void* data)
    {
    pObjDataItem item;

	if (objScopeLookupLocal(scope, name, type))
	    {
	    mssError(1, "OSML", "'%s' already exists in this scope", name);
	    return NULL;
	    }
	item = obj_internal_AllocDataItem(name, type, flags, tempobj, path, data);
	if (!item)
	    return NULL;
	if (xaAddItem(&scope->Items, (void*)item) < 0)
	    {
	    item->Flags &= ~OBJ_DI_F_OWNED;
	    obj_internal_FreeDataItem(item);
	    return NULL;
	    }

    return item;
    }


/*** objScopePublish() - make a token that refers to a copy of the given
 *** item.  The token resolves only in this scope, from this scope's
 *** session.  The token buffer must hold at least 33 characters.
 ***/
int
objScopePublish(pObjDataScope scope, pObjDataItem item, void* owner, char* token, int token_len)
    {
    pObjDataToken tok;

	if (token_len < OBJ_DS_TOKEN_LEN + 1 || (scope->Flags & OBJ_DS_F_APPSCOPE))
	    return -1;

	if (!OBJDS.Keystream)
	    {
	    OBJDS.Keystream = cxssKeystreamNew(NULL, 0);
	    if (!OBJDS.Keystream)
		return -1;
	    }

	tok = (pObjDataToken)nmMalloc(sizeof(ObjDataToken));
	if (!tok)
	    return -1;
	memset(tok, 0, sizeof(ObjDataToken));
	if (cxssKeystreamGenerateHex(OBJDS.Keystream, tok->Token, OBJ_DS_TOKEN_LEN) < 0)
	    {
	    nmFree(tok, sizeof(ObjDataToken));
	    return -1;
	    }
	tok->Owner = owner;
	tok->Item = obj_internal_AllocDataItem(item->Name, item->Type, item->Flags & ~OBJ_DI_F_OWNED, item->TempObj, item->Path, item->Data);
	if (!tok->Item)
	    {
	    nmFree(tok, sizeof(ObjDataToken));
	    return -1;
	    }
	if (xaAddItem(&scope->Tokens, (void*)tok) < 0)
	    {
	    obj_internal_FreeDataItem(tok->Item);
	    nmFree(tok, sizeof(ObjDataToken));
	    return -1;
	    }
	strtcpy(token, tok->Token, token_len);

    return 0;
    }


/*** objScopeRevoke() - remove all tokens that the given owner published.
 ***/
int
objScopeRevoke(pObjDataScope scope, void* owner)
    {
    int i;
    pObjDataToken tok;

	for(i=scope->Tokens.nItems-1; i>=0; i--)
	    {
	    tok = (pObjDataToken)scope->Tokens.Items[i];
	    if (tok->Owner == owner)
		{
		xaRemoveItem(&scope->Tokens, i);
		obj_internal_FreeDataItem(tok->Item);
		nmFree(tok, sizeof(ObjDataToken));
		}
	    }

    return 0;
    }


/*** objScopeResolveToken() - find the item a token refers to.  Only this
 *** scope's tokens are searched, and the session must match the scope's.
 ***/
pObjDataItem
objScopeResolveToken(pObjDataScope scope, pObjSession session, char* token)
    {
    int i;
    pObjDataToken tok;

	if (!scope || !token || scope->Session != session || strlen(token) != OBJ_DS_TOKEN_LEN)
	    return NULL;
	for(i=0; i<scope->Tokens.nItems; i++)
	    {
	    tok = (pObjDataToken)scope->Tokens.Items[i];
	    if (!strcmp(tok->Token, token))
		return tok->Item;
	    }

    return NULL;
    }
