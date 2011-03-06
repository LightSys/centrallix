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
#include <sqlite3.h>

#define SQLT_MAX_COLS                256
#define SQLT_MAX_KEYS                8
#define SQLT_NAME_LEN                32
#define SQLT_MAX_CONN                16

#define SQLT_NODE_F_USECXAUTH	1	/* use Centrallix usernames/passwords */
#define SQLT_NODE_F_SETCXAUTH	2	/* try to change empty passwords to Centrallix login passwords */

#define SQLT_NODE_F_USECXAUTH	1	/* use Centrallix usernames/passwords */
#define SQLT_NODE_F_SETCXAUTH	2	/* try to change empty passwords to Centrallix login passwords */

/*** Node ***/
typedef struct
    {
    char        Path[OBJSYS_MAX_PATH];
    //char        Server[64];
    //char        Username[64];
    //char        Password[64];
    //char        DefaultPassword[64];
    //char        Database[MYSD_NAME_LEN];
    char        Filename[64];
    char        AnnotTable[SQLT_NAME_LEN];
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
    SqltNode, *pSqltNode;


/*** Connection data ***/
typedef struct
    {
    //MYSQL        Handle;
    sqlite3*      Handle;
    char        Username[64];
    char        Password[64];
    pSqltNode        Node;
    int                Busy;
    int                LastAccess;
    }
    SqltConn, *pSqltConn;

/*** Table data ***/
typedef struct
    {
    char        Name[SQLT_NAME_LEN];
    //unsigned char ColFlags[SQLT_MAX_COLS];
    //unsigned char ColCxTypes[SQLT_MAX_COLS];
    //unsigned char ColKeys[SQLT_MAX_COLS];
    char*        Cols[SQLT_MAX_COLS];
    //char*        ColTypes[SQLT_MAX_COLS];
    //unsigned int ColLengths[SQLT_MAX_COLS];
    pObjPresentationHints ColHints[SQLT_MAX_COLS];
    int                nCols;
    char*        Keys[SQLT_MAX_KEYS];
    int                KeyCols[SQLT_MAX_KEYS];
    int                nKeys;
    pSqltNode        Node;
    }
    SqltTable, *pSqltTable;

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    Pathname        Pathname;
    int                Type;
    //int                Flags;
    pObject        Obj;
    int                Mask;
    //int                CurAttr;
    pSqltNode        Node;
    sqlite3_stmt*     Statement;
    //char*        RowBuf;
    //MYSQL_ROW        Row;
    //MYSQL_RES *     Result;
    //char*        ColPtrs[MYSD_MAX_COLS];
    //unsigned short ColLengths[MYSD_MAX_COLS];
    pSqltTable        TData;
    /*union
        {
        DateTime	Date;
        MoneyType	Money;
        IntVec		IV;
        StringVec	SV;sqlite3_stmt* ppStmt = nmMalloc(sizeof(sqlite3_stmt));
        } Types;*/
    }
    SqltData, *pSqltData;

#define SQLT_T_DATABASE       1
#define SQLT_T_TABLE          2
#define SQLT_T_COLSOBJ        3
#define SQLT_T_ROWSOBJ        4
#define SQLT_T_COLUMN         5
#define SQLT_T_ROW            6

/*** GLOBALS ***/
struct
    {
    XHashTable              DBNodesByPath;
    XArray                      DBNodeList;
    int                             AccessCnt;
    int                             LastAccess;
    }
    SQLT_INF;

/*** sqlt_internal_statementRowCount() - get the number of rows that a prepared statement returns
 ***/
int
sqlt_internal_StatementRowCount(sqlite3_stmt* stmt)
    {
    int nRows = 0;
    int step_code;
    while((step_code = sqlite3_step(stmt)) != SQLITE_DONE)
        {
        if(step_code != SQLITE_ROW) return -1;
        nRows++;
        }
    sqlite3_reset(stmt);
    return nRows;  
    }


/*** sqlt_internal_GetConn() - given a specific database node, get a connection
 *** to the server with a given login (from mss thread structures).
 ***/
