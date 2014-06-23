#ifndef _PTOD_H
#define _PTOD_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2004 LightSys Technology Services, Inc.		*/
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
/* Module:	ptod.h, ptod.c                                          */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 1, 2004                                             */
/*									*/
/* Description:	This module defines the PTOD atom data type as well as	*/
/*		routines for managing this very basic component within	*/
/*		centrallix.						*/
/************************************************************************/


#include "cxlib/datatypes.h"

/** typed POD structure, PTOD **/
typedef struct _TPOD
    {
    ObjData		Data;
    unsigned char	DataType;
    unsigned char	Flags;
    unsigned short	LinkCnt;
    int			AttachedLen;
    }
    TObjData, *pTObjData;

#define DATA_TF_NULL		1	/* data value is NULL */
#define DATA_TF_UNASSURED	2	/* data is unassured (replication) */
#define DATA_TF_UNMANAGED	4	/* pointed-to data not managed by ptod*/
#define DATA_TF_ATTACHED	8	/* pointed-to data directly alloc'd */
   
#define PTOD(x)	((pTObjData)(x))

/*** PTOD manipulation functions ***/
pTObjData ptodAllocate();
int ptodFree(pTObjData ptod);
pTObjData ptodLink(pTObjData ptod);
pTObjData ptodCreateInt(int data);
pTObjData ptodCreateString(char* data, int flags);
pTObjData ptodCreate(void* data, int datatype);
int ptodCopy(pTObjData src, pTObjData dst);
pTObjData ptodDuplicate(pTObjData ptod, int flags);
int ptodTypeOf(pTObjData ptod);
int ptodIsSet(pTObjData ptod, int flag);
int ptodMakeIndependent(pTObjData ptod);

int ptodPrint(pTObjData ptod);

#endif /* not defined _PTOD_H */

