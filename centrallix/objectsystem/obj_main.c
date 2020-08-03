#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "centrallix.h"
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "expression.h"
#include "cxlib/xhandle.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include <time.h>
#include "cxlib/xhandle.h"

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
/*		--> obj_main.c: contains the general functionality and	*/
/*		admin components of the objectsystem.			*/
/************************************************************************/


/*** OSML Globals ***/
OSYS_t OSYS;

char* obj_type_names[OBJ_TYPE_NAMES_CNT];

/*** objTypeID() - return the numeric type given a name
 ***/
int
objTypeID(char* typename)
    {
    int i;

	for(i=0;i<OBJ_TYPE_NAMES_CNT;i++) 
	    if (obj_type_names[i] && !strcmp(obj_type_names[i],typename))
		return i;

    return -1;
    }


/*** obj_internal_BuildIsA - scan the content type registry (from types.cfg)
 *** and determine what types are related to what other types.  This is an
 *** optimization that allows object open()s to run faster.  When the system
 *** runs as a .CGI, it might be beneficial to disable this routine and use
 *** a more intelligent IsA routine instead (because responsiveness depends
 *** on startup time + multi object-open timing, instead of just on object-
 *** open timing).
 ***/
int
obj_internal_BuildIsA()
    {
    int i,j,k,level,num_added,rel_level, ind_rel_level;
    pContentType type,related_type,indirect_rel_type;

        /** Add direct relationships for each type. **/
	for(i=0;i<OSYS.TypeList.nItems;i++)
	    {
	    type = (pContentType)(OSYS.TypeList.Items[i]);
	    for(j=0;j<type->IsA.nItems;j++)
	        {
		related_type = (pContentType)(type->IsA.Items[j]);
		xaAddItem(&type->RelatedTypes, (void*)related_type);
		xaAddItem(&type->RelationLevels, (void*)1);
		xaAddItem(&related_type->RelatedTypes, (void*)type);
		xaAddItem(&related_type->RelationLevels, (void*)-1);
		}
	    }

    	/** Loop through the types, adding relationships until we can't add any more **/
	level = 1;
	do  {
	    num_added = 0;
	    for(i=0;i<OSYS.TypeList.nItems;i++)
	        {
	        type = (pContentType)(OSYS.TypeList.Items[i]);

		/** For each relationship of level <n>, scan through for level 1 ties, or **/
		/** for each relationship of level <-n>, scan through for level -1 ties **/
		for(j=0;j<type->RelatedTypes.nItems;j++)
		    {
		    rel_level = (intptr_t)(type->RelationLevels.Items[j]);
		    related_type = (pContentType)(type->RelatedTypes.Items[j]);
		    if (rel_level == level || rel_level == -level)
		        {
			for(k=0;k<related_type->RelatedTypes.nItems;k++)
			    {
		    	    ind_rel_level = (intptr_t)(related_type->RelationLevels.Items[k]);
		    	    indirect_rel_type = (pContentType)(related_type->RelatedTypes.Items[k]);
			    if ((ind_rel_level == 1 && rel_level == level) || (ind_rel_level == -1 && rel_level == -level))
			        {
				num_added++;
				xaAddItem(&type->RelatedTypes, (void*)indirect_rel_type);
				xaAddItem(&type->RelationLevels, (void*)(intptr_t)(ind_rel_level + rel_level));
				}
			    }
			}
		    }
		}
	    level++;
	    }
	    while (num_added);

    return 0;
    }


/*** The following three functions are used for the evaluation of the 
 *** expressions in the types.cfg file.  The "param object" in this case
 *** is simply a character string, and these functions act accordingly.
 ***/

int obj_internal_TypeFnName(void* obj, char* name)
    {
    if (!strcmp(name,"name")) return DATA_T_STRING;
    else return -1;
    }
int obj_internal_GetFnName(void* obj, char* name, int type, pObjData val)
    {
    if (type != DATA_T_STRING) return -1;
    if (!strcmp(name,"name") && obj) 
        {
	if (obj)
	    {
	    val->String = (char*)obj;
	    return 0;
	    }
	else
	    { 
	    return 1;
	    }
	}
    else 
        return -1;
    }
int obj_internal_SetFnName(void* obj, char* name, int type, pObjData val) { return -1; }


