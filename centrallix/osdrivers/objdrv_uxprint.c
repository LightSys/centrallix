#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <signal.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "st_node.h"
#include "stparse.h"
#include "mtlexer.h"
#include "prtmgmt.h"
#include "mtsession.h"

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
/* Module: 	objdrv_uxprint.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 1, 1999					*/
/* Description:	UNIX printer objectsystem driver.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_uxprint.c,v 1.2 2001/09/27 19:26:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_uxprint.c,v $

    $Log: objdrv_uxprint.c,v $
    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:12  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:11  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** A single lpr print queue entry ***/
typedef struct
    {
    int		JobID;
    char	Name[128];
    char	Pathname[256];
    char	User[32];
    int		Size;
    int		Percent;
    char	Status[32];
    }
    LprQueueEntry, *pLprQueueEntry;

/*** Structure for managing the print queue listing. ***/
typedef struct
    {
    char	Pathname[256];		/* pathname to printer in objsys */
    XArray	Entries;		/* print queue entries */
    }
    LprPrintQueue, *pLprPrintQueue;

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    pFile	fd;
    char	Pathname[256];
    int		Flags;
    int		Type;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pFile	SpoolFileFD;
    char	SpoolPathname[256];
    pSnNode	Node;
    pFile	MasterFD;
    pFile	SlaveFD;
    char	ReqType[256];
    }
    UxpData, *pUxpData;

#define UXP_T_PRINTER		1
#define UXP_T_PRINTJOB		2

#define UXP(x) ((pUxpData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pUxpData	ObjInf;
    char	NameBuf[256];
    }
    UxpQuery, *pUxpQuery;

/*** GLOBALS ***/
struct
    {
    XHashTable	OpenFiles;
    XHashTable	PrintQueues;
    int		SpoolCnt;
    }
    UXP_INF;


/*** uxp_internal_LoadPrintQueue - load the information from the print queue
 *** into the print queue structures for further examination and reporting/
 *** querying to the user.  Takes path to the printer node as the parameter,
 *** and returns a pointer to the print queue structure.
 ***/
