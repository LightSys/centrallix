#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "multiquery.h"
#include "st_param.h"
#include "param.h"
#include "cxlib/qprintf.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_query.c						*/
/* Author:	Tim Young (TCY) / Greg Beeley (GRB)			*/
/* Description:	This driver offers the ability to use a query stored	*/
/*		in a structure file as a single entity.  You can     	*/
/*		reference a .qy file as a sybase table, csv, etc...	*/
/*		These stored queries are much like Views in traditional	*/
/*		RDBMS systems.						*/
/*									*/
/************************************************************************/


#define QY_MAX_PARAM	(16)

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		Mode;
    pSnNode	Node;
    pStpInf	ParsedInf;
    pPseudoObject CurrentFetch;
    StringVec   SVvalue;
    IntVec      IVvalue;
    int 	Type;
    char*	Name;
    int		CurrAttr;
    int		Version;
    pObject	MultiQueryObject;
    pParam	Parameters[QY_MAX_PARAM];
    int		nParameters;
    char*	SQL;
    char*	NameExpression;
    pParamObjects   ObjList;
    }
    QyData, *pQyData;


/*** Flags for QyData and QyQuery ***/
#define QY_F_USEHAVING_NAME		1	/* use HAVING clause when looking up by NAME */
#define QY_F_USEHAVING_CRITERIA		2	/* use HAVING clause when applying additional criteria */


#define QY(x) ((pQyData)(x))

#define QY_T_ITEM	1
#define QY_T_LIST	2


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pQyData	Data;
    pObjQuery	MultiQuery;
    int		Flags;
    char	NameBuf[256];
    int		ItemCnt;
    }
    QyQuery, *pQyQuery;


/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    QY_INF;


/*** qy_internal_NameToExpression - create an expression tree that will
 *** be passed to the query to look up one particular row using the 'name'
 *** attribute.
 ***/
pExpression
qy_internal_NameToExpression(char* name)
    {
    pExpression exp;
    char expstr[128];

	/** Build the string **/
	if (qpfPrintf(NULL, expstr, sizeof(expstr), ":name = %STR&DQUOT", name) < 0)
	    return NULL;

	/** Compile it. **/
	exp = expCompileExpression(expstr, NULL, MLX_F_ICASE | MLX_F_FILENAMES, 0);
    
    return exp;
    }


/*** qy_internal_StartQuery - start the query, as specified by the sql
 *** setting in the .qy file, and applying any query/parameter objects
 *** that are needed (visible as ":parameters:xyz" in the query itself).
 ***
 *** Returns the resulting open MultiQuery handle.
 ***/
pObjQuery
qy_internal_StartQuery(pQyData inf, char* name, pExpression criteria)
    {
    XString sql_string;
    pObjQuery mq;

	/** Build the SQL string **/
	xsInit(&sql_string);
	xsQPrintf(&sql_string, "%STR %[WHERE%] %[%STR = %STR&DQUOT%] %[HAVING%] ",
		inf->SQL,
		(name != NULL),
		name != NULL,
		inf->NameExpression,
		name,
		(criteria != NULL));

	/** Append on the criteria, if needed **/
	if (criteria)
	    expGenerateText(criteria, inf->ObjList, xsWrite, &sql_string, '\0', "cxsql", 0);

	/** Fire off the query **/
	mq = objMultiQuery(inf->Obj->Session, sql_string.String, inf->ObjList, 0);
	if (mq) objUnmanageQuery(inf->Obj->Session, mq);

	/** Release memory for the SQL string **/
	xsDeInit(&sql_string);

    return mq;
    }


/*** internal close & cleanup routine
 ***/
int
qy_internal_Close(pQyData inf)
    {
    int i;

	if (inf)
	    {
	    if (inf->MultiQueryObject)
		objClose(inf->MultiQueryObject);
	    for(i=0;i<inf->nParameters;i++)
		paramFree(inf->Parameters[i]);
	    if (inf->SQL)
		nmSysFree(inf->SQL);
	    if (inf->Node)
		inf->Node->OpenCnt--;
	    if (inf->ObjList)
		expFreeParamList(inf->ObjList);
	    if (inf->Name)
		{
		nmSysFree(inf->Name);
		inf->Name = NULL;
		}
	    if (inf->NameExpression)
		nmSysFree(inf->NameExpression);
	    nmFree(inf, sizeof(QyData));
	    }

    return 0;
    }


