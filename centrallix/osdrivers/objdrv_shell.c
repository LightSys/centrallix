#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/util.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "cxlib/strtcpy.h"
/** module definintions **/
#include "centrallix.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <err.h>

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
    XArray	argArray;   // pointers back to the structure file for arguments (null pointer terminated) -- don't free
    XArray	envArray;   // pointers to malloc()ed char*s that represent the environment (null pointer terminated) -- free with free() (except the final entry which is NULL)
    XArray	envList;    // pointers back to the structure file for environment variable names -- don't free
    XHashTable	envHash;    // EnvVar structures -- free() value if shouldfree==1, always nmFree() the structure
    pFile	shell_fd;
    int		curRead;
    int		curWrite;
    pid_t	shell_pid;
    int		done;
    char	vBuf[3];     // a verification buffer used to hold potentially valid split chars 
    int		vLen;
    }
    ShlData, *pShlData;

typedef struct
    {
    char*	value;
    char	changeable;
    char	shouldfree;
    }
    EnvVar, *pEnvVar;


/*** free wrapper for freeing EnvVars in an XHashTable
***/
int
free_EnvVar(void* p, void* arg)
    {
    pEnvVar ptr = (pEnvVar)p;
    arg = NULL ; /* avoid complaints from picky compilers */
    if(ptr->shouldfree)
	nmSysFree(ptr->value); // value was malloc()ed, not nmMalloc()ed
    nmFree(ptr,sizeof(EnvVar));
    return 0   ; /* meaningless, but is needed for type safety */
    }
    

#define SHL(x) ((pShlData)(x))

#define SHELL_DEBUG 0x00
#define SHELL_DEBUG_PARAM 0x01
#define SHELL_DEBUG_FORK 0x02
#define SHELL_DEBUG_EXEC 0x04
#define SHELL_DEBUG_OPEN 0x08
#define SHELL_DEBUG_INIT 0x10
#define SHELL_DEBUG_LAUNCH 0x20
#define SHELL_DEBUG_IO 0x40


/*** shl_internal_UpdateStatus -- checks the status of the shell and updates shell_pid accordingly
 ***/
void
shl_internal_UpdateStatus(pShlData inf)
    {
    int waitret;
    int retval;

    /** ensure there's actually a running shell to check **/
    if(inf->shell_pid<=0)
	return;

    waitret=waitpid(inf->shell_pid,&retval,WNOHANG);
    if(waitret>0)
	{
	inf->shell_pid = 0;
	}
    else if(waitret==0)
	{
	/** child is still running... no problems here **/
	}
    else
	{
	/** hmm... error... **/
	mssError(0,"SHL","Error checking for dead child: %s\n",strerror(errno));
	return;
	}
    }

/*** shl_internal_Launch -- launches the subprocess and sets up the communication
 ***/