int 
obj_types_GetAttrValue(void* ct_v, char* name, char* attrname, void* val_v)
    {
    pContentType ct = (pContentType)ct_v;
    pObjData pod = POD(val_v);

	if (!strcmp(attrname, "type_name")) 
	    pod->String = ct->Name;
	else if (!strcmp(attrname, "type_description")) 
	    pod->String = ct->Description;
	else if (!strcmp(attrname, "parent_type")) 
	    {
	    if (ct->Flags & CT_F_TOPLEVEL)
		return 1;
	    pod->String = ((pContentType)(ct->IsA.Items[0]))->Name;
	    }
	else
	    return -1;

    return 0;
    }


/*** objInitialize -- start up the objectsystem and initialize the various
 *** components thereof.
 ***/
int
objInitialize()
    {
    pFile fd;
    int t,i,a;
    pLxSession s;
    pContentType ct,parent_ct;
    char* ptr;
    char* filename;
    char sysbuf[64];
    pSysInfoData si;

	/** Zero the globals **/
	memset(&OSYS, 0, sizeof(OSYS));

	/** Initialize the arrays and hash tables **/
	xaInit(&(OSYS.OpenSessions), 256);
	xhInit(&(OSYS.TypeExtensions), 257, 0);
	xhInit(&(OSYS.DriverTypes), 257, 0);
	/*xhqInit(&(OSYS.DirectoryCache), 256, 0, 509, obj_internal_DiscardDC, 0);*/
	/*xaInit(&(OSYS.DirectoryQueue), 256);*/
	xaInit(&(OSYS.Drivers), 256);
	xhInit(&(OSYS.Types), 257, 0);
	xaInit(&(OSYS.TypeList), 256);
	xhnInitContext(&(OSYS.SessionHandleCtx));
	xhInit(&(OSYS.NotifiesByPath), 1027, 0);
	OSYS.PathID = 0;

	/** Setup the data type names list **/
	for(i=0;i<OBJ_TYPE_NAMES_CNT;i++) obj_type_names[i] = "(unknown)";
	obj_type_names[DATA_T_INTEGER] = "integer";
	obj_type_names[DATA_T_STRING] = "string";
	obj_type_names[DATA_T_DATETIME] = "datetime";
	obj_type_names[DATA_T_MONEY] = "money";
	obj_type_names[DATA_T_DOUBLE] = "double";
	obj_type_names[DATA_T_UNAVAILABLE] = "unavailable";
	obj_type_names[DATA_T_INTVEC] = "intvec";
	obj_type_names[DATA_T_STRINGVEC] = "stringvec";
	obj_type_names[DATA_T_CODE] = "code";
	obj_type_names[DATA_T_ARRAY] = "array";
	obj_type_names[DATA_T_BINARY] = "binary";

	chdir("/");

	/** Load the types.cfg file **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "types_config"), NULL, &filename, 0) < 0)
	    {
	    filename = OBJSYS_DEFAULT_TYPES_CFG;
	    }
	fd = fdOpen(filename, O_RDONLY, 0600);
	if (!fd)
	    {
	    perror(filename);
	    exit(1);
	    }
	s = mlxOpenSession(fd, MLX_F_EOF | MLX_F_EOL | MLX_F_POUNDCOMM);
	if (!s)
	    {
	    printf("could not open lexer session on '%s'", filename);
	    exit(1);
	    }
	while((t = mlxNextToken(s)) != MLX_TOK_EOF)
	    {
	    if (t==MLX_TOK_ERROR) 
		{
		printf("error while loading '%s'\n", filename);
		break;
		}
	    if (t==MLX_TOK_EOL) continue;

	    /** Get the content-type **/
	    if (t!=MLX_TOK_STRING) break;
	    ct = (pContentType)nmMalloc(sizeof(ContentType));
	    if (!ct) break;
	    strncpy(ct->Name,mlxStringVal(s,NULL),127);
	    ct->Name[127]=0;

	    /** Get the description of the type **/
	    t = mlxNextToken(s);
	    if (t != MLX_TOK_STRING)
		{
		nmFree(ct,sizeof(ContentType));
		break;
		}
	    strncpy(ct->Description,mlxStringVal(s,NULL),255);
	    ct->Description[255]=0;

	    /** Get the filename extension listing **/
	    xaInit(&(ct->Extensions),16);
	    while(1)
		{
	        t = mlxNextToken(s);
	        if (t != MLX_TOK_STRING && t != MLX_TOK_KEYWORD) 
		    {
		    xaDeInit(&(ct->Extensions));
		    nmFree(ct,sizeof(ContentType));
		    break;
		    }
	        a=1;
	        xaAddItem(&(ct->Extensions),mlxStringVal(s,&a));
	        t = mlxNextToken(s);
		if (t != MLX_TOK_COMMA) 
		    {
		    mlxHoldToken(s);
		    break;
		    }
		}

	    /** Get the optional type name expression. **/
	    t = mlxNextToken(s);
	    if (t != MLX_TOK_STRING)
	        {
		xaDeInit(&(ct->Extensions));
		nmFree(ct,sizeof(ContentType));
		break;
		}
	    ct->TypeNameObjList = (void*)expCreateParamList();
	    expAddParamToList((pParamObjects)(ct->TypeNameObjList), "this", NULL, 0);
	    expSetParamFunctions((pParamObjects)(ct->TypeNameObjList), "this", obj_internal_TypeFnName, 
	    	obj_internal_GetFnName, obj_internal_SetFnName);
	    ptr = mlxStringVal(s,NULL);
	    if (ptr && *ptr)
	        {
	        ct->TypeNameExpression = (void*)expCompileExpression(ptr, (pParamObjects)(ct->TypeNameObjList),
	    	    MLX_F_ICASE | MLX_F_FILENAMES, 0);
	        if (!ct->TypeNameExpression)
	            {
		    mssError(0,"OSML","Could not compile expression for type '%s'",ct->Name);
		    break;
		    }
		}
	    else
	        {
		ct->TypeNameExpression = NULL;
		}

	    /** Get the is-a type name, or '*' if top-level. **/
	    xaInit(&(ct->IsA),64);

	    /** MJM - doesn't appear that these are ever initialized anywhere.
	     ** is this the place to do it?
	     **/
	    xaInit(&(ct->RelatedTypes), 64);
	    xaInit(&(ct->RelationLevels), 64);
	    /** END MJM **/
	    
	    while(1)
		{
		t = mlxNextToken(s);
		if (t != MLX_TOK_STRING)
		    {
		    xaDeInit(&(ct->Extensions));
		    xaDeInit(&(ct->IsA));
		    /** MJM - related to the above addition **/
		    xaDeInit(&(ct->RelatedTypes));
		    xaDeInit(&(ct->RelationLevels));
		    /** END MJM **/
		    nmFree(ct,sizeof(ContentType));
		    break;
		    }
		ptr = mlxStringVal(s,NULL);
		if (!strcmp(ptr,"*"))
		    {
		    ct->Flags |= CT_F_TOPLEVEL;
		    t = mlxNextToken(s);
		    break;
		    }
		parent_ct = (pContentType)xhLookup(&(OSYS.Types),ptr);
		if (!parent_ct)
		    {
		    mssError(1,"OSML","Undefined parent type '%s' of type '%s'",ptr,ct->Name);
		    xaDeInit(&(ct->Extensions));
		    xaDeInit(&(ct->IsA));
		    /** MJM - also related to the above condition **/
		    xaDeInit(&(ct->RelatedTypes));
		    xaDeInit(&(ct->RelationLevels));
		    /** END MJM **/
		    nmFree(ct,sizeof(ContentType));
		    break;
		    }
		xaAddItem(&ct->IsA, (void*)parent_ct);
		t = mlxNextToken(s);
		if (t != MLX_TOK_COMMA) break;
		}
	    if (t != MLX_TOK_EOL && t != MLX_TOK_EOF)
		{
		break;
		}
	    xhAdd(&(OSYS.Types),ct->Name,(char*)ct);
	    xaAddItem(&(OSYS.TypeList), (void*)ct);
	    for(i=0;i<ct->Extensions.nItems;i++)
		{
		xhAdd(&(OSYS.TypeExtensions),(char*)(ct->Extensions.Items[i]),(char*)ct);
		}

	    /** Add to the /sys/cx.sysinfo directory **/
	    snprintf(sysbuf, sizeof(sysbuf), "/osml/types/%d", OSYS.TypeList.nItems);
	    si = sysAllocData(sysbuf, NULL, NULL, NULL, NULL, obj_types_GetAttrValue, NULL, 0);
	    sysAddAttrib(si, "type_name", DATA_T_STRING);
	    sysAddAttrib(si, "type_description", DATA_T_STRING);
	    sysAddAttrib(si, "parent_type", DATA_T_STRING);
	    sysRegister(si, (void*)ct);

	    if (t == MLX_TOK_EOF) break;
	    }

	/** Close lexer session and file descriptor **/
	mlxCloseSession(s);
	fdClose(fd, 0);

	/** Read the rootnode's path and type. **/
	ptr = NULL;
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "rootnode_type"), NULL, &ptr, 0) < 0 || !ptr)
	    {
	    mssError(1,"OSML","rootnode_type not specified in centrallix.conf!");
	    return -1;
	    }
	OSYS.RootType = NULL;
	ct = (pContentType)xhLookup(&OSYS.Types, ptr);
	if (!ct)
	    {
	    mssError(1,"OSML","Unknown type '%s' for rootnode",ptr);
	    return -1;
	    }
	OSYS.RootType = ct;
	filename=NULL;
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "rootnode_file"), NULL, &filename, 0) < 0 || !filename)
	    {
	    filename = OBJSYS_DEFAULT_ROOTNODE;
	    }
	memccpy(OSYS.RootPath, filename, 0, 255);
	OSYS.RootPath[255] = 0;

	/** Transaction log? **/
	filename = NULL;
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "transaction_log_file"), NULL, &filename, 0) < 0 || !filename)
	    filename = "";
	strtcpy(OSYS.TrxLogPath, filename, sizeof(OSYS.TrxLogPath));

	/** Build the Is-A database **/
	obj_internal_BuildIsA();

	/** Load the root driver **/
	rootInitialize();

	/** Load the OXT driver. **/
	oxtInitialize();

	/** Load the inheritance driver **/
	oihInitialize();

	/** Load the temp driver **/
	xhnInitContext(&OSYS.TempObjects);
	tmpInitialize();

	nmRegister(sizeof(Object),"Object");
	nmRegister(sizeof(ObjQuery),"ObjQuery");
	nmRegister(sizeof(ObjDriver),"ObjDriver");
	nmRegister(sizeof(Pathname),"Pathname");
	nmRegister(sizeof(DateTime),"DateTime");
	nmRegister(sizeof(ObjTrxTree),"ObjTrxTree");
	nmRegister(sizeof(ObjSession),"ObjSession");
	nmRegister(sizeof(DirectoryCache),"DirectoryCache");
	nmRegister(sizeof(ObjParam),"ObjParam");

    return 0;
    }


