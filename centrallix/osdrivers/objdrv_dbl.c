#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <regex.h>
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
#include "magic.h"
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2002 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_dbl.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 11th, 2002 					*/
/* Description:	Objectsystem driver for DBL ISAM files.  Requires there	*/
/*		to be DEF data associated with the IS1 file - see the	*/
/*		def_search_path setting in centrallix.conf for further	*/
/*		details.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_dbl.c,v 1.1 2002/07/29 17:47:36 kai5263499 Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_dbl.c,v $

    $Log: objdrv_dbl.c,v $
    Revision 1.1  2002/07/29 17:47:36  kai5263499
    Initial import of Greg's DBL object system driver...


 **END-CVSDATA***********************************************************/


/*** Module controls ***/
#define	DBL_DEFAULT_DEF_SEARCH_PATH "%.IS1 = %.DEF(^COMMON %)"
#define	DBL_MAX_PATH_ITEMS	16
#define DBL_FORCE_READONLY	1	/* not update safe yet!!! */


/*** Structure for storing table metadata information ***/
typedef struct
    {
    char	Table[32];
    char*	TablePtr;
    char*	RowColPtr;
    char*	TableSubPtr;
    }
    DblTableInf, *pDblTableInf;


/*** Structure for directory entry nodes ***/
typedef struct
    {
    char	Path[256];
    int		Type;
    }
    DblNode, *pDblNode;

#define DBL_NODE_T_DATA		1
#define DBL_NODE_T_INDEX	2	/* passthrough */
#define DBL_NODE_T_DEFINITION	3	/* passthrough */


/*** Structure used by this driver internally for open objects ***/
typedef struct 
    {
    pDblNode	Node;
    pDblTableInf TData;
    int		Type;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    }
    DblData, *pDblData;

#define DBL_T_DATABASE		1	/* not used */
#define DBL_T_TABLE		2
#define DBL_T_ROWSOBJ		3
#define DBL_T_COLSOBJ		4
#define DBL_T_ROW		5
#define DBL_T_COLUMN		6

#define DBL(x) ((pDblData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pDblData	ObjInf;
    pDblTableInf TableInf;
    }
    DblQuery, *pDblQuery;


/*** structure for search path item ***/
typedef struct
    {
    char*		SrcPattern;
    char*		DefPattern;
    char*		SearchRegex;
    }
    DblSearchPathItem, *pDblSearchPathItem;



/*** GLOBALS ***/
struct
    {
    XHashTable		DBNodes;
    XArray		DBNodeList;
    pStructInf		DblConfig;
    char*		DefSearchPath;
    pDblSearchPathItem	SearchItems[DBL_MAX_PATH_ITEMS];
    int			nSearchItems;
    }
    DBL_INF;



/*** dbl_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
dbl_internal_DetermineType(pObject obj, pDblData inf)
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
	    inf->Type = DBL_T_DATABASE;
	    obj->SubCnt = 1;
	    }
	if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	    {
	    inf->Type = DBL_T_TABLE;
	    inf->TablePtr = inf->Pathname.Elements[obj->SubPtr];
	    obj->SubCnt = 2;
	    }
	if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	    {
	    inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = DBL_T_ROWSOBJ;
	    else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = DBL_T_COLSOBJ;
	    obj->SubCnt = 3;
	    }
	if (inf->Pathname.nElements - 3 >= obj->SubPtr)
	    {
	    inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+2];
	    if (inf->Type == DBL_T_ROWSOBJ) inf->Type = DBL_T_ROW;
	    else if (inf->Type == DBL_T_COLSOBJ) inf->Type = DBL_T_COLUMN;
	    obj->SubCnt = 4;
	    }

    return 0;
    }


/*** dbl_internal_OpenNode() - locate and load the values from the DEF file
 *** containing the definitions for this table.
 ***/
