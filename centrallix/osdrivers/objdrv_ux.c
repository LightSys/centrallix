#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "stparse.h"
#include "st_node.h"

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
/* Module: 	objdrv_ux.c         					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 2, 1998					*/
/* Description:	UNIX filesystem objectsystem driver.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_ux.c,v 1.14 2004/06/12 00:10:15 mmcgill Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_ux.c,v $

    $Log: objdrv_ux.c,v $
    Revision 1.14  2004/06/12 00:10:15  mmcgill
    Chalk one up under 'didn't understand the build process'. The remaining
    os drivers have been updated, and the prototype for objExecuteMethod
    in obj.h has been changed to match the changes made everywhere it's
    called - param is now of type pObjData, not void*.

    Revision 1.13  2004/05/04 18:20:15  gbeeley
    - Another fix for the RDONLY vs RDWR issue

    Revision 1.12  2004/02/24 20:10:59  gbeeley
    - fixing some date/time related problems
    - efficiency improvement for net_http allowing browser to actually
      cache .js files and images.

    Revision 1.11  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.10  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.9  2003/04/30 02:15:35  jorupp
     * added stat calls before returning any data that would have come from it
       -- it's a bit inefficent -- really should just set a flag after a write or
          property change and clear that flag on stat(), and just run stat if
    	  the flag is set and we need the data

    Revision 1.8  2003/04/04 05:02:44  gbeeley
    Added more flags to objInfo dealing with content and seekability.
    Added objInfo capability to objdrv_struct.

    Revision 1.7  2003/03/31 23:23:40  gbeeley
    Added facility to get additional data about an object, particularly
    with regard to its ability to have subobjects.  Added the feature at
    the driver level to objdrv_ux, and to the "show" command in test_obj.

    Revision 1.6  2002/09/27 22:26:06  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.5  2002/08/10 02:09:45  gbeeley
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

    Revision 1.4  2002/08/01 08:25:21  mattphillips
    Include sys/time.h if configure tells us that struct tm is defined there.

    Revision 1.3  2001/10/16 23:53:02  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:11  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:11  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure used for storing filename annotations ***/
typedef struct
    {
    char	Filename[256];
    char	Annotation[256];
    }
    UxdAnnotation, *pUxdAnnotation;


/*** Node information structure for UXD ***/
typedef struct
    {
    pSnNode	SnNode;
    unsigned int LastSerialID;
    char	OSPath[OBJSYS_MAX_PATH];
    char	UXPath[OBJSYS_MAX_PATH];
    int		Flags;
    }
    UxdNode, *pUxdNode;

#define UXD_NODE_F_READONLY	1


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    pFile	fd;
    char	RealPathname[OBJSYS_MAX_PATH + 64];
    char	Buffer[OBJSYS_MAX_PATH];
    int		Flags;
    pObject	Obj;
    struct stat	Fileinfo;
    DIR*	Direc;
    int		Mask;
    int		CurAttr;
    char	UsrName[16];
    char	GrpName[16];
    DateTime	ATime;
    DateTime	MTime;
    DateTime	CTime;
    pUxdNode	Node;
    }
    UxdData, *pUxdData;

#define UXD_F_ISDIR	1
#define UXD_F_ISOPEN	2

#define UXD(x) ((pUxdData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pUxdData	File;
    char	NameBuf[256];
    }
    UxdQuery, *pUxdQuery;

/** Structure for caching the .type files in directories. **/
typedef struct
    {
    char	DirName[OBJSYS_MAX_PATH+64];
    char	Type[64];
    struct stat	Fileinfo;
    }
    UxdDirTypes, *pUxdDirTypes;

/*** GLOBALS ***/
struct
    {
    XHashTable	OpenFiles;
    XHashTable	Annotations;
    XHashTable	LoadedAnnot;
    XHashTable	NodesByOSPath;
    XHashTable	DirTypes;
    }
    UXD_INF;


/*** Attribute name list ***/
static struct
    {
    char* 	Name;
    int		Type;
    } 
    attr_inf[] =
    {
    {"owner",DATA_T_STRING},
    {"group",DATA_T_STRING},
    {"last_modification",DATA_T_DATETIME},
    {"last_access",DATA_T_DATETIME},
    {"last_change",DATA_T_DATETIME},
    {"permissions",DATA_T_INTEGER},
    {"size",DATA_T_INTEGER},
    {NULL,-1}
    };


/*** uxd_internal_DirType - get information from the .type file that
 *** may be in a directory to determine the default type of the objects
 *** within that directory.
 ***/