/*** objRegisterDriver -- register a new objectsystem driver.  This is 
 *** normally called from the Initialize routine of the driver in question.
 ***/
int
objRegisterDriver(pObjDriver drv)
    {
    int i;

	/** Add to drivers listing **/
	xaAddItem(&(OSYS.Drivers),(char*)drv);

	/** Make linkages to each base content type **/
	for(i=0;i<drv->RootContentTypes.nItems;i++)
	    {
	    xhAdd(&(OSYS.DriverTypes), (char*)(drv->RootContentTypes.Items[i]), (char*)drv);
	    }

	/** Transaction layer? **/
	if (drv->Capabilities & OBJDRV_C_ISTRANS) OSYS.TransLayer = drv;

	/** MultiQuery module? **/
	if (drv->Capabilities & OBJDRV_C_ISMULTIQUERY) OSYS.MultiQueryLayer = drv;

	/** Inheritance layer? **/
	if (drv->Capabilities & OBJDRV_C_ISINHERIT) OSYS.InheritanceLayer = drv;

    return 0;
    }


/*** objRegisterEventHandler - regsiter a new event handler routine.  This
 *** associates the non-changeable EventClassCode with the changeable event
 *** handler function pointer.  Event handlers must register each time the
 *** system re-starts.
 ***/
