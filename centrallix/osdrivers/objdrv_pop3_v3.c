#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is nmSysFree software; you can redistribute it and/or modify	*/
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
/* Module: 	POP3 Object System Driver  				*/
/* Author:	Matt McGill <matt_mcgill@tayloru.edu>                   */
/* Creation:	June 14                					*/
/* Description:	     Allows the integeration of multiple POP mail   	*/
/*		     servers, with selected maildrops and their      	*/
/*		     messages, to be accessed by Centrallix.            */
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_pop3_v3.c,v 1.4 2008/04/06 22:12:16 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_pop3_v3.c,v $

    $Log: objdrv_pop3_v3.c,v $
    Revision 1.4  2008/04/06 22:12:16  gbeeley
    - (change) use nmSysXxx for memory management

    Revision 1.3  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.2  2004/06/23 21:33:55  mmcgill
    Implemented the ObjInfo interface for all the drivers that are currently
    a part of the project (in the Makefile, in other words). Authors of the
    various drivers might want to check to be sure that I didn't botch any-
    thing, and where applicable see if there's a neat way to keep track of
    whether or not an object actually has subobjects (I did not set this flag
    unless it was immediately obvious how to test for the condition).

    Revision 1.1  2004/06/22 19:53:18  mmcgill
    Added a new POP3 driver. The driver is responsible for three types of
    objects: the POP server object, the POP maildrop object, and the POP
    message object. Servers contain maildrops, which contain messages.

    The driver works, but some functionality is missing. The only method
    of enumerating maildrops that has been implemented is 'currlogin'. The
    only method of authenticating is also 'currlogin'. In other words, the
    driver will assume that the username/password to use on the POP server
    are the username/password you used to log in to Centrallix. Other methods
    are stubbed out.

    The driver *should* handle concurrency properly, sharing one connection
    between multiple open objects on the same POP server. I haven't tested
    this strenuously yet.

    Also, when a message object is opened, it is pulled off the
    server and stored in memory in its entirety, until the object is closed.
    This isn't such a good idea for e-mails with a few 15meg attatchments,
    and should be changed whenever the object system is retrofitted with a
    universal caching mechanism.

    Revision 1.5  2004/06/11 21:06:57  mmcgill
    Did some code tree scrubbing.

    Changed popGetAttrValue(), popSetAttrValue(), popAddAttr(), and
    popExecuteMethod() to use pObjData as the type for val (or param in
    the case of popExecuteMethod) instead of void* for the audio, BerkeleyDB,
    GZip, HTTP, MBox, MIME, and Shell drivers, and found/fixed a 2-byte buffer
    overflow in objdrv_shell.c (line 1046).

    Also, the Berkeley API changed in v4 in a few spots, so objdrv_berk.c is
    broken as of right now.

    It should be noted that I haven't actually built the audio or Berkeley
    drivers, so I *could* have messed up, but they look ok. The others
    compiled, and passed a cursory testing.

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

    Revision 1.3  2002/07/29 01:18:07  jorupp
     * added the include and calls to build as a module

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:00  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:02  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** The type of an object opened by this driver.
 *** Corresponds to the SubCnt of each type, by the way.
 ***/
#define POP_T_SERVER		1
#define POP_T_MAILDROP		2
#define	POP_T_MSG		3

#define POP_ENUM_FILE		0
#define POP_ENUM_LIST		1
#define POP_ENUM_CURRLOGIN	2

#define POP_AUTH_CURRLOGIN	0
#define POP_AUTH_PROMPT		1

/*** Structures used by this driver internally. ***/

typedef struct
    {
    pXArray	uids;
    int		LinkCnt;
    pFile	Session;
    long	Size;
    char*	UserName;	/** username/crypt()'d passwd of user that opened the drop **/
    char*	CryptPasswd;
    char*	DropName;
    pSemaphore	Sem;
    } PopMaildrop, *pPopMaildrop;
    
typedef struct 
    {
    /** Variables all object types use **/
    char	Pathname[256];	/** Hold the full path to the object **/
    pObject	Obj;
    int		CurAttr;
    pSnNode	Node;
    pXArray	AttribNames;
    pXArray	AttribVals;
    char*	Host;
    char*	Port;
    char*	EnumList;
    char*	EnumFilename;

    /** Server-specific variables **/
    int		MaildropEnumMethod;
    int		AuthMethod;

    /** Maildrop/Message-specific variables **/
    pPopMaildrop    Drop;	/** The mail drop, or the maildrop related to the message **/
    int		MsgNum;
    char*	MsgUID;
    int		Pos;
    char*	Content;
    int		ContentSize;
    }
    PopData, *pPopData;


#define POP(x) ((pPopData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pPopData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    
    pXArray	Maildrops;	/** Names a POP server will enumerate - not used by a Maildrop or Message **/
    }
    PopQuery, *pPopQuery;

/*** GLOBALS ***/
struct 
    {
    XHashTable	OpenMaildrops;
    }
    POP_INF;



/*** pop_internal_GetType(pPopData inf) - returns the type of the object.
 *** Currently bases this of of inf->Obj->SubCnt, as the object type
 *** is directly related to its path length off the node. One path element
 *** means a server object, two path elements means an account object,
 *** three path elements means an e-mail object
 ***/
int
pop_internal_GetType(pPopData inf)
    {
    return inf->Obj->SubCnt;
    }


/*** pop_internal_GetResponse - receive a response from the server, and
 *** return 0 if negative, 1 if positive. < 0 on error.
 *** If buf == NULL, don't bother storing the response.
 ***/
