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
#include <ctype.h>
#include <ctpublic.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/xhashqueue.h"
#include "multiquery.h"
#include "cxlib/magic.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_CTYPE_B 1
#endif
#include "centrallix.h"
#include "hints.h"
#include "cxlib/qprintf.h"
#include "cxlib/strtcpy.h"

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



#ifndef HAVE_CTYPE_B
/*** AARRRRRGGGHH!!  Silly glibc guys went and changed the interface again,
 *** breaking the sybase libraries.  Here is our workaround.
 ***/
const unsigned short int* __ctype_b;
#endif

/*** Module Controls ***/
#define SYBD_USE_CURSORS	1	/* use cursors for all multirow SELECTs */
#define SYBD_CURSOR_ROWCOUNT	50	/* # of rows to fetch at a time */
#define SYBD_SHOW_SQL		1	/* debug printout SQL issued to Sybase */
#define SYBD_RESULTSET_CACHE	64	/* number of rows to hold in cache */
#define SYBD_RESULTSET_PERTBL	48	/* max rows to cache per table */

/*** compiled in limit - max total syb connections per CX server ***/
#define SYBD_MAX_CONNECTIONS	1024	/* per CX instance, not per db node! */


/*** This is a hack.  Couldn't get -D__USE_GNU to work.  ***/
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0400000
#endif


/*** Structure for foreign key information ***/
typedef struct
    {
    int		nKeys;
    char	ForeignKeys[16];
    char	PrimaryKeys[16];
    char	PrimaryKeyTable[32];
    }
    SybdRef, *pSybdRef;


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
    unsigned char ColLengths[256];
    int		nCols;
    char*	Keys[8];
    int		KeyCols[8];
    int		nKeys;
    pSybdRef	ColFK[256];
    pParamObjects ObjList;
    pExpression	RowAnnotExpr;
    char	Annotation[256];
    int		HasContent;
    int		RowCount;		/* -1 if unknown */
    pSemaphore	AutonameSem;		/* only one autoname insert at a time */
    }
    SybdTableInf, *pSybdTableInf;

#define SYBD_CF_ALLOWNULL	1
#define SYBD_CF_FOUND		2
#define SYBD_CF_PRIKEY		4
#define SYBD_CF_FOKEY		8	/* column is a foreign key referencing another table */


#define SYBD_MAX_NUM_TYPES	81
#define SYBD_MAX_TYPE_LEN	31
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
    char	Types[SYBD_MAX_NUM_TYPES][SYBD_MAX_TYPE_LEN];
    XHashTable	TypeNameToID;					/** for figuring out id from name **/
    pObjPresentationHints	TypeHints[SYBD_MAX_NUM_TYPES];		/** for storing hint info about each type **/
    int		Version;
    int		Flags;
    char	Username[32];		/* username to log into db server */
    char	Password[32];		/* password to log into db server */
    char	DefaultPassword[32];	/* default password for uninitialized user */
    }
    SybdNode, *pSybdNode;

#define SYBD_NODE_F_USECXAUTH	1	/* use Centrallix usernames/passwords */
#define SYBD_NODE_F_SETCXAUTH	2	/* try to change empty passwords to Centrallix login passwords */

/*** Structure used by this driver to manage connections to the db ***/
typedef struct
    {
    /*int		SessionID;*/
    CS_CONNECTION* CsConn;
    char	Username[32];
    char	Password[32];
    int		Busy;
    int		SPID;			/* sybase server-side process ID */
    }
    SybdConn, *pSybdConn;

/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    pSybdConn	SessionID;
    pSybdConn	ReadSessID;
    pSybdConn	WriteSessID;
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
    char	Autoname[256];
    }
    SybdData, *pSybdData;

#define SYBD_F_ROWPRESENT	1
#define SYBD_F_TFRRW		2
#define	SYBD_F_METAONLY		4	/* user opened the '?' object */

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
    pSybdConn	SessionID;
    pSybdConn	ObjSession;
    int		RowCnt;
    char	SQLbuf[65536];
    pSybdTableInf TableInf;
    CS_COMMAND*	Cmd;
    int		RowsSinceFetch;
    int		Flags;
    }
    SybdQuery, *pSybdQuery;

#define SYBD_QF_USECURSOR	1
#define	SYBD_QF_CURSOROPEN	2


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
    pFile		SqlLog;
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

/*** sybd_internal_AttrType() - return the Centrallix attribute type for
 *** a given column id in the given table.
 ***/
int
sybd_internal_AttrType(pSybdTableInf tdata, int colid)
    {
	if (tdata->ColTypes[colid] == 5 || tdata->ColTypes[colid] == 6 ||
	    tdata->ColTypes[colid] == 7 || tdata->ColTypes[colid] == 16)
	    return DATA_T_INTEGER;
	else if (tdata->ColTypes[colid] == 1 || tdata->ColTypes[colid] == 2 ||
	    tdata->ColTypes[colid] == 19 || tdata->ColTypes[colid] == 18)
	    return DATA_T_STRING;
	else if (tdata->ColTypes[colid] == 22 || tdata->ColTypes[colid] == 12)
	    return DATA_T_DATETIME;
	else if (tdata->ColTypes[colid] == 11 || tdata->ColTypes[colid] == 21)
	    return DATA_T_MONEY;
	else if (tdata->ColTypes[colid] == 8 || tdata->ColTypes[colid] == 23)
	    return DATA_T_DOUBLE;
	else
	    {
	    mssError(1, "SYBD", "Encountered type %d, which cannot be represented internally.", tdata->ColTypes[colid]);
	    return -1;
	    }
    }

/*** sybd_internal_Exec - executes a query and returns the CTlib command
 *** structure.
 ***/
CS_COMMAND*
sybd_internal_Exec(pSybdConn s, char* cmdtext)
    {
    CS_COMMAND* cmd = NULL;
    int rval;

    	/** Alloc the cmd **/
	if ((rval = ct_cmd_alloc(s->CsConn, &cmd)) != CS_SUCCEED)
	    {
	    mssError(1,"SYBD","ct_cmd_alloc() failed: %d", rval);
	    return NULL;
	    }
	
	/** Setup the cmd text **/
	if ((rval = ct_command(cmd, CS_LANG_CMD, cmdtext, CS_NULLTERM, CS_UNUSED)) != CS_SUCCEED)
	    {
	    ct_cmd_drop(cmd);
	    mssError(1,"SYBD","ct_cmd_alloc() failed: %d", rval);
	    return NULL;
	    }

	/** Send it to the server. **/
	thLock();
	if (SYBD_INF.SqlLog) fdPrintf(SYBD_INF.SqlLog, "EXEC (time=%ld spid=%d conn=%8.8x cmd=%8.8x): %s\n", mtRealTicks(), s->SPID, s->CsConn, cmd, cmdtext);
	if ((rval = ct_send(cmd)) != CS_SUCCEED)
	    {
	    thUnlock();
	    ct_cmd_drop(cmd);
	    mssError(1,"SYBD","Could not send SQL command to server: %d", rval);
	    return NULL;
	    }
	thUnlock();

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

	thLock();
	if (SYBD_INF.SqlLog) fdPrintf(SYBD_INF.SqlLog, "CLOSE (time=%ld cmd=%8.8x)\n", mtRealTicks(), cmd);
	thUnlock();

    return 0;
    }


/*** sybd_internal_GetError() - gets the error message that the server
 *** or client library returned for a given connection.
 ***/
int
sybd_internal_GetError(CS_CONNECTION* s, int* errcode, char* errtxt, int maxlen)
    {
    CS_RETCODE rval;
    CS_SERVERMSG smsg;
    CS_CLIENTMSG cmsg;
    

    return 0;
    }


/*** sybd_internal_GetConn - obtains a database connection within a 
 *** given database node.
 ***/