int
objRegisterEventHandler(char* class_code, int (*handler_function)())
    {
    pObjEventHandler eh;
    int i;
    pObjEvent e;

    	/** Allocate a new event handler structure **/
	eh = (pObjEventHandler)nmMalloc(sizeof(ObjEventHandler));
	if (!eh) return -1;

	/** Fill it in and add it to event handler hash table **/
	memccpy(eh->ClassCode, class_code, 0, 15);
	eh->ClassCode[15] = 0;
	eh->HandlerFunction = handler_function;
	xhAdd(&OSYS.EventHandlers, eh->ClassCode, (void*)eh);

	/** Step through the events and link with any w/ matching codes **/
	for(i=0;i<OSYS.Events.nItems;i++)
	    {
	    e = (pObjEvent)(OSYS.Events.Items[i]);
	    if (!strcmp(e->ClassCode, class_code)) e->Handler = eh;
	    }

    return 0;
    }


/*** obj_internal_WriteEventFile - rewrite the events file from the list of
 *** registered events.
 ***/
int
obj_internal_WriteEventFile()
    {
    pFile fd;
    int i;
    int uid;
    pObjEvent e;

	/** Re-write the events file. **/
	uid = geteuid();
	seteuid(0);
	fd = fdOpen("/omjnet/lightsys/events.cfg", O_WRONLY | O_TRUNC, 0600);
	seteuid(uid);
	if (!fd) return -1;
	for(i=0;i<OSYS.Events.nItems;i++)
	    {
	    e = (pObjEvent)(OSYS.Events.Items[i]);
	    fdWrite(fd, "\"", 1, 0,0);
	    fdWrite(fd, e->ClassCode, strlen(e->ClassCode), 0,0);
	    fdWrite(fd, "\" \"", 3, 0,0);
	    if (e->XData) fdWrite(fd, e->XData, strlen(e->XData), 0,0);
	    fdWrite(fd, "\" \"", 3, 0,0);
	    if (e->WhereClause) fdWrite(fd, e->WhereClause, strlen(e->WhereClause), 0,0);
	    fdWrite(fd, "\" \"", 3, 0,0);
	    fdWrite(fd, e->DirectoryPath, strlen(e->DirectoryPath), 0,0);
	    fdWrite(fd, "\"\n", 2, 0,0);
	    }
	fdClose(fd, 0);

    return 0;
    }