pLprPrintQueue
uxp_internal_LoadPrintQueue(char* nodepath, pSnNode nodeinfo)
    {
    pLprPrintQueue pq;
    int pipefd[2];
    pFile fd;
    char* lpname;
    char* spoolname;
    pLxSession lxs;
    int t;
    char* str;
    int childpid;
    char user[32];
    char filename[128];
    int rank,jobid;
    int cnt;
    pLprQueueEntry e;

    	/** Find out if this print queue is already hashed. **/
	pq = (pLprPrintQueue)xhLookup(&UXP_INF.PrintQueues, nodepath);

	/** If not, create new entry. **/
	if (!pq)
	    {
	    pq = (pLprPrintQueue)nmMalloc(sizeof(LprPrintQueue));
	    xaInit(&pq->Entries, 16);
	    strcpy(pq->Pathname, nodepath);
	    xhAdd(&UXP_INF.PrintQueues, pq->Pathname, (void*)pq);
	    }

	/** Determine printer's name, and centrallix spool dir. **/
	stAttrValue(stLookup(nodeinfo->Data,"unix_name"), NULL, &lpname, 0);
	stAttrValue(stLookup(nodeinfo->Data,"spool_directory"), NULL, &spoolname, 0);

	/** Ok, now open a pipe from the 'lpq' command. **/
	socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
	if ((childpid=fork()))
	    {
	    /** Close the child's end of the pipe, since we don't use it in the parent **/
	    close(pipefd[1]);

	    /** Open an fd to the pipe **/
	    fd = fdOpenFD(pipefd[0],O_RDONLY);

	    /** Open an mtlexer session **/
	    lxs = mlxOpenSession(fd, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY);

	    /** Read until we get "Rank" at the beginning of a line. **/
	    while(1)
	        {
		t = mlxNextToken(lxs);
		if (t != MLX_TOK_STRING) 
		    {
		    kill(childpid,SIGKILL);
		    wait(NULL);
		    return pq;
		    }
		str = mlxStringVal(lxs,0);
		while ((t=mlxNextToken(lxs) != MLX_TOK_EOL && t != MLX_TOK_ERROR));
		if (!strcmp(str,"Rank")) break;
		}

	    /** Clean out any existing entries from our internal queue list. **/
	    while(pq->Entries.nItems) 
	        {
		nmFree(pq->Entries.Items[0],sizeof(LprQueueEntry));
		xaRemoveItem(&pq->Entries,0);
		}

	    /** Ok, should be one line per print job now **/
	    for(cnt=0;;)
	        {
		/** Get rank, owner, jobid, filename **/
		if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
		rank = strtol(mlxStringVal(lxs,0),NULL,10);
		if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
		mlxCopyToken(lxs,user,32);
		if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
		jobid = strtol(mlxStringVal(lxs,0),NULL,10);
		if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
		mlxCopyToken(lxs,filename,128);

		/** If it isn't our file, skip  to eol and continue looking **/
		if (strncmp(filename, spoolname, strlen(spoolname)))
		    {
		    while ((t=mlxNextToken(lxs) != MLX_TOK_EOL && t != MLX_TOK_ERROR));
		    continue;
		    }

		/** Create the queue entry structure **/
		e = (pLprQueueEntry)nmMalloc(sizeof(LprQueueEntry));
		e->JobID = jobid;
		strcpy(e->User,user);
		e->Percent = 0;
		if (cnt==0) strcpy(e->Status,"Printing");
		else strcpy(e->Status,"Queued");
		strcpy(e->Pathname, filename);
		strcpy(e->Name, strrchr(filename,'/')+1);

		/** Get the file's size and skip to eol. **/
		if (mlxNextToken(lxs) != MLX_TOK_STRING) break;
		e->Size = strtol(mlxStringVal(lxs,0),NULL,10);
		while ((t=mlxNextToken(lxs) != MLX_TOK_EOL && t != MLX_TOK_ERROR));

		/** Add entry to our queue list **/
		xaAddItem(&pq->Entries, (void*)e);
		cnt++;
		}

	    /** Wait on lpq's completion. **/
	    wait(NULL);
	    }
	else
	    {
	    /** Close the parent's end of the pipe, since we don't use it in the child **/
	    close(pipefd[0]);

	    /** Make the pipe lpq's standard input and output **/
	    close(0);
	    close(1);
	    close(2);
	    dup(pipefd[1]);
	    dup(pipefd[1]);
	    dup(pipefd[1]);

	    /** Execute the lpq process, exit if failed. **/
	    execlp("lpq","lpq","-P",lpname,NULL);
	    _exit(0);
	    }

    return pq;
    }


/*** uxp_internal_FindQueueItem - locates a queued print job based on the print
 *** job's name.  Returns the pointer to the queue item.
 ***/
pLprQueueEntry
uxp_internal_FindQueueItem(pLprPrintQueue pq, char* name)
    {
    pLprQueueEntry e = NULL;
    int i;

    	/** Scan through the array **/
	for(i=0;i<pq->Entries.nItems;i++)
	    {
	    e = (pLprQueueEntry)(pq->Entries.Items[i]);
	    if (!strcmp(e->Name,name)) return e;
	    }

    return NULL;
    }


/*** uxp_internal_Filter - this is the worker thread that does the 
 *** html-to-pcl-or-fx conversion process, using prtConvertHTML.
 ***/
