#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctpublic.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "expression.h"
#include "xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "xhashqueue.h"
#include "multiquery.h"

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
/* Module: 	objdrv_sybase.c     					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 4, 1998					*/
/* Description:	Objectsystem driver for the Sybase database.		*/
/*									*/
/*		Update April 3, 2000 - added result set caching logic	*/
/*		to speed up repetitive queries, such as those on code	*/
/*		tables.	 CURRENTLY INCOMPLETE.				*/
/*									*/
/*		Update April 7, 2000 - added multiquery driver in this	*/
/*		module to handle passthrough querying.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_sybase.c,v 1.2 2001/09/27 19:26:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_sybase.c,v $

    $Log: objdrv_sybase.c,v $
    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:11  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:10  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Module Controls ***/
#define SYBD_USE_CURSORS	1	/* use cursors for all multirow SELECTs */
#define SYBD_CURSOR_ROWCOUNT	20	/* # of rows to fetch at a time */
#define SYBD_SHOW_SQL		1	/* debug printout SQL issued to Sybase */
#define SYBD_RESULTSET_CACHE	64	/* number of rows to hold in cache */
#define SYBD_RESULTSET_PERTBL	48	/* max rows to cache per table */


/*** Structure for storing table-key information ***/
typedef struct
    {
    char	Table[32];
    char*	ColBuf;
    int		ColBufSize;
    int		ColBufLen;
    char*	Cols[256];
    unsigned char ColIDs[256];
    unsigned char ColFlags[256];
    unsigned char ColTypes[256];
    unsigned char ColKeys[256];
    int		nCols;
    char*	Keys[8];
    int		KeyCols[8];
    int		nKeys;
    pParamObjects ObjList;
    pExpression	RowAnnotExpr;
    char	Annotation[256];
    int		HasContent;
    }
    SybdTableInf, *pSybdTableInf;

#define SYBD_CF_ALLOWNULL	1
#define SYBD_CF_FOUND		2
#define SYBD_CF_PRIKEY		4


/*** Structure for directory entry nodes ***/
typedef struct
    {
    char	Path[256];
    char	Server[32];
    char	Database[32];
    char	AnnotTable[32];
    char	Description[256];
    int		MaxConn;
    pSnNode	SnNode;
    XArray	Conns;
    XHashTable	TableInf;
    int		LastAccess;
    char	Types[81][14];
    }
    SybdNode, *pSybdNode;

/*** Structure used by this driver to manage connections to the db ***/
typedef struct
    {
    /*int		SessionID;*/
    CS_CONNECTION* SessionID;
    char	Username[32];
    char	Password[32];
    int		Busy;
    }
    SybdConn, *pSybdConn;

/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    CS_CONNECTION* SessionID;
    CS_CONNECTION* ReadSessID;
    CS_CONNECTION* WriteSessID;
    CS_COMMAND*	RWCmd;
    CS_IODESC	ContentIODesc;
    int		LinkCnt;
    pSybdNode	Node;
    pSybdTableInf TData;
    char	TmpFile[64];
    pFile	TmpFD;
    Pathname	Pathname;
    char*	NodePtr;
    char*	TablePtr;
    char*	TableSubPtr;
    char*	RowColPtr;
    int		Flags;
    int		Type;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    int		Size;
    int		ColID;
    union
        {
	DateTime	Date;
	MoneyType	Money;
	IntVec		IV;
	StringVec	SV;
	}
	Types;
    char*	ColPtrs[256];
    unsigned char ColNum[256];
    char	RowBuf[2048];
    }
    SybdData, *pSybdData;

#define SYBD_F_ROWPRESENT	1
#define SYBD_F_TFRRW		2

#define SYBD_T_DATABASE		1
#define SYBD_T_TABLE		2
#define SYBD_T_ROWSOBJ		3
#define SYBD_T_COLSOBJ		4
#define SYBD_T_COLUMN		5
#define SYBD_T_ROW		6
#define SYBD_T_ATTR		7

#define SYBD(x) ((pSybdData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pSybdData	ObjInf;
    CS_CONNECTION* SessionID;
    CS_CONNECTION* ObjSession;
    int		RowCnt;
    char	SQLbuf[2048];
    pSybdTableInf TableInf;
    CS_COMMAND*	Cmd;
    int		RowsSinceFetch;
    }
    SybdQuery, *pSybdQuery;


/*** Structure for one table in a passthru query ***/
typedef struct
    {
    pSybdTableInf	TData;
    pQueryElement	QELinkage;
    }
    MqsybSource, *pMqsybSource;

/*** Structure for private data in a passthru query ***/
typedef struct
    {
    pMqsybSource	Sources[16];
    int			nSources;
    XString		BaseQuery;
    XString		CurQuery;
    SybdData		Inf;
    }
    MqsybData, *pMqsybData;


/*** GLOBALS ***/
struct
    {
    CS_CONTEXT*		Context;
    XHashTable		DBNodes;
    XArray		DBNodeList;
    int			AccessCnt;
    XString		LastSQL;
    pQueryDriver	QueryDriver;
    pObjDriver		ObjDriver;
    }
    SYBD_INF;


/*** Attribute name list ***/
typedef struct
    {
    char* 	Name;
    int		Type;
    } 
    attr_t;

#if 0 /** GRB commented this out until we use it **/
static attr_t dbattr_inf[] =
    {
    {"owner",DATA_T_STRING},
    {"group",DATA_T_STRING},
    {"last_modification",DATA_T_DATETIME},
    {"last_access",DATA_T_DATETIME},
    {"last_change",DATA_T_DATETIME},
    {"permissions",DATA_T_INTEGER},
    {"host",DATA_T_STRING},
    {"service",DATA_T_STRING},
    {"server",DATA_T_STRING},
    {"database",DATA_T_STRING},
    {"max_connections",DATA_T_INTEGER},
    {NULL,-1}
    };

/*** Attributes for tables ***/
static attr_t tabattr_inf[] =
    {
    {"rowcount",DATA_T_INTEGER},
    {"columncount",DATA_T_INTEGER},
    {NULL,-1}
    };
#endif


/*** sybd_internal_Exec - executes a query and returns the CTlib command
 *** structure.
 ***/
CS_COMMAND*
sybd_internal_Exec(CS_CONNECTION* s, char* cmdtext)
    {
    CS_COMMAND* cmd;

    	/** Alloc the cmd **/
	ct_cmd_alloc(s, &cmd);
	
	/** Setup the cmd text **/
	ct_command(cmd, CS_LANG_CMD, cmdtext, CS_NULLTERM, CS_UNUSED);

	/** Send it to the server. **/
	if (ct_send(cmd) != CS_SUCCEED)
	    {
	    ct_cmd_drop(cmd);
	    mssError(1,"SYBD","Could not send SQL command to server");
	    return NULL;
	    }

    return cmd;
    }


/*** sybd_internal_Close - closes an open query exec that was previously
 *** opened with sybd_internal_Exec.
 ***/
int
sybd_internal_Close(CS_COMMAND* cmd)
    {

    	/** Make sure cmd is cleared out. **/
    	ct_cancel(NULL, cmd, CS_CANCEL_ALL);

	/** Release the memory **/
	ct_cmd_drop(cmd);

    return 0;
    }


/*** sybd_internal_GetConn - obtains a database connection within a 
 *** given database node.
 ***/
CS_CONNECTION*
sybd_internal_GetConn(pSybdNode db_node, char* user, char* pwd)
    {
    int i,found_one,rval;
    pSybdConn conn;
    CS_COMMAND* cmd;
    char sbuf[64];

    	/** Scan to see if we already have a useable connection **/
	for(i=0;i<db_node->Conns.nItems;i++)
	    {
	    conn = (pSybdConn)(db_node->Conns.Items[i]);
	    if (conn->Busy == 0 && !strcmp(user,conn->Username) && !strcmp(pwd,conn->Password))
	        {
		conn->Busy = 1;
		return conn->SessionID;
		}
	    }

	/** Didn't find one?  First, make sure we aren't exceeding the limit **/
	if (db_node->Conns.nItems >= db_node->MaxConn)
	    {
	    /** Look for one to remove. **/
	    for(found_one=i=0;i<db_node->Conns.nItems;i++)
	        {
	        conn = (pSybdConn)(db_node->Conns.Items[i]);
		if (conn->Busy == 0 && conn->SessionID != NULL)
		    {
		    xaRemoveItem(&(db_node->Conns),xaFindItem(&(db_node->Conns),(void*)conn));
		    ct_close(conn->SessionID, CS_FORCE_CLOSE);
		    found_one = 1;
		    break;
		    }
		}

	    /** If all were busy, oops -- can't make another conn. **/
	    if (!found_one)
	        {
		mssError(1,"SYBD","Maximum connections exceeded.");
		return NULL;
		}
	    }
	else
	    {
	    conn = (pSybdConn)nmMalloc(sizeof(SybdConn));
	    conn->SessionID = NULL;
	    if (!conn)
	        {
		mssError(0,"SYBD","Could not allocate new connection structure");
		return NULL;
		}
	    }

	/** Make the new connection using the conn structure. **/
	if (!conn->SessionID)
	    {
	    ct_con_alloc(SYBD_INF.Context, &(conn->SessionID));
	    }
	if (conn->SessionID == NULL)
	    {
	    mssError(0,"SYBD","Could not alloc new database connection");
	    nmFree(conn,sizeof(SybdConn));
	    return NULL;
	    }
	ct_con_props(conn->SessionID, CS_SET, CS_USERNAME, user, CS_NULLTERM, NULL);
	ct_con_props(conn->SessionID, CS_SET, CS_PASSWORD, pwd, CS_NULLTERM, NULL);
	ct_con_props(conn->SessionID, CS_SET, CS_APPNAME, "Centrallix", CS_NULLTERM, NULL);
	gethostname(sbuf,63);
	sbuf[63]=0;
	ct_con_props(conn->SessionID, CS_SET, CS_HOSTNAME, sbuf, CS_NULLTERM, NULL);
	if (ct_connect(conn->SessionID, db_node->Server, CS_NULLTERM) != CS_SUCCEED)
	    {
	    mssError(0,"SYBD","Could not connect to database!");
	    return NULL;
	    }

	/** Do a USE DATABASE only if database was specified in the node. **/
	if (db_node->Database[0])
	    {
	    sprintf(sbuf,"use %s",db_node->Database);
	    cmd = sybd_internal_Exec(conn->SessionID, sbuf);
	    while((rval=ct_results(cmd, (CS_INT*)&i)))
	        {
	        if (rval == CS_FAIL)
	            {
		    mssError(0,"SYBD","Could not 'use' database!");
		    sybd_internal_Close(cmd);
		    return NULL;
		    }
	        if (rval == CS_END_RESULTS || i == CS_CMD_DONE) break;
	        }
	    sybd_internal_Close(cmd);
	    strcpy(conn->Username, user);
	    strcpy(conn->Password, pwd);
	    conn->Busy = 1;
	    xaAddItem(&(db_node->Conns),(void*)conn);
	    }

    return conn->SessionID;
    }


/*** sybd_internal_ReleaseConn - release a connection back to the connection
 *** pool for this database node.
 ***/
int
sybd_internal_ReleaseConn(pSybdNode db_node, CS_CONNECTION* session)
    {
    int i;
    pSybdConn conn;

    	/** Scan through the list looking for this session **/
	for(i=0;i<db_node->Conns.nItems;i++)
	    {
	    conn = (pSybdConn)(db_node->Conns.Items[i]);
	    if (conn->SessionID == session)
	        {
		conn->Busy = 0;
		return 0;
		}
	    }

	mssError(1,"SYBD","Critical internal error - releasing released connection!");

    return -1;
    }


/*** sybd_internal_OpenNode - attempts to lookup a driver node in the
 *** node cache, otherwise opens it from the filesystem.  Expects the
 *** given path to the datasource's node.  'path' specifies that path,
 *** 'mode' specifies the open mode of the objOpen transaction, and
 *** node_only, if 1, indicates that the user was opening the node itself
 *** rather than a sub-node (e.g., a table, row, or column).  'mask' is
 *** the permissions mask to use if creating.
 ***/
