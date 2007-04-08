#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "expression.h"
#include "report.h"
#include "prtmgmt.h"
#include "cxlib/mtsession.h"

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
/* Module: 	objdrv_audio.c          				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 4, 2001 					*/
/* Description:	Audio clip player objectsystem driver.  Interfaces with	*/
/*		the Linux OSS audio subsystem via /dev/dsp and has a	*/
/*		method which is used to play any audio object in the 	*/
/*		objectsystem.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_audio.c,v 1.6 2007/04/08 03:52:00 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_audio.c,v $

    $Log: objdrv_audio.c,v $
    Revision 1.6  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.5  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.4  2004/06/23 21:33:55  mmcgill
    Implemented the ObjInfo interface for all the drivers that are currently
    a part of the project (in the Makefile, in other words). Authors of the
    various drivers might want to check to be sure that I didn't botch any-
    thing, and where applicable see if there's a neat way to keep track of
    whether or not an object actually has subobjects (I did not set this flag
    unless it was immediately obvious how to test for the condition).

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

    Revision 1.2  2002/08/10 02:09:45  gbeeley
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

    Revision 1.1  2001/11/12 20:43:44  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    char	ContentType[64];
    int		Flags;
    pObject	Obj;
    int		Mask;
    pSnNode	Node;
    int		AttrID;
    pStructInf	AttrOverride;
    }
    AudData, *pAudData;

#define AUD(x) ((pAudData)(x))

/*** Globals. ***/
struct
    {
    pFile	MasterFD;
    pFile	SlaveFD;
    pThread	tid;
    pObject	PlayObj;
    int		Childpid;
    }
    AUD_INF;


/*** audOpen - open a new report for report generation.
 ***/
void*
audOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pAudData inf;
    char* node_path;
    pSnNode node = NULL;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return NULL;
	    }

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** try to open it **/
	node = snReadNode(obj->Prev);

	/** If node access failed, quit out. **/
	if (!node)
	    {
	    mssError(0,"AUD","Could not open audio device node object");
	    return NULL;
	    }

	/** Allocate the structure **/
	inf = (pAudData)nmMalloc(sizeof(AudData));
	memset(inf, 0, sizeof(AudData));
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!inf) return NULL;
	inf->Obj = obj;
	inf->Mask = mask;
	inf->AttrOverride = stCreateStruct(NULL,NULL);
	memccpy(inf->ContentType, usrtype, 0, 63);
	inf->ContentType[63] = 0;

	/** Set object params. **/
	inf->Node = node;
	inf->Node->OpenCnt++;
	obj->SubCnt = 1;

    return (void*)inf;
    }


/*** audClose - close an open file or directory.
 ***/
int
audClose(void* inf_v, pObjTrxTree* oxt)
    {
    pAudData inf = AUD(inf_v);

	/** Release the memory **/
	inf->Node->OpenCnt --;
	stFreeInf(inf->AttrOverride);
	nmFree(inf,sizeof(AudData));

    return 0;
    }


/*** audCreate - create a new file without actually opening that 
 *** file.
 ***/
int
audCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = audOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	audClose(inf, oxt);

    return 0;
    }


/*** audDelete - delete an existing file or directory.
 ***/
int
audDelete(pObject obj, pObjTrxTree* oxt)
    {
    char* node_path;
    pSnNode node;

    	/** This driver doesn't support sub-nodes.  Yet.  Check for that. **/
	if (obj->SubPtr != obj->Pathname->nElements)
	    {
	    return -1;
	    }

	/** Determine node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);
	node = snReadNode(obj->Prev);
	if (!node) 
	    {
	    mssError(0,"AUD","Could not open audio node");
	    return -1;
	    }

	/** Delete the thing. **/
	if (snDelete(node) < 0) 
	    {
	    mssError(0,"AUD","Could not delete audio node");
	    return -1;
	    }

    return 0;
    }


/*** audRead - Attempt to read from the report generator thread, and start
 *** that thread if it hasn't been started yet...
 ***/
int
audRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pAudData inf = AUD(inf_v);*/
    mssError(1,"AUD","Cannot read from an audio object yet.");
    return -1;
    }


/*** audWrite - This fails for reports.
 ***/
int
audWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pAudData inf = AUD(inf_v);*/
    mssError(1,"AUD","Cannot write to an audio object yet.");
    return -1;
    }


/*** audOpenQuery - open a directory query.  We don't support directory queries yet.
 ***/
void*
audOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    return (void*)NULL;
    }


/*** audQueryFetch - get the next directory entry as an open object.
 ***/
void*
audQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pAudData inf;
    /*pAudQuery qy = ((pAudQuery)(qy_v));
    pObject llobj = NULL;
    char* objname = NULL;
    int cur_id = -1;*/

    	inf = NULL;

    return (void*)inf;
    }