int
pop_internal_GetResponse(pFile Conn, char* buf, int maxlen)
    {
	pLxSession s;
	char* line;

	if ( (s=mlxOpenSession(Conn, MLX_F_LINEONLY | MLX_F_NODISCARD)) == NULL) return -1;
	
	if (mlxNextToken(s) == MLX_TOK_ERROR) 
	    {
	    mlxCloseSession(s);
	    return -1;
	    }

	if ( (line = mlxStringVal(s, NULL)) == NULL) 
	    {
	    mlxCloseSession(s);
	    return -1;
	    }

	memset(buf, 0, maxlen);
	memccpy(buf, line+1, 0, maxlen-1);
	mlxCloseSession(s);
	return (line[0] == '+');
    }


#if 0
/*** pop_internal_ConnectToServer - attempts to connect to a pop server.
 ***/
int
pop_internal_ConnectToServer(pPopData inf)
    {
    char buf[256];
    int bytes_read;

	if (!inf->ServerName || !inf->Port) return -1;

	/** Make the connection **/
	if ( (inf->Conn = netConnectTCP(inf->ServerName, inf->Port, 0)) == NULL)
	    {
	    mssError(1, "POP", "Can't connect to '%s:%s'.", inf->ServerName, inf->Port);
	    pop_internal_FreePD(&inf);
	    return -1;
	    }
	
	/** Check the response, make sure it's positive **/
	if ( (bytes_read = fdRead(inf->Conn, buf, 255, 0, 0)) <= 0)
	    {
	    pop_internal_DisconnectFromServer(inf);
	    mssError(0, "POP", "Error receiving server response");
	    return -1;
	    }
	buf[bytes_read] = '\0';
	if (buf[0] != '+')
	    {
	    pop_internal_DisconnectFromServer(inf);
	    mssError(1, "POP", "Server returned a negative response: %s", buf+1);
	    return -1;
	    }
	return 0;
    }


/*** pop_internal_DisconnectFromServer - close the current connection
 ***/
int
pop_internal_DisconnectFromServer(pPopData inf)
    {
    if (!inf->Conn) return 0;
    if (inf->Authenticated) 
	{
	fdWrite(inf->Conn, "QUIT\r\n", strlen("QUIT\r\n"), 0, 0);
	inf->Authenticated = 0;
	}

    netCloseTCP(inf->Conn, 0, 0);
    inf->Conn = NULL;
    return 0;
    }
#endif


/*** pop_internal_FreeMaildrop - free up all the memory used for a maildrop
 ***/
void
pop_internal_FreeMaildrop(pPopMaildrop drop)
    {
    int i;
    char* uid;

	if (drop->LinkCnt != 1) return;

	if (drop->Session != NULL) netCloseTCP(drop->Session, 0, 0);

	if (drop->DropName != NULL) nmSysFree(drop->DropName);

	if (drop->Sem != NULL) syDestroySem(drop->Sem, 0);

	if (drop->uids != NULL)
	    {
	    for (i=0;i<xaCount(drop->uids);i++)
		{
		if ( (uid = xaGetItem(drop->uids, i)) != NULL) nmSysFree (uid);
		}
	    xaClear(drop->uids);
	    xaDeInit(drop->uids);
	    nmFree(drop->uids, sizeof(PopMaildrop));
	    }

	if (drop != NULL) nmFree(drop, sizeof(PopMaildrop));
    }


/*** pop_internal_ReleaseMaildrop - handle letting go of a maildrop; managing link counts, etc
 ***/
void
pop_internal_ReleaseMaildrop(pPopData inf)
    {
	inf->Drop->LinkCnt--;

	if (inf->Drop->LinkCnt == 0)
	    {
	    /** close the session, but don't free anything just yet **/
	    fdWrite(inf->Drop->Session, "QUIT\r\n", 6, FD_U_PACKET, 0);
	    netCloseTCP(inf->Drop->Session, 0, 0);
	    inf->Drop->Session = NULL;	// indicates that the session is closed
	    }
    }


/*** pop_internal_UpdateMaildrop - checks if the maildrop has changed, and updates
 *** its list of uids if it has
 ***/
int
pop_internal_UpdateMaildrop(pPopMaildrop drop)
    {
    char* passwd;
    char* tok, * size_str, * line;
    char* uid;
    int i, maildrop_size, num_messages;    /** size of the maildrop, and the number of messages **/
    char buf[80];
    pLxSession s;


	/** Make sure we actually have a connection **/
	if (drop->Session == NULL) return -1;
    
	/** mind your concurrency issues **/
	syGetSem(drop->Sem, 1, 0);

	/** Stat the server to find out if we need to update **/
	snprintf(buf, 32, "STAT\r\n");
	fdWrite(drop->Session, buf, strlen(buf), 0, FD_U_PACKET);

	if ( pop_internal_GetResponse(drop->Session, buf, 32) < 1)
	    {
	    mssError(1, "POP", "Could not STAT %s: %s", drop->DropName, buf);
	    goto piUM_Error;
	    }

	if ( (tok = strtok(buf, " ")) == NULL || (tok = strtok(NULL, " ")) == NULL || 
	     (size_str = strtok(NULL, " ")) == NULL)
	    {
	    mssError(1, "POP", "Incorrectly formatted response to STAT");
	    goto piUM_Error;
	    }
	num_messages = atoi(tok);
	maildrop_size = atol(size_str);

	if (drop->Size != maildrop_size || xaCount(drop->uids) != num_messages)
	/** Maildrop has changed, gotta pull the suids again **/
	    {
	    /** first clear the thing out **/
	    for (i=0;i<xaCount(drop->uids);i++)
		{
		if ( (uid = xaGetItem(drop->uids, i)) != NULL) nmSysFree (uid);
		}
	    xaClear(drop->uids);

	    /** Tell the server we want the uids **/
	    fdWrite(drop->Session, "UIDL\r\n", 6, 0, FD_U_PACKET);
	    if (pop_internal_GetResponse(drop->Session, buf, 80) < 0)
		{
		mssError(1, "POP", "Error getting UIDs from server: %s", buf);
		goto piUM_Error;
		}
    
	    /** Open a lexer session to read stuff in **/
	    if ( (s  = mlxOpenSession(drop->Session, MLX_F_LINEONLY | MLX_F_NODISCARD)) == NULL)
		{
		mssError(1, "POP", "Couldn't open Lexer Session");
		goto piUM_Error;
		}

	    for (i=0;i<num_messages;i++)
		{
		    if (mlxNextToken(s) == MLX_TOK_ERROR) goto piUM_Error;
		    line = mlxStringVal(s, NULL);
		    if (strtok(line, " \n\r") == NULL || (uid = strtok(NULL, " \r\n")) == NULL)
			{
			mssError(1, "POP", "Invalid response from server");
			goto piUM_Error;
			}
		    xaAddItem(drop->uids, nmSysStrdup(uid));
		}

	    /** read in that last line **/
	    mlxNextToken(s);
	    mlxCloseSession(s);	
	    drop->Size = maildrop_size;
	    
	    }   /** end if maildrop has changed **/

	syPostSem(drop->Sem, 1, 0);
	return 0;
    piUM_Error:
	syPostSem(drop->Sem, 1, 0);
	return -1;
    }


