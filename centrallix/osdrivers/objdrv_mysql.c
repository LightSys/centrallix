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
/* Centrallix Core                                                      */
/*                                                                      */
/* Copyright (C) 1998-2008 LightSys Technology Services, Inc.           */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             */
/* 02111-1307  USA                                                      */
/*                                                                      */
/* A copy of the GNU General Public License has been included in this   */
/* distribution in the file "COPYING".                                  */
/*                                                                      */
/* Module:      objdrv_mysql.c					        */
/* Author:      Greg Beeley (GB)                                        */
/* Creation:    February 21, 2008                                       */
/* Description: A MySQL driver for Centrallix.  Eventually this driver  */
/*              should be merged with the Sybase driver in something of */
/*              an intelligent manner.                                  */
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
    char        DatabaseCollation[64];
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
 ** the RunQuery functions, it means "no result set" but success.  It is
 ** equal to 0xFFFFFFFF on 32-bit platforms, and 0xFFFFFFFFFFFFFFFFLL on
 ** 64-bit platforms.
 **/
#define MYSD_RUNQUERY_ERROR	((MYSQL_RES*)~((long)0))

/*** mysd_internal_GetConn() - given a specific database node, get a connection
 *** to the server with a given login (from mss thread structures).
 ***/
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
		     ** Now try to change the password.
		     **/
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

	    /** get character set information **/
	    result = mysd_internal_RunQuery_conn(conn, node, "SELECT DEFAULT_CHARACTER_SET_NAME FROM information_schema.SCHEMATA  WHERE SCHEMA_NAME = '?'", node->Database);
	    if (!result || result == MYSD_RUNQUERY_ERROR)
		{
		/** just set to all NULLs **/
		strcpy(node->DatabaseCollation, "");
		}
	    else
		{
		/** collect the character name **/
		MYSQL_ROW row = mysql_fetch_row(result);
		if(row == 0)
		    {
		    strcpy(node->DatabaseCollation, "");
		    }
		else
		    {
		    // check for exceptions. If none, then "<CHARACTER SET NAME>_bin" will work
		    if(!strcmp(row[0], "binary")) strtcpy(node->DatabaseCollation, row[0], sizeof(node->DatabaseCollation));
		    else if (row[0] == NULL || strlen(row[0]) == 0) strcpy(node->DatabaseCollation, "");
		    else 
			{
			strtcpy(node->DatabaseCollation, row[0], sizeof(node->DatabaseCollation) - 4);
			strncat(node->DatabaseCollation, "_bin", 5); // will always fit
			}
		    }
		mysql_free_result(result);
		}
            }
	
        /** Make it busy **/
        conn->Busy = 1;
        conn->LastAccess = (node->ConnAccessCnt++);

    return conn;
    }


