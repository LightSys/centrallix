#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"
#include <errno.h>
#include <time.h>
#include <db.h>
#include "centrallix.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_berk.c     					*/
/* Author:	David O'Neill aka "affert"				*/
/* Creation:	July 1, 2003						*/
/* Description:	A Berkeley Database objectsystem driver.  This is a	*/
/*		raw driver, allowing very general access.		*/
/************************************************************************/


/**CVSDATA***************************************************************

    $Id: objdrv_berk.c,v 1.3 2004/06/11 21:06:57 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_berk.c,v $

    $Log: objdrv_berk.c,v $
    Revision 1.3  2004/06/11 21:06:57  mmcgill
    Did some code tree scrubbing.

    Changed xxxGetAttrValue(), xxxSetAttrValue(), xxxAddAttr(), and
    xxxExecuteMethod() to use pObjData as the type for val (or param in
    the case of xxxExecuteMethod) instead of void* for the audio, BerkeleyDB,
    GZip, HTTP, MBox, MIME, and Shell drivers, and found/fixed a 2-byte buffer
    overflow in objdrv_shell.c (line 1046).

    Also, the Berkeley API changed in v4 in a few spots, so objdrv_berk.c is
    broken as of right now.

    It should be noted that I haven't actually built the audio or Berkeley
    drivers, so I *could* have messed up, but they look ok. The others
    compiled, and passed a cursory testing.

    Revision 1.2  2003/08/13 14:45:55  affert
    Added a new attribute ('transdata') to allow data to be entered in hex
    translated form.  This is a temporary solution to the problem of allowing
    null bytes in a string attribute.
    Also fixed a few small bugs and updated some comments.

    Revision 1.1  2003/08/05 16:45:12  affert
    Initial Berkeley DB support.


 **END-CVSDATA***********************************************************/


#define	    BERK_TYPE_FILE	0
#define	    BERK_TYPE_DBT	1

#define	    BERK_AM_UNKNOWN	DB_UNKNOWN
#define	    BERK_AM_BTREE	DB_BTREE
#define	    BERK_AM_HASH	DB_HASH
#define	    BERK_AM_QUEUE	DB_QUEUE
#define	    BERK_AM_RECNO	DB_RECNO

#define	    BERK_NODESTATE_NONE	    0
#define	    BERK_NODESTATE_KEY	    1
#define	    BERK_NODESTATE_CURSOR   2

#define	    USE_ENV 0
#define	    DEBUGGER 0

/*** EnvNode holds an open environment and databse       ***/
/*** Both the objects and the queries can point to this  ***/
/*** It stays until the last thing pointing to it closes ***/
typedef struct EN
    {
    char	    Filename[OBJSYS_MAX_PATH];
#if USE_ENV
    DB_ENV*	    pDbEnv;
#endif
    DB*		    TheDatabase;
    int		    TheDatabaseIni;
    struct EN*	    pNext;
    int		    nNumOpen;
    int		    nSysOpen;
    pObject	    PrevObj;
    }
    EnvNode, *pEnvNode;

/*** BerkNode keeps track of a query that has been opened   ***/
/*** They are held in a linked list as well as being passed ***/
/*** into the Query functions.				    ***/
typedef struct BN
    {
    struct BN*	    pNext;	/* Used for finding and closing open cursors on updates */
    int		    fState;	/* fOpen is set to BERK_NODESTATE_name  */
    DBT*	    pKey;	/* This needs to keep it's own memory. It is set if the actual cursor must be closed */
    DBC*	    pCursor;	
    pEnvNode	    pMyEnvNode;
    int		    ObjSubPtr;	/* keep track of how deep we are into the path */
    }
    BerkNode, *pBerkNode;
    
/*** Structure used by this driver internally ***/
typedef struct
    {
    Pathname        Pathname;
    pObject         obj;
    char            Filename[OBJSYS_MAX_PATH];
    char	    ObjName[OBJSYS_MAX_PATH];
    DBT*	    pKey;	    /* NULL if opening a file */
    DBT*	    pData;	    /* NULL if opening file */
    pEnvNode	    pMyEnvNode;
    int             Type;	    /* BERK_TYPE_FILE | BERK_TYPE_DBT */
    int		    nOffset;	    /* Keep track of where to start on the next call to read or write */
    int             AccessMethod;   /* BERK_AM_name */
    int		    nAttrCursor;    /* to keep track of the stepping through the attributes */
    int		    mask;	    /* chmod thingy */
    char*	    pStrAttr;	    /* allocated to append NULL to end of string we're returning as an attribute */
    char	    Annotation[256];
    char*	    newKey;	    /* used for setting the key attribute */
    int		    AutoName;	    /* flag that says autoname has been asked for, but it hasn't been set yet */
    char*	    newData;	    /* used as a mask to keep track what of the data in inf->pNewData->data is new */
				    /* NOTE about newData: there is always an extra byte at the end */
				    /* If it is 1, then truncate the data to that point.	    */
    DBT*	    pNewData;	    /* If it is 0, then include the rest of the data in the database*/
    }
    BerkData, *pBerkData;   
 
/*** GLOBALS ***/
struct
    {
    pBerkNode	pBerkNodeList;	/* Used for finding and closing open cursors on updates */
    pEnvNode	pEnvList;
    pEnvNode	*pFDTable;	/* YES this is supposed to be double pointers */
    int*	pOffsetTable;	/* each FD has its own offset */
    int		TableSize;
    }
    BERK_INF;

int
berkInternalKeyToHex(char *dest, void* src, int dL, int sL)
    {
    int	    i;
    char    cTemp;
    if(dL < 2 * sL)
	{
	mssError(0, "BERK", "KeyToHex: Key is too long to fit into destination array");
	return -1;
	}
    for(i=0;i<sL;i++)
	{
	/* low bits */
	cTemp = ((unsigned char*)(src))[i];
	cTemp = cTemp & 0x0f;
	if(cTemp > 0x09)    dest[i*2+1]=cTemp + 'a' - 0x0a;
	else                dest[i*2+1]=cTemp + '0';
	/* high bits */
	cTemp = ((unsigned char*)(src))[i];
	cTemp = cTemp >> 4;
	cTemp = cTemp & 0x0f;
	if(cTemp > 0x09)    dest[i*2]=cTemp + 'a' - 0x0a;
	else                dest[i*2]=cTemp + '0';
	}
    return 0;
    }

/* translates a hex string into an array of bytes */
/* src must be NULL terminated and have an even number of bytes (so each byte gets a pair */
int
berkInternalHexToKey(void* dest, char* src, int dL, int sL)
    {
    int	    i;
    char    c[2];
    int	    value;
    c[1] = 0;
    if(2 * dL < sL)
	{
	mssError(0, "BERK", "HexToKey: hex value is too long to fit into destination");
	return -1;
	}
    if(sL != ((int)(sL/2))*2)
	{
	mssError(0, "BERK", "HexToKey: hex value must have an even length");
	return -1;
	}
    /* go until a NULL is found or we're past sL */
    for(i=0;src[i]&&i<sL;i+=2)
	{
	c[0]  = src[i];
	value = strtol(c, NULL, 16) << 4;
	c[0]  = src[i+1];
	value+= strtol(c, NULL, 16);
	((char*)dest)[((int)(i/2))]=(char)value;
	}
    if(i > sL)
	{
	mssError(0, "BERK", "HexToKey: hex value must be NULL termintated.  Results not defined");
	return -1;
	}
    return 0;
    }


/*** TranslateKey: translate the key from hex to bin.	***/
/*** also allocate the Key and Data DBTs.		***/
/*** Get the Key DBT completely ready to be used.	***/
int
berkInternalTranslateKey(pBerkData inf)
    {
    int	i, r;
    int value;
    char cur[2];

	cur[1]=0;
	
#if DEBUGGER
	mssError(0, "BERK", "  InternalTranslateKey called with key '%s'", inf->Pathname.Elements[inf->obj->SubPtr]);
#endif
	/* make sure there is an even number of chars */
	inf->pKey = nmMalloc(sizeof(DBT));
	if(!inf->pKey) return -1;
	inf->pData = nmMalloc(sizeof(DBT));
	if(!inf->pData) return -1;
	inf->pData->flags = DB_DBT_MALLOC;
	inf->pData->data = NULL;
	inf->pKey->data = NULL;
	/* if ObjName == "*", than they're asking for AutoName */
	if(inf->ObjName[0] == '*' && inf->ObjName[1] == '\0')
	    {
	    inf->AutoName = 1;
	    inf->pNewData = nmSysMalloc(sizeof(DBT));
	    if(!inf->pNewData) return -1;
	    memset(inf->pNewData, 0, sizeof(DBT));
	    return 0;
	    }
	i = strlen(inf->ObjName);
	if(((int)(i/2))*2!=i)
	    {
	    mssError(0, "BERK", "  InternalTranslateKey key had odd # of chars");
	    return -1;
	    }
	i = ((int)(i/2));
	/* allocate and initialize the key */
	inf->pKey->ulen=(i);
	inf->pKey->data=nmSysMalloc(inf->pKey->ulen);
	if(!inf->pKey->data) return -1;
	memset(inf->pKey->data, 0, i);
	inf->pKey->flags = DB_DBT_USERMEM;
	inf->pKey->size = inf->pKey->ulen;
	/* go through the key, and translate the hex into binary */
	r = berkInternalHexToKey(inf->pKey->data, inf->ObjName, inf->pKey->size, strlen(inf->ObjName));
#if DEBUGGER
	mssError(0, "BERK", "  InternalTranslateKey: post translataion key: '%s'", inf->pKey->data);
#endif
	return r;
    }