pDblNode
dbl_internal_OpenNode(pDblData inf, char* tablename)
    {
    pDblNode node;
    int i;
    pDblSearchPathItem search_path;
    pObject def_file = NULL;
    pLxSession lxs;
    char dummybuf[1];
    char* linebuf = NULL;
    int t;

	/** First, see if it is already loaded. **/
	node = (pDblNode)xhLookup(&DBL_INF.DBNodes, inf->Obj->Pathname);
	if (node) return node;

	/** If not, we need to track it down.  Try each of the paths in the
	 ** search listing.
	 **/
	for(i=0;i<DBL_INF.nSearchItems;i++)
	    {
	    search_path = DBL_INF.SearchItems[i];
	    def_file = dbl_internal_FindDefFile(inf->Obj->Pathname, search_path);
	    if (def_file) break;
	    }
	if (!def_file)
	    {
	    mssError(1,"DBL","Could not locate definition file for object '%s'", tablename);
	    goto error;
	    }

	/** Ok, found a DEF file containing the given table definition.
	 ** Load in the definition.  Use a dummy objRead() to seek to the
	 ** beginning of the object content.
	 **/
	linebuf = nmMalloc(512);
	if (!linebuf) goto error;
	objRead(def_file, dummybuf, 0, 0, FD_U_SEEK);
	lxs = mlxGenericSession(def_file, objRead, MLX_F_LINEONLY | MLX_F_EOF);
	if (!lxs) goto error;
	while ((t = mlxNextToken(lxs)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
	    {
	    mlxCopyToken(lxs, linebuf, 512);
	    }

    error:
	if (linebuf) nmFree(linebuf, 512);
	if (lxs) mlxCloseSession(lxs);
	if (def_file) objClose(def_file);
	return NULL;
    }


/*** dbl_internal_DetermineNodeType() - determine the node type from the 
 *** provided systype.
 ***/
int
dbl_internal_DetermineNodeType(pDblData inf, pContentType systype)
    {
    if (!strcmp(systype->Name, "application/dbl")) return DBL_NODE_T_DATA;
    else if (!strcmp(systype->Name, "application/dbl-index")) return DBL_NODE_T_INDEX;
    else if (!strcmp(systype->Name, "application/dbl-definition")) return DBL_NODE_T_DEFINITION;
    return -1;
    }


/*** dbl_internal_ParseOneDefItem() - breaks down one DEF search path item
 *** and returns a structure describing it.  Modifies the item string in place
 *** to allow the building of the structure.  Structure becomes invalid if the
 *** item string is released or otherwise modified somehow.
 ***/
pDblSearchPathItem
dbl_internal_ParseOneDefItem(char* itemstring)
    {
    pDblSearchPathItem item = NULL;
    char* sepptr;
    char* whitespaceptr;

	/** Alloc the item **/
	item = (pDblSearchPathItem)nmMalloc(sizeof(DblSearchPathItem));
	if (!item) return NULL;
	item->SrcPattern = NULL;
	item->DstPattern = NULL;
	item->SearchRegex = NULL;

	/** Parse source (.IS1) pattern **/
	item->SrcPattern = itemstring;
	sepptr = strchr(item->SrcPattern,'=');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';
	whitespaceptr = sepptr+1;
	while (whitespaceptr == ' ') *(whitespaceptr++) = '\0';

	/** Parse destination (.DEF) pattrn **/
	item->DstPattern = whitespaceptr;
	sepptr = strchr(item->DstPattern,'(');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';
	whitespaceptr = sepptr+1;
	while (whitespaceptr == ' ') *(whitespaceptr++) = '\0';

	/** Parse search regex in parentheses **/
	item->SearchRegex = whitespaceptr;
	sepptr = strchr(item->SearchRegex,')');
	if (!sepptr) goto error;
	*sepptr = '\0';
	whitespaceptr = sepptr-1;
	while (whitespaceptr > itemstring && *whitespaceptr == ' ') *(whitespaceptr--) = '\0';

    return item;

    /** Error exit handler **/
    error:
	if (item) nmFree(item,sizeof(DblSearchPathItem));
	return NULL;
    }


/*** dbl_internal_ParseDefPathItems() - parses a definition search path into
 *** multiple definition search items.  Loads a copy of the defpath into the
 *** globals, and modifies that copy to break it up into components which can
 *** be linked to from search path item structures, loaded also into the
 *** globals for this module.
 ***/
int
dbl_internal_ParseDefPathItems(char* defpath)
    {
    char* itemptr;
    char* defcopy;
    char* semiptr;
    pDblSearchPathItem item;

	/** Make the copy of the string so we can work on it. **/
	defcopy = nmSysStrdup(defpath);
	if (!defcopy) return -1;
	DBL_INF.DefSearchPath = defcopy;

	/** Break it up into semicolon-separated items **/
	itemptr = defcopy;
	while(itemptr && *itemptr && DBL_INF.nSearchItems < DBL_MAX_PATH_ITEMS)
	    {
	    semiptr = strchr(itemptr,';');
	    if (semiptr) *(semiptr++) = '\0';
	    item = dbl_internal_ParseOneDefItem(itemptr);
	    if (item)
	        DBL_INF.SearchItems[DBL_INF.nSearchItems++] = item;
	    itemptr = semiptr;
	    }

    return 0;
    }


/*** dblOpen - open a table, row, or column.
 ***/
void*
dblOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pDblData inf;
    char sbuf[256];
    int i,cnt,restype,n,ncols;
    pObjTrxTree new_oxt;
    pDblTableInf tdata;
    CS_COMMAND* cmd;
    char* ptr;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DblData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine node type **/
	dbl_internal_DetermineNodeType(inf,systype);

	/** Determine type and set pointers. **/
	dbl_internal_DetermineType(obj,inf);

	/** Access the DB node. **/

	/** Verify the table, if a table mentioned **/
	if (inf->TablePtr)
	    {
	    if (strpbrk(inf->TablePtr," \t\r\n"))
		{
		mssError(1,"DBL","Requested table %s is invalid", inf->TablePtr);
		nmFree(inf,sizeof(DblData));
		return NULL;
		}
	    }
	
    return (void*)inf;
    }


/*** dbl_internal_InsertRow - inserts a new row into the database, looking
 *** throught the OXT structures for the column values.  For text/image columns,
 *** it automatically inserts "" for an unspecified column value when the column
 *** does not allow nulls.
 ***/
int
dbl_internal_InsertRow(pDblData inf, CS_CONNECTION* session, pObjTrxTree oxt)
    {
    char* kptr;
    char* kendptr;
    int i,j,len,ctype,restype;
    pObjTrxTree attr_oxt, find_oxt;
    CS_COMMAND* cmd;
    pXString insbuf;
    char* tmpptr;
    char tmpch;

        /** Ok, look for the attribute sub-OXT's **/
        for(j=0;j<inf->TData->nCols;j++)
            {
	    /** If primary key, we have values in inf->RowColPtr. **/
	    if (inf->TData->ColFlags[j] & DBL_CF_PRIKEY)
	        {
		/** Determine position,length within prikey-coded name **/
		kptr = inf->RowColPtr;
		for(i=0;i<inf->TData->ColKeys[j] && kptr != (char*)1;i++) kptr = strchr(kptr,'|')+1;
		if (kptr == (char*)1)
		    {
		    mssError(1,"DBL","Not enough components in concat primary key (name)");
		    xsDeInit(insbuf);
		    nmFree(insbuf,sizeof(XString));
		    return -1;
		    }
		kendptr = strchr(kptr,'|');
		if (!kendptr) len = strlen(kptr); else len = kendptr-kptr;
		}
	    else
	        {
		/** Otherwise, we scan through the OXT's **/
                find_oxt=NULL;
                for(i=0;i<oxt->Children.nItems;i++)
                    {
                    attr_oxt = ((pObjTrxTree)(oxt->Children.Items[i]));
                    /*if (((pDblData)(attr_oxt->LLParam))->Type == DBL_T_ATTR)*/
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
                }
	    }

    return 0;
    }


/*** dblClose - close an open file or directory.
 ***/
int
dblClose(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int i;
    struct stat fileinfo;
    char sbuf[160];

    	/** Was this a create? **/
	if ((*oxt) && (*oxt)->OpType == OXT_OP_CREATE && (*oxt)->Status != OXT_S_COMPLETE)
	    {
	    switch (inf->Type)
	        {
		case DBL_T_TABLE:
		    /** We'll get to this a little later **/
		    break;

		case DBL_T_ROW:
		    /** Complete the oxt. **/
		    (*oxt)->Status = OXT_S_COMPLETE;

		    break;

		case DBL_T_COLUMN:
		    /** We wait until table is done for this. **/
		    break;
		}
	    }

	/** Free the info structure **/
	nmFree(inf,sizeof(DblData));

    return 0;
    }


/*** dblCreate - create a new object without actually opening that 
 *** object.
 ***/
int
dblCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* objd;

    	/** Open and close the object **/
	obj->Mode |= O_CREAT;
	objd = dblOpen(obj,mask,systype,usrtype,oxt);
	if (!objd) return -1;

    return dblClose(objd,oxt);
    }


/*** dblDelete - delete an existing object.
 ***/
int
dblDelete(pObject obj, pObjTrxTree* oxt)
    {
    char sbuf[256];
    pDblData inf;
    CS_COMMAND* cmd;
    char* ptr;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return -1;
	memset(inf,0,sizeof(DblData));
	inf->Obj = obj;

	/** Determine type and set pointers. **/
	dbl_internal_DetermineType(obj,inf);

	/** If a row, proceed else fail the delete. **/
	if (inf->Type != DBL_T_ROW)
	    {
	    nmFree(inf,sizeof(DblData));
	    puts("Unimplemented delete operation in DBL.");
	    mssError(1,"DBL","Unimplemented delete operation in DBL");
	    return -1;
	    }

	/** Access the DB node. **/

	/** Free the structure **/
	nmFree(inf,sizeof(DblData));

    return 0;
    }


/*** dblRead - read from the object's content.  Unsupported on these files.
 ***/
int
dblRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pDblData inf = DBL(inf_v);*/

    return -1;
    }


