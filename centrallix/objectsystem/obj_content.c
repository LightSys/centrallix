#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/magic.h"

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
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_content.c: contains implementation of the 	*/
/*		content-access (read/write) object methods.		*/
/************************************************************************/



/*** Reads content from an object, like fdRead().  Without OBJ_U_SEEK, reading
 *** continues where the previous read ended.  Use objInfo() to check whether
 *** an object has content and supports seeking.
 ***
 *** @param this The object to read from.
 *** @param buffer The buffer to read into.  The data is not null-terminated.
 *** @param maxcnt The maximum number of bytes to read.
 *** @param offset The byte offset to read from, if flags has OBJ_U_SEEK.
 *** @param flags 0, or OBJ_U_SEEK to read from offset.
 *** @returns The number of bytes read, 0 at the end of the content,
 *** 	or -1 if an error occurs.
 ***/
int 
objRead(pObject this, char* buffer, int maxcnt, int offset, int flags)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"OSML","Could not objRead(): resource exhaustion occurred");
	return -1;
	}

    if (maxcnt < 0 || offset < 0)
	{
	mssError(1,"OSML","Parameter error calling objRead()");
	return -1;
	}
    return this->Driver->Read(this->Data, buffer, maxcnt, offset, flags, &(this->Session->Trx));
    }


/*** Writes content to an object, like fdWrite().  Without OBJ_U_SEEK, writing
 *** continues where the previous write ended.
 ***
 *** @param this The object to write to, opened for writing.
 *** @param buffer The data to write.
 *** @param cnt The number of bytes to write.
 *** @param offset The byte offset to write at, if flags has OBJ_U_SEEK.
 *** @param flags 0, or OBJ_U_SEEK to write at offset, optionally with
 *** 	OBJ_U_TRUNCATE to truncate the content at offset.
 *** @returns The number of bytes written, or -1 if an error occurs.
 ***/
int 
objWrite(pObject this, char* buffer, int cnt, int offset, int flags)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"OSML","Could not objWrite(): resource exhaustion occurred");
	return -1;
	}

    if (cnt < 0 || offset < 0)
	{
	mssError(1,"OSML","Parameter error calling objWrite()");
	return -1;
	}
    return this->Driver->Write(this->Data, buffer, cnt, offset, flags, &(this->Session->Trx));
    }
