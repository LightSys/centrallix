#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"
/** module definintions **/
#include "centrallix.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

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
/* Module: 	Shell osdriver						*/
/* Author:	Jonathan Rupp	    					*/
/* Creation:	November 08, 2002					*/
/* Description:	A driver to provide shell access			*/
/*									*/
/*									*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_shell.c,v 1.6 2002/11/22 22:09:42 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_shell.c,v $

 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    char	sCurAttr[10];
    pSnNode	Node;
    char	program[80];
    char**	args;
    int		nArgs;
    pFile	shell_fd;
    int		curRead;
    int		curWrite;
    pid_t	shell_pid;
    }
    ShlData, *pShlData;


#define SHL(x) ((pShlData)(x))

#define SHELL_DEBUG 0x00
#define SHELL_DEBUG_PARAM 0x01
#define SHELL_DEBUG_FORK 0x02
#define SHELL_DEBUG_EXEC 0x04
#define SHELL_DEBUG_OPEN 0x08
#define SHELL_DEBUG_INIT 0x10
    
/*** shlOpen - open an object.
 ***/
void*
shlOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pShlData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
    char tty_name[32];
    int pty;
    pStructInf argStruct;
    char **args;

	/** Allocate the structure **/
	inf = (pShlData)nmMalloc(sizeof(ShlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(ShlData));
	inf->Obj = obj;
	inf->Mask = mask;

	obj->SubCnt=1;
	if(SHELL_DEBUG & SHELL_DEBUG_INIT) printf("%s was offered: (%i,%i,%i) %s\n",__FILE__,obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("opening: %s\n",node_path);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
		{
		nmFree(inf,sizeof(ShlData));
		mssError(0,"SHL","Could not create new node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    nmFree(inf,sizeof(ShlData));
	    mssError(0,"SHL","Could not open structure file");
	    return NULL;
	    }

	    

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** figure out command to run **/
	if(stAttrValue(stLookup(node->Data,"program"),NULL,&ptr,0)<0) ptr="";
	strncpy(inf->program,ptr,79);
	inf->program[79]='\0';
	if(SHELL_DEBUG & SHELL_DEBUG_PARAM) printf("program: %s\n",inf->program);	
	
	argStruct=stLookup(node->Data,"arg");
	
	if(!argStruct)
	    {
	    /** no 'arg' value present -- no parameters **/
	    inf->nArgs=0;
	    inf->args=(char**)nmMalloc((inf->nArgs+1)*sizeof(char*));
	    inf->args[0]=NULL;
	    }
	else
	    {
	    if(argStruct->Value->NodeType == EXPR_N_LIST)
		{
		/** 'arg' value is present and is a list -- parameters > 1 **/
		int i;
		char *ptr;
		pXArray values;
		values = &argStruct->Value->Children;
		inf->nArgs=values->nItems;
		inf->args=(char**)nmMalloc((inf->nArgs+1)*sizeof(char*));
		memset(inf->args,0,(inf->nArgs+1)*sizeof(char*));
		for(i=0;i<values->nItems;i++)
		    inf->args[i]=((pExpression)values->Items[i])->String;
		inf->args[values->nItems]=NULL;
		}
	    else
		{
		/** 'arg' value is present and is exactly one value -- 1 parameter **/
		inf->nArgs=1;
		inf->args=(char**)nmMalloc((inf->nArgs+1)*sizeof(char*));
		memset(inf->args,0,(inf->nArgs+1)*sizeof(char*));
		if(stAttrValue(argStruct,NULL,&inf->args[0],0)<0)
		    inf->args[0]=NULL;
		}
	    }
	
	pty=getpt();
	if(pty<0)
	    {
	    mssError(0,"SHL","getpy() failed");
	    shlClose(inf);
	    return NULL;
	    }
	if(grantpt(pty)<0 || unlockpt(pty)<0)
	    {
	    mssError(0,"SHL","granpt() or unlockpt() failed");
	    shlClose(inf);
	    return NULL;
	    }

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("shell got tty: %s\n",(const char*)ptsname(pty));

	strncpy(tty_name,(const char*)ptsname(pty),32);
	tty_name[31]='\0';
	
	inf->shell_pid=fork();
	if(inf->shell_pid < 0)
	    {
	    mssError(0,"SHL","Unable to fork");
	    inf->shell_pid=0;
	    shlClose(inf);
	    return NULL;
	    }
	if(inf->shell_pid==0)
	    {
	    int fd;
	    /** we're in the child process -- disable MTask context switches to be safe **/
	    thLock();

	    /** child -- shell **/
	    if(SHELL_DEBUG & SHELL_DEBUG_FORK)
		printf("in child\n");

	    if(SHELL_DEBUG * SHELL_DEBUG_EXEC)
		printf("exec: %s %p\n",inf->program,inf->args);

	    /** security issue: when centrallix runs as root with system
	     ** authentication, the threads run with ruid=root, euid=user,
	     ** and we need to get rid of the ruid=root here so that the
	     ** command's privs are more appropriate.  Same for group id.
	     **/
	    /** Removed mssError() calls to attempt to make this a bit safer,
	     **   as mssError() modifies the error stack.  _Nothing_ called in the child
	     **   part of the fork() should call anything else in centrallix or centrallix-lib
	     **   -- at least that's my opinion.... -- Jonathan Rupp 11/18/2002 **/
	    /** 
	     ** Re-added mssError() calls, because after a fork() the process spaces
	     ** are *completely* isolated.  The error stack in the parent Centrallix
	     ** will not be modified.  Plus, using mssError() allows for syslog() based
	     ** error reporting to still work, as well as stderr.
	     **/
	    if (getuid() != geteuid())
		{
		if (setreuid(geteuid(),-1) < 0)
		    {
		    /** Rats!  we couldn't do it! **/
		    mssError(1,"SHL","Could not drop privileges!");
		    _exit(1);
		    }
		}
	    if (getgid() != getegid())
		{
		if (setregid(getegid(),-1) < 0)
		    {
		    /** Rats!  we couldn't do it! **/
		    mssError(1,"SHL","Could not drop group privileges!");
		    _exit(1);
		    }
		}

	    /** close all open fds (except for 0-2 -- std{in,out,err}) **/
	    for(fd=3;fd<64;fd++)
		close(fd);
	    
	    /** open the terminal **/
	    fd = open(tty_name,O_RDWR);
	    
	    /** switch to terminal as stdout **/
	    dup2(fd,0);

	    /** switch to terminal as stdin **/
	    dup2(fd,1);

	    /** close old copy of FD **/
	    close(fd);

	    /** make the exec() call **/
	    execvp(inf->program,inf->args);

	    /** if exec() is successfull, this is never reached **/
	    _exit(1);
	    }
	/** parent -- centrallix **/
	if(SHELL_DEBUG & SHELL_DEBUG_FORK)
	    printf("still in parent :)\n");

	/** open up the terminal that we'll use for comm with the child process **/
	inf->shell_fd=fdOpenFD(pty,O_RDWR);

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("SHELL: returning object: %p\n",inf);
    return (void*)inf;
    }


/*** shlClose - close an open object.
 ***/
int
shlClose(void* inf_v, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    int childstat=0;
    int waitret;

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("SHELL: closing object: %p\n",inf);

	if(inf->shell_pid)
	    {
	    waitret=waitpid(inf->shell_pid,&childstat,WNOHANG);
	    if(waitret==0)
		{
		if(!kill(inf->shell_pid,SIGINT))
		    {
		    /** give it a second to respond to SIGINT **/
		    thSleep(1000);
		    waitret=waitpid(inf->shell_pid,&childstat,WNOHANG);
		    if(waitret==0)
			{
			kill(inf->shell_pid,SIGKILL);
			while(!waitpid(inf->shell_pid,&childstat,WNOHANG))
			    {
			    /** wait for it to die in 1 second intervals **/
			    thSleep(1000);
			    }
			}
		    }
		}
	    if(waitret<0)
		mssError(0,"SHL","Error while trying to reap process %i: %i\n",inf->shell_pid,errno);

	    if(SHELL_DEBUG & SHELL_DEBUG_FORK)
		printf("child (%i) returned: %i\n",inf->shell_pid,childstat);
	    }

	if(inf->args)
	    {
	    /** free memory for argument array **/
	    nmFree(inf->args,(inf->nArgs+1)*sizeof(char*));
	    }

	if(inf->Node)
	    {
	    /** Write the node first, if need be. **/
	    snWriteNode(inf->Obj->Prev,inf->Node);
	    
	    /** Release the memory **/
	    inf->Node->OpenCnt --;
	    }	

	nmFree(inf,sizeof(ShlData));

    return 0;
    }


/*** shlCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
shlCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = shlOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	shlClose(inf, oxt);

    return 0;
    }


/*** shlDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
shlDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pShlData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pShlData)shlOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
		{
		shlClose(inf, oxt);
		mssError(1,"SHL","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
		{
		shlClose(inf, oxt);
		mssError(1,"SHL","Cannot delete: object not empty");
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
	nmFree(inf,sizeof(ShlData));

    return 0;
    }


/*** shlRead - Read data from the shell's stdout
 ***/
int
shlRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    int i=-1;
    int waitret;
    int retval;
    
    /** can't seek backwards on a stream :) **/
    if(flags & FD_U_SEEK)
	{
	if(offset>inf->curRead)
	    {
	    /** scroll forward to point in the stream to read from **/
	    while(offset>inf->curRead)
		{
		char buf[1024];
		int i;
		int readlen = offset-inf->curRead>1024?1024:offset-inf->curRead;
		/** call shlRead instead of fdRead directly, as it has extra protection and this
		 **   will only ever go to one level of recursion **/
		i=shlRead(inf,buf,readlen,0,flags & ~FD_U_SEEK,oxt);
		if(i==-1)
		    return -1;
		inf->curRead+=i;
		}
	}
	/** this'll also catch if we scroll too far forward... **/
	if(offset<inf->curRead)
	    return -1;
	}

    while(i < 0)
	{
	i=fdRead(inf->shell_fd,buffer,maxcnt,0,flags & ~FD_U_SEEK);
	if(i < 0)
	    {
	    /** user doesn't want us to block **/
	    if(flags & FD_U_NOBLOCK)
		return -1;

	    /** if there is no more child process, that's probably why we can't read :) **/
	    if(!inf->shell_pid)
		return -1;

	    /** check and make sure the child process is still alive... **/
	    waitret=waitpid(inf->shell_pid,&retval,WNOHANG);
	    if(waitret==0)
		{
		/** child is alive, it just doesn't have any data yet -- wait for some **/
		thSleep(200);
		}
	    else if(waitret>0)
		{
		/** child died :( -- mark it as dead and retry the read */
		inf->shell_pid=0;
		}
	    else
		{
		/** hmm... error... **/
		mssError(0,"SHL","Error checking for dead child: %i\n",errno);
		return -1;
		}
	    }
	}
    inf->curRead+=i;
    return i;
    }


/*** shlWrite - Write data to the shell's stdin
 ***/
int
shlWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    int i=-1;
    int waitret;
    int retval;

    /** seek is _not_ allowed (obviously) **/
    if(flags & FD_U_SEEK && offset!=inf->curWrite)
	return -1;

    /** can't write to a dead child :) **/
    if(!inf->shell_pid)
	return -1;

    while(i < 0)
	{
	i=fdWrite(inf->shell_fd,buffer,cnt,0,flags & ~FD_U_SEEK);
	if(i < 0)
	    {
	    /** user doesn;t want us to block **/
	    if(flags & FD_U_NOBLOCK)
		return -1;

	    /** check and make sure the child process is still alive... **/
	    waitret=waitpid(inf->shell_pid,&retval,WNOHANG);
	    if(waitret==0)
		{
		/** child is alive, it just isn't ready for data yet -- wait a bit **/
		thSleep(200);
		}
	    else if(waitret>0)
		{
		/** child died :( **/
		inf->shell_pid=0;
		
		return -1;
		}
	    else
		{
		/** hmm... error... **/
		mssError(0,"SHL","Error checking for dead child: %i\n",errno);
		return -1;
		}
	    }
	}
    inf->curWrite+=i;
    return i;
    }