char*
uxd_internal_DirType(char* filepath)
    {
    pUxdDirTypes dt = NULL;
    char* slashptr;
    char tmpbuf[OBJSYS_MAX_PATH+70];
    struct stat fileinfo;
    int no_file = 0;
    int newer_file = 0;
    pFile fd;
    int cnt;

    	/** Strip off the filename from the directory first **/
	slashptr = strrchr(filepath,'/');
	if (!slashptr) return NULL;
	*slashptr = '\0';

	/** Check for the file **/
	snprintf(tmpbuf,OBJSYS_MAX_PATH+70,"%s/%s",filepath,".type");
	if (stat(tmpbuf,&fileinfo) <0) no_file = 1;

    	/** Lookup in hash table first **/
	dt = (pUxdDirTypes)xhLookup(&UXD_INF.DirTypes, filepath);
	if (dt && dt->Fileinfo.st_mtime < fileinfo.st_mtime) newer_file = 1;
	if (dt && !newer_file)
	    {
	    *slashptr = '/';
	    if (*(dt->Type)) return dt->Type;
	    else return NULL;
	    }
	else if (dt)
	    {
	    xhRemove(&UXD_INF.DirTypes, filepath);
	    }
	else if (!no_file)
	    {
	    dt = (pUxdDirTypes)nmMalloc(sizeof(UxdDirTypes));
	    snprintf(dt->DirName,OBJSYS_MAX_PATH+64,"%s",filepath);
	    }
	else
	    {
	    *slashptr = '/';
	    return NULL;
	    }

	/** Fill in the dt structure **/
	*slashptr = '/';
	memcpy(&(dt->Fileinfo),&fileinfo,sizeof(struct stat));
	fd = fdOpen(tmpbuf,O_RDONLY,0600);
	if (!fd)
	    {
	    nmFree(dt, sizeof(UxdDirTypes));
	    return NULL;
	    }
	cnt = fdRead(fd, dt->Type, 64, 0,0);
	if (cnt <= 1)
	    {
	    fdClose(fd,0);
	    nmFree(dt, sizeof(UxdDirTypes));
	    return NULL;
	    }
	dt->Type[cnt] = 0;
	if (strchr(dt->Type,'\n')) *(strchr(dt->Type,'\n')) = '\0';
	xhAdd(&UXD_INF.DirTypes, dt->DirName, (void*)dt);
	fdClose(fd,0);

    return dt->Type;
    }


/*** uxd_internal_LoadAnnot - load annotations for files in the given 
 *** directory.  Store them in the UXD_INF.Annotations hash table.
 ***/
int
uxd_internal_LoadAnnot(char* dirpath)
    {
    pUxdAnnotation ua;
    char sbuf[256];
    pFile fd;

    	/** Build the pathname to the annotations file **/
	snprintf(sbuf,256,"%s/.annotation",dirpath);

	/** Make sure it isn't already loaded. **/
	ua = (pUxdAnnotation)xhLookup(&UXD_INF.LoadedAnnot, sbuf);
	if (ua) return 0;

	/** Open the file and read its entries. **/
	fd = fdOpen(sbuf, O_RDONLY, 0600);
	if (!fd) return -1;

	/** Loop through the entries. **/
	ua = (pUxdAnnotation)nmMalloc(sizeof(UxdAnnotation));
	while (fdRead(fd, (void*)ua, sizeof(UxdAnnotation), 0,0) == sizeof(UxdAnnotation))
	    {
	    /** Prepend the centrallix's directory path **/
	    memmove(ua->Filename + strlen(dirpath)+1, ua->Filename, strlen(ua->Filename)+1);
	    memcpy(ua->Filename, dirpath, strlen(dirpath));
	    ua->Filename[strlen(dirpath)] = '/';

	    /** And add to the cache... **/
	    xhAdd(&UXD_INF.Annotations, ua->Filename, (void*)ua);
	    ua = (pUxdAnnotation)nmMalloc(sizeof(UxdAnnotation));
	    }

	/** Now add an entry for this directory. **/
	/** The way that loop worked, we have a leftover unused 'ua' struct **/
	snprintf(ua->Filename, 256, "%s", sbuf);
	strcpy(ua->Annotation, "");
	xhAdd(&UXD_INF.LoadedAnnot, ua->Filename, (void*)ua);

	/** Close the file and tail on outta here... **/
	fdClose(fd,0);

    return 0;
    }


/*** uxd_internal_ModifyAnnot - modify an existing annotation or add a new one to a
 *** file.  This routine automatically updates the hash table and/or the .annotation
 *** file in the target file's directory.
 ***/