/*** pop_internal_ConnectToMaildrop - establish a connection with a maildrop
 ***/
int
pop_internal_ConnectToMaildrop(pPopData inf)
    {
	pPopMaildrop drop = inf->Drop;
	char buf[32];
	char *passwd;

	/** make sure the caller isn't an idiot **/
	if (drop == NULL || (pop_internal_GetType(inf) == POP_T_SERVER)) 
	    {
	    mssError(1, "POP", "Called ConnectToMaildrop on a server object!");
	    return -1;
	    }

	/** Are we connected? **/
	if (drop->Session == NULL)
	    {
	    /** Snag a semaphore count so we don't run into concurrency issues **/
	    if (syGetSem(drop->Sem, 1, 0) < 0) return -1;

	    /** Now we need to open the connection **/
	    if ( (drop->Session = netConnectTCP(inf->Host, inf->Port, 0)) == NULL)
		{
		mssError(1, "POP", "Can't connect to '%s:%s'.", inf->Host, inf->Port);
		goto piCTM_Error;
		}

	    /** Make sure the response was good **/
	    if (pop_internal_GetResponse(drop->Session, buf, 32) < 1)
		{
		mssError(1, "POP", "Error connecting to %s:%s - %s", inf->Host, inf->Port, buf);
		goto piCTM_Error;
		}

	    /****** Trying to log in *******/
	    /** Figure out which password to use **/
	    switch (inf->AuthMethod)
		{
		case POP_AUTH_PROMPT:
		    mssError(1, "POP", "Prompting for password not yet implemented");
		    goto piCTM_Error;
		case POP_AUTH_CURRLOGIN:
		    passwd = mssPassword();
		    break;
		}

	    /** Send the user command **/
	    snprintf(buf, 32, "USER %s\r\n", drop->DropName);
	    fdWrite(drop->Session, buf, strlen(buf), 0, FD_U_PACKET);
	    
	    /** Check the response **/
	    if ( pop_internal_GetResponse(drop->Session, buf, 32) < 1)
		{
		mssError(1, "POP", "Could not log in to %s:  %s", drop->DropName, buf);
		goto piCTM_Error;
		}

	    /** Send the password command **/
	    snprintf(buf, 32, "PASS %s\r\n", passwd);
	    fdWrite(drop->Session, buf, strlen(buf), 0, FD_U_PACKET);

	    /** Check the response **/
	    if ( pop_internal_GetResponse(drop->Session, buf, 32) < 1)
		{
		mssError(1, "POP", "Could not log in to %s: %s", drop->DropName, buf);
		goto piCTM_Error;
		}

	    syPostSem(drop->Sem, 1, 0);
	    }	/** end if connected **/

	return 0;
    piCTM_Error:
	syPostSem(drop->Sem, 1, 0);
	return -1;
    }


/*** pop_internal_GetMaildrop - see if the given maildrop is open, or open it
 *** path must be the absolute path to the maildrop, ensuring that it is unique.
 ***/
int
pop_internal_GetMaildrop(pPopData inf)
    {
    pPopMaildrop drop;
    char* dropname;
    char* path;

	/** Make sure this is isn't a server object, and get some info **/
	obj_internal_PathPart(inf->Obj->Pathname, 0, 0);
	switch (pop_internal_GetType(inf))
	    {
	    case POP_T_SERVER: return -1;
	    case POP_T_MAILDROP:
		path = nmSysStrdup(inf->Obj->Pathname->Pathbuf);
		dropname = nmSysStrdup(inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1]);
		break;
	    case POP_T_MSG:
		path = nmSysStrdup(obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->Pathname->nElements-1));
		dropname = nmSysStrdup(obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-2, 1));
		obj_internal_PathPart(inf->Obj->Pathname, 0, 0);
		break;
	    }

	/** Has it ever been opened before? **/
	if ( (drop = (pPopMaildrop)xhLookup(&POP_INF.OpenMaildrops, path)) == NULL)
	    /****** Open a new maildrop ******/
	    {
	    /** Get the memory **/
	    drop = (pPopMaildrop)nmMalloc(sizeof(PopMaildrop));
	    memset(drop, 0, sizeof(PopMaildrop));
	    
	    /** First thing's first - We need to create the semaphore and stick 
	     ** this guy into the global hash NOW, so we don't have to worry
	     ** about a context switch, and some other thread opening the same
	     ** maildrop at the same time. That would be bad.
	     **/
	    drop->Sem = syCreateSem(0, 0);	/** Create with no counts, so nothing can use it **/
	    xhAdd(&POP_INF.OpenMaildrops, path, (char*)drop);

	    /** Now we're safe from context switches - allocate the uid array **/
	    drop->uids = (pXArray)nmMalloc(sizeof(XArray));
	    xaInit(drop->uids, 32);
	    drop->LinkCnt = 0;

	    /** Save the username and crypt()'d password of the centrallix user opening this **/
	    drop->UserName = nmSysStrdup(mssUserName());
	    drop->CryptPasswd = nmSysStrdup((const char*)crypt(mssPassword(), "fo"));

	    /** Save the name of the drop **/
	    drop->DropName = dropname;

	    /** Set this to something impossible, so that the UIDs will be 
	     ** retrieved later in the function
	     **/
	    drop->Size = -1;

	    /** Post a semaphore count, we might grab it again updating the maildrop if needed **/
	    syPostSem(drop->Sem, 1, 0);
	    }   /** end if not open **/
	
	drop->LinkCnt++;
	inf->Drop = drop;

	/** Make sure a connection is established **/
	if (inf->Drop->Session == NULL)
	    {
	    if (pop_internal_ConnectToMaildrop(inf) < 0)
		{
		mssError(1, "POP", "Couldn't establish connection to maildrop");
		pop_internal_ReleaseMaildrop(inf);
		return -1;
		}

	    /** Update the maildrop, if necessary **/
	    if (pop_internal_UpdateMaildrop(inf->Drop) < 0)
		{
		pop_internal_ReleaseMaildrop(inf);
		return -1;
		}
	    }

	return 0;

    }