pSqltConn
sqlt_internal_GetConn(pSqltNode node)
    {
    pSqltConn conn;
    int i, conn_cnt, found;
    int min_access;
    char* username;
    char* password;

        /** Is one available from the node's connection pool? **/
        conn_cnt = xaCount(&node->Conns);
        
        /** Use system auth? **/
        if (node->Flags & SQLT_NODE_F_USECXAUTH)
            {
            /** Do we have permission to do this? **/
            if (!(CxGlobals.Flags & CX_F_ENABLEREMOTEPW))
            {
            mssError(1,"SQLT","use_system_auth requested, but Centrallix global enable_send_credentials is turned off");
            return NULL;
            }
        
            /** Get usernamename/password from session **/
            username = mssUserName();
            password = mssPassword();
        
            if(!username || !password)
            return NULL;
            }
        //else
        //    {
        //    /** Use usernamename/password from node **/
        //    username = node->Username;
        //    password = node->Password;
        //   }
        
        conn = NULL;
        for(i=0;i<conn_cnt;i++)
            {
            conn = (pSqltConn)xaGetItem(&node->Conns, i);
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
                conn = (pSqltConn)nmMalloc(sizeof(SqltConn));
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
                    conn = (pSqltConn)xaGetItem(&node->Conns, i);
                    if (!conn->Busy && conn->LastAccess < min_access)
                        {
                        min_access = conn->LastAccess;
                        found = i;
                        }
                    }
                if (found < 0)
                    {
                    mssError(1, "SQLT", "Connection limit (%d) reached", node->MaxConn);
                    return NULL;
                    }
                sqlite3_close(conn->Handle); //TODO: check return value
                memset(conn->Password, 0, sizeof(conn->Password));
                xaRemoveItem(&node->Conns, found);
                }

            /** Attempt connection **/
            //if (mysql_init(&conn->Handle) == NULL)
            //    {
            //    mssError(1, "MYSD", "Memory exhausted");
            //    nmFree(conn, sizeof(MysdConn));
            //    return NULL;
            //    }

            int result = sqlite3_open(conn->Node->Filename, &conn->Handle);

            if (conn->Handle == NULL)
                {
                mssError(1, "SQLT", "Could not allocate memory for database connection");
                nmFree(conn, sizeof(SqltConn));
                return NULL;
                }

            if (result != SQLITE_OK)
                {
                mssError(1, "SQLT", "Could not connect to SQLite database [%s]: %s", 
                        node->Filename, sqlite3_errmsg(conn->Handle));
                sqlite3_close(conn->Handle);
                nmFree(conn, sizeof(SqltConn));
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


/*** sqlt_internal_ReleaseConn() - release a connection back to the connection
 *** pool.  Also sets the pointer to NULL.
 ***/
void
sqlt_internal_ReleaseConn(pSqltConn * conn)
    {

        /** Release it **/
        assert((*conn)->Busy);
        (*conn)->Busy = 0;
        (*conn) = NULL;

    return;
    }


/*** sqlt_internal_RunQuery() - safely runs a query on the database
*** This works simarly to prepared statments using the question mark syntax.
*** Instead of calling bindParam for each argument as is done in general,
*** all of the arguments to be replaced are passed as varargs
*** There are some differences:
*** ?d => decimal. inserts using sprintf("%d",int)
*** ?q => used for query clauses.
***     same as ? except without escaping
***     only use if you know the input is clean and is already a query segment
*** ?a => array, params for this are (char** array, int length, char separator
***     it will become: item1,item2,items3 (length = 3, separator = ',')
*** '?a' => same as array except with individual quoting specified
***     it will become: 'item1','item2','items3' (length = 3, separator = ',')
*** All parameters are sanatized with mysql_real_escape_string before
*** the query is built
*** This WILL NOT WORK WITH BINARY DATA
***/
sqlite3_stmt*
sqlt_internal_RunQuery(pSqltNode node, char* stmt, ...)
    {
    sqlite3_stmt* ppStmt = NULL;
    const char tail;
    const char* ptail = &tail;
    pSqltConn conn = NULL;
    XString query;
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
        xsInit(&query);

        if(!(conn = sqlt_internal_GetConn(node))) goto error;
        
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
                        if(sqlt_internal_SafeAppend(conn->Handle,&query,array[j])) goto error;
                        }
                    start = &stmt[i+2];
                    i++;
                    }
                else if(stmt[i+1]=='d') /** handle integers **/
                    {
                        sprintf(tmp,"%d",va_arg(ap,int));
                        if(sqlt_internal_SafeAppend(conn->Handle,&query,tmp)) goto error;
                        start = &stmt[i+2];
                        i++;
                    }                
                else if(stmt[i+1]=='q') /** handle pre-sanitized query sections **/
                    {
                        xsConcatenate(&query,va_arg(ap,char*),-1);
                        start = &stmt[i+2];
                        i++;
                    }
                else /** handle plain sanatize+insert **/
                    {
                    if(sqlt_internal_SafeAppend(&conn->Handle,&query,va_arg(ap,char*))) goto error;
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

        va_end(ap);

        if(sqlite3_prepare_v2(conn->Handle, query.String, query.Length, &ppStmt, &ptail) != SQLITE_OK) goto error;

        //if(mysql_query(&conn->Handle,query.String)) goto error;
        //if(!(result = mysql_store_result(&conn->Handle))) goto error;

        error:
            if(conn) sqlt_internal_ReleaseConn(&conn);
            xsDeInit(&query);
            return ppStmt;
    }



