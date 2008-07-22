#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
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

/**CVSDATA***************************************************************

    $Id: objdrv_mysql.c,v 1.1 2008/07/22 00:22:16 jncraton Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_mysql.c,v $

    $Log: objdrv_mysql.c,v $
    Revision 1.1  2008/07/22 00:22:16  jncraton
    - Initial integration of the MySQL driver
    - The driver is far from complete and shouldn't be used for anything
    - It is currently missing huge amounts of functionality


 **END-CVSDATA***********************************************************/

#define MYSQLD_MAX_COLS                256
#define MYSQLD_MAX_KEYS                8
#define MYSQLD_NAME_LEN                32
#define MYSD_MAX_CONN                16


/*** Node ***/
typedef struct
    {
    char        Path[OBJSYS_MAX_PATH];
    char        Server[64];
    char        Database[MYSQLD_NAME_LEN];
    char        AnnotTable[MYSQLD_NAME_LEN];
    char        Description[256];
    int                MaxConn;
    pSnNode        SnNode;
    XArray        Conns;
    int                ConnAccessCnt;
    XHashTable        Tables;
    XArray        Tablenames;
    int                LastAccess;
    }
    MysqlNode, *pMysqlNode;


/*** Connection data ***/
typedef struct
    {
    MYSQL        Handle;
    char        Username[64];
    char        Password[64];
    pMysqlNode        Node;
    int                Busy;
    int                LastAccess;
    }
    MysqlConn, *pMysqlConn;


/*** Table data ***/
typedef struct
    {
    char        Name[MYSQLD_NAME_LEN];
    pMysqlNode        Node;
    char*        Cols[MYSQLD_MAX_COLS];
    unsigned char ColFlags[MYSQLD_MAX_COLS];
    char*        ColTypes[MYSQLD_MAX_COLS];
    unsigned char ColCxTypes[MYSQLD_MAX_COLS];
    unsigned char ColKeys[MYSQLD_MAX_COLS];
    unsigned short ColLengths[MYSQLD_MAX_COLS];
    int                nCols;
    char*        Keys[MYSQLD_MAX_KEYS];
    int                KeyCols[MYSQLD_MAX_KEYS];
    int                nKeys;
    }
    MysqlTable, *pMysqlTable;

#define MYSQLD_COL_F_NULL        1        /* column allows nulls */
#define MYSQLD_COL_F_PRIKEY        2        /* column is part of primary key */


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    Pathname        Pathname;
    int                Type;
    int                Flags;
    pObject        Obj;
    int                Mask;
    int                CurAttr;
    pMysqlNode        Node;
    char*        RowBuf;
    MYSQL_ROW        Row;
    MYSQL_RES *     Result;
    char*        ColPtrs[MYSQLD_MAX_COLS];
    unsigned short ColLengths[MYSQLD_MAX_COLS];
    pMysqlTable        TData;
    }
    MysqlData, *pMysqlData;

#define MYSD_T_DATABASE        1
#define MYSD_T_TABLE                2
#define MYSD_T_COLSOBJ        3
#define MYSD_T_ROWSOBJ        4
#define MYSD_T_COLUMN                5
#define MYSD_T_ROW                6


#define MYSQLD(x) ((pMysqlData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pMysqlData        Data;
    char        NameBuf[256];
    int                ItemCnt;
    }
    MysqlQuery, *pMysqlQuery;


/*** GLOBALS ***/
struct
    {
    XHashTable                DBNodesByPath;
    XArray                DBNodeList;
    int                        AccessCnt;
    int                        LastAccess;
    }
    MYSQLD_INF;


/*** mysd_internal_GetConn() - given a specific database node, get a connection
 *** to the server with a given login (from mss thread structures).
 ***/