pSybdConn
sybd_internal_GetConn(pSybdNode db_node)
    {
    int i,found_one,rval;
    pSybdConn conn;
    CS_COMMAND* cmd;
    CS_INT restype = 0;
    char sbuf[64];
    char* user;
    char* pwd;

	/** Use system auth? **/
	if (db_node->Flags & SYBD_NODE_F_USECXAUTH)
	    {
	    /** Do we have permission to do this? **/
	    if (!(CxGlobals.Flags & CX_F_ENABLEREMOTEPW))
		{
		mssError(1,"SYBD","use_system_auth requested, but Centrallix global enable_send_credentials is turned off");
		return NULL;
		}

	    /** Get username/password from session **/
	    user = mssUserName();
	    pwd = mssPassword();

	    if(!user || !pwd)
		return NULL;
	    }
	else
	    {
	    /** Use username/password from node **/
	    user = db_node->Username;
	    pwd = db_node->Password;
	    }

	/** User/pass out of range? **/
	if (strlen(user) > 31 || strlen(pwd) > 31)
	    {
	    mssError(1,"SYBD","Username or password invalid");
	    return NULL;
	    }

    	/** Scan to see if we already have a useable connection **/
	for(i=0;i<db_node->Conns.nItems;i++)
	    {
	    conn = (pSybdConn)(db_node->Conns.Items[i]);
	    if (conn->Busy == 0 && !strcmp(user,conn->Username) && !strcmp(pwd,conn->Password))
	        {
		conn->Busy = 1;
		return conn;
		}
	    }

	/** Didn't find one?  First, make sure we aren't exceeding the limit **/
	if (db_node->Conns.nItems >= db_node->MaxConn)
	    {
	    /** Look for one to remove. **/
	    for(found_one=i=0;i<db_node->Conns.nItems;i++)
	        {
	        conn = (pSybdConn)(db_node->Conns.Items[i]);
		if (conn->Busy == 0 && conn->CsConn != NULL)
		    {
		    xaRemoveItem(&(db_node->Conns),xaFindItem(&(db_node->Conns),(void*)conn));
		    ct_close(conn->CsConn, CS_FORCE_CLOSE);
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
	    if (!conn)
	        {
		mssError(0,"SYBD","Could not allocate new connection structure");
		return NULL;
		}
	    conn->CsConn = NULL;
	    }

	/** Make the new connection using the conn structure. **/
	if (!conn->CsConn)
	    {
	    ct_con_alloc(SYBD_INF.Context, &(conn->CsConn));
	    }
	if (conn->CsConn == NULL)
	    {
	    mssError(0,"SYBD","Could not alloc new database connection");
	    nmFree(conn,sizeof(SybdConn));
	    return NULL;
	    }
	ct_con_props(conn->CsConn, CS_SET, CS_USERNAME, user, CS_NULLTERM, NULL);
	ct_con_props(conn->CsConn, CS_SET, CS_PASSWORD, pwd, CS_NULLTERM, NULL);
	ct_con_props(conn->CsConn, CS_SET, CS_APPNAME, "Centrallix", CS_NULLTERM, NULL);
	gethostname(sbuf,63);
	sbuf[63]=0;
	ct_con_props(conn->CsConn, CS_SET, CS_HOSTNAME, sbuf, CS_NULLTERM, NULL);
#if 00 /* locking */
	ct_diag(conn->CsConn, CS_INIT, CS_UNUSED, CS_UNUSED, NULL);
	i = 24;
	ct_diag(conn->CsConn, CS_MSGLIMIT, CS_ALLMSG_TYPE, CS_UNUSED, &i);
#endif
	if (ct_connect(conn->CsConn, db_node->Server, CS_NULLTERM) != CS_SUCCEED)
	    {
	    /** attempt to connect with default password, and then set password **/
	    ct_con_props(conn->CsConn, CS_SET, CS_PASSWORD, db_node->DefaultPassword, CS_NULLTERM, NULL);
	    if (ct_connect(conn->CsConn, db_node->Server, CS_NULLTERM) != CS_SUCCEED)
		{
		mssError(0,"SYBD","Could not connect to database!");
		ct_con_drop(conn->CsConn);
		nmFree(conn,sizeof(SybdConn));
		return NULL;
		}

	    /** Set password? **/
	    if ((db_node->Flags & SYBD_NODE_F_USECXAUTH) && (db_node->Flags & SYBD_NODE_F_SETCXAUTH))
		{
		if (strchr(pwd, '"') || strchr(db_node->DefaultPassword, '"'))
		    {
		    mssError(1,"SYBD","Warning: could not update password for user '%s': password contains invalid character(s).", user);
		    ct_close(conn->CsConn, CS_FORCE_CLOSE);
		    ct_con_drop(conn->CsConn);
		    nmFree(conn,sizeof(SybdConn));
		    return NULL;
		    }
		snprintf(sbuf,sizeof(sbuf),"sp_password \"%s\", \"%s\"", db_node->DefaultPassword, pwd);
		cmd = sybd_internal_Exec(conn, sbuf);
		while((rval=ct_results(cmd, (CS_INT*)&restype)))
		    {
		    if (rval == CS_FAIL)
			{
			mssError(0,"SYBD","Warning: could not change default password for user '%s'", user);
			break;
			}
		    if (rval == CS_END_RESULTS || restype == CS_CMD_DONE) break;
		    }
		sybd_internal_Close(cmd);
		}
	    }

#if 00 /* locking */
	/** Set lock wait to 1 second so we can retry on our side and avoid deadlocks **/
	cmd = sybd_internal_Exec(conn, "set lock wait 1");
	while((rval=ct_results(cmd, (CS_INT*)&restype)))
	    {
	    if (rval == CS_FAIL)
		{
		mssError(1,"SYBD","Warning: could not set lock wait timer; deadlock protection will be disabled", db_node->Database);
		break;
		}
	    if (rval == CS_END_RESULTS || restype == CS_CMD_DONE)
		break;
	    }
	sybd_internal_Close(cmd);
#endif

	/** Do a USE DATABASE only if database was specified in the node. **/
	if (db_node->Database[0])
	    {
	    if (strpbrk(db_node->Database," \t\r\n"))
		{
		mssError(1,"SYBD","Invalid database name '%s'",db_node->Database);
		ct_close(conn->CsConn, CS_FORCE_CLOSE);
		ct_con_drop(conn->CsConn);
		nmFree(conn,sizeof(SybdConn));
		return NULL;
		}
	    snprintf(sbuf,64,"use %s",db_node->Database);
	    cmd = sybd_internal_Exec(conn, sbuf);
	    while((rval=ct_results(cmd, (CS_INT*)&restype)))
	        {
	        if (rval == CS_FAIL)
	            {
		    mssError(0,"SYBD","Could not 'use' database '%s'!", db_node->Database);
		    sybd_internal_Close(cmd);
		    ct_close(conn->CsConn, CS_FORCE_CLOSE);
		    ct_con_drop(conn->CsConn);
		    nmFree(conn,sizeof(SybdConn));
		    return NULL;
		    }
	        if (rval == CS_END_RESULTS || restype == CS_CMD_DONE) break;
	        }
	    sybd_internal_Close(cmd);
	    }

	/** Get the spid **/
	conn->SPID = 0;
	if ((cmd = sybd_internal_Exec(conn, "select convert(integer,@@spid)")) != NULL)
	    {
	    while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	      while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	        {
		ct_get_data(cmd, 1, &conn->SPID, sizeof(int), (CS_INT*)&i);
		}
	    sybd_internal_Close(cmd);
	    }

	strcpy(conn->Username, user);
	strcpy(conn->Password, pwd);
	conn->Busy = 1;
	xaAddItem(&(db_node->Conns),(void*)conn);

    return conn;
    }


/*** sybd_internal_ReleaseConn - release a connection back to the connection
 *** pool for this database node.
 ***/
int
sybd_internal_ReleaseConn(pSybdNode db_node, pSybdConn conn)
    {
    int i;

	if (conn->Busy)
	    {
	    conn->Busy = 0;
	    return 0;
	    }

	mssError(1,"SYBD","Bark! Critical internal error - releasing released connection!");

    return -1;
    }


/*** sybd_internal_GetCxType - convert a sybase usertype to a centrallix datatype.
 *** Like GetDefaultHints, it might not be a bad idea to push this to a file somehow,
 *** to make the handling of types more flexable (and allow for user-defined types)
 ***/
int
sybd_internal_GetCxType(int ut)
    {
	if (ut == 1 || ut == 2 || ut == 18 || ut == 19 || ut == 25) return DATA_T_STRING;
	if (ut == 5 || ut == 6 || ut == 7 || ut == 16) return DATA_T_INTEGER;
	if (ut == 12 || ut == 22) return DATA_T_DATETIME;
	if (ut == 8 || ut == 23) return DATA_T_DOUBLE;
	if (ut == 11 || ut == 21) return DATA_T_MONEY;

	if (!CxGlobals.QuietInit)
	    mssError(1, "SYBD", "the usertype %d is not supported by Centrallix");
	return -1;
    }

/*** sybd_internal_GetCxValue - convert a sybase value of some type to its
 *** corresponding centrallix representation. Assumes val points to valid memory.
 ***/
int
sybd_internal_GetCxValue(void* ptr, int ut, pObjData val, int datatype)
    {
    int i,minus,n;
    unsigned int msl,lsl,divtmp;
    unsigned int days,fsec;
    float f;

	/** Make sure the passed in data type matches what we'll end up with **/
	if (sybd_internal_GetCxType(ut) != datatype)
	    {
	    mssError(1, "SYBD", "Mismatched data types. Got %d, should've been %s", 
		datatype, sybd_internal_GetCxType(ut));
	    return -1;
	    }

	if (ptr == NULL) return 1;
	if (ut==5 || ut==6 || ut==7 || ut==16)
	    {
	    val->Integer = 0;
	    if (ut==5 || ut==16) memcpy(val,ptr,1);
	    else if (ut==6) memcpy(val,ptr,2);
	    else memcpy(val,ptr,4);
	    return 0;
	    }
	else if (ut==1 || ut==2 || ut==18 || ut==19)
	    {
	    val->String = ptr;
	    return 0;
	    }
	else if (ut==22 || ut==12)
	    {
	    val->DateTime->Value = 0;

	    /** datetime **/
	    memcpy(&days,ptr,4);
	    memcpy(&fsec,ptr+4,4);

	    /** Convert the time **/
	    fsec /= 300;
	    val->DateTime->Part.Hour = fsec/3600;
	    fsec -= (val->DateTime->Part.Hour*3600);
	    val->DateTime->Part.Minute = fsec/60;
	    fsec -= (val->DateTime->Part.Minute*60);
	    val->DateTime->Part.Second = fsec;

	    /** Date out of range? (max year value = 4096) **/
	    if (days > 4096*365)
		{
		mssError(1, "SYBD", "Date/time out of range (n_days = %d)", days);
		return -1;
		}

	    /** Convert the date **/
	    if (days > 364) days++; /* hack to cover 1900 not being leap year */
	    days = days*4;
	    val->DateTime->Part.Year = days / (365*4 + 1);
	    days -= (val->DateTime->Part.Year * (365*4 + 1));
	    days = (days)/4;
	    val->DateTime->Part.Month = 0;
	    for(n=0;n<12;n++) 
		{
		if (days >= (obj_month_days[n] + ((n==1 && IS_LEAP_YEAR(val->DateTime->Part.Year+1900))?1:0)))
		    {
		    val->DateTime->Part.Month++;
		    days -= (obj_month_days[n] + ((n==1 && IS_LEAP_YEAR(val->DateTime->Part.Year+1900))?1:0));
		    }
		else
		    {
		    break;
		    }
		}
	    val->DateTime->Part.Day = days;
	    return 0;
	    }
	else if (ut == 8)
	    {
	    /** float **/
	    memcpy(val, ptr, 8);
	    return 0;
	    }
	else if (ut == 23)
	    {
	    memcpy(&f, ptr, 4);
	    val->Double = f;
	    return 0;
	    }
	else if (ut == 11 || ut == 21)
	    {
	    /** money **/
	    if (ut == 21)
		{
		/** smallmoney, 4-byte **/
		memcpy(&i, ptr, 4);
		val->Money->WholePart = i/10000;
		if (i < 0 && (i%10000) != 0) val->Money->WholePart--;
		val->Money->FractionPart = i - (val->Money->WholePart*10000);
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
		val->Money->WholePart = n;
		val->Money->FractionPart = msl;
		if (minus)
		    {
		    val->Money->WholePart = -val->Money->WholePart;
		    if (val->Money->FractionPart > 0)
			{
			val->Money->WholePart--;
			val->Money->FractionPart = 10000 - val->Money->FractionPart;
			}
		    }
		return 0;
		}
	    }

	mssError(1, "SYBD", "usertype %d not supported by Centrallix", ut);
	return -1;
    }

/*** sybd_internal_GetDefaultHints - returns a default hints structure for the given datatype.
 *** these are hard-coded right now, but there's no reason they couldn't be pushed out to
 *** a file to make them easier to change, should the sybase types change
 ***/
pObjPresentationHints
sybd_internal_GetDefaultHints(int usertype)
    {
    pObjPresentationHints hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
    pParamObjects tmplist;

	memset(hints, 0, sizeof(ObjPresentationHints));
	xaInit(&(hints->EnumList), 8);
	hints->GroupID = -1;
	hints->VisualLength2 = 1;
	
	tmplist = expCreateParamList();
	expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
	switch (usertype)
	    {
	    case 1: /** char **/
		break;
	    case 2: /** varchar **/
		break;
//	    case 3: /** binary **/
//		break;
//	    case 4: /** varbinary **/
//		break;
	    case 5: /** tinyint **/
		hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("255", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
	    case 6: /** smallint **/
		hints->MinValue = expCompileExpression("-32768", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("32767", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
	    case 7: /** int **/
		hints->MinValue = expCompileExpression("-2147438648", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MaxValue = expCompileExpression("2147438647", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
	    case 8: /** float **/
		break;
//	    case 10: /** numeric **/
//		break;
	    case 11: /** money **/
		hints->Format=nmSysStrdup("money");
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
	    case 12: /** datetime **/
		hints->Format=nmSysStrdup("datetime");
		break;
//	    case 13: /** intn **/
//		break;
//	    case 14: /** floatn **/
//		break;
//	    case 15: /** datetimn **/
//		break;
	    case 16: /** bit **/
		hints->Style |= OBJ_PH_STYLE_NOTNULL;
		hints->StyleMask |= OBJ_PH_STYLE_NOTNULL;
		hints->MaxValue = expCompileExpression("1", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
//	    case 17: /** moneyn **/
//		hints->Format="money";
	    case 18: /** sysname (this is pretty useless since you can't use this directly as a column's type) **/
		hints->Style |= OBJ_PH_STYLE_NOTNULL;
		hints->StyleMask |= OBJ_PH_STYLE_NOTNULL;
		hints->AllowChars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQQRSTUVWXYZ0123456789_$#@";
		break;
		/** constriaint expression to force alphabetic/_ as first char? **/
	    case 19: /** text **/
		break;
	    case 20: /** image **/
		break;
	    case 21: /** smallmoney **/
		hints->Format=nmSysStrdup("money");
		break;
	    case 22: /** smalldatetime **/
		hints->Format=nmSysStrdup("datetime");
		break;
	    case 23: /** real **/
		hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		break;
//	    case 24: /** nchar **/
//		break;
//	    case 25: /** nvarchar **/
//		break;
//	    case 26: /** decimal **/
//		break;
//	    case 27: /** decimaln **/
//		break;
//	    case 28: /** numericn **/
//		break;
//	    case 80: /** timestamp **/
//		break;
	    default:
		if (!CxGlobals.QuietInit)
		    mssError(1, "SYBD", "%d not a supported type", usertype);
		xaDeInit(&(hints->EnumList));
		nmFree(hints, sizeof(ObjPresentationHints));
		expFreeParamList(tmplist);
		return NULL;
	    }
	expFreeParamList(tmplist);
	return hints;
    }


/*** sybd_internal_MergTypeHints - look for hints that are relevent for the given type,
 *** and add them into the existing hints structure, over-riding where necessary.
 ***/
int
sybd_internal_MergeTypeHints(pStructInf type_inf, pObjPresentationHints hints, int ut)
    {
    pParamObjects tmplist;
    pExpression exp;
    char* ptr;
    int t, i;

	t = sybd_internal_GetCxType(ut);
	
	/** check for expressions **/
	if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE)
	    {
	    tmplist = expCreateParamList();
	    expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);

	    stAttrValue(stLookup(type_inf, "constraint"),NULL,&ptr,0);
	    if (ptr) 
		{
		if ( (exp = expCompileExpression(ptr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0)) == NULL)
		    mssError(0, "SYBD", "Error in constraint expression %s", ptr);
		else 
		    {
		    if (hints->Constraint) expFreeExpression(hints->Constraint);
		    hints->Constraint = exp;
		    }
		}
	    stAttrValue(stLookup(type_inf, "default"),NULL,&ptr,0);
	    if (ptr) 
		{
		if ( (exp = expCompileExpression(ptr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0)) == NULL)
		    mssError(0, "SYBD", "Error in default expression %s", ptr);
		else 
		    {
		    if (hints->DefaultExpr) expFreeExpression(hints->DefaultExpr);
		    hints->DefaultExpr = exp;
		    }
		}
	    stAttrValue(stLookup(type_inf, "min"),NULL,&ptr,0);
	    if (ptr) 
		{
		if ( (exp = expCompileExpression(ptr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0)) == NULL)
		    mssError(0, "SYBD", "Error in min expression %s", ptr);
		else 
		    {
		    if (hints->MinValue) expFreeExpression(hints->MinValue);
		    hints->MinValue = exp;
		    }
		}
	    stAttrValue(stLookup(type_inf, "max"),NULL,&ptr,0);
	    if (ptr) 
		{
		if ( (exp = expCompileExpression(ptr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0)) == NULL)
		    mssError(0, "SYBD", "Error in max expression %s", ptr);
		else 
		    {
		    if (hints->MaxValue) expFreeExpression(hints->MaxValue);
		    hints->MaxValue = exp;
		    }
		}

	    expFreeParamList(tmplist);
	    }
	
	/** check for allowed/disallowed characters **/
	if (t == DATA_T_STRING)
	    {
	    if (stAttrValue(stLookup(type_inf, "allowchars"),NULL,&ptr,0) >= 0)
		{
		if (hints->AllowChars) free(hints->AllowChars);
		hints->AllowChars = strdup(ptr);
		}
	    if (stAttrValue(stLookup(type_inf, "badchars"),NULL,&ptr,0) >= 0)
		{
		if (hints->BadChars) free(hints->BadChars);
		hints->BadChars = strdup(ptr);
		}
	    if (!((hints->Style & OBJ_PH_STYLE_LOWERCASE) || (hints->Style & OBJ_PH_STYLE_UPPERCASE)))
		{
		if (stAttrValue(stLookup(type_inf, "lowercase"),NULL,&ptr,0) >= 0 && !strcmp(ptr, "yes"))
		    {
		    hints->StyleMask |= OBJ_PH_STYLE_LOWERCASE;
		    hints->Style |= OBJ_PH_STYLE_LOWERCASE;
		    }
		else if (stAttrValue(stLookup(type_inf, "uppercase"),NULL,&ptr,0) >= 0 && !strcmp(ptr, "yes"))
		    {
		    hints->StyleMask |= OBJ_PH_STYLE_UPPERCASE;
		    hints->Style |= OBJ_PH_STYLE_UPPERCASE;
		    }
		}
	    }
	
	/** check for visual length **/
	if (stAttrValue(stLookup(type_inf, "length"),&i,NULL,0) >= 0) hints->VisualLength = i;
	if (stAttrValue(stLookup(type_inf, "height"),&i,NULL,0) >= 0) hints->VisualLength2 = i;

	/** check for Style **/

    return 0;
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
    int* TypeNum;
    pSybdConn conn;
    CS_COMMAND* cmd;
    CS_INT restype = 0;
    char* ptr,* TypeName;
    pSnNode snnode;
    pObjPresentationHints hints;
    unsigned char is_variable, length;
    pStructInf type_hints, type_inf;
    pParamObjects tmplist;
    char ver_buf[255];

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
	memset(db_node,0,sizeof(SybdNode));
	db_node->SnNode = snnode;
	strcpy(db_node->Path,path);
	if (stAttrValue(stLookup(snnode->Data,"server"),NULL,&ptr,0) < 0) ptr = NULL;
	strtcpy(db_node->Server,ptr?ptr:"",sizeof(db_node->Server));
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
	if (stAttrValue(stLookup(snnode->Data,"use_system_auth"),NULL,&ptr,0) == 0)
	    {
	    if (!strcasecmp(ptr,"yes"))
		db_node->Flags |= SYBD_NODE_F_USECXAUTH;
	    }
	if (stAttrValue(stLookup(snnode->Data,"set_passwords"),NULL,&ptr,0) == 0)
	    {
	    if (!strcasecmp(ptr,"yes"))
		db_node->Flags |= SYBD_NODE_F_SETCXAUTH;
	    }
	if (stAttrValue(stLookup(snnode->Data,"username"),NULL,&ptr,0) < 0) ptr = "cxguest";
	strtcpy(db_node->Username,ptr,sizeof(db_node->Username));
	if (stAttrValue(stLookup(snnode->Data,"password"),NULL,&ptr,0) < 0) ptr = "";
	strtcpy(db_node->Password,ptr,sizeof(db_node->Password));
	if (stAttrValue(stLookup(snnode->Data,"default_password"),NULL,&ptr,0) < 0) ptr = "";
	strtcpy(db_node->DefaultPassword,ptr,sizeof(db_node->DefaultPassword));

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

	xhInit(&(db_node->TypeNameToID), 31, 0);
	memset(db_node->TypeHints, 0, sizeof(void*)*SYBD_MAX_NUM_TYPES);

	/** find the typehints part of the node, if there is one **/
	type_hints = stLookup(snnode->Data, "typehints");

	/** Get a connection and get the version and types list. **/
	conn = sybd_internal_GetConn(db_node);
	if (conn)
	    {
	    if ((cmd=sybd_internal_Exec(conn, "select @@version")))
		{
		while(ct_results(cmd, &restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			ct_get_data(cmd, 1, ver_buf, sizeof(ver_buf)-1, (CS_INT*)&i);
			if (i >= 0 && i < sizeof(ver_buf))
			    {
			    ver_buf[i] = '\0';
			    ptr = strchr(ver_buf, '/');
			    if (ptr && ptr[1])
				{
				db_node->Version = strtoul(ptr+1, NULL, 10);
				}
			    }
			}
		    }
		sybd_internal_Close(cmd);
		}
	    if ((cmd=sybd_internal_Exec(conn, "select usertype,name,length,variable from systypes")))
	        {
		while(ct_results(cmd, &restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			TypeNum = (int*)malloc(4);
			*TypeNum=0;
			ct_get_data(cmd, 1, TypeNum, 4, (CS_INT*)&i);
			if (i<=0) 
			    { free(TypeNum); continue; }
			if (*TypeNum < 0 || *TypeNum >= SYBD_MAX_NUM_TYPES) 
			    { free(TypeNum); continue; }
			TypeName = (char*)malloc(SYBD_MAX_TYPE_LEN);
			ct_get_data(cmd, 2, TypeName, SYBD_MAX_TYPE_LEN-1, (CS_INT*)&i);
			if (i <= 0 || i > SYBD_MAX_TYPE_LEN-1)
			    { free(TypeName); free(TypeNum); continue; }
			TypeName[i]='\0';
			memcpy(db_node->Types[*TypeNum], TypeName, strlen(TypeName)+1);
			length = is_variable = 0;
			ct_get_data(cmd, 3, &length, 1, (CS_INT*)&i);
			ct_get_data(cmd, 4, &is_variable, 1, (CS_INT*)&i);
			
//			fprintf(stderr, "Type name: %s, Type Num: %d, is_variable: %d, length: %d\n", TypeName, *TypeNum,
//			    is_variable, length);
			/** add name->id entry in hash table **/
			xhAdd(&(db_node->TypeNameToID), TypeName, (char*)TypeNum);

			/** create a base hints entry for each type **/
			if ( (hints = sybd_internal_GetDefaultHints(*TypeNum)) != NULL)
			    db_node->TypeHints[*TypeNum] = hints;

			/** supliment that base entry a bit **/
			switch (sybd_internal_GetCxType(*TypeNum))
			    {
			    case DATA_T_INTEGER: hints->VisualLength = 13; break;
			    case DATA_T_DATETIME: hints->VisualLength = 20; break;
			    case DATA_T_MONEY: hints->VisualLength = 16; break;
			    case DATA_T_DOUBLE: hints->VisualLength = 16; break;
			    }

			/** check to see if there's anything pertaining to this type in the node **/
			if ( type_hints && (type_inf = stLookup(type_hints, TypeName)) != NULL)
			    sybd_internal_MergeTypeHints(type_inf, hints, *TypeNum);
			
			}
		    }
		sybd_internal_Close(cmd);
		}
	    sybd_internal_ReleaseConn(db_node, conn);
	    }
	else
	    {
	    mssError(0,"SYBD","Could not get connection to retrieve types list");
	    xhDeInit(&(db_node->TableInf));
	    xaDeInit(&(db_node->Conns));
	    nmFree(db_node, sizeof(SybdNode));
	    return NULL;
	    }

    return db_node;
    }


/*** sybd_internal_GetTableInf - returns a SybdTableInf structure containing 
 *** information about the attributes and primary keys of a given table.  The
 *** information is either obtained from the cache or read from the database.
 ***/
pSybdTableInf
sybd_internal_GetTableInf(pSybdNode node, pSybdConn conn, char* table)
    {
    pSybdTableInf tdata;
    pSybdRef fkeydata;
    char sbuf[480];
    char* ptr;
    char* tmpptr;
    int l,i,col,find_col;
    CS_INT restype = 0;
    int n, t;
    CS_COMMAND* cmd;

	/** Valid table name check. **/
	if (strpbrk(table,"\" \t\r\n\""))
	    {
	    mssError(0,"SYBD","Table name '%s' invalid", table);
	    return NULL;
	    }

    	/** See if this table's metadata is cached. **/
	tdata = (pSybdTableInf)xhLookup(&(node->TableInf),table);
	if (tdata) return tdata;

	/** Allocate a new structure. **/
	tdata = (pSybdTableInf)nmMalloc(sizeof(SybdTableInf));
	if (!tdata) return NULL;
	memset(tdata,0,sizeof(SybdTableInf));
	if (strlen(table) > 31)
	    {
	    mssError(1,"SYBD","Table name too long");
	    nmFree(tdata, sizeof(SybdTableInf));
	    return NULL;
	    }
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
	snprintf(sbuf,sizeof(sbuf),"SELECT c.name,c.colid,c.status,c.usertype,c.length FROM syscolumns c,sysobjects o WHERE c.id=o.id AND o.name='%s' ORDER BY c.colid",table);
	if (!(cmd=sybd_internal_Exec(conn, sbuf)))
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
	    ct_get_data(cmd, 1, sbuf, sizeof(sbuf)-1, (CS_INT*)&i);
	    sbuf[i] = 0;
	    if (strpbrk(sbuf,"'\"\t\r\n "))
		{
		sybd_internal_Close(cmd);
		nmSysFree(tdata->ColBuf);
		nmFree(tdata,sizeof(SybdTableInf));
		mssError(1,"SYBD","Column name %s in table %s is invalid.", sbuf, table);
		return NULL;
		}
	    l = strlen(sbuf)+1;
	    if (tdata->ColBufLen + l >= tdata->ColBufSize)
	        {
		/** Too many column names!  Realloc the buffer... **/
		tmpptr = (char*)nmSysRealloc(tdata->ColBuf, tdata->ColBufSize+1024);
		if (!tmpptr)
		    {
		    sybd_internal_Close(cmd);
		    nmSysFree(tdata->ColBuf);
		    nmFree(tdata,sizeof(SybdTableInf));
		    mssError(1,"SYBD","Realloc() failed while building column name list for table");
		    return NULL;
		    }

		/** Realloc might have moved the block!!! sigh... **/
		if (tmpptr != tdata->ColBuf)
		    {
		    ptr = ptr + (tmpptr - tdata->ColBuf);
		    for(i=0;i<tdata->nCols;i++)
			tdata->Cols[i] = tdata->Cols[i] + (tmpptr - tdata->ColBuf);
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

	    /** Get the column length (MJM) **/
	    tdata->ColLengths[tdata->nCols] = 0;
	    ct_get_data(cmd, 5, &(tdata->ColLengths[tdata->nCols]), 4, (CS_INT*)&i);
	    
	    tdata->nCols++;
	    }
	sybd_internal_Close(cmd);

	/** No columns?  If not, return err because table doesn't exist **/
	if (tdata->nCols == 0)
	    {
	    nmSysFree(tdata->ColBuf);
	    nmFree(tdata,sizeof(SybdTableInf));
	    mssError(1,"SYBD","Nonexistent table '%s'", table);
	    return NULL;
	    }

	/** Ok, done with that query.  Now load the primary key. **/
	snprintf(sbuf,sizeof(sbuf),"SELECT keycnt,key1,key2,key3,key4,key5,key6,key7,key8 FROM syskeys k, sysobjects o where k.id=o.id and o.name='%s' and k.type=1",table);
	if ((cmd=sybd_internal_Exec(conn, sbuf)))
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

	/** No primary keys from sp_primarykey?  Use sysindexes instead **/
	if (tdata->nKeys == 0)
	    {
	    snprintf(sbuf, sizeof(sbuf), "SELECT char_length(convert(varchar(255),keys1)), keys1 FROM sysindexes i, sysobjects o where i.id = o.id and o.name='%s' and (i.status & 2048) = 2048", table);
	    if ((cmd = sybd_internal_Exec(conn, sbuf)))
		{
		while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			n = 0;
			ct_get_data(cmd, 1, &n, 4, (CS_INT*)&i);
			if (n > 0 && n < 255 && (n & 0x0F) == 0)
			    {
			    ct_get_data(cmd, 2, sbuf, sizeof(sbuf)-1, (CS_INT*)&i);
			    if (i == n)
				{
				t = n/16;
				for(l=0;l<t;l++)
				    {
				    if (node->Version >= 15)
					col = sbuf[l*16 + 4];
				    else
					col = sbuf[l*16 + 2];
				    if (col == 0) continue;
				    find_col = -1;
				    for(i=0;i<tdata->nCols;i++) if (tdata->ColIDs[i] == col)
					{
					find_col = i;
					break;
					}
				    if (find_col >= 0)
					{
					tdata->KeyCols[tdata->nKeys] = find_col;
					tdata->Keys[tdata->nKeys] = tdata->Cols[find_col];
					tdata->ColFlags[find_col] |= SYBD_CF_PRIKEY;
					tdata->ColKeys[find_col] = tdata->nKeys;
					tdata->nKeys++;
					}
				    else
					{
					tdata->nKeys = 0;
					break;
					}
				    }
				}
			    }
			}
		    }
		sybd_internal_Close(cmd);
		}
	    }

	/** Next, get annotation information for the table and for its rows. **/
	tdata->Annotation[0] = 0;
	tdata->RowAnnotExpr = NULL;
	if (*(node->AnnotTable))
	    {
	    snprintf(sbuf, sizeof(sbuf), "SELECT a,b,c FROM %s WHERE a = '%s'", node->AnnotTable, table);
	    if ((cmd=sybd_internal_Exec(conn, sbuf)))
	        {
		while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
		    {
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
		        {
			/** Get the table's annotation **/
			ct_get_data(cmd, 2, tdata->Annotation, 255, (CS_INT*)&i);
			tdata->Annotation[i] = 0;

			/** Get the annotation expression and compile it **/
			ct_get_data(cmd, 3, sbuf, sizeof(sbuf)-1, (CS_INT*)&i);
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

	/** Finally, get the foreign key information for the table **/
	snprintf(sbuf, sizeof(sbuf), "select o2.name,keycnt,fokey1,fokey2,fokey3,fokey4,fokey5,fokey6,fokey7,fokey8,fokey9,fokey10,fokey11,fokey12,fokey13,fokey14,fokey15,fokey16,refkey1,refkey2,refkey3,refkey4,refkey5,refkey6,refkey7,refkey8,refkey9,refkey10,refkey11,refkey12,refkey13,refkey14,refkey15,refkey16 from sysobjects o1,sysobjects o2,sysreferences where o1.name='%s' and o1.id=tableid and o2.id=reftabid", table);
	if ((cmd=sybd_internal_Exec(conn, sbuf)))
	    {
	    while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	      while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	        {
		fkeydata = (pSybdRef)nmMalloc(sizeof(SybdRef));
		if (!fkeydata) break;
		fkeydata->nKeys = 0;

		/** get referenced table name **/
		ct_get_data(cmd, 1, fkeydata->PrimaryKeyTable, sizeof(fkeydata->PrimaryKeyTable)-1, (CS_INT*)&i);
		fkeydata->PrimaryKeyTable[i] = 0;

		/** Get column count for key **/
		ct_get_data(cmd, 2, &(fkeydata->nKeys), 4, (CS_INT*)&i);
		if (fkeydata->nKeys < 1 || fkeydata->nKeys > 16)
		    {
		    nmFree(fkeydata, sizeof(SybdRef));
		    break;
		    }

		/** Get individual keys **/
		for(l=0;l<fkeydata->nKeys;l++)
		    {
		    col = 0;
		    ct_get_data(cmd, 3+l, &col, sizeof(int), (CS_INT*)&i);

		    /** dereference foreign key column id's since we have the tdata with us **/
		    find_col = -1;
		    for(i=0;i<tdata->nCols;i++) if (tdata->ColIDs[i] == col) 
		        {
			find_col=i;
			break;
			}
		    if (find_col < 0) continue;
		    tdata->ColFK[find_col] = fkeydata;
		    tdata->ColFlags[find_col] |= SYBD_CF_FOKEY;
		    fkeydata->ForeignKeys[l] = find_col;
		    }
		for(l=0;l<fkeydata->nKeys;l++)
		    {
		    /** pri key table column id's are NOT dereferenced!! **/
		    col = 0;
		    ct_get_data(cmd, 3+16+l, &col, sizeof(int), (CS_INT*)&i);
		    fkeydata->PrimaryKeys[l] = col;
		    }
		}
	    sybd_internal_Close(cmd);
	    }

	/** Get the row count for the table **/
	snprintf(sbuf, sizeof(sbuf), "select count(1) from %s", table);
	if ((cmd=sybd_internal_Exec(conn, sbuf)))
	    {
	    while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
	      while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
	        {
		tdata->RowCount = -1;
		ct_get_data(cmd, 1, &tdata->RowCount, sizeof(int), (CS_INT*)&i);
		}
	    sybd_internal_Close(cmd);
	    }

	/** Cache the data **/
	tdata->AutonameSem = syCreateSem(1,0);
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
    static char fbuf[120];
    char* keyptrs[8];
    int i,col=0;
    char* ptr;
    int n_left;
    int n;
    unsigned long long col64;
    ObjData val;
    MoneyType m;
    char* mstr;

    	/** Get pointers to the key data. **/
	ptr = fbuf;
	n_left = 119;
	for(i=0;i<tdata->nKeys;i++)
	    {
	    if (i>0 && n_left > 0) 
		{
		*(ptr++)='|';
		n_left--;
		}
	    col = 0;
	    keyptrs[i] = NULL;
	    if (inf->ColPtrs[tdata->KeyCols[i]])
		{
		switch(tdata->ColTypes[tdata->KeyCols[i]])
		    {
		    case 11: /** 8 byte money **/
			memcpy(&col64, inf->ColPtrs[tdata->KeyCols[i]], 8);
			val.Money = &m;
			if (sybd_internal_GetCxValue(&col64, 11, &val, DATA_T_MONEY) < 0)
			    return NULL;
			mstr = objFormatMoneyTmp(val.Money, "0.0000");
			if (mstr) snprintf(ptr, n_left, "%s", mstr);
			break;
		    case 21: /** 4 byte money **/
			memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 4);
			break;
		    case 22: /** date value **/
		    case 12: /** date value **/
			memcpy(&col64, inf->ColPtrs[tdata->KeyCols[i]], 8);
			snprintf(ptr,n_left,"%8.8llX",col64);
			break;
		    case 7: /** INT **/
			memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 4);
			snprintf(ptr,n_left,"%d",col);
			break;
		    case 6: /** SMALLINT **/
			memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 2);
			snprintf(ptr,n_left, "%d", col);
			break;
		    case 5: /** TINYINT **/
		    case 16: /** BIT **/
			memcpy(&col, inf->ColPtrs[tdata->KeyCols[i]], 1);
			snprintf(ptr,n_left, "%d", col);
			break;
		    case 1: /** CHAR **/
		    case 2: /** VARCHAR **/
		    case 18: /** SYSNAME **/
		    case 19: /** TEXT **/
			n = strlen(inf->ColPtrs[tdata->KeyCols[i]]);
			while(n > 1 && inf->ColPtrs[tdata->KeyCols[i]][n-1] == ' ')
			    n--;
			snprintf(ptr,n_left,"%*.*s", n, n, inf->ColPtrs[tdata->KeyCols[i]]);
			break;
		    }
		}
	    else
		{
		/** NULL value ??? **/
		ptr[0] = '\0';
		}
	    n_left -= strlen(ptr);
	    ptr += strlen(ptr);
	    if (n_left == 0)
		{
		mssError(1,"SYBD","Error - key/object name maximum length exceeded");
		return NULL;
		}
	    }

    return fbuf;
    }


/*** sybd_internal_FilenameToKey - converts a primary key filename to a where
 *** clause directing access for that key, for a given table within a given 
 *** database node.  The returned name is stored in a static storage area and
 *** must be copied from that place before allowing a context switch....
 ***/
char*
sybd_internal_FilenameToKey(pSybdNode node, pSybdConn conn, char* table, char* filename)
    {
    static char wbuf[256];
    static char fbuf[120];
    char* wptr;
    pSybdTableInf key;
    char* ptr;
    int i;
    int is_string;
    char* sptr;
    int t;
    ObjData val;
    MoneyType m;
    DateTime dt;
    unsigned long long i64;
    unsigned int i32;

	/** Lookup the key data **/
	key = sybd_internal_GetTableInf(node, conn, table);
	if (!key) return NULL;

	/** Build the where clause condition **/
	if (strlen(filename) >= sizeof(fbuf))
	    {
	    mssError(1,"SYBD","Filename too long for concat key");
	    return NULL;
	    }
	strtcpy(fbuf,filename, sizeof(fbuf));
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
	    if ((wbuf + 255) - wptr <= 5 + 2 + strlen(key->Keys[i]) + strlen(ptr) + 2 + 2)
		{
		mssError(1,"SYBD","Bark! Internal row selection buffer too small for prikey query");
		return NULL;
		}
	    is_string = 1;
	    t = sybd_internal_GetCxType(key->ColTypes[key->KeyCols[i]]);
	    if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE || t == DATA_T_MONEY) is_string = 0;
	    if (wbuf[0]) 
	        {
		strcpy(wptr," AND ");
		wptr+=5;
		}
	    strcpy(wptr, key->Keys[i]);
	    wptr += strlen(wptr);
	    *(wptr++) = '=';
	    if (is_string)
		{
		sptr = objDataToStringTmp(DATA_T_STRING,ptr,DATA_F_QUOTED | DATA_F_SYBQUOTE);
		if ((wbuf + 255) - wptr <= strlen(sptr) + 1)
		    {
		    mssError(1,"SYBD","Bark! Internal row selection buffer too small for prikey query");
		    return NULL;
		    }
		strcpy(wptr,sptr);
		wptr += strlen(sptr);
		}
	    else if (t == DATA_T_DATETIME)
		{
		/** 8-byte raw value at ptr **/
		i64 = strtoull(ptr, NULL, 16);
		val.DateTime = &dt;
		if (sybd_internal_GetCxValue(&i64, key->ColTypes[key->KeyCols[i]], &val, t) < 0)
		    {
		    mssError(1,"SYBD","Invalid date/time value in object name");
		    return NULL;
		    }
		sptr = objDataToStringTmp(DATA_T_DATETIME,&val,DATA_F_QUOTED | DATA_F_SYBQUOTE);
		if ((wbuf + 255) - wptr <= strlen(sptr) + 1)
		    {
		    mssError(1,"SYBD","Bark! Internal row selection buffer too small for prikey query");
		    return NULL;
		    }
		strcpy(wptr,sptr);
		wptr += strlen(sptr);
		}
	    else if (t == DATA_T_MONEY && (key->ColTypes[key->KeyCols[i]] == 11 || key->ColTypes[key->KeyCols[i]] == 21))
		{
		/** String representation at full precision 0.0000 **/
		val.Money = &m;
		if (objDataToMoney(DATA_T_STRING, ptr, &m) < 0)
		    {
		    mssError(1,"SYBD","Invalid money value in object name");
		    return NULL;
		    }
		sptr = objFormatMoneyTmp(val.Money,"0.0000");

		/** 8-byte raw value at ptr **/
		/*val.Money = &m;
		i64 = strtoull(ptr, NULL, 16);
		if (sybd_internal_GetCxValue(&i64, key->ColTypes[key->KeyCols[i]], &val, t) < 0)
		    {
		    mssError(1,"SYBD","Invalid money value in object name");
		    return NULL;
		    }
		sptr = objFormatMoneyTmp(val.Money,"0.00");*/
		if ((wbuf + 255) - wptr <= strlen(sptr) + 1)
		    {
		    mssError(1,"SYBD","Bark! Internal row selection buffer too small for prikey query");
		    return NULL;
		    }
		strcpy(wptr,sptr);
		wptr += strlen(sptr);
		}
#if 00
	    else if (t == DATA_T_MONEY && key->ColTypes[key->KeyCols[i]] == 21)
		{
		/** 4-byte raw value at ptr **/
		val.Money = &m;
		i32 = strtoul(ptr, NULL, 16);
		if (sybd_internal_GetCxValue(&i32, key->ColTypes[key->KeyCols[i]], &val, t) < 0)
		    {
		    mssError(1,"SYBD","Invalid money value in object name");
		    return NULL;
		    }
		sptr = objFormatMoneyTmp(val.Money,"0.00");
		if ((wbuf + 255) - wptr <= strlen(sptr) + 1)
		    {
		    mssError(1,"SYBD","Bark! Internal row selection buffer too small for prikey query");
		    return NULL;
		    }
		strcpy(wptr,sptr);
		wptr += strlen(sptr);
		}
#endif
	    else
		{
		strcpy(wptr,ptr);
		wptr += strlen(ptr);
		}
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


/*** sybd_internal_TreeToClauseConstant - convert a constant value to the
 *** needed format for the WHERE or ORDER clause for the Sybase server.  This
 *** also checks for needed data type conversions and issues the appropriate
 *** convert() function to Sybase.
 ***/
int
sybd_internal_TreeToClauseConstant(pExpression tree, int data_type, pSybdTableInf *tdata, int n_tdata, pXString clause)
    {
    pExpression exp;
    int colid, t;
    char* ptr;
    ObjData od;

	/** Handle null values **/
	if (tree->Flags & EXPR_F_NULL)
	    {
	    xsConcatenate(clause, " NULL ", 6);
	    return 0;
	    }

	/** Derive data type from node type? **/
	if (data_type == -1)
	    {
	    switch(tree->NodeType)
		{
		case EXPR_N_DATETIME: data_type=DATA_T_DATETIME; break;
		case EXPR_N_MONEY: data_type=DATA_T_MONEY; break;
		case EXPR_N_STRING: data_type=DATA_T_STRING; break;
		case EXPR_N_INTEGER: data_type=DATA_T_INTEGER; break;
		case EXPR_N_DOUBLE: data_type=DATA_T_DOUBLE; break;
		}
	    }
   
	/** Output the correct data type constant **/
	switch(data_type)
	    {
	    case DATA_T_DATETIME:
		ptr = objFormatDateTmp(&(tree->Types.Date), obj_default_date_fmt);
		xsConcatPrintf(clause, " \"%s\" ", ptr);
		/*objDataToString(clause, DATA_T_DATETIME, &(tree->Types.Date), DATA_F_QUOTED);*/
	        break;

	    case DATA_T_MONEY:
		ptr = objFormatMoneyTmp(&(tree->Types.Money), "0.0000");
		xsConcatPrintf(clause, " %s ", ptr);
		/*objDataToString(clause, DATA_T_MONEY, &(tree->Types.Money), DATA_F_QUOTED);*/
	        break;

	    case DATA_T_DOUBLE:
		objDataToString(clause, DATA_T_DOUBLE, &(tree->Types.Double), DATA_F_QUOTED);
	  	break;

	    case DATA_T_INTEGER:
		if (!tree->Parent || tree->Parent->NodeType == EXPR_N_AND ||
		    tree->Parent->NodeType == EXPR_N_OR)
		    {
		    if (tree->Integer)
			xsConcatenate(clause, " (1=1) ", 6);
		    else
			xsConcatenate(clause, " (1=0) ", 7);
		    }
		else
		    {
		    objDataToString(clause, DATA_T_INTEGER, &(tree->Integer), DATA_F_QUOTED);
		    }
		break;

	    case DATA_T_STRING:
		if (tree->Parent && tree->Parent->NodeType == EXPR_N_FUNCTION && 
		    (!strcmp(tree->Parent->Name,"convert") || !strcmp(tree->Parent->Name,"datepart") || !strcmp(tree->Parent->Name,"dateadd") || !strcmp(tree->Parent->Name,"datediff")) &&
		    (void*)tree == (void*)(tree->Parent->Children.Items[0]))
		    {
		    if (!strcmp(tree->Parent->Name,"convert"))
			{
			if (!strcmp(tree->String,"integer")) xsConcatenate(clause,"int",3);
			else if (!strcmp(tree->String,"string")) xsConcatenate(clause,"varchar(255)",-1);
			else if (!strcmp(tree->String,"double")) xsConcatenate(clause,"double",6);
			else if (!strcmp(tree->String,"money")) xsConcatenate(clause,"money",5);
			else if (!strcmp(tree->String,"datetime")) xsConcatenate(clause,"datetime",8);
			}
		    else
			{
			if (tree->String[strspn(tree->String,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")] != '\0')
			    {
			    mssError(1,"SYBD","Invalid datepart()/dateadd()/convert()/datediff() parameters in Expression Tree");
			    }
			else
			    {
			    objDataToString(clause, DATA_T_STRING, tree->String, 0);
			    }
			}
		    }
		else
		    {
		    /** Autoconvert string to int, double, or money if comparing with a
		     ** field of tht type.
		     **/
		    if (tree->Parent && tree->Parent->NodeType == EXPR_N_COMPARE)
			{
			/** Find other half of compare **/
			exp = (pExpression)(tree->Parent->Children.Items[0]);
			if (exp == tree)
			    exp = (pExpression)(tree->Parent->Children.Items[1]);

			/** Is it a property node? **/
			if (exp && exp->NodeType == EXPR_N_OBJECT) exp = (pExpression)(exp->Children.Items[0]);
			if (exp && exp->ObjID != -1)
			    {
			    colid = sybd_internal_ColNameToID(tdata[0], exp->Name);
			    if (colid >= 0)
				{
				t = sybd_internal_AttrType(tdata[0], colid);
				if (t == DATA_T_MONEY || t == DATA_T_INTEGER || t == DATA_T_DOUBLE)
				    {
				    objDataFromStringAlloc(&od, t, tree->String);
				    if (t == DATA_T_MONEY)
					{
					objDataToString(clause, t, od.Money, 0);
					nmFree(od.Money, sizeof(MoneyType));
					return 0;
					}
				    else
				    	{
					objDataToString(clause, t, &od, 0);
					return 0;
					}
				    }
				}
			    }
			}

		    objDataToString(clause, DATA_T_STRING, tree->String, DATA_F_QUOTED | DATA_F_SYBQUOTE);
		    }
		break;

	    default:
		mssError(1, "SYBD", "Build clause: Unsupported constant data type %d", data_type);
		return -1;
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
sybd_internal_TreeToClause(pExpression tree, pSybdNode node, pSybdConn conn, pSybdTableInf *tdata, int n_tdata, pXString where_clause)
    {
    pExpression subtree, subtree2;
    char* ptr;
    int i,id = 0;
    int add_rtrim;

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
	    case EXPR_N_STRING:
	    case EXPR_N_INTEGER:
	    case EXPR_N_DATETIME:
	    case EXPR_N_MONEY:
	    case EXPR_N_DOUBLE:
		sybd_internal_TreeToClauseConstant(tree, -1, tdata, n_tdata, where_clause);
		break;

	    case EXPR_N_OBJECT:
	        subtree = (pExpression)(tree->Children.Items[0]);
	        sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
		break;

	    case EXPR_N_PROPERTY:
	        /** 'Frozen' object?  IF so, use current tree value **/
		if ((tree->Flags & EXPR_F_FREEZEEVAL) || (tree->Parent && (tree->Parent->Flags & EXPR_F_FREEZEEVAL)) || (tree->ObjID == -1))
		    {
		    /** Direct ref object? **/
		    if (tree->ObjID == -1)
			{
			/** This tdata->ObjList doesn't really fit, but we use it because
			 ** we need something here with the objectsystem's session in it.
			 **/
			expEvalTree(tree,tdata[id]->ObjList);
			}

		    /** Otherwise, treat like a normal constant **/
		    sybd_internal_TreeToClauseConstant(tree, tree->DataType, tdata, n_tdata, where_clause);
		    }
		else
		    {

	        /** Direct ref object? **/
#if 00
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
	                objDataToString(where_clause, DATA_T_STRING, &(tree->String), DATA_F_QUOTED | DATA_F_SYBQUOTE);
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
#endif
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
			    sybd_internal_TreeToClause(tdata[id]->RowAnnotExpr, node,conn,tdata, n_tdata, where_clause);
			xsConcatenate(where_clause, ") ",2);
			}
		    else
		        {
		        /** "Normal" type of object... **/
	                xsConcatenate(where_clause, " ", 1);
			if (!strncmp(tree->Name, "__cx_literal_", 13) && tree->Name[13])
			    xsConcatenate(where_clause, tree->Name+13, -1);
			else
			    xsConcatenate(where_clause, tree->Name, -1);
	                xsConcatenate(where_clause, " ", 1);
			}
		    }
		break;

	    case EXPR_N_COMPARE:
		/** Special case -- comparing :name with a constant value **/
	        subtree = (pExpression)(tree->Children.Items[0]);
	        subtree2 = (pExpression)(tree->Children.Items[1]);
		if (tree->CompareType == MLX_CMP_EQUALS && 
		    ( (subtree->NodeType == EXPR_N_PROPERTY && !strcmp(subtree->Name, "name") && subtree2->NodeType == EXPR_N_STRING && !(subtree2->Flags & EXPR_F_NULL)) ||
		      (subtree2->NodeType == EXPR_N_PROPERTY && !strcmp(subtree2->Name, "name") && subtree->NodeType == EXPR_N_STRING && !(subtree->Flags & EXPR_F_NULL)) ))
		    {
		    ptr = sybd_internal_FilenameToKey(node, conn, tdata[id]->Table, (subtree->NodeType==EXPR_N_PROPERTY)?(subtree2->String):(subtree->String));
		    if (ptr)
			{
			/** Use special case only if FilenameToKey generated a valid clause **/
			xsConcatenate(where_clause, " (", 2);
			xsConcatenate(where_clause, ptr, -1);
			xsConcatenate(where_clause, ") ", 2);
			break;
			}
		    }

		/** Normal case **/
	        xsConcatenate(where_clause, " (", 2);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, " ", 1);
		if (tree->CompareType & MLX_CMP_LESS) xsConcatenate(where_clause,"<",1);
		if (tree->CompareType & MLX_CMP_GREATER) xsConcatenate(where_clause,">",1);
		if (tree->CompareType & MLX_CMP_EQUALS) xsConcatenate(where_clause,"=",1);
	        xsConcatenate(where_clause, " ", 1);
		sybd_internal_TreeToClause(subtree2,node,conn,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, ") ", 2);
	        break;

	    case EXPR_N_AND:
	        xsConcatenate(where_clause, " (",2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, " AND ",5);
	        subtree = (pExpression)(tree->Children.Items[1]);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
	        xsConcatenate(where_clause, ") ",2);
	        break;

	    case EXPR_N_OR:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
                xsConcatenate(where_clause, " OR ",4);
                subtree = (pExpression)(tree->Children.Items[1]);
                sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
                xsConcatenate(where_clause, ") ",2);
                break;

	    case EXPR_N_ISNOTNULL:
	        xsConcatenate(where_clause, " (",2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
		xsConcatenate(where_clause, " IS NOT NULL) ",14);
		break;

	    case EXPR_N_ISNULL:
	        xsConcatenate(where_clause, " (",2);
	        subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
		xsConcatenate(where_clause, " IS NULL) ",10);
		break;

	    case EXPR_N_NOT:
	        xsConcatenate(where_clause, " ( NOT ( ",9);
		subtree = (pExpression)(tree->Children.Items[0]);
		sybd_internal_TreeToClause(subtree,node,conn,tdata,n_tdata,where_clause);
		xsConcatenate(where_clause, " ) ) ",5);
		break;

	    case EXPR_N_FUNCTION:
	        /** Special case 'condition()' and 'ralign()' which Sybase doesn't have. **/
		if (!strcmp(tree->Name,"condition") && tree->Children.nItems == 3)
		    {
		    /*xsConcatenate(where_clause, " isnull((select substring(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",max(1),255) where ", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, "), ", 3);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") ", 2);*/
		    xsConcatenate(where_clause, " (case when (", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") then (", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") else (", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") end) ", -1);
		    }
		else if (!strcmp(tree->Name,"atan2") && tree->Children.nItems == 2)
		    {
		    /** Sybase calls this function atn2() instead of atan2() **/
		    xsConcatenate(where_clause, " atn2(", 6);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",", 1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") ", 2);
		    }
		else if (!strcmp(tree->Name,"ralign") && tree->Children.nItems == 2)
		    {
		    xsConcatenate(where_clause, " substring('", -1);
		    for(i=0;i<255 && i<((pExpression)(tree->Children.Items[1]))->Integer;i++)
		        xsConcatenate(where_clause, " ", 1);
		    xsConcatenate(where_clause, "',1,", 4);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, " - char_length(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ")) ", 3);
		    }
		else if (!strcmp(tree->Name,"eval"))
		    {
		    mssError(1,"SYBD","Sybase does not support eval() CXSQL function");
		    /* just put silly thing as text instead of evaluated */
		    if (tree->Children.nItems == 1) sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    return -1;
		    }
		else if (!strcmp(tree->Name, "replace"))
		    {
		    xsConcatenate(where_clause, " str_replace(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",", 1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",", 1);
		    subtree = (pExpression)tree->Children.Items[2];

		    /** Sybase by default treats "" as " ", but str_replace() accepts NULL for "" **/
		    if (subtree && subtree->NodeType == EXPR_N_STRING && !(subtree->Flags & EXPR_F_NULL) && !strcmp(subtree->String,""))
			xsConcatenate(where_clause, "NULL) ", 6);
		    else
			{
			sybd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), node,conn,tdata, n_tdata, where_clause);
			xsConcatenate(where_clause, ") ", 2);
			}
		    }
		else if (!strcmp(tree->Name, "hash"))
		    {
		    /** Sybase ASE uses hashbytes() instead of hash(), but the param order is the same. **/
		    xsConcatenate(where_clause, " hashbytes(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ",", 1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ") ", 2);
		    }
		else if (!strcmp(tree->Name,"round") && tree->Children.nItems == 1)
		    {
		    /** Centrallix accepts single-argument round() but Sybase does not **/
		    xsConcatenate(where_clause, " round(", -1);
		    sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
		    xsConcatenate(where_clause, ", 0) ", -1);
		    }
		else
		    {
	            xsConcatenate(where_clause, " ", 1);
		    xsConcatenate(where_clause, tree->Name, -1);
		    xsConcatenate(where_clause, "(", 1);
		    add_rtrim = (!strcmp(tree->Name, "right") && tree->Children.nItems > 0 && (((pExpression)(tree->Children.Items[0]))->NodeType == EXPR_N_OBJECT || ((pExpression)(tree->Children.Items[0]))->NodeType == EXPR_N_PROPERTY));
		    for(i=0;i<tree->Children.nItems;i++)
		        {
			if (i == 0 && add_rtrim)
			    xsConcatenate(where_clause, "rtrim(", 6);
		        if (i != 0) xsConcatenate(where_clause,",",1);
		        subtree = (pExpression)(tree->Children.Items[i]);
		        sybd_internal_TreeToClause(subtree, node,conn,tdata, n_tdata, where_clause);
			if (i == 0 && add_rtrim)
			    xsConcatenate(where_clause, ")", 1);
		        }
		    xsConcatenate(where_clause, ") ", 2);
		    }
		break;

	    case EXPR_N_PLUS:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " + ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_MINUS:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " - ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_DIVIDE:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " / ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_MULTIPLY:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " * ", 3);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, ") ", 2);
		break;

	    case EXPR_N_IN:
	        xsConcatenate(where_clause, " (", 2);
	        sybd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), node,conn,tdata, n_tdata, where_clause);
	        xsConcatenate(where_clause, " IN (", 5);
		subtree = (pExpression)(tree->Children.Items[1]);
		if (subtree->NodeType == EXPR_N_LIST)
		    {
		    for(i=0;i<subtree->Children.nItems;i++)
		        {
			if (i != 0) xsConcatenate(where_clause, ",", 1);
			sybd_internal_TreeToClause((pExpression)(subtree->Children.Items[i]), node,conn,tdata, n_tdata, where_clause);
			}
		    }
		else
		    {
	            sybd_internal_TreeToClause(subtree, node,conn,tdata, n_tdata, where_clause);
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
    int maxlen;

    	/** Copy the row. **/
	endptr = inf->RowBuf;
	for(i=0;i<cnt;i++)
	    {
	    ptr = NULL;
	    maxlen = 1536;
	    if (((inf->RowBuf + 2048) - endptr) - 2 < maxlen) 
		maxlen = ((inf->RowBuf + 2048) - endptr) - 2;
	    if (maxlen <= 0)
		{
		mssError(1,"SYBD","Bark!  Row buffer overflow while retrieving record.");
		return -1;
		}
	    ct_get_data(s, i+1, endptr, maxlen, (CS_INT*)&n);
	    if (n==0) 
	        {
		inf->ColPtrs[i] = NULL;
		}
	    else
	        {
		if (inf->TData->ColTypes[i] == 1)
		    {
		    /* rtrim char() fields */
		    while (n && endptr[n-1] == ' ') n--;
		    }
		inf->ColPtrs[i] = endptr;
		endptr[n] = 0;
		endptr+=(n+1);
		}
	    }

    return 0;
    }


/*** sybd_internal_LookupRow() - given a table name and a row key name,
 *** retrieve the row from the database.
 ***/
int
sybd_internal_LookupRow(pSybdConn conn, pSybdData inf)
    {
    char* ptr;
    int cnt;
    char sbuf[256];
    CS_COMMAND* cmd;
    int ncols, i, n;
    CS_INT restype = 0;
    CS_USHORT msgid;

	/** Find a WHERE clause that will retrieve this row, given the row name **/
	ptr = sybd_internal_FilenameToKey(inf->Node, conn, inf->TablePtr, inf->RowColPtr);
	if (!ptr)
	    return -1;

	/** Run the SQL query **/
#if 00 /* locking */
	while(1)
	    {
#endif
	    snprintf(sbuf,sizeof(sbuf),"SELECT * from %s WHERE %s",inf->TablePtr, ptr);
	    if ((cmd=sybd_internal_Exec(conn, sbuf)) == NULL)
		{
		mssError(0,"SYBD","Could not retrieve row object [%s] from database table [%s]",
			inf->RowColPtr, inf->TablePtr);
		return -1;
		}
	    cnt = 0;

	    /** Fetch result info **/
	    while (ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED)
		{
		if (restype == CS_ROW_RESULT)
		    {
		    ct_res_info(cmd, CS_NUMDATA, (CS_INT*)&ncols, CS_UNUSED, NULL);
		    for(i=0; i < ncols && i < inf->TData->nCols; i++)
			inf->ColNum[i] = (unsigned char)i;
		    while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&n) == CS_SUCCEED)
			{
			cnt++;

			/** Good, found the row, let's load it **/
			if (sybd_internal_GetRow(inf,cmd,ncols) < 0)
			    return -1;
			}
		    }
#if 00 /* locking */
		else if (restype == CS_MSG_RESULT)
		    {
		    msgid = 0;
		    ct_res_info(cmd, CS_MSGTYPE, (CS_VOID*)&msgid, CS_UNUSED, NULL);
		    if (msgid == 12205)
		    }
#endif
		}
	    sybd_internal_Close(cmd);
#if 00 /* locking */
	    }
#endif

    return cnt;
    }


/*** sybd_internal_ColNameToID() - determine the column ID of a column with the given
 *** name, or return -1 if no such column.
 ***/
int
sybd_internal_ColNameToID(pSybdTableInf tdata, char* colname)
    {
    int i;

	for(i=0;i<tdata->nCols;i++)
	    {
	    if (!strcmp(colname, tdata->Cols[i]))
		return i;
	    }

    return -1;
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
    char* ptr;

	/** Allocate the structure **/
	inf = (pSybdData)nmMalloc(sizeof(SybdData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(SybdData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine type and set pointers. **/
	sybd_internal_DetermineType(obj,inf);

	/** Access the DB node. **/
	memccpy(sbuf, obj_internal_PathPart(obj->Pathname,0,obj->SubPtr), 0, sizeof(sbuf)-1);
	sbuf[sizeof(sbuf)-1] = '\0';
	obj_internal_PathPart(obj->Pathname,0,0);
	inf->Node = sybd_internal_OpenNode(sbuf,obj->Mode,obj,inf->Type == SYBD_T_DATABASE,mask);
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
	inf->SessionID = sybd_internal_GetConn(inf->Node);
	if (inf->SessionID == NULL)
	    {
	    mssError(0,"SYBD","Database session setup failed");
	    nmFree(inf,sizeof(SybdData));
	    return NULL;
	    }

	/** Verify the table, if a table mentioned **/
	if (inf->TablePtr)
	    {
	    if (strpbrk(inf->TablePtr," \t\r\n"))
		{
		mssError(1,"SYBD","Requested table %s is invalid", inf->TablePtr);
		sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		nmFree(inf,sizeof(SybdData));
		return NULL;
		}
	    tdata = sybd_internal_GetTableInf(inf->Node, inf->SessionID, inf->TablePtr);
	    inf->TData = tdata;
	    if (!tdata && (!(obj->Mode & O_CREAT) || inf->TableSubPtr))
	        {
		mssError(0,"SYBD","Requested table does not exist or is inaccessible");
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
		    /*new_oxt->Object = (void*)nmMalloc(sizeof(Object));
		    memset(new_oxt->Object, 0, sizeof(Object));*/
		    new_oxt->Object = obj_internal_AllocObj();
		    new_oxt->OpType = OXT_OP_CREATE;
		    new_oxt->Status = OXT_S_VISITED;
		    ((pObject)(new_oxt->Object))->Pathname = 
		        (pPathname)obj_internal_NormalizePath(obj->Pathname->Pathbuf,"rows");
		    obj_internal_AddChildTree(*oxt,new_oxt);
		    new_oxt = obj_internal_AllocTree();
		    new_oxt->AllocObj = 1;
		    /*new_oxt->Object = (void*)nmMalloc(sizeof(Object));
		    memset(new_oxt->Object, 0, sizeof(Object));*/
		    new_oxt->Object = obj_internal_AllocObj();
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
	    inf->ColID = sybd_internal_ColNameToID(inf->TData, inf->RowColPtr);

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
		(*oxt)->Object = obj;
		return inf;
		}
	    }

	/** If a row is being accessed, verify its existence / open its row **/
	if (inf->Type == SYBD_T_ROW)
	    {
	    cnt = 0;

	    /** Autonaming a new object?  Won't be able to look it up if so. **/
	    if (!(inf->Obj->Mode & OBJ_O_AUTONAME))
		{
		if ((cnt = sybd_internal_LookupRow(inf->SessionID, inf)) < 0)
		    {
		    sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		    nmFree(inf,sizeof(SybdData));
		    return NULL;
		    }
		}
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
		(*oxt)->Object = obj;
		(*oxt)->Status = OXT_S_VISITED;
		return inf;
		}
	    }

	/** Release session, for now **/
	sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
	inf->SessionID = NULL;

    return (void*)inf;
    }


/*** sybd_internal_BuildAutoname - figures out how to fill in the required 
 *** information for an autoname-based row insertion, and populates the
 *** "Autoname" field in the SybdData structure accordingly.  Looks in the
 *** OXT for a partially (or fully) completed key, and uses an autonumber
 *** type of approach to complete the remainder of the key.  Returns nonnegative
 *** on success and with the autoname semaphore held.  Returns negative on error
 *** with the autoname semaphore NOT held.  On success, the semaphore must be
 *** released after the successful insertion of the new row.
 ***/
int
sybd_internal_BuildAutoname(pSybdData inf, pSybdConn conn, pObjTrxTree oxt)
    {
    pObjTrxTree keys_provided[8];
    int n_keys_provided = 0;
    int i,j,colid;
    pObjTrxTree find_oxt, attr_oxt;
    pXString sql = NULL;
    char* key_values[8];
    char* ptr;
    int rval = 0;
    int first_clause = 1;
    int t;
    CS_COMMAND* cmd = NULL;
    CS_INT restype = 0;
    int intval;
    int len;

	if (inf->TData->nKeys == 0) return -1;
	for(j=0;j<inf->TData->nKeys;j++) key_values[j] = NULL;

	/** Snarf the autoname semaphore **/
	syGetSem(inf->TData->AutonameSem, 1, 0);

	/** Try to locate provided keys in the OXT, leave NULL if not found. **/
	for(j=0;j<inf->TData->nKeys;j++)
	    {
	    colid = inf->TData->KeyCols[j];
	    find_oxt=NULL;
	    for(i=0;i<oxt->Children.nItems;i++)
		{
		attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
		if (attr_oxt->OpType == OXT_OP_SETATTR)
		    {
		    if (!strcmp(attr_oxt->AttrName,inf->TData->Cols[colid]))
			{
			find_oxt = attr_oxt;
			find_oxt->Status = OXT_S_COMPLETE;
			break;
			}
		    }
		}
	    if (find_oxt && find_oxt->AttrValue != NULL) 
		{
		n_keys_provided++;
		keys_provided[j] = find_oxt;
		if (find_oxt->AttrType == DATA_T_MONEY)
		    {
		    ptr = objFormatMoneyTmp(find_oxt->AttrValue, "0.0000");
		    }
		else
		    {
		    ptr = objDataToStringTmp(find_oxt->AttrType, find_oxt->AttrValue, 0);
		    }
		key_values[j] = nmSysStrdup(ptr);
		if (!key_values[j])
		    {
		    rval = -ENOMEM;
		    goto exit_BuildAutoname;
		    }
		}
	    }

	/** Build the SQL to do the select max()+1 **/
	sql = (pXString)nmMalloc(sizeof(XString));
	if (!sql)
	    {
	    rval = -ENOMEM;
	    goto exit_BuildAutoname;
	    }
	xsInit(sql);
	for(j=0;j<inf->TData->nKeys;j++)
	    {
	    colid = inf->TData->KeyCols[j];
	    if (!key_values[j])
		{
		if (inf->TData->ColTypes[colid] != 5 && inf->TData->ColTypes[colid] != 6 && 
			inf->TData->ColTypes[colid] != 7 && inf->TData->ColTypes[colid] != 16)
		    {
		    mssError(1,"SYBD","Non-integer key field '%s' left NULL on autoname create", inf->TData->Cols[colid]);
		    rval = -1;
		    goto exit_BuildAutoname;
		    }
		xsPrintf(sql, "SELECT isnull(max(%s),0)+1 FROM %s", inf->TData->Cols[colid], inf->TData->Table);
		first_clause = 1;
		for(i=0;i<inf->TData->nKeys;i++)
		    {
		    if (key_values[i])
			{
			if (first_clause)
			    xsConcatenate(sql, " WHERE ", 7);
			else
			    xsConcatenate(sql, " AND ", 5);
			first_clause = 0;
			xsConcatenate(sql, inf->TData->Cols[inf->TData->KeyCols[i]], -1);
			xsConcatenate(sql, " = ", 3);
			t = sybd_internal_AttrType(inf->TData, inf->TData->KeyCols[i]);
			if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE || t == DATA_T_MONEY)
			    xsConcatenate(sql, key_values[i], -1);
			else
			    xsConcatPrintf(sql, "'%s'", key_values[i]);
			}
		    }

		/** Now that sql is built, send to server **/
		if ((cmd=sybd_internal_Exec(conn, sql->String)))
		    {
		    while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED) if (restype == CS_ROW_RESULT)
			{
			while(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,(CS_INT*)&i) == CS_SUCCEED)
			    {
			    /** Get the table's annotation **/
			    intval = 0;
			    ct_get_data(cmd, 1, &intval, 255, (CS_INT*)&i);
			    key_values[j] = nmSysMalloc(16);
			    if (!key_values[j])
				{
				rval = -ENOMEM;
				goto exit_BuildAutoname;
				}
			    sprintf(key_values[j], "%d", intval);
			    }
			}
		    sybd_internal_Close(cmd);
		    cmd = NULL;
		    }
		else
		    {
		    mssError(1,"SYBD","Could not obtain next-value information for key '%s' on table '%s'",
			    inf->TData->Cols[colid], inf->TData->Table);
		    rval = -1;
		    goto exit_BuildAutoname;
		    }
		}
	    }

	/** Okay, all keys filled in.  Build the autoname. **/
	inf->Autoname[0] = '\0';
	for(len=j=0;j<inf->TData->nKeys;j++)
	    {
	    if (j) len += 1; /* for | separator */
	    len += strlen(key_values[j]);
	    }
	if (len >= sizeof(inf->Autoname))
	    {
	    mssError(1,"SYBD","Autoname too long!");
	    rval = -1;
	    goto exit_BuildAutoname;
	    }
	for(j=0;j<inf->TData->nKeys;j++)
	    {
	    if (j) strcat(inf->Autoname,"|");
	    strcat(inf->Autoname, key_values[j]);
	    }

    exit_BuildAutoname:
	/** Release resources we used **/
	if (cmd) sybd_internal_Close(cmd);
	if (rval < 0) 
	    syPostSem(inf->TData->AutonameSem, 1, 0);
	if (sql)
	    {
	    xsDeInit(sql);
	    nmFree(sql, sizeof(XString));
	    }
	for(j=0;j<inf->TData->nKeys;j++) 
	    if (key_values[j]) nmSysFree(key_values[j]);

    return rval;
    }



/*** sybd_internal_InsertRow - inserts a new row into the database, looking
 *** throught the OXT structures for the column values.  For text/image columns,
 *** it automatically inserts "" for an unspecified column value when the column
 *** does not allow nulls.
 ***/
int
sybd_internal_InsertRow(pSybdData inf, pSybdConn conn, pObjTrxTree oxt)
    {
    char* kptr;
    char* kendptr;
    int i,j,len,ctype;
    CS_INT restype = 0;
    pObjTrxTree attr_oxt, find_oxt;
    CS_COMMAND* cmd;
    pXString insbuf;
    char* tmpptr;
    char tmpch;
    int colid;
    int holding_sem = 0;
    MoneyType m;

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

	/** If we are using an autoname-based create, here is where we build the name. **/
	if (inf->Obj->Mode & OBJ_O_AUTONAME && inf->TData->nKeys)
	    {
	    if (sybd_internal_BuildAutoname(inf, conn, oxt) < 0) return -1;
	    inf->RowColPtr = inf->Autoname;
	    holding_sem = 1; /* we hold the semaphore on successful BuildAutoname return */
	    }

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
		for(i=0;i<inf->TData->ColKeys[j] && kptr != (char*)1;i++) kptr = strchr(kptr,'|')+1;
		if (kptr == (char*)1)
		    {
		    mssError(1,"SYBD","Not enough components in concat primary key (name)");
		    xsDeInit(insbuf);
		    nmFree(insbuf,sizeof(XString));
		    if (holding_sem) syPostSem(inf->TData->AutonameSem, 1, 0);
		    return -1;
		    }
		kendptr = strchr(kptr,'|');
		if (!kendptr) len = strlen(kptr); else len = kendptr-kptr;

		/** Copy it to the INSERT statement buffer **/
		ctype = sybd_internal_AttrType(inf->TData, j);
		if (ctype == DATA_T_INTEGER)
		    {
		    xsConcatenate(insbuf, kptr, len);
		    }
		else if (ctype == DATA_T_STRING)
		    {
		    tmpch = kptr[len];
		    kptr[len] = 0;
		    tmpptr = objDataToStringTmp(DATA_T_STRING, kptr, DATA_F_QUOTED | DATA_F_SYBQUOTE);
		    kptr[len] = tmpch;
		    xsConcatenate(insbuf, tmpptr, -1);
		    }
		else if (ctype == DATA_T_MONEY)
		    {
		    tmpch = kptr[len];
		    kptr[len] = 0;
		    if (objDataToMoney(DATA_T_STRING, kptr, &m) == 0)
			{
			tmpptr = objFormatMoneyTmp(&m, "0.0000");
			if (!tmpptr)
			    tmpptr = " NULL ";
			xsConcatenate(insbuf, tmpptr, -1);
			}
		    kptr[len] = tmpch;
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
                        if (!strcmp(attr_oxt->AttrName,inf->TData->Cols[j]) || (!strncmp(attr_oxt->AttrName,"__cx_literal_",13) && !strcmp(attr_oxt->AttrName+13, inf->TData->Cols[j])))
                            {
                            find_oxt = attr_oxt;
                            find_oxt->Status = OXT_S_COMPLETE;
                            break;
                            }
                        }
                    }
		if (j!=0) xsConcatenate(insbuf,",",1);

                /** Print the appropriate type. **/
                if (!find_oxt || find_oxt->AttrValue == NULL)
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
                        mssError(1,"SYBD","Required column '%s' not specified in object create", inf->TData->Cols[j]);
			xsDeInit(insbuf);
			nmFree(insbuf,sizeof(XString));
			if (holding_sem) syPostSem(inf->TData->AutonameSem, 1, 0);
                        return -1;
                        }
                    }
                else 
		    {
		    if (find_oxt->AttrType == DATA_T_DATETIME)
			xsConcatPrintf(insbuf, " \"%s\" ", objFormatDateTmp(find_oxt->AttrValue, obj_default_date_fmt));
		    else if (find_oxt->AttrType == DATA_T_MONEY)
			xsConcatPrintf(insbuf, " %s ", objFormatMoneyTmp(find_oxt->AttrValue, "0.0000"));
		    else
			objDataToString(insbuf, find_oxt->AttrType, find_oxt->AttrValue, DATA_F_QUOTED | DATA_F_SYBQUOTE);
		    }
                }
	    }

        /** Add the trailing ')' and issue the query. **/
	xsConcatenate(insbuf,")", 1);
        if ((cmd = sybd_internal_Exec(conn, insbuf->String))==NULL)
            {
	    xsDeInit(insbuf);
            nmFree(insbuf,sizeof(XString));
            sybd_internal_Close(cmd);
	    mssError(0,"SYBD","Could not execute SQL to insert new table row");
	    if (holding_sem) syPostSem(inf->TData->AutonameSem, 1, 0);
            return -1;
            }
	while(ct_results(cmd,(CS_INT*)&restype) == CS_SUCCEED);
        sybd_internal_Close(cmd);
	xsDeInit(insbuf);
        nmFree(insbuf,sizeof(XString));
	if (inf->TData && inf->TData->RowCount >= 0) inf->TData->RowCount++;
	if (holding_sem) syPostSem(inf->TData->AutonameSem, 1, 0);

    return 0;
    }