pSybdNode
sybd_internal_OpenNode(char* path, int mode, pObject obj, int node_only, int mask)
    {
    pSybdNode db_node;
    int type,i;
    CS_CONNECTION* s;
    CS_COMMAND* cmd;
    CS_INT restype;
    char* ptr;
    pSnNode snnode;

    	/** First, do a lookup in the db node cache. **/
	db_node = (pSybdNode)xhLookup(&(SYBD_INF.DBNodes),path);
	if (db_node) 
	    {
	    db_node->LastAccess = SYBD_INF.AccessCnt;
	    SYBD_INF.AccessCnt++;
	    return db_node;
	    }

    	/** Mask CREATE if not opening the node itself. **/
	if (!node_only) 
	    {
	    mode = O_RDONLY;
	    }

	/** Was the node newly created? **/
	if ((obj->Prev->Flags & OBJ_F_CREATED) && (mode & O_CREAT))
	    {
	    snnode = snNewNode(obj->Prev, "application/sybase");
	    if (!snnode)
	        {
		mssError(0,"SYBD","Database node create failed");
		return NULL;
		}
	    snSetParamString(snnode,obj->Prev,"server","SYBASE");
	    snSetParamString(snnode,obj->Prev,"database","master");
	    snSetParamString(snnode,obj->Prev,"annot_table","LSTableAnnotations");
	    snSetParamString(snnode,obj->Prev,"description","Sybase Database");
	    snSetParamInteger(snnode,obj->Prev,"max_connections",16);
	    snWriteNode(obj->Prev,snnode);
	    }
	else
	    {
	    snnode = snReadNode(obj->Prev);
	    if (!snnode)
	        {
		mssError(0,"SYBD","Database node open failed");
		return NULL;
		}
	    }

	/** Create the DB node and fill it in. **/
	db_node = (pSybdNode)nmMalloc(sizeof(SybdNode));
	if (!db_node)
	    {
	    mssError(0,"SYBD","Could not allocate DB node structure");
	    return NULL;
	    }
	db_node->SnNode = snnode;
	memset(db_node,0,sizeof(SybdNode));
	strcpy(db_node->Path,path);
	if (stAttrValue(stLookup(snnode->Data,"server"),NULL,&ptr,0) < 0) ptr = NULL;
	strncpy(db_node->Server,ptr?ptr:"",63);
	db_node->Server[63]=0;
	if (stAttrValue(stLookup(snnode->Data,"database"),NULL,&ptr,0) < 0) ptr = NULL;
	strncpy(db_node->Database,ptr?ptr:"",31);
	db_node->Database[31]=0;
	if (stAttrValue(stLookup(snnode->Data,"annot_table"),NULL,&ptr,0) < 0) ptr = NULL;
	strncpy(db_node->AnnotTable,ptr?ptr:"",31);
	db_node->AnnotTable[31]=0;
	if (stAttrValue(stLookup(snnode->Data,"description"),NULL,&ptr,0) < 0) ptr = NULL;
	strncpy(db_node->Description,ptr?ptr:"",255);
	db_node->Description[255]=0;
	if (stAttrValue(stLookup(snnode->Data,"max_connections"),&i,NULL,0) < 0) i=16;
	db_node->MaxConn = i;

	/** Did we get the required data for the connection? **/
	if (!db_node->Server[0])
	    {
	    nmFree(db_node, sizeof(SybdNode));
	    mssError(1,"SYBD","Database server must be specified in node");
	    return NULL;
	    }

	/** Add node to the db node cache **/
	xaInit(&(db_node->Conns),16);
	xhInit(&(db_node->TableInf),255,0);
	db_node->LastAccess = (SYBD_INF.AccessCnt++);
	xhAdd(&(SYBD_INF.DBNodes), db_node->Path, (void*)db_node);
	xaAddItem(&SYBD_INF.DBNodeList, (void*)db_node);

	/** Get a connection and get the types list. **/
	s = sybd_internal_GetConn(db_node, mssUserName(), mssPassword());
	if (s)
	    {
	    if ((cmd=sybd_internal_Exec(s,"select usertype,name from systypes")))
	        {
		while(ct_results(cmd, &restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			type = 0;
			ct_get_data(cmd, 1, &type, 4, (CS_INT*)&i);
			if (i==0) continue;
			ct_get_data(cmd, 2, db_node->Types[type], 13, (CS_INT*)&i);
			db_node->Types[type][i] = 0;
			}
		    }
		sybd_internal_Close(cmd);
		}
	    sybd_internal_ReleaseConn(db_node, s);
	    }

    return db_node;
    }


/*** sybd_internal_GetTableInf - returns a SybdTableInf structure containing 
 *** information about the attributes and primary keys of a given table.  The
 *** information is either obtained from the cache or read from the database.
 ***/
pSybdTableInf
sybd_internal_GetTableInf(pSybdNode node, CS_CONNECTION* session, char* table)
    {
    pSybdTableInf tdata;
    char sbuf[160];
    char* ptr;
    char* tmpptr;
    int l,i,col,find_col,restype;
    CS_COMMAND* cmd;

    	/** See if this table's metadata is cached. **/
	tdata = (pSybdTableInf)xhLookup(&(node->TableInf),table);
	if (tdata) return tdata;

	/** Allocate a new structure. **/
	tdata = (pSybdTableInf)nmMalloc(sizeof(SybdTableInf));
	if (!tdata) return NULL;
	memset(tdata,0,sizeof(SybdTableInf));
	strcpy(tdata->Table,table);
	tdata->ColBufSize = 1024;
	tdata->ColBuf = (char*)nmSysMalloc(tdata->ColBufSize);
	if (!(tdata->ColBuf)) 
	    {
	    nmFree(tdata,sizeof(SybdTableInf));
	    return NULL;
	    }
	tdata->ColBufLen = 0;
	tdata->nCols = 0;
	tdata->HasContent = 0;

	/** Build the query to get the cols. **/
	sprintf(sbuf,"SELECT c.name,c.colid,c.status,c.usertype FROM syscolumns c,sysobjects o WHERE c.id=o.id AND o.name='%s' ORDER BY c.colid",table);
	if (!(cmd=sybd_internal_Exec(session,sbuf)))
	    {
	    nmSysFree(tdata->ColBuf);
	    nmFree(tdata,sizeof(SybdTableInf));
	    mssError(0,"SYBD","Could not obtain column information for requested table");
	    return NULL;
	    }

	/** Read the result set **/
	ptr = tdata->ColBuf;
	while(ct_results(cmd, (CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	  while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	    {
	    /** Get the col name. **/
	    ct_get_data(cmd, 1, sbuf, 159, (CS_INT*)&i);
	    sbuf[i] = 0;
	    l = strlen(sbuf)+1;
	    if (tdata->ColBufLen + l >= tdata->ColBufSize)
	        {
		tmpptr = (char*)nmSysRealloc(tdata->ColBuf, tdata->ColBufSize+1024);
		if (!tmpptr)
		    {
		    sybd_internal_Close(cmd);
		    nmSysFree(tdata->ColBuf);
		    nmFree(tdata,sizeof(SybdTableInf));
		    mssError(1,"SYBD","Realloc() failed while building column name list for table");
		    return NULL;
		    }
		tdata->ColBuf = tmpptr;
		tdata->ColBufSize += 1024;
		}
	    memcpy(ptr,sbuf,l);
	    tdata->Cols[tdata->nCols] = ptr;
	    ptr += l;
	    tdata->ColBufLen += l;

	    /** Get the col id **/
	    tdata->ColIDs[tdata->nCols] = 0;
	    ct_get_data(cmd, 2, &(tdata->ColIDs[tdata->nCols]), 4, (CS_INT*)&i);

	    /** Check nullability.  Look at bit 3 of status field. **/
	    col = 0;
	    ct_get_data(cmd, 3, &col, 4, (CS_INT*)&i);
	    if (col & 8) tdata->ColFlags[tdata->nCols] |= SYBD_CF_ALLOWNULL;

	    /** Get the column data type. **/
	    tdata->ColTypes[tdata->nCols] = 0;
	    ct_get_data(cmd, 4, &(tdata->ColTypes[tdata->nCols]), 4, (CS_INT*)&i);
	    if (tdata->ColTypes[tdata->nCols] == 19 || tdata->ColTypes[tdata->nCols] == 20)
	        tdata->HasContent = 1;
	    tdata->nCols++;
	    }
	sybd_internal_Close(cmd);

	/** No columns?  If not, return err because table doesn't exist **/
	if (tdata->nCols == 0)
	    {
	    nmSysFree(tdata->ColBuf);
	    nmFree(tdata,sizeof(SybdTableInf));
	    mssError(1,"SYBD","Nonexistent table");
	    return NULL;
	    }

	/** Ok, done with that query.  Now load the primary key. **/
	sprintf(sbuf,"SELECT keycnt,key1,key2,key3,key4,key5,key6,key7,key8 FROM syskeys k, sysobjects o where k.id=o.id and o.name='%s' and k.type=1",table);
	if ((cmd=sybd_internal_Exec(session,sbuf)))
	    {
	    while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	      while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	        {
		tdata->nKeys = 0;
		ct_get_data(cmd, 1, &(tdata->nKeys), 4, (CS_INT*)&i);
		if (tdata->nKeys < 1 || tdata->nKeys > 8)
		    {
		    tdata->nKeys = 0;
		    }
		for(l=0;l<tdata->nKeys;l++)
		    {
		    col = 0;
		    ct_get_data(cmd, l+2, &col, 4, (CS_INT*)&i);
		    find_col = -1;
		    for(i=0;i<tdata->nCols;i++) if (tdata->ColIDs[i] == col) 
		        {
			find_col=i;
			break;
			}
		    tdata->KeyCols[l] = find_col;
		    tdata->Keys[l] = tdata->Cols[find_col];
		    tdata->ColFlags[find_col] |= SYBD_CF_PRIKEY;
		    tdata->ColKeys[find_col] = l;
		    }
		}
	    sybd_internal_Close(cmd);
	    }

	/** Finally, get annotation information for the table and for its rows. **/
	tdata->Annotation[0] = 0;
	tdata->RowAnnotExpr = NULL;
	if (*(node->AnnotTable))
	    {
	    sprintf(sbuf, "SELECT a,b,c FROM %s WHERE a = '%s'", node->AnnotTable, table);
	    if ((cmd=sybd_internal_Exec(session,sbuf)))
	        {
		while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			/** Get the table's annotation **/
			ct_get_data(cmd, 2, tdata->Annotation, 255, (CS_INT*)&i);
			tdata->Annotation[i] = 0;

			/** Get the annotation expression and compile it **/
			ct_get_data(cmd, 3, sbuf, 255, (CS_INT*)&i);
			sbuf[i] = 0;
			tdata->ObjList = expCreateParamList();
			expAddParamToList(tdata->ObjList, NULL, NULL, 0);
			tdata->RowAnnotExpr = (pExpression)expCompileExpression(sbuf, tdata->ObjList, MLX_F_ICASE | MLX_F_FILENAMES, 0);
			}
		    }
		sybd_internal_Close(cmd);
		}
	    }
	else
	    {
	    tdata->Annotation[0] = 0;
	    tdata->RowAnnotExpr = NULL;
	    }

	/** No annotation expression?  Set it to "" if not. **/
	if (!tdata->RowAnnotExpr)
	    {
	    tdata->ObjList = expCreateParamList();
	    expAddParamToList(tdata->ObjList, NULL, NULL, 0);
	    tdata->RowAnnotExpr = (pExpression)expCompileExpression("''", tdata->ObjList, MLX_F_ICASE | MLX_F_FILENAMES, 0);
	    }

	/** Cache the data **/
	xhAdd(&(node->TableInf),tdata->Table,(char*)tdata);

    return tdata;
    }


/*** sybd_internal_KeyToFilename - converts the primary key contents of the current
 *** query record back to the filename.  This filename consists of the primary key
 *** fields separated by | character(s).
 ***/
char*
sybd_internal_KeyToFilename(pSybdTableInf tdata, pSybdData inf)
    {
    static char fbuf[80];
    char* keyptrs[8];
    int i,col=0;
    char* ptr;

    	/** Get pointers to the key data. **/
	ptr = fbuf;
	for(i=0;i<tdata->nKeys;i++)
	    {
	    if (i>0) *(ptr++)='|';
	    col = 0;
	    keyptrs[i] = NULL;
	    switch(tdata->ColTypes[tdata->KeyCols[i]])
	        {
		case 7: /** INT **/
		    memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 4);
		    sprintf(ptr,"%d",col);
		    break;
		case 6: /** SMALLINT **/
		    memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 2);
		    sprintf(ptr, "%d", col);
		    break;
		case 5: /** TINYINT **/
		case 16: /** BIT **/
		    memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 1);
		    sprintf(ptr, "%d", col);
		    break;
		case 1: /** CHAR **/
		case 2: /** VARCHAR **/
		case 18: /** SYSNAME **/
		case 19: /** TEXT **/
		    strcpy(ptr, inf->ColPtrs[tdata->KeyCols[i]]);
		    break;
		}
	    ptr += strlen(ptr);
	    }

    return fbuf;
    }


/*** sybd_internal_FilenameToKey - converts a primary key filename to a where
 *** clause directing access for that key, for a given table within a given 
 *** database node.  The returned name is stored in a static storage area and
 *** must be copied from that place before allowing a context switch....
 ***/
char*
sybd_internal_FilenameToKey(pSybdNode node, CS_CONNECTION* session, char* table, char* filename)
    {
    static char wbuf[256];
    static char fbuf[80];
    char* wptr;
    pSybdTableInf key;
    char* ptr;
    int i;

	/** Lookup the key data **/
	key = sybd_internal_GetTableInf(node,session,table);
	if (!key) return NULL;

	/** Build the where clause condition **/
	strcpy(fbuf,filename);
	ptr = strtok(fbuf,"|");
	wbuf[0]=0;
	wptr = wbuf;
	i=0;
	while(ptr)
	    {
	    if (i>=key->nKeys)
	        {
		mssError(1,"SYBD","Illegal concat key length in filename (too long)");
		return NULL;
		}
	    if (wbuf[0]) 
	        {
		strcpy(wptr," AND ");
		wptr+=5;
		}
	    strcpy(wptr, key->Keys[i]);
	    wptr += strlen(wptr);
	    *(wptr++) = '=';
	    if (ptr[0] < '0' || ptr[0] > '9') *(wptr++)='"';
	    strcpy(wptr,ptr);
	    wptr += strlen(ptr);
	    if (ptr[0] < '0' || ptr[0] > '9') *(wptr++)='"';
	    ptr = strtok(NULL,"|");
	    i++;
	    }
	*wptr = 0;
	
	/** Verify user specified _enough_ keys **/
	if (i != key->nKeys)
	    {
	    mssError(1,"SYBD","Illegal concat key length in filename (too short)");
	    return NULL;
	    }

    return wbuf;
    }