/*** Determines if this call is for a file or a DBT based on primary key ***/
int
berkInternalDetermineType(pBerkData inf)
    {
    int i;
    
	obj_internal_CopyPath(&(inf->Pathname), inf->obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1)=0;
	/* if SubPtr == nElements, then the filename is the last element */
	if(inf->Pathname.nElements == inf->obj->SubPtr)
	    {
	    /* This is a request to open the file */
	    inf->Type = BERK_TYPE_FILE;
	    inf->obj->SubCnt = 1;
	    /* copy the filename into the inf->Filename buffer for easy access */
	    for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1)=47;/* (char)47=='/' */
#if DEBUGGER
	    mssError(0, "BERK", "DetermineType: found file.  Filename = '%s' with inf->obj->SubPtr = '%d'", inf->Pathname.Pathbuf, inf->obj->SubPtr);
#endif
	    strcpy(inf->Filename, inf->Pathname.Pathbuf);
	    /* copy the object name into ObjName */
	    strncpy(inf->ObjName, (char*)(strrchr(inf->Pathname.Pathbuf, '/')+sizeof(char)), OBJSYS_MAX_PATH - 2);
	    /* make sure it is NULL terminated */
	    inf->ObjName[OBJSYS_MAX_PATH-1] = 0;
	    for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1)=0;
	    }
	else
	    {
	    inf->Type = BERK_TYPE_DBT;
	    inf->obj->SubCnt = 2;
	    for(i=1;i<inf->obj->SubPtr;i++) *(inf->Pathname.Elements[i]-1)=47;
	    strcpy(inf->Filename, inf->Pathname.Pathbuf);
	    strncpy(inf->ObjName, (char*)(strchr(inf->Pathname.Pathbuf, 0) + sizeof(char)), OBJSYS_MAX_PATH - 2);
	    inf->ObjName[OBJSYS_MAX_PATH - 1] = 0;
#if DEBUGGER
	    mssError(0, "BERK", "DetermineType: Filename = '%s' with inf->obj->SubPtr = '%d'", inf->Filename, inf->obj->SubPtr);
	    mssError(0, "BERK", "DetermineType: ObjName = '%s'", inf->ObjName);
#endif
	    for(i=1;i<inf->obj->SubPtr;i++) *(inf->Pathname.Elements[i]-1)=0;
	    /* get the key that is being asked for */
	    return berkInternalTranslateKey(inf);
	    }
    return 0;
    };

/*** Create the evnironment ***/
pEnvNode
berkInternalCreateEnv(pBerkData inf, pEnvNode pPrevNode)
    {
    pEnvNode pEnvNodeTemp;
#if DEBUGGER
    mssError(0, "BERK", " InternalCreateEnv called");
#endif
    /* if there's nothing in the list */
    if(BERK_INF.pEnvList==NULL)
	{
	/* create the first EnvNode */
	BERK_INF.pEnvList=nmMalloc(sizeof(EnvNode));
	if(!BERK_INF.pEnvList) return NULL;
	memset(BERK_INF.pEnvList, 0, sizeof(EnvNode));
	pEnvNodeTemp=BERK_INF.pEnvList;
	strcpy(pEnvNodeTemp->Filename, inf->Filename);
	}
    else
	{
	/* create a new EnvNode at the end of the list */
	pPrevNode->pNext=nmMalloc(sizeof(EnvNode));
	if(pPrevNode->pNext==NULL) return NULL;
	memset(pPrevNode->pNext, 0, sizeof(EnvNode));
	pEnvNodeTemp=pPrevNode->pNext;
	strcpy(pEnvNodeTemp->Filename, inf->Filename);
	}
    inf->pMyEnvNode = pEnvNodeTemp;
    inf->pMyEnvNode->PrevObj = inf->obj->Prev;
    objLinkTo(inf->pMyEnvNode->PrevObj);
    inf->pMyEnvNode->nNumOpen = 1;
    /* Create and open the environment */
#if USE_ENV 
    if(db_env_create(&(pEnvNodeTemp->pDbEnv), 0))
	{
	mssError(0, "BERK", "berkInternalCreateEnv db_env_create failed");
	return NULL;
	}
    /* tell it to not use memory mapping for files */
    pEnvNodeTemp->pDbEnv->set_flags(pEnvNodeTemp->pDbEnv, DB_NOMMAP, 1);
    inf->pMyEnvNode = pEnvNodeTemp;
    inf->pMyEnvNode->nSysOpen = 0;
    if(pEnvNodeTemp->pDbEnv->open(pEnvNodeTemp->pDbEnv, NULL, DB_INIT_CDB | DB_INIT_MPOOL, inf->mask))
	{
	mssError(0, "BERK", "berkInternalCreateEnv pDbEnv->open failed");
	return NULL;
	}
#endif    /* Success */
    return inf->pMyEnvNode;    
    }

/*** berkInternalDestroyEnv(pEnvNode p) ***/
void
berkInternalDestroyEnv(pEnvNode p)
    {
    pEnvNode pEnvNodeTemp;
    if(p)
	{
	if(p->TheDatabaseIni)
	    p->TheDatabase->close(p->TheDatabase, 0);
#if USE_ENV
	if(p->pDbEnv)
	    p->pDbEnv->close(p->pDbEnv, 0);
#endif
	/* unlink from the previous object */
	objClose(p->PrevObj);
	/* Remove this node from the list */
	pEnvNodeTemp = BERK_INF.pEnvList;
	if(pEnvNodeTemp == p)
	    {
	    BERK_INF.pEnvList = BERK_INF.pEnvList->pNext;
	    nmFree(p, sizeof(EnvNode));
	    }
	else
	    {
	    while(pEnvNodeTemp->pNext != NULL && pEnvNodeTemp->pNext!=p)
		pEnvNodeTemp = pEnvNodeTemp->pNext;
	    if(pEnvNodeTemp->pNext != NULL) /* if not, pEnvNodeTemp wasn't in the list */
		pEnvNodeTemp->pNext = pEnvNodeTemp->pNext->pNext;
	    nmFree(p, sizeof(EnvNode));
	    }
	}
    }

/*** this function is called to close the cursors that are in use.  ***/
/*** it is called by the write function, since a write can't happen ***/
/*** if there are currently any cursors open on the database	    ***/
int
berkInternalCloseCursors(pBerkData inf)
    {
    pBerkNode	pBerkNodeTemp;
    DBT		tempData;
    DBT		tempKey;
    
#if DEBUGGER
	mssError(0, "BERK", "InternalCloseCursors called");
#endif
	/* if there are no cursors: sucsess */
	if(BERK_INF.pBerkNodeList == NULL) return 0;
	
	pBerkNodeTemp  = BERK_INF.pBerkNodeList;
	tempData.flags = 0;
	tempKey.flags  = 0;
	/* Go through the list, closing each cursor that are open in this database */
	while(pBerkNodeTemp != NULL)
	    {
	    if(pBerkNodeTemp->pMyEnvNode == inf->pMyEnvNode && pBerkNodeTemp->fState == BERK_NODESTATE_CURSOR)
		{
		/* make sure there is memory for pKey */
		if(!pBerkNodeTemp->pKey) 
		    {
		    pBerkNodeTemp->pKey = nmMalloc(sizeof(DBT));
		    if(!pBerkNodeTemp->pKey) return -1;
		    }
		/* get rid of any previous data */
		if(pBerkNodeTemp->pKey->data)
		    nmSysFree(pBerkNodeTemp->pKey->data);
		memset(pBerkNodeTemp->pKey, 0, sizeof(DBT));
		/* get what's at the cursor */
		pBerkNodeTemp->pCursor->c_get(pBerkNodeTemp->pCursor, &tempKey, &tempData, DB_CURRENT);
		/* allocate memory for the primary key */
		pBerkNodeTemp->pKey->data = nmSysMalloc(tempKey.size);
		if(!pBerkNodeTemp->pKey->data) return -1;
		/* copy the key from where the cursor is pointing */
		pBerkNodeTemp->pKey->size = tempKey.size;
		memcpy(pBerkNodeTemp->pKey->data, tempKey.data, tempKey.size);
		/* close the cursor, but leave the memory allocated */
		pBerkNodeTemp->pCursor->c_close(pBerkNodeTemp->pCursor);
		/* The cursor is now in the 'key' state */
		pBerkNodeTemp->fState = BERK_NODESTATE_KEY;
		}
	    /* go to the next Node */
	    pBerkNodeTemp = pBerkNodeTemp->pNext;
	    }
	return 0; /* success */
    }