void
uxp_internal_Filter(void* v)
    {
    pUxpData inf = (pUxpData)v;
    pPrtSession s;
    char* type;
    char* printcmd;
    char* uxname;
    char* form=NULL;
    char cmd[128];

    	/** Set thread name **/
	thSetName(NULL, "Print Writer");

    	/** Open the new print session. **/
	stAttrValue(stLookup(inf->Node->Data,"type"), NULL, &type, 0);
	s = prtOpenSession(type, fdWrite, inf->SpoolFileFD, 0);
	if (!s)
	    {
	    fdClose(inf->SlaveFD, 0);
	    inf->SlaveFD = NULL;
	    thExit();
	    }

	/** Run the converter, and close when done. **/
	prtConvertHTML(fdRead, inf->SlaveFD, s);
	prtCloseSession(s);
	fdClose(inf->SlaveFD, 0);
	inf->SlaveFD = NULL;

	/** Print the thing via the OS's spooler **/
	fdClose(inf->SpoolFileFD, 0);
	stAttrValue(stLookup(inf->Node->Data,"if_type"),NULL,&printcmd,0);
	stAttrValue(stLookup(inf->Node->Data,"unix_name"),NULL,&uxname,0);
	stAttrValue(stLookup(inf->Node->Data,"form_name"),NULL,&form,0);
	if (printcmd && uxname)
	    {
	    if (!strcmp(printcmd,"lp"))
	        sprintf(cmd,"(%s -d%s %s%s %s; /bin/rm %s) &",printcmd, uxname, form?"-f":"",form?form:"", 
			inf->SpoolPathname, inf->SpoolPathname);
	    else
		sprintf(cmd,"(%s -P%s %s%s %s; /bin/rm %s) &",printcmd, uxname, form?"-f":"", form?form:"", 
			inf->SpoolPathname, inf->SpoolPathname);
	    system(cmd);
	    }

    thExit();
    }


/*** uxp_internal_StartFilter - start the html-to-pcl-or-fx conversion filter
 *** thread.
 ***/
int
uxp_internal_StartFilter(pUxpData inf)
    {
    int lowlevel_fd[2];

    	/** Create the socket pair and connect 'em with mtask **/
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, lowlevel_fd) < 0)
	    {
	    mssErrorErrno(1,"UXP","Could open socketpair pipe for print writer");
	    return -1;
	    }
	inf->MasterFD = fdOpenFD(lowlevel_fd[0], O_WRONLY);
	inf->SlaveFD = fdOpenFD(lowlevel_fd[1], O_RDONLY);

	/** Start the filter thread **/
	thCreate(uxp_internal_Filter, 0, (void*)inf);

    return 0;
    }


/*** uxpOpen - open a file or directory.
 ***/
void*
uxpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pUxpData inf;
    char* node_path;
    pSnNode node = NULL;
    char sbuf[256];
    int is_new = 0;
    pLprPrintQueue pq;

        /** Determine node path and attempt to open node. **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

        /** If CREAT and EXCL, we only create, failing if already exists. **/
        if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
            {
            node = snNewNode(obj->Prev, usrtype);                                                                  
	    if (!node)
                {
		mssError(0,"UXP","Could not create new printer node object");
                return NULL;
                }
	    is_new = 1;
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
	    is_new = 1;
            }

	/** Couldn't create or open existing node?  Dumb user. **/
	if (!node) 
	    {
	    mssError(0,"UXP","Could not open printer node object");
	    return NULL;
	    }

	/** New node?  If so, set some values **/
	if (is_new)
	    {
	    snSetParamString(node, obj->Prev, "unix_name", strrchr(obj->Pathname->Pathbuf,'/')+1);
	    snSetParamString(node, obj->Prev, "if_type", "lpr");
	    snSetParamString(node, obj->Prev, "type", "text/x-hp-pcl");
	    snSetParamString(node, obj->Prev, "valid_sources", "0,1,2,4");
	    snSetParamString(node, obj->Prev, "current_source", "0");
	    snSetParamString(node, obj->Prev, "auto_switch", "yes");
	    sprintf(sbuf,"/var/spool/centrallix/%s",strrchr(obj->Pathname->Pathbuf,'/')+1);
	    snSetParamString(node, obj->Prev, "spool_directory", sbuf);
	    node->Status = SN_NS_DIRTY;
	    snWriteNode(obj->Prev,node);
	    }

	/** Allocate the structure **/
	inf = (pUxpData)nmMalloc(sizeof(UxpData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(UxpData));
	inf->Obj = obj;
	inf->Mask = mask;
	memccpy(inf->ReqType, usrtype, 0, 255);
	inf->ReqType[255] = 0;

	/** Check to see if opening printer or print job. **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    inf->Type = UXP_T_PRINTER;
	else
	    inf->Type = UXP_T_PRINTJOB;

	/** Make sure print queue is accessible **/
	/*pq = uxp_internal_LoadPrintQueue(node_path, node);
	if (!pq)
	    {
	    nmFree(inf, sizeof(UxpData));
	    return NULL;
	    }*/

	/** If print job, make sure it exists first in the printer's queue. **/
	if (inf->Type == UXP_T_PRINTJOB)
	    {
	    if (uxp_internal_FindQueueItem(pq, obj_internal_PathPart(obj->Pathname, obj->SubPtr, 0)) == NULL)
	        {
		nmFree(inf, sizeof(UxpData));
		return NULL;
		}
	    }

	/** If opened the printer object directly, create a new print job **/
	inf->SpoolFileFD = NULL;
	inf->SpoolPathname[0] = '\0';
	inf->Node = node;

	/** Reset the pathpart stuff **/
	obj_internal_PathPart(obj->Pathname, 0,0);
	obj->SubCnt = 1;

    return (void*)inf;
    }