int
uxd_internal_ModifyAnnot(pUxdData inf, char* new_annotation)
    {
    pUxdAnnotation ua, d_ua;
    char sbuf[320];
    pFile fd;
    int cnt;
    char* ptr;
    struct stat fileinfo;

    	/** Check annotation load for this dir first. **/
	ptr = strrchr(inf->RealPathname,'/');
	if (!ptr) return -1;
	*ptr = '\0';
	uxd_internal_LoadAnnot(inf->RealPathname);
	*ptr = '/';

    	/** See if we've already got the annotation **/
	ua = (pUxdAnnotation)xhLookup(&UXD_INF.Annotations, inf->RealPathname);
	
	/** IF already got one, modify ua struct in place, find .annotation file, update it **/
	if (ua)
	    {
	    /** Modify cached structure. **/
	    memccpy(ua->Annotation, new_annotation, '\0', 255);
	    ua->Annotation[255] = 0;

	    /** Now write out to file.  Seek & find.  We only allow 300 chars in sbuf
	     ** so that we can tack on the .annotation to the end.
	     **/
	    snprintf(sbuf,300,"%s",ua->Filename);
	    strcpy(strrchr(sbuf,'/'),"/.annotation");
	    cnt = 0;
	    ptr = strrchr(inf->RealPathname,'/')+1;
	    fd = fdOpen(sbuf, O_RDWR, 0600);
	    if (!fd) 
	        {
		mssErrorErrno(1,"UXD","Could not access .annotation file for modification");
		return -1;
		}
	    while(fdRead(fd, sbuf, 256, cnt*sizeof(UxdAnnotation), FD_U_SEEK) == 256)
	        {
		if (!strcmp(sbuf, ptr))
		    {
		    fdWrite(fd, ua->Annotation, 256, cnt*sizeof(UxdAnnotation)+256, FD_U_SEEK);
		    break;
		    }
		cnt++;
		}
	    fdClose(fd,0);
	    }
	/** Otherwise, create new ua structure and append/create .annotation file. **/
	else
	    {
	    /** Create the structure. **/
	    ua = (pUxdAnnotation)nmMalloc(sizeof(UxdAnnotation));
	    if (!ua) return -1;
	    memccpy(ua->Annotation, new_annotation, '\0', 255);
	    ua->Annotation[255] = 0;

	    /** Open the file for append/create, with same mode as its directory **/
	    strcpy(sbuf, inf->RealPathname);
	    *((char*)strrchr(sbuf,'/')) = '\0';
	    stat(sbuf, &fileinfo);
	    strcpy(memchr(sbuf,'\0',256),"/.annotation");
	    d_ua = (pUxdAnnotation)xhLookup(&UXD_INF.LoadedAnnot, sbuf);
	    fd = fdOpen(sbuf, O_RDWR | O_APPEND | O_CREAT, fileinfo.st_mode);
	    if (!fd) 
	        {
		nmFree(ua,sizeof(UxdAnnotation));
		return -1;
		}

	    /** Copy correct relative path to ua structure **/
	    *((char*)strrchr(sbuf,'/')) = '\0';
	    strcpy(ua->Filename, strrchr(inf->RealPathname,'/')+1);
	    fdWrite(fd, (void*)ua, sizeof(UxdAnnotation), 0,0);
	    fdClose(fd,0);

	    /** Get centrallix-filename and add the ua struct to hash table **/
	    strcpy(ua->Filename, inf->RealPathname);
	    xhAdd(&UXD_INF.Annotations, ua->Filename, (void*)ua);

	    /** Set file as loaded if it didn't exist before. **/
	    if (!d_ua)
	        {
	        ua = (pUxdAnnotation)nmMalloc(sizeof(UxdAnnotation));
	        if (!ua) return -1;
		strcpy(memchr(sbuf,'\0',256), "/.annotation");
		strcpy(ua->Filename, sbuf);
		strcpy(ua->Annotation, "");
		xhAdd(&UXD_INF.LoadedAnnot, ua->Filename, (void*)ua);
		}
	    }

    return 0;
    }


/*** uxdOpen - open a file or directory.
 ***/