/*** mysd_internal_ReleaseConn() - release a connection back to the connection
 *** pool.  Also sets the pointer to NULL.
 ***/
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
*** This works simarly to prepared statments using the question mark syntax.
*** Instead of calling bindParam for each argument as is done in general,
*** all of the arguments to be replaced are passed as varargs
***
*** There are some differences:
*** ?d => decimal. inserts using sprintf("%d",int)
*** ?v => quoted or nonquoted, params are (char* str, int add_quote), see ?a.
*** ?q => used for query clauses.
***     same as ? except without escaping
***     only use if you know the input is clean and is already a query segment
*** ?a => array, params for this are (char** array, char add_quote[], int length, char separator
***     it will become: item1,item2,items3 (length = 3, separator = ',')
*** '?a' => same as array except with individual quoting specified
***     it will become: 'item1','item2','items3' (length = 3, separator = ',')
*** ?n => transform name of object into criteria (field1 = 'value1' and field2 = 'value2')
*** add_quote can be left NULL, in which case nothing gets quoted with ?a.
*** (add_quote has no effect on '?a' forms).  Otherwise, a non-'\0' value
*** for add_quote[x] means to add single quotes '' onto the value.
***
*** All parameters are sanatized with mysql_real_escape_string before
*** the query is built
*** This WILL NOT WORK WITH BINARY DATA
***/
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
         ** this is the place
         **/ 
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
 *** this uses mysql_real_escape_string and requires a connection
 ***/
 
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
 ***/
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

/*** mysd_internal_ParseTData() - given a mysql result set, parse the table
 *** data into a MysdTable structure.  Returns < 0 on failure.
 ***/
int
mysd_internal_ParseTData(MYSQL_RES *resultset, int rowcnt, pMysdTable tdata)
    {
    int i;
    MYSQL_ROW row;
    char data_desc[32];
    char* pptr;
    char* cptr;
    int len;
    char* pos;

        if (rowcnt > MYSD_MAX_COLS)
            {
            mssError(1,"MYSD","Too many columns (%d) in table [%s]", rowcnt, tdata->Name);
            return -1;
            }

        /** Loop through the result set with the column descriptions **/
        tdata->nCols = tdata->nKeys = 0;
        for(i=0;i<rowcnt;i++)
            {
            row = mysql_fetch_row(resultset);
            if (!row) break;

            /** Name **/
            tdata->Cols[tdata->nCols] = nmSysStrdup(row[0]);
            strtcpy(data_desc, row[1], sizeof(data_desc));

            /** Length, if char/varchar **/
            tdata->ColLengths[tdata->nCols] = 0;
            if ((pptr = strchr(data_desc, '(')) != NULL)
                {
                *pptr = '\0';
                tdata->ColLengths[tdata->nCols] = strtoui(pptr+1, NULL, 10);
                if ((cptr = strchr(pptr+1, ',')) != NULL)
                    len = strtoi(cptr+1, NULL, 10);
                }
                
            /** set lengths for various text types **/
            if(!strcmp(data_desc, "tinytext") || !strcmp(data_desc, "tinyblob")) tdata->ColLengths[tdata->nCols] = 0xFF;
            if(!strcmp(data_desc, "text") || !strcmp(data_desc, "blob")) tdata->ColLengths[tdata->nCols] = 0xFFFF;
            if(!strcmp(data_desc, "mediumtext") || !strcmp(data_desc, "mediumblob")) tdata->ColLengths[tdata->nCols] = 0xFFFFFF;
            if(!strcmp(data_desc, "longtext") || !strcmp(data_desc, "longblob")) tdata->ColLengths[tdata->nCols] = 0xFFFFFFFF;

            /** Type **/
            tdata->ColTypes[tdata->nCols] = nmSysStrdup(data_desc);
            if (!strcmp(data_desc, "int") || !strcmp(data_desc, "bigint") || !strcmp(data_desc, "tinyint") || !strcmp(data_desc, "smallint") || !strcmp(data_desc, "mediumint") || !strcmp(data_desc, "bit"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_INTEGER;
            else if (!strcmp(data_desc, "char") || !strcmp(data_desc, "varchar") 
                    || !strcmp(data_desc, "text") || !strcmp(data_desc, "tinytext") || !strcmp(data_desc, "mediumtext") || !strcmp(data_desc, "longtext")
                    || !strcmp(data_desc, "blob") || !strcmp(data_desc, "tinyblob") || !strcmp(data_desc, "mediumblob") || !strcmp(data_desc, "longblob"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_STRING;
            else if (!strcmp(data_desc, "float") || !strcmp(data_desc, "double"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_DOUBLE;
            else if (!strcmp(data_desc, "datetime") || !strcmp(data_desc, "date") || !strcmp(data_desc, "timestamp"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_DATETIME;
            else if (!strcmp(data_desc, "decimal"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_MONEY;
            else
                tdata->ColCxTypes[tdata->nCols] = DATA_T_UNAVAILABLE;

            tdata->ColFlags[i] = 0;
            /** unsigned? **/
            if(row[1] && (pos = strchr(row[1],'u')))
                {
                if(!strcmp(pos,"unsigned")) 
                    tdata->ColFlags[i] |= MYSD_COL_F_UNSIGNED;
                }
            
            /** Allow nulls **/
            if (row[2] && !strcmp(row[2], "YES"))
                tdata->ColFlags[i] |= MYSD_COL_F_NULL;

            /** Primary Key **/
            if (row[3] && !strcmp(row[3], "PRI"))
                {
                if (tdata->nKeys >= MYSD_MAX_KEYS)
                    {
                    mssError(1,"MYSD","Too many key fields in table [%s]",tdata->Name);
                    return -1;
                    }
                tdata->Keys[tdata->nKeys] = tdata->Cols[tdata->nCols];
                tdata->KeyCols[tdata->nKeys] = tdata->nCols;
                tdata->ColKeys[tdata->nCols] = tdata->nKeys;
                tdata->nKeys++;
                tdata->ColFlags[i] |= MYSD_COL_F_PRIKEY;
                }
            tdata->nCols++;
            }

    return 0;
    }

/*** mysd_internal_GetTData() - get a table information structure
 ***/
pMysdTable
mysd_internal_GetTData(pMysdNode node, char* tablename)
    {
    MYSQL_RES * result = NULL;
    MYSQL_ROW row;
    pMysdTable tdata = NULL;
    int length;
    int rowcnt;

        /** Value cached already? **/
        tdata = (pMysdTable)xhLookup(&node->Tables, tablename);
        if (tdata)
            return tdata;

        /** sanatize the table name and build the query**/
        length = strlen(tablename);
        /** this next bit will break any charset not ASCII **/ 
        /** throw an error if some joker tries to throw in a backtick **/
        if(strchr(tablename,'`')) goto error; 
        result = mysd_internal_RunQuery(node, "SHOW COLUMNS FROM `?`",tablename);
        if (!result || result == MYSD_RUNQUERY_ERROR)
            goto error;
        rowcnt = mysql_num_rows(result);
        if (rowcnt <= 0)
            goto error;

        /** Build the TData **/
        tdata = (pMysdTable)nmMalloc(sizeof(MysdTable));
        if (!tdata)
            goto error;
        memset(tdata, 0, sizeof(MysdTable));
        strtcpy(tdata->Name, tablename, sizeof(tdata->Name));
        tdata->Node = node;
        if (mysd_internal_ParseTData(result, rowcnt, tdata) < 0)
            {
            nmFree(tdata, sizeof(MysdTable));
            tdata = NULL;
            }

	/** Get annotation data **/
        if (result && result != MYSD_RUNQUERY_ERROR)
            mysql_free_result(result);
	result = mysd_internal_RunQuery(node, "SELECT a,b,c FROM `?` WHERE a = '?'", node->AnnotTable, tablename);
        if (result && result != MYSD_RUNQUERY_ERROR)
	    {
	    if (mysql_num_rows(result) == 1)
		{
		row = mysql_fetch_row(result);
		if (row)
		    {
		    tdata->Annotation = nmSysStrdup((row[1])?(row[1]):"");
		    tdata->ObjList = expCreateParamList();
		    expAddParamToList(tdata->ObjList, NULL, NULL, 0);
		    tdata->RowAnnotExpr = (pExpression)expCompileExpression((row[2])?(row[2]):"''", tdata->ObjList, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    }
		}
	    }

    error:
        if (result && result != MYSD_RUNQUERY_ERROR)
            mysql_free_result(result);
        if(tdata) 
            {
            xhAdd(&(node->Tables),tdata->Name,(char*)tdata);
            }
    return tdata;
    }


/*** mysd_internal_DupRow() - duplicate a MYSQL_ROW structure
 ***/
MYSQL_ROW
mysd_internal_DupRow(MYSQL_ROW row, MYSQL_RES* result)
    {
    MYSQL_ROW new_row;
    int n_fields;
    int i;

	n_fields = mysql_num_fields(result);
	if (n_fields <= 0) return NULL;

	new_row = nmSysMalloc((n_fields+1) * sizeof(char*));
	if (!new_row) return NULL;

	for(i=0;i<n_fields;i++)
	    {
	    if (row[i])
		new_row[i] = nmSysStrdup(row[i]);
	    else
		new_row[i] = NULL;
	    }
	new_row[n_fields] = (char*)(-1L);

    return new_row;
    }


/*** mysd_internal_FreeRow() - free a MYSQL_ROW structure that we
 *** created.
 ***/
void
mysd_internal_FreeRow(MYSQL_ROW row)
    {
    int i;

	for(i=0;row[i] != (char*)(-1L);i++)
	    {
	    if (row[i]) nmSysFree(row[i]);
	    }
	nmSysFree(row);

    return;
    }


/*** mysd_internal_GetNextRow() - get a given row from the database
 ***/

int
mysd_internal_GetNextRow(char* filename, int filename_size, pMysdQuery qy, pMysdData data, char* tablename)
    {
    MYSQL_RES * result = NULL;
    MYSQL_ROW row = NULL;
    int i = 0;
    int ret = 0;
    int len;
        
        if(!data->Result) /** we haven't executed the query yet **/
            {
            result = mysd_internal_RunQuery(data->Node,"SELECT * FROM `?` ?q",data->TData->Name,qy->Clause.String);
	    if (!result || result == MYSD_RUNQUERY_ERROR)
		return -1;
            data->Result=result;
            }

        filename[0] = 0x00;
	if (data->Row) mysd_internal_FreeRow(data->Row);
	data->Row = NULL;

        if((data->Row = mysql_fetch_row(data->Result)))
            {
	    data->Row = mysd_internal_DupRow(data->Row, data->Result);
            for(len = i = 0; i<data->TData->nKeys; i++)
                {
		len += (strlen(data->Row[data->TData->KeyCols[i]]) + 1);
		if (len >= filename_size)
		    {
		    mssError(1, "MYSD", "Object name too long while retrieving row");
		    return -1;
		    }
                sprintf(filename,"%s%s|",filename,data->Row[data->TData->KeyCols[i]]);
                }
            /** kill the trailing pipe **/
            filename[strlen(filename)-1]=0x00;
            }
        else
            {
            ret = -1;
            }

    return ret;
    }


/*** mysd_internal_GetRowByKey() - get a given row from the database
 *** returns number of rows on success and -1 on error
 ***/
int
mysd_internal_GetRowByKey(char* key, pMysdData data)
    {
    MYSQL_RES * result = NULL;
    MYSQL_ROW row = NULL;
    int i = 0;
    int ret = 0;

        if(!(data->TData)) return -1;

        //result = mysd_internal_RunQuery(data->Node,"SELECT * FROM `?` WHERE CONCAT_WS('|',`?a`)='?' LIMIT 0,1",data->TData->Name,data->TData->Keys,NULL,data->TData->nKeys,',',key);
        result = mysd_internal_RunQuery(data->Node, "SELECT * FROM `?` WHERE ?n LIMIT 0,1",
		data->TData->Name,
		data
		);
	if (!result || result == MYSD_RUNQUERY_ERROR)
	    {
	    result = NULL;
	    ret = -1;
	    }

	if (data->Row) mysd_internal_FreeRow(data->Row);
	data->Row = NULL;

        if(result && (data->Row = mysql_fetch_row(result)))
            {
	    data->Row = mysd_internal_DupRow(data->Row, result);
	    data->Result=result;
	    ret = mysql_num_rows(result);
            }
        else
            {
            if(data->Result) mysql_free_result(data->Result);
            data->Result=NULL;
            ret = 0;
            }

        return ret;
    }


/*** mysd_internal_RefreshRow - reload a row of data from the DB, for
 *** example after an update is performed, to make sure we have the data
 *** in the object.
 ***/
int
mysd_internal_RefreshRow(pMysdData data)
    {
    char* ptr;
    int rval;

	/** Get the object name (primary key) and then retrieve the row again **/
	ptr = obj_internal_PathPart(data->Obj->Pathname, data->Obj->SubPtr+2, 1);
	rval = mysd_internal_GetRowByKey(ptr, data);
	if (data->Result) mysql_free_result(data->Result);
	data->Result = NULL;

    return rval;
    }


/*** mysd_internal_BuildAutoname - figures out how to fill in the required 
 *** information for an autoname-based row insertion, and populates the
 *** "Objname" field in the MysdData structure accordingly.  Looks in the
 *** OXT for a partially (or fully) completed key, and uses an autonumber
 *** type of approach to complete the remainder of the key.  Returns nonnegative
 *** on success and with the autoname semaphore held.  Returns negative on error
 *** with the autoname semaphore NOT held.  On success, the semaphore must be
 *** released after the successful insertion of the new row.
 ***/
int
mysd_internal_BuildAutoname(pMysdData inf, pMysdConn conn, pObjTrxTree oxt)
    {
    pObjTrxTree keys_provided[8];
    int n_keys_provided = 0;
    int i,j,colid;
    pObjTrxTree find_oxt, attr_oxt;
    pXString sql = NULL;
    char* key_values[8];
    char* ptr;
    int rval = 0;
    int t;
    MYSQL_RES* result = NULL;
    MYSQL_ROW row = NULL;
    int restype;
    int intval;
    int len;

	if (inf->TData->nKeys == 0) return -1;
	for(j=0;j<inf->TData->nKeys;j++) key_values[j] = NULL;

	/** Snarf the autoname semaphore **/
	/*syGetSem(inf->TData->AutonameSem, 1, 0);*/

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
	    if (find_oxt) 
		{
		n_keys_provided++;
		keys_provided[j] = find_oxt;
		ptr = mysd_internal_CxDataToMySQL(find_oxt->AttrType, find_oxt->AttrValue);
		/*if (!strcmp(inf->TData->ColTypes[colid],"bit"))
		    {
		    if (strtoi(ptr,NULL,10))
			ptr = "1";
		    else
			ptr = "\\0";
		    }*/
		if (ptr)
		    {
		    key_values[j] = nmSysStrdup(ptr);
		    if (!key_values[j])
			{
			rval = -ENOMEM;
			goto exit_BuildAutoname;
			}
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
		if (inf->TData->ColCxTypes[colid] != DATA_T_INTEGER)
		    {
		    mssError(1,"MYSD","Non-integer key field '%s' left NULL on autoname create", inf->TData->Cols[colid]);
		    rval = -1;
		    goto exit_BuildAutoname;
		    }
		for(i=0;i<inf->TData->nKeys;i++)
		    {
		    if (key_values[i])
			{
			if (sql->String[0])
			    xsConcatenate(sql, " AND ", 5);
			else
			    xsConcatenate(sql, " WHERE ", 7);
			xsConcatenate(sql, "`", 1);
			mysd_internal_SafeAppend(&conn->Handle, sql, inf->TData->Cols[inf->TData->KeyCols[i]]);
			xsConcatenate(sql, "` = ", 4);
			t = inf->TData->ColCxTypes[inf->TData->KeyCols[i]];
			if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE || t == DATA_T_MONEY)
			    xsConcatenate(sql, key_values[i], -1);
			else
			    {
			    xsConcatenate(sql, "'", 1);
			    mysd_internal_SafeAppend(&conn->Handle, sql, key_values[i]);
			    xsConcatenate(sql, "'", 1);
			    }
			}
		    }

		/** Now that sql is built, send to server **/
		result=mysd_internal_RunQuery_conn(conn, inf->Node,
			"SELECT ifnull(max(`?`),0)+1 FROM `?` ?q",
			inf->TData->Cols[colid], inf->TData->Name, sql->String);
		if (result != MYSD_RUNQUERY_ERROR)
		    {
		    row = mysql_fetch_row(result);
		    if (row && row[0])
			{
			/** Got a value. **/
			key_values[j] = nmSysStrdup(row[0]);
			}
		    mysql_free_result(result);
		    result = NULL;
		    }
		else
		    {
		    mssError(1,"MYSD","Could not obtain next-value information for key '%s' on table '%s'",
			    inf->TData->Cols[colid], inf->TData->Name);
		    rval = -1;
		    goto exit_BuildAutoname;
		    }
		}
	    }

	/** Okay, all keys filled in.  Build the autoname. **/
	inf->Objname[0] = '\0';
	for(len=j=0;j<inf->TData->nKeys;j++)
	    {
	    if (j) len += 1; /* for | separator */
	    len += strlen(key_values[j]);
	    }
	if (len >= sizeof(inf->Objname))
	    {
	    mssError(1,"MYSD","Autoname too long!");
	    rval = -1;
	    goto exit_BuildAutoname;
	    }
	for(j=0;j<inf->TData->nKeys;j++)
	    {
	    if (j) strcat(inf->Objname,"|");
	    strcat(inf->Objname, key_values[j]);
	    }

    exit_BuildAutoname:
	/** Release resources we used **/
	if (result && result != MYSD_RUNQUERY_ERROR)
	    mysql_free_result(result);
	/*if (rval < 0)
	    syPostSem(inf->TData->AutonameSem, 1, 0);*/
	if (sql)
	    {
	    xsDeInit(sql);
	    nmFree(sql, sizeof(XString));
	    sql = NULL;
	    }
	for(j=0;j<inf->TData->nKeys;j++) 
	    if (key_values[j]) nmSysFree(key_values[j]);

    return rval;
    }


/*** mysd_internal_UpdateRow() - update a given row
 ***/
int
mysd_internal_UpdateRow(pMysdData data, char* newval, int col)
    {
    pMysdConn conn = NULL;
    int i = 0;
    MYSQL_RES* result;
    int use_quotes;
    
	/** Handle bits **/
	/*if (!strcmp(data->TData->ColTypes[col], "bit") && newval)
	    {
	    if (strtoi(newval, NULL, 10))
		newval = "1";
	    else
		newval = "0";
	    }*/

	/** Quote the value if not an integer and value is not NULL **/
	use_quotes = (data->TData->ColCxTypes[col] != DATA_T_INTEGER && newval != NULL);
        
        //result = mysd_internal_RunQuery(data->Node,"UPDATE `?` SET `?`=?v WHERE CONCAT_WS('|',`?a`)='?'", data->TData->Name, data->TData->Cols[col],newval?newval:"NULL",use_quotes, data->TData->Keys,NULL,data->TData->nKeys,',', data->Name);
        result = mysd_internal_RunQuery(data->Node,"UPDATE `?` SET `?`=?v WHERE ?n",
		data->TData->Name,
		data->TData->Cols[col],
		newval?newval:"NULL",
		use_quotes,
		data
		);
	if (result == MYSD_RUNQUERY_ERROR)
	    return -1;

	mysd_internal_RefreshRow(data);
        return 0;
    }
    
/*** mysd_internal_DeleteRow() - update a given row
 ***/
int
mysd_internal_DeleteRow(pMysdData data)
    {
    pMysdConn conn = NULL;
    int i = 0;
    MYSQL_RES* result;

	/** Only rows can be deleted **/
	if (data->Type != MYSD_T_ROW) return -1;

        //result = mysd_internal_RunQuery(data->Node,"DELETE FROM `?` WHERE CONCAT_WS('|',`?a`)='?'",data->TData->Name,data->TData->Keys,NULL,data->TData->nKeys,',', data->Name);
        result = mysd_internal_RunQuery(data->Node,"DELETE FROM `?` WHERE ?n",
		data->TData->Name,
		data
		);
	if (result == MYSD_RUNQUERY_ERROR)
	    return -1;
        return 0;
    }


/*** mysd_internal_InsertRow() - update a given row
 ***/
int
mysd_internal_InsertRow(pMysdData inf, pObjTrxTree oxt)
    {
    char* kptr;
    char* find_str;
    char* ptr;
    int i,j,len,ctype,restype;
    int* length;
    pObjTrxTree attr_oxt, find_oxt;
    pXString insbuf = NULL;
    char* values[MYSD_MAX_COLS];
    char* cols[MYSD_MAX_COLS];
    char* filename;
    MYSQL_RES* result = MYSD_RUNQUERY_ERROR;
    pMysdConn conn = NULL;
    char namebuf[OBJSYS_MAX_PATH];
    char* namekeys[MYSD_MAX_KEYS];
    char use_quotes[MYSD_MAX_COLS];

        /** Allocate a buffer for our insert statement. **/
        insbuf = (pXString)nmMalloc(sizeof(XString));
        if (!insbuf)
	    goto error;
        xsInit(insbuf);

	/** Get a connection. **/
	conn = mysd_internal_GetConn(inf->Node);
	if (!conn)
	    goto error;

	/** Auto-naming? **/
	if ((inf->Obj->Mode & OBJ_O_AUTONAME) && !strcmp(inf->Name, "*"))
	    {
	    if (mysd_internal_BuildAutoname(inf, conn, oxt) < 0)
		{
		mssError(0, "MYSD", "Could not generate a name for new object.");
		goto error;
		}
	    inf->Name = inf->Objname;
	    }

	/** Make a copy of the object name so we can parse it out **/
	strtcpy(namebuf, inf->Name, sizeof(namebuf));
	filename = namebuf;

	/** Get our keys array from the name **/
	memset(namekeys, 0, sizeof(namekeys));
	for(j=0;j<MYSD_MAX_KEYS;j++)
	    {
	    ptr = strchr(filename, '|');
	    namekeys[j] = filename;
	    if (ptr)
		{
		*ptr = '\0';
		filename = ptr+1;
		}
	    else
		{
		break;
		}
	    }

        /** Ok, look for the attribute sub-OXT's **/
        for(j=0;j<inf->TData->nCols;j++)
            {
            find_oxt = NULL;
            find_str = NULL;

	    /** Look in the filename **/
            if (inf->TData->ColFlags[j] & MYSD_COL_F_PRIKEY)
                {
		if (namekeys[inf->TData->ColKeys[j]])
		    find_str = namekeys[inf->TData->ColKeys[j]];
                }

            /** we scan through the OXT's **/
            for(i=0;i<oxt->Children.nItems;i++)
                {
                attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
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
            if (j!=0) xsConcatenate(insbuf,"\0",1);

            /** Print the appropriate type. **/
            if (!find_oxt && !find_str)
                {
                if (inf->TData->ColFlags[j] & MYSD_COL_F_NULL)
                    {
		    /** Mark the field as NULL with a unique value here **/
		    values[j] = (char*)-1;
                    }
                else
                    {
                    mssError(1,"MYSD","Required column '%s' not specified in object create", inf->TData->Cols[j]);
		    goto error;
                    }
                }
            else 
                {
		if ((!find_oxt || !find_oxt->AttrValue) && !find_str)
		    {
		    if (inf->TData->ColFlags[j] & MYSD_COL_F_NULL)
			{
			/** Mark the field as NULL with a unique value here **/
			values[j] = (char*)-1;
			}
		    else
			{
			mssError(1,"MYSD","Required column '%s' set to NULL in object create", inf->TData->Cols[j]);
			goto error;
			}
		    }
		else
		    {
		    values[j] = (char*)(uintptr_t)insbuf->Length;
		    if(!find_str)
			{
			find_str = mysd_internal_CxDataToMySQL(find_oxt->AttrType,find_oxt->AttrValue);
			/*if (!strcmp(inf->TData->ColTypes[j],"bit"))
			    {
			    if (strtoi(find_str,NULL,10))
				find_str = "1";
			    else
				find_str = "\\0";
			    }*/
			}
		    if(find_str)
			xsConcatenate(insbuf,find_str,-1);
		    else
			{
			mssError(1,"MYSD","Unable to convert data");
			goto error;
			}
		    }
		}
            }

	/** Entirely leave NULLs out of the insert **/
        i = 0;
        for(j=0;j<inf->TData->nCols;j++)
            {
            if(values[j] != (char*)-1)
                {
                cols[i] = inf->TData->Cols[j];
                values[i] = (char*)((uintptr_t)values[j] + (uintptr_t)insbuf->String);
		use_quotes[i] = (inf->TData->ColCxTypes[j] != DATA_T_INTEGER);
                i++;
                }
            }

	/** Do the insert **/
        result = mysd_internal_RunQuery_conn(conn, inf->Node,"INSERT INTO `?` (`?a`) VALUES(?a)",inf->TData->Name,cols,NULL,i,',',values,use_quotes,i,',');
	/*if (result != MYSD_RUNQUERY_ERROR)
	    mysd_internal_RefreshRow(inf);*/
        
        /** clean up **/
    error:
	if (conn)
	    mysd_internal_ReleaseConn(&conn);
	if (insbuf)
	    {
	    xsDeInit(insbuf);
	    nmFree(insbuf,sizeof(XString));
	    }

    return (result == MYSD_RUNQUERY_ERROR)?(-1):0;
    }


/*** mysd_internal_GetTablenames() - throw the table names in an Xarray
 ***/
int
mysd_internal_GetTablenames(pMysdNode node)
    {
    MYSQL_RES * result = NULL;
    MYSQL_ROW row;
    pMysdConn conn;
    int nTables;
    char* tablename;
    
        result = mysd_internal_RunQuery(node,"SHOW TABLES");
	if (result == MYSD_RUNQUERY_ERROR || !result)
	    return -1;

        nTables = mysql_num_rows(result);
        xaInit(&node->Tablenames,nTables);
        
        while((row = mysql_fetch_row(result)))
            {
	    if (row[0])
		{
		tablename = nmSysStrdup((char*)(row[0]));
		if(xaAddItem(&node->Tablenames, tablename) < 0)
		    {
		    mysql_free_result(result);
		    return -1;
		    }
		}
            }
    
    mysql_free_result(result);
    return 0;
    }

/*** mysd_internal_OpenNode() - access the node object and get the mysql
 *** server connection parameters.
 ***/
pMysdNode
mysd_internal_OpenNode(char* path, int mode, pObject obj, int node_only, int mask)
    {
    pMysdNode db_node;
    pSnNode sn_node;
    char* ptr = NULL;
    int i;
    
        /** First, do a lookup in the db node cache. **/
        db_node = (pMysdNode)xhLookup(&(MYSD_INF.DBNodesByPath), path);
        if (db_node) 
            {
            db_node->LastAccess = MYSD_INF.AccessCnt;
            MYSD_INF.AccessCnt++;
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
            sn_node = snNewNode(obj->Prev, "application/mysql");
            if (!sn_node)
                {
                mssError(0,"MYSQL","Database node create failed");
                return NULL;
                }
            snSetParamString(sn_node,obj->Prev,"server","localhost");
            snSetParamString(sn_node,obj->Prev,"database","");
            snSetParamString(sn_node,obj->Prev,"annot_table","CXTableAnnotations");
            snSetParamString(sn_node,obj->Prev,"description","MySQL Database");
            snSetParamInteger(sn_node,obj->Prev,"max_connections",16);
            snWriteNode(obj->Prev,sn_node);
            }
        else
            {
            sn_node = snReadNode(obj->Prev);
            if (!sn_node)
                {
                mssError(0,"MYSQL","Database node open failed");
                return NULL;
                }
            }

        /** Create the DB node and fill it in. **/
        db_node = (pMysdNode)nmMalloc(sizeof(MysdNode));
        if (!db_node)
            {
            mssError(0,"MYSQL","Could not allocate DB node structure");
            return NULL;
            }
        memset(db_node,0,sizeof(MysdNode));
        db_node->SnNode = sn_node;
        strtcpy(db_node->Path,path,sizeof(db_node->Path));
        if (stAttrValue(stLookup(sn_node->Data,"server"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->Server,ptr?ptr:"localhost",sizeof(db_node->Server));
        if (stAttrValue(stLookup(sn_node->Data,"database"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->Database,ptr?ptr:"",sizeof(db_node->Database));
        if (stAttrValue(stLookup(sn_node->Data,"annot_table"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->AnnotTable,ptr?ptr:"CXTableAnnotations",sizeof(db_node->AnnotTable));
        if (stAttrValue(stLookup(sn_node->Data,"description"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->Description,ptr?ptr:"",sizeof(db_node->Description));
        if (stAttrValue(stLookup(sn_node->Data,"max_connections"),&i,NULL,0) < 0) i=16;
        if (stAttrValue(stLookup(sn_node->Data,"use_system_auth"),NULL,&ptr,0) == 0)
            {
            if (!strcasecmp(ptr,"yes"))
            db_node->Flags |= MYSD_NODE_F_USECXAUTH;
            }
        if (stAttrValue(stLookup(sn_node->Data,"set_passwords"),NULL,&ptr,0) == 0)
            {
            if (!strcasecmp(ptr,"yes"))
            db_node->Flags |= MYSD_NODE_F_SETCXAUTH;
            }
        if (stAttrValue(stLookup(sn_node->Data,"username"),NULL,&ptr,0) < 0) ptr = "cxguest";
        strtcpy(db_node->Username,ptr,sizeof(db_node->Username));
        if (stAttrValue(stLookup(sn_node->Data,"password"),NULL,&ptr,0) < 0) ptr = "";
        strtcpy(db_node->Password,ptr,sizeof(db_node->Password));
        if (stAttrValue(stLookup(sn_node->Data,"default_password"),NULL,&ptr,0) < 0) ptr = "";
        strtcpy(db_node->DefaultPassword,ptr,sizeof(db_node->DefaultPassword));
        db_node->MaxConn = i;
        xaInit(&(db_node->Conns),16);
        xhInit(&(db_node->Tables),255,0);
        db_node->ConnAccessCnt = 0;
        
        /** Get table names **/
        if((i=mysd_internal_GetTablenames(db_node)) < 0)
            {
            mssError(0,"MYSD","Unable to query for table names");
            nmFree(db_node,sizeof(MysdNode));
            return NULL;
            }

        /** Add node to the db node cache **/
        xhAdd(&(MYSD_INF.DBNodesByPath), db_node->Path, (void*)db_node);
        xaAddItem(&MYSD_INF.DBNodeList, (void*)db_node);

    return db_node;
    }

/*** mysd_internal_DetermineType() - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
mysd_internal_DetermineType(pObject obj, pMysdData inf)
    {
    int i;

        /** Determine object type (depth) and get pointers set up **/
        obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
        for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;

        /** Set up pointers based on number of elements past the node object **/
        if (inf->Pathname.nElements == obj->SubPtr)
            {
            inf->Type = MYSD_T_DATABASE;
            obj->SubCnt = 1;
            }
        if (inf->Pathname.nElements - 1 >= obj->SubPtr)
            {
            inf->Type = MYSD_T_TABLE;
            obj->SubCnt = 2;
            }
        if (inf->Pathname.nElements - 2 >= obj->SubPtr)
            {
            if (!strncmp(inf->Pathname.Elements[obj->SubPtr+1],"rows",4)) inf->Type = MYSD_T_ROWSOBJ;
            else if (!strncmp(inf->Pathname.Elements[obj->SubPtr+1],"columns",7)) inf->Type = MYSD_T_COLSOBJ;
            else return -1;
            obj->SubCnt = 3;
            }
        if (inf->Pathname.nElements - 3 >= obj->SubPtr)
            {
            if (inf->Type == MYSD_T_ROWSOBJ) inf->Type = MYSD_T_ROW;
            else if (inf->Type == MYSD_T_COLSOBJ) inf->Type = MYSD_T_COLUMN;
            obj->SubCnt = 4;
            }

	/** Point to name. **/
	strtcpy(inf->Objname, inf->Pathname.Elements[obj->SubPtr + obj->SubCnt - 1 - 1], sizeof(inf->Objname));
	inf->Name = inf->Objname;

    return 0;
    }

/*** mysd_internal_TreeToClause - convert an expression tree to the appropriate
 *** clause for the SQL statement.
 ***/
int
mysd_internal_TreeToClause(pExpression tree, pMysdTable *tdata, pXString where_clause, MYSQL * conn)
    {
    pExpression subtree;
    int i,id = 0;
    XString tmp;
    char* fn_use_name;
    int use_stock_fn_call;
    char quote;
    char* ptr;
    int len;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"MYSD","Failed to run query: resource exhaustion occurred");
	    return -1;
	    }
    
        xsInit(&tmp);

        /** If int or string, just put the content, otherwise recursively build **/
        switch (tree->NodeType)
            {
            case EXPR_N_DATETIME:
          mysd_DO_DATETIME:
		ptr = objFormatDateTmp(&(tree->Types.Date),"yyyy-MM-dd HH:mm:ss");
		xsConcatQPrintf(where_clause, "convert(%STR&DQUOT, datetime)", ptr);
                /*objDataToString(where_clause, DATA_T_DATETIME, &(tree->Types.Date), DATA_F_QUOTED);*/
                break;

            case EXPR_N_MONEY:
          mysd_DO_MONEY:
		ptr = objFormatMoneyTmp(&(tree->Types.Money), "0.0000");
		xsConcatenate(where_clause, ptr, -1);
                /*objDataToString(where_clause, DATA_T_MONEY, &(tree->Types.Money), DATA_F_QUOTED);*/
                break;

            case EXPR_N_DOUBLE:
          mysd_DO_DOUBLE:
                objDataToString(where_clause, DATA_T_DOUBLE, &(tree->Types.Double), DATA_F_QUOTED);
                  break;

            case EXPR_N_INTEGER:
          mysd_DO_INTEGER:
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
          mysd_DO_STRING:
		quote = '\'';
                if (tree->Parent && tree->Parent->NodeType == EXPR_N_FUNCTION && 
                    (!strcmp(tree->Parent->Name,"convert") || !strcmp(tree->Parent->Name,"datepart")) &&
                    (void*)tree == (void*)(tree->Parent->Children.Items[0]))
                    {
                    if (!strcmp(tree->Parent->Name,"convert"))
                        {
			quote = ' ';
                        if (!strcmp(tree->String,"integer")) xsConcatenate(&tmp,"signed integer",-1);
                        else if (!strcmp(tree->String,"string"))
			    {
			    xsConcatenate(&tmp,"char",-1);
			    tree->Parent->DataType = DATA_T_STRING;
			    }
                        else if (!strcmp(tree->String,"double")) xsConcatenate(&tmp,"double",6);
                        else if (!strcmp(tree->String,"money")) xsConcatenate(&tmp,"decimal(14,4)",13);
                        else if (!strcmp(tree->String,"datetime")) xsConcatenate(&tmp,"datetime",8);
                        }
                    else
                        {
                        if (strpbrk(tree->String,"\"' \t\r\n"))
                            {
                            mssError(1,"mysd","Invalid datepart() parameters in Expression Tree");
                            }
                        else
                            {
                            objDataToString(&tmp, DATA_T_STRING, tree->String, 0);
                            }
                        }
                    }
                else
                    {
                    objDataToString(&tmp, DATA_T_STRING, tree->String, 0);
                    }
                xsConcatenate(where_clause, &quote, 1);
                mysd_internal_SafeAppend(conn,where_clause,tmp.String);
                xsConcatenate(where_clause, &quote, 1);
                xsDeInit(&tmp);
                xsInit(&tmp);
                break;

            case EXPR_N_OBJECT:
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
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
                    if (tree->DataType == DATA_T_INTEGER) goto mysd_DO_INTEGER;
                    else if (tree->DataType == DATA_T_STRING) goto mysd_DO_STRING;
                    else if (tree->DataType == DATA_T_DATETIME) goto mysd_DO_DATETIME;
                    else if (tree->DataType == DATA_T_MONEY) goto mysd_DO_MONEY;
                    else if (tree->DataType == DATA_T_DOUBLE) goto mysd_DO_DOUBLE;
                    }

                /** Direct ref object? **/
                if (tree->ObjID == -1)
                    {
                    expEvalTree(tree,NULL);

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
                    /** Is this a special type of property (i.e., name or annotation?) **/
                    if (!strcmp(tree->Name,"name"))
                        {
                        xsConcatenate(where_clause, " CONCAT_WS('|', ", -1);
                        for(i=0;i<tdata[id]->nKeys;i++)
                            {
                            if (i != 0) xsConcatenate(where_clause, " , ", 3);
                            xsConcatenate(where_clause, "cast(`", -1);
			    mysd_internal_SafeAppend(conn, where_clause, tdata[id]->Keys[i]);
                            xsConcatenate(where_clause, "` as char)", 10);
                            }
                        xsConcatenate(where_clause, ") ",2);
                        }
                    else if (!strcmp(tree->Name,"annotation"))
                        {
                        /** no anotation support atm **/
                        }
                    else
                        {
                        /** "Normal" type of object... **/
                        xsConcatenate(where_clause, " `", 2);
			mysd_internal_SafeAppend(conn, where_clause, tree->Name);
                        xsConcatenate(where_clause, "` ", 2);
                        }
                    }
                break;

            case EXPR_N_COMPARE:
                xsConcatenate(where_clause, " (", 2);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " ", 1);
                if (tree->CompareType & MLX_CMP_LESS) xsConcatenate(where_clause,"<",1);
                if (tree->CompareType & MLX_CMP_GREATER) xsConcatenate(where_clause,">",1);
                if (tree->CompareType & MLX_CMP_EQUALS) xsConcatenate(where_clause,"=",1);
                xsConcatenate(where_clause, " ", 1);
                subtree = (pExpression)(tree->Children.Items[1]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, ") ", 2);
                break;

            case EXPR_N_AND:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " AND ",5);
                subtree = (pExpression)(tree->Children.Items[1]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, ") ",2);
                break;

            case EXPR_N_OR:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " OR ",4);
                subtree = (pExpression)(tree->Children.Items[1]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, ") ",2);
                break;

            case EXPR_N_ISNOTNULL:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " IS NOT NULL) ",14);
                break;

            case EXPR_N_ISNULL:
                xsConcatenate(where_clause, " (",2);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " IS NULL) ",10);
                break;

            case EXPR_N_NOT:
                xsConcatenate(where_clause, " ( NOT ( ",9);
                subtree = (pExpression)(tree->Children.Items[0]);
                mysd_internal_TreeToClause(subtree,tdata,where_clause,conn);
                xsConcatenate(where_clause, " ) ) ",5);
                break;

            case EXPR_N_FUNCTION:
                /** Special case functions which MySQL doesn't have. **/
		fn_use_name = tree->Name;
		use_stock_fn_call = 0;
                if (!strcmp(tree->Name,"condition") && tree->Children.nItems == 3)
                    {
		    fn_use_name = "if";
		    use_stock_fn_call = 1;
                    }
		else if (!strcmp(tree->Name,"isnull"))
		    {
		    /** MySQL isnull() does something different than CX isnull().  Use ifnull() instead. **/
		    fn_use_name = "ifnull";
		    use_stock_fn_call = 1;
		    }
		else if (!strcmp(tree->Name,"convert"))
		    {
		    /** MySQL convert() params are reversed to what CX, Sybase, and MS SQL expect. **/
		    xsConcatenate(where_clause, " convert(", -1);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, " , ", 3);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, ") ", 2);
		    }
		else if (!strcmp(tree->Name,"upper") || !strcmp(tree->Name, "lower"))
		    {
		    /** check if database is using an expected character set **/
		    pMysdTable tempTdata = *tdata;

		    /** need to change collation to guarantee the result is case sensitive (in case it is used in compares) **/
		    xsConcatPrintf(where_clause, " (%s(", tree->Name);
		    len = xsLength(where_clause);
		    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, where_clause, conn);

		    if (xsLength(where_clause) - len == 6 && !strcmp(xsString(where_clause)+len, " NULL "))
			{
			/** upper()/lower() param evaluated to NULL, don't use collation. **/
			xsConcatenate(where_clause, ")) ", 3);
			}
		    else
			{
			/** only change collation if found one **/
			if(tempTdata->Node->DatabaseCollation[0] != '\0')
			    {
			    xsConcatenate(where_clause, ") collate ", -1);
			    xsConcatenate(where_clause, tempTdata->Node->DatabaseCollation, -1);
			    xsConcatenate(where_clause, ") ", -1);
			    } 
			else
			    xsConcatenate(where_clause, ")) ", 3);
			}
		    }
		else if (!strcmp(tree->Name,"charindex"))
		    {
		    /** MySQL locate() function is the equivalent of CX charindex() **/
		    fn_use_name = "locate";
		    use_stock_fn_call = 1;
		    }
		else if (!strcmp(tree->Name,"getdate"))
		    {
		    /** MySQL now() function is the equivalent of CX getdate() **/
		    fn_use_name = "now";
		    use_stock_fn_call = 1;
		    }
		else if (!strcmp(tree->Name,"user_name"))
		    {
		    /** MySQL equivalent is substring_index(current_user(),'@',1) **/
		    xsConcatenate(where_clause, " substring_index(current_user(),'@',1) ", -1);
		    }
		else if (!strcmp(tree->Name,"datediff"))
		    {
		    /** MySQL uses timestampdiff() **/
		    subtree = (pExpression)(tree->Children.Items[0]);
		    if (subtree->DataType != DATA_T_STRING || subtree->Flags & EXPR_F_NULL)
			{
			mssError(1,"MYSD","Invalid date part for datediff()");
			return -1;
			}
		    if (!strcmp(subtree->String,"day") || !strcmp(subtree->String,"month") ||
			    !strcmp(subtree->String,"year") || !strcmp(subtree->String, "hour") ||
			    !strcmp(subtree->String,"minute") || !strcmp(subtree->String, "second"))
			{
			xsConcatenate(where_clause, " timestampdiff(", -1);
			xsConcatenate(where_clause, subtree->String, -1);
			xsConcatenate(where_clause, ", ", -1);
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
			xsConcatenate(where_clause, ", ", -1);
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), tdata,  where_clause,conn);
			xsConcatenate(where_clause, ") ", -1);
			}
		    else
			{
			mssError(1,"MYSD","Invalid date part to datediff()");
			return -1;
			}
		    }
		else if (!strcmp(tree->Name,"dateadd"))
		    {
		    /** MySQL uses date_add(date, interval increment datepart) instead of dateadd(datepart, increment, date) **/
		    xsConcatenate(where_clause, " date_add(", -1);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, ", interval ", -1);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, " ", 1);
		    subtree = (pExpression)(tree->Children.Items[0]);
		    if (subtree->DataType != DATA_T_STRING || subtree->Flags & EXPR_F_NULL)
			{
			mssError(1,"MYSD","Invalid date part to dateadd()");
			return -1;
			}
		    if (!strcmp(subtree->String,"day") || !strcmp(subtree->String,"month") ||
			    !strcmp(subtree->String,"year") || !strcmp(subtree->String, "hour") ||
			    !strcmp(subtree->String,"minute") || !strcmp(subtree->String, "second"))
			{
			xsConcatenate(where_clause,subtree->String,-1);
			}
		    else
			{
			mssError(1,"MYSD","Invalid date part to dateadd()");
			return -1;
			}
		    xsConcatenate(where_clause, ") ", 2);
		    }
                else if (!strcmp(tree->Name,"ralign") && tree->Children.nItems == 2)
                    {
                    xsConcatenate(where_clause, " substring('", -1);
                    for(i=0;i<255 && i<((pExpression)(tree->Children.Items[1]))->Integer;i++)
                        xsConcatenate(where_clause, " ", 1);
                    xsConcatenate(where_clause, "',1,", 4);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
                    xsConcatenate(where_clause, " - char_length(", -1);
                    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                    xsConcatenate(where_clause, ")) ", 3);
                    }
                else if (!strcmp(tree->Name,"eval"))
                    {
                    mssError(1,"MYSD","MySQL does not support eval() CXSQL function");
                    /* just put silly thing as text instead of evaluated */
                    if (tree->Children.nItems == 1) mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                    return -1;
                    }
		else if (!strcmp(tree->Name,"datepart"))
		    {
		    /** MySQL uses year(), month(), day(), hour(), minute(), second() **/
		    subtree = (pExpression)(tree->Children.Items[0]);
		    if (subtree->DataType == DATA_T_STRING && subtree->String && (!strcmp(subtree->String, "year") || !strcmp(subtree->String, "month") || !strcmp(subtree->String, "day") || !strcmp(subtree->String, "hour") || !strcmp(subtree->String, "minute") || !strcmp(subtree->String, "second")))
			{
			xsConcatenate(where_clause, " ", 1);
			xsConcatenate(where_clause, subtree->String, -1);
			xsConcatenate(where_clause, "(", 1);
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
			xsConcatenate(where_clause, ") ", 2);
			}
		    else if (subtree->DataType == DATA_T_STRING && subtree->String && (!strcmp(subtree->String, "weekday")))
			{
			xsConcatenate(where_clause, " dayofweek(", -1);
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
			xsConcatenate(where_clause, ") ", 2);
			}
		    else
			{
			mssError(1,"MYSD","Invalid date part for datepart()");
			return -1;
			}
		    }
		else if (!strcmp(tree->Name, "replace"))
		    {
		    xsConcatenate(where_clause, " replace(", -1);
		    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata, where_clause, conn);
		    xsConcatenate(where_clause, ",", 1);
		    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, where_clause, conn);
		    xsConcatenate(where_clause, ",", 1);
		    subtree = (pExpression)tree->Children.Items[2];

		    /** MySQL does not accept NULL for the replacement, use "" instead **/
		    if (subtree && (subtree->Flags & EXPR_F_NULL))
			xsConcatenate(where_clause, "\"\") ", 4);
		    else
			{
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[2]), tdata, where_clause, conn);
			xsConcatenate(where_clause, ") ", 2);
			}
		    }
		else if (!strcmp(tree->Name, "hash"))
		    {
		    /** MySQL uses MD5(), SHA1(), SHA2(data, bitsize) **/
		    subtree = (pExpression)(tree->Children.Items[0]);
		    if (subtree->DataType == DATA_T_STRING && subtree->String && (!strcmp(subtree->String, "md5") || !strcmp(subtree->String, "sha1") || !strcmp(subtree->String, "sha256") || !strcmp(subtree->String, "sha384") || !strcmp(subtree->String, "sha512")))
			{
			/** Function call itself **/
			if (!strcmp(subtree->String, "md5"))
			    xsConcatenate(where_clause, " MD5(", 5);
			else if (!strcmp(subtree->String, "sha1"))
			    xsConcatenate(where_clause, " SHA1(", 6);
			else if (!strcmp(subtree->String, "sha256") || !strcmp(subtree->String, "sha384") || !strcmp(subtree->String, "sha512"))
			    xsConcatenate(where_clause, " SHA2(", 6);

			/** Data **/
			mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata, where_clause, conn);

			/** Bit length, if needed **/
			if (!strcmp(subtree->String, "sha256") || !strcmp(subtree->String, "sha384") || !strcmp(subtree->String, "sha512"))
			    {
			    xsConcatQPrintf(where_clause, ", %POS", strtoi(subtree->String+3, NULL, 10));
			    }
			xsConcatenate(where_clause, ") ", 2);
			}
		    else
			{
			mssError(1,"MYSD","Invalid algorithm for hash()");
			return -1;
			}
		    }
                else
                    {
		    use_stock_fn_call = 1;
		    }

		if (use_stock_fn_call)
		    {
                    xsConcatenate(where_clause, " ", 1);
                    xsConcatenate(where_clause, fn_use_name, -1);
                    xsConcatenate(where_clause, "(", 1);
                    for(i=0;i<tree->Children.nItems;i++)
                        {
                        if (i != 0) xsConcatenate(where_clause,",",1);
                        subtree = (pExpression)(tree->Children.Items[i]);
                        mysd_internal_TreeToClause(subtree, tdata,  where_clause,conn);
                        }
                    xsConcatenate(where_clause, ") ", 2);
                    }
                break;

            case EXPR_N_PLUS:
		/** This is a toughie.  In Centrallix and Sybase/MSSQL, the + operator
		 ** can be used for addition or for string concatenation, and so the
		 ** behavior depends on the run-time type of the operands.  In MySQL
		 ** we have to make this decision in advance, and use CONCAT() for
		 ** strings and + for numbers.
		 **
		 ** If we know the type of operand 1 in advance, we choose + or CONCAT()
		 ** statically.  Otherwise, we just have to punt...
		 **/
		if (tree->Children.nItems < 2)
		    {
		    mssError(1,"MYSD","Insufficient arguments to + operator");
		    return -1;
		    }
		subtree = (pExpression)tree->Children.Items[0];
		if (subtree->NodeType == EXPR_N_PROPERTY && !(subtree->Flags & EXPR_F_FREEZEEVAL) && subtree->DataType == DATA_T_UNAVAILABLE)
		    {
		    /** A property under our control **/
		    for(i=0;i<tdata[0]->nCols;i++)
			{
			if (!strcmp(subtree->Name, tdata[0]->Cols[i]))
			    {
			    if (tdata[0]->ColCxTypes[i] == DATA_T_STRING)
				subtree->DataType = DATA_T_STRING;
			    }
			}
		    }
		if (tree->Parent && tree->Parent->NodeType == EXPR_N_PLUS && tree->Parent->DataType == DATA_T_STRING)
		    {
		    /** We're within a string concatenation expression already... **/
		    subtree->DataType = DATA_T_STRING;
		    }
		if (subtree->NodeType == EXPR_N_FUNCTION && (!strcmp(subtree->Name, "user_name") || !strcmp(subtree->Name, "substring") || !strcmp(subtree->Name, "right")))
		    {
		    /** These functions always invariably return a string... **/
		    subtree->DataType = DATA_T_STRING;
		    }
		if (subtree->NodeType == EXPR_N_FUNCTION && (!strcmp(subtree->Name, "charindex")))
		    {
		    /** These functions always invariably return an integer... **/
		    subtree->DataType = DATA_T_INTEGER;
		    }
		if (subtree->NodeType == EXPR_N_FUNCTION && (!strcmp(subtree->Name, "sin") || !strcmp(subtree->Name, "cos") || !strcmp(subtree->Name, "power") || !strcmp(subtree->Name, "sqrt") || !strcmp(subtree->Name, "atan2") || !strcmp(subtree->Name, "radians")))
		    {
		    /** These functions always invariably return a floating point type **/
		    subtree->DataType = DATA_T_DOUBLE;
		    }
		if (subtree->DataType == DATA_T_STRING)
		    {
		    /** We get here if 1) op 1 is a constant STRING, 2) op 1 is one of
		     ** of our properties and we know it is a string, 3) the parent
		     ** node of this + operator is another + operator and it used CONCAT,
		     ** or 4) op 1 is a function call that is known to return a string.
		     **/
		    tree->DataType = DATA_T_STRING;
		    xsConcatenate(where_clause, " CONCAT(", -1);
		    mysd_internal_TreeToClause(subtree, tdata,  where_clause,conn);
		    xsConcatenate(where_clause, " , ", 3);
		    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, ") ", 2);
		    }
		else
		    {
		    /** Let's leave room to change this into a CONCAT if after building
		     ** the first subexpression we find out that it is a string.  After all,
		     ** this needs to work, but it doesn't need to look pretty.
		     **/
		    i = strlen(where_clause->String);
		    xsConcatenate(where_clause, "       (", -1);
		    mysd_internal_TreeToClause(subtree, tdata,  where_clause,conn);
		    if (subtree->DataType == DATA_T_STRING)
			{
			tree->DataType = DATA_T_STRING;
			xsSubst(where_clause, i+1, 6, "CONCAT", 6);
			xsConcatenate(where_clause, " , ", 3);
			}
		    else
			{
			xsConcatenate(where_clause, " + ", 3);
			}
		    mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
		    xsConcatenate(where_clause, ") ", 2);
		    }
                break;

            case EXPR_N_MINUS:
                xsConcatenate(where_clause, " (", 2);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, " - ", 3);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, ") ", 2);
                break;

            case EXPR_N_DIVIDE:
                xsConcatenate(where_clause, " (", 2);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, " / ", 3);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, ") ", 2);
                break;

            case EXPR_N_MULTIPLY:
                xsConcatenate(where_clause, " (", 2);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, " * ", 3);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[1]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, ") ", 2);
                break;

            case EXPR_N_IN:
                xsConcatenate(where_clause, " (", 2);
                mysd_internal_TreeToClause((pExpression)(tree->Children.Items[0]), tdata,  where_clause,conn);
                xsConcatenate(where_clause, " IN (", 5);
                subtree = (pExpression)(tree->Children.Items[1]);
                if (subtree->NodeType == EXPR_N_LIST)
                    {
                    for(i=0;i<subtree->Children.nItems;i++)
                        {
                        if (i != 0) xsConcatenate(where_clause, ",", 1);
                        mysd_internal_TreeToClause((pExpression)(subtree->Children.Items[i]), tdata,  where_clause,conn);
                        }
                    }
                else
                    {
                    mysd_internal_TreeToClause(subtree, tdata,  where_clause,conn);
                    }
                xsConcatenate(where_clause, ") ) ", 4);
                break;
            }

        if (tree->Flags & EXPR_F_DESC) xsConcatenate(where_clause, " DESC ", 6);

    return 0;
    }

