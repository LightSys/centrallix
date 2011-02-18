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
/* Module: 	objdrv_query.c						*/
/* Author:	Tim Young						*/
/* Creation:	<create data goes here>					*/
/* Description:	This driver offers the ability to use a query stored	*/
/*		in a structure file as a single entity.  You can     	*/
/*		reference a .qy file as a sybase table, csl, etc...	*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_query.c,v 1.6 2011/02/18 03:53:33 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_query.c,v $

    $Log: objdrv_query.c,v $
    Revision 1.6  2011/02/18 03:53:33  gbeeley
    MultiQuery one-statement security, IS NOT NULL, memory leaks

    - fixed some memory leaks, notated a few others needing to be fixed
      (thanks valgrind)
    - "is not null" support in sybase & mysql drivers
    - objMultiQuery now has a flags option, which can control whether MQ
      allows multiple statements (semicolon delimited) or not.  This is for
      security to keep subqueries to a single SELECT statement.

    Revision 1.5  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.4  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.3  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:06  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:05  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    pSnNode	Node;
    pStpInf	ParsedInf;
    pPseudoObject CurrentFetch;
    StringVec   SVvalue;
    IntVec      IVvalue;
    int 	Type;
    pXString	Name;
    int		CurrAttr;
    int		Version;
    pObjQuery	MultiQuery;
    pObject	MultiQueryObject;
    }
    QyData, *pQyData;


#define QY(x) ((pQyData)(x))

#define QY_T_ITEM	1
#define QY_T_LIST	2

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pQyData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    QyQuery, *pQyQuery;


/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    Qy_INF;


/*** QyOpen - open an object.
 ***/
void*
QyOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pQyData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    pStructInf find_inf;
    char* ptr;
    char* name;
    char* sql;
    char newsql[254];
    char sqlstring[254];
    pXString tmpXString;
    struct passwd* pwent;

	/** Allocate the structure **/
	inf = (pQyData)nmMalloc(sizeof(QyData));
	if (!inf) return NULL;
	/** Zero out the record **/
	memset(inf,0,sizeof(QyData));
	/** Set the parent Object **/
	inf->Obj = obj;
	/** Set the mask passed in from opening (see ../doc/OSDrivers.txt) **/
	inf->Mask = mask;
	/** Multiquery **/
	inf->CurrentFetch=NULL;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
		nmFree(inf,sizeof(QyData));
		mssError(0,"QY","Could not create new node object");
		return NULL;
		}
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
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    nmFree(inf,sizeof(QyData));
	    mssError(0,"QY","Could not open node object");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	/** increase the count of open items **/
	inf->Node->OpenCnt++;
	 /** Check format version **/
        if (stAttrValue(stLookup(inf->Node->Data,"version"),&(inf->Version),NULL,0) < 0)
            {
	    inf->Version = 1;
	    }
	else
	    {
	    inf->Version = 2;
	    }

	    /** If this is not the first object opened **/	
	    if ((pQyData)(obj->Data))
		{
		inf->ParsedInf = ((pQyData)(obj->Data))->ParsedInf;
		}
	    else
		{
		/** Open a parsed node **/
		inf->ParsedInf = stpAllocInf(inf->Node,obj->Pathname->OpenCtl[obj->SubPtr-1],obj,2);
		}
	    inf->ParsedInf->OpenCnt++;