/*** qy_internal_GetParamType() - get the data type of a parameter
 ***/
int
qy_internal_GetParamType(void* inf_v, char* attrname)
    {
    pQyData inf = QY(inf_v);
    pParam param;
    int i;

	/** Find the param **/
	for(i=0;i<inf->nParameters;i++)
	    {
	    param = inf->Parameters[i];
	    if (!strcmp(attrname, param->Name))
		{
		if (!param->Value)
		    return -1;
		return param->Value->DataType;
		}
	    }

    return -1;
    }


/*** qy_internal_GetParamValue() - get the value of a parameter
 ***/
int
qy_internal_GetParamValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    pQyData inf = QY(inf_v);
    pParam param;
    int i;

	/** Find the param **/
	for(i=0;i<inf->nParameters;i++)
	    {
	    param = inf->Parameters[i];
	    if (!strcmp(attrname, param->Name))
		{
		if (!param->Value)
		    return 1;
		if (datatype != param->Value->DataType)
		    {
		    mssError(1,"QY","Type mismatch accessing parameter '%s'", param->Name);
		    return -1;
		    }
		if (param->Value->Flags & DATA_TF_NULL)
		    return 1;
		objCopyData(&(param->Value->Data), val, datatype);
		return 0;
		}
	    }

    return -1;
    }


/*** qy_internal_SetParamValue() - set the value of a parameter - not
 *** supported.
 ***/
int
qy_internal_SetParamValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    return -1;
    }


/*** qyOpen - open an object.
 ***/