/*** Verify the existance of a record: create it if needed and allowed	***/
/*** Also, integrate any data that is in inf->newData if needed		***/
int
berkInternalRecVerify(pBerkData inf)
    {
    int r;
    int i;

	
	inf->pData->flags = DB_DBT_MALLOC;
	if(!inf->pMyEnvNode->TheDatabaseIni)
	    {
	    mssError(0, "BERK", "berkInternalRecVerify: The database hasn't been initalized yet");
	    return -1;
	    }
	r = inf->pMyEnvNode->TheDatabase->get(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
	if(r == DB_LOCK_DEADLOCK)
	    {
	    for(i = 0;i<10 && r == DB_LOCK_DEADLOCK; i++)
		 r = inf->pMyEnvNode->TheDatabase->get(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
	    if(r ==  DB_LOCK_DEADLOCK)
		{
		mssError(0, "BERK", "Unable to get around deadlock");
		return -1;
		}
	    }
	if(r == EINVAL)
	    {
	    mssError(0, "BERK", "berkInternalRecVerify: get returned EINVAL invalid parameter error");
	    return -1;
	    }
        if(r == DB_NOTFOUND)
	    {
	    if(inf->obj->Mode & O_CREAT)
		{
#if DEBUGGER
		mssError(0, "BERK", "berkInternalRecVerify: key not found, so going to create it. pKey = '%s'", inf->pKey->data);
#endif
		/* put an array with 1 element that is NULL into data */
		if(inf->pNewData && inf->pNewData->data)
		    {
		    inf->pData->data = nmSysMalloc(inf->pNewData->size);
		    if(!inf->pData->data) return -1;
		    memcpy(inf->pData->data, inf->pNewData->data, inf->pNewData->size);
		    inf->pData->size = inf->pNewData->size;
		    nmSysFree(inf->pNewData->data);
		    nmSysFree(inf->pNewData);
		    nmSysFree(inf->newData);
		    }
		else
		    {
		    inf->pData->data = nmSysMalloc(1);
		    if(!inf->pData->data) return -1;
		    inf->pData->size = 1;
		    ((char*)inf->pData->data)[0] = 0;
		    }
		inf->pData->flags = 0;
		/* create the thang */
		/* make sure it has writing allowed */
		if(!((inf->obj->Mode & O_ACCMODE) == O_WRONLY || (inf->obj->Mode & O_ACCMODE) == O_RDWR)) return -1;
		/* close any open cursors (to allow writing to the database) */
		berkInternalCloseCursors(inf);
		/* add the pKey to the database */
		r = inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		for(i=0;r==DB_LOCK_DEADLOCK && i < 10;i++)
		    r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		if(r != 0)
		    return -1;
		return 0;
		}
	    mssError(0, "BERK", "Open failed: tried to get key '%s' which doesn't exist", inf->pKey->data);
	    return -1;
	    }
	else if(r == 0 && inf->obj->Mode & O_CREAT && inf->obj->Mode & O_EXCL)
	    {
	    mssError(0, "BERK", "RecVerify called with O_CREAT and O_EXCL with '%s' that exists", inf->pKey->data);
	    return -1;
	    }
	else if(r)
	    {
	    mssError(0, "BERK", "get failed for some reason '%d'", r);
	    return -1;
	    }
	if(inf->pNewData)
	    {
#if DEBUGGER
	    mssError(0, "BERK", "  Integrating the new data with the data in the just opened record");
#endif
		if(inf->pNewData->size > inf->pData->size)
		{
		    inf->pData->data = nmSysRealloc(inf->pData->data, inf->pNewData->size);
		    memset((void*)(inf->pData->data+inf->pData->size), 0, (inf->pNewData->size - inf->pData->size));
		    inf->pData->size = inf->pNewData->size;
		}
		for(i=0;i<inf->pNewData->size;i++)
		    {
			if(inf->newData[i])
			    ((char*)(inf->pData->data))[i] = ((char*)(inf->pNewData->data))[i];
		    }
		if(inf->newData[inf->pNewData->size] && inf->pData->size > inf->pNewData->size)
		    inf->pData->data = nmSysRealloc(inf->pData->data, inf->pNewData->size);
		/* add the newData to the database */
		r = inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		for(i=0;r==DB_LOCK_DEADLOCK && i < 10;i++)
		    r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		if(r)
		    return -1;
		nmSysFree(inf->pNewData->data);
		nmSysFree(inf->pNewData);
		nmSysFree(inf->newData);
		inf->pNewData = NULL;
		inf->newData = NULL;
	    }
	return 0;
    }


/*** called when something goes wrong and the inf needs to be destroyed ***/
void
berkInternalDestructor(pBerkData inf)
    {
    pEnvNode	pEnvNodeTemp;
    int		i, r;
    /* if inf is NULL, nothing needs to be done */
    if(inf)
	{
	/* find out if there has been a key update */
	if(inf->newKey)
	    {
	    /* Make sure that this is a DBT */
	    if(inf->Type == BERK_TYPE_DBT && ((inf->obj->Mode & O_ACCMODE) == O_RDONLY || (inf->obj->Mode & O_ACCMODE) == O_RDWR))
		{
		/* get the data */
		berkInternalRead(inf, -1, 0, 0);
		/* remove the current key */
		inf->pMyEnvNode->TheDatabase->del(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, 0);
		/* free pKey */
		nmSysFree(inf->pKey->data);
		/* copy newKey into pKey */
		inf->pKey->data = nmSysMalloc(strlen(inf->newKey));
		if(inf->pKey->data)
		    {
		    memcpy(inf->pKey->data, inf->newKey, strlen(inf->newKey));
		    inf->pKey->size = strlen(inf->newKey);
		    /* insert the new key with the saved data */
		    berkInternalCloseCursors(inf);
		    r = DB_LOCK_DEADLOCK;
		    for(i=0;r==DB_LOCK_DEADLOCK && i < 10;i++)
			r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		    /* free newKey */
		    nmSysFree(inf->newKey);
		    }
		}
	    }
	/* free pKey */
        if(inf->pKey)
	    {
	    if(inf->pKey->data)
		nmSysFree(inf->pKey->data);
	    nmFree(inf->pKey, sizeof(DBT));
	    }
	/* free pData */
	if(inf->pData)
	    {
	    if(inf->pData->data)
		nmSysFree(inf->pData->data);
	    nmFree(inf->pData, sizeof(DBT));
	    }
	/* remove myself from the EnvNode list */
	if(inf->pMyEnvNode)
	    {
	    inf->pMyEnvNode->nNumOpen--;
	    /* if nobody else is using the Env, the get rid of it */
	    if(inf->pMyEnvNode->nNumOpen <= 0)
		{
		inf->pMyEnvNode->TheDatabaseIni = 0;
		inf->pMyEnvNode->TheDatabase->close(inf->pMyEnvNode->TheDatabase, 0);
		}
	    }
	nmFree(inf, sizeof(BerkData));
	}
    }

/*** open the environment and the database ***/
int
berkInternalPrepareDb(pBerkData inf)
    {
    pEnvNode pEnvNodeTemp;
    pEnvNode pPrevNode;
        pEnvNodeTemp = BERK_INF.pEnvList;
#if DEBUGGER
	mssError(0, "BERK", "PrepareDb called");
#endif
	/* find the environment */
        while(pEnvNodeTemp != NULL && inf->pMyEnvNode==NULL)
            {
            if(!strcmp(inf->Filename, pEnvNodeTemp->Filename))
                {
                inf->pMyEnvNode = pEnvNodeTemp;
                inf->pMyEnvNode->nNumOpen++;
                }
            pPrevNode=pEnvNodeTemp;
            pEnvNodeTemp=pEnvNodeTemp->pNext;
            }
	/* if the Env wasn't found, create a new one */
        if(inf->pMyEnvNode == NULL)
	    if(!berkInternalCreateEnv(inf, pPrevNode))
		{
		mssError(0, "BERK", "berkInternalPrepareDb: berkInternalCreateEnv failed");
		return -1;
		}

	/* if the database itself isn't open, open it */
	if(!inf->pMyEnvNode->TheDatabaseIni)
	    {
	    /* It's time to open TheDatabase */
	    inf->pMyEnvNode->TheDatabase = nmMalloc(sizeof(DB));
	    if(!inf->pMyEnvNode->TheDatabase) 
		{
		mssError(0, "BERK", "berkInternalPrepareDb: malloc failed");
		berkInternalDestructor(inf);
		return -1;
		}
	    /* create the database handle */
	    if(db_create(&(inf->pMyEnvNode->TheDatabase), /*inf->pMyEnvNode->pDbEnv*/NULL, 0))
		{
		mssError(0, "BERK", "berkInternalPrepareDb: db_create failed");
		berkInternalDestructor(inf);
		return -1;
		}
	    /* open the database */
	    if(inf->pMyEnvNode->TheDatabase->open(inf->pMyEnvNode->TheDatabase, inf->Filename, NULL, DB_UNKNOWN, 0, inf->mask) != 0)
		{
		if(!(inf->obj->Mode & O_CREAT))
		    {
		    mssError(0, "BERK", "berkInternalPrepareDb: open the database failed (filename == '%s')", inf->Filename);
		    berkInternalDestructor(inf);
		    }
		else /* we've gotta create this database */
		    {
		    /* Database creation not yet supported */
		    mssError(0, "BERK", "berkInternalPrepareDb: tried to create a database, which isn't supported yet '%s'", inf->Filename);
		    return -1;
		    /* we CAN'T actually open the thing now, since we don't know the access method */
		    inf->pMyEnvNode->TheDatabaseIni = 0;
		    inf->AccessMethod = DB_UNKNOWN;
		    return 0;
		    }
		    
		return -1;
		}
	    inf->AccessMethod = inf->pMyEnvNode->TheDatabase->get_type(inf->pMyEnvNode->TheDatabase);
	    inf->pMyEnvNode->TheDatabaseIni = 1;
	    }
	return 0;
    }


/*** Open an object ***/
void*
berkOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pBerkData	inf;
    int		i, r;

	/* Say hello to me! */
	inf = (pBerkData)nmMalloc(sizeof(BerkData));
	if(!inf) return NULL;
	memset(inf, 0, sizeof(BerkData));
	inf->obj = obj;
	inf->mask = mask;
	i=0;

#if DEBUGGER
	mssError(0, "BERK", "berkOpen obj->Pathname is '%s'", obj->Pathname->Pathbuf);
#endif
	
	/* Copy the path into the inf->Pathname */
        obj_internal_CopyPath(&(inf->Pathname), obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;
	
	/* Determine the type.  This sets up the DBTs needed if this is a subobject */
        if(berkInternalDetermineType(inf) < 0)
	    {
	    mssError(0, "BERK", "berkOpen berkInternalDetermineType failed");
	    berkInternalDestructor(inf);
	    return NULL;
	    }
	if(inf->AutoName)
	    {
	    /* gets rid of OBJ_O_EXCL */
	    /* FIXME:  this is a hack until the AUTONAME Mode bug gets fixed */
	    obj->Prev->Mode &= ~OBJ_O_EXCL;
	    }
	/* get the environment, and the database open */
	if(berkInternalPrepareDb(inf) != 0)
	    {
	    mssError(0, "BERK", "berkOpen berkInternalPrepareDb failed");
	    berkInternalDestructor(inf);
	    return NULL;
	    }
	if(inf->Type == BERK_TYPE_DBT)
	    {
	    if(inf->AutoName)
		{
		return (void*)inf;
		}
	    inf->pData->flags = DB_DBT_MALLOC;
#if DEBUGGER
	    mssError(0, "BERK", "berkOpen:  about to call get with inf->pKey->data = '%s' and size '%d'", inf->pKey->data, inf->pKey->size);
#endif
	    /* make sure the record exists and is valid */
	    if(berkInternalRecVerify(inf))
		{
		mssError(0, "BERK", "berkOpen: RecVerify failed");
		return NULL;
		}
	    }
	return (void*)inf;
    }

/*** close the object ***/
int
berkClose(void* inf_v, pObjTrxTree* oxt)
    {
    /* make it all go away */
#if DEBUGGER
    mssError(0, "BERK", "berkClose called. '%s' was Filename", ((pBerkData)inf_v)->Filename);
#endif
    berkInternalDestructor((pBerkData)inf_v);
    return 0;
    }

/*** create a new object ***/
int
berkCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pBerkData inf;
	
	obj->Mode |= O_CREAT;
	inf = berkOpen(obj, mask, systype, usrtype, oxt);
	if(!inf) return -1;
	/* you can't use AutoName and call Create */
	if(inf->AutoName)
	    {
	    berkInternalDestructor(inf);
	    return -1;
	    }
    return datClose(inf, oxt);
    }

/*** DBTs (records) can be deleted ***/
int
berkDelete(pObject obj, pObjTrxTree* oxt)
    {
    pBerkData	inf;
    int		r, i;
	/* this assumes the object calling is pointing to a pBerkData */
	/* is this a good assumption?  (for what it's worth, it's worked so far) */
	inf = (pBerkData)obj->Data;
	/* check to make sure this is a DBT */
	if(inf->Type != BERK_TYPE_DBT)
	    {
	    mssError(0, "BERK", "delete: only works for DBTs.  Can't delete whole file");
	    return -1;
	    }
#if DEBUGGER
	mssError(0, "BERK", " inf->pKey to be delete = '%s'", inf->pKey->data);
#endif
	i = 0;
	/* delete the record */
	r = inf->pMyEnvNode->TheDatabase->del(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, 0);
	if(r == EACCES )
	    {
	    mssError(0, "BERK", "berkDelete: failed because the database is read only");
	    }
	while(r == DB_LOCK_DEADLOCK && i < 10)
	    {
	    i++;
	    r = inf->pMyEnvNode->TheDatabase->del(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, 0);
	    }
	if(r)
	    {
	    mssError(0, "BERK", "berkDelete: failed for some reason");
	    }
#if DEBUGGER
	mssError(0, "BERK",  "berkDelete has exited successfully");
#endif
    return 0;
    }

/*** this function reads the content into the inf->pData ***/
/*** Must be run only on DBTs, not files ***/
/*** NOTE: this function ignores both the offset and the inf->nOffset unless SEEK flag is set ***/
int
berkInternalRead(pBerkData inf, int maxcnt, int offset, int flags)
    {
    int	    r, i;
	
	/* if inf->AutoName, then the only data that exists is in pData already */
	if(inf->AutoName)
	    {
	    mssError(0, "BERK", "berkRead note: autoname hasn't been resolved yet: data possbily not complete");
	    if(inf->pData->data)
		return 0;
	    mssError(0, "BERK", "berkRead tried to read from an empty uninitialized DBT opened using AUTONAME");
	    return -1;
	    }
	/* prepare the data DBT for the read */
        if(inf->pData->data)
            {
            nmSysFree(inf->pData->data);
            inf->pData->data = NULL;
            inf->pData->size = 0;
            }
	/* maxcnt == -1 is used to say grab ALL of the data */
	if(maxcnt == -1 || flags != FD_U_SEEK)
            {
	    /* tells the database to allocate as much memory as is needed */
            inf->pData->flags = DB_DBT_MALLOC;
            }
        else
            {
	    /* put the partial limitations into the DBT */
            inf->pData->flags = DB_DBT_MALLOC | DB_DBT_PARTIAL;
            inf->pData->dlen = maxcnt;
            inf->pData->doff = offset;
            }
        r = DB_LOCK_DEADLOCK; /*make sure the loop is run at least once */
	/* do the read.  Try again if it gets killed to undo a deadlock */
        for(i=0;r==DB_LOCK_DEADLOCK && i<10;i++)
            r = inf->pMyEnvNode->TheDatabase->get(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
	/* return error if it didn't work */
        if(r!=0)
	    {
	    switch(r)
		{
		case DB_NOTFOUND:
		case DB_KEYEMPTY:
		    mssError(0, "BERK", "    berkInternalRead: pKey '%s' not found", (char*)inf->pKey->data);
		    break;
		case DB_LOCK_DEADLOCK:
		    mssError(0, "BERK", "    berkInternalRead: DB->get couldn't run without deadlock");
		    break;
		case ENOMEM:
		    mssError(0, "BERK", "    berkInternalRead: DB->get failed: not enough memory");
		    break;
		case EINVAL:
		    mssError(0, "BERK", "    berkInternalRead: invalid flag set for DB->get");
		    break;
		default:
		    mssError(0, "BERK", "    berkInternalRead: DB->get failed: unknown error");
		}
            return -1;
	    }
	return 0;
    }

/*** NOTE Read and write are valid only for DBTs ***/
/*** You can't read or write a whole database	 ***/
int
berkRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pBerkData	inf;
    int		r;
#if DEBUGGER
	mssError(0, "BERK", "berkRead called.  asked for '%d' bytes", maxcnt);
#endif
	inf=(pBerkData)inf_v;
	/* Make sure that this is a DBT */
	if(inf->Type != BERK_TYPE_DBT){ mssError(0, "BERK", "berkRead can only be called on DBTs"); return -1;}
	/* Make sure that maxcnt is positive */
	if(maxcnt <= 0) return -1;
	/* Get the offset to the right place */
	if(flags == FD_U_SEEK)inf->nOffset = offset;
	else offset = inf->nOffset;
	/* Make sure that reading is allowed */
	if(!((inf->obj->Mode & O_ACCMODE) == O_RDONLY || (inf->obj->Mode & O_ACCMODE) == O_RDWR))
	    {
	    mssError(0, "BERK", "berkRead: error: mode isn't O_RDONLY || O_RDWR");
	    return -1;
	    }
	/* Copies the data at the offset into the inf->pData */
	if(berkInternalRead(inf, maxcnt, offset, FD_U_SEEK) == -1)
	    {
	    mssError(0, "BERK", "berkRead: call to berkInternalRead failed");
	    return -1;
	    }
	/* copy the data into buffer */
	if(inf->pData->size < maxcnt)
	    {
	    /* copy all the data in pData->data */
	    memcpy(buffer, inf->pData->data, inf->pData->size);
	    /* NULL terminate the data */ 
	    buffer[inf->pData->size]=0;
	    /* update the offset for next read or write */
	    inf->nOffset += inf->pData->size;
#if DEBUGGER
	    mssError(0, "BERK", "berkRead: returning '%d' bytes with value '%s'", inf->pData->size, buffer);
#endif
	    return inf->pData->size;
	    }
	else
	    {
	    /* copy the part of the data that will fit */
	    memcpy(buffer, inf->pData->data, maxcnt);
	    /* update the offset for the next read or write */
	    inf->nOffset += maxcnt;
#if DEBUGGER
	    mssError(0, "BERK", "berkRead: returning '%d' bytes with value '%s'", maxcnt, buffer);
#endif
	    return maxcnt;
	    }
    }

/*** write content into a DBT ***/
int
berkWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pBerkData	inf;
    int		r;
    int		i; /* used to limit # of times DEADLOCK will retry */
    void*	pt;
#if DEBUGGER	
	mssError(0, "BERK", "berkWrite called.  supposed to write '%s'", buffer);
#endif
	inf=(pBerkData)inf_v;
	/* Make sure this is a DBT object */
	if(inf->Type != BERK_TYPE_DBT) return -1;
	/* Make sure writing is allowed on this object */
	if(!((inf->obj->Mode & O_ACCMODE) == O_WRONLY || (inf->obj->Mode & O_ACCMODE) == O_RDWR)) return -1;
	/* make sure offset is set right. */
	if(flags & FD_U_SEEK) inf->nOffset = offset; 
	else offset = inf->nOffset;
	
	/* deal with uninitialized AUTONAME calls */
	if(inf->AutoName)
	    {
	    if(!inf->pNewData)
		{
		inf->pNewData = nmSysMalloc(sizeof(DBT));
		inf->pNewData->data = NULL;
		}
	    if(!inf->pNewData->data)
		{
		inf->pNewData->size = cnt+offset;
		inf->pNewData->data = nmSysMalloc(cnt+offset); if(!inf->pNewData->data) return -1;
		inf->newData = nmSysMalloc(cnt+offset+1); if(!inf->newData) return -1;
		memset(inf->pNewData->data, 0, inf->pNewData->size);
		memset(inf->newData, 0, inf->pNewData->size+1);
		}
	    else if(cnt + offset > inf->pNewData->size)
		{
		pt = nmSysMalloc(cnt+offset); if(!pt) return -1;
		memcpy(pt, inf->pNewData->data, inf->pNewData->size);
		nmSysFree(inf->pNewData->data);
		inf->pNewData->data = pt;
		inf->pNewData->size = cnt+offset;
		inf->newData = nmSysRealloc(inf->newData, inf->pNewData->size+1);
		}
	    /* keep track of what data has been written */
	    for(i=offset;i<offset+cnt;i++)
		inf->newData[i] = 1;
	    memcpy((void*)inf->pNewData->data + offset, (void*)buffer, cnt);
	    inf->nOffset+=cnt;
	    return cnt;
	    }
	    
	/* Close any open cursors on this Database */
	if(berkInternalCloseCursors(inf)) return -1;
	/* Fill inf->pData with what is currently in the object */
	if(berkInternalRead(inf, -1, 0, flags) == -1) return -1;
	/* Grow inf->pData->data if needed */
	if(cnt+offset > inf->pData->size)
	    {
	    pt = nmSysMalloc(cnt+offset); if(!pt) return -1;
	    memcpy(pt, inf->pData->data, inf->pData->size);
	    nmSysFree(inf->pData->data);
	    inf->pData->data = pt;
	    inf->pData->size = cnt+offset;
	    }
	/* Copy what is in buffer into DBT for writing */
	memcpy((void*)(inf->pData->data+offset), (void*)buffer, cnt);
	
	/*** do the write ***/
	r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
	/* if the put returns DB_LOCK_DEADLOCK, try again, but not forever */
	for(i=0;r==DB_LOCK_DEADLOCK && i < 10;i++)
	    r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
	if(r!=0)
	    return -1;
	/* The write has worked.  Set the nOffset for next read or write */
	inf->nOffset += cnt;
	return cnt;
    }