/*** sybd_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
sybd_internal_DetermineType(pObject obj, pSybdData inf)
    {
    int i;

	/** Determine object type (depth) and get pointers set up **/
	obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;
	inf->TablePtr = NULL;
	inf->TableSubPtr = NULL;
	inf->RowColPtr = NULL;

	/** Set up pointers based on number of elements past the node object **/
	if (inf->Pathname.nElements == obj->SubPtr)
	    {
	    inf->Type = SYBD_T_DATABASE;
	    obj->SubCnt = 1;
	    }
	if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	    {
	    inf->Type = SYBD_T_TABLE;
	    inf->TablePtr = inf->Pathname.Elements[obj->SubPtr];
	    obj->SubCnt = 2;
	    }
	if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	    {
	    inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = SYBD_T_ROWSOBJ;
	    else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = SYBD_T_COLSOBJ;
	    obj->SubCnt = 3;
	    }
	if (inf->Pathname.nElements - 3 >= obj->SubPtr)
	    {
	    inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+2];
	    if (inf->Type == SYBD_T_ROWSOBJ) inf->Type = SYBD_T_ROW;
	    else if (inf->Type == SYBD_T_COLSOBJ) inf->Type = SYBD_T_COLUMN;
	    obj->SubCnt = 4;
	    }

    return 0;
    }



/*** sybd_internal_TreeToClause - convert an expression tree to the appropriate
 *** WHERE clause for the SQL statement.
 ***
 *** GRB - update works for a component of an ORDER BY as well.
 ***
 *** GRB - update; works for multiple-table sources as well.  This is useful for
 *** the mqsyb multiquery sybase passthrough component.
 ***/
int
sybd_internal_TreeToClause(pExpression tree, pSybdTableInf *tdata, int n_tdata, pXString where_clause)
    {
    pExpression subtree;
    int i,id = 0;

#if 0
    	/** What id in the tdata are we dealing with? **/
	if (tree->NodeType == EXPR_N_PROPERTY && tree->Parent)
	    id = expObjID(tree->Parent,NULL);
	else
	    id = expObjID(tree,NULL);
	if (id < 0) id = 0;
#endif

	/** If int or string, just put the content, otherwise recursively build **/
	switch (tree->NodeType)
	    {
	    case EXPR_N_DATETIME:
	  SYBD_DO_DATETIME:
	        objDataToString(where_clause, DATA_T_DATETIME, &(tree->Types.Date), DATA_F_QUOTED);
	        break;

	    case EXPR_N_MONEY:
	  SYBD_DO_MONEY:
	        objDataToString(where_clause, DATA_T_MONEY, &(tree->Types.Money), DATA_F_QUOTED);
	        break;

	    case EXPR_N_DOUBLE:
	  SYBD_DO_DOUBLE:
	        objDataToString(where_clause, DATA_T_DOUBLE, &(tree->Types.Double), DATA_F_QUOTED);
	  	break;

	    case EXPR_N_INTEGER:
	  SYBD_DO_INTEGER:
	        if (!tree->Parent || tree->Parent->NodeType == EXPR_N_AND ||
		    tree->Parent->NodeType == EXPR_N_OR)
		    {
		    if (tree->Integer)
		        xsConcatenate(where_clause, " (1=1) ", 6);
		    else
		        xsConcatenate(where_clause, " (1=0) ", 7);
		    }
		else
		    {
	            objDataToString(where_clause, DATA_T_INTEGER, &(tree->Integer), DATA_F_QUOTED);
		    }
		break;

	    case EXPR_N_STRING:
	  SYBD_DO_STRING:
	        if (tree->Parent && tree->Parent->NodeType == EXPR_N_FUNCTION && 
		    (!strcmp(tree->Parent->Name,"convert") || !strcmp(tree->Parent->Name,"datepart")) &&
		    (void*)tree == (void*)(tree->Parent->Children.Items[0]))
		    {
		    if (!strcmp(tree->Parent->Name,"convert"))
		        {
			if (!strcmp(tree->String,"integer")) xsConcatenate(where_clause,"int",3);
			else if (!strcmp(tree->String,"string")) xsConcatenate(where_clause,"varchar(255)",-1);
			else if (!strcmp(tree->String,"double")) xsConcatenate(where_clause,"double",6);
			else if (!strcmp(tree->String,"money")) xsConcatenate(where_clause,"money",5);
			else if (!strcmp(tree->String,"datetime")) xsConcatenate(where_clause,"datetime",8);
			}
		    else
		        {
			objDataToString(where_clause, DATA_T_STRING, tree->String, 0);
			}
		    }
		else
		    {
	            objDataToString(where_clause, DATA_T_STRING, tree->String, DATA_F_QUOTED);
		    }
		break;

	    case EXPR_N_OBJECT:
	        subtree = (pExpression)(tree->Children.Items[0]);
	        sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
		break;

	    case EXPR_N_PROPERTY:
	        /** 'Frozen' object?  IF so, use current tree value **/
		if (tree->Flags & EXPR_F_FREEZEEVAL || (tree->Parent && (tree->Parent->Flags & EXPR_F_FREEZEEVAL)))
		    {
		    if (tree->Flags & EXPR_F_NULL)
		        {
			xsConcatenate(where_clause, " NULL ", 6);
			break;
			}
		    if (tree->DataType == DATA_T_INTEGER) goto SYBD_DO_INTEGER;
		    else if (tree->DataType == DATA_T_STRING) goto SYBD_DO_STRING;
		    else if (tree->DataType == DATA_T_DATETIME) goto SYBD_DO_DATETIME;
		    else if (tree->DataType == DATA_T_MONEY) goto SYBD_DO_MONEY;
		    else if (tree->DataType == DATA_T_DOUBLE) goto SYBD_DO_DOUBLE;
		    }

	        /** Direct ref object? **/
	        if (tree->ObjID == -1)
		    {
		    /** This tdata->ObjList doesn't really fit, but we use it because
		     ** we need something here with the objectsystem's session in it.
		     **/
		    expEvalTree(tree,tdata[id]->ObjList);

		    /** Insert NULL, integer, or string, depending on evaluator results **/
		    if (tree->Flags & EXPR_F_NULL)
		        {
			xsConcatenate(where_clause, " NULL ", 6);
			}
		    else if (tree->DataType == DATA_T_INTEGER)
		        {
	                objDataToString(where_clause, DATA_T_INTEGER, &(tree->Integer), DATA_F_QUOTED);
			}
		    else if (tree->DataType == DATA_T_STRING)
		        {
	                objDataToString(where_clause, DATA_T_STRING, &(tree->String), DATA_F_QUOTED);
			}
		    else if (tree->DataType == DATA_T_DOUBLE)
		        {
	                objDataToString(where_clause, DATA_T_DOUBLE, &(tree->Types.Double), DATA_F_QUOTED);
			}
		    else if (tree->DataType == DATA_T_MONEY)
		        {
	                objDataToString(where_clause, DATA_T_MONEY, &(tree->Types.Money), DATA_F_QUOTED);
			}
		    else if (tree->DataType == DATA_T_DATETIME)
		        {
	                objDataToString(where_clause, DATA_T_DATETIME, &(tree->Types.Date), DATA_F_QUOTED);
			}
		    }
		else
		    {
		    /** Is this a special type of property (i.e., name or annotation?) **/
		    if (!strcmp(tree->Name,"name"))
		        {
			xsConcatenate(where_clause, " (",2);
			for(i=0;i<tdata[id]->nKeys;i++)
			    {
			    if (i != 0) xsConcatenate(where_clause, " + '|' + ", 9);
			    xsConcatenate(where_clause, "convert(varchar,", -1);
			    xsConcatenate(where_clause, tdata[id]->Keys[i], -1);
			    xsConcatenate(where_clause, ")", 1);
			    }
			xsConcatenate(where_clause, ") ",2);
			}
		    else if (!strcmp(tree->Name,"annotation"))
		        {
			xsConcatenate(where_clause, " (",2);
			if (!tdata[id]->RowAnnotExpr)
			    xsConcatenate(where_clause, " 1 ", 3);
			else
			    sybd_internal_TreeToClause(tdata[id]->RowAnnotExpr, tdata, n_tdata, where_clause);
			xsConcatenate(where_clause, ") ",2);
			}
		    else
		        {
		        /** "Normal" type of object... **/
	                xsConcatenate(where_clause, " ", 1);
	                xsConcatenate(where_clause, tree->Name, -1);
	                xsConcatenate(where_clause, " ", 1);
			}
		    }
		break;

	    case EXPR_N_COMPARE:
	        xsConcatenate(where_clause, " (", 2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, " ", 1);
		if (tree->CompareType & MLX_CMP_LESS) xsConcatenate(where_clause,"<",1);
		if (tree->CompareType & MLX_CMP_GREATER) xsConcatenate(where_clause,">",1);
		if (tree->CompareType & MLX_CMP_EQUALS) xsConcatenate(where_clause,"=",1);
	        xsConcatenate(where_clause, " ", 1);
	        subtree = (pExpression)(tree->Children.Items[1]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, ") ", 2);
	        break;

	    case EXPR_N_AND:
	        xsConcatenate(where_clause, " (",2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, " AND ",5);
	        subtree = (pExpression)(tree->Children.Items[1]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, ") ",2);
	        break;

	    case EXPR_N_OR:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
                xsConcatenate(where_clause, " OR ",4);
                subtree = (pExpression)(tree->Children.Items[1]);
                sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
                xsConcatenate(where_clause, ") ",2);
                break;

	    case EXPR_N_ISNULL:
	        xsConcatenate(where_clause, " (",2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
		xsConcatenate(where_clause, " IS NULL) ",10);
		break;

	    case EXPR_N_NOT:
	        xsConcatenate(where_clause, " ( NOT ( ",9);
		subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,tdata,n_tdata,where_clause);
		xsConcatenate(where_clause, " ) ) ",5);
		break;

	    case EXPR_N_FUNCTION:
	        /** Special case 'condition()' and 'ralign()' which Sybase doesn't have. **/
		if (!strcmp(tree->Name,"condition") && tree->Children.nItems == 3)
		    {
		    xsConcatenate(where_clause, " isnull((select substring(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",max(1),255) where ", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, "), ", 3);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") ", 2);
		    }
		else if (!strcmp(tree->Name,"ralign") && tree->Children.nItems == 2)
		    {
		    xsConcatenate(where_clause, " substring('", -1);
		    for(i=0;i<255 && i<((pExpression)(tree->Children.Items[1]))->Integer;i++)
		        xsConcatenate(where_clause, " ", 1);
		    xsConcatenate(where_clause, "',1,", 4);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, " - char_length(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ")) ", 3);
		    }
		else
		    {
	            xsConcatenate(where_clause, " ", 1);
		    xsConcatenate(where_clause, tree->Name, -1);
		    xsConcatenate(where_clause, "(", 1);
		    for(i=0;i<tree->Children.nItems;i++)
		        {
		        if (i != 0) xsConcatenate(where_clause,",",1);
		        subtree = (pExpression)(tree->Children.Items[i]);
		        sybd_internal_TreeToClause(subtree, tdata, n_tdata, where_clause);
		        }
		    xsConcatenate(where_clause, ") ", 2);
		    }
		break;

	    case EXPR_N_PLUS:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " + ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_MINUS:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " - ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_DIVIDE:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " / ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_MULTIPLY:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " * ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_IN:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " IN (", 5);
		subtree = (pExpression)(tree->Children.Items[1]);
		if (subtree->NodeType == EXPR_N_LIST)
		    {
		    for(i=0;i<subtree->Children.nItems;i++)
		        {
			if (i != 0) xsConcatenate(where_clause, ",", 1);
			sybd_internal_TreeToClause((pExpression)(subtree->Children.Items[i]), tdata, n_tdata, where_clause);
			}
		    }
		else
		    {
	            sybd_internal_TreeToClause(subtree, tdata, n_tdata, where_clause);
		    }
	        xsConcatenate(where_clause, ") ) ", 4);
		break;
	    }

	if (tree->Flags & EXPR_F_DESC) xsConcatenate(where_clause, " DESC ", 6);

    return 0;
    }


/*** sybd_internal_GetRow - copy a row to the inf structure.
 ***/
int
sybd_internal_GetRow(pSybdData inf, CS_COMMAND* s, int cnt)
    {
    int i,n;
    char* ptr;
    char* endptr;

    	/** Copy the row. **/
	endptr = inf->RowBuf;
	for(i=0;i<cnt;i++)
	    {
	    ptr = NULL;
	    ct_get_data(s, i+1, endptr, 255, (CS_INT*)&n);
	    if (n==0) 
	        {
		inf->ColPtrs[i] = NULL;
		}
	    else
	        {
		inf->ColPtrs[i] = endptr;
		endptr[n] = 0;
		endptr+=(n+1);
		}
	    }

    return 0;
    }



/*** sybdOpen - open a table, row, or column.
 ***/