void*
qyOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pQyData inf;
    pSnNode node = NULL;
    pStructInf param_inf;
    char* sql;
    char* name_expr;
    int i,j;
    pParam one_param;
    pStruct one_open_ctl, open_ctl;
    pObjQuery lookup_qy;
    char* endorsement_name;

	/** Allocate the structure **/
	inf = (pQyData)nmMalloc(sizeof(QyData));
	if (!inf) goto error;

	/** Zero out the structure **/
	memset(inf,0,sizeof(QyData));

	/** Set the parent Object **/
	inf->Obj = obj;

	/** Set the mask passed in from opening (see ../doc/OSDrivers.txt) **/
	inf->Mask = mask;

	/** Multiquery **/
	inf->CurrentFetch=NULL;

	inf->CurrAttr = 0;

	/** Object List **/
	inf->ObjList = expCreateParamList();
	expAddParamToList(inf->ObjList,"this",NULL,EXPR_O_CURRENT);
	expAddParamToList(inf->ObjList,"parameters",(void*)inf,0);
	expSetParamFunctions(inf->ObjList, "parameters", qy_internal_GetParamType, qy_internal_GetParamValue, qy_internal_SetParamValue);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		mssError(0,"QY","Could not create new node object");
		goto error;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** Security check **/
	if (node && endVerifyEndorsements(node->Data, stGetObjAttrValue, &endorsement_name) < 0)
	    {
	    mssError(1,"QY","Security check failed - endorsement '%s' required", endorsement_name);
	    goto error;
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    mssError(0,"QY","Could not open node object");
	    goto error;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));

	/** Link to the node, increasing its reference count **/
	inf->Node->OpenCnt++;

	/** Get SQL string **/
	if (stAttrValue(stLookup(inf->Node->Data,"sql"),NULL,&sql,0) != 0)
	    {
	    mssError(1,"QY","'sql' property must be supplied for query objects");
	    goto error;
	    }
	inf->SQL = nmSysStrdup(sql);

	/** Get Name Expression string **/
	if (stAttrValue(stLookup(inf->Node->Data,"name_expression"),NULL,&name_expr,0) != 0)
	    {
	    name_expr = ":name";
	    }
	inf->NameExpression = nmSysStrdup(name_expr);

	/** Get parameter list **/
	for(i=0; i<inf->Node->Data->nSubInf; i++)
	    {
	    if (inf->nParameters >= QY_MAX_PARAM)
		{
		mssError(1,"QY","Too many parameters in query; maximum is %d", QY_MAX_PARAM);
		goto error;
		}
	    param_inf = inf->Node->Data->SubInf[i];
	    if (stStructType(param_inf) == ST_T_SUBGROUP && !strcmp(param_inf->UsrType,"query/parameter"))
		{
		/** Found a parameter.  Now set it up **/
		one_param = paramCreateFromInf(param_inf);
		if (!one_param) goto error;
		inf->Parameters[inf->nParameters++] = one_param;

		/** Set the value supplied by the user, if needed. **/
		open_ctl = obj->Pathname->OpenCtl[obj->SubPtr-1];
		if (open_ctl)
		    {
		    for(j=0; j<open_ctl->nSubInf; j++)
			{
			one_open_ctl = open_ctl->SubInf[j];
			if (!strcmp(one_open_ctl->Name, one_param->Name))
			    {
			    paramSetValueFromInfNe(one_param, one_open_ctl);
			    break;
			    }
			}
		    }

		/** Apply hints **/
		paramEvalHints(one_param, NULL, obj->Session);
		}
	    }

	/** If the .qy file is the end of the Pathname, We  **/
	/** will not need to chop apart extra where clauses **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    inf->Type = QY_T_LIST;
	    inf->Name = nmSysStrdup(inf->Node->Data->Name);
	    /** Number of path items we have looked at **/
	    obj->SubCnt = 1;
	    }
	else
	    {
	    inf->Type = QY_T_ITEM;
	    obj->SubCnt = 2;
	    inf->Name = nmSysStrdup(obj_internal_PathPart(obj->Pathname, obj->SubPtr, 1));
	    obj_internal_PathPart(obj->Pathname, 0, 0);

	    /** Run the query to find the requested object, by name **/
	    lookup_qy = qy_internal_StartQuery(inf, inf->Name, NULL);
	    if (!lookup_qy)
		{
		mssError(1,"QY","Could not open SQL query to fetch requested object '%s'", inf->Name);
		goto error;
		}
	    inf->MultiQueryObject = objQueryFetch(lookup_qy, inf->Obj->Mode);
	    objQueryClose(lookup_qy);
	    lookup_qy = NULL;

	    /** Didn't find it? **/
	    if (!inf->MultiQueryObject)
		goto error;
	    else
		objUnmanageObject(inf->Obj->Session, inf->MultiQueryObject);
	    }

	return (void*)inf;

    error:
	/** Erroring out.  Clean up after ourselves, then return NULL **/
	qy_internal_Close(inf);
	return NULL;
    }


/*** qyClose - close an open object.
 ***/
int
qyClose(void* inf_v, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);

    	/** Write the node first, if need be. **/
	/** If it is dirty, it will write.  Otherwise simply move on **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	/** Release the memory **/
	inf->Node->OpenCnt --;
	qy_internal_Close(inf);

    return 0;
    }


/*** qyCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
qyCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = qyOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	qyClose(inf, oxt);

    return 0;
    }


/*** qyDeleteObj - delete and close an open object.
 ***/
int
qyDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    pQyData inf = (pQyData)inf_v;
    int rval = 0;

	/** Delete the underlying object, if we can. **/
	if (inf->Type == QY_T_ITEM && inf->MultiQueryObject)
	    {
	    rval = objDeleteObj(inf->MultiQueryObject);
	    inf->MultiQueryObject = NULL;
	    }
	else
	    {
	    rval = -1;
	    }

	/** Clean up. **/
	qy_internal_Close(inf);

    return rval;
    }


/*** qyDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
qyDelete(pObject obj, pObjTrxTree* oxt)
    {
    pQyData inf;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pQyData)qyOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Call DeleteObj with it **/
	return qyDeleteObj(inf, oxt);
    }


/*** qyRead - Structure elements have no content.  Fails.
 ***/