/*** shlOpenQuery - This driver has no subobjects
 ***/
void*
shlOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** shlQueryFetch - This object has no subobjects
 ***/
void*
shlQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** shlQueryClose - driver has no subobjects
 ***/
int
shlQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** shlGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
shlGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	if (!strncmp(attrname,"arg",3)) return DATA_T_STRING;

	mssError(1,"SHL","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** shlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
shlGetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    *((char**)val) = "application/octet-stream";
	    return 0;
	    }

	if (!strcmp(attrname,"outer_type"))
	    {
	    *((char**)val) = "system/shell";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	if (!strncmp(attrname,"arg",3))
	    {
	    if(!sscanf(attrname,"arg%i",&i))
		return -1;
	    if(i>=inf->nArgs || i<0)
		return -1;
	    *(char**)val = inf->args[i];
	    return 0;
	    }

	mssError(1,"SHL","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** shlGetNextAttr - get the next attribute name for this object.
 ***/
char*
shlGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pShlData inf = SHL(inf_v);
    int i;
    
    if(inf->CurAttr>=inf->nArgs)
	return NULL;

    if(inf->CurAttr>999999)
	return NULL;

    i=snprintf(inf->sCurAttr,10,"arg%02i",inf->CurAttr++);
    inf->sCurAttr[10]='\0';

    return inf->sCurAttr;
    }


/*** shlGetFirstAttr - get the first attribute name for this object.
 ***/
char*
shlGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pShlData inf = SHL(inf_v);

    inf->CurAttr=0;
    return shlGetNextAttr(inf_v,oxt);
    }