void*
sybdOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSybdData inf;
    char sbuf[256];
    int i,cnt,restype,n,ncols;
    pObjTrxTree new_oxt;
    pSybdTableInf tdata;
    CS_COMMAND* cmd;

	/** Allocate the structure **/
	inf = (pSybdData)nmMalloc(sizeof(SybdData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(SybdData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine type and set pointers. **/
	sybd_internal_DetermineType(obj,inf);

	/** Access the DB node. **/
	inf->Node = sybd_internal_OpenNode(obj_internal_PathPart(obj->Pathname,0,obj->SubPtr),obj->Mode,obj,inf->Type == SYBD_T_DATABASE,mask);
	obj_internal_PathPart(obj->Pathname,0,0);
	if (!(inf->Node))
	    {
	    mssError(0,"SYBD","Could not open database node!");
	    nmFree(inf,sizeof(SybdData));
	    return NULL;
	    }

	/** Grab a database connection **/
	inf->ReadSessID = NULL;
	inf->WriteSessID = NULL;
	inf->RWCmd = NULL;
	inf->Size = -1;
	inf->SessionID = sybd_internal_GetConn(inf->Node, mssUserName(), mssPassword());
	if (inf->SessionID == NULL)
	    {
	    mssError(0,"SYBD","Database session setup failed");
	    nmFree(inf,sizeof(SybdData));
	    return NULL;
	    }

	/** Verify the table, if a table mentioned **/
	if (inf->TablePtr)
	    {
	    tdata = sybd_internal_GetTableInf(inf->Node, inf->SessionID, inf->TablePtr);
	    inf->TData = tdata;
	    if (!tdata && (!(obj->Mode & O_CREAT) || inf->TableSubPtr))
	        {
		mssError(1,"SYBD","Requested table does not exist");
		sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		nmFree(inf,sizeof(SybdData));
		return NULL;
		}
	    else if (!tdata && (obj->Mode & O_CREAT) && !(inf->TableSubPtr))
	        {
		/** Create a new table, but we don't have the columns yet **/
		if (!*oxt)
		    {
		    *oxt = obj_internal_AllocTree();
		    (*oxt)->Status = OXT_S_VISITED;
		    (*oxt)->OpType = OXT_OP_CREATE;
		    (*oxt)->Object = (void*)obj;
		    (*oxt)->LLParam = (void*)inf;
		    }

		/** Add the 'rows' and 'columns' entities if not already. **/
		if ((*oxt)->Children.nItems == 0)
		    {
		    new_oxt = obj_internal_AllocTree();
		    new_oxt->AllocObj = 1;
		    new_oxt->Object = (void*)nmMalloc(sizeof(Object));
		    new_oxt->OpType = OXT_OP_CREATE;
		    new_oxt->Status = OXT_S_VISITED;
		    ((pObject)(new_oxt->Object))->Pathname = 
		        (pPathname)obj_internal_NormalizePath(obj->Pathname->Pathbuf,"rows");
		    obj_internal_AddChildTree(*oxt,new_oxt);
		    new_oxt = obj_internal_AllocTree();
		    new_oxt->AllocObj = 1;
		    new_oxt->Object = (void*)nmMalloc(sizeof(Object));
		    new_oxt->OpType = OXT_OP_CREATE;
		    new_oxt->Status = OXT_S_VISITED;
		    ((pObject)(new_oxt->Object))->Pathname = 
		        (pPathname)obj_internal_NormalizePath(obj->Pathname->Pathbuf,"columns");
		    obj_internal_AddChildTree(*oxt,new_oxt);
		    }
		sybd_internal_ReleaseConn(inf->Node,inf->SessionID);
		inf->SessionID = NULL;
		(*oxt)->OpType = OXT_OP_CREATE;
		return inf;
		}
	    }
	
	/** If a column is being accessed, verify its existence / open the metadata row **/
	if (inf->Type == SYBD_T_COLUMN)
	    {
	    inf->ColID = -1;
	    for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->RowColPtr, inf->TData->Cols[i]))
	        {
		inf->ColID = i;
		break;
		}
	    if (inf->ColID == -1)
	        {
		/** User specified a column that doesn't exist. **/
		if (!(obj->Mode & O_CREAT))
		    {
		    mssError(1,"SYBD","Column object does not exist.");
		    sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		    nmFree(inf,sizeof(SybdData));
		    return NULL;
		    }

		/** Adding a column but not in a table-level transaction? **/
		if ((!*oxt) || !((*oxt)->Parent) || !((*oxt)->Parent->Parent))
		    {
		    mssError(1,"SYBD","Attempt to add column outside of table create");
		    sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		    nmFree(inf,sizeof(SybdData));
		    return NULL;
		    }

		/** Ok - column add.  Release the conn and return. **/
		sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		inf->SessionID = NULL;
		if (!*oxt) *oxt = obj_internal_AllocTree();
		(*oxt)->Status = OXT_S_VISITED;
		(*oxt)->OpType = OXT_OP_CREATE;
		(*oxt)->LLParam = (void*)inf;
		return inf;
		}
	    }

	/** If a row is being accessed, verify its existence / open its row **/
	if (inf->Type == SYBD_T_ROW)
	    {
	    sprintf(sbuf,"SELECT * from %s WHERE %s",inf->TablePtr,
	      sybd_internal_FilenameToKey(inf->Node,inf->SessionID,inf->TablePtr,inf->RowColPtr));
	    if ((cmd=sybd_internal_Exec(inf->SessionID, sbuf)) == NULL)
	        {
		mssError(0,"SYBD","Could not retrieve row object from database");
		sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		nmFree(inf,sizeof(SybdData));
		return NULL;
		}
	    cnt = 0;
	    while (ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	        {
	        ct_res_info(cmd, CS_NUMDATA, (CS_INT*)&ncols, CS_UNUSED, NULL);
	        for(i=0;i<ncols;i++) inf->ColNum[i] = (unsigned char)i;
	        while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&n) == CS_SUCCEED)
	            {
		    cnt++;
		    sybd_internal_GetRow(inf,cmd,ncols);
		    }
		}
	    sybd_internal_Close(cmd);
	    if (cnt == 0)
	        {
		/** User specified a row that doesn't exist. **/
		if (!(obj->Mode & O_CREAT))
		    {
		    mssError(1,"SYBD","Row object does not exist.");
		    sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		    nmFree(inf,sizeof(SybdData));
		    return NULL;
		    }

		/** Ok - row insert.  Release the conn and return.  Do the work on close(). **/
		sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		inf->SessionID = NULL;
		if (!*oxt) *oxt = obj_internal_AllocTree();
		(*oxt)->OpType = OXT_OP_CREATE;
		(*oxt)->LLParam = (void*)inf;
		return inf;
		}
	    }

    return (void*)inf;
    }


/*** sybd_internal_InsertRow - inserts a new row into the database, looking
 *** throught the OXT structures for the column values.  For text/image columns,
 *** it automatically inserts "" for an unspecified column value when the column
 *** does not allow nulls.
 ***/
int
sybd_internal_InsertRow(pSybdData inf, CS_CONNECTION* session, pObjTrxTree oxt)
    {
    char* kptr;
    char* kendptr;
    int i,j,len,ctype,restype;
    pObjTrxTree attr_oxt, find_oxt;
    CS_COMMAND* cmd;
    pXString insbuf;

        /** Allocate a buffer for our insert statement. **/
	insbuf = (pXString)nmMalloc(sizeof(XString));
        if (!insbuf)
            {
            return -1;
            }
	xsInit(insbuf);
	xsConcatenate(insbuf, "INSERT INTO ", 12);
	xsConcatenate(insbuf, inf->TablePtr, -1);
	xsConcatenate(insbuf, " VALUES (", 9);

        /** Ok, look for the attribute sub-OXT's **/
        for(j=0;j<inf->TData->nCols;j++)
            {
	    /** If primary key, we have values in inf->RowColPtr. **/
	    if (inf->TData->ColFlags[j] & SYBD_CF_PRIKEY)
	        {
		/** Add , separator if necessary **/
		if (j!=0) xsConcatenate(insbuf,",",1);

		/** Determine position,length within prikey-coded name **/
		kptr = inf->RowColPtr;
		for(i=0;i<inf->TData->ColKeys[j];i++) kptr = strchr(kptr,'|')+1;
		kendptr = strchr(kptr,'|');
		if (!kendptr) len = strlen(kptr); else len = kendptr-kptr;

		/** Copy it to the INSERT statement buffer **/
		ctype = inf->TData->ColTypes[j];
		if (ctype == 5 || ctype == 6 || ctype == 7 || ctype == 16)
		    {
		    xsConcatenate(insbuf, kptr, len);
		    }
		else if (ctype == 1 || ctype == 2 || ctype == 18 || ctype == 19)
		    {
		    xsConcatenate(insbuf, "\"", 1);
		    xsConcatenate(insbuf, kptr, len);
		    xsConcatenate(insbuf, "\"", 1);
		    }
		}
	    else
	        {
		/** Otherwise, we scan through the OXT's **/
                find_oxt=NULL;
                for(i=0;i<oxt->Children.nItems;i++)
                    {
                    attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
                    /*if (((pSybdData)(attr_oxt->LLParam))->Type == SYBD_T_ATTR)*/
		    if (attr_oxt->OpType == OXT_OP_SETATTR)
                        {
                        if (!strcmp(attr_oxt->AttrName,inf->TData->Cols[j]))
                            {
                            find_oxt = attr_oxt;
                            find_oxt->Status = OXT_S_COMPLETE;
                            break;
                            }
                        }
                    }
		if (j!=0) xsConcatenate(insbuf,",",1);

                /** Print the appropriate type. **/
                if (!find_oxt)
                    {
                    if (inf->TData->ColFlags[j] & SYBD_CF_ALLOWNULL)
                        {
			xsConcatenate(insbuf,"NULL",4);
                        }
                    else if (inf->TData->ColTypes[j] == 19 || inf->TData->ColTypes[j] == 20)
		        {
			xsConcatenate(insbuf,"''",2);
		        }
		    else
                        {
                        mssError(1,"SYBD","Required column not specified in object create");
                        nmFree(insbuf,4096);
                        return -1;
                        }
                    }
                else 
		    {
		    objDataToString(insbuf, find_oxt->AttrType, find_oxt->AttrValue, DATA_F_QUOTED);
		    }
                }
	    }

        /** Add the trailing ')' and issue the query. **/
	xsConcatenate(insbuf,")", 1);
        if ((cmd = sybd_internal_Exec(session,insbuf->String))==NULL)
            {
	    xsDeInit(insbuf);
            nmFree(insbuf,sizeof(XString));
            sybd_internal_Close(cmd);
	    mssError(0,"SYBD","Could not execute SQL to insert new table row");
            return -1;
            }
	while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED);
        sybd_internal_Close(cmd);
	xsDeInit(insbuf);
        nmFree(insbuf,sizeof(XString));

    return 0;
    }


/*** sybdClose - close an open file or directory.
 ***/
int
sybdClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int i;
    struct stat fileinfo;
    char sbuf[160];

    	/** Is a ReadSess in progress? **/
	if (inf->ReadSessID != NULL) 
	    {
	    if (inf->RWCmd) sybd_internal_Close(inf->RWCmd);
	    inf->RWCmd = NULL;
	    sybd_internal_ReleaseConn(inf->Node, inf->ReadSessID);
	    inf->ReadSessID = NULL;
	    }

	/** Write session is a bit more complex.  IF needed, complete the write. **/
	if (inf->WriteSessID != NULL) 
	    {
	    if (inf->RWCmd) 
	        {
		/** If using a tmp file, copy to database now that we have the size **/
		if (inf->Size == -1)
		    {
		    fdClose(inf->TmpFD,0);
		    stat(inf->TmpFile, &fileinfo);
		    inf->ContentIODesc.total_txtlen = fileinfo.st_size;
		    inf->ContentIODesc.log_on_update = CS_TRUE;
		    ct_data_info(inf->RWCmd, CS_SET, CS_UNUSED, &(inf->ContentIODesc));
		    inf->TmpFD = fdOpen(inf->TmpFile, O_RDONLY, 0600);
		    while((i=fdRead(inf->TmpFD,sbuf,160,0,0)))
		        {
			ct_send_data(inf->RWCmd, sbuf, (CS_INT)i);
			}
		    }

		/** Clean up after the data send operation. **/
		ct_send(inf->RWCmd);
		if (inf->Size == -1) 
		    {
		    fdClose(inf->TmpFD,0);
		    unlink(inf->TmpFile);
		    }
		while(ct_results(inf->RWCmd, (CS_INT*)&i) == CS_SUCCEED);
		sybd_internal_Close(inf->RWCmd);
		}
	    inf->RWCmd = NULL;
	    sybd_internal_ReleaseConn(inf->Node, inf->WriteSessID);
	    inf->WriteSessID = NULL;
	    }

    	/** Was this a create? **/
	if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	    {
	    switch (inf->Type)
	        {
		case SYBD_T_TABLE:
		    /** We'll get to this a little later **/
		    break;

		case SYBD_T_ROW:
		    /** Need to get a session? **/
		    if (inf->SessionID == NULL) inf->SessionID = 
		        sybd_internal_GetConn(inf->Node, mssUserName(), mssPassword());

		    /** Perform the insert. **/
		    if (sybd_internal_InsertRow(inf,inf->SessionID, *oxt) < 0)
		        {
			/** FAIL the oxt. **/
			(*oxt)->Status = OXT_S_FAILED;

			/** Release the open object data **/
			sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
			inf->SessionID = NULL;
			nmFree(inf,sizeof(SybdData));
			return -1;
			}

		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case SYBD_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }

	/** Disconnect the DB session, if needed. **/
	if (inf->SessionID != NULL)
	    {
	    sybd_internal_ReleaseConn(inf->Node,inf->SessionID);
	    inf->SessionID = NULL;
	    }

	/** Free the info structure **/
	nmFree(inf,sizeof(SybdData));

    return 0;
    }


/*** sybdCreate - create a new object without actually opening that 
 *** object.
 ***/
int
sybdCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* objd;

    	/** Open and close the object **/
	obj->Mode |= O_CREAT;
	objd = sybdOpen(obj,mask,systype,usrtype,oxt);
	if (!objd) return -1;

    return sybdClose(objd,oxt);
    }