/*** pop_internal_Create - creates a new object
 *** POP server objects can be created, but mail messages cannot (go write an SMPT driver).
 *** Receives a pointer to a PopData object that's been allocated and init'd by popOpen.
 *** FIXME: creating a new server hasn't yet been implemented
 ***/
void*
pop_internal_Create(pPopData inf, char* usrtype)
    {
	nmFree(inf, sizeof(PopData));
	mssError(1, "POP", "FIXME: Creating POP server objects not yet implemented!");
	return NULL;
    }


/*** pop_internal_SetParam - attempt to look up 'param_name' in node, make
 *** a copy of the resulting value, and return it. Up to the caller to free
 *** returned pointer.
 ***/
char*
pop_internal_GetParam(pSnNode node, char* param_name)
    {
    char *ptr, *ptr2;
    
	if (stAttrValue(stLookup(node->Data, param_name), NULL, &ptr, 0) < 0)
	    {
	    return NULL;
	    }
	if ( (ptr2 = nmSysStrdup(ptr)) == NULL)
	    {
	    mssError(1, "POP", "Couldn't get memory for param!");
	    return NULL;
	    }
	return ptr2;
    }


#if 0
/*** pop_internal_Login - authenticate to the server. Only makes sense for
 *** account and msg objects.
 *** FIXME FIXME FIXME Need to add support for AUTH LOGIN!!!! FIXME FIXME FIXME
 ***/
int
pop_internal_Login(pPopData inf)
    {
    char* username;
    char buf[64];

	/** Send Username **/
	username = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements-1, 1);
	snprintf(buf, 32, "USER %s\r\n", username);
	fdWrite(inf->Conn, buf, strlen(buf), 0, FD_U_PACKET);

	/** Get response **/
	if (pop_internal_GetResponse(inf, buf, 64) != 1) 
	    {
	    mssError(1, "POP", "Could not authenticate to server: %s", buf);
	    return -1;
	    }
	
	/** Now authenticate **/
	snprintf(buf, 256, "PASS %s\r\n", mssPassword());
	fdWrite(inf->Conn, buf, strlen(buf), 0, FD_U_PACKET);

	/** Get the response **/
	if (pop_internal_GetResponse(inf, buf, 64) != 1)
	    {
	    mssError(1, "POP", "Could not authenticate to server: %s", buf);
	    return -1;
	    }
	
	return 0;
    }
#endif

/*** pop_internal_Cleanup - totally free up all the memory in a pPopData struct
 ***/
void pop_internal_Cleanup(pPopData inf)
    {
    int i;
    char* item;

	/** first the names **/
/*
	for (i=0;i<xaCount(inf->AttribNames); i++)
	    {
	    if ( (item = xaGetItem(inf->AttribNames, i)) != NULL) free(item);
	    }
*/	
	/** then the values **/
	for (i=0;i<xaCount(inf->AttribVals); i++)
	    {
	    if ( (item = xaGetItem(inf->AttribVals, i)) != NULL) nmSysFree(item);
	    }
	
	if (inf->EnumList) nmSysFree(inf->EnumList);
	if (inf->EnumFilename) nmSysFree(inf->EnumFilename);

	/** then the server-specific stuff **/
	
	/** Then the arrays themselves **/
	xaDeInit(inf->AttribNames);
	xaDeInit(inf->AttribVals);
	nmFree(inf->AttribNames, sizeof(XArray));
	nmFree(inf->AttribVals, sizeof(XArray));

	/** Finally the memory for the struct itself **/
	nmFree(inf, sizeof(PopData));
	
	return;
    }


/*** pop_internal_OpenMaildrop - stuff that's specific to opening a server
 ***/
int
pop_internal_OpenMaildrop(pPopData inf, char* usrtype)
    {
    char* dropname;
    char* username;
    char buf[32];

	/** Check to make sure this is even a valid maildrop, based on the enumeration type **/
	switch (inf->MaildropEnumMethod)
	    {
	    case POP_ENUM_CURRLOGIN:
		dropname = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
		if ( (username = mssUserName()) == NULL)
		    {
		    mssError(1, "POP", "User did not authenticate");
		    return -1;
		    }
		if (strcmp(username, dropname))
		    {
		    mssError(1, "POP", "'%s' not found in %s", dropname, inf->Obj->Pathname->Pathbuf);
		    return -1;
		    }
		break;
	    case POP_ENUM_FILE:
		mssError(1, "POP", "Can't open maildrops with 'file' enumeration type yet");
		return -1;
	    case POP_ENUM_LIST:
		mssError(1, "POP", "Can't open maildrops with 'list' enumeration type yet");
		return -1;
	    }
	
	/** Get our Maildrop object **/
	if (pop_internal_GetMaildrop(inf) < 0) return -1;

	/***** Get any attributes that are specific to us ******/

	/** num_messages **/
	xaAddItem(inf->AttribNames, "num_messages");
	snprintf(buf, 32, "%d", xaCount(inf->Drop->uids));
	xaAddItem(inf->AttribVals, nmSysStrdup(buf));

	/** size **/
	xaAddItem(inf->AttribNames, "size");
	snprintf(buf, 32, "%ld", inf->Drop->Size);
	xaAddItem(inf->AttribVals, nmSysStrdup(buf));

	return 0;
    }