int
qyRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    return (inf->Type == QY_T_ITEM && inf->MultiQueryObject)?(objRead(inf->MultiQueryObject, buffer, maxcnt, offset, flags)):-1;
    }


/*** qyWrite - Again, no content.  This fails.
 ***/
int
qyWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    return (inf->Type == QY_T_ITEM && inf->MultiQueryObject)?(objWrite(inf->MultiQueryObject, buffer, cnt, offset, flags)):-1;
    }


/*** qyOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
qyOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    pQyQuery qy;

	/** Not supported for QY_T_ITEM **/
	if (inf->Type == QY_T_ITEM)
	    {
	    mssError(1,"QY","Queries not supported on query object's row items");
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pQyQuery)nmMalloc(sizeof(QyQuery));
	if (!qy) return NULL;
	/** zero out memory **/
	memset(qy, 0, sizeof(QyQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;

	/** Start the query **/
	qy->MultiQuery = qy_internal_StartQuery(inf, NULL, query->Tree);
	if (!qy->MultiQuery)
	    {
	    nmFree(qy,sizeof(QyQuery));
	    return NULL;
	    }
    
    return (void*)qy;
    }


/*** qyQueryFetch - get the next object from the underlying multiquery.
 ***/
void*
qyQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pQyQuery qy = ((pQyQuery)(qy_v));
    pQyData inf;
    char* ptr;

	/** Alloc the structure **/
	inf = (pQyData)nmMalloc(sizeof(QyData));
	if (!inf) return NULL;
	memset(inf, 0, sizeof(QyData));
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Mode = mode;

	/** increase count of open objects to the node **/
	inf->Node->OpenCnt++;
	inf->Type = QY_T_ITEM;
	inf->Name = NULL;
	inf->Obj = obj;

	/** Fetch next result from underlying multiquery **/
	inf->MultiQueryObject = objQueryFetch(qy->MultiQuery, mode);
	if (!inf->MultiQueryObject)
	    {
	    /** if no more items, return NULL **/
	    qy_internal_Close(inf);
	    return NULL;
	    }
	objUnmanageObject(inf->Obj->Session, inf->MultiQueryObject);
	qy->ItemCnt++;

	/** Get the name of the query subobject. **/
	ptr = NULL;
	objGetAttrValue(inf->MultiQueryObject, "name", DATA_T_STRING, POD(&ptr));
	if (!ptr)
	    {
	    qy_internal_Close(inf);
	    return NULL;
	    }
	if (obj_internal_AddToPath(obj->Pathname, ptr) < 0)
	    {
	    mssError(1,"QUERY","Query result pathname exceeds internal limits");
	    qy_internal_Close(inf);
	    return NULL;
	    }
	inf->Name = nmSysStrdup(ptr);

    return (void*)inf;
    }


/*** qyQueryClose - close the query.
 ***/
int
qyQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pQyQuery qy = (pQyQuery)qy_v;

	/** Free the structure **/
	if (qy->MultiQuery) objQueryClose(qy->MultiQuery); 
	nmFree(qy,sizeof(QyQuery));

    return 0;
    }


/*** qyGetAttrType - get the type (DATA_T_qy) of an attribute by name.
 ***/
int
qyGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);

	if (inf == NULL) return -1;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Put checking for your own attributes here. **/
	if (inf->Type == QY_T_LIST)
	    {
	    /** None, currently **/
	    return -1;
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    return objGetAttrType(inf->MultiQueryObject, attrname);
	    }

    return -1;
    }


/*** qyGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
qyGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);

	if (inf == NULL) return -1;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    val->String = inf->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (inf->Type == QY_T_LIST)
		val->String = "system/void";
	    else if (objGetAttrValue(inf->MultiQueryObject, attrname, DATA_T_STRING, val) != 0)
		val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (inf->Type == QY_T_LIST) *((char**)val) = "system/query";
	    else if (inf->Type == QY_T_ITEM) *((char**)val) = "system/row";
	    return 0;
	    }

	if (inf->Type == QY_T_ITEM)
	    {
	    /** Pass the attribute question to the multiquery **/
	    return objGetAttrValue(inf->MultiQueryObject, attrname, datatype, val);
	    }
	if (inf->Type == QY_T_LIST)
	    {
	    return -1;
	    }

	mssError(1,"QY","Could not locate requested attribute");

    return -1;
    }