/*** audQueryClose - close the query.
 ***/
int
audQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    /*pAudQuery qy = ((pAudQuery)(qy_v));*/

    return -1;
    }


/*** audGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
audGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Annotation, string. **/
	if (!strcmp(attrname, "annotation")) return DATA_T_STRING;

    return -1;
    }


/*** audGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
audGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pAudData inf = AUD(inf_v);

	/** right now, all audio device attrs are strings **/
	if (datatype != DATA_T_STRING)
	    {
	    mssError(1,"AUD","Type mismatch accessing attribute '%s'", attrname);
	    return -1;
	    }

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    /* *((char**)val) = inf->Node->Data->Name;*/
	    val->String = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->Pathname->nElements - 1, 0);
	    obj_internal_PathPart(inf->Obj->Pathname,0,0);
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "system/audio-device";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
        if (!strcmp(attrname,"annotation"))
            {
            val->String = "";
            return 0;
            }

	mssError(1,"AUD","Could not find requested device attribute '%s'", attrname);

    return -1;
    }


/*** audGetNextAttr - get the next attribute name for this object.
 ***/
char*
audGetNextAttr(void* inf_v, pObjTrxTree *oxt)
    {
    /*pAudData inf = AUD(inf_v);
    int i;
    pStructInf subinf;*/

    return NULL;
    }


/*** audGetFirstAttr - get the first attribute name for this object.
 ***/
char*
audGetFirstAttr(void* inf_v, pObjTrxTree *oxt)
    {
    pAudData inf = AUD(inf_v);

    	/** Reset the attribute cnt. **/
	inf->AttrID = 0;

    return audGetNextAttr(inf_v, oxt);
    }


/*** audSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
audSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree *oxt)
    {
    pAudData inf = AUD(inf_v);
    pStructInf find_inf;
    int type;
    int n;

	/** right now, all audio device attrs are strings **/
	if (datatype != DATA_T_STRING)
	    {
	    mssError(1,"AUD","Type mismatch setting attribute '%s'", attrname);
	    return -1;
	    }

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Node->Data)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"AUD","SetAttr 'name': name too long for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssErrorErrno(1,"AUD","SetAttr 'name': could not rename report node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    strcpy(inf->Node->Data->Name,val->String);
	    return 0;
	    }
	
	/** Content-type?  can't set that **/
	if (!strcmp(attrname,"content_type")) 
	    {
	    mssError(1,"AUD","Illegal attempt to modify content type");
	    return -1;
	    }

	/** Otherwise, try to set top-level attrib inf value in the override struct **/
	/** First, see if the thing exists in the original inf struct. **/
	type = audGetAttrType(inf_v, attrname, oxt);
	if (type < 0) return -1;

	/** Now, look for it in the override struct. **/
	find_inf = stLookup(inf->AttrOverride, attrname);

	/** If not found, add a new one **/
	if (!find_inf)
	    {
	    find_inf = stAddAttr(inf->AttrOverride, attrname);
	    n = 0;
	    }

	/** Set the value. **/
	if (find_inf)
	    {
	    stSetAttrValue(find_inf, type, val, 0);
	    return 0;
	    }

    return -1;
    }


/*** audAddAttr - add an attribute to an object. Fails for this.
 ***/
int
audAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree *oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    return -1;
    }


/*** audOpenAttr - open an attribute as an object.  Fails.
 ***/
void*
audOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree *oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    return NULL;
    }


/*** audGetFirstMethod -- no methods.  Fails.
 ***/
char*
audGetFirstMethod(void* inf_v, pObjTrxTree *oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    return "Play";
    }


/*** audGetNextMethod -- no methods here.  Fails.
 ***/
char*
audGetNextMethod(void* inf_v, pObjTrxTree *oxt)
    {
    /*pAudData inf = AUD(inf_v);*/

    return NULL;
    }


/*** aud_internal_Player - worker thread to play an object.
 ***/