/*** shlSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
shlSetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree oxt)
    {
    pShlData inf = SHL(inf_v);
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
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"SHL","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
		strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
		strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
		if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"SHL","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
		strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** shlAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
shlAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pShlData inf = SHL(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** shlOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
shlOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** shlGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
shlGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** shlGetNextMethod -- same as above.  Always fails. 
 ***/
char*
shlGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** shlExecuteMethod - No methods to execute, so this fails.
 ***/
int
shlExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** shlInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
shlInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Setup the structure **/
	strcpy(drv->Name,"SHL - Shell osdriver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/shell");

	/** Setup the function references. **/
	drv->Open = shlOpen;
	drv->Close = shlClose;
	drv->Create = shlCreate;
	drv->Delete = shlDelete;
	drv->OpenQuery = shlOpenQuery;
	drv->QueryClose = shlQueryClose;
	drv->Read = shlRead;
	drv->Write = shlWrite;
	drv->GetAttrType = shlGetAttrType;
	drv->GetAttrValue = shlGetAttrValue;
	drv->GetFirstAttr = shlGetFirstAttr;
	drv->GetNextAttr = shlGetNextAttr;
	drv->SetAttrValue = shlSetAttrValue;
	drv->AddAttr = shlAddAttr;
	drv->OpenAttr = shlOpenAttr;
	drv->GetFirstMethod = shlGetFirstMethod;
	drv->GetNextMethod = shlGetNextMethod;
	drv->ExecuteMethod = shlExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(ShlData),"ShlData");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(shlInitialize);
MODULE_PREFIX("shl");
MODULE_DESC("SHL ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