/*** pop_internal_OpenServer - stuff that's specific to opening a server
 ***/
int
pop_internal_OpenServer(pPopData inf, char* usrtype)
    {
    char*   attribval;
    char*   maildrop;
    pFile   conn;

	/***** Fill out all our attributes *****/

	/** maildrop_enum_method **/
	xaAddItem(inf->AttribNames, "maildrop_enum_method");
	switch(inf->MaildropEnumMethod)
	    {
	    case POP_ENUM_CURRLOGIN: xaAddItem(inf->AttribVals, nmSysStrdup("currlogin")); break;
	    case POP_ENUM_FILE:	xaAddItem(inf->AttribVals, nmSysStrdup("file")); break;
	    case POP_ENUM_LIST:	xaAddItem(inf->AttribVals, nmSysStrdup("list")); break;
	    }

	/** auth_method **/
	xaAddItem(inf->AttribNames, "auth_method");
	switch (inf->AuthMethod)
	    {
	    case POP_AUTH_PROMPT: xaAddItem(inf->AttribVals, nmSysStrdup("prompt")); break;
	    case POP_AUTH_CURRLOGIN: xaAddItem(inf->AttribVals, nmSysStrdup("currlogin")); break;
	    }
	
	return 0;
    }

/*** pop_internal_OpenMessage
 ***/
int pop_internal_OpenMessage(pPopData inf, char* usrtype)
    {
    int i;
    char* uid;
    int id, alloc, bytes_read, bytes_to_read;
    long size;
    char buf[64];
    char* tok, * line, *start;
    pLxSession s;


	/** Get a pointer to the maildrop (opening it if necessary), check
	 ** to make sure this is a legit UID, and store some values we'll need later
	 **/
	if (pop_internal_GetMaildrop(inf) < 0) return -1;
	tok = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	i=0;

	for (i=0;i<xaCount(inf->Drop->uids);i++)
	    {
	    uid = xaGetItem(inf->Drop->uids, i);
	    if (uid != NULL && !strcmp(tok, uid)) break;
	    }

	if (i == xaCount(inf->Drop->uids))
	    {
	    mssError(1, "POP", "'%s' does not contain the message '%s'", inf->Drop->DropName, tok);
	    pop_internal_ReleaseMaildrop(inf);
	    return -1;
	    }
	i++;
	    
	inf->MsgUID = xaGetItem(inf->Drop->uids, i);
	inf->MsgNum = i;
	inf->Pos =0;

	/** Now read the contents of the e-mail **/
	if (pop_internal_GetType(inf) != POP_T_MSG) return -1;

	/** How big is this message? */
	fdPrintf(inf->Drop->Session, "LIST %d\r\n", inf->MsgNum);
	if (pop_internal_GetResponse(inf->Drop->Session, buf, 64) < 1)
	    {
	    mssError(1, "POP", "LIST command failed: %s", buf);
	    return -1;
	    }
	if (strtok(buf, " \r\n") == NULL || strtok(NULL, " \r\n") == NULL || (tok = strtok(NULL, " \r\n")) == NULL)
	    {
	    mssError(1, "POP", "Invalid output from LIST command");
	    return -1;
	    }
	size = atol(tok);
	inf->ContentSize = size;

	/** Allocate some memory, and retrieve the e-mail **/
	if ( (inf->Content = (char*)nmSysMalloc(size+1)) == NULL) return -1;
	memset(inf->Content, 0, size+1);
	fdPrintf(inf->Drop->Session, "RETR %d\r\n", inf->MsgNum);

	if (pop_internal_GetResponse(inf->Drop->Session, buf, 64) < 1)
	    {
	    mssError(1, "POP", "Could not read message: $s", buf);
	    return -1;
	    }
	
	/** parse the mail - the termination character is .\r\n, and if a line
	 ** that's part of the message starts with a '.', it's byte-stuffed with
	 ** another one. We need to strip those extras off if need-be
	 **/
	if ( (s = mlxOpenSession(inf->Drop->Session, MLX_F_LINEONLY | MLX_F_NODISCARD)) == NULL)
	    {
	    mssError(1, "POP", "popRead() - Couldn't start a lexer session");
	    nmSysFree(inf->Content);
	    return -1;
	    }

	alloc = 1;
	mlxNextToken(s);
	if ( (line = mlxStringVal(s, &alloc)) == NULL)
	    {
	    mssError(0, "POP", "Error reading message content");
	    nmSysFree(inf->Content);
	    return -1;
	    }
	bytes_read = 0;
	while (strcmp(line, ".\r\n"))
	    {
	    if (line[0] == '.')
		{
		memcpy(inf->Content+bytes_read, line+1, strlen(line+1));
		bytes_read += strlen(line+1);
		}
	    else
		{
		memcpy(inf->Content+bytes_read, line, strlen(line));
		bytes_read += strlen(line);
		}
	    mlxNextToken(s);
	    if ( (line = mlxStringVal(s, &alloc)) == NULL)
		{
		mssError(1, "POP", "Error reading message content");
		nmSysFree(inf->Content);
		return -1;
		}
	    }
	return 0;

    }


/*** pop_internal_Open - open an object
 *** Opening an object should result in an open connection to the server. Opening an account or
 *** msg object should result in an open connection, with the user already authenticated to the server
 *** ready to execute commands.
 ***/
