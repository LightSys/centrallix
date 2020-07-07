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
#include "hints.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxlib/util.h"
#include <assert.h>
#include <mysql.h>

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Core                                                       */
/*                                                                         */
/* Copyright (C) 1998-2008 LightSys Technology Services, Inc.                */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify        */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation; either version 2 of the License, or        */
/* (at your option) any later version.                                        */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                                */
/*                                                                         */
/* You should have received a copy of the GNU General Public License        */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA                  */
/* 02111-1307  USA                                                        */
/*                                                                        */
/* A copy of the GNU General Public License has been included in this        */
/* distribution in the file "COPYING".                                        */
/*                                                                         */
/* Module:         objdrv_mysql.c                                                 */
/* Author:        Greg Beeley (GB)                                               */
/* Creation:        February 21, 2008                                              */
/* Description:        A MySQL driver for Centrallix.  Eventually this driver        */
/*                should be merged with the Sybase driver in something of        */
/*                an intelligent manner.                                    */
/************************************************************************/


#define MYSD_MAX_COLS                256
#define MYSD_MAX_KEYS                8
#define MYSD_NAME_LEN                32
#define MYSD_MAX_CONN                16

#define MYSD_NODE_F_USECXAUTH	1	/* use Centrallix usernames/passwords */
#define MYSD_NODE_F_SETCXAUTH	2	/* try to change empty passwords to Centrallix login passwords */

/*** Node ***/
typedef struct
    {
    char        Path[OBJSYS_MAX_PATH];
    char        Server[64];
    char        Username[64];
    char        Password[64];
    char        DefaultPassword[64];
    char        Database[MYSD_NAME_LEN];
    char        AnnotTable[MYSD_NAME_LEN];
    char        Description[256];
    int                MaxConn;
    int         Flags;
    pSnNode        SnNode;
    XArray        Conns;
    int                ConnAccessCnt;
    XHashTable        Tables;
    XArray        Tablenames;
    int                LastAccess;
    }
    MysdNode, *pMysdNode;


/*** Connection data ***/
typedef struct
    {
    MYSQL        Handle;
    char        Username[64];
    char        Password[64];
    pMysdNode        Node;
    int                Busy;
    int                LastAccess;
    }
    MysdConn, *pMysdConn;


/*** Table data ***/
typedef struct
    {
    char        Name[MYSD_NAME_LEN];
    unsigned char ColFlags[MYSD_MAX_COLS];
    unsigned char ColCxTypes[MYSD_MAX_COLS];
    unsigned char ColKeys[MYSD_MAX_COLS];
    char*        Cols[MYSD_MAX_COLS];
    char*        ColTypes[MYSD_MAX_COLS];
    unsigned int ColLengths[MYSD_MAX_COLS];
    pObjPresentationHints ColHints[MYSD_MAX_COLS];
    int                nCols;
    char*        Keys[MYSD_MAX_KEYS];
    int                KeyCols[MYSD_MAX_KEYS];
    int                nKeys;
    pMysdNode        Node;
    char*		Annotation;
    pParamObjects	ObjList;
    pExpression		RowAnnotExpr;
    }
    MysdTable, *pMysdTable;

#define MYSD_COL_F_NULL         1       /* column allows nulls */
#define MYSD_COL_F_PRIKEY       2       /* column is part of primary key */
#define MYSD_COL_F_UNSIGNED     4       /* Flag for unsigned on integer fields */


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    Pathname        Pathname;
    char*	    Name;
    int                Type;
    int                Flags;
    pObject        Obj;
    int                Mask;
    int                CurAttr;
    pMysdNode        Node;
    char*        RowBuf;
    MYSQL_ROW        Row;
    MYSQL_RES *     Result;
    char*        ColPtrs[MYSD_MAX_COLS];
    unsigned short ColLengths[MYSD_MAX_COLS];
    pMysdTable        TData;
    union
        {
        DateTime	Date;
        MoneyType	Money;
        IntVec		IV;
        StringVec	SV;
        } Types;
    char	Objname[256];
    }
    MysdData, *pMysdData;