/*** sybdCommit - commit changes made to the object, but don't close it.
 ***/
int
sybdCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    struct stat fileinfo;
    int i;
    CS_INT restype = 0;
    char sbuf[160];

	/** Write session is a bit complex.  IF needed, complete the write. **/
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
		    while((i=fdRead(inf->TmpFD,sbuf,sizeof(sbuf),0,0)))
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
		while(ct_results(inf->RWCmd, (CS_INT*)&restype) == CS_SUCCEED);
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
		        sybd_internal_GetConn(inf->Node);
		    if (!inf->SessionID)
			{
			mssError(0,"SYBD","Database connection failed");
			return -1;
			}

		    /** Perform the insert. **/
		    if (sybd_internal_InsertRow(inf,inf->SessionID, *oxt) < 0)
		        {
			/** FAIL the oxt. **/
			(*oxt)->Status = OXT_S_FAILED;

			/** Release the open object data **/
			sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
			inf->SessionID = NULL;
			return -1;
			}

		    sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
		    inf->SessionID = NULL;

		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case SYBD_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }

    return 0;
    }


/*** sybdClose - close an open file or directory.
 ***/
int
sybdClose(void* inf_v, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int rval;

    	/** Is a ReadSess in progress? **/
	if (inf->ReadSessID != NULL) 
	    {
	    if (inf->RWCmd) sybd_internal_Close(inf->RWCmd);
	    inf->RWCmd = NULL;
	    sybd_internal_ReleaseConn(inf->Node, inf->ReadSessID);
	    inf->ReadSessID = NULL;
	    }

	/** Commit changes (this closes WriteSessID too) **/
	rval = sybdCommit(inf_v, oxt);

	/** Disconnect the DB session, if needed. **/
	if (inf->SessionID != NULL)
	    {
	    sybd_internal_ReleaseConn(inf->Node,inf->SessionID);
	    inf->SessionID = NULL;
	    }

	/** Free the info structure **/
	obj_internal_FreePathStruct(&inf->Pathname);
	nmFree(inf,sizeof(SybdData));

    return rval;
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


/*** sybdDeleteObj - delete an existing object that is already open.
 ***/
int
sybdDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    char sbuf[256];
    pSybdData inf = SYBD(inf_v);
    CS_COMMAND* cmd;
    int rval;
    CS_INT restype = 0;
    char* ptr;

	/** Grab a database connection **/
	inf->SessionID = sybd_internal_GetConn(inf->Node);
	if (inf->SessionID == NULL)
	    {
	    mssError(0,"SYBD","Database session setup failed");
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }

	/** Create the where clause for the delete. **/
	ptr = sybd_internal_FilenameToKey(inf->Node,inf->SessionID,inf->TablePtr,inf->RowColPtr);
	if (!ptr)
	    {
	    sybd_internal_ReleaseConn(inf->Node,inf->SessionID);
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }
	snprintf(sbuf,sizeof(sbuf),"DELETE FROM %s WHERE %s",inf->TablePtr, ptr);

	/** Run the delete **/
	if ((cmd = sybd_internal_Exec(inf->SessionID,sbuf)) == NULL)
	    {
	    mssError(0,"SYBD","Delete operation failed");
	    sybd_internal_ReleaseConn(inf->Node,inf->SessionID);
	    nmFree(inf,sizeof(SybdData));
	    return -1;
	    }
	while(ct_results(cmd, (CS_INT*)&restype) == CS_SUCCEED);
	sybd_internal_Close(cmd);
	sybd_internal_ReleaseConn(inf->Node,inf->SessionID);

	/** Free the structure **/
	nmFree(inf,sizeof(SybdData));

    return 0;
    }