/** **/

	/** If the .qy file is the end of the Pathname, We  **/
	/** will not need to chop apart extra where clauses **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    inf->Type = QY_T_LIST;
	    inf->CurrAttr = 0;
	    inf->Name = NULL;
	    /** Number of path items we have looked at **/
	    obj->SubCnt = 1;
	    }
	else
	    {
/** This code is not completely working yet.  do show query.qy/primarykey and watch it blow **/
	    /** Pass the override parameter the next pathpart, that could be "where" info **/
	    stpParsePathOverride(inf->ParsedInf,obj->Pathname->Elements[obj->SubPtr + 1]);

	    inf->Type = QY_T_ITEM;
	    obj->SubCnt = 2;
	    if ((pQyData)(obj->Data)) 
		{
		/** If the multiquery object is already open **/
		/** This really should never happen though   **/
		inf->MultiQuery = ((pQyData)(obj->Data))->MultiQuery;
		}
	    else
		{
		/** Open a new Multiquery, returning the one row **/
	    	find_inf = stpLookup(inf->ParsedInf,"sql");
	    	sql = (char*)find_inf->StrVal[0];
	    	find_inf = stpLookup(inf->ParsedInf,"primarykey");
		tmpXString = stpSubstParam(inf->ParsedInf, sql);
		memccpy(newsql,tmpXString->String,'\0',254);
		newsql[254]='\0';
	    	sprintf (sqlstring, "%s where :%s = %s",newsql,
			find_inf->StrVal[0],
			obj->Pathname->Elements[obj->SubPtr]);
	        inf->MultiQuery = objMultiQuery(inf->Obj->Session, sqlstring, NULL, 0);
/** Right now only returns one row **/
	        inf->MultiQueryObject = objQueryFetch(inf->MultiQuery,0);
		}
	    }

    return (void*)inf;
    }


/*** QyClose - close an open object.
 ***/
int
QyClose(void* inf_v, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);

    	/** Write the node first, if need be. **/
	/** If it is dirty, it will write.  Otherwise simply move on **/
	snWriteNode(inf->Obj->Prev, inf->Node);

	/** Release the malloc'd information **/
        /** free the override and parsed st structure **/
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	inf->ParsedInf->OpenCnt --;
	if (inf->ParsedInf->OpenCnt == 0)
	    stpFreeInf(inf->ParsedInf);
	if (inf->Name)
	    {
	    xsDeInit(inf->Name);
	    nmFree(inf->Name, sizeof(XString));
	    inf->Name = NULL;
	    }
	nmFree(inf,sizeof(QyData));

    return 0;
    }


/*** QyCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
QyCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = QyOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	QyClose(inf, oxt);

    return 0;
    }


/*** QyDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
QyDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pQyData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pQyData)QyOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		QyClose(inf, oxt);
		mssError(1,"QY","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 1;
	    if (!is_empty)
	        {
		QyClose(inf, oxt);
		mssError(1,"QY","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    nmFree(inf,sizeof(QyData));
	    return -1;
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	inf->Node->OpenCnt --;
	inf->ParsedInf->OpenCnt --;
	if (inf->ParsedInf->OpenCnt == 0)
	    stpFreeInf(inf->ParsedInf);
	if (inf->Name)
	    {
	    xsDeInit(inf->Name);
	    nmFree(inf->Name, sizeof(XString));
	    inf->Name = NULL;
	    }
	nmFree(inf,sizeof(QyData));

    return 0;
    }


/*** QyRead - Structure elements have no content.  Fails.
 ***/
int
QyRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    mssError(1,"QY","Multiqueries do not have content.");
    return -1;
    }


/*** QyWrite - Again, no content.  This fails.
 ***/
int
QyWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    mssError(1,"QY","Multiqueries do not have content.");
    return -1;
    }