/*** uxpClose - close an open file or directory.
 ***/
int
uxpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    char cmd[320];
    char* printcmd;
    char* uxname;
    char* form=NULL;

    	/** Node need to be updated? **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	/** Close the master end of the filter socketpair? **/
	if (inf->MasterFD) fdClose(inf->MasterFD, 0);

	/** Need to spool print job? **/
	if (inf->SpoolFileFD && !inf->MasterFD)
	    {
	    fdClose(inf->SpoolFileFD, 0);
	    stAttrValue(stLookup(inf->Node->Data,"if_type"),NULL,&printcmd,0);
	    stAttrValue(stLookup(inf->Node->Data,"unix_name"),NULL,&uxname,0);
	    stAttrValue(stLookup(inf->Node->Data,"form_name"),NULL,&form,0);
	    if (printcmd && uxname)
	        {
		if (!strcmp(printcmd,"lp"))
	            sprintf(cmd,"(%s -d%s %s%s %s; /bin/rm %s) &",printcmd, uxname, form?"-f":"",form?form:"", 
			inf->SpoolPathname, inf->SpoolPathname);
	        else
		    sprintf(cmd,"(%s -P%s %s%s %s; /bin/rm %s) &",printcmd, uxname, form?"-f":"", form?form:"", 
			inf->SpoolPathname, inf->SpoolPathname);
	        system(cmd);
		}
	    }

	/** Free the structure **/
	nmFree(inf,sizeof(UxpData));

    return 0;
    }


/*** uxpCreate - create a new file without actually opening that 
 *** file.
 ***/
int
uxpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* create_inf;

    	/** Open the thing and then close it. **/
	obj->Mode |= (O_CREAT | O_EXCL);
	create_inf = uxpOpen(obj, mask, systype, usrtype, oxt);
	if (create_inf == NULL) return -1;

	/** Close the thing **/
	uxpClose(create_inf, oxt);

    return 0;
    }


/*** uxpDelete - delete an existing file or directory.  Hmm...
 ***/
int
uxpDelete(pObject obj, pObjTrxTree* oxt)
    {

    return -1;
    }


/*** uxpRead - read from a print job.  What in the world does reading from
 *** a print job mean?  This fails.  Sigh.
 ***/
int
uxpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pUxpData inf = UXP(inf_v);*/

    return -1;
    }


/*** uxpWrite - write to a new print job.  Each time this driver is opened
 *** and written to, it spools a new print job.
 ***/