/*** berkQueryOpen - prepare a query to be run: open a cursor to the database ***/
void*
berkOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pBerkData	inf;
    pBerkNode	qy;
    
	inf=(pBerkData)inf_v;
#if DEBUGGER
	mssError(0, "BERK", "OpenQuery called");
#endif
	if(inf->Type != BERK_TYPE_FILE) return NULL;
	
	/* get new BerkNode */
	if(!BERK_INF.pBerkNodeList)
	    {
	    BERK_INF.pBerkNodeList = nmMalloc(sizeof(BerkNode));
	    if(!BERK_INF.pBerkNodeList) return NULL;
	    memset(BERK_INF.pBerkNodeList, 0, sizeof(BerkNode));
	    qy = BERK_INF.pBerkNodeList;
	    }
	else
	    {
	    qy = BERK_INF.pBerkNodeList;
	    while(qy->pNext != NULL)
		qy = qy->pNext;
	    qy->pNext = nmMalloc(sizeof(BerkNode));
	    if(!qy->pNext) return NULL;
	    memset(qy->pNext, 0, sizeof(BerkNode));
	    qy = qy->pNext;
	    }
	qy->pMyEnvNode = inf->pMyEnvNode;
	qy->pMyEnvNode->nNumOpen++;
	qy->fState = BERK_NODESTATE_NONE;
	query->Flags = 0;
	qy->pKey = nmMalloc(sizeof(DBT));
	if(!qy->pKey) return NULL;
	qy->pKey->flags = DB_DBT_MALLOC;
	qy->ObjSubPtr = inf->obj->SubPtr;

	/* NOTE: pCursor doesn't need to be given memory, the cursor function allocates the memory for it */
	
	return (void*)qy;
    
    }