void*
uxdOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pUxdData inf;
    pUxdNode node;
    pSnNode snnode = NULL;
    char* path;
    char* ptr;
    char* ptrcpy;
    char* uxpart;
    pStructInf paramdata;
    pPathname tmp_path = NULL;
    int basecnt, i, e, is_new_node = 0;
    pFile fd;

	/** Allocate the structure **/
	inf = (pUxdData)nmMalloc(sizeof(UxdData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(UxdData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Attempt to access the node information. **/
	path = obj_internal_PathPart(obj->Pathname,0,obj->SubPtr);
	node = (pUxdNode)xhLookup(&UXD_INF.NodesByOSPath, path);
	if (node) snnode = node->SnNode;

	/** If CREAT and EXCL, only create, failing if the thing already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    /** Already have a node?  Error if so, if user specified O_EXCL. **/
	    /** Otherwise, unless O_EXCL, try reading the node first before creating **/
	    if (snnode && (obj->Mode & O_EXCL))
	        {
		nmFree(inf, sizeof(UxdData));
		mssError(1,"UXD","Node/directory already exists");
		return NULL;
		}
	    else if (!snnode && !(obj->Mode & O_EXCL)) snnode = snReadNode(obj->Prev);
	    
	    /** Ok, no existing node.  Try to make a new one. **/
	    if (!snnode) snnode = snNewNode(obj->Prev, usrtype);
	    if (!snnode)
	        {
		nmFree(inf, sizeof(UxdData));
		mssError(1,"UXD","Could not create/open UXD node");
		return NULL;
		}

	    /** Opened/created the node.  Good.  Now build the UxdNode structure for it. **/
	    node = (pUxdNode)nmMalloc(sizeof(UxdNode));
	    node->SnNode = snnode;
	    path = obj_internal_PathPart(obj->Pathname,0,obj->SubPtr);
	    strcpy(node->OSPath, path);
	    node->LastSerialID = snGetSerial(snnode);
	    node->Flags = 0;
	    is_new_node = 1;

	    /** Is this a readonly node? **/
	    ptr = NULL;
	    if (!(stAttrValue(stLookup(snnode->Data, "readonly"), NULL, &ptr, 0) < 0) && ptr)
		{
		if (!strcasecmp(ptr,"yes")) node->Flags |= UXD_NODE_F_READONLY;
		}

	    /** Lookup 'path' in the open params so we know where in the unix fs to point the node. **/
	    ptr = NULL;
	    stAttrValue(stLookup(snnode->Data, "path"), NULL, &ptr, 0);
	    if (!ptr) stAttrValue_ne(stLookup_ne(obj->Pathname->OpenCtl[obj->SubPtr - 1],"path"),&ptr);
	    if (!ptr)
	        {
		nmFree(inf, sizeof(UxdData));
		nmFree(node, sizeof(UxdNode));
		mssError(1,"UXD","Open params or Node file must contain path= attribute");
		return NULL;
		}
	    if (!stLookup(snnode->Data, "path"))
	        {
		ptrcpy = nmSysStrdup(ptr);
	        paramdata = stAddAttr(snnode->Data, "path");
	        stAddValue(paramdata, ptrcpy, 0);
		}
	    memccpy(node->UXPath, ptr, 0, OBJSYS_MAX_PATH - 1);
	    node->UXPath[OBJSYS_MAX_PATH - 1] = 0;

	    /** Create the actual directory, if need be. **/
	    /** O_EXCL can be set if dir already exists since O_EXCL is mainly for the node itself **/
	    if (mkdir(ptr, mask) >= 0) obj->Flags |= OBJ_F_CREATED;
	    }
	else
	    {
	    /** Not creating.  Try to open the snnode if it wasn't cached. **/
	    if (!snnode) snnode = snReadNode(obj->Prev);
	    if (!snnode)
	        {
		mssError(0,"UXD","Could not read node file");
		nmFree(inf, sizeof(UxdData));
		if (node) nmFree(node, sizeof(UxdNode));
		}
	    }

	/** Do we need to initialize the uxd node information structure? **/
	if (!node)
	    {
	    node = (pUxdNode)nmMalloc(sizeof(UxdNode));
	    node->SnNode = snnode;
	    path = obj_internal_PathPart(obj->Pathname,0,obj->SubPtr);
	    strcpy(node->OSPath, path);
	    node->LastSerialID = snGetSerial(snnode);
	    node->Flags = 0;

	    /** Is this a readonly node? **/
	    ptr = NULL;
	    if (!(stAttrValue(stLookup(snnode->Data, "readonly"), NULL, &ptr, 0) < 0) && ptr)
		{
		if (!strcasecmp(ptr,"yes")) node->Flags |= UXD_NODE_F_READONLY;
		}

	    ptr = NULL;
	    if (stAttrValue(stLookup(snnode->Data, "path"), NULL, &ptr, 0) < 0 || !ptr)
	        {
		mssError(1,"UXD","Node file must contain path= attribute");
		nmFree(inf, sizeof(UxdData));
		nmFree(node, sizeof(UxdNode));
		return NULL;
		}
	    memccpy(node->UXPath, ptr, 0, OBJSYS_MAX_PATH - 1);
	    node->UXPath[OBJSYS_MAX_PATH - 1] = 0;
	    is_new_node = 1;
	    }

	/** If node shows readonly access, deny any read/write access **/
	if ((obj->Mode & (O_RDONLY | O_RDWR | O_WRONLY)) != O_RDONLY && (node->Flags & UXD_NODE_F_READONLY))
	    {
	    mssError(1,"UXD","Permission denied: node for this subtree is set to readonly");
	    nmFree(inf, sizeof(UxdData));
	    if (is_new_node) nmFree(node, sizeof(UxdNode));
	    return NULL;
	    }

	/** Figure out the real requested UNIX pathname. **/
	uxpart = obj_internal_PathPart(obj->Pathname,obj->SubPtr+0, 0);
	if (!uxpart) uxpart = "";
	if (strlen(node->UXPath) + strlen(uxpart) + 1 > OBJSYS_MAX_PATH + 64 - 1)
	    {
	    mssError(1,"UXD","Pathname exceeds internal representation");
	    nmFree(inf, sizeof(UxdData));
	    if (is_new_node) nmFree(node, sizeof(UxdNode));
	    return NULL;
	    }
	tmp_path = (pPathname)obj_internal_NormalizePath(node->UXPath, uxpart);
	strcpy(inf->RealPathname, obj_internal_PathPart(tmp_path,0,0));
	/*basecnt = tmp_path->nElements - (obj->Pathname->nElements-1);*/
	basecnt = tmp_path->nElements - 1 - (obj->Pathname->nElements - obj->SubPtr - 1);
	obj_internal_PathPart(obj->Pathname,0,0);

	/** Increment open count on the SnNode to hold it in the cache **/
	snnode->OpenCnt++;

	/** Verify the existence of the given pathname and determine what to do **/
	/** about possibly creating the last element of the path, if it doesn't **/
	/** already exist. **/
	for(i=basecnt;i<=tmp_path->nElements;i++)
	    {
	    /** Access a path element **/
	    path = obj_internal_PathPart(tmp_path,0,i);
	    if (stat(path,&(inf->Fileinfo)) < 0)
	        {
		/** Couldn't get it?  if not last or not create, error. **/
		e = errno;
		if (e != ENOENT || (!(obj->Mode & O_CREAT) || i != tmp_path->nElements))
		    {
		    mssErrorErrno(1,"UXD","Cannot access UNIX path component '%s'",path);
	    	    nmFree(inf, sizeof(UxdData));
	    	    if (is_new_node) nmFree(node, sizeof(UxdNode));
		    nmFree(tmp_path, sizeof(Pathname));
		    snnode->OpenCnt--;
	    	    return NULL;
		    }

		/** Try to make a directory or file? **/
		if (!strcmp(usrtype, "system/directory")) 
		    {
		    if (mkdir(path, obj->Mode) < 0)
		        {
		        mssErrorErrno(1,"UXD","Cannot create directory '%s'",path);
	    	        nmFree(inf, sizeof(UxdData));
	    	        if (is_new_node) nmFree(node, sizeof(UxdNode));
		        nmFree(tmp_path, sizeof(Pathname));
		        snnode->OpenCnt--;
	    	        return NULL;
			}
		    }
		else 
		    {
		    fd = fdOpen(path, obj->Mode, mask);
		    if (!fd)
		        {
		        mssErrorErrno(1,"UXD","Cannot create file '%s'",path);
	    	        nmFree(inf, sizeof(UxdData));
	    	        if (is_new_node) nmFree(node, sizeof(UxdNode));
		        nmFree(tmp_path, sizeof(Pathname));
		        snnode->OpenCnt--;
	    	        return NULL;
			}
		    inf->fd = fd;
		    inf->Flags |= UXD_F_ISOPEN;
		    }
		obj->Flags |= OBJ_F_CREATED;
		stat(path,&(inf->Fileinfo));
		}

	    /** Was it a file?  Or last element?  That ends the UXD's part of the path **/
	    if (!(S_ISDIR(inf->Fileinfo.st_mode)) || i == tmp_path->nElements)
	        {
		/** Last item is a directory? **/
		if (S_ISDIR(inf->Fileinfo.st_mode)) inf->Flags |= UXD_F_ISDIR;

		/** Set the count of elements we "consumed" **/
		obj->SubCnt = 1 + i - basecnt;
		strcpy(inf->RealPathname, path);

		/** Exit the search loop. **/
		break;
		}
	    }

	/** Access called for RDWR but we can't open it RDWR? **/
	if ((obj->Mode & O_ACCMODE) == O_RDWR && fdAccess(inf->RealPathname,W_OK) < 0)
	    {
	    obj->Mode &= ~O_ACCMODE;
	    obj->Mode |= O_RDONLY;
	    }

	/** Release the resources we temporarily used. **/
        nmFree(tmp_path, sizeof(Pathname));

	/** Cache the node if it was a new one. **/
	if (is_new_node)
	    {
	    xhAdd(&UXD_INF.NodesByOSPath, node->OSPath, (void*)node);
	    }
	inf->Node = node;

    return (void*)inf;
    }


/*** uxdClose - close an open file or directory.
 ***/
int
uxdClose(void* inf_v, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);

	/** If directory ... **/
	if (inf->Flags & UXD_F_ISDIR)
	    {
	    if (inf->Flags & UXD_F_ISOPEN) closedir(inf->Direc);
	    }
	else
	    {
	    if (inf->Flags & UXD_F_ISOPEN) fdClose(inf->fd,0);
	    }

	/** Free the structure **/
	snWriteNode(inf->Obj->Prev, inf->Node->SnNode);
	inf->Node->SnNode->OpenCnt--;
	nmFree(inf,sizeof(UxdData));

    return 0;
    }