int
pop_internal_Open(pPopData inf, char* usrtype)
    {
    char buf[32];
    char* tok, * attribval, * uid;
    int bytes_read, i;

	/******** Init Variables everyone uses *********/

	/** Open the node **/
	if ( !(inf->Node = snReadNode(inf->Obj->Prev)) )
	    {
	    mssError(0, "POP", "Could not open structure file!");
	    goto piO_Cleanup;
	    }
	inf->Node->OpenCnt++;
	
	/** Decide how much of the path is ours.
	 **
	 ** At our top level is a pop server object. That object can
	 ** contain user account objects. Each of those can contain
	 ** pop e-mail message objects. That's as far as we can go.
	 ** If there are more than 3 elements after the node, we take
	 ** three, otherwise we take what's left. 
	 **/
	if (inf->Obj->Pathname->nElements - inf->Obj->SubPtr + 1 > 3)
	    inf->Obj->SubCnt = 3;
	else
	    inf->Obj->SubCnt = inf->Obj->Pathname->nElements - inf->Obj->SubPtr + 1;
	
	/** Init arrays for attributes **/
	inf->AttribNames = (pXArray)nmMalloc(sizeof(XArray));
	inf->AttribVals = (pXArray)nmMalloc(sizeof(XArray));
	if (inf->AttribNames == NULL || inf->AttribVals == NULL) goto piO_Cleanup;
	xaInit(inf->AttribNames, 16);
	xaInit(inf->AttribVals, 16);

	/** Snag some attributes that all our objects share **/
	/** host **/
	xaAddItem(inf->AttribNames, "host");
	if ( (attribval = pop_internal_GetParam(inf->Node, "host")) == NULL)
	    {
	    mssError(1, "POP", "'host' not defined in %s!", inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1]);
	    return -1;
	    }
	inf->Host = nmSysStrdup(attribval);
	xaAddItem(inf->AttribVals, inf->Host);
	
	/** port **/
	xaAddItem(inf->AttribNames, "port");
	if ( (attribval = pop_internal_GetParam(inf->Node, "port")) == NULL) attribval = "110";
	inf->Port = nmSysStrdup(attribval);
	xaAddItem(inf->AttribVals, inf->Port);

	/** auth_method - maildrops/messages need to know this for opening maildrops **/
	if ( (attribval = pop_internal_GetParam(inf->Node, "auth_method")) == NULL ||
	     (strcmp(attribval, "currlogin") && strcmp(attribval, "prompt")) ) attribval = "currlogin";

	if (!strcmp(attribval, "prompt")) inf->AuthMethod = POP_AUTH_PROMPT;
	else inf->AuthMethod = POP_AUTH_CURRLOGIN;

	/** maildrop_enum_method **/
	if ( ((attribval = pop_internal_GetParam(inf->Node, "maildrop_enum_method")) == NULL)|| 
	     !strcmp(attribval, "currlogin")) inf->MaildropEnumMethod = POP_ENUM_CURRLOGIN;
	else if (!strcmp(attribval, "file")) inf->MaildropEnumMethod = POP_ENUM_FILE;
	else if (!strcmp(attribval, "list")) inf->MaildropEnumMethod = POP_ENUM_LIST;
	else inf->MaildropEnumMethod = POP_ENUM_CURRLOGIN;

	/** maildrop_list **/
	if (inf->MaildropEnumMethod == POP_ENUM_LIST )
	    {
	    if ( (inf->EnumList = pop_internal_GetParam(inf->Node, "maildrop_list")) == NULL)
		{
		mssError(1, "POP", "specified 'list' as Maildrop Enumeration Method, but did not set 'maildrop_list'");
		return -1;
		}
	    }

	/** maildrop_file **/
	else if (!strcmp(attribval, "file"))
	    {
	    if ( (inf->EnumFilename = pop_internal_GetParam(inf->Node, "maildrop_file")) == NULL)
		{
		mssError(1, "POP", "specified 'file' as Maildrop Enumeration Method, but did not set 'maildrop_file'");
		return -1;
		}
	    }

	/********** Do tasks specific to each object on open ********/
	switch (pop_internal_GetType(inf))
	    {
	    case POP_T_SERVER:
		if (pop_internal_OpenServer(inf, usrtype) < 0) goto piO_Cleanup;
		break;
	    case POP_T_MAILDROP:
		if (pop_internal_OpenMaildrop(inf, usrtype) < 0) goto piO_Cleanup;
		break;
	    case POP_T_MSG:
		if (pop_internal_OpenMessage(inf, usrtype) < 0) goto piO_Cleanup;
		break;
	    }
	return 0;

piO_Cleanup:
	return -1;
    }

/*** popOpen - open an object.
 *** This function decides weather we need to open or create an object, and
 *** calls the appropriate function.
 ***/
void*
popOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pPopData inf;


	/** Allocate the structure **/
	inf = (pPopData)nmMalloc(sizeof(PopData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(PopData));
	inf->Obj = obj;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));

	/** Are we creating, or opening an existing object? **/
	if (inf->Obj->Mode & O_CREAT)
	    {
	    if (pop_internal_Create(inf, usrtype) < 0) goto pO_Cleanup;
	    }
	else
	    {
	    if (pop_internal_Open(inf, usrtype) < 0) goto pO_Cleanup;
	    }
	
	return inf;

pO_Cleanup:
    pop_internal_Cleanup(inf);
    return NULL;
    }


/*** popClose - close an open object.
 ***/
int
popClose(void* inf_v, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);

    	/** Write the node first, if need be. **/
	/** XXX Do we need this? **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	if (pop_internal_GetType(inf) == POP_T_MAILDROP || pop_internal_GetType(inf) == POP_T_MSG)
	    {
		if (inf->Content != NULL) nmSysFree(inf->Content);
		pop_internal_ReleaseMaildrop(inf);
	    }	
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	pop_internal_Cleanup(inf);

    return 0;
    }


/*** popCreate - create a new object, without actually returning a
 *** descriptor for it. The only object it would make any sense allowing
 *** the creation of is a server object, where we would create a new
 *** node. But we'd have to give it a dummy server name and the user
 *** would then have to be sure to set it as an attribute.
 ***/