/*** sybdDelete - delete an existing object.
 ***/
int
sybdDelete(pObject obj, pObjTrxTree* oxt)
    {
    char sbuf[256];
    pSybdData inf;
    CS_COMMAND* cmd;

	/** Allocate the structure **/
	inf = (pSybdData)nmMalloc(sizeof(SybdData));
	if (!inf) return -1;
	memset(inf,0,sizeof(SybdData));
	inf->Obj = obj;

	/** Determine type and set pointers. **/
	sybd_internal_DetermineType(obj,inf);

	/** If a row, proceed else fail the delete. **/
	if (inf->Type != SYBD_T_ROW)
	    {
	    nmFree(inf,sizeof(SybdData));
	    puts("Unimplemented delete operation in SYBD.");
	    mssError(1,"SYBD","Unimplemented delete operation in SYBD");
	    return -1;
	    }

	/** Access the DB node. **/
	inf->Node = sybd_internal_OpenNode(obj->Pathname->Pathbuf,obj->Mode,obj,inf->Type == SYBD_T_DATABASE,0600);
	if (!(inf->Node))
	    {
	    mssError(0,"SYBD","Could not open database node!");
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }

	/** Grab a database connection **/
	inf->SessionID = sybd_internal_GetConn(inf->Node, mssUserName(), mssPassword());
	if (inf->SessionID == NULL)
	    {
	    mssError(0,"SYBD","Database session setup failed");
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }

	/** Create the where clause for the delete. **/
	sprintf(sbuf,"DELETE FROM %s WHERE %s",inf->TablePtr,
	    sybd_internal_FilenameToKey(inf->Node,inf->SessionID,inf->TablePtr,inf->RowColPtr));

	/** Run the delete **/
	if ((cmd = sybd_internal_Exec(inf->SessionID,sbuf)) == NULL)
	    {
	    mssError(0,"SYBD","Delete operation failed");
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }
	sybd_internal_Close(cmd);
	sybd_internal_ReleaseConn(inf->Node,inf->SessionID);

	/** Free the structure **/
	nmFree(inf,sizeof(SybdData));

    return 0;
    }


/*** sybd_internal_PrepareText - prepares for a Text or Image I/O operation
 *** to the database.  This routine retrieves the i/o descriptor and gets the
 *** text/image column ready for reading, with a given max text size.  If 
 *** preparing for an update operation, the calling routine can just close()
 *** the returned CS_COMMAND*.
 ***/
CS_COMMAND*
sybd_internal_PrepareText(pSybdData inf, CS_CONNECTION* session, int maxtextsize)
    {
    CS_COMMAND* cmd;
    char* col;
    int i;
    char buffer[1];
    char sbuf[160];
    int rcnt,rval;

	/** Determine column to use. **/
	for(i=0;i<inf->TData->nCols;i++)
	    {
	    if (inf->TData->ColTypes[i] == 19 || inf->TData->ColTypes[i] == 20)
		{
		col = inf->TData->Cols[i];
		break;
		}
	    }
	if (!col) 
	    {
	    mssError(1,"SYBD","This table's rows do not have content");
	    return NULL;
	    }

	/** If writing, make sure we have a textptr **/
	if (inf->WriteSessID != NULL)
	    {
	    sprintf(sbuf,"UPDATE %s SET %s='' where %s", inf->TablePtr,col,
	        sybd_internal_FilenameToKey(inf->Node, 
		session,inf->TablePtr,inf->RowColPtr));
	    if ((inf->RWCmd = sybd_internal_Exec(session, sbuf)) == NULL) 
	        {
		mssError(0,"SYBD","Could not run update to initialize textptr for content BLOB");
		return NULL;
		}
	    while (ct_results(inf->RWCmd,(CS_INT*)&rval)==CS_SUCCEED);
	    sybd_internal_Close(inf->RWCmd);
	    }

	/** Build the command. **/
	sprintf(sbuf,"set textsize %d select %s from %s where %s set textsize 255",
	    maxtextsize,col,inf->TablePtr,sybd_internal_FilenameToKey(inf->Node,
	        session,inf->TablePtr,inf->RowColPtr));
	if ((inf->RWCmd = sybd_internal_Exec(session, sbuf)) == NULL) 
	    {
	    mssError(0,"SYBD","Could not run SQL to retrieve content BLOB from database");
	    return NULL;
	    }

	/** Wait for the actual result row to come back. **/
	while (ct_results(inf->RWCmd,(CS_INT*)&rval)==CS_SUCCEED && rval!=CS_ROW_RESULT);
	if (rval != CS_ROW_RESULT)
	    {
	    sybd_internal_Close(inf->RWCmd);
	    mssError(1,"SYBD","SQL query to retrieve content BLOB failed");
	    return NULL;
	    }
	if (ct_fetch(inf->RWCmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) != CS_SUCCEED)
	    {
	    sybd_internal_Close(inf->RWCmd);
	    mssError(1,"SYBD","Row does not exist");
	    return NULL;
	    }

	/** Get the i/o descriptor for the content BLOB **/
	ct_get_data(inf->RWCmd,1,buffer,0,(CS_INT*)&rcnt);
	if (ct_data_info(inf->RWCmd,CS_GET,1,&(inf->ContentIODesc)) != CS_SUCCEED)
	    {
	    sybd_internal_Close(inf->RWCmd);
	    mssError(1,"SYBD","Could not retrieve descriptor for content BLOB");
	    return NULL;
	    }

    return cmd;
    }


/*** sybdRead - read from the object's content.  This only returns content
 *** when a row is opened within a table that has any attribute that
 *** is a 'text' or 'image' data type.  If more than one attribute has such
 *** a datatype, the first one is used.
 ***/
int
sybdRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int rcnt,rval;

    	/** Only for row objects. **/
	if (inf->Type != SYBD_T_ROW) 
	    {
	    mssError(1,"SYBD","Only database row objects have content");
	    return -1;
	    }

	/** Is a write session in progress?  If so, clear it. **/
	if (inf->WriteSessID != NULL)
	    {
	    if (inf->RWCmd) sybd_internal_Close(inf->RWCmd);
	    inf->RWCmd = NULL;
	    if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->WriteSessID;
	    else sybd_internal_ReleaseConn(inf->Node, inf->WriteSessID);
	    inf->WriteSessID = NULL;
	    }

    	/** Need to get a session first?  If so, get it and start the query. **/
	if (inf->ReadSessID == NULL)
	    {
	    /** Get the session. **/
	    if (inf->SessionID != NULL)
	        {
		inf->ReadSessID = inf->SessionID;
		inf->SessionID = NULL;
		inf->Flags |= SYBD_F_TFRRW;
		}
	    else
	        {
		inf->ReadSessID = sybd_internal_GetConn(inf->Node, mssUserName(), mssPassword());
		if (!inf->ReadSessID) 
		    {
		    mssError(0,"SYBD","Could not get database session to read database BLOB");
		    return -1;
		    }
		}

	    /** Get ready to do the i/o. **/
	    if (sybd_internal_PrepareText(inf, inf->ReadSessID, 256000) == NULL)
	        {
		if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->ReadSessID;
		else sybd_internal_ReleaseConn(inf->Node, inf->ReadSessID);
		inf->ReadSessID = NULL;
		inf->Flags &= ~SYBD_F_TFRRW;
		return -1;
		}
	    }

	/** Ok, we got the query underway.  Start reading results. **/
	rcnt = 0;
	if (inf->RWCmd)
	    {
	    rval = ct_get_data(inf->RWCmd,1,buffer,maxcnt,(CS_INT*)&rcnt);
	    if (rval == CS_END_ITEM || rval == CS_END_DATA)
	        {
	        sybd_internal_Close(inf->RWCmd);
	        inf->RWCmd = NULL;
	        }
	    else 
	        {
	        if (rval != CS_SUCCEED) rcnt = -1;
		}
	    }

	/** End of data? **/
	if (rcnt <= 0)
	    {
	    if (inf->RWCmd) sybd_internal_Close(inf->RWCmd);
	    inf->RWCmd = NULL;
	    if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->ReadSessID;
	    else sybd_internal_ReleaseConn(inf->Node, inf->ReadSessID);
	    inf->ReadSessID = NULL;
	    inf->Flags &= ~SYBD_F_TFRRW;
	    }

    return rcnt;
    }


/*** sybd_internal_OpenTmpFile - name/create a new tmp file and return a file 
 *** descriptor for it.
 ***/
pFile
sybd_internal_OpenTmpFile(char* name)
    {
    char ch;
    time_t t;

	/** Use 26 letters, with timestamp and random value **/
	t = time(NULL);
    	for(ch='a';ch<='z';ch++)
	    {
	    sprintf(name,"/tmp/LS-%8.8lX%4.4lX%c",t,lrand48()&0xFFFF,ch);
	    if (access(name,F_OK) < 0)
	        {
		return fdOpen(name, O_RDWR, 0600);
		}
	    }
	mssErrorErrno(1,"SYBD","Could not open temp file");

    return NULL;
    }


/*** sybdWrite - write to an object's content.  If the user specified a 'size' value
 *** and O_TRUNC was specified, use that size and stream the writes to the database.
 *** Otherwise, if just 'size' specified, use the greater of 'size' and current size,
 *** and pad with NUL's if 'size' is smaller.  If 'size' not specified, we have to write
 *** the Write() calls to a temp file first, determine the size, and proceed as before,
 *** deleting the temp file when done.  Sigh.  Reason?  Sybase wants to know the exact
 *** size of a BLOB write operation before writing can start.
 ***/
int
sybdWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);

    	/** Only for row objects. **/
	if (inf->Type != SYBD_T_ROW)
	    {
	    mssError(1,"SYBD","Only database row objects have content");
	    return -1;
	    }

	/** Is a read session in progress?  If so, clear it. **/
	if (inf->ReadSessID != NULL)
	    {
	    if (inf->RWCmd) sybd_internal_Close(inf->RWCmd);
	    inf->RWCmd = NULL;
	    if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->ReadSessID;
	    else sybd_internal_ReleaseConn(inf->Node, inf->ReadSessID);
	    inf->ReadSessID = NULL;
	    }

    	/** Need to get a session first?  If so, get it and start the query. **/
	if (inf->WriteSessID == NULL)
	    {
	    /** Get the session. **/
	    if (inf->SessionID != NULL)
	        {
		inf->WriteSessID = inf->SessionID;
		inf->SessionID = NULL;
		inf->Flags |= SYBD_F_TFRRW;
		}
	    else
	        {
		inf->WriteSessID = sybd_internal_GetConn(inf->Node, mssUserName(), mssPassword());
		if (!inf->WriteSessID) 
		    {
		    mssError(0,"SYBD","Could not get database session for BLOB write");
		    return -1;
		    }
		}

	    /** In an insert-row transaction? **/
	    if (*oxt && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	        {
	        /** If so, complete the insert before we do this. **/
	        sybd_internal_InsertRow(inf,inf->WriteSessID,*oxt);
		(*oxt)->Status = OXT_S_COMPLETE;
	        }

	    /** Get ready to do the i/o. **/
	    if (sybd_internal_PrepareText(inf, inf->WriteSessID, 255) == NULL)
	        {
		if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->WriteSessID;
		else sybd_internal_ReleaseConn(inf->Node, inf->WriteSessID);
		inf->WriteSessID = NULL;
		inf->Flags &= ~SYBD_F_TFRRW;
		return -1;
		}

	    /** Initiate the send data. **/
	    sybd_internal_Close(inf->RWCmd);
	    ct_cmd_alloc(inf->WriteSessID, &(inf->RWCmd));
	    ct_command(inf->RWCmd, CS_SEND_DATA_CMD, NULL, CS_UNUSED, CS_COLUMN_DATA);

	    /** IF size specified, tell Sybase about it now. **/
	    if (inf->Size >= 0) 
	        {
		inf->ContentIODesc.total_txtlen = inf->Size;
		inf->ContentIODesc.log_on_update = CS_TRUE;
		ct_data_info(inf->RWCmd, CS_SET, CS_UNUSED, &(inf->ContentIODesc));
		}
	    }

	/** Ok, got the i/o descriptor.  Do we need to open a tmp file? **/
	if (inf->Size == -1 && !inf->TmpFD)
	    {
	    inf->TmpFD = sybd_internal_OpenTmpFile(inf->TmpFile);
	    if (!inf->TmpFD)
	        {
		if (inf->Flags & SYBD_F_TFRRW) inf->SessionID = inf->WriteSessID;
		else sybd_internal_ReleaseConn(inf->Node, inf->WriteSessID);
		inf->WriteSessID = NULL;
		inf->Flags &= ~SYBD_F_TFRRW;
		return -1;
		}
	    }

	/** Alrighty then.  Send the data to Sybase or to the File. **/
	if (inf->Size == -1)
	    {
	    if (fdWrite(inf->TmpFD, buffer, cnt, 0,0) < 0) 
	        {
		mssErrorErrno(1,"SYBD","Write to BLOB temp file failed");
		return -1;
		}
	    }
	else
	    {
	    if (ct_send_data(inf->RWCmd, buffer, (CS_INT)cnt) != CS_SUCCEED) 
	        {
		mssError(1,"SYBD","Write to Sybase BLOB failed");
		return -1;
		}
	    }

    return 0;
    }


/*** sybdOpenQuery - open a directory query.  We basically reformat the where clause
 *** and issue a query to the DB.
 ***/