/*** uxdCreate - create a new file without actually opening that 
 *** file.
 ***/
int
uxdCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

	/** Call open, then close. **/
	obj->Mode |= O_CREAT;
	inf = uxdOpen(obj,mask,systype,usrtype,oxt);
	if (!inf) return -1;
	uxdClose(inf,oxt);

    return 0;
    }


/*** uxdDeleteObj - delete a file or directory for an open
 *** object.  This will replace uxdDelete in the future.
 ***/
int
uxdDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);

	/** Directory? **/
	if (inf->Flags & UXD_F_ISDIR)
	    {
	    if (rmdir(inf->RealPathname) < 0) 
	        {
		mssErrorErrno(1,"UXD","Could not delete directory");
		uxdClose((void*)inf, oxt);
		return -1;
		}
	    }
	else
	    {
	    if (unlink(inf->RealPathname) < 0)
	        {
		mssErrorErrno(1,"UXD","Could not delete file");
		uxdClose((void*)inf, oxt);
		return -1;
		}
	    }
	
    return uxdClose((void*)inf, oxt);
    }


/*** uxdDelete - delete an existing file or directory.
 ***/
int
uxdDelete(pObject obj, pObjTrxTree* oxt)
    {
    pUxdData inf;

	/** Make sure it exists first **/
	obj->Mode &= ~O_CREAT;
	inf = (pUxdData)uxdOpen(obj,0600,NULL,"system/uxfile",oxt);
	if (!inf) 
	    {
	    mssErrorErrno(0,"UXD","Could not delete file/directory");
	    return -1;
	    }

	uxdDeleteObj(inf,oxt);

    return 0;
    }


/*** uxdRead - read from a file.  Fails on a directory.
 ***/