/*** mysdOpen() - open an object.
 ***/
void*
mysdOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pMysdData inf;
    int rval;
    int length;
    char* tablename;
    char* node_path;
    char* table;
    char* ptr;

        /** Allocate the structure **/
        inf = (pMysdData)nmMalloc(sizeof(MysdData));
        if (!inf) return NULL;
        memset(inf,0,sizeof(MysdData));
        inf->Obj = obj;
        inf->Mask = mask;
        inf->Result = NULL;
	inf->Row = NULL;

        /** Determine the type **/
        if(mysd_internal_DetermineType(obj,inf))
            {
            mssError(1,"MYSD","Unable to determine type.");
            nmFree(inf,sizeof(MysdData));
            return NULL;
            }

        /** Determine the node path **/
        node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

        /** this leaks memory MUST FIX **/
        if(!(inf->Node = mysd_internal_OpenNode(node_path, obj->Mode, obj, inf->Type == MYSD_T_DATABASE, inf->Mask)))
            {
            mssError(0,"MYSD","Couldn't open node.");
            nmFree(inf,sizeof(MysdData));
            return NULL;
            }
        
        /** get table name from path **/
        if(obj->SubCnt > 1)
            {
            tablename = obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1);
            length = obj->Pathname->Elements[obj->Pathname->nElements - 1] - obj->Pathname->Elements[obj->Pathname->nElements - 2];
            if(!(inf->TData = mysd_internal_GetTData(inf->Node,tablename)))
                {
                if(obj->Mode & O_CREAT)
                    /** Table creation code could some day go here,
                     ** but it is not supported right now
                     **/
                    mssError(1,"MYSD","Table creation is not supported currently");
                else
                    mssError(1,"MYSD","Table object does not exist.");
                nmFree(inf,sizeof(MysdData));
                return NULL;
                }
            }
        
        /** Set object params. **/
        obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
        inf->Node->SnNode->OpenCnt++;

        if(inf->Type == MYSD_T_ROW)
            {
            rval = 0;
            /** Autonaming a new object?  Won't be able to look it up if so. **/
            if (!(inf->Obj->Mode & OBJ_O_AUTONAME))
                {
                ptr = obj_internal_PathPart(obj->Pathname, obj->SubPtr+2, 1);
                if ((rval = mysd_internal_GetRowByKey(ptr,inf)) < 0)
                    {
                    mssError(1,"MYSD","Unable to fetch row by key.");
                    if(inf->Result) mysql_free_result(inf->Result);
                    inf->Result = NULL;
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                else
                    {
                     if(inf->Result) mysql_free_result(inf->Result);
                     inf->Result = NULL;                   
                    }
                }
            if (rval == 0)
                {
                /** User specified a row that doesn't exist. **/
                if (!(obj->Mode & O_CREAT))
                    {
                    mssError(1,"MYSD","Row object does not exist.");
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }

                /** row insert. set up the transaction **/
                if (!*oxt) *oxt = obj_internal_AllocTree();
                (*oxt)->OpType = OXT_OP_CREATE;
                (*oxt)->LLParam = (void*)inf;
                (*oxt)->Object = obj;
                (*oxt)->Status = OXT_S_VISITED;
                return inf;
                }
            }

    return (void*)inf;
    }


