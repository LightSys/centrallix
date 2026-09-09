#ifndef _OBJ_PRIVATE_H
#define _OBJ_PRIVATE_H

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
/* Module:	obj_private.h, obj_*.c					*/
/* Author:	Israel Fuller						*/
/* Date:	September 2nd, 2026					*/
/* 									*/
/* Description:	Internal declarations shared between the ObjectSystem	*/
/* 		sources.  These are not a part of the public		*/
/* 		ObjectSystem interface.					*/
/************************************************************************/


#include "obj.h"


/** objectsystem directory entry caching **/
extern int obj_internal_DiscardDC(pXHashQueue hq, pXHQElement xe, int locked);

/** objectsystem object and path management **/
int obj_internal_FreePath(pPathname this);
int obj_internal_FreePathStruct(pPathname this);
pPathname obj_internal_NormalizePath(char* cwd, char* name);
int obj_internal_AddChildTree(pObjTrxTree parent_oxt, pObjTrxTree child_oxt);
pObject obj_internal_AllocObj();
int obj_internal_FreeObj(pObject);
int obj_internal_TrxLog(pObject this, char* op, char* fmt, ...);

/** objectsystem transaction functions **/
int obj_internal_FreeTree(pObjTrxTree oxt);
pObjTrxTree obj_internal_AllocTree();
pObjTrxTree obj_internal_FindTree(pObjTrxTree oxt, char* path);
int obj_internal_SetTreeAttr(pObjTrxTree oxt, int type, pObjData val);
pObjTrxTree obj_internal_FindAttrOxt(pObjTrxTree oxt, char* attrname);

/** objectsystem path manipulation **/
char* obj_internal_PathPart(pPathname path, int start_element, int length);
int obj_internal_PathPrefixCnt(pPathname full_path, pPathname prefix);
int obj_internal_CopyPath(pPathname dest, pPathname src);
int obj_internal_AddToPath(pPathname path, char* new_element);
int obj_internal_RenamePath(pPathname path, int element_id, char* new_element);
void obj_internal_OpenCtlToString(pPathname pathinfo, int pathstart, int pathend, pXString str);
int obj_internal_PathToText(pPathname pathinfo, int pathend, pXString str);

/** objectsystem replication services - open object notification (Rn) system **/
int obj_internal_RnDelete(pObjReqNotifyItem item);
int obj_internal_RnNotifyAttrib(pObject this, char* attrname, pTObjData newvalue, int send_this);

#endif /*_OBJ_PRIVATE_H*/