/*** QyOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
QyOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
#if 00
    pObjQuery qyinf = NULL;
    char* where_clause;
    char* usr_where_clause;
    int len;
    void* subst_ptr[8];
    int alloc_ptr[8];
    int subst_int[8];
    double subst_dbl[8];
    int subst_types[8];
    int n_subst;
    char* ptr;
    char* sptr;
    char attrname[33];
    char nbuf[16];
    int i;

    	/** Get the where clause restrictions **/
	where_clause = qy->ItemWhere;
	usr_where_clause = qy->Query->QyText;
	if (!usr_where_clause) usr_where_clause = "";
	if (*usr_where_clause || *where_clause) 
	    len = strlen(usr_where_clause) + strlen(where_clause) + 12;
	else
	    len = 0;

	/** Perform any needed substitution on the parameterized where clause **/
	if (*where_clause)
	    {
	    n_subst = 0;
	    ptr = where_clause;
	    while(ptr = strstr(ptr,"::"))
	        {
		ptr += 2;

		/** Get the attribute name after the :: **/
		i = 0;
		while(((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= 'a' && *ptr <= 'z') || 
		      (*ptr >= '0' && *ptr <= '9') || *ptr == '_') && i < 32)
		    {
		    attrname[i++] = *(ptr++);
		    }
		attrname[i] = 0;

		/** Get the attr type and string -or- integer value **/
		if (qy->ObjInf->LLObj && (subst_types[n_subst] = objGetAttrType(qy->ObjInf->LLObj, attrname)) >= 0)
		    {
		    alloc_ptr[n_subst] = 0;
		    subst_ptr[n_subst] = NULL;
		    switch(subst_types[n_subst])
		        {
			case DATA_T_INTEGER:
			    if (objGetAttrValue(qy->ObjInf->LLObj, attrname, DATA_T_INTEGER, &(subst_int[n_subst])) == 1)
			        {
			        subst_types[n_subst] = -1;
			        len += 6;
			        }
			    else
			        {
			        len += 12;
			        }
			    break;

			case DATA_T_STRING:
			    if (objGetAttrValue(qy->ObjInf->LLObj, attrname, DATA_T_STRING, &sptr) == 1)
			        {
				subst_types[n_subst] = -1;
				len += 6;
				}
			    else
			        {
			        subst_ptr[n_subst] = (void*)nmSysStrdup(sptr);
				alloc_ptr[n_subst] = 1;
			        len += strlen(sptr);
				}
			    break;
			}
		    n_subst++;
		    }
		else
		    {
		    mssError(0, "QY", "Parent attribute in .qy file where clause does not exist.");
		    return NULL;
		    }
		}
	    }

	/** Allocate memory for the new where clause **/
	if (len) qy->QyText = (char*)nmMalloc(len);
	if (len && !(qy->QyText)) 
	    {
	    return NULL;
	    }

	/** Build the "final" where clause to use **/
	ptr = qy->QyText;
	if (*usr_where_clause)
	    {
	    ptr = memccpy(ptr, "(", '\0', len)-1;
	    ptr = memccpy(ptr, usr_where_clause, '\0', len)-1;
	    ptr = memccpy(ptr, ")", '\0', len)-1;
	    }
	if (*where_clause)
            {
            if (*usr_where_clause)
	        ptr = memccpy(ptr, " AND (", '\0', len)-1;
	    else
	        ptr = memccpy(ptr, "(", '\0', len)-1;
            sptr = where_clause;
            i = 0;
            while(*sptr)
                {
                if (*sptr == ':' && *(sptr+1) == ':')
                    {
		    switch(subst_types[i])
		        {
			case -1: /* NULL */
			    ptr = memccpy(ptr, "NULL", '\0', len)-1;
			    break;

			case DATA_T_INTEGER:
                            sprintf(nbuf,"%d",subst_int[i]);
                            ptr = memccpy(ptr, nbuf, '\0', 16)-1;
			    break;

			case DATA_T_STRING:
                            *(ptr++) = '"';
                            ptr = memccpy(ptr, subst_ptr[i], '\0', len)-1;
                            *(ptr++) = '"';
			    break;
                        }
		    if (alloc_ptr[i] && subst_ptr[i]) 
		        {
			nmSysFree(subst_ptr[i]);
			alloc_ptr[i] = 0;
			}
                    sptr += 2;
                    while(((*sptr >= 'A' && *sptr <= 'Z') || (*sptr >= 'a' && *sptr <= 'z') || 
                           (*sptr >= '0' && *sptr <= '9') || *sptr == '_')) sptr++;
		    i++;
                    }
                else
                    {
                    *(ptr++) = *(sptr++);
                    }
                }
            ptr = memccpy(ptr, ")", '\0', len)-1;
            }
	*ptr = '\0';

	/** Issue the query. **/
	qy->LLQueryObj = objOpen(qy->ObjInf->Obj->Session, qy->ItemSrc, O_RDONLY, 0600, "system/directory");
	if (qy->LLQueryObj) 
	    {
	    qyinf = objOpenQuery(qy->LLQueryObj, qy->QyText, NULL,NULL,NULL);
	    if (!qyinf) objClose(qy->LLQueryObj);
	    }