/*** berkInternalOpenCursorKey() opens a cursor to a specific key ***/
int
berkInternalOpenCursorKey(pBerkNode qy)
    {
    DBT	    tempData;
    int	    r;
#if DEBUGGER
	mssError(0, "BERK", "InternalOpenCursorKey");
#endif
	tempData.flags = 0;
	if(!qy->pKey) return -1;
	if(qy->pMyEnvNode->TheDatabase->cursor(qy->pMyEnvNode->TheDatabase, NULL, &qy->pCursor, 0)) return -1;
	r = qy->pCursor->c_get(qy->pCursor, qy->pKey, &tempData, DB_SET);
	switch(r)
	    {
	    case DB_NOTFOUND:
		{
		/* find the next key after the one we were looking for */
		if(qy->pCursor->c_get(qy->pCursor, qy->pKey, &tempData, DB_SET_RANGE)) return -1;
		break;
		}
	    case 0:
		if(qy->pCursor->c_get(qy->pCursor, qy->pKey, &tempData, DB_NEXT)) return -1;
		break;
	    default:
		return -1;
	    }
	return 0;
    }
	
	

/*** berkQueryFetch - get the next item from the database. ***/
void*
berkQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pBerkNode	    qy;
    pBerkData	    inf;
    int		    oldMode;
    DBT		    tempData;
    char*	    p;
#if DEBUGGER
    char*	    pTemp;
#endif
    unsigned char   cTemp;
    int		    i;

#if DEBUGGER
	mssError(0, "BERK", "QueryFetch");
#endif
	tempData.flags = 0;
	qy = (pBerkNode)qy_v;
	/* this switch puts the key we are fetching into qy->pKey */
	switch(qy->fState)
	    {
	    case BERK_NODESTATE_KEY:
		/* this opens the key, and gets the next key into qy->pKey */
		if(berkInternalOpenCursorKey(qy))
		    {
		    mssError(0, "BERK", "QueryFetch: InternalOpenCursorKey failed");
		    return NULL;
		    }
		break;
	    case BERK_NODESTATE_NONE:
		/*create a cursor.  Becuase it isn't initialized, calling get with DB_NEXT will get the first key */
		if(qy->pMyEnvNode->TheDatabase->cursor(qy->pMyEnvNode->TheDatabase, NULL, &qy->pCursor, 0))
		    {
		    mssError(0, "BERK", "QueryFetch: creating the cursor failed");
		    return NULL;
		    }
		/* because there's no break, this goes on to run the next get statement */
	    case BERK_NODESTATE_CURSOR:
		/* NOTE: c_get will return DB_NOTFOUND when it reaches the end of the data */
		/* this is not an error condition, but we still return NULL on it */
		i = qy->pCursor->c_get(qy->pCursor, qy->pKey, &tempData, DB_NEXT);
		if(i)
		    {
		    if(i!=DB_NOTFOUND)
			mssError(0, "BERK", "QueryFetch: c_get Failed");
#if DEBUGGER
		    else
			mssError(0, "BERK", "QueryFetch: c_get has returned all the data");
#endif
		    return NULL;
		    }
		break;
	    default: /* not in a valid state */
		mssError(0, "BERK", "QueryFetch: qy->fState is in invalid state");
		return NULL;
		break;
	    }
	/* we now have an open cursor to the database */
	qy->fState = BERK_NODESTATE_CURSOR;
	
	/* Say hello to me! */
	inf = (pBerkData)nmMalloc(sizeof(BerkData)); if(!inf) return NULL;
	memset(inf, 0, sizeof(BerkData));
	inf->obj = obj;
	obj->SubPtr = qy->ObjSubPtr;
#if DEBUGGER
	pTemp = nmSysMalloc(qy->pKey->size+1);
	memcpy(pTemp, qy->pKey->data, qy->pKey->size);
	pTemp[qy->pKey->size] = 0;
	mssError(0, "BERK", " QueryFetch: pKey is '%s'", pTemp);
	nmSysFree(pTemp);
#endif

	/* copy the path into inf */
	obj_internal_CopyPath(&(inf->Pathname), obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = '/';

	/* look for the end of the pathname */
	p = memchr(inf->Pathname.Elements[0], 0, OBJSYS_MAX_PATH);
	/* Didn't find a NULL at the end of the pathname string */
	if(!p) return NULL;
	/* make sure there is at least one element in the pathname */
	if(p - inf->Pathname.Elements[0] == 0) return NULL;
	/* add a '/' at the end if it isn't there */
	if(*(p-1) != '/') { p[0] = '/'; p++; }
	/* make sure the pathname + the translated keyname will fit into a Pathname */
	if( p - inf->Pathname.Elements[0] + qy->pKey->size * 2 > OBJSYS_MAX_PATH) return NULL;
	/* translate the key into hex */
	if(berkInternalKeyToHex(p, qy->pKey->data, OBJSYS_MAX_PATH - (p - inf->Pathname.Elements[0]), qy->pKey->size)) return NULL;
	inf->Pathname.nElements++;
	/* there are too many elements in the path */
	if(inf->Pathname.nElements > OBJSYS_MAX_ELEMENTS)
	    {
	    mssError(0, "BERK", "QueryFetch: too many elements for Pathname");
	    nmFree(inf, sizeof(BerkData)); return NULL; 
	    }
	    
	inf->Pathname.Elements[inf->Pathname.nElements-1]=p;
	
	/* NULL seperate the elements in the pathname */
	for(i=1;i<inf->Pathname.nElements;i++)
	    *(inf->Pathname.Elements[i]-1) = 0;


	obj_internal_CopyPath(inf->obj->Pathname, &(inf->Pathname));
#if DEBUGGER
	mssError(0, "BERK", "QueryFetch: path check '%s'", inf->obj->Pathname->Elements[inf->obj->Pathname->nElements-1]);
#endif
        /* Determine the type.  This sets up the DBTs needed if this is a subobject */
        if(berkInternalDetermineType(inf) < 0)
            {
	    mssError(0, "BERK", "QueryFetch: call to DetermineType failed");
            berkInternalDestructor(inf);
            return NULL;
            }

	/* get the [already open] database */
	inf->pMyEnvNode = qy->pMyEnvNode;
	inf->pMyEnvNode->nNumOpen++;
#if DEBUGGER
	mssError(0, "BERK", "QueryFetch: success with Pathname = '%s'", inf->Pathname.Elements[inf->Pathname.nElements-1]);
#endif
	return (void*)inf;
		
    }

/*** berkQueryClose - Close the query ***/
int
berkQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pBerkNode	qy;
    pBerkNode	pTemp;

#if DEBUGGER
	mssError(0, "BERK", "QueryClose");
#endif
	qy = (pBerkNode)qy_v;
	/* remove the query from the list */
	if(qy == BERK_INF.pBerkNodeList)
	    BERK_INF.pBerkNodeList = BERK_INF.pBerkNodeList->pNext;
	else
	    {
	    pTemp = BERK_INF.pBerkNodeList;
	    while(pTemp != NULL && pTemp->pNext != qy)
		pTemp = pTemp->pNext;
	    if(pTemp!=NULL)
		pTemp = pTemp->pNext->pNext;
	    }

	/* remove it from the EnvNode that it's part of */
	qy->pMyEnvNode->nNumOpen--;
	if(qy->pMyEnvNode->nNumOpen <= 0)
	    {
	    qy->pMyEnvNode->TheDatabaseIni = 0;
	    qy->pMyEnvNode->TheDatabase->close(qy->pMyEnvNode->TheDatabase, 0);
	    }
	
	/* free all the memory it owns */
	if(qy->pKey)
	    {
	    if(qy->pKey->data)
		nmSysFree(qy->pKey->data);
	    nmFree(qy->pKey, sizeof(DBT));
	    }
	/* TODO  Look at the following statement.  Will that screw up if Cursor is already closed? */
	qy->pCursor->c_close(qy->pCursor);

	/* Goodbye */
	nmFree(qy, sizeof(BerkNode));
	return 0;
    }

/*** berkGetAttrType - returns the type for the attribute ***/
int
berkGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pBerkData inf;
	inf = (pBerkData)inf_v;

	/* These are the required attributes */
	if(!strcmp(attrname, "name")) return DATA_T_STRING;
	if(!strcmp(attrname, "content_type") || !strcmp(attrname, "inner_type") ||
		!strcmp(attrname, "outer_type")) return DATA_T_STRING;
	if(!strcmp(attrname, "annotation")) return DATA_T_STRING;

	/* the DBTs and files have different attributes */
	if(inf->Type == BERK_TYPE_DBT)
	    {
	    if(!strcmp(attrname, "data")) return DATA_T_STRING;
	    if(!strcmp(attrname, "key")) return DATA_T_STRING;	/* TODO: these can't really stay as strings */
	    if(!strcmp(attrname, "size"))return DATA_T_INTEGER;
	    if(!strcmp(attrname, "key_size")) return DATA_T_INTEGER;
	    if(!strcmp(attrname, "transdata")) return DATA_T_STRING;	/* temp solution: allow data to be translated into hex */
	    }
	
	if(inf->Type == BERK_TYPE_FILE)
	    {
	    if(!strcmp(attrname, "access_method")) return DATA_T_STRING;
	    }
	
	/* didn't find any valid attributes */
	return -1;


    }