#define MYSD_T_DATABASE        1
#define MYSD_T_TABLE                2
#define MYSD_T_COLSOBJ        3
#define MYSD_T_ROWSOBJ        4
#define MYSD_T_COLUMN                5
#define MYSD_T_ROW                6


#define MYSD(x) ((pMysdData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pMysdData        Data;
    char                  NameBuf[256];
    XString            Clause;
    int                ItemCnt;
    }
    MysdQuery, *pMysdQuery;


/*** GLOBALS ***/
struct
    {
    XHashTable              DBNodesByPath;
    XArray                      DBNodeList;
    int                             AccessCnt;
    int                             LastAccess;
    }
    MYSD_INF;

MYSQL_RES* mysd_internal_RunQuery(pMysdNode node, char* stmt, ...);
MYSQL_RES* mysd_internal_RunQuery_conn(pMysdConn conn, pMysdNode node, char* stmt, ...);
MYSQL_RES* mysd_internal_RunQuery_conn_va(pMysdConn conn, pMysdNode node, char* stmt, va_list ap);

/** This value is returned if the query failed.  If NULL is returned from
 *  ** the RunQuery functions, it means "no result set" but success.  It is
 *   ** equal to 0xFFFFFFFF on 32-bit platforms, and 0xFFFFFFFFFFFFFFFFLL on
 *    ** 64-bit platforms.
 *     **/
#define MYSD_RUNQUERY_ERROR	((MYSQL_RES*)~((long)0))

/*** mysd_internal_GetConn() - given a specific database node, get a connection
 *  *** to the server with a given login (from mss thread structures).
 *   ***/