#endif

    pQyData inf = QY(inf_v);
    pQyQuery qy;
    char* sql;
    char newsql[255];
    pXString tmpXString;
    pStructInf find_inf;

    	/** If there are more parameters, they are part of the where **/
	/** clause and need to be parsed out.  For now, just exit    **/
	if (inf->Obj->SubPtr != inf->Obj->Pathname->nElements)
	    {
	    mssError(1,"STP","A where in this form is unsupported");
	    return NULL;
	    }

	/** Allocate the query structure **/
	qy = (pQyQuery)nmMalloc(sizeof(QyQuery));
	if (!qy) return NULL;
	/** zero out memory **/
	memset(qy, 0, sizeof(QyQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;

	find_inf = stLookup(inf->Node->Data,"sql");
	sql = (char*)find_inf->StrVal[0];
	tmpXString = stpSubstParam(inf->ParsedInf, sql);
	memccpy(newsql,tmpXString->String,'\0',254);
	newsql[254]='\0';
	inf->MultiQuery = objMultiQuery(inf->Obj->Session, newsql, NULL, 0);
    
    return (void*)qy;
    }


/*** QyQueryFetch - get the next directory entry as an open object.
 ***/
void*
QyQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pQyQuery qy = ((pQyQuery)(qy_v));
    pQyData inf;
    char* new_obj_name = "newobj";
    char* ptr;

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/

	    {
	    /** if ((qy->Data->Type == QY_T_LIST) &&
	        (qy->ItemCnt > qy->Data->Node->Data->nSubInf)) return NULL; **/
	    /** Alloc the structure **/
	    inf = (pQyData)nmMalloc(sizeof(QyData));
	    if (!inf) return NULL;
	    strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	    inf->Node = qy->Data->Node;
	    inf->ParsedInf = qy->Data->ParsedInf;
	    /** increase count of open objects to the node **/
	    inf->Node->OpenCnt++;
	    inf->ParsedInf->OpenCnt++;
	    inf->Type = QY_T_ITEM;
	    inf->Name = NULL;
	    inf->Obj = obj;
	    /** increase count of open objects to query **/
	    /** This variable holds the CurrAttr number for next **/
	    inf->CurrAttr = qy->ItemCnt;
	    inf->MultiQuery = qy->Data->MultiQuery;
	    inf->MultiQueryObject = objQueryFetch(qy->Data->MultiQuery,mode);
	    if (!inf->MultiQueryObject)
		{
		/** if no more items, return NULL **/
		nmFree(inf, sizeof(QyData));
		return NULL;
		}
	    qy->ItemCnt++;
    	    return (void*)inf;
	    }
 	return NULL;

    }


/*** QyQueryClose - close the query.
 ***/
int
QyQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    int i;
    pQyQuery qy = (pQyQuery)qy_v;

	/** Free the structure **/
	if (!qy->Data->MultiQuery) objQueryClose(qy->Data->MultiQuery); 
	nmFree(qy,sizeof(QyQuery));
    return 0;
    }


/*** QyGetAttrType - get the type (DATA_T_Qy) of an attribute by name.
 ***/
int
QyGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    int i;
    pStructInf find_inf;

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
	    return stpAttrType(inf->ParsedInf,attrname);
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    /** return DATA_T_STRING; **/
	    return objGetAttrType((void*)inf->MultiQueryObject, attrname);
	    }

    return -1;
    }