void*
sybdOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    pSybdQuery qy;
    int i,restype;
    XString sql;

	/** Allocate the query structure **/
	qy = (pSybdQuery)nmMalloc(sizeof(SybdQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(SybdQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->SessionID = NULL;
	qy->ObjSession = NULL;
	qy->Cmd = NULL;

	/** State that we won't do full query unless we get to SYBD_T_ROWSOBJ below **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;
	query->Flags &= ~OBJ_QY_F_FULLSORT;

	/** Build the query SQL based on object type. **/
	qy->Cmd = NULL;
	switch(inf->Type)
	    {
	    case SYBD_T_DATABASE:
	        /** Select the list of tables from the DB. **/
		if (SYBD_USE_CURSORS)
		    sprintf(qy->SQLbuf,"DECLARE _c CURSOR FOR SELECT name from sysobjects where type = 'U' ORDER BY name");
		else
		    sprintf(qy->SQLbuf,"SELECT name from sysobjects where type = 'U' ORDER BY name");
		qy->SessionID = inf->SessionID;
		qy->ObjSession = inf->SessionID;
		inf->SessionID = NULL;
		if (qy->SessionID == NULL)
		    qy->SessionID = sybd_internal_GetConn(inf->Node,mssUserName(),mssPassword());
		if (qy->SessionID == NULL)
		    {
		    nmFree(qy,sizeof(SybdQuery));
		    mssError(0,"SYBD","Could not obtain database session to list tables");
		    return NULL;
		    }
		if ((qy->Cmd=sybd_internal_Exec(qy->SessionID, qy->SQLbuf))==NULL)
		    {
		    nmFree(qy,sizeof(SybdQuery));
		    mssError(0,"SYBD","Could not execute SQL for query on database object");
		    return NULL;
		    }
		qy->RowCnt = 0;
		break;

	    case SYBD_T_TABLE:
	        /** No SQL needed -- always returns just 'columns' and 'rows' **/
		qy->RowCnt = 0;
	        break;

	    case SYBD_T_COLSOBJ:
	        /** Get a columns list. **/
		qy->TableInf = qy->ObjInf->TData;
		qy->RowCnt = 0;
		break;

	    case SYBD_T_ROWSOBJ:
	        /** Query the rows within a table -- traditional sql query here. **/
		qy->SessionID = inf->SessionID;
		qy->ObjSession = inf->SessionID;
		inf->SessionID = NULL;
		qy->RowCnt = 0;
		if (qy->SessionID == NULL)
		    qy->SessionID = sybd_internal_GetConn(inf->Node,mssUserName(),mssPassword());
		if (qy->SessionID == NULL)
		    {
		    nmFree(qy,sizeof(SybdQuery));
		    mssError(0,"SYBD","Could not obtain database session to list rows");
		    return NULL;
		    }
		qy->TableInf = sybd_internal_GetTableInf(inf->Node,qy->SessionID,inf->TablePtr);
		xsInit(&sql);
		query->Flags |= (OBJ_QY_F_FULLQUERY | OBJ_QY_F_FULLSORT);
		if (query->Tree)
		    {
		    xsConcatenate(&sql, " WHERE ", 7);
		    qy->TableInf->ObjList->Session = qy->ObjInf->Obj->Session;
		    sybd_internal_TreeToClause((pExpression)(query->Tree),&(qy->TableInf),1,&sql);
		    }
		if (query->SortBy[0])
		    {
		    xsConcatenate(&sql," ORDER BY ", 10);
		    for(i=0;query->SortBy[i];i++)
			{
			if (i != 0) xsConcatenate(&sql, ", ", 2);
			sybd_internal_TreeToClause((pExpression)(query->SortBy[i]),&(qy->TableInf),1,&sql);
			}
	  	    }
		if (SYBD_USE_CURSORS)
		    sprintf(qy->SQLbuf,"DECLARE _c CURSOR FOR SELECT * FROM %s %s",inf->TablePtr, sql.String);
		else
		    sprintf(qy->SQLbuf,"SELECT * FROM %s %s",inf->TablePtr, sql.String);
		if (strcmp(qy->SQLbuf, SYBD_INF.LastSQL.String) || 1)
		    {
		    if (SYBD_SHOW_SQL) printf("SQL = %s\n",qy->SQLbuf);
		    xsCopy(&SYBD_INF.LastSQL, qy->SQLbuf, -1);
		    }
		xsDeInit(&sql);
		if ((qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf))==NULL)
		    {
		    nmFree(qy,sizeof(SybdQuery));
		    mssError(0,"SYBD","Could not execute SQL for query on table object");
		    return NULL;
		    }
		break;

	    case SYBD_T_COLUMN:
	    case SYBD_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(SybdQuery));
		qy = NULL;
		break;
	    }

	/** Cursor mode retrieval?  Open cursor if so. **/
	if (SYBD_USE_CURSORS && qy->Cmd)
	    {
	    while(ct_results(qy->Cmd,(CS_INT*)&restype)==CS_SUCCEED && restype != CS_CMD_DONE);
	    sybd_internal_Close(qy->Cmd);
	    sprintf(qy->SQLbuf,"OPEN _c SET CURSOR ROWS %d FOR _c FETCH _c", SYBD_CURSOR_ROWCOUNT);
	    qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	    if (!qy->Cmd)
	        {
		nmFree(qy,sizeof(SybdQuery));
		mssError(0,"SYBD","Could not open cursor for query result set retrieval");
		return NULL;
		}
	    sprintf(qy->SQLbuf,"FETCH _c");
	    qy->RowsSinceFetch = 0;
	    }

    return (void*)qy;
    }


/*** sybdQueryFetch - get the next directory entry as an open object.
 ***/
void*
sybdQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pSybdQuery qy = ((pSybdQuery)(qy_v));
    pSybdData inf;
    char filename[80];
    char* ptr;
    int new_type;
    int i,cnt;
    pSybdTableInf tdata = qy->ObjInf->TData;
    CS_CONNECTION* s2;
    int restype;

    	/** Fetch the row. **/
	if (qy->SessionID != NULL)
	    {
	    cnt=0;
	    while(1)
	        {
	        if (qy->RowsSinceFetch == 0)
	            {
	            while(ct_results(qy->Cmd,(CS_INT*)&restype)==CS_SUCCEED)
		        {
		        if (restype==CS_ROW_RESULT)
		            {
			    cnt++;
			    break;
			    }
		        }
	            if (cnt == 0) return NULL;
		    }
	        cnt = 0;
	        while(ct_fetch(qy->Cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	            {
		    cnt++;
		    break;
		    }

		/** Handle the row or lack of rows, and possibly do another FETCH if cursor-mode **/
		if (cnt > 0) 
		    {
		    /** Got a row.  Good -- return it. **/
		    break;
		    }
		else if (cnt == 0 && (!(SYBD_USE_CURSORS) || qy->RowsSinceFetch < SYBD_CURSOR_ROWCOUNT))
		    {
		    /** No rows left and fewer than rowcount (or no) rows returned - query over. **/
		    return NULL;
		    }
		else
		    {
		    /** No rows but we filled the cursor last time.  Do another fetch. **/
	    	    sybd_internal_Close(qy->Cmd);
	    	    qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	    	    if (!qy->Cmd)
	        	{
			mssError(0,"SYBD","Could not fetch next part of query results");
			return NULL;
			}
	    	    qy->RowsSinceFetch = 0;
		    }
		}
	    if (cnt == 0) return NULL;
	    }
	qy->RowCnt++;
	qy->RowsSinceFetch++;

	/** Allocate the structure **/
	inf = (pSybdData)nmMalloc(sizeof(SybdData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(SybdData));
	inf->TData = tdata;

    	/** Get the next name based on the query type. **/
	switch(qy->ObjInf->Type)
	    {
	    case SYBD_T_DATABASE:
	        /** Get filename from the first column - table name. **/
		new_type = SYBD_T_TABLE;
		ct_get_data(qy->Cmd, 1, filename, 80, (CS_INT*)&i);
		filename[i] = 0;
		s2 = sybd_internal_GetConn(qy->ObjInf->Node, mssUserName(), mssPassword());
		tdata = sybd_internal_GetTableInf(qy->ObjInf->Node,s2,filename);
		inf->TData = tdata;
		sybd_internal_ReleaseConn(qy->ObjInf->Node,s2);
	        break;

	    case SYBD_T_TABLE:
	        /** Filename is either "rows" or "columns" **/
		if (qy->RowCnt == 1) 
		    {
		    strcpy(filename,"columns");
		    new_type = SYBD_T_COLSOBJ;
		    }
		else if (qy->RowCnt == 2) 
		    {
		    strcpy(filename,"rows");
		    new_type = SYBD_T_ROWSOBJ;
		    }
		else 
		    {
		    nmFree(inf,sizeof(SybdData));
		    /*mssError(1,"SYBD","Table object has only two subobjects: 'rows' and 'columns'");*/
		    return NULL;
		    }
	        break;

	    case SYBD_T_ROWSOBJ:
	        /** Get the filename from the primary key of the row. **/
		sybd_internal_GetRow(inf,qy->Cmd,qy->TableInf->nCols);
		new_type = SYBD_T_ROW;
	        strcpy(filename,sybd_internal_KeyToFilename(qy->TableInf,inf));
	        break;

	    case SYBD_T_COLSOBJ:
	        /** Loop through the columns in the TableInf structure. **/
		new_type = SYBD_T_COLUMN;
		if (qy->RowCnt <= qy->TableInf->nCols)
		    {
		    strcpy(filename,qy->TableInf->Cols[qy->RowCnt-1]);
		    }
		else
		    {
		    nmFree(inf,sizeof(SybdData));
		    return NULL;
		    }
	        break;
	    }

	/** Build the filename. **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	*(ptr++) = '/';
	strcpy(ptr,filename);
	obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;
	/*inf = sybdOpen(obj, mode & (~O_CREAT), NULL, "");*/

	/** Fill out the remainder of the structure. **/
	inf->Obj = obj;
	inf->Mask = 0600;
	inf->Type = new_type;
	inf->Node = qy->ObjInf->Node;
	obj->SubPtr = qy->ObjInf->Obj->SubPtr;
	sybd_internal_DetermineType(obj,inf);

    return (void*)inf;
    }


/*** sybdQueryDelete - delete the contents of a query result set.  This is
 *** not yet supported.
 ***/
int
sybdQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** sybdQueryClose - close the query.
 ***/
int
sybdQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pSybdQuery qy = ((pSybdQuery)(qy_v));
    int restype,rid;

    	/** Release the command structure. **/
	if (qy->Cmd) sybd_internal_Close(qy->Cmd);

	/** Deallocate the cursor? **/
	if (SYBD_USE_CURSORS && (qy->ObjInf->Type == SYBD_T_DATABASE || qy->ObjInf->Type == SYBD_T_ROWSOBJ))
	    {
	    sprintf(qy->SQLbuf,"CLOSE _c DEALLOCATE CURSOR _c");
	    qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	    if (qy->Cmd)
	        {
	        while((rid=ct_results(qy->Cmd,(CS_INT*)&restype))==CS_SUCCEED && restype != CS_CMD_DONE)
		    {
		    /*printf("ctr=%d, typ=%d;  ",rid,restype);*/
		    }
		/*printf("ctr=%d, typ=%d.\n",rid,restype);*/
		sybd_internal_Close(qy->Cmd);
		qy->Cmd = NULL;
		}
	    }

	/** Release the session id back to the object, or just release it. **/
	if (qy->SessionID != NULL)
	    {
	    if (qy->ObjSession != NULL)
	        {
		if (qy->ObjInf->SessionID == NULL)
		    {
		    qy->ObjInf->SessionID = qy->SessionID;
		    }
		else
		    {
		    sybd_internal_ReleaseConn(qy->ObjInf->Node, qy->SessionID);
		    }
		}
	    else
	        {
		sybd_internal_ReleaseConn(qy->ObjInf->Node, qy->SessionID);
		}
	    }

	/** Free the structure **/
	nmFree(qy,sizeof(SybdQuery));

    return 0;
    }


/*** sybdGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
sybdGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int i;
    pSybdTableInf tdata;

    	/** Name attribute?  String. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** Content-type attribute?  String. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Annotation?  String. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Attr type depends on object type. **/
	if (inf->Type == SYBD_T_ROW)
	    {
	    tdata = inf->TData;
	    for(i=0;i<tdata->nCols;i++)
	        {
		if (!strcmp(attrname,tdata->Cols[i]))
		    {
		    if (tdata->ColTypes[i] == 5 || tdata->ColTypes[i] == 6 ||
		        tdata->ColTypes[i] == 7 || tdata->ColTypes[i] == 16)
			return DATA_T_INTEGER;
		    else if (tdata->ColTypes[i] == 1 || tdata->ColTypes[i] == 2 ||
		        tdata->ColTypes[i] == 19 || tdata->ColTypes[i] == 18)
			return DATA_T_STRING;
		    else if (tdata->ColTypes[i] == 22 || tdata->ColTypes[i] == 12)
		        return DATA_T_DATETIME;
		    else if (tdata->ColTypes[i] == 11 || tdata->ColTypes[i] == 21)
		        return DATA_T_MONEY;
		    else if (tdata->ColTypes[i] == 8 || tdata->ColTypes[i] == 23)
		        return DATA_T_DOUBLE;
		    }
		}
	    }
	else if (inf->Type == SYBD_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    }

	mssError(1,"SYBD","Invalid column for GetAttrType");

    return -1;
    }