int
uxdRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);
    int rval;

	/** Verify not a directory **/
	if (inf->Flags & UXD_F_ISDIR) 
	    {
	    mssError(1,"UXD","UNIX directories do not have content");
	    return -1;
	    }

	/** Ok, do we need to open the dumb thing? **/
	if (!(inf->Flags & UXD_F_ISOPEN))
	    {
	    inf->fd = fdOpen(inf->RealPathname, inf->Obj->Mode, inf->Mask);
	    if (!(inf->fd)) 
	        {
		mssErrorErrno(1,"UXD","Could not read from file");
		return -1;
		}
	    inf->Flags |= UXD_F_ISOPEN;
	    }

	/** Now, do the read. **/
	rval = fdRead(inf->fd, buffer, maxcnt, offset, flags);

    return rval;
    }


/*** uxdWrite - write to a file.  Fails on directories.
 ***/
int
uxdWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);
    int rval;

	/** Verify not a directory **/
	if (inf->Flags & UXD_F_ISDIR)
	    {
	    mssError(1,"UXD","UNIX directories do not have content");
	    return -1;
	    }

	/** Ok, do we need to open the dumb thing? **/
	if (!(inf->Flags & UXD_F_ISOPEN))
	    {
	    inf->fd = fdOpen(inf->RealPathname, inf->Obj->Mode, inf->Mask);
	    if (!(inf->fd))
	        {
		mssErrorErrno(1,"UXD","Could not read from file");
		return -1;
		}
	    inf->Flags |= UXD_F_ISOPEN;
	    }

	/** Now, do the write. **/
	rval = fdWrite(inf->fd, buffer, cnt, offset, flags);

    return rval;
    }


/*** uxdOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries, and adding intelligence here wouldn't
 *** help performance since the filesystem doesn't support such queries.
 *** So, we leave the query matching logic to the ObjectSystem management
 *** layer in this case.
 ***/
void*
uxdOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);
    pUxdQuery qy;

	/** Verify it is a directory **/
	if (!(inf->Flags & UXD_F_ISDIR)) 
	    {
	    mssError(1,"UXD","Plain files do not support queries");
	    return NULL;
	    }

	/** Open the thing if it hasn't been opened yet. **/
	if (!(inf->Flags & UXD_F_ISOPEN))
	    {
	    inf->Direc = opendir(inf->RealPathname);
	    if (!(inf->Direc)) 
	        {
		mssErrorErrno(1,"UXD","Could not open directory for query");
		return NULL;
		}
	    inf->Flags |= UXD_F_ISOPEN;
	    }
	else
	    {
	    /** Rewind the directory... **/
	    seekdir(inf->Direc, 0);
	    }

	/** Allocate the query structure **/
	qy = (pUxdQuery)nmMalloc(sizeof(UxdQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(UxdQuery));
	qy->File = inf;
    
    return (void*)qy;
    }


/*** uxdQueryFetch - get the next directory entry as an open object.
 ***/
void*
uxdQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pUxdQuery qy = ((pUxdQuery)(qy_v));
    pUxdData inf;
    struct dirent *d;

	/** Read the next item. **/
      GET_NEXT_DIRITEM:
	d = readdir(qy->File->Direc);
	if (!d) return NULL;

	/** If we're at root and just read '..', get another. **/
	/*if (!strcmp(d->d_name,"..") && !strcmp(qy->File->Obj->Pathname->Pathbuf,".")) 
	    d=readdir(qy->File->Direc);
	if (!d) return NULL;*/

	/** GRB - Don't return ., .., .type, .content, and .annotation **/
	if (d->d_name[0] == '.' && (d->d_name[1] == '\0' || 
	    (d->d_name[1] == '.' && d->d_name[2] == '\0') ||
	    !strcmp(d->d_name+1,"type") || !strcmp(d->d_name+1, "content") ||
	    !strcmp(d->d_name+1,"annotation")))
	    {
	    goto GET_NEXT_DIRITEM;
	    }

	/** Build the filename. **/
	if (strlen(d->d_name) + 1 + strlen(qy->File->RealPathname) >= OBJSYS_MAX_PATH) 
	    {
	    mssError(1,"UXD","Query result pathname exceeds internal limits");
	    return NULL;
	    }
	inf = (pUxdData)nmMalloc(sizeof(UxdData));
	memset(inf,0,sizeof(UxdData));
	snprintf(inf->RealPathname, OBJSYS_MAX_PATH, "%s/%s",qy->File->RealPathname,d->d_name);
	stat(inf->RealPathname, &(inf->Fileinfo));
	if (S_ISDIR(inf->Fileinfo.st_mode)) inf->Flags |= UXD_F_ISDIR;
	inf->Node = qy->File->Node;
	inf->Node->SnNode->OpenCnt++;
	inf->Obj = obj;

    return (void*)inf;
    }


/*** uxdQueryClose - close the query.
 ***/
int
uxdQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(UxdQuery));

    return 0;
    }


/*** uxdGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
uxdGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    /*pUxdData inf = UXD(inf_v);*/
    int i;

    	/** If name, it's a string. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** If content-type, it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") || !strcmp(attrname,"outer_type")) 
	    return DATA_T_STRING;

	/** If annotation, it's a string. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Search for it **/
	for(i=0;attr_inf[i].Name;i++) 
	    {
	    if (!strcmp(attrname,attr_inf[i].Name)) return attr_inf[i].Type;
	    }

	mssError(1,"UXD","Invalid file/directory attribute");

    return -1;
    }