/*** sybdDelete - delete an existing object.
 ***/
int
sybdDelete(pObject obj, pObjTrxTree* oxt)
    {
    pSybdData inf;
    int rval = 0;

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

	/** Delete it **/
	rval = sybdDeleteObj(inf, oxt);

    return 0;
    }


/*** sybd_internal_PrepareText - prepares for a Text or Image I/O operation
 *** to the database.  This routine retrieves the i/o descriptor and gets the
 *** text/image column ready for reading, with a given max text size.  If 
 *** preparing for an update operation, the calling routine can just close()
 *** the returned CS_COMMAND*.
 ***/
CS_COMMAND*
sybd_internal_PrepareText(pSybdData inf, pSybdConn conn, int maxtextsize)
    {
    char* col;
    int i;
    char buffer[1];
    char sbuf[160];
    int rcnt;
    CS_INT restype = 0;
    char* ptr;

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
	    ptr = sybd_internal_FilenameToKey(inf->Node, conn, inf->TablePtr, inf->RowColPtr);
	    if (!ptr)
		{
		return NULL;
		}
	    snprintf(sbuf,160,"UPDATE %s SET %s='' where %s", inf->TablePtr,col, ptr);
	    if ((inf->RWCmd = sybd_internal_Exec(conn, sbuf)) == NULL) 
	        {
		mssError(0,"SYBD","Could not run update to initialize textptr for content BLOB");
		return NULL;
		}
	    while (ct_results(inf->RWCmd,(CS_INT*)&restype)==CS_SUCCEED);
	    sybd_internal_Close(inf->RWCmd);
	    }

	/** Build the command. **/
	ptr = sybd_internal_FilenameToKey(inf->Node, conn, inf->TablePtr, inf->RowColPtr);
	if (!ptr)
	    {
	    return NULL;
	    }
	snprintf(sbuf,160,"set textsize %d select %s from %s where %s set textsize 255",
	    maxtextsize,col,inf->TablePtr,ptr);
	if ((inf->RWCmd = sybd_internal_Exec(conn, sbuf)) == NULL) 
	    {
	    mssError(0,"SYBD","Could not run SQL to retrieve content BLOB from database");
	    return NULL;
	    }

	/** Wait for the actual result row to come back. **/
	while (ct_results(inf->RWCmd,(CS_INT*)&restype)==CS_SUCCEED && restype!=CS_ROW_RESULT);
	if (restype != CS_ROW_RESULT)
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

    return inf->RWCmd;
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
		inf->ReadSessID = sybd_internal_GetConn(inf->Node);
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
    pFile fd;

	/** Use 26 letters, with timestamp and random value **/
	t = time(NULL);
    	for(ch='a';ch<='z';ch++)
	    {
	    sprintf(name,"/tmp/LS-%8.8lX%4.4lX%c",t,lrand48()&0xFFFF,ch);
	    if (access(name,F_OK) < 0)
	        {
		fd = fdOpen(name, O_RDWR | O_NOFOLLOW, 0600);
		if (fd) return fd;
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
		inf->WriteSessID = sybd_internal_GetConn(inf->Node);
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
	    ct_cmd_alloc(inf->WriteSessID->CsConn, &(inf->RWCmd));
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
	    if (fdWrite(inf->TmpFD, buffer, cnt, 0, 0) < 0) 
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
    int i;
    CS_INT restype = 0;
    XString sql;
    pExpression exp;

	/** Allocate the query structure **/
	qy = (pSybdQuery)nmMalloc(sizeof(SybdQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(SybdQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->SessionID = NULL;
	qy->ObjSession = NULL;
	qy->Cmd = NULL;
	qy->Flags = 0;

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
		    {
		    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"DECLARE _c CURSOR FOR SELECT name from sysobjects where type = 'U' ORDER BY name");
		    qy->Flags |= SYBD_QF_USECURSOR;
		    }
		else
		    {
		    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"SELECT name from sysobjects where type = 'U' ORDER BY name");
		    }
		qy->SessionID = inf->SessionID;
		qy->ObjSession = inf->SessionID;
		inf->SessionID = NULL;
		if (qy->SessionID == NULL)
		    qy->SessionID = sybd_internal_GetConn(inf->Node);
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
		    qy->SessionID = sybd_internal_GetConn(inf->Node);
		if (qy->SessionID == NULL)
		    {
		    nmFree(qy,sizeof(SybdQuery));
		    mssError(0,"SYBD","Could not obtain database session to list rows");
		    return NULL;
		    }
		qy->TableInf = sybd_internal_GetTableInf(inf->Node,qy->SessionID,inf->TablePtr);
		xsInit(&sql);
		if (SYBD_USE_CURSORS && (inf->TData->RowCount < 0 || inf->TData->RowCount > SYBD_CURSOR_ROWCOUNT))
		    {
		    xsPrintf(&sql,"DECLARE _c CURSOR FOR SELECT * FROM %s",inf->TablePtr);
		    qy->Flags |= SYBD_QF_USECURSOR;
		    }
		else
		    {
		    xsPrintf(&sql,"SELECT * FROM %s",inf->TablePtr);
		    }
		query->Flags |= (OBJ_QY_F_FULLQUERY | OBJ_QY_F_FULLSORT);
		if (query->Tree)
		    {
		    xsConcatenate(&sql, " WHERE ", 7);
		    qy->TableInf->ObjList->Session = qy->ObjInf->Obj->Session;
		    exp = expReducedDuplicate(query->Tree);
		    sybd_internal_TreeToClause(exp, inf->Node, qy->SessionID, &(qy->TableInf), 1, &sql);
		    expFreeExpression(exp);
		    }
		if (query->SortBy[0])
		    {
		    xsConcatenate(&sql," ORDER BY ", 10);
		    for(i=0;query->SortBy[i] && i < (sizeof(query->SortBy)/sizeof(void*));i++)
			{
			if (i != 0) xsConcatenate(&sql, ", ", 2);
			sybd_internal_TreeToClause((pExpression)(query->SortBy[i]),inf->Node, qy->SessionID,&(qy->TableInf),1,&sql);
			}
	  	    }
		/*if (SYBD_USE_CURSORS && (inf->TData->RowCount < 0 || inf->TData->RowCount > SYBD_CURSOR_ROWCOUNT))
		    {
		    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"DECLARE _c CURSOR FOR SELECT * FROM %s %s",inf->TablePtr, sql.String);
		    qy->Flags |= SYBD_QF_USECURSOR;
		    }
		else
		    {
		    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"SELECT * FROM %s %s",inf->TablePtr, sql.String);
		    }*/
		if (strcmp(sql.String, SYBD_INF.LastSQL.String) || 1)
		    {
		    if (SYBD_INF.SqlLog)
			{
			thLock();
			fdPrintf(SYBD_INF.SqlLog, "SQL:  %s\n",sql.String);
			thUnlock();
			}
		    xsCopy(&SYBD_INF.LastSQL, sql.String, -1);
		    }
		if ((qy->Cmd = sybd_internal_Exec(qy->SessionID, sql.String))==NULL)
		    {
		    sybd_internal_ReleaseConn(inf->Node, qy->SessionID);
		    nmFree(qy,sizeof(SybdQuery));
		    xsDeInit(&sql);
		    mssError(0,"SYBD","Could not execute SQL for query on table object");
		    return NULL;
		    }
		xsDeInit(&sql);
		break;

	    case SYBD_T_COLUMN:
	    case SYBD_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(SybdQuery));
		qy = NULL;
		break;
	    }

	/** Cursor mode retrieval?  Open cursor if so. **/
	if (qy && (qy->Flags & SYBD_QF_USECURSOR) && qy->Cmd)
	    {
	    while(ct_results(qy->Cmd,(CS_INT*)&restype)==CS_SUCCEED && restype != CS_CMD_DONE);
	    sybd_internal_Close(qy->Cmd);
	    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"OPEN _c SET CURSOR ROWS %d FOR _c FETCH _c", SYBD_CURSOR_ROWCOUNT);
	    qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	    if (!qy->Cmd)
	        {
		nmFree(qy,sizeof(SybdQuery));
		mssError(0,"SYBD","Could not open cursor for query result set retrieval");
		return NULL;
		}
	    qy->Flags |= SYBD_QF_CURSOROPEN;
	    snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"FETCH _c");
	    qy->RowsSinceFetch = 0;
	    }

    return (void*)qy;
    }