/*** QyGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
QyGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pQyData inf = QY(inf_v);
    pStructInf find_inf, tmp_inf;
    char* ptr;
    int i;
    int tmptype;
    char* tstr;

	if (inf == NULL) return -1;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Type == QY_T_LIST)
		{
	    	find_inf = stpLookup(inf->ParsedInf,"primarykey");
/*		if (!find_inf) return -1; */
	   	val->String = (char*)find_inf->StrVal[0];
	    	return 0;
		}
	    if (inf->Type == QY_T_ITEM)
		{
	    	find_inf = stpLookup(inf->ParsedInf,"primarykey");
		if (!find_inf) return -1;
		ptr = (char*)find_inf->StrVal[0];
		if (!inf->MultiQueryObject) return -1;
	        objGetAttrValue((void*)inf->MultiQueryObject, ptr, datatype, val);
		tmptype = objGetAttrType((void*)inf->MultiQueryObject, ptr);
		if (tmptype != DATA_T_STRING)
		    tstr = objDataToStringTmp(tmptype, val, 0);
		else
		    tstr = val->String;
		inf->Name = (pXString)nmMalloc(sizeof(XString));
		xsInit(inf->Name);
		xsConcatenate(inf->Name,tstr,-1);
	   	val->String = inf->Name->String;
	    	return 0;
		}
	    }
	if (!strcmp(attrname,"annotation"))
	    {
		find_inf = stpLookup(inf->ParsedInf, "annotation");
		if (!find_inf) val->String="";
		else
		   {
		   /* if the annotation begins with a :, is data reference */
		   if (find_inf->StrVal[0][0] == ':')
			{
			ptr = find_inf->StrVal[0];
			if (!inf->MultiQueryObject) return -1;
	        	objGetAttrValue((void*)inf->MultiQueryObject, ptr+1, datatype, val);
			tmptype = objGetAttrType((void*)inf->MultiQueryObject, ptr+1);
			if (tmptype != DATA_T_STRING)
		    	    tstr = objDataToStringTmp(tmptype, val, 0);
			else
		    	    tstr = val->String;
			inf->Name = (pXString)nmMalloc(sizeof(XString));
			xsInit(inf->Name);
			xsConcatenate(inf->Name,tstr,-1);
	   		val->String = inf->Name->String;
	    		return 0;
			}
		   else
			{
			val->String = find_inf->StrVal[0];
	    		return 0;
			}
		   
		   }
	    	return 1;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (inf->Type == QY_T_LIST) *((char**)val) = "system/void";
	    else if (inf->Type == QY_T_ITEM) *((char**)val) = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (inf->Type == QY_T_LIST) *((char**)val) = "system/query";
	    else if (inf->Type == QY_T_ITEM) *((char**)val) = "system/row";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/
	if (inf->Type == QY_T_ITEM)
	    {
	    /** Pass the attribute question to the multiquery **/
	    return objGetAttrValue((void*)inf->MultiQueryObject, attrname, datatype, val);
	    }
	if (inf->Type == QY_T_LIST)
	    {
	    find_inf = stpLookup(inf->ParsedInf,attrname);
	    return stpAttrValue(find_inf,inf->Obj,&val->Integer,&val->String,0);
/*	    find_inf = stpLookup(inf->ParsedInf,attrname);
	    if (find_inf)
		{
                if (find_inf->StrVal[0] != NULL && find_inf->nVal > 1)
                    {
                    inf->SVvalue.Strings = find_inf->StrVal;
                    inf->SVvalue.nStrings = find_inf->nVal;
                    *(pStringVec*)val = &(inf->SVvalue);
                    }
                else if (find_inf->nVal > 1)
                    {
                    inf->IVvalue.Integers = find_inf->IntVal;
                    inf->IVvalue.nIntegers = find_inf->nVal;
                    *(pIntVec*)val = &(inf->IVvalue);
                    }
                else if (find_inf->StrVal[0] != NULL) *(char**)val = find_inf->StrVal[0];
                else *(int*)val = find_inf->IntVal[0];
                return 0;
		}
*/
	    }

	mssError(1,"QY","Could not locate requested attribute");

    return -1;
    }


/*** QyGetNextAttr - get the next attribute name for this object.
 ***/