/*** obj_internal_ReadEventFile - read the current contents of the events
 *** file into the event registry via calling objRegisterEvent with the
 *** no-write-file option.
 ***/
int
obj_internal_ReadEventFile()
    {
    pFile fd;
    int uid;
    pLxSession lxs;
    int t;
    XString code, path, where, xdata;
    char* ptr;

    	/** Open the file **/
	uid = geteuid();
	seteuid(0);
	fd = fdOpen("/omjnet/lightsys/events.cfg", O_RDONLY, 0600);
	seteuid(uid);
	if (!fd) return -1;

	/** Open a lexer/tokenizer session on the file **/
	lxs = mlxOpenSession(fd, MLX_F_EOF | MLX_F_EOL);
	if (!lxs) return -1;

	/** Initialize the string buffers **/
	xsInit(&code);
	xsInit(&path);
	xsInit(&where);
	xsInit(&xdata);

	/** Read the lines, building an event for each one. **/
	while(1)
	    {
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_EOF)
	        {
		mlxCloseSession(lxs);
		fdClose(fd, 0);
		fd = NULL;
		break;
		}

	    /** Read code, xdata, where, and path **/
	    if (t != MLX_TOK_STRING) break;
	    ptr = mlxStringVal(lxs,NULL);
	    xsCopy(&code, ptr, -1);
	    if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
	    xsCopy(&xdata, mlxStringVal(lxs,NULL), -1);
	    if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
	    xsCopy(&where, mlxStringVal(lxs,NULL), -1);
	    if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
	    xsCopy(&path, mlxStringVal(lxs,NULL), -1);

	    /** Register the event **/
	    objRegisterEvent(code.String, path.String, (where.String[0])?where.String:NULL, OBJ_EV_F_NOSAVE, xdata.String);
	    }

	/** An error occurred? (fd is still open) **/
	if (fd)
	    {
	    puts("Centrallix: could not read events.cfg file!");
	    mlxCloseSession(lxs);
	    fdClose(fd, 0);
	    fd = NULL;
	    return -1;
	    }

    return 0;
    }


/*** objRegisterEvent - register a new event.  This only has to be done 
 *** once, and is retained across system re-starts.  After a restart, the
 *** event will not be active until its handler is registered via the above
 *** objRegisterEventHandler function.
 ***/