/*** sybd_internal_CloseCursor - shut down an open cursor
 ***/
int
sybd_internal_CloseCursor(pSybdQuery qy)
    {
    int rid;
    CS_INT restype = 0;

	/** Close any existing command **/
	if (qy->Cmd)
	    {
	    sybd_internal_Close(qy->Cmd);
	    qy->Cmd = NULL;
	    }

	/** Shut down the cursor **/
	snprintf(qy->SQLbuf,sizeof(qy->SQLbuf),"CLOSE _c DEALLOCATE CURSOR _c");
	qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	if (qy->Cmd)
	    {
	    while((rid=ct_results(qy->Cmd,(CS_INT*)&restype))==CS_SUCCEED && restype != CS_CMD_DONE)
		{
		}
	    sybd_internal_Close(qy->Cmd);
	    qy->Cmd = NULL;
	    }
	qy->Flags &= ~SYBD_QF_CURSOROPEN;

    return 0;
    }


/*** sybdQueryFetch - get the next directory entry as an open object.
 ***/
void*
sybdQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pSybdQuery qy = ((pSybdQuery)(qy_v));
    pSybdData inf = NULL;
    char filename[120];
    char* ptr;
    int new_type;
    int i,cnt;
    pSybdTableInf tdata = qy->ObjInf->TData;
    pSybdConn conn2;
    CS_INT restype = 0;
    int rid;

    	/** Fetch the row. **/
	if (qy->SessionID != NULL)
	    {
	    cnt=0;
	    while(qy->Cmd != NULL)
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
	            if (cnt == 0) 
			goto end_results;
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
		else if (cnt == 0 && (!(qy->Flags & SYBD_QF_USECURSOR) || qy->RowsSinceFetch < SYBD_CURSOR_ROWCOUNT))
		    {
		    /** No rows left and fewer than rowcount (or no) rows returned - query over. **/
		    /** Release the command structure now that we're done. **/
		    if (qy->Cmd) sybd_internal_Close(qy->Cmd);
		    qy->Cmd = NULL;

		    goto end_results;
		    }
		else
		    {
		    /** No rows but we filled the cursor last time.  Do another fetch. **/
	    	    sybd_internal_Close(qy->Cmd);
	    	    qy->Cmd = sybd_internal_Exec(qy->SessionID, qy->SQLbuf);
	    	    if (!qy->Cmd)
	        	{
			mssError(0,"SYBD","Could not fetch next part of query results");
			goto error;
			}
	    	    qy->RowsSinceFetch = 0;
		    }
		}
	    if (cnt == 0)
		goto end_results;
	    }
	qy->RowCnt++;
	qy->RowsSinceFetch++;

	/** Allocate the structure **/
	inf = (pSybdData)nmMalloc(sizeof(SybdData));
	if (!inf)
	    goto error;
	memset(inf,0,sizeof(SybdData));
	inf->TData = tdata;

    	/** Get the next name based on the query type. **/
	switch(qy->ObjInf->Type)
	    {
	    case SYBD_T_DATABASE:
	        /** Get filename from the first column - table name. **/
		new_type = SYBD_T_TABLE;
		ct_get_data(qy->Cmd, 1, filename, 119, (CS_INT*)&i);
		filename[i] = 0;
		conn2 = sybd_internal_GetConn(qy->ObjInf->Node);
		if (!conn2)
		    {
		    mssError(0,"SYBD","Database connection failed");
		    goto error;
		    }
		tdata = sybd_internal_GetTableInf(qy->ObjInf->Node,conn2,filename);
		inf->TData = tdata;
		sybd_internal_ReleaseConn(qy->ObjInf->Node,conn2);
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
		    goto end_results;
		    }
	        break;

	    case SYBD_T_ROWSOBJ:
	        /** Get the filename from the primary key of the row. **/
		if (sybd_internal_GetRow(inf,qy->Cmd,qy->TableInf->nCols) < 0)
		    {
		    goto error;
		    }
		new_type = SYBD_T_ROW;
		ptr = sybd_internal_KeyToFilename(qy->TableInf,inf);
		if (!ptr)
		    {
		    goto error;
		    }
	        strcpy(filename,ptr);
	        break;

	    case SYBD_T_COLSOBJ:
	        /** Loop through the columns in the TableInf structure. **/
		new_type = SYBD_T_COLUMN;
		if (qy->RowCnt <= qy->TableInf->nCols)
		    {
		    memccpy(filename,qy->TableInf->Cols[qy->RowCnt-1], 0, 119);
		    filename[119] = 0;
		    }
		else
		    {
		    goto end_results;
		    }
	        break;
	    }

	/** Build the filename. **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(filename) >= 255)
	    {
	    mssError(1,"SYBD","Pathname too long for internal representation");
	    goto error;
	    }
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

    error:
    end_results:
	/** Release the new object structure **/
	if (inf)
	    nmFree(inf, sizeof(SybdData));

	/** Close any pending command **/
	if (qy->Cmd)
	    {
	    sybd_internal_Close(qy->Cmd);
	    qy->Cmd = NULL;
	    }

	/** Shutdown the cursor **/
	if (qy->Flags & SYBD_QF_CURSOROPEN)
	    {
	    sybd_internal_CloseCursor(qy);
	    }

	/** No more results. **/
	return NULL;
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
    CS_INT restype = 0;
    int rid;

    	/** Release the command structure. **/
	if (qy->Cmd)
	    {
	    sybd_internal_Close(qy->Cmd);
	    qy->Cmd = NULL;
	    }

	/** Deallocate the cursor? **/
	if (qy->Flags & SYBD_QF_CURSOROPEN)
	    {
	    sybd_internal_CloseCursor(qy);
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

	/** Bypass system names? **/
	if (!strncmp(attrname, "__cx_literal_", 13))
	    attrname = attrname + 13;

    	/** Attr type depends on object type. **/
	if (inf->Type == SYBD_T_ROW)
	    {
	    tdata = inf->TData;
	    i = sybd_internal_ColNameToID(tdata, attrname);
	    if (i >= 0)
		return sybd_internal_AttrType(tdata, i);
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
sybdGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int i,t,minus,n,rval;
    unsigned int msl,lsl,divtmp;
    pSybdTableInf tdata;
    char* ptr;
    int days,fsec;
    float f;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Obj->Mode & OBJ_O_AUTONAME)
		{
		if (!inf->Autoname[0])
		    return 1; /* value is NULL */
		val->String = inf->Autoname;
		return 0;
		}
	    ptr = inf->Pathname.Elements[inf->Pathname.nElements-1];
	    if (ptr[0] == '.' && ptr[1] == '\0')
	        {
	        val->String = "/";
		}
	    else
	        {
	        val->String = ptr;
		}
	    return 0;
	    }

	/** Is it an annotation? **/
	if (!strcmp(attrname, "annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE:
		    val->String = inf->Node->Description;
		    break;
		case SYBD_T_TABLE:
		    val->String = inf->TData->Annotation;
		    break;
		case SYBD_T_ROWSOBJ:
		    val->String = "Contains rows for this table";
		    break;
		case SYBD_T_COLSOBJ:
		    val->String = "Contains columns for this table";
		    break;
		case SYBD_T_COLUMN:
		    val->String = "Column within this table";
		    break;
		case SYBD_T_ROW:
		    if (!inf->TData->RowAnnotExpr)
		        {
			val->String = "";
			break;
			}
		    expModifyParam(inf->TData->ObjList, NULL, inf->Obj);
		    inf->TData->ObjList->Session = inf->Obj->Session;
		    expEvalTree(inf->TData->RowAnnotExpr, inf->TData->ObjList);
		    if (inf->TData->RowAnnotExpr->Flags & EXPR_F_NULL ||
		        inf->TData->RowAnnotExpr->String == NULL)
			{
			val->String = "";
			}
		    else
		        {
			val->String = inf->TData->RowAnnotExpr->String;
			}
		    break;
		}
	    return 0;
	    }

	/** If Attr is content-type, report the type. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE: val->String = "system/void"; break;
		case SYBD_T_TABLE: val->String = "system/void"; break;
		case SYBD_T_ROWSOBJ: val->String = "system/void"; break;
		case SYBD_T_COLSOBJ: val->String = "system/void"; break;
		case SYBD_T_ROW: 
		    {
		    if (inf->TData->HasContent)
		        val->String = "application/octet-stream";
		    else
		        val->String = "system/void";
		    break;
		    }
		case SYBD_T_COLUMN: val->String = "system/void"; break;
		}
	    return 0;
	    }

	/** Outer type... **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE: val->String = "application/sybase"; break;
		case SYBD_T_TABLE: val->String = "system/table"; break;
		case SYBD_T_ROWSOBJ: val->String = "system/table-rows"; break;
		case SYBD_T_COLSOBJ: val->String = "system/table-columns"; break;
		case SYBD_T_ROW: val->String = "system/row"; break;
		case SYBD_T_COLUMN: val->String = "system/column"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == SYBD_T_COLUMN)
	    {
	    if (strcmp(attrname,"datatype")) return -1;
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}

	    /** Get the table info. **/
	    tdata=inf->TData;

	    /** Search table info for this column. **/
	    i = sybd_internal_ColNameToID(tdata, inf->RowColPtr);
	    if (i >= 0)
	        {
		val->String = inf->Node->Types[tdata->ColTypes[i]];
		return 0;
		}
	    }
	else if (inf->Type == SYBD_T_ROW)
	    {
	    /** Bypass system names? **/
	    if (!strncmp(attrname, "__cx_literal_", 13))
		attrname = attrname + 13;

	    /** Get the table info. **/
	    tdata = inf->TData;

	    /** Search through the columns. **/
	    i = sybd_internal_ColNameToID(tdata, attrname);
	    if (i >= 0)
	        {
		ptr = inf->ColPtrs[i];
		t = tdata->ColTypes[i];

		if (datatype == DATA_T_DATETIME) val->DateTime = &(inf->Types.Date);
		else if (datatype == DATA_T_MONEY) val->Money = &(inf->Types.Money);
		if ((rval = sybd_internal_GetCxValue(ptr, t, val, datatype)) < 0)
		    {
		    mssError(0, "SYBD", "Couldn't get value for %s.", attrname);
		    return -1;
		    }

		return rval;
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
sybdSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    int type;
    CS_COMMAND* cmd;
    pSybdConn conn;
    char sbuf[320];
    pXString xs;
    char* ptr;
    CS_INT restype = 0;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Type == SYBD_T_DATABASE) return -1;
	    }

	/** Changing the 'annotation'? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"SYBD","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** Choose the appropriate action based on object type **/
	    switch(inf->Type)
	        {
		case SYBD_T_DATABASE:
		    memccpy(inf->Node->Description, val?(val->String):"", '\0', 255);
		    inf->Node->Description[255] = 0;
		    /**
		    objParamsSet(inf->Node->Params, "description", val->String, 0);
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
		    memccpy(inf->TData->Annotation, val?(val->String):"", '\0', 255);
		    inf->TData->Annotation[255] = 0;
		    while(strchr(inf->TData->Annotation,'"')) *(strchr(inf->TData->Annotation,'"')) = '\'';
		    if (inf->Node->AnnotTable[0])
		        {
		        /** Get a database server connection. **/
		        conn = inf->SessionID;
		        if (!conn) conn=sybd_internal_GetConn(inf->Node);
		        if (!conn) return -1;

			/** Build the SQL to update the annotation table **/
			xs = xsNew();
			if (!xs)
			    {
			    sybd_internal_ReleaseConn(inf->Node, conn);
			    return -1;
			    }
			xsPrintf(xs, "UPDATE %s set b = \"%s\" WHERE a = '%s'", inf->Node->AnnotTable,
				inf->TData->Annotation, inf->TData->Table);
			cmd = sybd_internal_Exec(conn, xs->String);
			if (cmd)
			    {
		    	    while(ct_results(cmd, (CS_INT*)&restype) == CS_SUCCEED);
			    sybd_internal_Close(cmd);
			    }
		        if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node, conn);
			xsFree(xs);
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
		if (datatype != DATA_T_INTEGER)
		    {
		    mssError(1,"SYBD","Type mismatch setting attribute '%s' (should be integer)", attrname);
		    return -1;
		    }
		if (!val)
		    {
		    mssError(1,"SYBD","Size attribute cannot be NULL");
		    return -1;
		    }
		inf->Size = val->Integer;
		}
	    else
	        {
	        /** Otherwise, check Oxt. **/
	        if (*oxt)
	            {
		    /** We're within a transaction.  Fill in the oxt. **/
		    type = sybdGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    if (datatype != type)
			{
			mssError(1,"SYBD","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
				attrname, obj_type_names[datatype], obj_type_names[type]);
			return -1;
			}
		    (*oxt)->AllocObj = 0;
		    (*oxt)->Object = NULL;
		    (*oxt)->Status = OXT_S_VISITED;
		    if (strlen(attrname) >= 64)
			{
			mssError(1,"SYBD","Attribute name '%s' too long",attrname);
			return -1;
			}
		    strcpy((*oxt)->AttrName, attrname);
		    obj_internal_SetTreeAttr(*oxt, type, val);
		    }
	        else
	            {
		    /** Get a db server connection. **/
		    conn = inf->SessionID;
		    if (!conn) conn=sybd_internal_GetConn(inf->Node);
		    if (!conn) return -1;

		    /** Bypass system names? **/
		    if (!strncmp(attrname, "__cx_literal_", 13))
			attrname = attrname + 13;

		    /** No transaction.  Simply do an update. **/
		    type = sybdGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    if (datatype != type)
			{
			mssError(1,"SYBD","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
				attrname, obj_type_names[datatype], obj_type_names[type]);
			if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
			return -1;
			}
		    ptr = sybd_internal_FilenameToKey(inf->Node, conn,inf->TablePtr,inf->RowColPtr);
		    if (!ptr)
			{
			if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
			return -1;
			}
		    xs = xsNew();
		    if (!xs)
			{
			if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
			return -1;
			}
		    if (!val)
			{
			/** Handle NULLs **/
	                xsPrintf(xs,"UPDATE %s SET %s=NULL WHERE %s",inf->TablePtr, attrname, ptr);
			}
		    else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
		        {
	                xsPrintf(xs,"UPDATE %s SET %s=%s WHERE %s",inf->TablePtr,
	                    attrname,objDataToStringTmp(type,val,DATA_F_QUOTED | DATA_F_SYBQUOTE), ptr);
			}
		    else if (type == DATA_T_STRING)
		        {   /** objDataToString quotes strings **/
	                xsPrintf(xs,"UPDATE %s SET %s=%s WHERE %s",inf->TablePtr, attrname,
			    objDataToStringTmp(type,*(void**)val,DATA_F_QUOTED | DATA_F_SYBQUOTE), ptr);
			}
		    else if (type == DATA_T_MONEY)
		        {
	                xsPrintf(xs,"UPDATE %s SET %s=%s WHERE %s",inf->TablePtr, attrname,
			    objFormatMoneyTmp(*(void**)val, "0.0000"), ptr);
			}
		    else if (type == DATA_T_DATETIME)
			{
	                xsPrintf(xs,"UPDATE %s SET %s=\"%s\" WHERE %s",inf->TablePtr, attrname,
			    objFormatDateTmp(*(void**)val, obj_default_date_fmt), ptr);
			}

		    /** Start the update. **/
		    cmd = sybd_internal_Exec(conn, xs->String);
		    if (!cmd) 
		        {
			xsFree(xs);
			if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
			mssError(1,"SYBD","Could not execute SQL to update attribute value");
			return -1;
			}

		    /** Read the results **/
		    while(ct_results(cmd, (CS_INT*)&restype) == CS_SUCCEED);
		    sybd_internal_Close(cmd);
		    xsFree(xs);

		    /** Re-read the row from the db since it has changed, and we
		     ** need to give feedback to the user on what other effects
		     ** the update operation may have had.
		     **/
		    if (sybd_internal_LookupRow(conn, inf) <= 0)
			{
			if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
			mssError(1,"SYBD","Could not retrieve updated record");
			return -1;
			}

		    /** Release the session **/
		    if (!inf->SessionID) sybd_internal_ReleaseConn(inf->Node,conn);
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
sybdAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
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
sybdExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree* oxt)
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
mqsybAnalyze(pQueryStatement stmt)
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
	while((select_qs = mq_internal_FindItem(stmt->QTree, MQ_T_SELECTCLAUSE, select_qs)) != NULL)
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
	    from_qs = mq_internal_FindItem(stmt->QTree, MQ_T_FROMCLAUSE, NULL);
	    if (!from_qs) continue;
	    for(i=0;i<from_qs->Children.nItems;i++)
	        {
		item = (pQueryStructure)(from_qs->Children.Items[i]);
		from_tmpobjs[i] = objOpen(stmt->Query->SessionID, item->Source, O_RDONLY, 0600, "system/directory");
		if (from_tmpobjs[i]->Driver == SYBD_INF.ObjDriver ||
		    (from_tmpobjs[i]->Driver == OSYS.TransLayer && from_tmpobjs[i]->TLowLevelDriver == SYBD_INF.ObjDriver))
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
mqsybStart(pQueryElement qe, pQueryStatement stmt, pExpression additional_expr)
    {
    return 0;
    }


/*** mqsybNextItem - Retrieve the next item in the query result set.  Return:
 ***   1 == valid row fetched.
 ***   0 == end of result set; no row fetched.
 ***  -1 == error occurred.
 ***/
int
mqsybNextItem(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqsybFinish - End the current query.
 ***/
int
mqsybFinish(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }


/*** mqsybRelease - release any private data areas that were allocated by the
 *** Analyze routine.
 ***/
int
mqsybRelease(pQueryElement qe, pQueryStatement stmt)
    {
    return 0;
    }

/*** sybdInfo - Return the capabilities of the object
 ***/
int
sybdInfo(void* inf_v, pObjectInfo info)
    {
    pSybdData inf = SYBD(inf_v);

	info->Flags |= ( OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK );
	switch (inf->Type)
	    {
	    case SYBD_T_DATABASE:
	    case SYBD_T_TABLE:
	    case SYBD_T_COLSOBJ:
	    case SYBD_T_ROWSOBJ:
		info->Flags |= ( OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_HAVE_CONTENT |
		    OBJ_INFO_F_NO_CONTENT );
		if (inf->TData && inf->TData->RowCount > 0) 
		    {
		    info->Flags |= ( OBJ_INFO_F_HAS_SUBOBJ | OBJ_INFO_F_SUBOBJ_CNT_KNOWN );
		    info->nSubobjects = inf->TData->RowCount;
		    }
		break;
	    case SYBD_T_COLUMN:
		info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CANT_HAVE_CONTENT |
		    OBJ_INFO_F_NO_CONTENT );
		break;
	    case SYBD_T_ROW:
		info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ );
		if (inf->TData && inf->TData->HasContent) info->Flags |= OBJ_INFO_F_CAN_HAVE_CONTENT;
		break;
	    }
	return 0;
    }


/*** datPresentationHints - return a presentation-hints structure
 *** dcontaining information about a particular attribute.
 ***/
pObjPresentationHints
sybdPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pSybdData inf = SYBD(inf_v);
    pObjPresentationHints hints=NULL;
    int datatype, i;

	if (!strcmp(attrname, "name") || !strcmp(attrname, "inner_type") || !strcmp(attrname, "outer_type") ||
	    !strcmp(attrname, "content_type") || !strcmp(attrname, "annotation") || !strcmp(attrname, "last_modification"))
	    {
	    if ( (hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
	    memset(hints, 0, sizeof(ObjPresentationHints));
	    xaInit(&(hints->EnumList), 8);
	    hints->GroupID=-1;
	    hints->VisualLength2=1;
	    if (!strcmp(attrname, "annotation")) hints->VisualLength = 60;
	    else hints->VisualLength = 30;
	    if (!strcmp(attrname, "name")) hints->Length = 30;
	    else if (!strcmp(attrname, "annotation")) hints->Length = 255;
	    else 
		{
		hints->Style |= OBJ_PH_STYLE_READONLY;
		hints->StyleMask |= OBJ_PH_STYLE_READONLY;
		}
	    return hints;
	    }
        
	switch(inf->Type)
	    {
	    case SYBD_T_DATABASE: break;
	    case SYBD_T_TABLE: break;
	    case SYBD_T_ROWSOBJ: break;
	    case SYBD_T_COLSOBJ: break;
	    case SYBD_T_COLUMN:
		if (strcmp(attrname, "datatype"))
		    {
		    mssError(1, "SYBD", "No attribute %s", attrname);
		    return NULL;
		    }

		if ( (hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
		xaInit(&(hints->EnumList), 8);
		memset(hints, 0, sizeof(ObjPresentationHints));
		hints->GroupID=-1;
		hints->VisualLength2=1;
		hints->Style |= OBJ_PH_STYLE_READONLY;
		hints->StyleMask |= OBJ_PH_STYLE_READONLY;
		hints->VisualLength = 15;

		return hints;
		break;
	    case SYBD_T_ROW:
		/** the attributes of a row are the column names, with the values being the field values **/

		/** Bypass system names? **/
		if (!strncmp(attrname, "__cx_literal_", 13))
		    attrname = attrname + 13;

		/** find the name of the column, and get its data type **/
		i = sybd_internal_ColNameToID(inf->TData, attrname);
		if (i >= 0)
		    {
		    datatype = inf->TData->ColTypes[i];
		    
		    /** Duplicate the hints structure **/
		    hints=objDuplicateHints(inf->Node->TypeHints[datatype]);

		    /** TODO: pull stuff out of the node to flesh the structure out **/
		    
		    /** If this is a stringish datatype, set its length and visual length.
		     ** It's important to make sure we don't override something the user
		     ** might've set.
		     ** Note - don't set length for a text or image type.
		     **/
		    if (sybd_internal_GetCxType(datatype) == DATA_T_STRING && datatype != 19 && datatype != 20)
			{
			if (hints->Length == 0) hints->Length = inf->TData->ColLengths[i];
			if (hints->VisualLength == 0) hints->VisualLength = inf->TData->ColLengths[i];
			}

		    /** Set the KEY style flag if the field is a part of the primary key **/
		    if (inf->TData->ColFlags[i] & SYBD_CF_PRIKEY)
			hints->Style |= OBJ_PH_STYLE_KEY;
		    hints->StyleMask |= OBJ_PH_STYLE_KEY;
		    }
		else
		    {
		    mssError(1, "SYBD", "No attribute '%s'", attrname);
		    return NULL;
		    }
		break;
	    default:
		mssError(1, "SYBD", "Can't get hints for that type yet");
		break;
	    }
        return hints;
    }

/*** sybdInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
sybdInitialize()
    {
    pObjDriver drv;
    int maxconn;
#if 00
    pQueryDriver qdrv;
#endif

#ifndef HAVE_CTYPE_B
	__ctype_b = *(__ctype_b_loc());
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
	if (SYBD_SHOW_SQL)
	    SYBD_INF.SqlLog = fdOpen("/var/log/centrallix-sybase.log", O_RDWR | O_APPEND | O_CREAT, 0600);

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
	drv->DeleteObj = sybdDeleteObj;
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
	drv->Commit = sybdCommit;
	drv->Info = sybdInfo;
	drv->PresentationHints = sybdPresentationHints;

	/** Initialize CT Library **/
	if (cs_ctx_alloc(CS_VERSION_100, &SYBD_INF.Context) != CS_SUCCEED) return -1;
	if (ct_init(SYBD_INF.Context, CS_VERSION_100) != CS_SUCCEED) return -1;
	maxconn = SYBD_MAX_CONNECTIONS;
	if (ct_config(SYBD_INF.Context, CS_SET, CS_MAX_CONNECT, &maxconn, CS_UNUSED, NULL) == CS_FAIL)
	    {
	    maxconn = 256;
	    if (ct_config(SYBD_INF.Context, CS_SET, CS_MAX_CONNECT, &maxconn, CS_UNUSED, NULL) == CS_FAIL)
		{
		mssError(1, "SYBD", "Warning: could not increase connection limit to %d", SYBD_MAX_CONNECTIONS);
		}
	    else
		{
		mssError(1, "SYBD", "Warning: could only increase connection limit to %d instead of %d", maxconn, SYBD_MAX_CONNECTIONS);
		}
	    }

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


MODULE_INIT(sybdInitialize);
MODULE_PREFIX("sybd");
MODULE_DESC("Sybase ObjectSystem Driver");
MODULE_VERSION(0,9,0);
MODULE_IFACE(CX_CURRENT_IFACE);