int
uxpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    int rval;
    char* spooldir = NULL;
    char* type = NULL;
    struct stat fileinfo;
    int start_filter = 0;

	/** Start the filter process? **/
	if (!inf->SpoolFileFD && !inf->MasterFD)
	    {
	    start_filter = 1;
	    }

    	/** Ok, need to create a new print job? **/
	if (inf->SpoolFileFD == NULL)
	    {
	    /** Get spool dir. **/
	    stAttrValue(stLookup(inf->Node->Data,"spool_directory"), NULL, &spooldir, 0);
	    if (!spooldir) 
	        {
		mssError(1,"UXP","Spool directory not specified in node object");
		return -1;
		}

	    /** Generate a spool file name **/
	    do  {
	        sprintf(inf->SpoolPathname,"%s/%8.8d.job",spooldir,UXP_INF.SpoolCnt++);
		} while (lstat(inf->SpoolPathname, &fileinfo) == 0);

	    /** Open the spool file **/
	    inf->SpoolFileFD = fdOpen(inf->SpoolPathname, O_WRONLY | O_CREAT | O_EXCL, 0600);
	    if (!inf->SpoolFileFD) 
	        {
		mssErrorErrno(1,"UXP","Could not open spool file");
		return -1;
		}
	    }

	/** Ok, delayed start of filter process to wait until spoofile-fd is open **/
	if (start_filter)
	    {
	    stAttrValue(stLookup(inf->Node->Data,"type"), NULL, &type, 0);
	    if (type && strcmp(type, inf->ReqType) && !strcmp(inf->ReqType,"text/html"))
	        {
		uxp_internal_StartFilter(inf);
		}
	    }

	/** Need to filter the data? **/
	if (inf->MasterFD)
	    {
	    rval = fdWrite(inf->MasterFD, buffer, cnt, offset, flags);
	    }
	else
	    {
	    /** Write to the spool file. **/
	    rval = fdWrite(inf->SpoolFileFD, buffer, cnt, offset, flags);
	    }

    return rval;
    }


/*** uxpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries, and adding intelligence here wouldn't
 *** help performance since the filesystem doesn't support such queries.
 *** So, we leave the query matching logic to the ObjectSystem management
 *** layer in this case.
 ***/
void*
uxpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    /*pUxpData inf = UXP(inf_v);
    pUxpQuery qy;*/

    return NULL;
    }


/*** uxpQueryFetch - get the next directory entry as an open object.
 ***/
void*
uxpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    /*pUxpQuery qy = ((pUxpQuery)(qy_v));*/

    return NULL;
    }


/*** uxpQueryClose - close the query.
 ***/
int
uxpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** uxpGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
uxpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    pStructInf search_inf;

    	/** If name, it's a string. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** If content-type, it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname, "inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** If annotation, it's a string. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Hunt around in the structinf for the thing **/
	search_inf = stLookup(inf->Node->Data, attrname);
	if (search_inf)
	    {
	    if (search_inf->StrVal[0])
	        return DATA_T_STRING;
	    else
	        return DATA_T_INTEGER;
	    }

	mssError(1,"UXP","Invalid attribute for printer object");

    return -1;
    }


/*** uxpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
uxpGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    char* ptr;
    pStructInf find_inf;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    ptr = strrchr(inf->Obj->Pathname->Pathbuf+1,'/')+1;
	    if (ptr)
	        {
		strcpy(inf->Pathname,ptr);
	        *((char**)val) = inf->Pathname;
		}
	    else
	        {
	        *((char**)val) = "/";
		}
	    /* *((char**)val) = inf->Node->Data->Name;*/
	    }
	else if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    stAttrValue(stLookup(inf->Node->Data,"type"), NULL, &ptr, 0);
	    *((char**)val) = ptr;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    *((char**)val) = "system/printer";
	    }
	else if (!strcmp(attrname,"spool_file"))
	    {
	    *((char**)val) = inf->SpoolPathname;
	    }
	else
	    {
	    /** Lookup in the structinf tree. **/
	    find_inf = stLookup(inf->Node->Data, attrname);
	    if (!find_inf && !strcmp(attrname,"annotation"))
	        {
		*((char**)val) = "Printer";
		return 0;
		}
	    else if (!find_inf)
	        {
		mssError(1,"UXP","Invalid attribute for printer object");
		return -1;
		}

	    /** Found it - return the value **/
	    if (find_inf->StrVal[0])
		*((char**)val) = find_inf->StrVal[0];
	    else
	        *((int*)val) = find_inf->IntVal[0];
	    }

    return 0;
    }


/*** uxpGetNextAttr - get the next attribute name for this object.
 ***/