/*** sqlt_internal_getRowByKey()
 ***/
int
sqlt_internal_GetRowByKey(char* key, pSqltData data, char* tablename)
    {
        if(!(data->TData)) return -1;

        if(!(data->Statement = sqlt_internal_RunQuery(data->Node,"SELECT * FROM `?` WHERE CONCAT_WS('|',`?a`)='?' LIMIT 0,1",data->TData->Name,data->TData->Keys,data->TData->nKeys,',',key))) return -1;
        else 
            {
            int ret = sqlt_internal_statementRowCount(data->Statement);
            sqlite3_step(data->Statement);
            return ret;
            }
    }


/*** sqlt_internal_ParseTData() - given a sqlt prepared statement, parse the table
 *** data into a SqltTable structure.  Returns < 0 on failure.
 ***/
int
sqlt_internal_ParseTData(sqlite3_stmt* stmt, pSqltTable tdata)
    {
    int step_code;
    char data_desc[32];
    //char* pptr;
    //char* cptr;
    //int len;
    //char* pos;
    int rowcnt = sqlt_internal_StatementRowCount(stmt);

        if (rowcnt > SQLT_MAX_COLS)
            {
            mssError(1,"SQLT","Too many columns (%d) in table [%s]", rowcnt, tdata->Name);
            return -1;
            }

        /** Loop through the result set with the column descriptions **/
        tdata->nCols = tdata->nKeys = 0;

        while((step_code = sqlite3_step(stmt)) != SQLITE_DONE)
            {
            if(step_code != SQLITE_ROW) return -1;
            
            //row = mysql_fetch_row(resultset);
            //if (!row) break;

            /** Name **/
            tdata->Cols[tdata->nCols] = nmSysStrdup((const char*) sqlite3_column_text(stmt, 0));
            strtcpy(data_desc, (const char*) sqlite3_column_text(stmt, 1), sizeof(data_desc));

            /** Length, if char/varchar **/
            //tdata->ColLengths[tdata->nCols] = 0;
            //if ((pptr = strchr(data_desc, '(')) != NULL)
            //    {
            //    *pptr = '\0';
            //    tdata->ColLengths[tdata->nCols] = strtol(pptr+1, NULL, 10);
            //    if ((cptr = strchr(pptr+1, ',')) != NULL)
            //        len = strtol(cptr+1, NULL, 10);
            //    }
                
            /** set lengths for various text types **/
            //if(!strcmp(data_desc, "tinytext") || !strcmp(data_desc, "tinyblob")) tdata->ColLengths[tdata->nCols] = 0xFF;
            //if(!strcmp(data_desc, "text") || !strcmp(data_desc, "blob")) tdata->ColLengths[tdata->nCols] = 0xFFFF;
            //if(!strcmp(data_desc, "mediumtext") || !strcmp(data_desc, "mediumblob")) tdata->ColLengths[tdata->nCols] = 0xFFFFFF;
            //if(!strcmp(data_desc, "longtext") || !strcmp(data_desc, "longblob")) tdata->ColLengths[tdata->nCols] = 0xFFFFFFFF;

            /** Type **/
            //tdata->ColTypes[tdata->nCols] = nmSysStrdup(data_desc);
            //if (!strcmp(data_desc, "integer"))
            //    tdata->ColCxTypes[tdata->nCols] = DATA_T_INTEGER;
            //else if (!strcmp(data_desc, "text") || !strcmp(data_desc, "blob"))
            //    tdata->ColCxTypes[tdata->nCols] = DATA_T_STRING;
            //else if (!strcmp(data_desc, "real"))
            //    tdata->ColCxTypes[tdata->nCols] = DATA_T_DOUBLE;
            //else
            //    tdata->ColCxTypes[tdata->nCols] = DATA_T_UNAVAILABLE;

            //tdata->ColFlags[tdata->ncols] = 0;
            /** unsigned? **/
            //if(row[1] && (pos = strchr(row[1],'u')))
            //    {
            //    if(!strcmp(pos,"unsigned")) 
            //        tdata->ColFlags[i] |= MYSD_COL_F_UNSIGNED;
            //    }
            
            /** Allow nulls **/
            //if (row[2] && !strcmp(row[2], "YES"))
            //    tdata->ColFlags[i] |= MYSD_COL_F_NULL;

            /** Primary Key **/
            //if (row[3] && !strcmp(row[3], "PRI"))
            //    {
            //    if (tdata->nKeys >= MYSD_MAX_KEYS)
            //        {
            //        mssError(1,"MYSD","Too many key fields in table [%s]",tdata->Name);
            //        return -1;
            //        }
            //    tdata->Keys[tdata->nKeys] = tdata->Cols[tdata->nCols];
            //    tdata->KeyCols[tdata->nKeys] = tdata->nCols;
            //    tdata->ColKeys[tdata->nCols] = tdata->nKeys;
            //    tdata->nKeys++;
            //    tdata->ColFlags[i] |= MYSD_COL_F_PRIKEY;
            //    }
            tdata->nCols++;
            }

    return 0;
    }