int
popCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = popOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	popClose(inf, oxt);

    return 0;
    }


/*** popDelete - delete an existing object. 
 *** This makes very good sense for e-mails, no sense at all for accounts,
 *** and I suppose it could be done for servers (by deleting the structure file),
 *** but do we want to be able to do that?
 *** FIXME FIXME FIXME FIXME FIXME ***/
int
popDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pPopData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

	/** Haven't thought out the deletion process just yet - we'll defer this until
	 ** later.
	 **/

	mssError(1, "POP", "FIXME: Deletion of POP objects hasn't been implemented yet!!!");
	return -1;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pPopData)popOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

#if 0
	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		popClose(inf, oxt);
		mssError(1,"POP","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		popClose(inf, oxt);
		mssError(1,"POP","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(PopData));
#endif
    switch (pop_internal_GetType(inf))
	{
	case POP_T_SERVER:
	    popClose(inf, oxt);
	    mssError(1, "POP", "Can't delete server objects (yet)");
	    return -1;
	case POP_T_MAILDROP:
	    popClose(inf, oxt);
	    mssError(1, "POP", "Can't delet POP3 account objects");
	    return -1;
	case POP_T_MSG:
	    /** FIXME FIXME FILL ME IN FIXME FIXME **/
	    popClose(inf, oxt);
	    mssError(1, "POP", "Haven't implemented deleting mail messages just yet");
	    return -1;
	}

    return 0;
    }


/*** popRead - Structure elements have no content.  Fails.
 ***/
int
popRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    int id, alloc, bytes_read, bytes_to_read;
    long size;
    char buf[64];
    char* tok, * line, *start;
    pLxSession s;

	/** update the position to reflect the seek offset **/
	if (flags & FD_U_SEEK) inf->Pos = offset;

	/** is there anything else to read? **/
	if (inf->Pos == inf->ContentSize) return 0; 
	
	/** is this even a valid read? **/
	if (inf->Pos < 0 || inf->Pos >= inf->ContentSize) return -1;

	/** Now worry about transferring stuff to the caller's buffer **/
	start = inf->Content + inf->Pos;

	/** figure out how many bytes to read **/
	if (inf->Pos + maxcnt > inf->ContentSize) bytes_to_read = inf->ContentSize - inf->Pos;
	else bytes_to_read = maxcnt;

	/** copy them over, and update the position **/
	memcpy(buffer, start, bytes_to_read);
	inf->Pos += bytes_to_read;

	return bytes_to_read;

    }


/*** popWrite - Again, no content.  This fails.
 ***/
int
popWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    return -1;
    }


/*** popOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
popOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    pPopQuery qy;
    char* maildrop, * uid;
    pLxSession s;
    int i, tok;
    pFile f;

	/** Allocate the query structure **/
	qy = (pPopQuery)nmMalloc(sizeof(PopQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(PopQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	qy->Maildrops = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(qy->Maildrops, 16);

	switch (pop_internal_GetType(inf))
	    {
	    case POP_T_SERVER:
		/********* Now get the list of maildrops, based on the selected method ********/
		switch (inf->MaildropEnumMethod)
		    {
		    case POP_ENUM_CURRLOGIN:
			/** This is easy - add login name to list of maildrops **/
			if ( (maildrop = mssUserName()) == NULL)
			    {
			    mssError(1, "POP", "You have not authenticated with Centrallix");
			    goto pOQ_Cleanup;
			    }
			xaAddItem(qy->Maildrops, nmSysStrdup(maildrop));
			break;
		    case POP_ENUM_LIST:
			mssError(1, "POP", "Maildrop Enumeration Method 'list' not yet implemented.");
			goto pOQ_Cleanup;
			break;
		    case POP_ENUM_FILE:
			mssError(1, "POP", "Maildrop Enumeration Method 'file' not yet implemented.");
			goto pOQ_Cleanup;
			break;
		    }
		break;
	    case POP_T_MAILDROP:
		/** We don't have to do squat here, QueryFetch will handle it **/
		
		break;
	    case POP_T_MSG:
		mssError(1, "POP", "Queries against mail should be handled by the RFC822 driver");
		goto pOQ_Cleanup;
		break;
	    }

    
	return (void*)qy;

    pOQ_Cleanup:
	if (qy->Maildrops != NULL)
	    {
	    xaDeInit(qy->Maildrops);
	    nmFree(qy->Maildrops, sizeof(XArray));
	    }
	nmFree(qy, sizeof(PopQuery));
	return NULL;
    }



/*** popQueryFetch - get the next directory entry as an open object.
 ***/
void*
popQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pPopQuery qy = ((pPopQuery)(qy_v));
    pPopData inf;
    char* uid;

	/** Give the object the correct path info **/
	obj->SubPtr = qy->Data->Obj->SubPtr;

	/** What type of object are we running a query on? **/
	switch (pop_internal_GetType(qy->Data))
	    {
	    case POP_T_SERVER:
		/** Make sure we haven't enumerated them all yet **/
		if (qy->ItemCnt >= xaCount(qy->Maildrops)) return NULL;
		
		/** Make sure we can actually set the path **/
		obj->SubCnt = qy->Data->Obj->SubCnt+1;
		if (obj_internal_AddToPath(obj->Pathname, xaGetItem(qy->Maildrops, qy->ItemCnt)) < 0) return NULL;
		qy->ItemCnt++;
		break;
	    case POP_T_MAILDROP:
		uid = NULL;
		while (uid == NULL && qy->ItemCnt < xaCount(qy->Data->Drop->uids))
		    uid = xaGetItem(qy->Data->Drop->uids, qy->ItemCnt++);

		if (uid == NULL) return NULL;

		obj->SubCnt = qy->Data->Obj->SubCnt+1;
		if (obj_internal_AddToPath(obj->Pathname, uid) < 0) return NULL;
		break;
	    case POP_T_MSG:
		mssError(1, "POP", "Queries on Messages should be handled by rfc822 driver!");
		return NULL;
	    }
	
	/** Need to open the object the way all the objects are open - there's a bunch
	 ** of stuff to do
	 **/
	if ( (inf = popOpen(obj, 0, NULL, NULL, oxt)) == NULL)
	    {
	    mssError(0, "POP", "Couldn't open object %s", obj->Pathname->Pathbuf);
	    return NULL;
	    }


    return (void*)inf;
    }


/*** popQueryClose - close the query.
 ***/
int
popQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pPopQuery qy = ((pPopQuery)(qy_v));
    int i;
    char* item;

	if (qy->Maildrops != NULL)
	    {
	    for (i=0;i<xaCount(qy->Maildrops);i++)
		{
		if ( (item = xaGetItem(qy->Maildrops, i)) != NULL) nmSysFree(item);
		}
	    }
	xaDeInit(qy->Maildrops);
	nmFree(qy->Maildrops, sizeof(XArray));
	nmFree(qy,sizeof(PopQuery));

    return 0;
    }