char*
uxpGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);

    	/** Look for an attr entry **/
	while(inf->CurAttr < inf->Node->Data->nSubInf && 
	      (inf->Node->Data->SubInf[inf->CurAttr]->Type != ST_T_ATTRIB ||
	       !strcmp(inf->Node->Data->SubInf[inf->CurAttr]->Name,"annotation")))
	    {
	    inf->CurAttr++;
	    }

	/** No mo attr? **/
	if (inf->CurAttr >= inf->Node->Data->nSubInf) return NULL;

    return inf->Node->Data->SubInf[inf->CurAttr++]->Name;
    }


/*** uxpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
uxpGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    
    	inf->CurAttr = 0;

    return uxpGetNextAttr(inf_v, oxt);
    }


/*** uxpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
uxpSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pUxpData inf = UXP(inf_v);
    pStructInf attr_inf;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	    if (strlen(inf->Obj->Pathname->Pathbuf) - 
	        strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		strlen(*(char**)(val)) + 1 > 255)
		{
		mssError(1,"UXP","SetAttr 'name': new value exceeds internal size limits");
		return -1;
		}
	    strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	    strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	    if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
	        {
		mssErrorErrno(1,"UXP","Could not rename printer node object");
		return -1;
		}
	    strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
	    }
	else
	    {
	    attr_inf = stLookup(inf->Node->Data, attrname);
	    if (!attr_inf && !strcmp(attrname,"annotation"))
	        {
		attr_inf = stAddAttr(inf->Node->Data, attrname);
		attr_inf->StrAlloc[0] = 1;
		attr_inf->StrVal[0] = nmSysStrdup(*((char**)val));
		}
	    else if (!attr_inf)
	        {
		mssError(1,"UXP","Invalid attribute for printer object");
		return -1;
		}
	    else
	        {
		if (attr_inf->StrVal[0])
		    {
		    if (attr_inf->StrAlloc[0]) nmSysFree(attr_inf->StrVal[0]);
		    attr_inf->StrAlloc[0] = 1;
		    attr_inf->StrVal[0] = nmSysStrdup(*((char**)val));
		    }
		else
		    {
		    attr_inf->IntVal[0] = *(int*)val;
		    }
		}
	    }

    return 0;
    }


/*** uxpAddAttr - add an attribute to an object.  This doesn't work for
 *** unix printer objects, so we just deny the request.
 ***/
int
uxpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** uxpOpenAttr - open an attribute as if it were an object with 
 *** content.  The UNIX printer objects have no attributes that are
 *** suitable for this.
 ***/
void*
uxpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** uxpGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
uxpGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** uxpGetNextMethod -- same as above.  Always fails. 
 ***/
char*
uxpGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** uxpExecuteMethod - No methods to execute, so this fails.
 ***/
int
uxpExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** uxpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
uxpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&UXP_INF,0,sizeof(UXP_INF));
	xhInit(&UXP_INF.PrintQueues,255,0);

	/** Setup the structure **/
	strcpy(drv->Name,"UXP - UNIX Printer Interface Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/printer");

	/** Setup the function references. **/
	drv->Open = uxpOpen;
	drv->Close = uxpClose;
	drv->Create = uxpCreate;
	drv->Delete = uxpDelete;
	drv->OpenQuery = uxpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = uxpQueryFetch;
	drv->QueryClose = uxpQueryClose;
	drv->Read = uxpRead;
	drv->Write = uxpWrite;
	drv->GetAttrType = uxpGetAttrType;
	drv->GetAttrValue = uxpGetAttrValue;
	drv->GetFirstAttr = uxpGetFirstAttr;
	drv->GetNextAttr = uxpGetNextAttr;
	drv->SetAttrValue = uxpSetAttrValue;
	drv->AddAttr = uxpAddAttr;
	drv->OpenAttr = uxpOpenAttr;
	drv->GetFirstMethod = uxpGetFirstMethod;
	drv->GetNextMethod = uxpGetNextMethod;
	drv->ExecuteMethod = uxpExecuteMethod;

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