/*** sqlt_internal_GetTData() - get a table information structure
 ***/
pSqltTable
sqlt_internal_GetTData(pSqltNode node, char* tablename)
    {
    //MYSQL_RES * result = NULL;
    sqlite3_stmt * stmt = NULL; 
    pSqltTable tdata = NULL;
    int length;
    int rowcnt;

        /** Value cached already? **/
        tdata = (pSqltTable)xhLookup(&node->Tables, tablename);
        if (tdata)
            return tdata;

        /** sanatize the table name and build the query**/
        length = strlen(tablename);
        /** this next bit will break any charset not ASCII **/ 
        /** throw an error if some joker tries to throw in a backtick **/
        if(strchr(tablename,'`')) goto error; 
        stmt = sqlt_internal_RunQuery(node, "SHOW COLUMNS FROM `?`",tablename);
        if (!stmt)
            goto error;

        rowcnt = sqlt_internal_statementRowCount(stmt);

        if (rowcnt <= 0)
            goto error;

        /** Build the TData **/
        tdata = (pSqltTable)nmMalloc(sizeof(SqltTable));
        if (!tdata)
            goto error;
        memset(tdata, 0, sizeof(SqltTable));
        strtcpy(tdata->Name, tablename, sizeof(tdata->Name));
        tdata->Node = node;
        if (sqlt_internal_ParseTData(stmt, tdata) < 0)
            {
            nmFree(tdata, sizeof(SqltTable));
            tdata = NULL;
            }

    error:
        if (stmt)
            sqlite3_finalize(stmt);
        if(tdata) 
            {
            xhAdd(&(node->Tables),tdata->Name,(char*)tdata);
            }
    return tdata;
    }


/*** sqlt_internal_DetermineType() - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
sqlt_internal_DetermineType(pObject obj, pSqltData inf)
    {
    int i;

        /** Determine object type (depth) and get pointers set up **/
        obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
        for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;

        /** Set up pointers based on number of elements past the node object **/
        if (inf->Pathname.nElements == obj->SubPtr)
            {
            inf->Type = SQLT_T_DATABASE;
            obj->SubCnt = 1;
            }
        if (inf->Pathname.nElements - 1 >= obj->SubPtr)
            {
            inf->Type = SQLT_T_TABLE;
            obj->SubCnt = 2;
            }
        if (inf->Pathname.nElements - 2 >= obj->SubPtr)
            {
            if (!strncmp(inf->Pathname.Elements[obj->SubPtr+1],"rows",4)) inf->Type = SQLT_T_ROWSOBJ;
            else if (!strncmp(inf->Pathname.Elements[obj->SubPtr+1],"columns",7)) inf->Type = SQLT_T_COLSOBJ;
            else return -1;
            obj->SubCnt = 3;
            }
        if (inf->Pathname.nElements - 3 >= obj->SubPtr)
            {
            if (inf->Type == SQLT_T_ROWSOBJ) inf->Type = SQLT_T_ROW;
            else if (inf->Type == SQLT_T_COLSOBJ) inf->Type = SQLT_T_COLUMN;
            obj->SubCnt = 4;
            }

    return 0;
    }