pMysdConn
mysd_internal_GetConn(pMysdNode node)
    {
    MYSQL_RES * result;
    pMysdConn conn;
    int i, conn_cnt, found;
    int min_access;
    char* username;
    char* password;

        /** Is one available from the node's connection pool? **/
        conn_cnt = xaCount(&node->Conns);
        
        /** Use system auth? **/
        if (node->Flags & MYSD_NODE_F_USECXAUTH)
            {
            /** Do we have permission to do this? **/
            if (!(CxGlobals.Flags & CX_F_ENABLEREMOTEPW))
		{
		mssError(1,"MYSD","use_system_auth requested, but Centrallix global enable_send_credentials is turned off");
		return NULL;
		}
        
            /** Get usernamename/password from session **/
            username = mssUserName();
            password = mssPassword();
        
            if(!username || !password)
		{
		mssError(1,"MYSD","Connect to database: username and/or password not supplied");
		return NULL;
		}
            }
        else
            {
            /** Use usernamename/password from node **/
            username = node->Username;
            password = node->Password;
            }
        
        conn = NULL;
        for(i=0;i<conn_cnt;i++)
            {
            conn = (pMysdConn)xaGetItem(&node->Conns, i);
            if (!conn->Busy && !strcmp(username, conn->Username) && !strcmp(password, conn->Password))
		{
		if (mysql_ping(&conn->Handle) != 0)
		    {
		    /** Got disconnected.  Discard the connection. **/
		    mysql_close(&conn->Handle);
		    memset(conn->Password, 0, sizeof(conn->Password));
		    xaRemoveItem(&node->Conns, i);
		    nmFree(conn, sizeof(MysdConn));
		    i--;
		    conn_cnt--;
		    conn = NULL;
		    continue;
		    }
                break;
		}
            conn = NULL;
            }

        /** Didn't get one? **/
        if (!conn)
            {
            if (conn_cnt < node->MaxConn)
                {
                /** Below pool maximum?  Alloc if so **/
                conn = (pMysdConn)nmMalloc(sizeof(MysdConn));
                if (!conn)
		    {
		    mssError(0,"MYSD","Could not connect to database server");
		    return NULL;
		    }
                conn->Node = node;
                }
            else
                {
                /** Try to free and reuse a connection **/
                min_access = 0x7fffffff;
                found = -1;
                for(i=0;i<conn_cnt;i++)
                    {
                    conn = (pMysdConn)xaGetItem(&node->Conns, i);
                    if (!conn->Busy && conn->LastAccess < min_access)
                        {
                        min_access = conn->LastAccess;
                        found = i;
                        }
                    }
                if (found < 0)
                    {
                    mssError(1, "MYSD", "Connection limit (%d) reached for server [%s]",
                            node->MaxConn, node->Server);
                    return NULL;
                    }
		conn = (pMysdConn)xaGetItem(&node->Conns, found);
                mysql_close(&conn->Handle);
                memset(conn->Password, 0, sizeof(conn->Password));
                xaRemoveItem(&node->Conns, found);
                nmFree(conn, sizeof(MysdConn));
                }

            /** Attempt connection **/
            if (mysql_init(&conn->Handle) == NULL)
                {
                mssError(1, "MYSD", "Memory exhausted");
                nmFree(conn, sizeof(MysdConn));
                return NULL;
                }
            if (mysql_real_connect(&conn->Handle, node->Server, username, password, node->Database, 0, NULL, 0) == NULL)
                {
		if (node->Flags & MYSD_NODE_F_SETCXAUTH)
		    {
		    if (mysql_real_connect(&conn->Handle, node->Server, username, node->DefaultPassword, node->Database, 0, NULL, 0) == NULL)
			{
			mssError(1, "MYSD", "Could not connect to MySQL server [%s], DB [%s]: %s",
				node->Server, node->Database, mysql_error(&conn->Handle));
			mysql_close(&conn->Handle);
			nmFree(conn, sizeof(MysdConn));
			return NULL;
			}

		    /** Successfully connected using user default password.
 * 		     ** Now try to change the password.
 * 		     		     **/
		    result = mysd_internal_RunQuery_conn(conn, node, "SET PASSWORD = PASSWORD('?')", password);

		    if (result == MYSD_RUNQUERY_ERROR)
			mssError(1, "MYSD", "Warning: could not update password for user [%s]: %s",
				username, mysql_error(&conn->Handle));
		    }
		else
		    {
		    mssError(1, "MYSD", "Could not connect to MySQL server [%s], DB [%s]: %s",
			    node->Server, node->Database, mysql_error(&conn->Handle));
		    mysql_close(&conn->Handle);
		    nmFree(conn, sizeof(MysdConn));
		    return NULL;
		    }
                }

            /** Success! **/
            strtcpy(conn->Username, username, sizeof(conn->Username));
            strtcpy(conn->Password, password, sizeof(conn->Password));
            xaAddItem(&node->Conns, conn);
            }

        /** Make it busy **/
        conn->Busy = 1;
        conn->LastAccess = (node->ConnAccessCnt++);

    return conn;
    }


/*** mysd_internal_ReleaseConn() - release a connection back to the connection
 *  *** pool.  Also sets the pointer to NULL.
 *   ***/
void
mysd_internal_ReleaseConn(pMysdConn * conn)
    {

        /** Release it **/
        assert((*conn)->Busy);
        (*conn)->Busy = 0;
        (*conn) = NULL;

    return;
    }

/*** mysd_internal_RunQuery() - safely runs a query on the database
 * *** This works simarly to prepared statments using the question mark syntax.
 * *** Instead of calling bindParam for each argument as is done in general,
 * *** all of the arguments to be replaced are passed as varargs
 * ***
 * *** There are some differences:
 * *** ?d => decimal. inserts using sprintf("%d",int)
 * *** ?v => quoted or nonquoted, params are (char* str, int add_quote), see ?a.
 * *** ?q => used for query clauses.
 * ***     same as ? except without escaping
 * ***     only use if you know the input is clean and is already a query segment
 * *** ?a => array, params for this are (char** array, char add_quote[], int length, char separator
 * ***     it will become: item1,item2,items3 (length = 3, separator = ',')
 * *** '?a' => same as array except with individual quoting specified
 * ***     it will become: 'item1','item2','items3' (length = 3, separator = ',')
 * *** ?n => transform name of object into criteria (field1 = 'value1' and field2 = 'value2')
 * *** add_quote can be left NULL, in which case nothing gets quoted with ?a.
 * *** (add_quote has no effect on '?a' forms).  Otherwise, a non-'\0' value
 * *** for add_quote[x] means to add single quotes '' onto the value.
 * ***
 * *** All parameters are sanatized with mysql_real_escape_string before
 * *** the query is built
 * *** This WILL NOT WORK WITH BINARY DATA
 * ***/