/*** mysdClose() - close an open object.
 ***/
int
mysdClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);

        /** commit changes before we get rid of the transaction **/
        mysdCommit(inf,oxt);

            /** Write the node first, if need be. **/
        snWriteNode(inf->Obj->Prev, inf->Node->SnNode);
        
        /** Release the memory **/
        inf->Node->SnNode->OpenCnt --;
        obj_internal_FreePathStruct(&inf->Pathname);
	if (inf->Row) mysd_internal_FreeRow(inf->Row);
	inf->Row = NULL;
        nmFree(inf,sizeof(MysdData));

    return 0;
    }


/*** mysdCreate() - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
mysdCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

        /** Call open() then close() **/
        obj->Mode = O_CREAT | O_EXCL;
        inf = mysdOpen(obj, mask, systype, usrtype, oxt);
        if (!inf) return -1;
        mysdClose(inf, oxt);

    return 0;
    }


/*** mysdDelete() - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
mysdDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pMysdData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

            /** Open the thing first to get the inf ptrs **/
        obj->Mode = O_WRONLY;
        inf = (pMysdData)mysdOpen(obj, 0, NULL, "", oxt);
        if (!inf) return -1;

        if (inf->Type != MYSD_T_ROW)
            {
            nmFree(inf,sizeof(MysdData));
            mssError(1,"MYSD","Unimplemented delete operation in MYSD");
            return -1;
            }

        /** delete the row from the DB **/
        if(!mysd_internal_DeleteRow(inf)) return -1;

        /** Physically delete the node, and then remove it from the node cache **/
	if (inf->Type == MYSD_T_DATABASE)
	    unlink(inf->Node->SnNode->NodePath);

        /** Release, don't call close because that might write data to a deleted object **/
        inf->Node->SnNode->OpenCnt --;
        obj_internal_FreePathStruct(&inf->Pathname);
	if (inf->Row) mysd_internal_FreeRow(inf->Row);
	inf->Row = NULL;
        nmFree(inf,sizeof(MysdData));

    return 0;
    }


