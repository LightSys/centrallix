#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "datatypes.h"
#include "ptod.h"

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
/* Module:	ptod.h,ptod.c                                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 1, 2004                                             */
/*									*/
/* Description:	This module defines the PTOD atom data type as well as	*/
/*		routines for managing this very basic component within	*/
/*		centrallix.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: ptod.c,v 1.3 2004/08/27 01:28:33 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/ptod.c,v $

    $Log: ptod.c,v $
    Revision 1.3  2004/08/27 01:28:33  jorupp
     * cleaning up some compile warnings

    Revision 1.2  2004/06/12 04:02:29  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.1  2004/05/04 18:19:48  gbeeley
    - new definition location for PTOD type.

 **END-CVSDATA***********************************************************/


/*** ptodAllocate() - create a new, uninitialized PTOD object without any
 *** storage for pointed-to data (like money, datetime, etc.).
 ***/
pTObjData
ptodAllocate()
    {
    pTObjData ptod;

	/** Allocate the memory **/
	ptod = (pTObjData)nmMalloc(sizeof(TObjData));

	/** Init data type, etc. **/
	ptod->Data.Generic = NULL;
	ptod->DataType = DATA_T_UNAVAILABLE;
	ptod->Flags = DATA_TF_NULL;
	ptod->LinkCnt = 1;
	ptod->AttachedLen = 0;

    return ptod;
    }


/*** ptodFree() - unlink from the PTOD object, and if the link count goes
 *** to zero, free the memory.  If it was marked as being managed, also
 *** free memory for the pointed-to data itself.
 ***/
int
ptodFree(pTObjData ptod)
    {

	/** Link count to 0? **/
	if ((ptod->LinkCnt--) != 0) return 0;

	/** Managed? free data memory as well if so **/
	if (!(ptod->Flags & DATA_TF_UNMANAGED) && !(ptod->Flags & DATA_TF_ATTACHED))
	    {
	    switch(ptod->DataType)
		{
		case DATA_T_STRING:
		    if (ptod->Data.String) nmSysFree(ptod->Data.String);
		    break;
		case DATA_T_BINARY:
		    if (ptod->Data.Binary.Data) nmSysFree(ptod->Data.Binary.Data);
		    break;
		case DATA_T_MONEY:
		    if (ptod->Data.Money) nmFree(ptod->Data.Money, sizeof(MoneyType));
		    break;
		case DATA_T_DATETIME:
		    if (ptod->Data.DateTime) nmFree(ptod->Data.DateTime, sizeof(DateTime));
		    break;
		case DATA_T_STRINGVEC:
		    /** FIXME - how do we want to handle managed intvec/stringvec? **/
		    if (ptod->Data.StringVec) nmFree(ptod->Data.StringVec, sizeof(StringVec));
		    break;
		case DATA_T_INTVEC:
		    if (ptod->Data.IntVec) nmFree(ptod->Data.IntVec, sizeof(IntVec));
		    break;
		default:
		    break;
		}
	    }

	/** Ok, now free the ptod itself **/
	if (ptod->Flags & DATA_TF_ATTACHED)
	    nmSysFree(ptod);
	else
	    nmFree(ptod, sizeof(TObjData));

    return 0;
    }


/*** ptodLink() - link to a ptod object so that it doesn't get released until
 *** all references are removed
 ***/
pTObjData
ptodLink(pTObjData ptod)
    {

	/** link **/
	ptod->LinkCnt++;

    return ptod;
    }


/*** ptodCopy() - copy a ptod object.  If the original ptod is unmanaged, the
 *** copy also will be unmanaged and the data pointer(s) will point to the same
 *** place as the original!  Otherwise, a copy is made.  If the data is
 *** attached, then that data is copied.  An error occurs if there is not enough
 *** room to copy data to an 'attached' ptod.
 ***
 *** note that the managed/attached flags ARE HONORED on the destination ptod.
 ***/
int
ptodCopy(pTObjData src, pTObjData dst)
    {

	if (src->Flags & DATA_TF_UNMANAGED)
	    {
	    /** unmanaged/unattached copy? **/
	    }
	else if (src->Flags & DATA_TF_ATTACHED)
	    {
	    /** Attached copy? **/
	    }
	else
	    {
	    /** Managed copy **/
	    }

    return 0;
    }


/*** ptodDuplicate() - duplicate a ptod object by allocating a new one.  Flags
 *** indicate UNMANAGED/ATTACHED status.
 ***/
pTObjData
ptodDuplicate(pTObjData ptod, int flags)
    {
	return NULL;
    }


/*** ptodPrint() - do a debugging print of the ptod data value to standard
 *** output.
 ***/
int
ptodPrint(pTObjData ptod)
    {
    char* type_names[] = { "(unknown)", "integer", "string", "double", "datetime", "intvec", "stringvec", "money", "array", "code", "binary" };
    int t;

	/** type **/
	t = ptod->DataType;
	if (t < 0 || t > sizeof(type_names)/sizeof(char*)) t = 0;

	printf("PTOD: type %s, ", type_names[t]);
	if (ptod->Flags & DATA_TF_UNASSURED) printf("unassured, ");
	if (ptod->Flags & DATA_TF_UNMANAGED) printf("unmanaged, ");
	if (ptod->Flags & DATA_TF_ATTACHED) printf("attached, ");
	if (ptod->Flags & DATA_TF_NULL) 
	    {
	    printf("null\n");
	    }
	else
	    {
	    if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE)
		printf("%s\n", (char*)objDataToStringTmp(t, (void*)&(ptod->Data), 0));
	    else
		printf("%s\n", (char*)objDataToStringTmp(t, (void*)(ptod->Data.Generic), 0));
	    }

    return 0;
    }