/*** sybdGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
sybdGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int i,t,minus,n;
    unsigned int msl,lsl,divtmp;
    pSybdTableInf tdata;
    char* ptr;
    int days,fsec;
    float f;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    ptr = inf->Pathname.Elements[inf->Pathname.nElements-1];
	    if (ptr[0] == '.' && ptr[1] == '\0')
	        {
	        *((char**)val) = "/";
		}
	    else
	        {
	        *((char**)val) = ptr;
		}
	    return 0;
	    }

	/** Is it an annotation? **/
	if (!strcmp(attrname, "annotation"))
	    {
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE:
		    *(char**)val = inf->Node->Description;
		    break;
		case SYBD_T_TABLE:
		    *(char**)val = inf->TData->Annotation;
		    break;
		case SYBD_T_ROWSOBJ:
		    *(char**)val = "Contains rows for this table";
		    break;
		case SYBD_T_COLSOBJ:
		    *(char**)val = "Contains columns for this table";
		    break;
		case SYBD_T_COLUMN:
		    *(char**)val = "Column within this table";
		    break;
		case SYBD_T_ROW:
		    if (!inf->TData->RowAnnotExpr)
		        {
			*(char**)val = "";
			break;
			}
		    expModifyParam(inf->TData->ObjList, NULL, inf->Obj);
		    inf->TData->ObjList->Session = inf->Obj->Session;
		    expEvalTree(inf->TData->RowAnnotExpr, inf->TData->ObjList);
		    if (inf->TData->RowAnnotExpr->Flags & EXPR_F_NULL ||
		        inf->TData->RowAnnotExpr->String == NULL)
			{
			*(char**)val = "";
			}
		    else
		        {
			*(char**)val = inf->TData->RowAnnotExpr->String;
			}
		    break;
		}
	    return 0;
	    }

	/** If Attr is content-type, report the type. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE: *((char**)val) = "system/void"; break;
		case SYBD_T_TABLE: *((char**)val) = "system/void"; break;
		case SYBD_T_ROWSOBJ: *((char**)val) = "system/void"; break;
		case SYBD_T_COLSOBJ: *((char**)val) = "system/void"; break;
		case SYBD_T_ROW: 
		    {
		    if (inf->TData->HasContent)
		        *((char**)val) = "application/octet-stream";
		    else
		        *((char**)val) = "system/void";
		    break;
		    }
		case SYBD_T_COLUMN: *((char**)val) = "system/void"; break;
		}
	    return 0;
	    }

	/** Outer type... **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE: *((char**)val) = "application/sybase"; break;
		case SYBD_T_TABLE: *((char**)val) = "system/table"; break;
		case SYBD_T_ROWSOBJ: *((char**)val) = "system/table-rows"; break;
		case SYBD_T_COLSOBJ: *((char**)val) = "system/table-columns"; break;
		case SYBD_T_ROW: *((char**)val) = "system/row"; break;
		case SYBD_T_COLUMN: *((char**)val) = "system/column"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == SYBD_T_COLUMN)
	    {
	    if (strcmp(attrname,"datatype")) return -1;

	    /** Get the table info. **/
	    tdata=inf->TData;

	    /** Search table info for this column. **/
	    for(i=0;i<tdata->nCols;i++) if (!strcmp(tdata->Cols[i],inf->RowColPtr))
	        {
		*((char**)val) = inf->Node->Types[tdata->ColTypes[i]];
		return 0;
		}
	    }
	else if (inf->Type == SYBD_T_ROW)
	    {
	    /** Get the table info. **/
	    tdata = inf->TData;

	    /** Search through the columns. **/
	    for(i=0;i<tdata->nCols;i++) if (!strcmp(tdata->Cols[i],attrname))
	        {
		ptr = inf->ColPtrs[i];
		t = tdata->ColTypes[i];
		if (ptr == NULL) return 1;
		if (t==5 || t==6 || t==7 || t==16)
		    {
		    *(int*)val = 0;
		    if (t==5 || t==16) memcpy(val,ptr,1);
		    else if (t==6) memcpy(val,ptr,2);
		    else memcpy(val,ptr,4);
		    return 0;
		    }
		else if (t==1 || t==2 || t==18 || t==19)
		    {
		    *(char**)val = ptr;
		    return 0;
		    }
		else if (t==22 || t==12)
		    {
		    /** datetime **/
		    *(pDateTime*)val = &(inf->Types.Date);
		    memcpy(&days,ptr,4);
		    memcpy(&fsec,ptr+4,4);

		    /** Convert the time **/
		    fsec /= 300;
		    inf->Types.Date.Part.Hour = fsec/3600;
		    fsec -= (inf->Types.Date.Part.Hour*3600);
		    inf->Types.Date.Part.Minute = fsec/60;
		    fsec -= (inf->Types.Date.Part.Minute*60);
		    inf->Types.Date.Part.Second = fsec;

		    /** Convert the date **/
		    if (days > 364) days++; /* hack to cover 1900 not being leap year */
		    days = days*4;
		    inf->Types.Date.Part.Year = days / (365*4 + 1);
		    days -= (inf->Types.Date.Part.Year * (365*4 + 1));
		    days = (days)/4;
		    inf->Types.Date.Part.Month = 0;
		    for(n=0;n<12;n++) 
		        {
			if (days >= (obj_month_days[n] + ((n==1 && IS_LEAP_YEAR(inf->Types.Date.Part.Year+1900))?1:0)))
		            {
			    inf->Types.Date.Part.Month++;
			    days -= (obj_month_days[n] + ((n==1 && IS_LEAP_YEAR(inf->Types.Date.Part.Year+1900))?1:0));
			    }
			else
			    {
			    break;
			    }
			}
		    inf->Types.Date.Part.Day = days;
		    return 0;
		    }
		else if (t == 8)
		    {
		    /** float **/
		    memcpy(val, ptr, 8);
		    return 0;
		    }
		else if (t == 23)
		    {
		    memcpy(&f, ptr, 4);
		    *(double*)val = f;
		    return 0;
		    }
		else if (t == 11 || t == 21)
		    {
		    /** money **/
		    *(pMoneyType*)val = &(inf->Types.Money);
		    if (t == 21)
		        {
			/** smallmoney, 4-byte **/
			memcpy(&i, ptr, 4);
			inf->Types.Money.WholePart = i/10000;
			if (i < 0 && (i%10000) != 0) inf->Types.Money.WholePart--;
			inf->Types.Money.FractionPart = i - (inf->Types.Money.WholePart*10000);
			return 0;
			}
		    else
		        {
			/** normal 8-byte money **/
			memcpy(&lsl, ptr+4, 4);
			memcpy(&msl, ptr, 4);
			minus = 0;
			if (msl >= 0x80000000)
			    {
			    /** Negate **/
			    minus = 1;
			    msl = ~msl;
			    lsl = ~lsl;
			    if (lsl == 0xFFFFFFFF) msl++;
			    lsl++;
			    }
			/** Long division, 16 bits = 1 "digit" **/
			divtmp = msl/10000;
			n = divtmp;
			msl -= divtmp*10000;
			msl = (msl<<16) + (lsl>>16);
			divtmp = msl/10000;
			n = (n<<16) + divtmp;
			msl -= divtmp*10000;
			msl = (msl<<16) + (lsl & 0xFFFF);
			divtmp = msl/10000;
			n = (n<<16) + divtmp;
			msl -= divtmp*10000;
			inf->Types.Money.WholePart = n;
			inf->Types.Money.FractionPart = msl;
			if (minus)
			    {
			    inf->Types.Money.WholePart = -inf->Types.Money.WholePart;
			    if (inf->Types.Money.FractionPart > 0)
			        {
				inf->Types.Money.WholePart--;
				inf->Types.Money.FractionPart = 10000 - inf->Types.Money.FractionPart;
				}
			    }
			return 0;
			}
		    }
		}
	    }

	mssError(1,"SYBD","Invalid column for GetAttrValue");

    return -1;
    }


/*** sybdGetNextAttr - get the next attribute name for this object.
 ***/
char*
sybdGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    pSybdTableInf tdata;

	/** Attribute listings depend on object type. **/
	switch(inf->Type)
	    {
	    case SYBD_T_DATABASE:
	        return NULL;
	
	    case SYBD_T_TABLE:
	        return NULL;

	    case SYBD_T_ROWSOBJ:
	        return NULL;

	    case SYBD_T_COLSOBJ:
	        return NULL;

	    case SYBD_T_COLUMN:
	        /** only attr is 'datatype' **/
		if (inf->CurAttr++ == 0) return "datatype";
	        break;

	    case SYBD_T_ROW:
	        /** Get the table info. **/
		tdata = inf->TData;

	        /** Return attr in table inf **/
		if (inf->CurAttr < tdata->nCols) return tdata->Cols[inf->CurAttr++];
	        break;
	    }

    return NULL;
    }


/*** sybdGetFirstAttr - get the first attribute name for this object.
 ***/
char*
sybdGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = sybdGetNextAttr(inf_v,oxt);

    return ptr;
    }


/*** sybdSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
sybdSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int type,rval;
    CS_COMMAND* cmd;
    CS_CONNECTION* sess;
    char sbuf[160];

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Type == SYBD_T_DATABASE) return -1;
	    }

	/** Changing the 'annotation'? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    /** Choose the appropriate action based on object type **/
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE:
		    memccpy(inf->Node->Description, *(char**)val, '\0', 255);
		    inf->Node->Description[255] = 0;
		    /**
		    objParamsSet(inf->Node->Params, "description", *(char**)val, 0);
		    ptr = obj_internal_PathPart(inf->Obj->Pathname, 0, inf->Obj->SubPtr);
		    node_fd = fdOpen(ptr, O_WRONLY, 0600);
		    if (node_fd)
		        {
			objParamsWrite(node_fd, inf->Node->Params);
			fdClose(node_fd,0);
			}
		    obj_internal_PathPart(inf->Obj->Pathname, 0, 0);
		    **/
		    break;
		    
		case SYBD_T_TABLE:
		    memccpy(inf->TData->Annotation, *(char**)val, '\0', 255);
		    inf->TData->Annotation[255] = 0;
		    if (inf->Node->AnnotTable[0])
		        {
		        /** Get a session. **/
		        sess = inf->SessionID;
		        if (!sess) sess=sybd_internal_GetConn(inf->Node,mssUserName(),mssPassword());
		        if (!sess) return -1;

			/** Build the SQL to update the annotation table **/
			sprintf(sbuf, "UPDATE %s set b = '%s' WHERE a = '%s'", inf->Node->AnnotTable,
				inf->TData->Annotation, inf->TData->Table);
			cmd = sybd_internal_Exec(sess,sbuf);
			if (cmd)
			    {
		    	    while(ct_results(cmd, (CS_INT*)&rval) == CS_SUCCEED);
			    sybd_internal_Close(cmd);
			    }
		        if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,sess);
			}
		    break;

		case SYBD_T_ROWSOBJ:
		case SYBD_T_COLSOBJ:
		case SYBD_T_COLUMN:
		    /** Can't change any of these (yet) **/
		    return -1;

		case SYBD_T_ROW:
		    /** Not yet implemented :) **/
		    return -1;
		}
	    return 0;
	    }

	/** If this is a row, check the OXT. **/
	if (inf->Type == SYBD_T_ROW)
	    {
	    /** Attempting to set 'suggested' size for content write? **/
	    if (!strcmp(attrname,"size")) 
	        {
		inf->Size = *(int*)val;
		}
	    else
	        {
	        /** Otherwise, check Oxt. **/
	        if (*oxt)
	            {
		    /** We're within a transaction.  Fill in the oxt. **/
		    type = sybdGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    (*oxt)->AllocObj = 0;
		    (*oxt)->Object = NULL;
		    (*oxt)->Status = OXT_S_VISITED;
		    strcpy((*oxt)->AttrName, attrname);
		    obj_internal_SetTreeAttr(*oxt, type, val);
		    }
	        else
	            {
		    /** Get a session. **/
		    sess = inf->SessionID;
		    if (!sess) sess=sybd_internal_GetConn(inf->Node,mssUserName(),mssPassword());
		    if (!sess) return -1;

		    /** No transaction.  Simply do an update. **/
		    type = sybdGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
		        {
	                sprintf(sbuf,"UPDATE %s SET %s=%s WHERE %s",inf->TablePtr,
	                    attrname,objDataToStringTmp(type,val,DATA_F_QUOTED),
			    sybd_internal_FilenameToKey(inf->Node,
			    sess,inf->TablePtr,inf->RowColPtr));
			}
		    else if (type == DATA_T_STRING || type == DATA_T_MONEY || type == DATA_T_DATETIME)
		        {
	                sprintf(sbuf,"UPDATE %s SET %s='%s' WHERE %s",inf->TablePtr,
	                    attrname,objDataToStringTmp(type,*(void**)val,DATA_F_QUOTED),
			    sybd_internal_FilenameToKey(inf->Node,
			    sess,inf->TablePtr,inf->RowColPtr));
			}

		    /** Start the update. **/
		    cmd = sybd_internal_Exec(sess,sbuf);
		    if (!cmd) 
		        {
			mssError(1,"SYBD","Could not execute SQL to update attribute value");
			return -1;
			}

		    /** Read the results and release the db session **/
		    rval = CS_FAIL;
		    while(ct_results(cmd, (CS_INT*)&rval) == CS_SUCCEED);
		    sybd_internal_Close(cmd);
		    if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,sess);
		    }
		}
	    }
	else
	    {
	    /** Don't support setattr on anything else yet. **/
	    return -1;
	    }

    return 0;
    }


/*** sybdAddAttr - add an attribute to an object.  This doesn't work for
 *** unix filesystem objects, so we just deny the request.
 ***/