/*** berkGetAttrValue - gets the value of an attribute by name.  The 'val' ***/
/*** pointer must point to an appropriate data type. ***/
int
berkGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pBerkData inf;
	inf = (pBerkData)inf_v;

	if(!strcmp(attrname, "name" ))
	    {
	    if(datatype != DATA_T_STRING) return -1;
	    val->String=inf->ObjName;
	    return 0;
	    }
	if(!strcmp(attrname, "content_type") || !strcmp(attrname, "inner_type"))
	    { 
	    if(datatype != DATA_T_STRING) return -1;
	    if(inf->Type == BERK_TYPE_DBT)
		val->String="application/octet-stream";
	    else
		val->String="system/void";
	    return 0;
	    }
	if(!strcmp(attrname, "outer_type"))
	    {
	    if(datatype != DATA_T_STRING) return -1;
	    if(inf->Type == BERK_TYPE_DBT)
		val->String = "application/berkdbt";
	    else
		val->String = "application/berkfile";
	    return 0;
	    }
	if(!strcmp(attrname, "annotation"))
	    {
	    if(datatype!= DATA_T_STRING) return -1;
	    val->String=inf->Annotation;
	    return 0;
	    }
	if(inf->Type == BERK_TYPE_DBT)
	    {
	    if(datatype == DATA_T_STRING)
		{
		/* inf->pStrAttr holds the string data with an added NULL at the end */
		if(inf->pStrAttr)
		    nmSysFree(inf->pStrAttr);
		if(!strcmp(attrname, "data"))
		    {
		    /* get the data from the database.  this call simply reads the all the data into pData */
		    if(berkInternalRead(inf, -1, 0, 0))
			{
			mssError(0, "BERK", "berkGetAttrValue tried to read data, which failed");
			return -1;
			}
		    /* copy the data into pStrAttr, then return it */
		    inf->pStrAttr = nmSysMalloc(inf->pData->size+1);
		    memcpy(inf->pStrAttr, inf->pData->data, inf->pData->size);
		    inf->pStrAttr[inf->pData->size] = 0;
		    val->String = inf->pStrAttr;
		    }
		else if(!strcmp(attrname, "key"))
		    {
		    /* check to see if key has been set */
		    if(inf->newKey)
			{
			inf->pStrAttr = nmSysMalloc(strlen(inf->newKey) + 1);
			memcpy(inf->pStrAttr, inf->newKey, strlen(inf->newKey) + 1);
			}
		    else
			{
			inf->pStrAttr = nmSysMalloc(inf->pKey->size+1);
			memcpy(inf->pStrAttr, inf->pKey->data, inf->pKey->size);
			}
		    /* make sure the string is NULL terminated */
		    inf->pStrAttr[inf->pKey->size] = 0;
		    val->String = inf->pStrAttr;
		    }
		else if(!strcmp(attrname, "transdata"))
		    {
		    /* data translated into hex form */
		    inf->pStrAttr = nmSysMalloc(inf->pData->size * 2 + 1);
		    inf->pStrAttr[inf->pData->size * 2] = 0;
		    if(berkInternalKeyToHex(inf->pStrAttr, inf->pData->data, inf->pData->size*2+1, inf->pData->size))
			return -1;
		    val->String = inf->pStrAttr;
		    }
		else /* attrname wasn't data or key */
		    return -1;
		return 0;
		}
	    if(datatype == DATA_T_INTEGER)
		{
		if(!strcmp(attrname, "size"))
		    {
		    if(berkInternalRead(inf, -1, 0, 0))
			{
			mssError(0, "BERK", "berkGetAttrValue was asked to find size: the read failed");
			return -1;
			}
		    val->Integer = inf->pData->size;
		    }
		else if(!strcmp(attrname, "key_size"))
		    {
		    if(inf->newKey)
			val->Integer = strlen(inf->newKey);
		    else
			val->Integer = inf->pKey->size;
		    }
		else
		    return -1;
		return 0;
		}
	    }
	if(inf->Type == BERK_TYPE_FILE)
	    {
	    if(!strcmp(attrname, "access_method"))
		{
		if(datatype != DATA_T_STRING) return -1;
		/* update the AccessMethod */
		inf->AccessMethod = inf->pMyEnvNode->TheDatabase->get_type(inf->pMyEnvNode->TheDatabase);
		switch(inf->AccessMethod)
		    {
		case BERK_AM_UNKNOWN:	val->String = "unknown";	break;
		case BERK_AM_BTREE:	val->String = "btree";	break;
		case BERK_AM_HASH:	val->String = "hash";	break;
		case BERK_AM_QUEUE:	val->String = "queue";	break;
		case BERK_AM_RECNO:	val->String = "recno";	break;
		default:
		    return -1;
		    }
		return 0;
		}
	    }
	/* didn't find any matching attributes */
	return -1;
    }

/*** berkGetNextAttr - get the next attribute name for this object ***/
char*
berkGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pBerkData inf;
	inf = (pBerkData)inf_v;
	if(inf->Type == BERK_TYPE_FILE)
	    {
	    if(inf->nAttrCursor)
		return NULL;
	    inf->nAttrCursor++;
	    return "access_method";
	    }
	inf->nAttrCursor++;
	switch(inf->nAttrCursor)
	    {
	    case 1:
		return "data";
	    case 2:
		return "key";
	    case 3:
		return "size";
	    case 4:
		return "key_size";
	    case 5:
		return "transdata";
	    default:
		return NULL;
	    }
    }

/*** berkGetFirstAttr - get the first attribute name for this object ***/
char*
berkGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pBerkData inf;
	inf = (pBerkData)inf_v;
	inf->nAttrCursor = 1;
	if(inf->Type == BERK_TYPE_FILE)
	    return "access_method";
	else
	    return "data";
    }