/*** dblWrite - write to an object's content.  Unsupported for these types of things.
 ***/
int
dblWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pDblData inf = DBL(inf_v);*/

    return -1;
    }


/*** dblOpenQuery - open a directory query.  We basically reformat the where clause
 *** and issue a query to the DB.
 ***/
void*
dblOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    pDblQuery qy;
    int i,restype;
    XString sql;

	/** Allocate the query structure **/
	qy = (pDblQuery)nmMalloc(sizeof(DblQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(DblQuery));
	qy->ObjInf = inf;
	qy->RowCnt = -1;
	qy->ObjSession = NULL;

	/** State that we won't do full query unless we get to DBL_T_ROWSOBJ below **/
	query->Flags &= ~OBJ_QY_F_FULLQUERY;
	query->Flags &= ~OBJ_QY_F_FULLSORT;

	/** Build the query SQL based on object type. **/
	qy->Cmd = NULL;
	switch(inf->Type)
	    {
	    case DBL_T_DATABASE:
	        /** Select the list of tables from the DB. **/
		qy->RowCnt = 0;
		break;

	    case DBL_T_TABLE:
	        /** No SQL needed -- always returns just 'columns' and 'rows' **/
		qy->RowCnt = 0;
	        break;

	    case DBL_T_COLSOBJ:
	        /** Get a columns list. **/
		qy->TableInf = qy->ObjInf->TData;
		qy->RowCnt = 0;
		break;

	    case DBL_T_ROWSOBJ:
	        /** Query the rows within a table -- iteration here. **/
		break;

	    case DBL_T_COLUMN:
	    case DBL_T_ROW:
	        /** These don't support queries for sub-objects. **/
	        nmFree(qy,sizeof(DblQuery));
		qy = NULL;
		break;
	    }

    return (void*)qy;
    }