int
shl_internal_Launch(pShlData inf)
    {
    char tty_name[32];
    int pty;
    int i;
    int maxfiles;
    gid_t gidlist[1];

    for(i=0;i<xaCount(&inf->envList);i++)
	{
	pEnvVar pEV;
	char* name;
	char* p;
	name=(char*)xaGetItem(&inf->envList,i);
	pEV=(pEnvVar)xhLookup(&inf->envHash,name);
	if(pEV)
	    {
	    int len = strlen(name)+2+(pEV->value?strlen(pEV->value):0);
	    p = (char*)nmSysMalloc(len);
	    snprintf(p,len,"%s=%s",name,pEV->value?pEV->value:"");
	    p[len-1]='\0';
	    xaAddItem(&inf->envArray,p);
	    }
	}
    xaAddItem(&inf->envArray,NULL);

    
    if(SHELL_DEBUG & SHELL_DEBUG_LAUNCH)
	{
	char **ptr;
	int i=0;
	printf("%s(%p)\n",__FUNCTION__,inf);
	
	printf("program: %s\n",inf->program);

	ptr=(char**)(inf->argArray.Items);
	do
	    {
	    printf("arg: %p: %s\n",ptr,*ptr);
	    }
	while(*(ptr++));

	ptr=(char**)(inf->envArray.Items);
	do
	    {
	    printf("env: %p: %s\n",ptr,*ptr);
	    }
	while(*(ptr++));
	
	}

    pty=getpt();
    if(pty<0)
	{
	mssErrorErrno(0,"SHL","getpy() failed");
	inf->shell_pid=0;
	return -1;
	}

    /** grantpt() uses the effective uid for permission checks and the real
     ** uid for what to chown the device to.  So we need to swap our
     ** uids here, then swap em back.
     **/
    setreuid(geteuid(), getuid());
    if(grantpt(pty)<0)
	{
        setreuid(geteuid(), getuid());
	mssErrorErrno(0,"SHL","grantpt() failed");
	inf->shell_pid=0;
	return -1;
	}
    setreuid(geteuid(), getuid());

    if (unlockpt(pty)<0)
	{
	mssErrorErrno(0,"SHL","unlockpt() failed");
	inf->shell_pid=0;
	return -1;
	}

    if (ptsname_r(pty, tty_name, sizeof(tty_name)) != 0)
	{
	mssErrorErrno(0,"SHL","ptsname_r() failed");
	inf->shell_pid=0;
	return -1;
	}

    if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	printf("shell got tty: %s\n",(const char*)tty_name);

    inf->shell_pid=fork();
    if(inf->shell_pid < 0)
	{
	mssErrorErrno(1,"SHL","Unable to fork");
	inf->shell_pid=0;
	return -1;
	}
    if(inf->shell_pid==0)
	{
	int fd;
	/** we're in the child process -- disable MTask context switches to be safe **/
	thLock();

	/** child -- shell **/
	if(SHELL_DEBUG & SHELL_DEBUG_FORK)
	    printf("in child\n");

	/** security issue: when centrallix runs as root with system
	 ** authentication, the threads run with ruid=root, euid=user,
	 ** and we need to get rid of the ruid=root here so that the
	 ** command's privs are more appropriate.  Same for group id.
	 **/
	setgroups(0, gidlist);
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
	maxfiles = sysconf(_SC_OPEN_MAX);
	if (maxfiles <= 0)
	    {
	    printf("Warning: sysconf(_SC_OPEN_MAX) returned <= 0; using maxfiles=2048.\n");
	    maxfiles = 2048;
	    }
	for(fd=3;fd<maxfiles;fd++)
	    close(fd);

	/** start a new process session and open the terminal **/
	setsid();
	fd = open(tty_name,O_RDWR);
	
	/** switch to terminal as stdout **/
	dup2(fd,0);

	/** switch to terminal as stdin **/
	dup2(fd,1);

	/** switch to terminal as stderr **/
	dup2(fd,2);

	/** close old copy of FD **/
	if (fd > 2) close(fd);


	/** make the exec() call **/
	execve(inf->program,(char**)(inf->argArray.Items),(char**)(inf->envArray.Items));

	/** if exec() is successfull, this is never reached **/
	warn("execve() failed");

	    {
	    /** execve shouldn't fail -- dump the args for debugging **/
	    char **ptr;
	    int i=0;
	    printf("program: %s\n",inf->program);

	    ptr=(char**)(inf->argArray.Items);
	    do
		{
		printf("arg: %p: %s\n",ptr,*ptr);
		}
	    while(*(ptr++));

	    ptr=(char**)(inf->envArray.Items);
	    do
		{
		printf("env: %p: %s\n",ptr,*ptr);
		}
	    while(*(ptr++));
	    
	    }

	_exit(1);
	}
    /** parent -- centrallix **/
    if(SHELL_DEBUG & SHELL_DEBUG_FORK)
	printf("still in parent :)\n");

    /** open up the terminal that we'll use for comm with the child process **/
    inf->shell_fd=fdOpenFD(pty,O_RDWR);

    return 0;
    }


/*** shl_internal_SetParam() - sets a parameter to pass to the system command,
 *** via a "changeable" environment variable.
 ***/