/*** berkSetAttrValue - sets the value of an attribute. 'val' must ***/
/*** point to an appropriate data type.***/
int
berkSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pBerkData inf;
    int i, r, NewDataFlag;

    inf = (pBerkData)inf_v;
    NewDataFlag = 0;
    /* this is the same no matter what type this is */
    if(!strcmp(attrname, "annotation"))
	{
	if(datatype != DATA_T_STRING)
	    {
	    mssError(0, "BERK", "Trying to set 'annotation' attribute, datatype needs to be DATA_T_STRING");
	    return -1;
	    }
	memccpy(inf->Annotation, val->String, '\0', 255);
	inf->Annotation[255] = 0;
	return 0;
	}
    /* all other attributes are unique to there inf->Type */
    switch(inf->Type)
	{
	case BERK_TYPE_DBT:
	    {
	    if(!strcmp(attrname, "name"))
		{
		i = strlen(val->String);
		if(i != ((int)(i/2))*2)
		    {
		    mssError(0, "BERK", "SetAttrVal: invalid name: must be hex encoded, and therefor even length");
		    return -1;
		    }
		/* set the name */
		memccpy(inf->ObjName, val->String, '\0', OBJSYS_MAX_PATH);
		if(inf->AutoName)
		    {
		    inf->pKey->data = nmSysMalloc(i / 2);
		    inf->pKey->size = i / 2;
		    
		    if(berkInternalHexToKey(inf->pKey->data, inf->ObjName, inf->pKey->size, i)) return -1;
		    if(berkInternalRecVerify(inf)) return -1;
		    inf->AutoName = 0;
		    break;
		    }
		else
		    {
		    /* remove old newKey */
		    if(inf->newKey)
			nmSysFree(inf->newKey);
		    inf->newKey = nmSysMalloc((i / 2) + 1);
		    inf->newKey[(i/2)] = '\0';
		    if(!inf->newKey) return -1;
		    if(berkInternalHexToKey(inf->newKey, inf->ObjName, (i / 2), i)) return -1;
		    }
		/* change the name */
		return 0;
		}
	    if(!strcmp(attrname, "data"))
		{
		if(datatype != DATA_T_STRING)
		    {
		    mssError(0, "BERK", "Trying to set 'data' attribute, datatype needs to be string");
		    return -1;
		    }
		/* make sure it has writing allowed */
		if(!((inf->obj->Mode & O_ACCMODE) == O_WRONLY || (inf->obj->Mode & O_ACCMODE) == O_RDWR)) return -1;
		/* copy the new value from 'val' to 'inf->pData->data' */
		if(inf->pData->data) nmSysFree(inf->pData->data);
		/* inf->pData->size = strlen(*(const char**)val); */
		inf->pData->size = strlen(val->String);
		inf->pData->data = nmSysMalloc(inf->pData->size); if(!inf->pData->data) return -1;
		/* memcpy(inf->pData->data, *(void**)val, inf->pData->size); */
		memcpy(inf->pData->data, val->Generic, inf->pData->size); 
		NewDataFlag = 1;
		}
	    if(!strcmp(attrname, "transdata"))
		{
		if(datatype!= DATA_T_STRING)
		    {
		    mssError(0, "BERK", "Trying to set 'transdata' attribute, datatype needs to be string");
		    return -1;
		    }
		//i = strlen(*(const char**)val);
		i = strlen(val->String);
		
		if(i != (((int)(i/2))*2))
		    {
		    mssError(0, "BERK", "length of transdata must be even");
		    return -1;
		    }
		if(inf->pData->data) nmSysFree(inf->pData->data);
		inf->pData->size = i/2;
		inf->pData->data = nmSysMalloc(inf->pData->size);
		if(!inf->pData->data) return -1;
		berkInternalHexToKey(inf->pData->data, val->String, inf->pData->size, i);
		NewDataFlag = 1;
		}
	    if(NewDataFlag)
		{
		/* if inf->AutoName, then all we have to do is set inf->pData->data */
		if(inf->AutoName)
		    {
		    inf->newData = nmSysRealloc(inf->newData, inf->pData->size+1);
		    /* say that we've changed all the data */
		    memset(inf->newData, 1, inf->pData->size+1);
		    inf->pNewData->size = inf->pData->size;
		    if(inf->pNewData->data) nmSysFree(inf->pNewData->data);
		    inf->pNewData->data = nmSysMalloc(inf->pNewData->size);
		    memcpy(inf->pNewData->data, inf->pData->data, inf->pData->size);
		    return 0;
		    }
		/* close any cursors that are open on this database */
		if(berkInternalCloseCursors(inf)) return -1;
		/*** do the write ***/
		r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		/* if the put returns DB_LOCK_DEADLOCK, try again, but not forever */
		for(i=0;r==DB_LOCK_DEADLOCK && i < 10;i++)
		    r=inf->pMyEnvNode->TheDatabase->put(inf->pMyEnvNode->TheDatabase, NULL, inf->pKey, inf->pData, 0);
		if(r!=0)
		    return -1;
		return 0;
		}
	    if(!strcmp(attrname, "key"))
		{
		/* grrr, you're making me support renaming of objects */
		if(datatype != DATA_T_STRING)
		    {
		    mssError(0, "BERK", "Ha! You can't rename this key because datatype != DATA_T_STRING");
		    return -1;
		    }
		/* the name has finally arrived */
		if(inf->AutoName)
		    {
		    /* ok: we need to do these things */
		    /* set allocate and initialize data into pKey*/
#if 0		    
		    inf->pKey->data = nmSysMalloc(strlen(*(const char**)val));
		    inf->pKey->size = strlen(*(const char**)val);
		    memcpy(inf->pKey->data, *(void**)val, strlen(*(const char**)val));
#endif
		    inf->pKey->data = nmSysMalloc(strlen(val->String));
		    inf->pKey->size = strlen(val->String);
		    memcpy(inf->pKey->data, val->Generic, strlen(val->String));
		    /* translate into hex format */
		    if(berkInternalKeyToHex(inf->ObjName, inf->pKey->data, OBJSYS_MAX_PATH, inf->pKey->size)) return -1;
		    /* verify that the record is in the database OR create it */
		    if(berkInternalRecVerify(inf)) return -1;
		    /* tell it that the need for a name has been satisfied */
		    inf->AutoName = 0;
		    break;
		    }
		/* remove the old newKey */
		if(inf->newKey)
		    nmSysFree(inf->newKey);
		/* make room for the new one */
		inf->newKey = nmSysMalloc(strlen(val->String) + 1);
		inf->newKey[strlen(val->String)] = '\0';
		if(!inf->newKey) return -1;
		/* copy the key into inf->newKey */
		memcpy((void*)inf->newKey, val->Generic, strlen(val->String));
		/* on close, if newKey has memory, it will be copied in */
		}
	    break;
	    }
	case BERK_TYPE_FILE:
	    {
	    if(!strcmp(attrname, "access_method"))
		{
		/* make sure all our ducks are in a row */
		if(datatype != DATA_T_STRING)
		    {
		    mssError(0, "BERK", "datatype must be DATA_T_STRING for you to set the access_method");
		    return -1;
		    }
		if(inf->pMyEnvNode->TheDatabaseIni)
		    {
		    mssError(0, "BERK", "SetAttrValue: can't set access_method if the database exists already.");
		    return -1;
		    }
		mssError(0, "BERK", "Databases can't be created yet.  Tough luck");
		return -1;
		/* alrighty, let's do this */
		if(!strcmp(val->String, "btree")) inf->AccessMethod = BERK_AM_BTREE;
		if(!strcmp(val->String, "hash"))  inf->AccessMethod = BERK_AM_HASH;
		if(!strcmp(val->String, "queue")) inf->AccessMethod = BERK_AM_QUEUE;
		if(!strcmp(val->String, "recno")) inf->AccessMethod = BERK_AM_RECNO;
		if(!strcmp(val->String, "unknown")) return -1;
		r=inf->pMyEnvNode->TheDatabase->open(inf->pMyEnvNode->TheDatabase, inf->Filename, NULL, inf->AccessMethod, DB_CREATE, inf->mask);
		if(r)
		    {
		    if(r == EINVAL)
			mssError(0, "BERK", "Yeah, dang (IE something went wrong: Tried to create the database, which isn't supported yet");
		    mssError(0, "BERK", "SetAttrValue: Setting access_method.  open failed: (cont. in next error message)");
		    mssError(0, "BERK", "	Filename = '%s' and access_method = '%s'", inf->Filename, val->String);
		    berkInternalDestructor(inf);
		    }
		inf->pMyEnvNode->TheDatabaseIni = 1;
		return 0;
		}
		/* no other attributes are writeable at this point.  */
	    break;
	    }
	default:
	    return -1;
	} /* end of switch(inf->Type) */
    return -1;
    }

/*** berkAddAttr - add an attribute.  Refused  ***/
int
berkAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    return -1;
    }

/*** berkOpenAttr - going to be removed  ***/
void*
berkOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }

/*** berkGetFirstMethod - get the non-existant methods  ***/
char*
berkGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }

/*** berkGetNextMethod - no methods  ***/
char*
berkGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }

/*** berkExecuteMethod - no methods  ***/
int
berkExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** This section contains the functions that are passed to Berkeley for system calls ***/

int
berkInternalSysClose(int fd)
    {
#if DEBUGGER
    mssError(0, "BERK", "   SysClose got called with fd '%d'", fd);
#endif
    BERK_INF.pFDTable[fd]->nSysOpen--;
    if(BERK_INF.pFDTable[fd]->nSysOpen <= 0 && BERK_INF.pFDTable[fd]->nNumOpen <= 0)
	berkInternalDestroyEnv(BERK_INF.pFDTable[fd]);
    BERK_INF.pFDTable[fd] = NULL;
    return 0;
    }

void
berkInternalSysDirfree(char **namesp, int cnt)
    {
    mssError(0, "BERK", "   SysDirfree called");
    }

int
berkInternalSysDirlist(const char *dir, char ***namesp, int *cntp)
    {
    mssError(0, "BERK", "   SysDirlist called on dir '%s'", dir);
    /* error means 'not supported' */
    return ENOTSUP;
    }
int
berkInternalSysExists(const char *path, int *isdirp)
    {
#if DEBUGGER
    mssError(0, "BERK", "   SysExists called with the path '%s'", path);
#endif
    if(!strcmp(path, "/var/tmp"))
	{
	/* asked for existance of temp folder.  we're gonna say 'yes' */
	if(isdirp)
	    *(isdirp) = 1;
	return 0;
	}
    mssError(0, "BERK", "SysExist asked for '%s': we told it that it doesn't exist.  Yes, we did, my precious", path);
    /* just tell it that the file doesn't exist */
    return ENOENT;
    }

/*** berkInternalSysFree, pretty straight forward ***/
void
berkInternalSysFree(void *ptr)
    {
    nmSysFree(ptr);
    return;
    }

/*** SysFsync  this doesn't need to do anything, becuase everything gets written at write time ***/
int
berkInternalSysFsync(int fd)
    {
#if DEBUGGER
    mssError(0, "BERK", "   SysFsync got run for '%d'", fd);
#endif
    return 0;
    }

int
berkInternalSysIoinfo(const char *path, int fd, u_int32_t *mbytesp, u_int32_t *bytesp, u_int32_t *iosizep)
    {
    int r;
    ObjData MyObjData;
#if DEBUGGER
    mssError(0, "BERK", "   SysIoinfo asked for the file IO info for '%s'", path);
#endif
    
    if(BERK_INF.pFDTable[fd] != NULL)
	{
	if(!BERK_INF.pFDTable[fd]->PrevObj) 
	    {
	    mssError(0, "BERK", "SysIoinfo: pFDTable[fd]->PrevObj is NULL");
	    return ENOTSUP;
	    }
	
	if(objGetAttrType(BERK_INF.pFDTable[fd]->PrevObj, "size") != DATA_T_INTEGER)
	    {
	    return ENOTSUP;
	    }
	objGetAttrValue(BERK_INF.pFDTable[fd]->PrevObj, "size", DATA_T_INTEGER, &MyObjData);
	
	if(mbytesp)
	    *mbytesp = 0;
	if(bytesp)
	    *bytesp = MyObjData.Integer;
	if(iosizep)
	    *iosizep = 2048;
	return 0;
	}
    mssError(0, "BERK", "SysIoinfo run with bad fd.   path was '%s'", path);
    return ENOTSUP;
    }

/*** berkInternalSysMalloc, pretty straight forward ***/
void*
berkInternalSysMalloc(size_t size)
    {
    return nmSysMalloc(size);
    }

int
berkInternalSysMap(char *path, size_t len, int is_region, int is_rdonly, void **addr)
    {
    mssError(0, "BERK", "   SysMap tried to memory map path = '%s'", path);
    return ENOTSUP;
    }