/*** dblQueryFetch - get the next directory entry as an open object.
 ***/
void*
dblQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pDblQuery qy = ((pDblQuery)(qy_v));
    pDblData inf;
    char filename[120];
    char* ptr;
    int new_type;
    int i,cnt;
    pDblTableInf tdata = qy->ObjInf->TData;
    CS_CONNECTION* s2;
    int restype;

	/** Allocate the structure **/
	inf = (pDblData)nmMalloc(sizeof(DblData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(DblData));
	inf->TData = tdata;

    	/** Get the next name based on the query type. **/
	switch(qy->ObjInf->Type)
	    {
	    case DBL_T_DATABASE:
	        break;

	    case DBL_T_TABLE:
	        /** Filename is either "rows" or "columns" **/
		if (qy->RowCnt == 1) 
		    {
		    strcpy(filename,"columns");
		    new_type = DBL_T_COLSOBJ;
		    }
		else if (qy->RowCnt == 2) 
		    {
		    strcpy(filename,"rows");
		    new_type = DBL_T_ROWSOBJ;
		    }
		else 
		    {
		    nmFree(inf,sizeof(DblData));
		    /*mssError(1,"DBL","Table object has only two subobjects: 'rows' and 'columns'");*/
		    return NULL;
		    }
	        break;

	    case DBL_T_ROWSOBJ:
	        /** Get the filename from the primary key of the row. **/
		new_type = DBL_T_ROW;
	        break;

	    case DBL_T_COLSOBJ:
	        /** Loop through the columns in the TableInf structure. **/
		new_type = DBL_T_COLUMN;
		if (qy->RowCnt <= qy->TableInf->nCols)
		    {
		    memccpy(filename,qy->TableInf->Cols[qy->RowCnt-1], 0, 119);
		    filename[119] = 0;
		    }
		else
		    {
		    nmFree(inf,sizeof(DblData));
		    return NULL;
		    }
	        break;
	    }

	/** Build the filename. **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(filename) >= 255)
	    {
	    mssError(1,"DBL","Pathname too long for internal representation");
	    nmFree(inf,sizeof(DblData));
	    return NULL;
	    }
	*(ptr++) = '/';
	strcpy(ptr,filename);
	obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;

	/** Fill out the remainder of the structure. **/
	inf->Obj = obj;
	inf->Mask = 0600;
	inf->Type = new_type;
	inf->Node = qy->ObjInf->Node;
	obj->SubPtr = qy->ObjInf->Obj->SubPtr;
	dbl_internal_DetermineType(obj,inf);

    return (void*)inf;
    }