int
shl_internal_SetParam(pShlData inf, char* paramname, int type, pObjData paramvalue)
    {
    pEnvVar pEV;

	/** only do the change if the subprocess hasn't started yet -- now these 
	 ** will always reflect the values used to start the subprocess
	 **/
	if(inf->shell_pid == -1)
	    {
	    pEV = (pEnvVar)xhLookup(&inf->envHash,paramname);
	    if(pEV && pEV->changeable && (type==DATA_T_STRING || type==DATA_T_INTEGER) )
		{
		if(pEV->shouldfree)
		    {
		    nmSysFree(pEV->value);
		    }
		if (!paramvalue)
		    {
		    pEV->value = nmSysStrdup("");
		    }
		else if (type == DATA_T_STRING)
		    {
		    /* pEV->value = (char*)malloc(strlen(*(char**)val+1)); */ /* whoops */
		    pEV->value = (char*)nmSysMalloc(strlen(paramvalue->String)+1);
		    strcpy(pEV->value,paramvalue->String);
		    }
		else if (type == DATA_T_INTEGER)
		    {
		    pEV->value = (char*)nmSysMalloc(20);
		    snprintf(pEV->value,20,"%i", paramvalue->Integer);
		    pEV->value[19]='\0';
		    }
		else
		    pEV->value = nmSysStrdup("");
		pEV->shouldfree = 1; // we just malloc()ed memory... make sure it gets free()ed
		return 0;
		}
	    }

    return -1;
    }

    
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
    pStructInf argStruct;
    pStructInf changeStruct;
    int i,j;
    pStruct paramdata;
    int nameindex;
    char* endorsement_name;

	/** Allocate the structure **/
	inf = (pShlData)nmMalloc(sizeof(ShlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(ShlData));
	inf->Obj = obj;
	inf->Mask = mask;
	inf->vLen = 0;

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

	/** Security check **/
	if (endVerifyEndorsements(node->Data, stGetObjAttrValue, &endorsement_name) < 0)
	    {
	    inf->Node->OpenCnt --;
	    nmFree(inf,sizeof(ShlData));
	    mssError(1,"SHL","Security check failed - endorsement '%s' required", endorsement_name);
	    return NULL;
	    }

	/** figure out command to run **/
	if(stAttrValue(stLookup(node->Data,"program"),NULL,&ptr,0)<0) ptr="";
	strncpy(inf->program,ptr,79);
	inf->program[79]='\0';
	if(SHELL_DEBUG & SHELL_DEBUG_PARAM) printf("program: %s\n",inf->program);	
	
	argStruct=stLookup(node->Data,"arg");

	/** init structures for holding arguments and environmental variables **/
	xaInit(&inf->argArray,argStruct?5:1);
	xaInit(&inf->envArray,3);
	xaInit(&inf->envList,3);
	xhInit(&inf->envHash,17,0);
	
	if(!argStruct)
	    {
	    /** no 'arg' value present -- no parameters **/
	    }
	else
	    {
	    if(argStruct->Value->NodeType == EXPR_N_LIST)
		{
		/** 'arg' value is present and is a list -- parameters > 1 **/
		int i;
		pXArray values;
		values = &argStruct->Value->Children;
		for(i=0;i<values->nItems;i++)
		    xaAddItem(&inf->argArray,((pExpression)(values->Items[i]))->String);
		}
	    else
		{
		char *ptr;
		/** 'arg' value is present and is exactly one value -- 1 parameter **/
		if(stAttrValue(argStruct,NULL,&ptr,0)<0)
		    ptr=NULL;
		xaAddItem(&inf->argArray,ptr);
		}
	    xaAddItem(&inf->argArray,NULL);
	    }

	for(i=0;i<node->Data->nSubInf;i++)
	    {
	    pStructInf str = node->Data->SubInf[i];
	    if (strcmp(str->Name, "program") && strcmp(str->Name,"arg") && strcmp(str->Name,"changeable") ) 
		{
		pEnvVar pEV = (pEnvVar)nmMalloc(sizeof(EnvVar));
		if(stAttrValue(str,NULL,&pEV->value,0)<0)
		    pEV->value=NULL;
		pEV->changeable = 0;
		pEV->shouldfree = 0;
		xhAdd(&inf->envHash,str->Name,(char*)pEV); // add it to the hash
		xaAddItem(&inf->envList,str->Name); // add the name to the list
		}
	    }

	changeStruct=stLookup(node->Data,"changeable");

	if(changeStruct)
	    {
	    if(changeStruct->Value->NodeType == EXPR_N_LIST)
		{
		/** 'arg' value is present and is a list -- parameters > 1 **/
		int i;
		pXArray values;
		pEnvVar pEV=NULL;
		values = &changeStruct->Value->Children;
		for(i=0;i<values->nItems;i++)
		    {
		    pEV=(pEnvVar)xhLookup(&inf->envHash,((pExpression)(values->Items[i]))->String);
		    if(pEV)
			{
			pEV->changeable = 1;
			}
		    else
			mssError(0,"SHL","can't find %s to mark changeable\n",((pExpression)(values->Items[i]))->String);
		    }
		}
	    else
		{
		char *ptr;
		pEnvVar pEV=NULL;
		/** 'arg' value is present and is exactly one value -- 1 parameter **/
		if(stAttrValue(changeStruct,NULL,&ptr,0)<0)
		    ptr=NULL;
		if(ptr)
		    pEV=(pEnvVar)xhLookup(&inf->envHash,ptr);
		if(pEV)
		    {
		    pEV->changeable = 1;
		    }
		else
		    mssError(0,"SHL","can't find %s to mark changeable\n",ptr);
		}
	    }


	/** note that the child process hasn't been started yet **/
	inf->shell_pid = -1; 

	/** this is set to 1 when a read fails with EIO **/
	inf->done = 0;

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("SHELL: returning object: %p\n",inf);

	/** Set initial param values? **/
	nameindex = obj->SubPtr - 1 + obj->SubCnt - 1;
	if (obj->Pathname->OpenCtl[nameindex])
	    {
	    paramdata = obj->Pathname->OpenCtl[nameindex];
	    for(i=0;i<paramdata->nSubInf;i++)
		{
		shl_internal_SetParam(inf, paramdata->SubInf[i]->Name, DATA_T_STRING, POD(&(paramdata->SubInf[i]->StrVal)));
		}
	    }

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
    int i;

	if(SHELL_DEBUG & SHELL_DEBUG_OPEN)
	    printf("SHELL: closing object: %p\n",inf);

	if(inf->shell_pid>0)
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
#if 0
	if(inf->args)
	    {
	    /** free memory for argument array **/
	    nmFree(inf->args,(inf->nArgs+1)*sizeof(char*));
	    }

	if(inf->envs)
	    {
	    /** free memory for environment array **/
	    nmFree(inf->envs,(inf->nEnvs+1)*sizeof(char*));
	    }

	xhClear(&inf->envHash,free_wrapper,NULL);
	xhDeInit(&inf->envHash);
#endif
	

	xaDeInit(&inf->argArray);
	for(i=0;i<xaCount(&inf->envArray);i++)
	    {
	    if (xaGetItem(&inf->envArray,i))
		nmSysFree(xaGetItem(&inf->envArray,i));
	    }
	xaDeInit(&inf->envList);
	xhClear(&inf->envHash,free_EnvVar,NULL);
	xhDeInit(&inf->envHash);



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
    int temp;
    int waitret;
    int retval;
    int ind;

    if(maxcnt < 4) /* cannot verify properly witout a larger buffer size */
	{
	mssError(1,"SHL","Shell requires a buffer size of at least 4 to verify properly");
	return -1;
	}

    if(SHELL_DEBUG & SHELL_DEBUG_IO)
	printf("%s -- %p, %p, %i, %i, %i, %p\n",__FUNCTION__,inf_v,buffer,maxcnt,offset,flags,oxt);

    /** launch the program if it's not running already **/
    if(inf->shell_pid == -1)
	if(shl_internal_Launch(inf) < 0)
	    return -1;
    
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

    /** check if vBuf has data to insert **/
    if(inf->vLen > 0)
	memcpy(buffer, inf->vBuf, inf->vLen);

    while(i < 0)
	{

	i=fdRead(inf->shell_fd,(buffer+inf->vLen),(maxcnt-inf->vLen),0,flags & ~FD_U_SEEK);
	if(i < 0)
	    {
	    /** if we get EIO, set the done flag **/
	    if(errno == EIO)
		{
		inf->done = 1;
		if(inf->vLen != 0)
		    {
		    mssError(1,"SHL","Shell returned an invalid character");
		    if(SHELL_DEBUG & SHELL_DEBUG_IO)
			printf("Recived EIO with %d bytes in vBuf. char(s) = %02x %02x %02x\n", inf->vLen, 
			(unsigned char) inf->vBuf[0], (unsigned char) inf->vBuf[1], (unsigned char) inf->vBuf[2]);
		    }
		return -1;
		}

	    /** user doesn't want us to block **/
	    if(flags & FD_U_NOBLOCK)
		{
		if(inf->vLen != 0)
		    {
		    /** stopped early because couldn't block, but still lost some data that may be invalid, so report **/
		    mssError(1,"SHL","Shell returned an invalid character");
		    if(SHELL_DEBUG & SHELL_DEBUG_IO)
			printf("Cannot block with %d bytes in vBuf. char(s) = %02x %02x %02x\n ", inf->vLen, 
			(unsigned char) inf->vBuf[0], (unsigned char) inf->vBuf[1], (unsigned char) inf->vBuf[2]);
		    }
		return -1;
		}

	    /** if there is no more child process, that's probably why we can't read :) **/
	    if(inf->shell_pid==0)
		{
		if(inf->vLen != 0)
		    {
		    mssError(1,"SHL","Shell returned an invalid character");
		    if(SHELL_DEBUG & SHELL_DEBUG_IO) 
			printf("No more child process with %d bytes in vBuf. char(s) = %02x %02x %02x\n", inf->vLen, 
			(unsigned char) inf->vBuf[0], (unsigned char) inf->vBuf[1], (unsigned char) inf->vBuf[2]);
		    }
		return -1;
		}

	    shl_internal_UpdateStatus(inf);

	    if(inf->shell_pid>0)
		{
		/** child is alive, it just doesn't have any data yet -- wait for some **/
		thSleep(200);
		}
	    else
		{
		/** child just died -- retry the read **/
		}
	    }
	if(i > 0 && i + inf->vLen <= 3) /* verification needs at least 3 bytes to work consistently, unless at end */
	    {
	    inf->vLen += i; /* up offset */
	    i = -1;         /* make sure the read runs again */
	    }
	}
    i+= inf->vLen; /* update i to be correct */

    /** check the new input to make sure everything is in a valid state **/
    if((ind = nVerifyUTF8(buffer, i)) != UTIL_VALID_CHAR) /** TODO: this may not be needed; could be something besides text **/
        {
        if(i - ind <= 3) /* if error is in last 3 bytes, store for later */
	    {
	    inf->vLen = i - ind;
	    memcpy(inf->vBuf, buffer+ind, inf->vLen);
	    i = ind;
	    memset(buffer+i, '\0', inf->vLen); /* overwrite bad value */
	    }
	else /* is definently an error, end read */
	    {
	    mssError(1,"SHL","Shell returned an invalid character");
	    if(SHELL_DEBUG & SHELL_DEBUG_IO)
		printf("Invalid char in buffer: ind = %d, char = %02x\n", ind, (unsigned char) buffer[ind]);
	    return -1;
	    }
        }
    else /*everything is valid, reset vLen*/
	inf->vLen = 0;

    /** if read returned nothing, vBuf should be empty **/
    if(inf->vLen != 0 && i == 0)
	    {
	    mssError(1,"SHL","Shell returned an invalid character");
	    if(SHELL_DEBUG & SHELL_DEBUG_IO)
	    	printf("Read 0 bytes, but still had %d byte(s) in vBuf. char(s): %02x %02x %02x\n", inf->vLen, 
	    	(unsigned char) inf->vBuf[0], (unsigned char) inf->vBuf[1], (unsigned char) inf->vBuf[2]);
	    return -1;
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

    if(SHELL_DEBUG & SHELL_DEBUG_IO)
	printf("%s -- %p, %p, %i, %i, %i, %p\n",__FUNCTION__,inf_v,buffer,cnt,offset,flags,oxt);

    /** launch the program if it's not running already **/
    if(inf->shell_pid == -1)
	if(shl_internal_Launch(inf) < 0)
	    return -1;

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
	    shl_internal_UpdateStatus(inf);
	    if(inf->shell_pid>0)
		{
		/** child is alive, it just isn't ready for data yet -- wait a bit **/
		thSleep(200);
		}
	    else if(inf->shell_pid==0)
		{
		/** child died :( **/
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
    pEnvVar pEV;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	if (!strcmp(attrname,"status")) return DATA_T_STRING;

	if (!strncmp(attrname,"arg",3)) return DATA_T_STRING;

	pEV = (pEnvVar)xhLookup(&inf->envHash,attrname);
	if(pEV)
	    {
	    return DATA_T_STRING;
	    }

	mssError(1,"SHL","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** shlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
shlGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pShlData inf = SHL(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;
    pEnvVar pEV;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    /*val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];*/
	    val->String = inf->Node->Data->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    val->String = "application/octet-stream";
	    return 0;
	    }

	if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "system/shell";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	if (!strncmp(attrname,"arg",3))
	    {
	    if(!sscanf(attrname,"arg%i",&i))
		return -1;
	    if(i>=xaCount(&inf->argArray) || i<0)
		return -1;
	    val->String = (char*)xaGetItem(&inf->argArray,i);
	    return 0;
	    }

	if (!strcmp(attrname,"status"))
	    {
	    shl_internal_UpdateStatus(inf);
	    errno=0;

	    if (inf->shell_pid==-1)
		{
		val->String = "not started";
		return 0;
		}
	    /** it is 'running' if any of the following is true:
	      1. the process is running
	      2. the file descriptor can be read from
	      3. the error when reading is EAGAIN
	    **/
	    else if(inf->shell_pid>0 || !inf->done )
		{
		val->String = "running";
		return 0;
		}
	    else if (inf->shell_pid==0)
		{
		val->String = "dead";
		return 0;
		}
	    else
		{
		val->String = "unknown";
		return 0;
		}
	    }

	pEV = (pEnvVar)xhLookup(&inf->envHash,attrname);
	if(pEV)
	    {
	    val->String = pEV->value;
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

    if(inf->CurAttr>=xaCount(&inf->argArray)+xaCount(&inf->envList)+1)
	return NULL;

    if(inf->CurAttr==xaCount(&inf->argArray)+xaCount(&inf->envList))
	{
	inf->CurAttr++;
	return "status";
	}

    if(inf->CurAttr>999999 && xaCount(&inf->argArray)>=999999)
	return NULL;

    if(inf->CurAttr<xaCount(&inf->argArray))
	{
	i=snprintf(inf->sCurAttr,10,"arg%02i",inf->CurAttr++);
	inf->sCurAttr[10]='\0';
	return inf->sCurAttr;
	}
    else
	{
	return (char*)xaGetItem(&inf->envList,(inf->CurAttr++)-xaCount(&inf->argArray));
	}
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
shlSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pShlData inf = SHL(inf_v);

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
		    mssError(1,"SHL","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
		strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
		strcpy(strrchr(inf->Pathname,'/')+1,val->String);
		if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"SHL","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
		strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}

	    /** Set dirty flag **/
	    inf->Node->Status = SN_NS_DIRTY;

	    return 0;
	    }

	/** Try to set a command parameter (env variable) **/
	if (shl_internal_SetParam(inf, attrname, datatype, val) < 0)
	    return -1;

    return -1;
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
shlExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** shlInfo - Return the capabilities of the object.
 ***/
int
shlInfo(void* inf_v, pObjectInfo info)
    {

	/** Shell objects have limited capability... **/
	info->Flags = OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CANT_SEEK;
	info->nSubobjects = 0;

    return 0;
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
	drv->Info = shlInfo;

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