/*** mysdDeleteObj() - delete an already-open object.
 ***/
int
mysdDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pMysdData inf = (pMysdData)inf_v;
    int rval = 0;

	/** Delete it. **/
	if (inf->Type == MYSD_T_ROW)
	    {
	    /** Ok, can delete rows **/
	    if (!mysd_internal_DeleteRow(inf))
		rval = -1;
	    }
	else
	    {
	    /** Unimplemented delete **/
            mssError(1,"MYSD","Unimplemented delete operation in MYSD");
	    rval = -1;
	    }

        /** Release, don't call close because that might write data to a deleted object **/
        inf->Node->SnNode->OpenCnt --;
        obj_internal_FreePathStruct(&inf->Pathname);
	if (inf->Row) mysd_internal_FreeRow(inf->Row);
	inf->Row = NULL;
        nmFree(inf,sizeof(MysdData));

    return rval;
    }


/*** mysdRead() - Structure elements have no content.  Fails.
 ***/
int
mysdRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    return -1;
    }


/*** mysdWrite() - Again, no content.  This fails.
 ***/
int
mysdWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    return -1;
    }


/*** mysdOpenQuery() - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
mysdOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    pMysdQuery qy;
    pMysdConn escape_conn = NULL;
    int i;
        /** check if we should really allow a query **/
        if(inf->Type==MYSD_T_ROW) return 0;
        if(inf->Type==MYSD_T_COLUMN) return 0;
        
        /** Allocate the query structure **/
        qy = (pMysdQuery)nmMalloc(sizeof(MysdQuery));
        if (!qy) return NULL;
        memset(qy, 0, sizeof(MysdQuery));
        qy->Data = inf;
        if(qy->Data->Result) mysql_free_result(qy->Data->Result);
        qy->Data->Result = NULL;
        qy->ItemCnt = 0;
        qy->Data->Result = NULL;
        xsInit(&qy->Clause);

        if(inf->Type==MYSD_T_ROWSOBJ)
            {
            query->Flags |= OBJ_QY_F_FULLQUERY;
            query->Flags |= OBJ_QY_F_FULLSORT;
            if(query->Tree || query->SortBy[0]) 
                {
                escape_conn = mysd_internal_GetConn(qy->Data->Node);
		if (!escape_conn)
		    return NULL;
                if (query->Tree)
                    {
                    xsConcatenate(&qy->Clause, " WHERE ", 7);
                    mysd_internal_TreeToClause((pExpression)(query->Tree),&(qy->Data->TData),&qy->Clause,&escape_conn->Handle);
                    }
                if (query->SortBy[0])
                    {
                    xsConcatenate(&qy->Clause," ORDER BY ", 10);
                    for(i=0;query->SortBy[i] && i < (sizeof(query->SortBy)/sizeof(void*));i++)
                        {
                        if (i != 0) xsConcatenate(&qy->Clause, ", ", 2);
                        mysd_internal_TreeToClause((pExpression)(query->SortBy[i]),&(qy->Data->TData),&qy->Clause,&escape_conn->Handle);
                        }
                    }
                mysd_internal_ReleaseConn(&escape_conn);
                }
            }
        else
            {
            query->Flags &= ~OBJ_QY_F_FULLQUERY;
            query->Flags &= ~OBJ_QY_F_FULLSORT;
            }
            
    return (void*)qy;
    }

