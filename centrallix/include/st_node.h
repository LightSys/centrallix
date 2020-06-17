#ifndef _ST_NODE_H
#define _ST_NODE_H

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
/* Module: 	st_node.c,st_node.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 17, 1998 					*/
/* Description:	Handles read/write/create of structure file nodes that	*/
/*		are currently used by the structure file driver and the	*/
/*		querytree object driver for config information.		*/
/************************************************************************/


#include "stparse.h"
#include "cxlib/xarray.h"
#include "cxlib/datatypes.h"
#include "obj.h"
#include <sys/stat.h>


/*** Structure for storing top-level node accesses ***/
typedef struct
    {
    int		Magic;
    char	NodePath[256];
    char	OpenType[64];
    int		Status;
    pStructInf	Data;
    XArray	Opens;
    DateTime	LastModification;
    int		OpenCnt;
    int		RevisionCnt;
    }
    SnNode, *pSnNode;

#define SN_NS_CLEAN		0
#define SN_NS_DIRTY		1


/** Basic node access functions. **/
pSnNode snReadNode(pObject obj);
int snWriteNode(pObject obj, pSnNode node);
pSnNode snNewNode(pObject obj, char* content_type);
int snDelete(pSnNode node);
int snGetSerial(pSnNode node);
pDateTime snGetLastModification(pSnNode node);

/** Special node parameter functions. **/
int snSetParamString(pSnNode node, pObject obj, char* paramname, char* default_val);
int snSetParamInteger(pSnNode node, pObject obj, char* paramname, int default_val);

#endif /* _ST_NODE_H */