MYSQL_RES*
mysd_internal_RunQuery_conn_va(pMysdConn conn, pMysdNode node, char* stmt, va_list ap)
    {
    MYSQL_RES * result = MYSD_RUNQUERY_ERROR;
    XString query;
    int i, j;
    int length = 0;
    char * start;
    char ** array = NULL;
    int items = 0;
    char separator;
    char quote = 0x00;
    char tmp[32];
    char * add_quote;
    char * str;
    char * endstr;
    int err;
    char * errtxt;
    char ch;
    pMysdData data;

        xsInit(&query);

        length=-1;
        start=stmt;
        for(i = 0; stmt[i]; i++)
            {
            length++;
            if(stmt[i] == '?')
                {
                /** throw on everything new that is just constant **/
                if(xsConcatenate(&query,start,length)) goto error;
                /** do the insertion **/
                if(stmt[i+1]=='a') /** handle arrays **/
                    {
                    array = va_arg(ap,char**);
                    add_quote = va_arg(ap,char*);
                    items = va_arg(ap,int);
                    separator = va_arg(ap,int);
                    if(stmt[i+2] == '\'' || stmt[i+2]=='`') quote = stmt[i+2]; else quote = 0x00;
                    for(j = 0; j < items; j++)
                        {
                        if(j > 0) 
                            {
                            if(quote) if(xsConcatenate(&query,&quote,1)) goto error;
                            if(xsConcatenate(&query,&separator,1)) goto error;
                            if(quote) if(xsConcatenate(&query,&quote,1)) goto error;
                            }
			if (!array[j])
			    {
			    xsConcatenate(&query, "null", 4);
			    }
			else
			    {
			    if (!quote && add_quote && add_quote[j])
				xsConcatenate(&query, "'", 1);
			    if(mysd_internal_SafeAppend(&conn->Handle,&query,array[j])) goto error;
			    if (!quote && add_quote && add_quote[j])
				xsConcatenate(&query, "'", 1);
			    }
                        }
                    start = &stmt[i+2];
                    i++;
                    }
		else if(stmt[i+1]=='v') /** quoted / nonquoted **/
		    {
			str = va_arg(ap,char*);
			quote = va_arg(ap,int);
			if (!str)
			    {
			    xsConcatenate(&query, "null", 4);
			    }
			else
			    {
			    if (quote)
				xsConcatenate(&query, "'", 1);
			    if(mysd_internal_SafeAppend(&conn->Handle,&query,str)) goto error;
			    if (quote)
				xsConcatenate(&query, "'", 1);
			    }
			start = &stmt[i+2];
			i++;
		    }
                else if(stmt[i+1]=='d') /** handle integers **/
                    {
                        sprintf(tmp,"%d",va_arg(ap,int));
                        if(mysd_internal_SafeAppend(&conn->Handle,&query,tmp)) goto error;
                        start = &stmt[i+2];
                        i++;
                    }                
                else if(stmt[i+1]=='q') /** handle pre-sanitized query sections **/
                    {
                        xsConcatenate(&query,va_arg(ap,char*),-1);
                        start = &stmt[i+2];
                        i++;
                    }
		else if(stmt[i+1]=='n') /** object name criteria - expect pMysdData **/
		    {
			data = va_arg(ap, pMysdData);
			xsConcatenate(&query, " (", 2);
			str = data->Name;
			for(j=0; j<data->TData->nKeys; j++)
			    {
			    endstr = strchr(str, '|');
			    if (!endstr)
				endstr = str + strlen(str);
			    if (j != 0)
				xsConcatenate(&query, " and ", 5);
			    xsConcatenate(&query, "`", 1);
			    if (mysd_internal_SafeAppend(&conn->Handle, &query, data->TData->Keys[j])) goto error;
			    xsConcatenate(&query, "` = '", 5);
			    ch = *endstr;
			    *endstr = '\0';
			    if (mysd_internal_SafeAppend(&conn->Handle, &query, str))
				{
				*endstr = ch;
				goto error;
				}
			    *endstr = ch;
			    xsConcatenate(&query, "'", 1);
			    str = (*endstr)?(endstr+1):(endstr);
			    }
			xsConcatenate(&query, ") ", 2);
                        start = &stmt[i+2];
                        i++;
		    }
                else /** handle plain sanitize+insert **/
                    {
                    if(mysd_internal_SafeAppend(&conn->Handle,&query,va_arg(ap,char*))) goto error;
                    start = &stmt[i+1];
                    }
                length = -1;
                }
            }
        /** insert the last constant bit **/
        if(xsConcatenate(&query,start,-1)) goto error;

        /** If you want to do something with all the DB queries
 *          ** this is the place
 *                   **/ 
        /* printf("TEST: query=\"%s\"\n",query.String); */

        if(mysql_query(&conn->Handle,query.String)) goto error;
        result = mysql_store_result(&conn->Handle);
	err = mysql_errno(&conn->Handle);
	if (err)
	    {
	    errtxt = (char*)mysql_error(&conn->Handle);
	    mssError(1,"MYSD","SQL command failed: %s", errtxt);
	    if (result) mysql_free_result(result);
	    result = MYSD_RUNQUERY_ERROR;
	    if (err == 1022 || err == 1061 || err == 1062)
		errno = EEXIST;
	    else
		errno = EINVAL;
	    }

    error:
	xsDeInit(&query);
	return result;
    }