/*** mysdQueryFetch() - get the next directory entry as an open object.
 ***/
void*
mysdQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pMysdQuery qy = ((pMysdQuery)(qy_v));
    pMysdData inf = NULL;
    MYSQL_RES * result = NULL;
    pMysdConn conn;
    char name_buf[(MYSD_NAME_LEN+1)*MYSD_MAX_KEYS];
    char* new_obj_name = NULL;

        /** Alloc the structure **/
        if(!(inf = (pMysdData)nmMalloc(sizeof(MysdData)))) return NULL;
	memset(inf, 0, sizeof(MysdData));
        inf->Pathname.OpenCtlBuf = NULL;
	inf->Row = NULL;
	inf->Result = NULL;

            /** Get the next name based on the query type. **/
        switch(qy->Data->Type)
            {
            case MYSD_T_DATABASE:
                if(qy->ItemCnt < qy->Data->Node->Tablenames.nItems)
                    {
                    inf->Type = MYSD_T_TABLE;
                    new_obj_name = qy->Data->Node->Tablenames.Items[qy->ItemCnt];
		    inf->TData = mysd_internal_GetTData(qy->Data->Node, new_obj_name);
                    }
                else
                    {
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                /** Get filename from the first column - table name. **/
                break;

            case MYSD_T_TABLE:
            /** Filename is either "rows" or "columns" **/
                if (qy->ItemCnt == 0) 
                    {
                    new_obj_name = "columns";
                    inf->Type = MYSD_T_COLSOBJ;
                    }
                else if (qy->ItemCnt == 1) 
                    {
                    new_obj_name = "rows";
                    inf->Type = MYSD_T_ROWSOBJ;
                    }
                else 
                    {
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                break;

            case MYSD_T_ROWSOBJ:
                /** Get the rows **/
                if (!(mysd_internal_GetNextRow(name_buf, sizeof(name_buf), qy, qy->Data, qy->Data->TData->Name)))
                    {
                    inf->Type = MYSD_T_ROW;
                    inf->Row = qy->Data->Row;
		    qy->Data->Row = NULL;
                    /*inf->Result = qy->Data->Result;*/
		    inf->Result = NULL;
                    new_obj_name = name_buf;
                    }
                else
                    {
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                break;

            case MYSD_T_COLSOBJ:
                /** return columns until they are all exhausted **/
                if(!(qy->Data->TData = mysd_internal_GetTData(qy->Data->Node,qy->Data->TData->Name)))
                    {
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                if (qy->ItemCnt < qy->Data->TData->nCols)
                    {
                    inf->Type = MYSD_T_COLUMN;
                    new_obj_name = qy->Data->TData->Cols[qy->ItemCnt];
                    }
                else
                    {
                    nmFree(inf,sizeof(MysdData));
                    return NULL;
                    }
                break;

        }

        /** Build the filename. **/
	if (obj_internal_AddToPath(obj->Pathname, new_obj_name) < 0)
	    return NULL;
#if 0
        if (strlen(new_obj_name) > 255) 
            {
            mssError(1,"MYSQL","Query result pathname exceeds internal representation");
            return NULL;
            }
        /** Set up a new element at the end of the pathname **/
        obj->Pathname->Elements[obj->Pathname->nElements] = obj->Pathname->Pathbuf + strlen(obj->Pathname->Pathbuf);
        obj->Pathname->nElements++;
        sprintf(obj->Pathname->Pathbuf,"%s\%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);
#endif

        obj_internal_CopyPath(&(inf->Pathname), obj->Pathname);
	strtcpy(inf->Objname, inf->Pathname.Elements[inf->Pathname.nElements - 1], sizeof(inf->Objname));
	inf->Name = inf->Objname;
        if (!inf->TData)
	    inf->TData = qy->Data->TData;
        inf->Node = qy->Data->Node;
        inf->Node->SnNode->OpenCnt++;
        inf->Obj = obj;
        qy->ItemCnt++;

    return (void*)inf;
    }


/*** mysdQueryDelete() - delete objects matching the query
 ***/
int
mysdQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    pMysdQuery qy = (pMysdQuery)qy_v;
    MYSQL_RES * result;

	/** Run the delete. **/
	result = mysd_internal_RunQuery(qy->Data->Node, "DELETE FROM `?` ?q", qy->Data->TData->Name, qy->Clause.String);
	if (result == MYSD_RUNQUERY_ERROR)
	    return -1;

    return 0;
    }


/*** mysdQueryClose() - close the query.
 ***/
int
mysdQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
        pMysdQuery qy = (pMysdQuery)qy_v;
        static MYSQL_RES * last_result = NULL;
        
        /** Free the last result and store the pointer to this query's result **/
        if(last_result) mysql_free_result(last_result);
        last_result = qy->Data->Result;
        qy->Data->Result = NULL;
        
        /** Free the structure **/
        nmFree(qy_v,sizeof(MysdQuery));

    return 0;
    }


/*** mysdGetAttrType() - get the type of an attribute by name.
 ***/
int
mysdGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    int i;
    pStructInf find_inf;

        /** If name, it's a string **/
        if (!strcmp(attrname,"name")) return DATA_T_STRING;

        /** If 'content-type', it's also a string. **/
        if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
        if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
        if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
        if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

        /** Check for attributes in the node object if that was opened **/
        if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
            {
            }

            /** Attr type depends on object type. **/
        if (inf->Type == MYSD_T_ROW)
            {
            for(i=0;i<inf->TData->nCols;i++)
                {
                if (!strcmp(attrname,inf->TData->Cols[i]))
                    {
                    return(inf->TData->ColCxTypes[i]);
                    }
                }
            }
	else if (inf->Type == MYSD_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    }

    return -1;
    }


/*** mysdGetAttrValue() - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
mysdGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;
    int ret = 1;
        if (!strcmp(attrname,"name"))
            {
            val->String = inf->Name;
            return 0;
            }

        if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
            {
            switch(inf->Type)
                {
                case MYSD_T_DATABASE: val->String = "system/void"; break;
                case MYSD_T_TABLE: val->String = "system/void"; break;
                case MYSD_T_ROWSOBJ: val->String = "system/void"; break;
                case MYSD_T_COLSOBJ: val->String = "system/void"; break;
                case MYSD_T_ROW: val->String = "application/octet-stream"; break;
                case MYSD_T_COLUMN: val->String = "system/void"; break;
                }
            return 0;
            }

        if (!strcmp(attrname,"outer_type"))
            {
            switch(inf->Type)
                {
                case MYSD_T_DATABASE: val->String = "application/mysql"; break;
                case MYSD_T_TABLE: val->String = "system/table"; break;
                case MYSD_T_ROWSOBJ: val->String = "system/table-rows"; break;
                case MYSD_T_COLSOBJ: val->String = "system/table-columns"; break;
                case MYSD_T_ROW: val->String = "system/row"; break;
                case MYSD_T_COLUMN: val->String = "system/column"; break;
                }
            return 0;
            }

        if (!strcmp(attrname,"annotation"))
            {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"MYSD","Type mismatch accessing attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** Different for various objects. **/
	    switch(inf->Type)
	        {
		case MYSD_T_DATABASE:
		    val->String = inf->Node->Description;
		    break;
		case MYSD_T_TABLE:
		    val->String = (inf->TData->Annotation)?(inf->TData->Annotation):"";
		    break;
		case MYSD_T_ROWSOBJ:
		    val->String = "Contains rows for this table";
		    break;
		case MYSD_T_COLSOBJ:
		    val->String = "Contains columns for this table";
		    break;
		case MYSD_T_COLUMN:
		    val->String = "Column within this table";
		    break;
		case MYSD_T_ROW:
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

        if (!strcmp(attrname,"last_modification"))
            {
            val->String = NULL;
            return 1;
            }        
        

        /** Column object?  Type is the only one. **/
        if (inf->Type == MYSD_T_COLUMN)
            {
            if (strcmp(attrname,"datatype")) return -1;
            if (datatype != DATA_T_STRING)
                {
                mssError(1,"MYSD","Type mismatch accessing attribute '%s' (should be string)", attrname);
                return -1;
                }


            /** Search table info for this column. **/
            for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->TData->Cols[i], inf->Name))
                {
                val->String = inf->TData->ColTypes[i];
                return 0;
                }
            }

        else if (inf->Type == MYSD_T_ROW)
            {
            for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->TData->Cols[i],attrname))
                {
                if(!inf->Row || !inf->Row[i])
                    {
                    return 1;
                    }
                else
                    {
                    switch(inf->TData->ColCxTypes[i])
                        {
                            case DATA_T_STRING:
                                val->String=inf->Row[i];
                                ret=0;
                                break;
                            case DATA_T_DATETIME:
                                val->DateTime=&(inf->Types.Date);
                                objDataToDateTime(DATA_T_STRING, inf->Row[i], val->DateTime, "ISO");
                                ret=0;
                                break;
                            case DATA_T_MONEY:
                                val->Money = &(inf->Types.Money);
                                objDataToMoney(DATA_T_STRING, inf->Row[i], val->Money);
                                ret=0;
                                break;
                            case DATA_T_INTEGER:
				if (!strcmp(inf->TData->ColTypes[i], "bit"))
				    {
				    if (inf->Row[i][0] == 0x01 || inf->Row[i][0] == '1')
					val->Integer = 1;
				    else
					val->Integer = 0;
				    }
				else
				    {
				    val->Integer=objDataToInteger(DATA_T_STRING, inf->Row[i], NULL);
				    }
                                ret=0;
                                break;
                            case DATA_T_DOUBLE:
                                val->Double = objDataToDouble(DATA_T_STRING, inf->Row[i]);
                                ret=0;
                                break;
                        }
                    }
                }
            return ret;
            }
        
        
        mssError(1,"MYSD","Could not locate requested attribute");

    return -1;
    }