/*** popGetAttrType - get the type (DATA_T_pop) of an attribute by name.
 ***/
int
popGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    int i;
    pStructInf find_inf;

    	/** Defaults are all strings **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;
	else if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	else if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	else if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	else if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/
	else if (xaFindItem(inf->AttribNames, attrname) != -1) return DATA_T_STRING;


    return -1;
    }


/*** popGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
popGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pPopData inf = POP(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	else if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type"))
	    {
	    if (pop_internal_GetType(inf) == POP_T_MSG)
		val->String = "message/rfc822";
	    else
		val->String = "system/void";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/

	/** If annotation, and not found, return "" **/
	else if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	/** outer_type **/
	else if (!strcmp(attrname, "outer_type"))
	    {
	    switch (pop_internal_GetType(inf))
		{
		case POP_T_SERVER:
		    val->String = "network/pop3";
		    break;
		case POP_T_MAILDROP:
		    val->String = "network/pop3maildrop";
		    break;
		case POP_T_MSG:
		    val->String = "network/pop3message";
		    break;
		}
		return 0;
	    }
	else if ( (i = xaFindItem(inf->AttribNames, attrname)) != -1)
	    {
	    val->String = xaGetItem(inf->AttribVals, i);
	    return 0;
	    }
	
	mssError(1,"POP","Could not locate requested attribute: %s", attrname);

    return -1;
    }


/*** popGetNextAttr - get the next attribute name for this object.
 ***/
char*
popGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);

	if (inf->CurAttr == xaCount(inf->AttribNames)) return NULL;

	return xaGetItem(inf->AttribNames, inf->CurAttr++);
    }


/*** popGetFirstAttr - get the first attribute name for this object.
 ***/
char*
popGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = popGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** popSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
popSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"POP","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"POP","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		/** Set dirty flag **/
		inf->Node->Status = SN_NS_DIRTY;
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	else if (!strcmp(attrname,"content_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }
	else
	    {
	    mssError(0, "POP", "Object does not have attribute %s.\n", attrname);
	    return -1;
	    }


    return 0;
    }


/*** popAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
popAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pPopData inf = POP(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** popOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
popOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/ char*
popGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popGetNextMethod -- same as above.  Always fails. 
 ***/
char*
popGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** popExecuteMethod - No methods to execute, so this fails.
 ***/
int
popExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** popInfo - Return the capabilities of the object
 ***/
int
popInfo(void* inf_v, pObjectInfo info)
    {
    pPopData inf = POP(inf_v);

	/** Base capabilities **/
	switch (pop_internal_GetType(inf))
	    {
	    case POP_T_SERVER:
		info->Flags = (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
			OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
			OBJ_INFO_F_NO_CONTENT);
		break;
	    case POP_T_MAILDROP:
		info->Flags = (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
			OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
			OBJ_INFO_F_NO_CONTENT);
		break;
	    case POP_T_MSG:
		info->Flags = (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
			OBJ_INFO_F_CAN_SEEK_REWIND | OBJ_INFO_F_CAN_SEEK_FULL | OBJ_INFO_F_CAN_HAVE_CONTENT);
		break;
	    }
		
	
	return 0;
    }

/*** popInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
popInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&POP_INF,0,sizeof(POP_INF));
	xhInit(&POP_INF.OpenMaildrops, 13, 0);

	/** Setup the structure **/
	strcpy(drv->Name,"POP - POP3 mailbox driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"network/pop3");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = popOpen;
	drv->Close = popClose;
	drv->Create = popCreate;
	drv->Delete = popDelete;
	drv->OpenQuery = popOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = popQueryFetch;
	drv->QueryClose = popQueryClose;
	drv->Read = popRead;
	drv->Write = popWrite;
	drv->GetAttrType = popGetAttrType;
	drv->GetAttrValue = popGetAttrValue;
	drv->GetFirstAttr = popGetFirstAttr;
	drv->GetNextAttr = popGetNextAttr;
	drv->SetAttrValue = popSetAttrValue;
	drv->AddAttr = popAddAttr;
	drv->OpenAttr = popOpenAttr;
	drv->GetFirstMethod = popGetFirstMethod;
	drv->GetNextMethod = popGetNextMethod;
	drv->ExecuteMethod = popExecuteMethod;
	drv->Info = popInfo;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(PopData),"PopData");
	nmRegister(sizeof(PopQuery),"PopQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(popInitialize);
MODULE_PREFIX("pop");
MODULE_DESC("POP ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);