/*** dblQueryDelete - delete the contents of a query result set.  This is
 *** not yet supported.
 ***/
int
dblQueryDelete(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** dblQueryClose - close the query.
 ***/
int
dblQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pDblQuery qy = ((pDblQuery)(qy_v));

	/** Free the structure **/
	nmFree(qy,sizeof(DblQuery));

    return 0;
    }


/*** dblGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
dblGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int i;
    pDblTableInf tdata;

    	/** Name attribute?  String. **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

    	/** Content-type attribute?  String. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Annotation?  String. **/
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Attr type depends on object type. **/
	if (inf->Type == DBL_T_ROW)
	    {
	    tdata = inf->TData;
	    for(i=0;i<tdata->nCols;i++)
	        {
		if (!strcmp(attrname,tdata->Cols[i]))
		    {
		    }
		}
	    }
	else if (inf->Type == DBL_T_COLUMN)
	    {
	    if (!strcmp(attrname,"datatype")) return DATA_T_STRING;
	    }

	mssError(1,"DBL","Invalid column for GetAttrType");

    return -1;
    }


/*** dblGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
dblGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int i,t,minus,n;
    unsigned int msl,lsl,divtmp;
    pDblTableInf tdata;
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
		case DBL_T_DATABASE:
		    *(char**)val = inf->Node->Description;
		    break;
		case DBL_T_TABLE:
		    *(char**)val = inf->TData->Annotation;
		    break;
		case DBL_T_ROWSOBJ:
		    *(char**)val = "Contains rows for this table";
		    break;
		case DBL_T_COLSOBJ:
		    *(char**)val = "Contains columns for this table";
		    break;
		case DBL_T_COLUMN:
		    *(char**)val = "Column within this table";
		    break;
		case DBL_T_ROW:
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
		case DBL_T_DATABASE: *((char**)val) = "system/void"; break;
		case DBL_T_TABLE: *((char**)val) = "system/void"; break;
		case DBL_T_ROWSOBJ: *((char**)val) = "system/void"; break;
		case DBL_T_COLSOBJ: *((char**)val) = "system/void"; break;
		case DBL_T_ROW: 
		    {
		    if (inf->TData->HasContent)
		        *((char**)val) = "application/octet-stream";
		    else
		        *((char**)val) = "system/void";
		    break;
		    }
		case DBL_T_COLUMN: *((char**)val) = "system/void"; break;
		}
	    return 0;
	    }

	/** Outer type... **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE: *((char**)val) = "application/dbl"; break;
		case DBL_T_TABLE: *((char**)val) = "system/table"; break;
		case DBL_T_ROWSOBJ: *((char**)val) = "system/table-rows"; break;
		case DBL_T_COLSOBJ: *((char**)val) = "system/table-columns"; break;
		case DBL_T_ROW: *((char**)val) = "system/row"; break;
		case DBL_T_COLUMN: *((char**)val) = "system/column"; break;
		}
	    return 0;
	    }

	/** Column object?  Type is the only one. **/
	if (inf->Type == DBL_T_COLUMN)
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
	else if (inf->Type == DBL_T_ROW)
	    {
	    /** Get the table info. **/
	    tdata = inf->TData;

	    /** Search through the columns. **/
	    for(i=0;i<tdata->nCols;i++) if (!strcmp(tdata->Cols[i],attrname))
	        {
		ptr = inf->ColPtrs[i];
		t = tdata->ColTypes[i];
		}
	    }

	mssError(1,"DBL","Invalid column for GetAttrValue");

    return -1;
    }