/*** mysdGetNextAttr() - get the next attribute name for this object.
 ***/
char*
mysdGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);

        /** Attribute listings depend on object type. **/
        switch(inf->Type)
            {
            case MYSD_T_DATABASE:
                return NULL;

            case MYSD_T_TABLE:
                return NULL;

            case MYSD_T_ROWSOBJ:
                return NULL;

            case MYSD_T_COLSOBJ:
                return NULL;

            case MYSD_T_COLUMN:
                /** only attr is 'datatype' **/
                if (inf->CurAttr++ == 0) return "datatype";
                break;

            case MYSD_T_ROW:
                /** Return attr in table inf **/
                if (inf->CurAttr < inf->TData->nCols) return inf->TData->Cols[inf->CurAttr++];
                break;
            }

    return NULL;
    }


/*** mysdGetFirstAttr() - get the first attribute name for this object.
 ***/
char*
mysdGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    char* ptr;
        /** Set the current attribute. **/
        inf->CurAttr = 0;

        /** Return the next one. **/
        ptr = mysdGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** mysdSetAttrValue() - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
mysdSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    pStructInf find_inf;
    const int max_money_length = 64;
    char* tmp;
    int length;
    int i,j;
    int type;
    DateTime dt;
    ObjData od;

        type = mysdGetAttrType(inf, attrname, oxt);
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
                    mssError(1,"MYSQL","SetAttr 'name': name too large for internal representation");
                    return -1;
                    }
                obj_internal_CopyPath(&(inf->Pathname), inf->Obj->Pathname);
                strcpy(strrchr(inf->Pathname.Pathbuf,'/')+1,val->String);
                if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname.Pathbuf) < 0) 
                    {
                    mssError(1,"MYSQL","SetAttr 'name': could not rename structure file node object");
                    return -1;
                    }
                obj_internal_CopyPath(inf->Obj->Pathname, &inf->Pathname);
                }
            return 0;
            }

        /** Set content type if that was requested. **/
        if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
            {
            if (datatype != DATA_T_STRING)
                {
                mssError(1,"MYSD","Type mismatch accessing attribute '%s' (should be string)", attrname);
                return -1;
                }
            switch(inf->Type)
                {
                case MYSD_T_DATABASE: val->String = "system/void"; break;
                case MYSD_T_TABLE: val->String = "system/void"; break;
                case MYSD_T_ROWSOBJ: val->String = "system/void"; break;
                case MYSD_T_COLSOBJ: val->String = "system/void"; break;
                case MYSD_T_ROW: val->String = "application/octet-stream"; break;
                case MYSD_T_COLUMN: val->String = "system/void"; break;
                }
            return 0;
            }
        if (!strcmp(attrname,"outer_type"))
            {
            return -1;
            }

        if(inf->Type == MYSD_T_ROW)
            {
            for(i=0;i<inf->TData->nCols;i++) 
                {
                if (!strcmp(inf->TData->Cols[i],attrname))
                    {
		    if (datatype == DATA_T_STRING && type == DATA_T_DATETIME)
			{
			if(val) /** make sure val can be a NULL and still handle as datetime **/
			    {
			    /** Promote string to date time **/
			    objDataToDateTime(DATA_T_STRING, val->String, &dt, NULL);
			    od.DateTime = &dt;
			    val = &od;
			    }
			datatype = DATA_T_DATETIME;
			}
                    if (*oxt)
                        {
			/** This is part of a larger transaction, such as during an INSERT **/
                        if (type < 0) return -1;
                        if (datatype != type && val)
                            {
                            mssError(1,"MYSD","Type mismatch setting attribute '%s' [requested=%s, actual=%s]",
                                    attrname, obj_type_names[datatype], obj_type_names[type]);
                            return -1;
                            }
                        if (strlen(attrname) >= 64)
                            {
                            mssError(1,"MYSD","Attribute name '%s' too long",attrname);
                            return -1;
                            }
                        (*oxt)->AllocObj = 0;
                        (*oxt)->Object = NULL;
                        (*oxt)->Status = OXT_S_VISITED;
                        strcpy((*oxt)->AttrName, attrname);
                        obj_internal_SetTreeAttr(*oxt, type, val); 
                        }
                    else
                        {
			if (!val)
			    {
                            if(mysd_internal_UpdateRow(inf,NULL,i) < 0) return -1;
			    }
                        else if(datatype == DATA_T_DOUBLE || datatype == DATA_T_INTEGER) 
                            {
                            if(mysd_internal_UpdateRow(inf,mysd_internal_CxDataToMySQL(datatype,(ObjData*)val),i) < 0) return -1;
                            }
                        else
                            {
                            if(mysd_internal_UpdateRow(inf,mysd_internal_CxDataToMySQL(datatype,*(ObjData**)val),i) < 0) return -1;
                            }
                        }
                    }
                }
            }
    return 0;
    }