/*** sqlt_internal_OpenNode() - access the node object and get the sqlite
 *** server connection parameters.
 ***/
pSqltNode
sqlt_internal_OpenNode(char* path, int mode, pObject obj, int node_only, int mask)
    {
    pSqltNode db_node;
    pSnNode sn_node;
    char* ptr = NULL;
    int i;
    
        /** First, do a lookup in the db node cache. **/
        db_node = (pSqltNode)xhLookup(&(SQLT_INF.DBNodesByPath), path);
        if (db_node) 
            {
            db_node->LastAccess = SQLT_INF.AccessCnt;
            SQLT_INF.AccessCnt++;
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
            sn_node = snNewNode(obj->Prev, "application/sqlite");
            if (!sn_node)
                {
                mssError(0,"SQLT","Database node create failed");
                return NULL;
                }
 
            //snSetParamString(sn_node,obj->Prev,"server","localhost");
            //snSetParamString(sn_node,obj->Prev,"database","");
            snSetParamString(sn_node, obj->Prev,"filename","");
            snSetParamString(sn_node,obj->Prev,"annot_table","CXTableAnnotations");
            snSetParamString(sn_node,obj->Prev,"description","SQLite Database");
            snSetParamInteger(sn_node,obj->Prev,"max_connections",16);
            snWriteNode(obj->Prev,sn_node);
            }
        else
            {
            sn_node = snReadNode(obj->Prev);
            if (!sn_node)
                {
                mssError(0,"SQLT","Database node open failed");
                return NULL;
                }
            }

        /** Create the DB node and fill it in. **/
        db_node = (pSqltNode)nmMalloc(sizeof(SqltNode));
        if (!db_node)
            {
            mssError(0,"SQLT","Could not allocate DB node structure");
            return NULL;
            }
        memset(db_node,0,sizeof(SqltNode));
        db_node->SnNode = sn_node;
        strtcpy(db_node->Path,path,sizeof(db_node->Path));
        //if (stAttrValue(stLookup(sn_node->Data,"server"),NULL,&ptr,0) < 0) ptr = NULL;
        //strtcpy(db_node->Server,ptr?ptr:"localhost",sizeof(db_node->Server));
        //if (stAttrValue(stLookup(sn_node->Data,"database"),NULL,&ptr,0) < 0) ptr = NULL;
        //strtcpy(db_node->Database,ptr?ptr:"",sizeof(db_node->Database));
        if (stAttrValue(stLookup(sn_node->Data,"filename"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->Filename,ptr?ptr:"",sizeof(db_node->Filename));
        if (stAttrValue(stLookup(sn_node->Data,"annot_table"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->AnnotTable,ptr?ptr:"CXTableAnnotations",sizeof(db_node->AnnotTable));
        if (stAttrValue(stLookup(sn_node->Data,"description"),NULL,&ptr,0) < 0) ptr = NULL;
        strtcpy(db_node->Description,ptr?ptr:"",sizeof(db_node->Description));
        if (stAttrValue(stLookup(sn_node->Data,"max_connections"),&i,NULL,0) < 0) i=16;
        if (stAttrValue(stLookup(sn_node->Data,"use_system_auth"),NULL,&ptr,0) == 0)
            {
            if (!strcasecmp(ptr,"yes"))
            db_node->Flags |= SQLT_NODE_F_USECXAUTH;
            }
        if (stAttrValue(stLookup(sn_node->Data,"set_passwords"),NULL,&ptr,0) == 0)
            {
            if (!strcasecmp(ptr,"yes"))
            db_node->Flags |= SQLT_NODE_F_SETCXAUTH;
            }
        //if (stAttrValue(stLookup(sn_node->Data,"username"),NULL,&ptr,0) < 0) ptr = "cxguest";
        //strtcpy(db_node->Username,ptr,sizeof(db_node->Username));
        //if (stAttrValue(stLookup(sn_node->Data,"password"),NULL,&ptr,0) < 0) ptr = "";
        //strtcpy(db_node->Password,ptr,sizeof(db_node->Password));
        //if (stAttrValue(stLookup(sn_node->Data,"default_password"),NULL,&ptr,0) < 0) ptr = "";
        //strtcpy(db_node->DefaultPassword,ptr,sizeof(db_node->DefaultPassword));
        db_node->MaxConn = i;
        xaInit(&(db_node->Conns),16);
        xhInit(&(db_node->Tables),255,0);
        db_node->ConnAccessCnt = 0;
        
        /** Get table names **/
        //
        if((i=sqlt_internal_GetTablenames(db_node)) < 0)
            {
            mssError(0,"SQLT","Unable to query for table names");
            nmFree(db_node,sizeof(SqltNode));
            return NULL;
            }

        /** Add node to the db node cache **/
        xhAdd(&(SQLT_INF.DBNodesByPath), db_node->Path, (void*)db_node);
        xaAddItem(&SQLT_INF.DBNodeList, (void*)db_node);

    return db_node;
    }


/*** sqlt_internal_GetTablenames() - throw the table names in an Xarray
 ***/
int
sqlt_internal_GetTablenames(pSqltNode node)
    {
    //MYSQL_RES * result = NULL;
    //MYSQL_ROW row;
    //pMysdConn conn;

    sqlite3_stmt* stmt = NULL;

    if(!(stmt = sqlt_internal_RunQuery(node,"SHOW TABLES"))) return -1;

    xaInit(&node->Tablenames, sqlt_internal_statementRowCount(stmt));
    
    int step_code;    
    while((step_code = sqlite3_step(stmt)) != SQLITE_DONE)
        {
        if(step_code != SQLITE_ROW) return -1;
        if(xaAddItem(&node->Tablenames, (void*) sqlite3_column_text(stmt, 0)) < 0) {sqlite3_finalize(stmt); return -1;}
        }
    
    sqlite3_finalize(stmt);
    return 0;
    }


/*** sqlt_internal_SafeAppend - appends a string on a query
 ***/
int
sqlt_internal_SafeAppend(sqlite3* conn, pXString dst, char* src)
    {
        char* escaped_src;
        int length;
        int rval = 0;
        length = strlen(src);
        
        escaped_src = nmMalloc(length*2+1);
        //mysql_real_escape_string(conn,escaped_src,src,length);
        if(xsConcatenate(dst,escaped_src,-1)) rval = -1;
        
        //nmFree(escaped_src,length*2+1);

    //TODO: escape

    return rval;
    }


/*** sqltOpen() - open an object.
 ***/
void*
sqltOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pSqltData inf;
    int rval;
    int length;
    char* tablename;
    char* node_path;
    char* ptr;

        /** Allocate the structure **/
        inf = (pSqltData)nmMalloc(sizeof(SqltData));
        if (!inf) return NULL;
        memset(inf,0,sizeof(SqltData));
        inf->Obj = obj;
        inf->Mask = mask;
        //inf->Result = NULL;

        /** Determine the type **/
        if(sqlt_internal_DetermineType(obj,inf))
            {
            mssError(1,"SQLT","Unable to determine type.");
            nmFree(inf,sizeof(SqltData));
            return NULL;
            }

        /** Determine the node path **/
        node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

        /** this leaks memory MUST FIX **/
        if(!(inf->Node = sqlt_internal_OpenNode(node_path, obj->Mode, obj, inf->Type == SQLT_T_DATABASE, inf->Mask)))
            {
            mssError(1,"SQLT","Couldn't open node.");
            nmFree(inf,sizeof(SqltData));
            return NULL;
            }
        
        /** get table name from path **/
        if(obj->SubCnt > 1)
            {
            tablename = obj_internal_PathPart(obj->Pathname, 2, obj->SubPtr-1);
            length = obj->Pathname->Elements[obj->Pathname->nElements - 1] - obj->Pathname->Elements[obj->Pathname->nElements - 2];
            if(!(inf->TData = sqlt_internal_GetTData(inf->Node,tablename)))
                {
                if(obj->Mode & O_CREAT)
                    /** Table creation code could some day go here,
                     ** but it is not supported right now
                     **/
                    mssError(1,"SQLT","Table creation is not supported currently");
                else
                    mssError(1,"SQLT","Table object does not exist.");
                nmFree(inf,sizeof(SqltData));
                return NULL;
                }
            }
        
        /** Set object params. **/
        obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
        inf->Node->SnNode->OpenCnt++;

        if(inf->Type == SQLT_T_ROW)
            {
            rval = 0;
            /** Autonaming a new object?  Won't be able to look it up if so. **/
            if (!(inf->Obj->Mode & OBJ_O_AUTONAME))
                {
                ptr = obj_internal_PathPart(obj->Pathname, 4, obj->SubPtr-3);
                if ((rval = sqlt_internal_GetRowByKey(ptr,inf,tablename)) < 0)
                    {
                    mssError(1,"SQLT","Unable to fetch row by key.");
                    if(inf->Statement) sqlite3_finalize(inf->Statement);
                    inf->Statement = NULL;
                    nmFree(inf,sizeof(SqltData));
                    return NULL;
                    }
                else if(rval == 0)
                    {
                    if(inf->Statement) sqlite3_finalize(inf->Statement);
                    inf->Statement = NULL;                   
                    }
                }
            if (rval == 0)
                {
                /** User specified a row that doesn't exist. **/
                if (!(obj->Mode & O_CREAT))
                    {
                    mssError(1,"SQLT","Row object does not exist.");
                    nmFree(inf,sizeof(SqltData));
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


/*** sqltInitialize() - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
sqltInitialize()
    {
    pObjDriver drv;

        /** Allocate the driver **/
        drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
        if (!drv) return -1;
        memset(drv, 0, sizeof(ObjDriver));

        /** Initialize globals **/
        memset(&SQLT_INF,0,sizeof(SQLT_INF));
        xhInit(&SQLT_INF.DBNodesByPath,255,0);
        xaInit(&SQLT_INF.DBNodeList,127);

        /** Init mysql client library **/
        //if (mysql_library_init(0, NULL, NULL) != 0)
        //    {
        //    mssError(1, "MYSQL", "Could not init mysql client library");
        //    return -1;
        //    }

        /** Setup the structure **/
        strcpy(drv->Name,"SQLite ObjectSystem Driver");
        drv->Capabilities = OBJDRV_C_TRANS | OBJDRV_C_FULLQUERY;
        xaInit(&(drv->RootContentTypes),16);
        xaAddItem(&(drv->RootContentTypes),"application/sqlite");

        /** Setup the function references. **/
        drv->Open = sqltOpen;
        drv->Close = /*mysdClose*/NULL;
        drv->Create = /*mysdCreate*/NULL;
        drv->Delete = /*mysdDelete*/NULL;
        drv->OpenQuery = /*mysdOpenQuery*/NULL;
        drv->QueryDelete = NULL;
        drv->QueryFetch = /*mysdQueryFetch*/NULL;
        drv->QueryClose = /*mysdQueryClose*/NULL;
        drv->Read = /*mysdRead*/NULL;
        drv->Write = /*mysdWrite*/NULL;
        drv->GetAttrType = /*mysdGetAttrType*/NULL;
        drv->GetAttrValue = /*mysdGetAttrValue*/NULL;
        drv->GetFirstAttr = /*mysdGetFirstAttr*/NULL;
        drv->GetNextAttr = /*mysdGetNextAttr*/NULL;
        drv->SetAttrValue = /*mysdSetAttrValue*/NULL;
        drv->AddAttr = /*mysdAddAttr*/NULL;
        drv->OpenAttr = /*mysdOpenAttr*/NULL;
        drv->GetFirstMethod = /*mysdGetFirstMethod*/NULL;
        drv->GetNextMethod = /*mysdGetNextMethod*/NULL;
        drv->ExecuteMethod = /*mysdExecuteMethod*/NULL;
        drv->PresentationHints = /*mysdPresentationHints*/NULL;
        drv->Info = /*mysdInfo*/NULL;
        drv->Commit = /*mysdCommit*/NULL;

        nmRegister(sizeof(SqltData),"SqltData");
        //nmRegister(sizeof(MysdQuery),"MysdQuery");

        /** Register the driver **/
        if (objRegisterDriver(drv) < 0) 
            {
            return -1;
            }
    return 0;
    }


MODULE_INIT(sqltInitialize);
MODULE_PREFIX("sqlt");
MODULE_DESC("SQLite ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