MYSQL_RES*
mysd_internal_RunQuery_conn(pMysdConn conn, pMysdNode node, char* stmt, ...)
    {
    MYSQL_RES * result = NULL;
    va_list ap;

        va_start(ap,stmt);
	result = mysd_internal_RunQuery_conn_va(conn, node, stmt, ap);
	va_end(ap);

    return result;
    }

MYSQL_RES*
mysd_internal_RunQuery(pMysdNode node, char* stmt, ...)
    {
    MYSQL_RES * result = NULL;
    pMysdConn conn = NULL;
    va_list ap;

        /**start up ap and create query XString **/
        
        va_start(ap,stmt);

        if(!(conn = mysd_internal_GetConn(node)))
	    return NULL;

	result = mysd_internal_RunQuery_conn_va(conn, node, stmt, ap);
       
	mysd_internal_ReleaseConn(&conn);

	va_end(ap);

    return result;
    }


/*** mysd_internal_SafeAppend - appends a string on a query
 *  *** this uses mysql_real_escape_string and requires a connection
 *   ***/
 
int
mysd_internal_SafeAppend(MYSQL* conn, pXString dst, char* src)
    {
        char* escaped_src;
        int length;
        int rval = 0;
        length = strlen(src);
        
        escaped_src = nmMalloc(length*2+1);
        mysql_real_escape_string(conn,escaped_src,src,length);
        if(xsConcatenate(dst,escaped_src,-1)) rval = -1;
        
        nmFree(escaped_src,length*2+1);

    return rval;
    }

/*** mysd_internal_CxDataToMySQL() - convert cx data to mysql field values
 *  ***/
char*
mysd_internal_CxDataToMySQL(int type, pObjData val)
    {
    char* tmp;
    int length;
    int j;

	/** Handle nulls **/
	if (!val)
	    return NULL;

	/** Convert based on data type **/
        if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
            {
            return objDataToStringTmp(type,val,0);
            }
        if (type == DATA_T_DATETIME)
            {
            return (char*)objFormatDateTmp((pDateTime)val,"yyyy-MM-dd HH:mm:ss");
            }
        if (type == DATA_T_MONEY)
            {
            return (char*)objFormatMoneyTmp((pMoneyType)val,"^.####");
            }
        if (type == DATA_T_STRING)
            {
            return objDataToStringTmp(type,val,0);
            }
        return NULL;
    }