/*** mysdAddAttr() - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
mysdAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** mysdOpenAttr() - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
mysdOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** mysdGetFirstMethod() -- return name of First method available on the object.
 ***/
char*
mysdGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mysdGetNextMethod() -- return successive names of methods after the First one.
 ***/
char*
mysdGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mysdExecuteMethod() - Execute a method, by name.
 ***/
int
mysdExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** mysqlPresentationHints() - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
mysdPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    pObjPresentationHints hints=NULL;
    pExpression exp;
    int i;
    int datatype;
    char * ptr;
    pParamObjects tmplist;
    
        if (!strcmp(attrname, "name") || !strcmp(attrname, "inner_type") || !strcmp(attrname, "outer_type") ||
            !strcmp(attrname, "content_type") || !strcmp(attrname, "annotation") || !strcmp(attrname, "last_modification"))
            {
            if ( (hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
            memset(hints, 0, sizeof(ObjPresentationHints));
            hints->GroupID=-1;
            hints->VisualLength2=1;
            xaInit(&(hints->EnumList), 8);
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
            case MYSD_T_DATABASE: break;
            case MYSD_T_TABLE: break;
            case MYSD_T_ROWSOBJ: break;
            case MYSD_T_COLSOBJ: break;
            case MYSD_T_COLUMN:
                if (strcmp(attrname, "datatype"))
                    {
                    mssError(1, "MYSD", "No attribute %s", attrname);
                    return NULL;
                    }

                if ( (hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
                memset(hints, 0, sizeof(ObjPresentationHints));
                hints->GroupID=-1;
                hints->VisualLength2=1;
                hints->Style |= OBJ_PH_STYLE_READONLY;
                hints->StyleMask |= OBJ_PH_STYLE_READONLY;
                hints->VisualLength = 15;
                xaInit(&(hints->EnumList), 8);
                return hints;
                break;
            case MYSD_T_ROW:
                /** the attributes of a row are the column names, with the values being the field values **/
                /** find the name of the column, and get its data type **/
                for (i=0;i<inf->TData->nCols;i++) 
                    {
                    if (!strcmp(attrname, inf->TData->Cols[i]))
                        {
                        /** Check to see if we already know the hints **/
                        if(inf->TData->ColHints[i]) return objDuplicateHints(inf->TData->ColHints[i]);

                        /** Otherwise compile them and add them to the table data **/
                        if ( (hints = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints))) == NULL) return NULL;
                        memset(hints, 0, sizeof(ObjPresentationHints));
                        datatype = inf->TData->ColCxTypes[i];
                        hints->GroupID=-1;
                        hints->VisualLength2=1;
                        xaInit(&(hints->EnumList), 8);

			/** Note whether it is a primary key field **/
			if (inf->TData->ColFlags[i] & MYSD_COL_F_PRIKEY)
			    hints->Style |= OBJ_PH_STYLE_KEY;
			hints->StyleMask |= OBJ_PH_STYLE_KEY;

			/** Set some hints info based on data type information **/
                        if (datatype == DATA_T_STRING)
                            {
                            /** use the length that we pulled when we grabbed the table data **/
                            if (hints->Length == 0) hints->Length = inf->TData->ColLengths[i];
                            if (hints->VisualLength == 0) hints->VisualLength = inf->TData->ColLengths[i];
                            }
                        if (datatype == DATA_T_MONEY)
                            {
                            hints->Format=nmSysStrdup("money");
                            }
                        if (datatype == DATA_T_DATETIME)
                            {
                             hints->Format=nmSysStrdup("datetime");
                            }
                        if (datatype == DATA_T_INTEGER)
                            {
                            /** use the sizes from the MySQL datatypes **/
                            tmplist = expCreateParamList();
                            expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
                            hints->DefaultExpr = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                            if(!strcmp(inf->TData->ColTypes[i], "tinyint"))
                                {
                                if(inf->TData->ColFlags[i] & MYSD_COL_F_UNSIGNED)
                                    {
                                    hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("255", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                else
                                    {
                                    hints->MinValue = expCompileExpression("-128", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("127", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                }
                            if(!strcmp(inf->TData->ColTypes[i], "smallint"))
                                {
                                if(inf->TData->ColFlags[i] & MYSD_COL_F_UNSIGNED)
                                    {
                                    hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("65535", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                else
                                    {
                                    hints->MinValue = expCompileExpression("-32768", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("32767", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                }
                            if(!strcmp(inf->TData->ColTypes[i], "mediumint"))
                                {
                                if(inf->TData->ColFlags[i] & MYSD_COL_F_UNSIGNED)
                                    {
                                    hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("16777215", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                else
                                    {
                                    hints->MinValue = expCompileExpression("-8388608", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("8388607", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                }
                            if(!strcmp(inf->TData->ColTypes[i], "int"))
                                {
                                if(inf->TData->ColFlags[i] & MYSD_COL_F_UNSIGNED)
                                    {
                                    hints->MinValue = expCompileExpression("0", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("4294967295", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                else
                                    {
                                    hints->MinValue = expCompileExpression("-2147483648", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    hints->MaxValue = expCompileExpression("2147483647", tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
                                    }
                                }
                            expFreeParamList(tmplist);
                            }
                        inf->TData->ColHints[i] = objDuplicateHints(hints);
                        break;
                        }
                    }
                if (i == inf->TData->nCols)
                    {
                    mssError(1, "MYSD", "No attribute '%s'", attrname);
                    return NULL;
                    }
                break;
            default:
                mssError(1, "MYSD", "Can't get hints for that type yet");
                break;
            }

        return hints;
    }


/*** mysqlInfo() - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
mysdInfo(void* inf_v, pObjectInfo info_struct)
    {
    memset(info_struct, sizeof(ObjectInfo), 0);
    return 0;
    }


/*** mysqlCommit() - commit any changes made to the underlying data source.
 ***/
int
mysdCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pMysdData inf = MYSD(inf_v);
    struct stat fileinfo;
    int i;
    char sbuf[160];

        /** Was this a create? **/
        if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
            {
            switch (inf->Type)
                {
                case MYSD_T_TABLE:
                    /** We'll get to this a little later **/
                    break;

                case MYSD_T_ROW:
                    /** Perform the insert. **/
                    if (mysd_internal_InsertRow(inf, *oxt) < 0)
                        {
                        /** FAIL the oxt. **/
                        (*oxt)->Status = OXT_S_FAILED;

                        return -1;
                        }

                    /** Complete the oxt. **/
                    (*oxt)->Status = OXT_S_COMPLETE;
                    break;

                case MYSD_T_COLUMN:
                    break;
                }
            }

    return 0;
    }


/*** mysqlInitialize() - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
mysdInitialize()
    {
    pObjDriver drv;

        /** Allocate the driver **/
        drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
        if (!drv) return -1;
        memset(drv, 0, sizeof(ObjDriver));

        /** Initialize globals **/
        memset(&MYSD_INF,0,sizeof(MYSD_INF));
        xhInit(&MYSD_INF.DBNodesByPath,255,0);
        xaInit(&MYSD_INF.DBNodeList,127);

        /** Init mysql client library **/
        if (mysql_library_init(0, NULL, NULL) != 0)
            {
            mssError(1, "MYSQL", "Could not init mysql client library");
            return -1;
            }

        /** Setup the structure **/
        strcpy(drv->Name,"MySQL ObjectSystem Driver");
        drv->Capabilities = OBJDRV_C_TRANS | OBJDRV_C_FULLQUERY;
        xaInit(&(drv->RootContentTypes),16);
        xaAddItem(&(drv->RootContentTypes),"application/mysql");

        /** Setup the function references. **/
        drv->Open = mysdOpen;
        drv->Close = mysdClose;
        drv->Create = mysdCreate;
        drv->Delete = mysdDelete;
        drv->DeleteObj = mysdDeleteObj;
        drv->OpenQuery = mysdOpenQuery;
        drv->QueryDelete = mysdQueryDelete;
        drv->QueryFetch = mysdQueryFetch;
        drv->QueryClose = mysdQueryClose;
        drv->Read = mysdRead;
        drv->Write = mysdWrite;
        drv->GetAttrType = mysdGetAttrType;
        drv->GetAttrValue = mysdGetAttrValue;
        drv->GetFirstAttr = mysdGetFirstAttr;
        drv->GetNextAttr = mysdGetNextAttr;
        drv->SetAttrValue = mysdSetAttrValue;
        drv->AddAttr = mysdAddAttr;
        drv->OpenAttr = mysdOpenAttr;
        drv->GetFirstMethod = mysdGetFirstMethod;
        drv->GetNextMethod = mysdGetNextMethod;
        drv->ExecuteMethod = mysdExecuteMethod;
        drv->PresentationHints = mysdPresentationHints;
        drv->Info = mysdInfo;
        drv->Commit = mysdCommit;

        nmRegister(sizeof(MysdData),"MysdData");
        nmRegister(sizeof(MysdQuery),"MysdQuery");

        /** Register the driver **/
        if (objRegisterDriver(drv) < 0) 
            {
            return -1;
            }
    return 0;
    }
    
MODULE_INIT(mysdInitialize);
MODULE_PREFIX("mysd");
MODULE_DESC("MySQL ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