int
sybdAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** sybdOpenAttr - open an attribute as if it were an object with 
 *** content.  The Sybase database objects don't yet have attributes that are
 *** suitable for this.
 ***/
void*
sybdOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** sybdGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
sybdGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** sybdGetNextMethod -- same as above.  Always fails. 
 ***/
char*
sybdGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** sybdExecuteMethod - No methods to execute, so this fails.
 ***/
int
sybdExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** mqsybAnalyze - Sybase passthrough multiquery driver routine.  This
 *** function analyzes the QS structure and existing QE sources, and determines
 *** 1) how many data sources come from a single SYBD node object, and then 2)
 *** how they should be combined into a single Sybase passthrough query.  IT
 *** MUST BE NOTED: queries using Sybase passthrough are non-updateable and the
 *** objects returned as a part of the query CANNOT be normally handled as 
 *** objectsystem objects.  This is a restriction of using the passthrough mode
 *** and is a tradeoff to obtain the higher performance of having the joins and
 *** such performed at the Sybase level.
 ***/
int
mqsybAnalyze(pMultiQuery mq)
    {
    pQueryStructure where_qs = NULL, from_qs, with_qs, select_qs = NULL;
    pQueryStructure item, orderby_qs, groupby_qs;
    int is_passthrough, isnt_unique;
    pSybdNode from_nodes[16];
    pQueryStructure from_sources[16];
    pObject from_tmpobjs[16];
    int n_unique_nodes = 0;
    pSybdNode unique_from_nodes[16];
    int srcs_per_unique_node[16];
    int id_remapping[16];
    int i,j,k,mask,n_items;
    pSybdNode node;
    char* pathpart;
    pQueryElement qe;
    pExpression new_exp;

    	/** First, search for SELECT clauses that are parts of PASSTHROUGH queries **/
	while((select_qs = mq_internal_FindItem(mq->QTree, MQ_T_SELECTCLAUSE, select_qs)) != NULL)
	    {
	    /** Is this SELECT a part of a passthrough query? **/
	    is_passthrough = 0;
	    with_qs = NULL;
	    while((with_qs = mq_internal_FindItem(select_qs->Parent, MQ_T_WITHCLAUSE, with_qs)) != NULL)
	        {
		if (!strcmp(with_qs->Name,"passthrough")) 
		    {
		    is_passthrough = 1;
		    break;
		    }
		}
	    if (!is_passthrough) continue;

	    /** Ok, is passthrough.  Find what FROM sources we can handle **/
	    from_qs = mq_internal_FindItem(mq->QTree, MQ_T_FROMCLAUSE, NULL);
	    if (!from_qs) continue;
	    for(i=0;i<from_qs->Children.nItems;i++)
	        {
		item = (pQueryStructure)(from_qs->Children.Items[i]);
		from_tmpobjs[i] = objOpen(mq->SessionID, item->Source, O_RDONLY, 0600, "system/directory");
		if (from_tmpobjs[i]->Driver == SYBD_INF.ObjDriver ||
		    (from_tmpobjs[i]->Driver == OSYS.TransLayer && from_tmpobjs[i]->LowLevelDriver == SYBD_INF.ObjDriver))
		    {
		    from_sources[i] = item;
		    }
		else
		    {
		    from_sources[i] = NULL;
		    objClose(from_tmpobjs[i]);
		    from_tmpobjs[i] = NULL;
		    }
		}

	    /** Ok, find out which node is associated with which from sources. **/
	    /** We know it'll be in the list because we just opened it. **/
	    for(i=0;i<from_qs->Children.nItems;i++)
	        {
		item = (pQueryStructure)(from_qs->Children.Items[i]);

		/** Try looking up components of the pathname in the DBNodes hash table **/
		/** Passthru only valid if querying a TableName/rows, not db or cols or rows **/
		for(j=0;j<4 && j<from_tmpobjs[i]->Pathname->nElements;j++)
		    {
		    pathpart = obj_internal_PathPart(from_tmpobjs[i]->Pathname,0,from_tmpobjs[i]->Pathname->nElements-j);
		    node = (pSybdNode)xhLookup(&SYBD_INF.DBNodes, (void*)pathpart);
		    if (node && j==2) 
		        {
			from_nodes[i] = node;
			break;
			}
		    }
		objClose(from_tmpobjs[i]);
		from_tmpobjs[i] = NULL;

		/** Build the unique node list. **/
		for(j=isnt_unique=0;j<n_unique_nodes;j++)
		    {
		    if (node == unique_from_nodes[j])
		        {
			isnt_unique = 1;
			srcs_per_unique_node[j]++;
			break;
			}
		    }
		if (!isnt_unique)
		    {
		    srcs_per_unique_node[n_unique_nodes] = 1;
		    unique_from_nodes[n_unique_nodes++] = node;
		    }
		}

	    /** Alrighty then.  We have the list of nodes and from sources. **/
	    /** Handle all groups that have at least two sources for a node **/
	    for(i=0;i<n_unique_nodes;i++)
	        {
		if (srcs_per_unique_node[i] >= 2)
		    {
		    /** Mark the sources used, so the projection module doesn't get em **/
		    for(k=mask=j=0;j<from_qs->Children.nItems;j++) if (from_nodes[j] == unique_from_nodes[i])
		        {
			from_sources[j]->Flags |= MQ_SF_USED;
			mask |= (1<<j);
			id_remapping[k++] = j;
			}

		    /** Allocate a QE structure **/
		    qe = mq_internal_AllocQE();
		    qe->Driver = SYBD_INF.QueryDriver;

		    /** Grab up the SELECT items relevant to this passthrough query. **/
		    for(j=0;j<select_qs->Children.nItems;j++)
		        {
			item = (pQueryStructure)(select_qs->Children.Items[j]);
			if ((item->Expr->ObjCoverageMask & mask) &&
			    !(item->Expr->ObjCoverageMask & ~mask))
			    {
			    item->QELinkage = qe;
			    xaAddItem(&qe->AttrNames, (void*)item->Presentation);
			    xaAddItem(&qe->AttrExprPtr, (void*)item->RawData.String);
			    xaAddItem(&qe->AttrCompiledExpr, (void*)item->Expr);
			    xaAddItem(&qe->AttrDeriv, (void*)(item->QELinkage));
			    }
			}
		    
		    /** Find WHERE clause items relevant to this query. **/
		    qe->Constraint = NULL;
		    where_qs = mq_internal_FindItem(select_qs->Parent, MQ_T_WHERECLAUSE, NULL);
		    if (where_qs) for(j=0;j<where_qs->Children.nItems;j++)
		        {
			item = (pQueryStructure)(where_qs->Children.Items[j]);
			if ((item->Expr->ObjCoverageMask & mask) &&
			    !(item->Expr->ObjCoverageMask & ~mask))
			    {
			    if (qe->Constraint)
			        {
				new_exp = expAllocExpression();
				new_exp->NodeType = EXPR_N_AND;
				xaAddItem(&new_exp->Children, (void*)qe->Constraint);
				xaAddItem(&new_exp->Children, (void*)item->Expr);
				qe->Constraint = new_exp;
				}
			    else
			        {
				qe->Constraint = item->Expr;
				}
			    xaRemoveItem(&where_qs->Children,j);
			    j--;
			    continue;
			    }
			}

		    /** Now find order-by stuff that is relevant to this query. **/
		    n_items = 0;
		    memset(qe->OrderBy,0,sizeof(pExpression)*17);
		    orderby_qs = mq_internal_FindItem(select_qs->Parent, MQ_T_ORDERBYCLAUSE, NULL);
		    if (orderby_qs) for(j=0;j<orderby_qs->Children.nItems;j++)
		        {
			item = (pQueryStructure)(orderby_qs->Children.Items[j]);
			if (item->Expr && (item->Expr->ObjCoverageMask & mask) &&
                            !(item->Expr->ObjCoverageMask & ~mask))
			    {
			    qe->OrderBy[n_items++] = item->Expr;
			    item->Expr = NULL;
			    for(k=0;k<srcs_per_unique_node[i];k++) expRemapID(qe->OrderBy[n_items-1], id_remapping[k], k);
			    }
			}
		    groupby_qs = mq_internal_FindItem(select_qs->Parent, MQ_T_ORDERBYCLAUSE, NULL);
		    if (groupby_qs) for(j=0;j<groupby_qs->Children.nItems;j++)
		        {
			item = (pQueryStructure)(groupby_qs->Children.Items[j]);
			if (item->Expr && (item->Expr->ObjCoverageMask & mask) &&
                            !(item->Expr->ObjCoverageMask & ~mask))
			    {
			    qe->OrderBy[n_items++] = exp_internal_CopyTree(item->Expr);
			    for(k=0;k<srcs_per_unique_node[i];k++) expRemapID(qe->OrderBy[n_items-1], id_remapping[k], k);
			    }
			}
		    qe->OrderBy[n_items] = NULL;
		    }
		}
	    }

    return 0;
    }


/*** mqsybStart - Sybase passthrough multiquery driver routine.  This function
 *** starts the query.
 ***/
int
mqsybStart(pQueryElement qe, pMultiQuery mq, pExpression additional_expr)
    {
    return 0;
    }


/*** mqsybNextItem - Retrieve the next item in the query result set.  Return:
 ***   1 == valid row fetched.
 ***   0 == end of result set; no row fetched.
 ***  -1 == error occurred.
 ***/
int
mqsybNextItem(pQueryElement qe, pMultiQuery mq)
    {
    return 0;
    }


/*** mqsybFinish - End the current query.
 ***/
int
mqsybFinish(pQueryElement qe, pMultiQuery mq)
    {
    return 0;
    }


/*** mqsybRelease - release any private data areas that were allocated by the
 *** Analyze routine.
 ***/
int
mqsybRelease(pQueryElement qe, pMultiQuery mq)
    {
    return 0;
    }


/*** sybdInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
sybdInitialize()
    {
    pObjDriver drv;
#if 00
    pQueryDriver qdrv;
#endif

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&SYBD_INF,0,sizeof(SYBD_INF));
	xhInit(&SYBD_INF.DBNodes,255,0);
	xaInit(&SYBD_INF.DBNodeList,127);
	SYBD_INF.AccessCnt = 1;
	xsInit(&SYBD_INF.LastSQL);
	xsCopy(&SYBD_INF.LastSQL, "", -1);

	/** Setup the structure **/
	strcpy(drv->Name,"SYBD - Sybase Database Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY | OBJDRV_C_TRANS;
	/*drv->Capabilities = OBJDRV_C_TRANS;*/
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/sybase");

	/** Setup the function references. **/
	drv->Open = sybdOpen;
	drv->Close = sybdClose;
	drv->Create = sybdCreate;
	drv->Delete = sybdDelete;
	drv->OpenQuery = sybdOpenQuery;
	drv->QueryDelete = sybdQueryDelete;
	drv->QueryFetch = sybdQueryFetch;
	drv->QueryClose = sybdQueryClose;
	drv->Read = sybdRead;
	drv->Write = sybdWrite;
	drv->GetAttrType = sybdGetAttrType;
	drv->GetAttrValue = sybdGetAttrValue;
	drv->GetFirstAttr = sybdGetFirstAttr;
	drv->GetNextAttr = sybdGetNextAttr;
	drv->SetAttrValue = sybdSetAttrValue;
	drv->AddAttr = sybdAddAttr;
	drv->OpenAttr = sybdOpenAttr;
	drv->GetFirstMethod = sybdGetFirstMethod;
	drv->GetNextMethod = sybdGetNextMethod;
	drv->ExecuteMethod = sybdExecuteMethod;

	/** Initialize CT Library **/
	if (cs_ctx_alloc(CS_VERSION_100, &SYBD_INF.Context) != CS_SUCCEED) return -1;
	if (ct_init(SYBD_INF.Context, CS_VERSION_100) != CS_SUCCEED) return -1;

	nmRegister(sizeof(SybdTableInf),"SybdTableInf");
	nmRegister(sizeof(SybdData),"SybdData");
	nmRegister(sizeof(SybdQuery),"SybdQuery");
	nmRegister(sizeof(SybdConn),"SybdConn");
	nmRegister(sizeof(SybdNode),"SybdNode");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	SYBD_INF.ObjDriver = drv;

	/** Register with the multiquery layer for SQL passthrough **/
#if 00
	qdrv = (pQueryDriver)nmMalloc(sizeof(QueryDriver));
	if (!qdrv) return -1;
	memset(qdrv,0,sizeof(QueryDriver));
	strcpy(qdrv->Name,"MQSYB - MultiQuery Passthrough SQL driver for Sybase");
	qdrv->Precedence = 1000;
	qdrv->Analyze = mqsybAnalyze;
	qdrv->Start = mqsybStart;
	qdrv->NextItem = mqsybNextItem;
	qdrv->Finish = mqsybFinish;
	qdrv->Release = mqsybRelease;
	if (mqRegisterQueryDriver(qdrv) < 0) return -1;
	SYBD_INF.QueryDriver = qdrv;
#endif

    return 0;
    }


#ifndef HAS_IOPUTC
/*** We had to include this to make the sybase .so libraries work.  The define
 *** came from libio.h.  Sybase library wanted it as a function not a define.
 *** Sigh.
 ***/
#undef _IO_putc
int _IO_putc(char _ch, FILE* _fp)
    {
    return (((_fp)->_IO_write_ptr >= (_fp)->_IO_write_end) \
        ? __overflow(_fp, (unsigned char)(_ch)) \
        : (unsigned char)(*(_fp)->_IO_write_ptr++ = (_ch)));
    }
#endif