char*
QyGetNextAttr(pQyData inf, pObjTrxTree oxt)
    {
    char* ptr;

	if (inf->Type == QY_T_LIST)
	    {
	    /** do not display annotation, sql, primarykey, or version **/
	    while(1)
		{
	    	ptr = stpGetNextAttr(inf->ParsedInf);
		if (ptr == NULL) return ptr;
		if ((strcmp(ptr,"sql")) && (strcmp(ptr,"primarykey")) &&
		    (strcmp(ptr,"annotation")) && (strcmp(ptr,"version")))
	    		return ptr;
		}
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    ptr = objGetNextAttr((void*)inf->MultiQueryObject);
	    return ptr;
	    }
        /** No find? **/
    return NULL;
    }


/*** QyGetFirstAttr - get the first attribute name for this object.
 ***/
char*
QyGetFirstAttr(pQyData inf, pObjTrxTree oxt)
    {
    char* ptr;
	if (inf->Type == QY_T_LIST)
	    {
	    ptr = stpGetFirstAttr(inf->ParsedInf);
	    if (ptr == NULL) return ptr;
	    if ((strcmp(ptr,"sql")) && (strcmp(ptr,"primarykey")) &&
	        (strcmp(ptr,"annotation")) && (strcmp(ptr,"version")))
	    	    return ptr;
	    while(1)
		{
	    	ptr = stpGetNextAttr(inf->ParsedInf);
		if (ptr == NULL) return ptr;
		if ((strcmp(ptr,"sql")) && (strcmp(ptr,"primarykey")) &&
		    (strcmp(ptr,"annotation")) && (strcmp(ptr,"version")))
	    		return ptr;
		}
	    }
	if (inf->Type == QY_T_ITEM)
	    {
	    ptr = objGetFirstAttr((void*)inf->MultiQueryObject);
	    return ptr;
	    }
    return ;
    }


/*** QySetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
QySetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree oxt)
    {
    pQyData inf = QY(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"STP","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"STP","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/
	if (inf->Type == QY_T_LIST)
	    {
		/** deal with parameters **/
		stpSetAttrValue(inf->ParsedInf, attrname, val);
		return 0;
	    }
	if (inf->Type == QY_T_ITEM)
	    {
		objSetAttrValue(inf->MultiQueryObject, attrname, datatype, val);
		return 0;
	    }
	return -1;
    }


/*** QyAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some.
 ***/
int
QyAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pQyData inf = QY(inf_v);
    pStructInf new_inf;
    char* ptr;
    /** Pass on to Multiquery **/
    return objAddAttr(inf->MultiQueryObject, attrname, type, val);	
    }


/*** QyOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
QyOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** QyGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
QyGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** QyGetNextMethod -- same as above.  Always fails. 
 ***/
char*
QyGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** QyExecuteMethod - No methods to execute, so this fails.
 ***/
int
QyExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** QyInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
QyInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&Qy_INF,0,sizeof(Qy_INF));
	Qy_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"QY -  Query Driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/query");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = QyOpen;
	drv->Close = QyClose;
	drv->Create = QyCreate;
	drv->Delete = QyDelete;
	drv->OpenQuery = QyOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = QyQueryFetch;
	drv->QueryClose = QyQueryClose;
	drv->Read = QyRead;
	drv->Write = QyWrite;
	drv->GetAttrType = QyGetAttrType;
	drv->GetAttrValue = QyGetAttrValue;
	drv->GetFirstAttr = QyGetFirstAttr;
	drv->GetNextAttr = QyGetNextAttr;
	drv->SetAttrValue = QySetAttrValue;
	drv->AddAttr = QyAddAttr;
	drv->OpenAttr = QyOpenAttr;
	drv->GetFirstMethod = QyGetFirstMethod;
	drv->GetNextMethod = QyGetNextMethod;
	drv->ExecuteMethod = QyExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(QyData),"QyData");
	nmRegister(sizeof(QyQuery),"QyQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