/*** uxdGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
uxdGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pUxdData inf = UXD(inf_v);
    struct passwd *pw;
    struct group *gr;
    struct tm *t;
    char* ptr;
    pUxdAnnotation ua;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    ptr = strrchr(inf->RealPathname,'/');
	    /*ptr = obj_internal_PathPart(inf->Obj->Pathname,inf->Obj->SubPtr + inf->Obj->SubCnt - 1, 1);*/
	    if (ptr)
	        {
		strcpy(inf->Buffer,ptr+1);
	        val->String = inf->Buffer;
		}
	    else
	        {
	        val->String = "/";
		}
	    /*obj_internal_PathPart(inf->Obj->Pathname,0,0);*/
	    }
	else if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    ptr = strrchr(inf->RealPathname,'/');
	    if (!ptr) return -1;
	    *ptr = '\0';
	    uxd_internal_LoadAnnot(inf->RealPathname);
	    *ptr = '/';
	    ua = (pUxdAnnotation)xhLookup(&UXD_INF.Annotations, inf->RealPathname);
	    if (!ua) val->String = "";
	    else val->String = ua->Annotation;
	    }
	else if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Flags & UXD_F_ISDIR) 
	        {
		val->String = "system/void";
		}
	    else 
	        {
		ptr = uxd_internal_DirType(inf->RealPathname);
		if (ptr) val->String = ptr;
		else val->String = "application/octet-stream";
		}
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Flags & UXD_F_ISDIR) val->String = "system/directory";
	    else val->String = "system/file";
	    }
	else if (!strcmp(attrname,"owner"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (!(inf->UsrName[0]))
		{
		stat(inf->RealPathname, &(inf->Fileinfo));
		pw = getpwuid(inf->Fileinfo.st_uid);
		if (!pw) snprintf(inf->UsrName,16,"%d",inf->Fileinfo.st_uid);
		else snprintf(inf->UsrName,16,"%s",pw->pw_name);
		}
	    val->String = inf->UsrName;
	    }
	else if (!strcmp(attrname,"group"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (!(inf->GrpName[0]))
		{
		stat(inf->RealPathname, &(inf->Fileinfo));
		gr = getgrgid(inf->Fileinfo.st_gid);
		if (!gr) snprintf(inf->GrpName,16,"%d",inf->Fileinfo.st_gid);
		else snprintf(inf->GrpName,16,"%s",gr->gr_name);
		}
	    val->String = inf->GrpName;
	    }
	else if (!strcmp(attrname,"size"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    stat(inf->RealPathname, &(inf->Fileinfo));
	    val->Integer = inf->Fileinfo.st_size;
	    }
	else if (!strcmp(attrname,"permissions"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    stat(inf->RealPathname, &(inf->Fileinfo));
	    val->Integer = inf->Fileinfo.st_mode;
	    }
	else if (!strcmp(attrname,"last_modification"))
	    {
	    if (datatype != DATA_T_DATETIME)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be datetime)", attrname);
		return -1;
		}
	    if (inf->MTime.Value == 0)
		{
		stat(inf->RealPathname, &(inf->Fileinfo));
		t = localtime(&(inf->Fileinfo.st_mtime));
		inf->MTime.Part.Second = t->tm_sec;
		inf->MTime.Part.Minute = t->tm_min;
		inf->MTime.Part.Hour = t->tm_hour;
		inf->MTime.Part.Day = t->tm_mday - 1;
		inf->MTime.Part.Month = t->tm_mon;
		inf->MTime.Part.Year = t->tm_year;
		}
	    val->DateTime = &(inf->MTime);
	    }
	else if (!strcmp(attrname,"last_change"))
	    {
	    if (datatype != DATA_T_DATETIME)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be datetime)", attrname);
		return -1;
		}
	    if (inf->CTime.Value == 0)
		{
		stat(inf->RealPathname, &(inf->Fileinfo));
		t = localtime(&(inf->Fileinfo.st_ctime));
		inf->CTime.Part.Second = t->tm_sec;
		inf->CTime.Part.Minute = t->tm_min;
		inf->CTime.Part.Hour = t->tm_hour;
		inf->CTime.Part.Day = t->tm_mday - 1;
		inf->CTime.Part.Month = t->tm_mon;
		inf->CTime.Part.Year = t->tm_year;
		}
	    val->DateTime = &(inf->CTime);
	    }
	else if (!strcmp(attrname,"last_access"))
	    {
	    if (datatype != DATA_T_DATETIME)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be datetime)", attrname);
		return -1;
		}
	    if (inf->ATime.Value == 0)
		{
		stat(inf->RealPathname, &(inf->Fileinfo));
		t = localtime(&(inf->Fileinfo.st_atime));
		inf->ATime.Part.Second = t->tm_sec;
		inf->ATime.Part.Minute = t->tm_min;
		inf->ATime.Part.Hour = t->tm_hour;
		inf->ATime.Part.Day = t->tm_mday - 1;
		inf->ATime.Part.Month = t->tm_mon;
		inf->ATime.Part.Year = t->tm_year;
		}
	    val->DateTime = &(inf->ATime);
	    }
	else
	    {
	    mssError(1,"UXD","Invalid file/directory attribute");
	    return -1;
	    }

    return 0;
    }


/*** uxdGetNextAttr - get the next attribute name for this object.
 ***/