/*** qyGetNextAttr - get the next attribute name for this object.
 ***/
char*
qyGetNextAttr(pQyData inf, pObjTrxTree oxt)
    {
    char* ptr;

	if (inf->Type == QY_T_LIST)
	    {
	    return NULL;
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    ptr = objGetNextAttr(inf->MultiQueryObject);
	    return ptr;
	    }

        /** No find? **/
    return NULL;
    }


/*** qyGetFirstAttr - get the first attribute name for this object.
 ***/
char*
qyGetFirstAttr(pQyData inf, pObjTrxTree oxt)
    {
    char* ptr;

	if (inf->Type == QY_T_LIST)
	    {
	    return NULL;
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    ptr = objGetFirstAttr(inf->MultiQueryObject);
	    return ptr;
	    }

    return NULL;
    }


/*** qySetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
qySetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree oxt)
    {
    pQyData inf = QY(inf_v);

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    return -1;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    return -1;
	    }

	if (inf->Type == QY_T_LIST)
	    {
	    return -1;
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    return objSetAttrValue(inf->MultiQueryObject, attrname, datatype, val);
	    }

    return -1;
    }


/*** qyAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some.
 ***/
int
qyAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pQyData inf = QY(inf_v);
    /** Pass on to Multiquery **/
    return objAddAttr(inf->MultiQueryObject, attrname, type, val);	
    }


/*** qyOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
qyOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** qyGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
qyGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** qyGetNextMethod -- same as above.  Always fails. 
 ***/
char*
qyGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** qyExecuteMethod - No methods to execute, so this fails.
 ***/
int
qyExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** qyPresentationHints - return the presentation hints for the object.
 *** Here, we just pass it through to the MQ layer.
 ***/
pObjPresentationHints
qyPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    pObjPresentationHints hints;

	if (inf->Type == QY_T_ITEM && inf->MultiQueryObject)
	    hints = objPresentationHints(inf->MultiQueryObject, attrname);
	else
	    hints = NULL;

    return hints;
    }


/*** qyInfo - Return the capabilities of the object.
 ***/
int
qyInfo(void* inf_v, pObjectInfo info)
    {
    pQyData inf = QY(inf_v);
    pObjectInfo ll_info;

	if (inf->Type == QY_T_ITEM && inf->MultiQueryObject) 
	    {
	    /** Get basic character of object from underlying object **/
	    ll_info = objInfo(inf->MultiQueryObject);
	    if (ll_info)
		{
		memcpy(info, ll_info, sizeof(ObjectInfo));
		return 0;
		}
	    }

    return -1;
    }


/*** qyInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
qyInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&QY_INF,0,sizeof(QY_INF));
	QY_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"QY - Query Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/query");

	/** Setup the function references. **/
	drv->Open = qyOpen;
	drv->Close = qyClose;
	drv->Create = qyCreate;
	drv->Delete = qyDelete;
	drv->DeleteObj = qyDeleteObj;
	drv->OpenQuery = qyOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = qyQueryFetch;
	drv->QueryClose = qyQueryClose;
	drv->Read = qyRead;
	drv->Write = qyWrite;
	drv->GetAttrType = qyGetAttrType;
	drv->GetAttrValue = qyGetAttrValue;
	drv->GetFirstAttr = qyGetFirstAttr;
	drv->GetNextAttr = qyGetNextAttr;
	drv->SetAttrValue = qySetAttrValue;
	drv->AddAttr = qyAddAttr;
	drv->OpenAttr = qyOpenAttr;
	drv->GetFirstMethod = qyGetFirstMethod;
	drv->GetNextMethod = qyGetNextMethod;
	drv->ExecuteMethod = qyExecuteMethod;
	drv->Info = qyInfo;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(QyData),"QyData");
	nmRegister(sizeof(QyQuery),"QyQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