pMysqlConn
mysd_internal_GetConn(pMysqlNode node)
    {
    pMysqlConn conn;
    int i, conn_cnt, found;
    int min_access;
    char* username;
    char* password;

        /** Is one available from the node's connection pool? **/
        conn_cnt = xaCount(&node->Conns);
        username = mssUserName();
        password = mssPassword();
        conn = NULL;
        for(i=0;i<conn_cnt;i++)
            {
            conn = (pMysqlConn)xaGetItem(&node->Conns, i);
            if (!conn->Busy && !strcmp(username, conn->Username) && !strcmp(password, conn->Password))
                break;
            conn = NULL;
            }

        /** Didn't get one? **/
        if (!conn)
            {
            if (conn_cnt < node->MaxConn)
                {
                /** Below pool maximum?  Alloc if so **/
                conn = (pMysqlConn)nmMalloc(sizeof(MysqlConn));
                if (!conn) return NULL;
                conn->Node = node;
                }
            else
                {
                /** Try to free and reuse a connection **/
                min_access = 0x7fffffff;
                found = -1;
                for(i=0;i<conn_cnt;i++)
                    {
                    conn = (pMysqlConn)xaGetItem(&node->Conns, i);
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
                mysql_close(&conn->Handle);
                memset(conn->Password, 0, sizeof(conn->Password));
                xaRemoveItem(&node->Conns, found);
                }

            /** Attempt connection **/
            if (mysql_init(&conn->Handle) == NULL)
                {
                mssError(1, "MYSD", "Memory exhausted");
                nmFree(conn, sizeof(MysqlConn));
                return NULL;
                }
            if (mysql_real_connect(&conn->Handle, node->Server, username, password, node->Database, 0, NULL, 0) == NULL)
                {
                mssError(1, "MYSD", "Could not connect to MySQL server [%s], DB [%s]: %s",
                        node->Server, node->Database, mysql_error(&conn->Handle));
                mysql_close(&conn->Handle);
                nmFree(conn, sizeof(MysqlConn));
                return NULL;
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
 *** pool.  Also sets the pointer to NULL.
 ***/
void
mysd_internal_ReleaseConn(pMysqlConn * conn)
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
*** There are some differences:
*** ?a => array, params for this are (char** array, int length, char separator
***     it will become: item1,item2,items3 (length = 3, separator = ',')
*** '?a' => same as array except with individual quoting specified
***     it will become: 'item1','item2','items3' (length = 3, separator = ',')
*** All parameters are sanatized with mysql_real_escape_string before
*** the query is built
*** This WILL NOT WORK WITH BINARY DATA
 ***/
MYSQL_RES*
mysd_internal_RunQuery(pMysqlNode node, char* stmt, ...)
    {
    MYSQL_RES * result = NULL;
    pMysqlConn conn = NULL;
    pXString query = NULL;
    va_list ap;
    int i, j;
    int length = 0;
    char * start;
    char ** array = NULL;
    int items = 0;
    char separator;
    char quote = 0x00;
    char tmp[32];

        /**start up ap and create query XString **/
        
        va_start(ap,stmt);
        query = nmMalloc(sizeof(XString));
        xsInit(query);

        if(!(conn = mysd_internal_GetConn(node))) goto error;
        
        length=-1;
        start=stmt;
        for(i = 0; stmt[i]; i++)
            {
            length++;
            if(stmt[i] == '?')
                {
                /** throw on everything new that is just constant **/
                if(xsConcatenate(query,start,length)) goto error;
                /** do the insertion **/
                if(stmt[i+1]=='a') /** handle arrays **/
                    {
                    array = va_arg(ap,char**);
                    items = va_arg(ap,int);
                    separator = va_arg(ap,int);
                    if(stmt[i+2] == '\'' || stmt[i+2]=='`') quote = stmt[i+2]; else quote = 0x00;
                    for(j = 0; j < items; j++)
                        {
                        if(j > 0) 
                            {
                            if(quote) if(xsConcatenate(query,&quote,1)) goto error;
                            if(xsConcatenate(query,&separator,1)) goto error;
                            if(quote) if(xsConcatenate(query,&quote,1)) goto error;
                            }
                        if(mysd_internal_SafeAppend(&conn->Handle,query,array[j])) goto error;
                        }
                    start = &stmt[i+2];
                    i++;
                    }
                else if(stmt[i+1]=='d') /** handle integers **/
                    {
                        sprintf(tmp,"%d",va_arg(ap,int));
                        if(mysd_internal_SafeAppend(&conn->Handle,query,tmp)) goto error;
                        start = &stmt[i+2];
                        i++;
                    }
                else /** handle plain sanatize+insert **/
                    {
                    if(mysd_internal_SafeAppend(&conn->Handle,query,va_arg(ap,char*))) goto error;
                    start = &stmt[i+1];
                    }
                length = -1;
                }
            }
        /** insert the last constant bit **/
        if(xsConcatenate(query,start,-1)) goto error;

        //printf("TEST: query=\"%s\"\n",query->String);

        va_end(ap);


        if(mysql_query(&conn->Handle,query->String)) goto error;
        if(!(result = mysql_store_result(&conn->Handle))) goto error;

        error:
            if(conn) mysd_internal_ReleaseConn(&conn);
            if(query) 
                {
                xsDeInit(query);
                nmFree(query,sizeof(XString));
                }
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
    

/*** mysd_internal_ParseTData() - given a mysql result set, parse the table
 *** data into a MysqlTable structure.  Returns < 0 on failure.
 ***/
int
mysd_internal_ParseTData(MYSQL_RES *resultset, int rowcnt, pMysqlTable tdata)
    {
    int i;
    MYSQL_ROW row;
    char data_desc[32];
    char* pptr;
    char* cptr;
    int len;

        if (rowcnt > MYSQLD_MAX_COLS)
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
                tdata->ColLengths[tdata->nCols] = strtol(pptr+1, NULL, 10);
                if ((cptr = strchr(pptr+1, ',')) != NULL)
                    len = strtol(cptr+1, NULL, 10);
                }

            /** Type **/
            tdata->ColTypes[tdata->nCols] = nmSysStrdup(data_desc);
            if (!strcmp(data_desc, "int") || !strcmp(data_desc, "tinyint") || !strcmp(data_desc, "smallint") || !strcmp(data_desc, "mediumint") || !strcmp(data_desc, "bit"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_INTEGER;
            else if (!strcmp(data_desc, "char") || !strcmp(data_desc, "varchar"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_STRING;
            else if (!strcmp(data_desc, "float") || !strcmp(data_desc, "double"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_DOUBLE;
            else if (!strcmp(data_desc, "datetime") || !strcmp(data_desc, "date") || !strcmp(data_desc, "timestamp"))
                tdata->ColCxTypes[tdata->nCols] = DATA_T_DATETIME;
            else
                tdata->ColCxTypes[tdata->nCols] = DATA_T_UNAVAILABLE;

            /** Allow nulls **/
            tdata->ColFlags[i] = 0;
            if (row[2] && !strcmp(row[2], "YES"))
                tdata->ColFlags[i] |= MYSQLD_COL_F_NULL;

            /** Primary Key **/
            if (row[3] && !strcmp(row[3], "PRI"))
                {
                if (tdata->nKeys >= MYSQLD_MAX_KEYS)
                    {
                    mssError(1,"MYSD","Too many key fields in table [%s]",tdata->Name);
                    return -1;
                    }
                tdata->Keys[tdata->nKeys] = tdata->Cols[tdata->nCols];
                tdata->KeyCols[tdata->nKeys] = tdata->nCols;
                tdata->ColKeys[tdata->nCols] = tdata->nKeys;
                tdata->nKeys++;
                tdata->ColFlags[i] |= MYSQLD_COL_F_PRIKEY;
                }
            tdata->nCols++;
            }

    return 0;
    }

/*** mysd_internal_GetTData() - get a table information structure
 ***/
pMysqlTable
mysd_internal_GetTData(pMysqlNode node, char* tablename)
    {
    MYSQL_RES * result = NULL;
    pMysqlTable tdata = NULL;
    int length;
    int rowcnt;

        /** Value cached already? **/
        tdata = (pMysqlTable)xhLookup(&node->Tables, tablename);
        if (tdata)
            return tdata;

        /** sanatize the table name and build the query**/
        length = strlen(tablename);
        /** this next bit will break any charset not ASCII **/ 
        if(strchr(tablename,'`')) /** throw an error if some joker tries to throw in a backtick **/
            goto error;
        result = mysd_internal_RunQuery(node, "SHOW COLUMNS FROM `?`",tablename);
        if (!result)
            goto error;
        rowcnt = mysql_num_rows(result);
        if (rowcnt <= 0)
            goto error;

        /** Build the TData **/
        tdata = (pMysqlTable)nmMalloc(sizeof(MysqlTable));
        if (!tdata)
            goto error;
        memset(tdata, 0, sizeof(MysqlTable));
        strtcpy(tdata->Name, tablename, sizeof(tdata->Name));
        tdata->Node = node;
        if (mysd_internal_ParseTData(result, rowcnt, tdata) < 0)
            {
            nmFree(tdata, sizeof(MysqlTable));
            tdata = NULL;
            }

    error:
        if (result)
            mysql_free_result(result);
        if(tdata) xhAdd(&(node->Tables),tdata->Name,(char*)tdata);
    return tdata;
    }

/*** mysd_internal_GetRow() - get a given row from the database
 ***/

int
mysd_internal_GetRow(char* filename, pMysqlData data, char* tablename, int row_num)
    {
    MYSQL_RES * result = NULL;
    MYSQL_ROW row = NULL;
    int i = 0;
    int ret = 0;

        if(!(data->TData)) return -1;

        if(!(result = mysd_internal_RunQuery(data->Node,"SELECT * FROM `?` LIMIT ?d,1",data->TData->Name,row_num))) ret = -1;

        filename[0] = 0x00;
        if(result && (data->Row = mysql_fetch_row(result)))
            {
            for(i = 0; i<data->TData->nKeys; i++)
                {
                sprintf(filename,"%s%s|",filename,data->Row[i]);
                data->Result=result;
                }
            /** kill the trailing pipe **/
            filename[strlen(filename)-1]=0x00;
            }
        else
            {
            data->Result=result;
            ret = -1;
            }

        return ret;
    }

/*** mysd_internal_UpdateRow() - update a given row
 ***/
int
mysd_internal_UpdateRow(pMysqlData data, char* newval, int col)
    {
    pMysqlConn conn = NULL;
    int i = 0;
    char* filename;
    char tablename[MYSQLD_NAME_LEN];

        /** get the filename from the path **/
        filename = data->Pathname.Elements[data->Pathname.nElements - 1]; /** pkey|pkey... **/
        
        /** get the table name from the path **/
        i = (data->Pathname.Elements[data->Pathname.nElements - 2] - data->Pathname.Elements[data->Pathname.nElements - 3]);
        memcpy(tablename, data->Pathname.Elements[data->Pathname.nElements - 3], i); 
        tablename[i-1]=0x00; /** clean trailing slash **/
        
        if(!mysd_internal_RunQuery(data->Node,"UPDATE `?` SET `?`='?' WHERE CONCAT_WS('|',`?a`)='?'",tablename,data->TData->Cols[col],newval,data->TData->Keys,data->TData->nKeys,',',filename)) return -1;
        return 0;
    }

/*** mysd_internal_InsertRow() - update a given row
 ***/
int
mysd_internal_InsertRow(pMysqlData inf, pObjTrxTree oxt)
    {
    char* kptr;
    char* kendptr;
    int i,j,len,ctype,restype;
    pObjTrxTree attr_oxt, find_oxt;
    pXString insbuf;
    char* values[MYSQLD_MAX_COLS];
    char* cols[MYSQLD_MAX_COLS];

        /** Allocate a buffer for our insert statement. **/
        insbuf = (pXString)nmMalloc(sizeof(XString));
        if (!insbuf)
            {
            return -1;
            }
        xsInit(insbuf);

        /** Ok, look for the attribute sub-OXT's **/
        for(j=0;j<inf->TData->nCols;j++)
            {
            /** we scan through the OXT's **/
            find_oxt=NULL;
            for(i=0;i<oxt->Children.nItems;i++)
                {
                attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
                if (attr_oxt->OpType == OXT_OP_SETATTR)
                    {
                    if (!strcmp(attr_oxt->AttrName,inf->TData->Cols[j]))
                        {
                        find_oxt = attr_oxt;
                        find_oxt->Status = OXT_S_COMPLETE;
                        //break;
                        }
                    }
                }
            if (j!=0) xsConcatenate(insbuf,"\0",1);

            /** Print the appropriate type. **/
            if (!find_oxt)
                {
                if (inf->TData->ColFlags[j] & MYSQLD_COL_F_NULL)
                    {
                    values[j] = (char*)insbuf->Length;
                    xsConcatenate(insbuf,"NULL",4);
                    }
                else if (inf->TData->ColCxTypes[j] == 19 || inf->TData->ColCxTypes[j] == 20)
                    {
                    values[j] = (char*)insbuf->Length;
                    xsConcatenate(insbuf,"",0);
                    }
                else
                    {
                    mssError(1,"MYSD","Required column '%s' not specified in object create", inf->TData->Cols[j]);
                    xsDeInit(insbuf);
                    nmFree(insbuf,sizeof(XString));
                    return -1;
                    }
                }
            else 
                {
                values[j] = (char*)insbuf->Length;
                objDataToString(insbuf, find_oxt->AttrType, find_oxt->AttrValue, 0);
                }
            }

        i = 0;
        for(j=0;j<inf->TData->nCols;j++)
            {
            values[j] = (char*)((unsigned int)values[j] + (unsigned int)insbuf->String);
            if(strcmp(values[j],"NULL"))
                {
                cols[i] = inf->TData->Cols[j];
                values[i] = values[j];
                i++;
                }
            }

        mysd_internal_RunQuery(inf->Node,"INSERT INTO `?` (`?a`) VALUES('?a')",inf->TData->Name,cols,i,',',values,i,',');
        
        /** clean up **/
        xsDeInit(insbuf);
        nmFree(insbuf,sizeof(XString));

    return 0;
    }

/*** mysd_internal_GetTablenames() - throw the table names in an Xarray
 ***/
int
mysd_internal_GetTablenames(pMysqlNode node)
    {
    MYSQL_RES * result;
    MYSQL_ROW row;
    pMysqlConn conn;
    int nTables;
    char* tablename;
    
        if(!(result = mysd_internal_RunQuery(node,"SHOW TABLES"))) return -1;
        nTables = mysql_num_rows(result);
        xaInit(&node->Tablenames,nTables);
        
        while((row = mysql_fetch_row(result)))
            {
            if(!(tablename=nmMalloc(MYSQLD_NAME_LEN))) {mysql_free_result(result); return -1;}
            memcpy(tablename, row[0], strlen(row[0]) + 1);
            if(xaAddItem(&node->Tablenames,row[0]) < 0) {mysql_free_result(result); return -1;};
            }
    
    mysql_free_result(result);
    return 0;
    }

/*** mysd_internal_OpenNode() - access the node object and get the mysql
 *** server connection parameters.
 ***/
pMysqlNode
mysd_internal_OpenNode(char* path, int mode, pObject obj, int node_only, int mask)
    {
    pMysqlNode db_node;
    pSnNode sn_node;
    char* ptr = NULL;
    int i;

        /** First, do a lookup in the db node cache. **/
        db_node = (pMysqlNode)xhLookup(&(MYSQLD_INF.DBNodesByPath), path);
        if (db_node) 
            {
            db_node->LastAccess = MYSQLD_INF.AccessCnt;
            MYSQLD_INF.AccessCnt++;
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
        db_node = (pMysqlNode)nmMalloc(sizeof(MysqlNode));
        if (!db_node)
            {
            mssError(0,"MYSQL","Could not allocate DB node structure");
            return NULL;
            }
        memset(db_node,0,sizeof(MysqlNode));
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
        db_node->MaxConn = i;
        xaInit(&(db_node->Conns),16);
        xhInit(&(db_node->Tables),255,0);
        db_node->ConnAccessCnt = 0;
        
        /** Get table names **/
        if((i=mysd_internal_GetTablenames(db_node)) < 0)
            {
            mssError(0,"MYSD","Unable to query for table names");
            nmFree(db_node,sizeof(MysqlNode));
            return NULL;
            }

        /** Add node to the db node cache **/
        xhAdd(&(MYSQLD_INF.DBNodesByPath), db_node->Path, (void*)db_node);
        xaAddItem(&MYSQLD_INF.DBNodeList, (void*)db_node);

    return db_node;
    }

/*** mysd_internal_GetCxType - convert a mysql usertype to a centrallix datatype.
 *** Like GetDefaultHints, it might not be a bad idea to push this to a file somehow,
 *** to make the handling of types more flexable (and allow for user-defined types)
 ***/
int
mysd_internal_GetCxType(int ut)
    {
        if (ut == 1 || ut == 2 || ut == 18 || ut == 19 || ut == 25) return DATA_T_STRING;
        if (ut == 5 || ut == 6 || ut == 7 || ut == 16) return DATA_T_INTEGER;
        if (ut == 12 || ut == 22) return DATA_T_DATETIME;
        if (ut == 8 || ut == 23) return DATA_T_DOUBLE;
        if (ut == 11 || ut == 21) return DATA_T_MONEY;

        mssError(1, "SYBD", "the usertype %d is not supported by Centrallix");
        return -1;
    }

/*** sybd_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
mysd_internal_DetermineType(pObject obj, pMysqlData inf)
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
            obj->SubCnt = 3;
            }
        if (inf->Pathname.nElements - 3 >= obj->SubPtr)
            {
            if (inf->Type == MYSD_T_ROWSOBJ) inf->Type = MYSD_T_ROW;
            else if (inf->Type == MYSD_T_COLSOBJ) inf->Type = MYSD_T_COLUMN;
            obj->SubCnt = 4;
            }

    return 0;
    }
    
/*** mysdOpen - open an object.
 ***/
void*
mysdOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pMysqlData inf;
    int rval;
    int length;
    int cnt;
    char* tablename;
    char* node_path;
    char* table;
    char* ptr;

        /** Allocate the structure **/
        inf = (pMysqlData)nmMalloc(sizeof(MysqlData));
        if (!inf) return NULL;
        memset(inf,0,sizeof(MysqlData));
        inf->Obj = obj;
        inf->Mask = mask;
        inf->Result = NULL;

        /** Determine the type **/
        mysd_internal_DetermineType(obj,inf);
        
        /** Determine the node path **/
        node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

        /** this leaks memory MUST FIX **/
        if(!(inf->Node = mysd_internal_OpenNode(node_path, obj->Mode, obj, inf->Type == MYSD_T_DATABASE, inf->Mask)))
            {
            nmFree(inf,sizeof(MysqlData));
            return NULL;
            }
        
        /** get table name from path **/
        if(obj->SubCnt > 1)
            {
            tablename = obj_internal_PathPart(obj->Pathname, 2, obj->SubPtr-1);
            length = obj->Pathname->Elements[obj->Pathname->nElements - 1] - obj->Pathname->Elements[obj->Pathname->nElements - 2];
            if(!(inf->TData = mysd_internal_GetTData(inf->Node,tablename)))
                {
                nmFree(inf,sizeof(MysqlData));
                return NULL;
                }
            }
        
        /** Set object params. **/
        obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
        inf->Node->SnNode->OpenCnt++;

        if(inf->Type == MYSD_T_ROW)
            {
            cnt = 0;
            /** Autonaming a new object?  Won't be able to look it up if so. **/
            // if (!(inf->Obj->Mode & OBJ_O_AUTONAME))
                // {
                // if ((cnt = sybd_internal_LookupRow(inf->SessionID, inf)) < 0)
                    // {
                    // sybd_internal_ReleaseConn(inf->Node, inf->SessionID);
                    // nmFree(inf,sizeof(MysqlData));
                    // return NULL;
                    // }
                // }
            if (cnt == 0)
                {
                /** User specified a row that doesn't exist. **/
                if (!(obj->Mode & O_CREAT))
                    {
                    mssError(1,"MYSD","Row object does not exist.");
                    nmFree(inf,sizeof(MysqlData));
                    return NULL;
                    }

                /** Ok - row insert.  Release the conn and return.  Do the work on close(). **/
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


/*** mysdClose - close an open object.
 ***/
int
mysdClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);

        /** commit changes before we get rid of the transaction **/
        mysdCommit(inf,oxt);

            /** Write the node first, if need be. **/
        snWriteNode(inf->Obj->Prev, inf->Node->SnNode);
        
        /** Release the memory **/
        inf->Node->SnNode->OpenCnt --;
        if(inf->Result) mysql_free_result(inf->Result);
        if(inf->Pathname.OpenCtlBuf) nmFree(inf->Pathname.OpenCtlBuf,inf->Pathname.OpenCtlLen);
        nmFree(inf,sizeof(MysqlData));

    return 0;
    }


/*** mysqlCreate - create a new object, without actually returning a
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


/*** mysqlDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
mysdDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pMysqlData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

            /** Open the thing first to get the inf ptrs **/
        obj->Mode = O_WRONLY;
        inf = (pMysqlData)mysdOpen(obj, 0, NULL, "", oxt);
        if (!inf) return -1;

        /** Check to see if user is deleting the 'node object'. **/
        if (obj->Pathname->nElements == obj->SubPtr)
            {
            if (inf->Node->SnNode->OpenCnt > 1) 
                {
                mysdClose(inf, oxt);
                mssError(1,"MYSQL","Cannot delete structure file: object in use");
                return -1;
                }

            /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
            /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
            is_empty = 0;
            if (!is_empty)
                {
                mysdClose(inf, oxt);
                mssError(1,"MYSQL","Cannot delete: object not empty");
                return -1;
                }
            stFreeInf(inf->Node->SnNode->Data);

            /** Physically delete the node, and then remove it from the node cache **/
            unlink(inf->Node->SnNode->NodePath);
            snDelete(inf->Node->SnNode);
            }
        else
            {
            /** Delete of sub-object processing goes here **/
            }

        /** Release, don't call close because that might write data to a deleted object **/
        nmFree(inf,sizeof(MysqlData));

    return 0;
    }


/*** mysqlRead - Structure elements have no content.  Fails.
 ***/
int
mysdRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    return -1;
    }


/*** mysdWrite - Again, no content.  This fails.
 ***/
int
mysdWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    return -1;
    }


/*** mysdOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
mysdOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    pMysqlQuery qy;
        /** Allocate the query structure **/
        qy = (pMysqlQuery)nmMalloc(sizeof(MysqlQuery));
        if (!qy) return NULL;
        memset(qy, 0, sizeof(MysqlQuery));
        qy->Data = inf;
        qy->ItemCnt = 0;
    
    return (void*)qy;
    }

/*** mysqlQueryFetch - get the next directory entry as an open object.
 ***/
void*
mysdQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pMysqlQuery qy = ((pMysqlQuery)(qy_v));
    pMysqlData inf = NULL;
    MYSQL_RES * result = NULL;
    pMysqlConn conn;
    char name_buf[(MYSQLD_NAME_LEN+1)*MYSQLD_MAX_KEYS];
    char* new_obj_name = NULL;

        /** Alloc the structure **/
        if(!(inf = (pMysqlData)nmMalloc(sizeof(MysqlData)))) return NULL;
        inf->Pathname.OpenCtlBuf = NULL;
            /** Get the next name based on the query type. **/
        switch(qy->Data->Type)
            {
            case MYSD_T_DATABASE:
                if(qy->ItemCnt < qy->Data->Node->Tablenames.nItems)
                    {
                    inf->Type = MYSD_T_TABLE;
                    new_obj_name = qy->Data->Node->Tablenames.Items[qy->ItemCnt];
                    }
                else
                    {
                    nmFree(inf,sizeof(MysqlData));
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
                    nmFree(inf,sizeof(MysqlData));
                    return NULL;
                    }
                break;

            case MYSD_T_ROWSOBJ:
                /** Get the rows **/
                if (!(mysd_internal_GetRow(name_buf,qy->Data,qy->Data->TData->Name,qy->ItemCnt)))
                    {
                    inf->Type = MYSD_T_ROW;
                    inf->Row = qy->Data->Row;
                    inf->Result = qy->Data->Result;
                    new_obj_name = name_buf;
                    }
                else
                    {
                    nmFree(inf,sizeof(MysqlData));
                    return NULL;
                    }
                break;

            case MYSD_T_COLSOBJ:
                /** return columns until they are all exhausted **/
                if(!(qy->Data->TData = mysd_internal_GetTData(qy->Data->Node,qy->Data->TData->Name)))
                    {
                    nmFree(inf,sizeof(MysqlData));
                    return NULL;
                    }
                if (qy->ItemCnt < qy->Data->TData->nCols)
                    {
                    inf->Type = MYSD_T_COLUMN;
                    new_obj_name = qy->Data->TData->Cols[qy->ItemCnt];
                    }
                else
                    {
                    nmFree(inf,sizeof(MysqlData));
                    return NULL;
                    }
                break;

        }
        /** Build the filename. **/
        /** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
        if (strlen(new_obj_name) > 255) 
            {
            mssError(1,"MYSQL","Query result pathname exceeds internal representation");
            return NULL;
            }
        /** Set up a new element at the end of the pathname **/
        obj->Pathname->Elements[obj->Pathname->nElements] = obj->Pathname->Pathbuf + strlen(obj->Pathname->Pathbuf);
        obj->Pathname->nElements++;
        sprintf(obj->Pathname->Pathbuf,"%s\%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);

        obj_internal_CopyPath(&(inf->Pathname), obj->Pathname);
        inf->TData = qy->Data->TData;
        inf->Node = qy->Data->Node;
        inf->Node->SnNode->OpenCnt++;
        inf->Obj = obj;
        qy->ItemCnt++;

    return (void*)inf;
    }


/*** mysqlQueryClose - close the query.
 ***/
int
mysdQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

        /** Free the structure **/
        pMysqlQuery qy = (pMysqlQuery)qy_v;
        if(qy->Data->Result) mysql_free_result(qy->Data->Result);
        qy->Data->Result = NULL;
        nmFree(qy_v,sizeof(MysqlQuery));

    return 0;
    }


/*** mysqlGetAttrType - get the type (DATA_T_mysql) of an attribute by name.
 ***/
int
mysdGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
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

    return -1;
    }


/*** mysqlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
mysdGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;
        if (!strcmp(attrname,"name"))
            {
            val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
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
                case MYSD_T_DATABASE: val->String = "application/sybase"; break;
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
            val->String = "I should implement annotation";
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
            for(i=0;i<inf->TData->nCols;i++)
                {
                val->String = inf->TData->ColTypes[i];
                return 0;
                }
            }

        else if (inf->Type == MYSD_T_ROW)
            {
            for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->TData->Cols[i],attrname))
                {
                switch(inf->TData->ColCxTypes[i])
                    {
                        case DATA_T_STRING:
                            val->String=inf->Row[i];
                            break;
                        case DATA_T_INTEGER:
                            if(inf->Row[i] != NULL)
                                {
                                val->Integer=atoi(inf->Row[i]);
                                }
                            else
                                {
                                val->Integer=0; /** returns 0 if field is NULL **/
                                }
                            break;
                    }
                }
            return 0;
            }
        
        
        mssError(1,"MYSD","Could not locate requested attribute");

    return -1;
    }


