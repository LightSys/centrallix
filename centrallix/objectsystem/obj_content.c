#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/magic.h"

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
/*		--> obj_content.c: contains implementation of the 	*/
/*		content-access (read/write) object methods.		*/
/************************************************************************/



/*** objRead - read content from an object at a particular optional seek
 *** offset.  Very similar to MTask's fdRead().
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


/*** objWrite - write content to an object at a particular optional seek
 *** offset.  Also very similar to MTask's fdWrite().
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


/*** objTransfer - copy content from source to destination, either using
 *** objRead/objWrite or fdRead/fdWrite, and using a maximum byte count.  If
 *** max_xfer is set to -1, then the transfer is unlimited in size and ends once
 *** the end of src is found.
 ***/
int
objTransfer(void* src, int (*src_read)(), void* dst, int (*dst_write)(), int max_xfer)
    {
    char xfer_buf[256];
    int rcnt, wcnt, target_rcnt, actual_wcnt;
    int xfer_cnt = 0;

	/** Read from source until end or max_xfer reached **/
	while(max_xfer < 0 || xfer_cnt < max_xfer)
	    {
	    /** Constrain the read size **/
	    target_rcnt = sizeof(xfer_buf);
	    if (max_xfer >= 0 && target_rcnt > (max_xfer - xfer_cnt))
		target_rcnt = max_xfer - xfer_cnt;

	    /** Do the read **/
	    rcnt = src_read(src, xfer_buf, target_rcnt, 0, 0);
	    if (rcnt < 0)
		return -1;
	    else if (rcnt == 0)
		break;

	    /** Loop, attempting to write all that was read in **/
	    actual_wcnt = 0;
	    while (actual_wcnt < rcnt)
		{
		/** Attempt a write **/
		wcnt = dst_write(dst, xfer_buf + actual_wcnt, rcnt - actual_wcnt, 0, 0);
		if (wcnt <= 0)
		    return -1;
		actual_wcnt += wcnt;
		}
	    xfer_cnt += actual_wcnt;
	    }

    return xfer_cnt;
    }


/*** objSeek -- seek to a location in an object.  This actually just calls
 *** objRead(), but this is a separate API call just so that the calling
 *** code has greater clarity.
 ***/
int
objSeek(pObject this, int offset)
    {
    char buffer[1];
    return objRead(this, buffer, 0, offset, OBJ_U_SEEK);
    }