/*** dblGetNextAttr - get the next attribute name for this object.
 ***/
char*
dblGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    pDblTableInf tdata;

	/** Attribute listings depend on object type. **/
	switch(inf->Type)
	    {
	    case DBL_T_DATABASE:
	        return NULL;
	
	    case DBL_T_TABLE:
	        return NULL;

	    case DBL_T_ROWSOBJ:
	        return NULL;

	    case DBL_T_COLSOBJ:
	        return NULL;

	    case DBL_T_COLUMN:
	        /** only attr is 'datatype' **/
		if (inf->CurAttr++ == 0) return "datatype";
	        break;

	    case DBL_T_ROW:
	        /** Get the table info. **/
		tdata = inf->TData;

	        /** Return attr in table inf **/
		if (inf->CurAttr < tdata->nCols) return tdata->Cols[inf->CurAttr++];
	        break;
	    }

    return NULL;
    }


/*** dblGetFirstAttr - get the first attribute name for this object.
 ***/
char*
dblGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = dblGetNextAttr(inf_v,oxt);

    return ptr;
    }


/*** dblSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
dblSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pDblData inf = DBL(inf_v);
    int type,rval;
    CS_COMMAND* cmd;
    CS_CONNECTION* sess;
    char sbuf[160];
    char* ptr;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Type == DBL_T_DATABASE) return -1;
	    }

	/** Changing the 'annotation'? **/
	if (!strcmp(attrname,"annotation"))
	    {
	    /** Choose the appropriate action based on object type **/
	    switch(inf->Type)
	        {
		case DBL_T_DATABASE:
		    memccpy(inf->Node->Description, *(char**)val, '\0', 255);
		    inf->Node->Description[255] = 0;
		    break;
		    
		case DBL_T_TABLE:
		    memccpy(inf->TData->Annotation, *(char**)val, '\0', 255);
		    inf->TData->Annotation[255] = 0;
		    while(strchr(inf->TData->Annotation,'"')) *(strchr(inf->TData->Annotation,'"')) = '\'';
		    break;

		case DBL_T_ROWSOBJ:
		case DBL_T_COLSOBJ:
		case DBL_T_COLUMN:
		    /** Can't change any of these (yet) **/
		    return -1;

		case DBL_T_ROW:
		    /** Not yet implemented :) **/
		    return -1;
		}
	    return 0;
	    }

	/** If this is a row, check the OXT. **/
	if (inf->Type == DBL_T_ROW)
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
		    type = dblGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
		    (*oxt)->AllocObj = 0;
		    (*oxt)->Object = NULL;
		    (*oxt)->Status = OXT_S_VISITED;
		    if (strlen(attrname) >= 64)
			{
			mssError(1,"DBL","Attribute name '%s' too long",attrname);
			return -1;
			}
		    strcpy((*oxt)->AttrName, attrname);
		    obj_internal_SetTreeAttr(*oxt, type, val);
		    }
	        else
	            {
		    /** No transaction.  Simply do an update. **/
		    type = dblGetAttrType(inf_v, attrname, oxt);
		    if (type < 0) return -1;
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


/*** dblAddAttr - add an attribute to an object.  This doesn't work for
 *** unix filesystem objects, so we just deny the request.
 ***/
int
dblAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** dblOpenAttr - open an attribute as if it were an object with 
 *** content.  The Sybase database objects don't yet have attributes that are
 *** suitable for this.
 ***/
void*
dblOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
dblGetFirstMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblGetNextMethod -- same as above.  Always fails. 
 ***/
char*
dblGetNextMethod(void* inf_v, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** dblExecuteMethod - No methods to execute, so this fails.
 ***/
int
dblExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree* oxt)
    {
    return -1;
    }



/*** dblInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
dblInitialize()
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
	memset(&DBL_INF,0,sizeof(DBL_INF));
	xhInit(&DBL_INF.DBNodes,255,0);
	xaInit(&DBL_INF.DBNodeList,127);
	DBL_INF.DblConfig = stLookup(CxGlobals.ParsedConfig,"dbl");
	DBL_INF.nSearchItems = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"DBL - DBL ISAM File Driver");
	drv->Capabilities = OBJDRV_C_FULLQUERY | OBJDRV_C_TRANS;
	/*drv->Capabilities = OBJDRV_C_TRANS;*/
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/dbl");
	xaAddItem(&(drv->RootContentTypes),"application/dbl-index");
	xaAddItem(&(drv->RootContentTypes),"application/dbl-definition");

	/** Setup the function references. **/
	drv->Open = dblOpen;
	drv->Close = dblClose;
	drv->Create = dblCreate;
	drv->Delete = dblDelete;
	drv->OpenQuery = dblOpenQuery;
	drv->QueryDelete = dblQueryDelete;
	drv->QueryFetch = dblQueryFetch;
	drv->QueryClose = dblQueryClose;
	drv->Read = dblRead;
	drv->Write = dblWrite;
	drv->GetAttrType = dblGetAttrType;
	drv->GetAttrValue = dblGetAttrValue;
	drv->GetFirstAttr = dblGetFirstAttr;
	drv->GetNextAttr = dblGetNextAttr;
	drv->SetAttrValue = dblSetAttrValue;
	drv->AddAttr = dblAddAttr;
	drv->OpenAttr = dblOpenAttr;
	drv->GetFirstMethod = dblGetFirstMethod;
	drv->GetNextMethod = dblGetNextMethod;
	drv->ExecuteMethod = dblExecuteMethod;

	nmRegister(sizeof(DblTableInf),"DblTableInf");
	nmRegister(sizeof(DblData),"DblData");
	nmRegister(sizeof(DblQuery),"DblQuery");
	nmRegister(sizeof(DblNode),"DblNode");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;
	DBL_INF.ObjDriver = drv;

    return 0;
    }


MODULE_INIT(dblInitialize);
MODULE_PREFIX("dbl");
MODULE_DESC("DBL ISAM File ObjectSystem Driver");
MODULE_VERSION(0,9,0);
MODULE_IFACE(CX_CURRENT_IFACE);