/*** mysqlGetNextAttr - get the next attribute name for this object.
 ***/
char*
mysdGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);

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


/*** mysqlGetFirstAttr - get the first attribute name for this object.
 ***/
char*
mysdGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    char* ptr;
        /** Set the current attribute. **/
        inf->CurAttr = 0;

        /** Return the next one. **/
        ptr = mysdGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** mysdSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
mysdSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    pStructInf find_inf;
    int i;
    int type;
    
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
            for(i=0;i<inf->TData->nCols;i++) if (!strcmp(inf->TData->Cols[i],attrname))
                {
                if (*oxt) /** Check it this is part of a larger transaction **/
                    {
                    if (type < 0) return -1;
                    if (datatype != type)
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
                    }
                else
                    {
                    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
                        {
                        printf("Only string data can be updated at the moment...I guess I should work on that.\n");
                        }
                    if (type == DATA_T_STRING)
                        {
                        mysd_internal_UpdateRow(inf,objDataToStringTmp(type,*(void**)val,0),i);
                        /** Set dirty flag **/
                        inf->Node->SnNode->Status = SN_NS_DIRTY;
                        }
                    }
                }
            }


    return 0;
    }


/*** mysqlAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
mysdAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** mysqlOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
mysdOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** mysqlGetFirstMethod -- return name of First method available on the object.
 ***/
