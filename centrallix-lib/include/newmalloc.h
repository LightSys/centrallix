#ifndef _NEWMALLOC_H
#define _NEWMALLOC_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	NewMalloc memory management module                      */
/* Author:	Greg Beeley (GRB)                                       */
/*									*/
/* Description:	Module that implements block caching and debugging	*/
/*		facilities for effective memory management.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: newmalloc.h,v 1.2 2003/03/04 06:28:22 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/newmalloc.h,v $

    $Log: newmalloc.h,v $
    Revision 1.2  2003/03/04 06:28:22  jorupp
     * added buffer overflow checking to newmalloc
    	-- define BUFFER_OVERFLOW_CHECKING in newmalloc.c to enable

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


void nmInitialize();
void nmSetErrFunction(int (*error_fn)());
void nmClear();
void nmCheckAll(); // checks for buffer overflows
void* nmMalloc(int size);
void nmFree(void* ptr,int size);
void nmStats();
void nmRegister(int size,char* name);
void nmDebug();
void nmDeltas();
void* nmSysMalloc(int size);
void nmSysFree(void* ptr);
void* nmSysRealloc(void* ptr, int newsize);
char* nmSysStrdup(char* ptr);

void nmEnableTagging();
void nmRegisterTagID(int tag_id, char* name);
void nmSetTag(void* ptr, int tag_id, void* tag);
void* nmGetTag(void* ptr, int tag_id);
void nmRemoveTag(void* ptr, int tag_id);

#endif