void
aud_internal_Player(void* v)
    {
    /*pAudData inf = AUD(v);*/
    int pipefd[2];
    char buf[2048];
    int rval, wval;
	
	thSetName(NULL,"Audio Player");
        /** Ok, now open a pipe to the 'sox' command. **/
        /*socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);*/
	pipe(pipefd);
        if ((AUD_INF.Childpid=fork()))
            {
            /** Close the child's end of the pipe, since we don't use it in the parent **/
            close(pipefd[0]);

            /** Open an fd to the pipe **/
            AUD_INF.MasterFD = fdOpenFD(pipefd[1],O_RDONLY);

	    /** Start writing object data to the pipe. **/
	    while(1)
	    	{
		if (AUD_INF.PlayObj)
		    rval = objRead(AUD_INF.PlayObj, buf, 2048, 0, 0);
		else
		    break;
		if (rval <= 0) break;
		if (AUD_INF.MasterFD) 
		    wval = fdWrite(AUD_INF.MasterFD, buf, rval, 0, 0);
		else
		    break;
		if (wval < rval) break;
		}
	
	    /** Clean up. **/
	    if (AUD_INF.MasterFD) fdClose(AUD_INF.MasterFD, 0);
	    AUD_INF.MasterFD = NULL;
	    if (AUD_INF.PlayObj) objClose(AUD_INF.PlayObj);
	    AUD_INF.PlayObj = NULL;

            /** Wait on sox's completion. **/
            wait(NULL);
	    if (AUD_INF.Childpid) AUD_INF.Childpid = 0;
	    AUD_INF.tid = NULL;
            }
        else
            {
            /** Close the parent's end of the pipe, since we don't use it in the child **/
            close(pipefd[1]);

            /** Make the pipe sox's standard input and output **/
            close(0);
            /*close(1);
            close(2);*/
            dup(pipefd[0]);
            /*dup(pipefd[0]);
            dup(pipefd[0]);*/

            /** Execute the sox process, exit if failed. **/
            execl("/usr/bin/sox","sox","-t","wav","-","-t","ossdsp","/dev/dsp",NULL);
            /*execlp("dd","dd","of=/tmp/tmp.wav",NULL);*/
	    printf("sox exec failed!\n");
            _exit(0);
            }

    thExit();
    }


/*** audExecuteMethod - call a method.  Our main functionality is the Play
 *** method for playing an audio file.
 ***/
int
audExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree *oxt)
    {
    pAudData inf = AUD(inf_v);
    char* playfile;

    	/** A method we know about? **/
	if (!strcmp(methodname,"Play"))
	    {
	    /** Already playing one? **/
	    if (AUD_INF.PlayObj)
	    	{
		if (AUD_INF.tid) thKill(AUD_INF.tid);
		AUD_INF.tid = NULL;
		if (AUD_INF.Childpid && AUD_INF.Childpid != 1) 
		    {
		    kill(AUD_INF.Childpid,9);
		    wait(NULL);
		    AUD_INF.Childpid = 0;
		    }
		if (AUD_INF.MasterFD) fdClose(AUD_INF.MasterFD, 0);
		AUD_INF.MasterFD = NULL;
		objClose(AUD_INF.PlayObj);
		AUD_INF.PlayObj = NULL;
		}

	    /** Open the object to play **/
	    playfile = param->String;
	    AUD_INF.PlayObj = objOpen(inf->Obj->Session, playfile, O_RDONLY, 0600, "application/octet-stream");
	    if (!AUD_INF.PlayObj)
	    	{
		mssError(0,"AUD","Could not open requested audio file");
		return -1;
		}
	
	    /** play the file **/
	    AUD_INF.tid = thCreate(aud_internal_Player, 0, NULL);
	    return 0;
	    }


    return -1;
    }


/*** audInfo - Find additional information about the object.  We won't
 *** actually try to find out if files actually exist in a directory,
 *** but we'll say that they *can* exist in it.
 ***/
int
audInfo(void* inf_v, pObjectInfo info)
    {
    /*pAudData inf = AUD(inf_v);*/

	info->Flags |= (OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | 
		OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT | 
		OBJ_INFO_F_NO_CONTENT | OBJ_INFO_F_SUPPORTS_INHERITANCE);
	return 0;
    }

/*** audInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem management layer.
 ***/
int
audInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&AUD_INF,0,sizeof(AUD_INF));

	/** Setup the structure **/
	strcpy(drv->Name,"AUD - Audio File Player Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/audio-device");

	/** Setup the function references. **/
	drv->Open = audOpen;
	drv->Close = audClose;
	drv->Create = audCreate;
	drv->Delete = audDelete;
	drv->OpenQuery = audOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = audQueryFetch;
	drv->QueryClose = audQueryClose;
	drv->Read = audRead;
	drv->Write = audWrite;
	drv->GetAttrType = audGetAttrType;
	drv->GetAttrValue = audGetAttrValue;
	drv->GetFirstAttr = audGetFirstAttr;
	drv->GetNextAttr = audGetNextAttr;
	drv->SetAttrValue = audSetAttrValue;
	drv->AddAttr = audAddAttr;
	drv->OpenAttr = audOpenAttr;
	drv->GetFirstMethod = audGetFirstMethod;
	drv->GetNextMethod = audGetNextMethod;
	drv->ExecuteMethod = audExecuteMethod;
	drv->Info = audInfo;
	/*drv->PresentationHints = audPresentationHints*/;

	nmRegister(sizeof(AudData),"AudData");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