int
objRegisterEvent(char* class_code, char* pathname, char* where_condition, int flags, char* xdata)
    {
    pObjEvent e;
    pObjEventHandler eh;

    	/** Allocate a new event structure **/
	e = (pObjEvent)nmMalloc(sizeof(ObjEvent));
	if (!e) return -1;

	/** Fill it in **/
	memccpy(e->ClassCode, class_code, 0, 15);
	e->ClassCode[15] = 0;
	if (where_condition)
	    {
	    e->WhereClause = nmSysStrdup(where_condition);
	    }
	else
	    {
	    e->WhereClause = NULL;
	    }
	e->XData = nmSysStrdup(xdata);
	e->Flags = flags;
	memccpy(e->DirectoryPath, pathname, 0, 255);
	e->DirectoryPath[255] = 0;

	/** Strip any trailing '/' off of the path, unless path is solely '/' **/
	if (e->DirectoryPath[1] && e->DirectoryPath[strlen(e->DirectoryPath)-1] == '/')
	    {
	    e->DirectoryPath[strlen(e->DirectoryPath)-1] = '\0';
	    }

	/** Lookup the handler, if it has one. **/
	eh = (pObjEventHandler)xhLookup(&OSYS.EventHandlers, class_code);
	e->Handler = eh;

	/** Add the event to the registry in various places. **/
	xhAdd(&OSYS.EventsByXData, e->XData, (void*)e);
	xhAdd(&OSYS.EventsByPath, e->DirectoryPath, (void*)e);
	xaAddItem(&OSYS.Events, (void*)e);

	/** Update the event file **/
	if (!(flags & OBJ_EV_F_NOSAVE))
	    {
	    obj_internal_WriteEventFile();
	    }

    return 0;
    }


/*** objUnRegisterEvent - remove an existing event from the event registry
 ***/
int
objUnRegisterEvent(char* class_code, char* xdata)
    {
    pObjEvent e;

    	/** Locate the event. **/
	e = (pObjEvent)xhLookup(&OSYS.EventsByXData, xdata);
	if (!e) return -1;

	/** Remove from global catalog listings of events **/
	xhRemove(&OSYS.EventsByXData, e->XData);
	xhRemove(&OSYS.EventsByPath, e->DirectoryPath);
	xaRemoveItem(&OSYS.Events, xaFindItem(&OSYS.Events, (void*)e));

	/** Update the event file **/
	obj_internal_WriteEventFile();

    return 0;
    }


/*** obj_internal_TrxLog() - log a transaction to the transaction log, using
 *** qprintf to format the log lines.  Each log entry always begins with the
 *** timestamp, the username, the operation, and the path to the object in
 *** question.  After that, the caller has discretion over the log entry
 *** format.
 ***/
int
obj_internal_TrxLog(pObject this, char* op, char* fmt, ...)
    {
    char* buf;
    char* ptr;
    struct tm* thetime;
    time_t tval;
    char tbuf[40];
    va_list va;
    int rval = 0;
    pFile fd;

	if (!(OSYS.TrxLogPath[0]))
	    return 0;

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%Y-%b-%d %T", thetime);

	/** Allocate a buffer **/
	buf = (char*)nmMalloc(OBJSYS_MAX_PATH + 256);

	/** Log header.  -1 leaves room for \n **/
	if (qpfPrintf(NULL, buf, OBJSYS_MAX_PATH + 256 - 1, "%STR&DQUOT,%STR&DQUOT,%STR&DQUOT,%STR&DQUOT,", tbuf, mssUserName(), op, objGetPathname(this)) < 0)
	    {
	    nmFree(buf, OBJSYS_MAX_PATH + 256);
	    return -1;
	    }
	ptr = buf + strlen(buf);

	/** Custom part **/
	va_start(va, fmt);
	rval = qpfPrintf_va(NULL, ptr, (OBJSYS_MAX_PATH + 256 - 1) - (ptr - buf), fmt, va);
	va_end(va);
	strcat(ptr, "\n");

	/** Write it **/
	fd = fdOpen(OSYS.TrxLogPath, O_WRONLY | O_APPEND, 0600);
	if (fd)
	    {
	    fdPrintf(fd, "%s", buf);
	    fdClose(fd, 0);
	    }

	nmFree(buf, OBJSYS_MAX_PATH + 256);

    return (rval<0)?(-1):0;
    }