char*
mysdGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mysqlGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
mysdGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mysqlExecuteMethod - Execute a method, by name.
 ***/
int
mysdExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** mysqlPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
mysdPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    /** No hints yet on this **/
    return NULL;
    }


/*** mysqlInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
mysdInfo(void* inf_v, pObjectInfo info_struct)
    {
    memset(info_struct, sizeof(ObjectInfo), 0);
    return 0;
    }


/*** mysqlCommit - commit any changes made to the underlying data source.
 ***/
int
mysdCommit(void* inf_v, pObjTrxTree* oxt)
    {
    pMysqlData inf = MYSQLD(inf_v);
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
                    /** Need to get a session? **/

                    /** Perform the insert. **/
                    if (mysd_internal_InsertRow(inf, *oxt) < 0)
                        {
                        /** FAIL the oxt. **/
                        (*oxt)->Status = OXT_S_FAILED;

                        /** Release the open object data **/
                        return -1;
                        }

                    /** Complete the oxt. **/
                    (*oxt)->Status = OXT_S_COMPLETE;
                    break;

                case MYSD_T_COLUMN:
                    /** We wait until table is done for this. **/
                    break;
                }
            }

    return 0;
    }


/*** mysqlInitialize - initialize this driver, which also causes it to 
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
        memset(&MYSQLD_INF,0,sizeof(MYSQLD_INF));
        xhInit(&MYSQLD_INF.DBNodesByPath,255,0);
        xaInit(&MYSQLD_INF.DBNodeList,127);

        /** Init mysql client library **/
        if (mysql_library_init(0, NULL, NULL) != 0)
            {
            mssError(1, "MYSQL", "Could not init mysql client library");
            return -1;
            }

        /** Setup the structure **/
        strcpy(drv->Name,"MySQL ObjectSystem Driver");
        drv->Capabilities = OBJDRV_C_TRANS; //OBJDRV_C_FULLQUERY eventually
        xaInit(&(drv->RootContentTypes),16);
        xaAddItem(&(drv->RootContentTypes),"application/mysql");

        /** Setup the function references. **/
        drv->Open = mysdOpen;
        drv->Close = mysdClose;
        drv->Create = mysdCreate;
        drv->Delete = mysdDelete;
        drv->OpenQuery = mysdOpenQuery;
        drv->QueryDelete = NULL;
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

        nmRegister(sizeof(MysqlData),"MysqlData");
        nmRegister(sizeof(MysqlQuery),"MysqlQuery");

        /** Register the driver **/
        if (objRegisterDriver(drv) < 0) 
            {
            return -1;
            }
    return 0;
    }
    
MODULE_INIT(mysdInitialize);
MODULE_PREFIX("mysd");
MODULE_DESC("UNSTABLE MySQL ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);