int
berkInternalSysOpen(const char *path, int flags, ...)
    {
    int		i, *p;
    pEnvNode	pEnvNodeTemp, *e;
    char	cBuf[PATH_MAX];
    char*	pcBuf;

	pcBuf = getwd(cBuf);
#if DEBUGGER
	if(flags & O_CREAT)
	    mssError(0, "BERK", "   SysOpen got '%s' in working dir '%s' & O_CREAT", path, cBuf);
	else
	    mssError(0, "BERK", "   SysOpen got '%s' in working dir '%s' & !O_CREAT", path, cBuf);
#endif

	i=0;
	/* find next available FD */
	while(i<BERK_INF.TableSize && BERK_INF.pFDTable[i]!=NULL)
	    i++;
	/* if table is full, grow it */
	if(i == BERK_INF.TableSize)
	    {
	    BERK_INF.TableSize += 16;
	    e = nmMalloc(sizeof(pEnvNode) * BERK_INF.TableSize);
	    if(!e) return -1;
	    memset(e, 0, sizeof(pEnvNode) * BERK_INF.TableSize);
	    memcpy(e, BERK_INF.pFDTable, sizeof(pEnvNode) * (BERK_INF.TableSize - 16));
	    nmFree(BERK_INF.pFDTable, (BERK_INF.TableSize - 16) * sizeof(pEnvNode));
	    BERK_INF.pFDTable = e;
	    
	    p = nmMalloc(sizeof(int*) * BERK_INF.TableSize);
	    if(!p) return -1;
	    memset(p, 0, sizeof(int*) * BERK_INF.TableSize);
	    memcpy(p, BERK_INF.pOffsetTable, sizeof(int*) * (BERK_INF.TableSize - 16));
	    nmFree(BERK_INF.pOffsetTable, (BERK_INF.TableSize - 16) * sizeof(pEnvNode));
	    BERK_INF.pOffsetTable = p;
	    }
	/* at this point, i is the next available FD */
	pEnvNodeTemp = BERK_INF.pEnvList;
	while(pEnvNodeTemp && strcmp(pEnvNodeTemp->Filename, path))
	    {
	    pEnvNodeTemp = pEnvNodeTemp->pNext;
	    }
	/* even if this is null, we still want to put it in the list */
	BERK_INF.pFDTable[i] = pEnvNodeTemp;
	/* initialize the Offset to the beginning of the file: needed for when FD is reused */
	BERK_INF.pOffsetTable[i] = 0;
	if(pEnvNodeTemp)
	    {
#if DEBUGGER
	    mssError(0, "BERK", "   SysOpen is returning '%d'", i);
#endif
	    /* SysOpen is used to keep track how many times this file has been opened: is -- on close */
	    BERK_INF.pFDTable[i]->nSysOpen++;
	    return i;
	    }
	/*implied else*/
	mssError(0, "BERK", "berkInternalSysOpen: Got call to open file '%s'.  Not found", path);
	/* this is the error message saying that part of the directory tree couldn't be found */
	errno = ENOENT;
	return -1;
    }

ssize_t
berkInternalSysRead(int fd, void *buf, size_t nbytes)
    {
    int r;
#if DEBUGGER
	mssError(0, "BERK", "   SysRead, fd = '%d' trying to read '%d' bytes", fd, nbytes);
#endif
	if(BERK_INF.pFDTable[fd])
	    {
	    if(!BERK_INF.pFDTable[fd]->PrevObj)
		{
		mssError(0, "BERK", "SysRead: this filedescriptor has lost its link to its object");
		return -1;
		}
	    r = objRead(BERK_INF.pFDTable[fd]->PrevObj, buf, nbytes, BERK_INF.pOffsetTable[fd], FD_U_SEEK);
	    if(r!=-1)
		BERK_INF.pOffsetTable[fd] += r;
#if DEBUGGER
	    mssError(0, "BERK", "   SysRead returning '%d' bytes read", r);
#endif
	    return r;
	    }
	else
	    {
	    mssError(0, "BERK", "SysRead was called, but no files are open");    
	    return -1;
	    }
    }

void*
berkInternalSysRealloc(void *ptr, size_t size)
    {
    return nmSysRealloc(ptr, size);
    }

int
berkInternalSysRename(const char *from, const char *to)
    {
    mssError(0, "BERK", "   SysRename tried to rename '%s' to '%s', which is not supported.", from, to);
    /* this error says that one or both of the names are not allowed to be changed by this process */
    errno = EACCES;
    return -1;
    }

/*** TODO: Find out if centrallix supports files that are so big that an int can't hold the whole offset ***/
int
berkInternalSysSeek(int fd, size_t pgsize, db_pgno_t pageno, u_int32_t relative, int rewind, int whence)
    {
#if DEBUGGER
    mssError(0, "BERK", "   SysSeek on fd = '%d' amount = '%d', rewind = '%d'", fd, pgsize * pageno + relative, rewind);
#endif
    switch(whence)
	{
	case SEEK_SET:
	    BERK_INF.pOffsetTable[fd] = ((pgsize*pageno)+relative) * (1-2*((rewind)?1:0));
	    if(BERK_INF.pOffsetTable[fd] < 0)
	        return ESPIPE;
	    return 0;
	case SEEK_CUR:
	    BERK_INF.pOffsetTable[fd] += ((pgsize*pageno)+relative) * (1-2*((rewind)?1:0));
	    if(BERK_INF.pOffsetTable[fd] < 0)
	        return ESPIPE;
	    return 0;
	case SEEK_END:
	    mssError(0, "BERK", "SysSeek wants to seek from the end of the file, which isn't supported yet");
	    /* I don't know how to find out file length from here */
	default:
	    return ESPIPE;
		
	}
    
    }

/*** NOTE: this is a hack: it wants to be able to sleep for seconds, which thSleep doesn't support ***/
int
berkInternalSysSleep(u_long seconds, u_long microseconds)
    {
    return thSleep(microseconds);
    }

/*** Berkely isn't allowed to unlink.  ***/
int
berkInternalSysUnlink(const char *path)
    {
    mssError(0, "BERK", "   SysUnlink tried to unlink '%s'", path);
    errno = EACCES;
    return -1;
    }

int
berkInternalSysUnmap(void *addr, size_t len)
    {
    mssError(0, "BERK", "   SysUnmap tried to unmap something");
    return -1;
    }

ssize_t
berkInternalSysWrite(int fd, const void *buffer, size_t nbytes)
    {
    int r;
    void *temp;
#if DEBUGGER
	mssError(0, "BERK", "	SysWrite tried to write '%d' bytes to fd '%d'", nbytes, fd);
#endif
	temp = nmMalloc(nbytes); 
	if(!temp) return -1;
	memcpy(temp, buffer, nbytes);
        if(BERK_INF.pFDTable[fd])
            {
            r = objWrite(BERK_INF.pFDTable[fd]->PrevObj, temp, nbytes, BERK_INF.pOffsetTable[fd], FD_U_SEEK);
            if(r!=-1)
                BERK_INF.pOffsetTable[fd] += r;
	    else
		mssError(0, "BERK", "   SysWrite: objWrite failed to write");
	    nmFree(temp, nbytes);
#if DEBUGGER
	    mssError(0, "BERK", "   SysWrite is returning '%d'", r);
#endif
            return r;
	    }
        else
	    {
	    mssError(0, "BERK", "    SysWrite failed to write: PrevObj fd not properly opened");
	    nmFree(temp, nbytes);
            return -1;
	    }
    }

int
berkInternalSysYield()
    {
    return thYield();
    }

/*** berkInitialize - initialize the driver, and register it with OSML  ***/
int
berkInitialize()
    {
    pObjDriver drv;

    /** Allocate the driver **/
    drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
    if(!drv) return -1;
    memset(drv, 0, sizeof(ObjDriver));

    /** Set up Globals **/
    memset(&BERK_INF, 0, sizeof(BERK_INF));
    BERK_INF.TableSize = 16;
    BERK_INF.pFDTable = nmMalloc(sizeof(pEnvNode) * BERK_INF.TableSize);
    if(!BERK_INF.pFDTable) return -1;
    memset(BERK_INF.pFDTable, 0, sizeof(pEnvNode) * BERK_INF.TableSize);
    BERK_INF.pOffsetTable = nmMalloc(sizeof(int*) * BERK_INF.TableSize);
    if(!BERK_INF.pOffsetTable) { nmFree(BERK_INF.pFDTable, sizeof(pEnvNode) * BERK_INF.TableSize); return -1; }
    memset(BERK_INF.pOffsetTable, 0, sizeof(int*) * BERK_INF.TableSize);

    strcpy(drv->Name, "BERK - Berkeley Database Driver");
    drv->Capabilities = 0;
    xaInit(&(drv->RootContentTypes), 16);
    xaAddItem(&(drv->RootContentTypes), "application/berkfile");
    xaAddItem(&(drv->RootContentTypes), "application/berkrec");
    
    /*** setup the function references ***/
    drv->Open=berkOpen;
    drv->Close=berkClose;
    drv->Create=berkCreate;
    drv->Delete=berkDelete;
    drv->OpenQuery=berkOpenQuery;
    drv->QueryDelete=NULL;
    drv->QueryFetch=berkQueryFetch;
    drv->QueryClose=berkQueryClose;
    drv->Read=berkRead;
    drv->Write=berkWrite;
    drv->GetAttrType=berkGetAttrType;
    drv->GetAttrValue=berkGetAttrValue;
    drv->GetFirstAttr=berkGetFirstAttr;
    drv->GetNextAttr=berkGetNextAttr;
    drv->SetAttrValue=berkSetAttrValue;
    drv->AddAttr=berkAddAttr;
    drv->OpenAttr=berkOpenAttr;
    drv->GetFirstMethod=berkGetFirstMethod;
    drv->GetNextMethod=berkGetNextMethod;
    drv->ExecuteMethod=berkExecuteMethod;
    drv->PresentationHints = NULL;

    /* override functions to allow berkeley to talk to the Previous Object */
    db_env_set_func_close(berkInternalSysClose);
    db_env_set_func_dirfree(berkInternalSysDirfree);
    db_env_set_func_dirlist(berkInternalSysDirlist);
    db_env_set_func_exists(berkInternalSysExists);
    db_env_set_func_free(berkInternalSysFree);
    db_env_set_func_fsync(berkInternalSysFsync);
    db_env_set_func_ioinfo(berkInternalSysIoinfo);
    db_env_set_func_malloc(berkInternalSysMalloc);
    db_env_set_func_map(berkInternalSysMap);
    db_env_set_func_open(berkInternalSysOpen);
    db_env_set_func_read(berkInternalSysRead);
    db_env_set_func_realloc(berkInternalSysRealloc);
    db_env_set_func_rename(berkInternalSysRename);
    db_env_set_func_seek(berkInternalSysSeek);
    db_env_set_func_sleep(berkInternalSysSleep);
    db_env_set_func_unlink(berkInternalSysUnlink);
    db_env_set_func_unmap(berkInternalSysUnmap);
    db_env_set_func_write(berkInternalSysWrite);
    db_env_set_func_yield(berkInternalSysYield);

    
    if(objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(berkInitialize);
MODULE_PREFIX("berk");
MODULE_DESC("BerkeleyDB ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