char*
uxdGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pUxdData inf = UXD(inf_v);

	/** Get the next attr from the list unless last one already **/
	if (attr_inf[inf->CurAttr].Name == NULL) return NULL;

    return attr_inf[inf->CurAttr++].Name;
    }


/*** uxdGetFirstAttr - get the first attribute name for this object.
 ***/
char*
uxdGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pUxdData inf = UXD(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = uxdGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** uxdSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
uxdSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pUxdData inf = UXD(inf_v);

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /*if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	    if (strlen(inf->Obj->Pathname->Pathbuf) - 
	        strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		strlen(val->String) + 1 > 255)
		{
		mssError(1,"UXD","SetAttr 'name': too long for internal representation");
		return -1;
		}
	    strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	    strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	    if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
	        {
		mssErrorErrno(1,"UXD","Could not rename file/directory");
		return -1;
		}
	    strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);*/
	    }
	else if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    uxd_internal_ModifyAnnot(inf, val->String);
	    }
	else if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    mssError(1,"UXD","Cannot modify file/directory's content type");
	    return -1;
	    }
	else if (!strcmp(attrname,"owner"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    }
	else if (!strcmp(attrname,"group"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    }
	else if (!strcmp(attrname,"size"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    mssError(1,"UXD","Cannot modify file/directory's size");
	    return -1;
	    }
	else if (!strcmp(attrname,"permissions"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"UXD","Type mismatch accessing attribute '%s' (should be integer)", attrname);
		return -1;
		}
	    }
	else if (!strcmp(attrname,"last_modification"))
	    {
	    }
	else if (!strcmp(attrname,"last_change"))
	    {
	    }
	else if (!strcmp(attrname,"last_access"))
	    {
	    }
	else
	    {
	    mssError(1,"UXD","Invalid file/directory attribute");
	    return -1;
	    }

    return 0;
    }


/*** uxdAddAttr - add an attribute to an object.  This doesn't work for
 *** unix filesystem objects, so we just deny the request.
 ***/
int
uxdAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** uxdOpenAttr - open an attribute as if it were an object with 
 *** content.  The UNIX filesystem objects have no attributes that are
 *** suitable for this.
 ***/
void*
uxdOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxdGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
uxdGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxdGetNextMethod -- same as above.  Always fails. 
 ***/
char*
uxdGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** uxdExecuteMethod - No methods to execute, so this fails.
 ***/
int
uxdExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** uxdInfo - Find additional information about the object.  We won't
 *** actually try to find out if files actually exist in a directory,
 *** but we'll say that they *can* exist in it.
 ***/
int
uxdInfo(void* inf_v, pObjectInfo info)
    {
    pUxdData inf = UXD(inf_v);

	/** Basic settings for all uxd objects **/
	info->Flags = OBJ_INFO_F_CANT_ADD_ATTR;

	/** Is it a directory? **/
	if (inf->Flags & UXD_F_ISDIR)
	    {
	    info->Flags |= (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_HAVE_CONTENT |
		    OBJ_INFO_F_NO_CONTENT);
	    }
	else
	    {
	    info->Flags |= (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ | 
		    OBJ_INFO_F_CAN_SEEK_FULL | OBJ_INFO_F_CAN_HAVE_CONTENT | 
		    OBJ_INFO_F_HAS_CONTENT);
	    }

    return 0;
    }


/*** uxdInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
uxdInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&UXD_INF,0,sizeof(UXD_INF));
	xhInit(&UXD_INF.OpenFiles,255,0);
	xhInit(&UXD_INF.Annotations,511,0);
	xhInit(&UXD_INF.LoadedAnnot,255,0);
	xhInit(&UXD_INF.NodesByOSPath,255,0);
	xhInit(&UXD_INF.DirTypes,255,0);

	/** Setup the structure **/
	strcpy(drv->Name,"UXD - UNIX Filesystem Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/file");
	xaAddItem(&(drv->RootContentTypes),"system/directory");
	xaAddItem(&(drv->RootContentTypes),"system/uxfile");
	xaAddItem(&(drv->RootContentTypes),"system/uxfileref");

	/** Setup the function references. **/
	drv->Open = uxdOpen;
	drv->Close = uxdClose;
	drv->Create = uxdCreate;
	drv->Delete = uxdDelete;
	drv->DeleteObj = uxdDeleteObj;
	drv->OpenQuery = uxdOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = uxdQueryFetch;
	drv->QueryClose = uxdQueryClose;
	drv->Read = uxdRead;
	drv->Write = uxdWrite;
	drv->GetAttrType = uxdGetAttrType;
	drv->GetAttrValue = uxdGetAttrValue;
	drv->GetFirstAttr = uxdGetFirstAttr;
	drv->GetNextAttr = uxdGetNextAttr;
	drv->SetAttrValue = uxdSetAttrValue;
	drv->AddAttr = uxdAddAttr;
	drv->OpenAttr = uxdOpenAttr;
	drv->GetFirstMethod = uxdGetFirstMethod;
	drv->GetNextMethod = uxdGetNextMethod;
	drv->ExecuteMethod = uxdExecuteMethod;
	drv->Info = uxdInfo;

	nmRegister(sizeof(UxdData),"UxdData");
	nmRegister(sizeof(UxdQuery),"UxdQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

